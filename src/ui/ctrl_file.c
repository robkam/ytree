/***************************************************************************
 *
 * src/ui/ctrl_file.c
 * File Window Controller (Input & Event Handling)
 *
 ***************************************************************************/

#define NO_YTREE_MACROS

#ifndef YTREE_H
#include "../../include/ytree.h"
#endif

#include "../../include/sort.h"
#include "../../include/watcher.h"
#include "../../include/ytree_ui.h"
#include <libgen.h>

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#if !defined(__NeXT__) && !defined(ultrix)
#endif /* __NeXT__ ultrix */

/* Local Statics for metrics calculation - Pushed to Renderer */
static int max_disp_files;
static int x_step;
static int my_x_step;
/* static int  hide_left; */ /* Removed: Unused */
static int hide_right;

/* Metrics statics moved to file_list.c */

static long preview_line_offset = 0;
static int saved_fixed_width = 0;

/* --- Forward Declarations --- */
/* file_list.c: ReadFileList, ReadGlobalFileList, BuildFileEntryList,
 *              FreeFileEntryList, InvalidateVolumePanels, DisplayFileWindow */
static void RereadWindowSize(ViewContext *ctx, DirEntry *dir_entry);
static void ListJump(ViewContext *ctx, DirEntry *dir_entry, char *str);
static void UpdatePreview(ViewContext *ctx, DirEntry *dir_entry);

/*
 * UpdateStatsPanel
 * Centralized stats panel update that shows the correct information based on
 * context.
 * - In file window mode: Shows stats for currently selected file
 * - In directory mode: Shows stats for current directory
 */
void UpdateStatsPanel(ViewContext *ctx, DirEntry *dir_entry, Statistic *s) {
  /* Safety checks */
  if (!ctx || !dir_entry || !s)
    return;

  /* Check if we're in file window with files available */
  if (ctx->focused_window == FOCUS_FILE && ctx->active->file_count > 0) {
    /* File Window Mode - show individual file stats */
    int file_index = dir_entry->start_file + dir_entry->cursor_pos;
    FileEntry *fe_ptr = ctx->active->file_entry_list[file_index].file;
    if (fe_ptr) {
      DisplayFileStatistic(ctx, fe_ptr, s);

      /* Also update attributes section */
      if (dir_entry->global_flag)
        DisplayGlobalFileParameter(ctx, fe_ptr);
      else
        DisplayFileParameter(ctx, fe_ptr);
    }
  } else {
    /* Directory Mode - show directory stats */
    if (dir_entry->global_flag) {
      DisplayDiskStatistic(ctx, s);
      DisplayDirStatistic(ctx, dir_entry, "SHOW ALL", s);
    } else {
      DisplayDirStatistic(ctx, dir_entry, NULL, s);
    }
    /* Show directory attributes section */
    DisplayDirParameter(ctx, dir_entry);
  }

  /* Stage the stats panel for refresh */
  wnoutrefresh(ctx->ctx_border_window);
}

int ChangeFileOwner(ViewContext *ctx, FileEntry *fe_ptr) {
  int owner_id;
  char filepath[PATH_LENGTH + 1];

  if ((owner_id = GetNewOwner(ctx, fe_ptr->stat_struct.st_uid)) == -1)
    return -1;

  GetFileNamePath(fe_ptr, filepath);
  if (chown(filepath, owner_id, (gid_t)-1)) {
    UI_Message(ctx, "chown failed!*\"%s\"*%s", filepath, strerror(errno));
    return -1;
  }

  fe_ptr->stat_struct.st_uid = owner_id;
  return 0;
}

int ChangeFileGroup(ViewContext *ctx, FileEntry *fe_ptr) {
  int group_id;
  char filepath[PATH_LENGTH + 1];

  if ((group_id = GetNewGroup(ctx, fe_ptr->stat_struct.st_gid)) == -1)
    return -1;

  GetFileNamePath(fe_ptr, filepath);
  if (chown(filepath, (uid_t)-1, group_id)) {
    UI_Message(ctx, "chgrp failed!*\"%s\"*%s", filepath, strerror(errno));
    return -1;
  }

  fe_ptr->stat_struct.st_gid = group_id;
  return 0;
}

static void fmovedown(ViewContext *ctx, int *start_file, int *cursor_pos,
                      int *start_x, DirEntry *dir_entry) {
  Nav_MoveDown(cursor_pos, start_file, (int)ctx->active->file_count,
               max_disp_files, 1);

  DisplayFiles(ctx, ctx->active, dir_entry, *start_file,
               *start_file + *cursor_pos, *start_x,
               ctx->ctx_file_window); /* Update dynamic header path */
  {
    char path[PATH_LENGTH];
    GetPath(dir_entry, path);
    DisplayHeaderPath(ctx, path);
  }
  ctx->active->start_file = *start_file;
}

static void fmoveup(ViewContext *ctx, int *start_file, int *cursor_pos,
                    int *start_x, DirEntry *dir_entry) {
  Nav_MoveUp(cursor_pos, start_file);

  DisplayFiles(ctx, ctx->active, dir_entry, *start_file,
               *start_file + *cursor_pos, *start_x,
               ctx->ctx_file_window); /* Update dynamic header path */
  {
    char path[PATH_LENGTH];
    GetPath(dir_entry, path);
    DisplayHeaderPath(ctx, path);
  }
  ctx->active->start_file = *start_file;
}

static void fmoveright(ViewContext *ctx, int *start_file, int *cursor_pos,
                       int *start_x, DirEntry *dir_entry) {
  if (x_step == 1) {
    /* Special case: scroll entire file window */
    /*------------------------*/
    (*start_x)++;
    DisplayFiles(ctx, ctx->active, dir_entry, *start_file,
                 *start_file + *cursor_pos, *start_x, ctx->ctx_file_window);
    if (hide_right <= 0)
      (*start_x)--;
  } else if ((unsigned int)(*start_file + *cursor_pos) >=
             ctx->active->file_count - 1) {
    /* last position reached */
    /*-------------------------*/
  } else {
    if ((unsigned int)(*start_file + *cursor_pos + x_step) >=
        ctx->active->file_count) {
      /* full step not possible;
       * position on last entry
       */
      my_x_step = (int)ctx->active->file_count - *start_file - *cursor_pos - 1;
    } else {
      my_x_step = x_step;
    }
    if (*cursor_pos + my_x_step < max_disp_files) {
      /* RIGHT possible without scrolling */
      /*----------------------------*/
      *cursor_pos += my_x_step;
    } else {
      /* Scrolling */
      /*----------*/
      *start_file += x_step;
      *cursor_pos -= x_step - my_x_step;
    }
    DisplayFiles(ctx, ctx->active, dir_entry, *start_file,
                 *start_file + *cursor_pos, *start_x, ctx->ctx_file_window);
  } /* Update dynamic header path */
  {
    char path[PATH_LENGTH];
    GetPath(dir_entry, path);
    DisplayHeaderPath(ctx, path);
  }
  return;
}

static void fmoveleft(ViewContext *ctx, int *start_file, int *cursor_pos,
                      int *start_x, DirEntry *dir_entry) {
  if (x_step == 1) {
    /* Special case: scroll entire file window */
    /*----------------------------------------*/
    if (*start_x > 0)
      (*start_x)--;
    DisplayFiles(ctx, ctx->active, dir_entry, *start_file,
                 *start_file + *cursor_pos, *start_x, ctx->ctx_file_window);
  } else if (*start_file + *cursor_pos <= 0) {
    /* first position reached */
    /*-------------------------*/
  } else {
    if (*start_file + *cursor_pos - x_step < 0) {
      /* full step not possible;
       * position on first entry
       */
      my_x_step = *start_file + *cursor_pos;
    } else {
      my_x_step = x_step;
    }
    if (*cursor_pos - my_x_step >= 0) {
      /* LEFT possible without scrolling */
      /*---------------------------*/
      *cursor_pos -= my_x_step;
    } else {
      /* Scrolling */
      /*----------*/
      if ((*start_file -= x_step) < 0)
        *start_file = 0;
    }
    DisplayFiles(ctx, ctx->active, dir_entry, *start_file,
                 *start_file + *cursor_pos, *start_x, ctx->ctx_file_window);
  } /* Update dynamic header path */
  {
    char path[PATH_LENGTH];
    GetPath(dir_entry, path);
    DisplayHeaderPath(ctx, path);
  }
  ctx->active->start_file = *start_file;
  return;
}

static void fmovenpage(ViewContext *ctx, int *start_file, int *cursor_pos,
                       int *start_x, DirEntry *dir_entry) {
  Nav_PageDown(cursor_pos, start_file, (int)ctx->active->file_count,
               max_disp_files);

  DisplayFiles(ctx, ctx->active, dir_entry, *start_file,
               *start_file + *cursor_pos, *start_x,
               ctx->ctx_file_window); /* Update dynamic header path */
  {
    char path[PATH_LENGTH];
    GetPath(dir_entry, path);
    DisplayHeaderPath(ctx, path);
  }
  ctx->active->start_file = *start_file;
}

static void fmoveppage(ViewContext *ctx, int *start_file, int *cursor_pos,
                       int *start_x, DirEntry *dir_entry) {
  Nav_PageUp(cursor_pos, start_file, max_disp_files);

  DisplayFiles(ctx, ctx->active, dir_entry, *start_file,
               *start_file + *cursor_pos, *start_x,
               ctx->ctx_file_window); /* Update dynamic header path */
  {
    char path[PATH_LENGTH];
    GetPath(dir_entry, path);
    DisplayHeaderPath(ctx, path);
  }
  ctx->active->start_file = *start_file;
}

/* Local helper to refresh file window, maintaining file cursor */
DirEntry *RefreshFileView(ViewContext *ctx, DirEntry *dir_entry) {
  char *saved_name = NULL;
  Statistic *s = &ctx->active->vol->vol_stats;
  int found_idx = -1;
  int start_x = 0;

  /* 1. Save current filename */
  if (ctx->active->file_count > 0) {
    /* Check bounds before access */
    if (dir_entry->start_file + dir_entry->cursor_pos <
        (int)ctx->active->file_count) {
      FileEntry *curr =
          ctx->active
              ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
              .file;
      if (curr)
        saved_name = strdup(curr->name);
    }
  }

  /* 2. Perform Safe Tree Refresh (Save/Rescan/Restore) */
  /* Update the local pointer with the valid address returned by RefreshTreeSafe
   */
  dir_entry = RefreshTreeSafe(ctx, ctx->active, dir_entry);

  /* 3. Rebuild File List from the refreshed tree */
  BuildFileEntryList(ctx, ctx->active);

  /* 4. Restore Cursor Position */
  if (saved_name) {
    int k;
    for (k = 0; k < (int)ctx->active->file_count; k++) {
      if (strcmp(ctx->active->file_entry_list[k].file->name, saved_name) == 0) {
        found_idx = k;
        break;
      }
    }
    free(saved_name);
  }

  if (found_idx != -1) {
    /* Calculate new start_file and cursor_pos */
    if (found_idx >= dir_entry->start_file &&
        found_idx < dir_entry->start_file + max_disp_files) {
      /* Still on screen, just move cursor */
      dir_entry->cursor_pos = found_idx - dir_entry->start_file;
    } else {
      /* Off screen, recenter or scroll */
      dir_entry->start_file = found_idx;
      dir_entry->cursor_pos = 0;
      /* Bounds check */
      if (dir_entry->start_file + max_disp_files >
          (int)ctx->active->file_count) {
        dir_entry->start_file =
            MAXIMUM(0, (int)ctx->active->file_count - max_disp_files);
        dir_entry->cursor_pos =
            (int)ctx->active->file_count - 1 - dir_entry->start_file;
      }
    }
  } else {
    /* File gone, reset to top */
    dir_entry->start_file = 0;
    dir_entry->cursor_pos = 0;
  }

  /* 5. Update Display */
  if (dir_entry->global_flag)
    DisplayDiskStatistic(ctx, s);
  else
    DisplayDirStatistic(ctx, dir_entry, NULL, s);

  DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
               dir_entry->start_file + dir_entry->cursor_pos, start_x,
               ctx->ctx_file_window);

  ctx->active->start_file = dir_entry->start_file;

  return dir_entry;
}

/* External declarations */

int HandleFileWindow(ViewContext *ctx, DirEntry *dir_entry) {
  DEBUG_LOG("HandleFileWindow ENTERED for %s", dir_entry->name);
  FileEntry *fe_ptr;
  FileEntry *new_fe_ptr;
  DirEntry *de_ptr = NULL;
  DirEntry *dest_dir_entry;
  int ch;
  int unput_char;
  int list_pos;
  int start_x = 0;
  char filepath[PATH_LENGTH + 1];
  BOOL path_copy;
  int term;
  static char to_dir[PATH_LENGTH + 1];
  static char to_path[PATH_LENGTH + 1];
  static char to_file[PATH_LENGTH + 1];
  BOOL need_dsp_help;
  BOOL maybe_change_x_step;
  char new_name[PATH_LENGTH + 1];
  char expanded_new_name[PATH_LENGTH + 1];
  char expanded_to_file[PATH_LENGTH + 1];
  char new_login_path[PATH_LENGTH + 1];
  int get_dir_ret;
  YtreeAction action = ACTION_NONE;            /* Initialize action */
  DirEntry *last_stats_dir = NULL;             /* Track context changes */
  struct Volume *start_vol = ctx->active->vol; /* Safety Check Variable */
  Statistic *s = &ctx->active->vol->vol_stats;
  char watcher_path[PATH_LENGTH + 1];

  /* ADDED INSTRUCTION: Focus Unification */
  ctx->focused_window = FOCUS_FILE;

  unput_char = '\0';
  fe_ptr = NULL;

  /* Reset cursor position flags */
  /*--------------------------------------*/

  need_dsp_help = TRUE;
  maybe_change_x_step = TRUE;

  BuildFileEntryList(ctx, ctx->active);

  /* Sanitize cursor position immediately after BuildFileEntryList */
  if (ctx->active->file_count > 0) {
    if (dir_entry->cursor_pos < 0)
      dir_entry->cursor_pos = 0;
    if (dir_entry->start_file < 0)
      dir_entry->start_file = 0;

    /* Bounds Check: ensure we aren't past the end */
    if ((unsigned int)(dir_entry->start_file + dir_entry->cursor_pos) >=
        ctx->active->file_count) {
      dir_entry->start_file =
          MAXIMUM(0, (int)ctx->active->file_count - max_disp_files);
      dir_entry->cursor_pos =
          (int)ctx->active->file_count - 1 - dir_entry->start_file;
    }
  } else {
    dir_entry->cursor_pos = 0;
    dir_entry->start_file = 0;
  }
  ctx->active->start_file = dir_entry->start_file;

  /* Initial Display using Centralized Function if applicable */
  if (ctx->preview_mode) {
    RefreshView(ctx, dir_entry);
    UpdatePreview(ctx, dir_entry);
    /* Note: preview mode doesn't show stats panel, so no UpdateStatsPanel
     * needed */
  } else {
    /* Standard Logic for Big/Small Window */
    if (dir_entry->global_flag || dir_entry->big_window ||
        dir_entry->tagged_flag) {
      SwitchToBigFileWindow(ctx);
    }

    /* Show initial stats (UpdateStatsPanel handles both file and dir stats) */
    UpdateStatsPanel(ctx, dir_entry, s);

    DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                 dir_entry->start_file + dir_entry->cursor_pos, start_x,
                 ctx->ctx_file_window);
  }

  if (s->login_mode == DISK_MODE || s->login_mode == USER_MODE) {
    GetPath(dir_entry, watcher_path);
    Watcher_SetDir(ctx, watcher_path);
  }

  do {
    /* Critical Safety: If volume was deleted (e.g. via K menu), abort
     * immediately */
    if (ctx->active->vol != start_vol)
      return ESC;

    if (maybe_change_x_step) {
      maybe_change_x_step = FALSE;

      x_step = (GetPanelMaxColumn(ctx->active) > 1)
                   ? getmaxy(ctx->ctx_file_window)
                   : 1;
      max_disp_files =
          getmaxy(ctx->ctx_file_window) * GetPanelMaxColumn(ctx->active);
    }

    if (unput_char) {
      ch = unput_char;
      unput_char = '\0';
    } else {
      /* Guard fe_ptr access against empty file list */
      if (ctx->active->file_count > 0) {
        fe_ptr =
            ctx->active
                ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
                .file;

        /* Dynamic Stats Update: Check if context changed */
        if (fe_ptr && fe_ptr->dir_entry != last_stats_dir) {
          last_stats_dir = fe_ptr->dir_entry;
          /* Pass "SHOW ALL" if global flag is set, otherwise NULL for default
           */
          DisplayDirStatistic(ctx, last_stats_dir,
                              (dir_entry->global_flag) ? "SHOW ALL" : NULL, s);
        }
      } else {
        fe_ptr = NULL;
      }

      if (dir_entry->global_flag)
        DisplayGlobalFileParameter(ctx, fe_ptr);
      else
        DisplayFileParameter(ctx, fe_ptr);

      RefreshView(ctx, dir_entry);
      ch = (ctx->resize_request) ? -1 : GetEventOrKey(ctx);
      if (ch == LF)
        ch = CR;
    }

    /* Re-check safety after blocking Getch */
    if (ctx->active->vol != start_vol)
      return ESC;

    if (IsUserActionDefined(ctx)) { /* User commands take precedence */
      ch = FileUserMode(ctx,
                        &(ctx->active->file_entry_list[dir_entry->start_file +
                                                       dir_entry->cursor_pos]),
                        ch, &ctx->active->vol->vol_stats);
      if (ctx->active->vol != start_vol)
        return ESC;
    }

    if (ctx->resize_request) {
      /* Simplified Resize using Centralized Function */
      RereadWindowSize(ctx, dir_entry);
      RefreshView(ctx, dir_entry);
      if (ctx->preview_mode)
        UpdatePreview(ctx, dir_entry);
      need_dsp_help = TRUE;
      ctx->resize_request = FALSE;
    }

    action = GetKeyAction(ctx, ch);

    /* Special remapping for MODE_1: TAB/BTAB act as UP/DOWN */
    if (GetPanelFileMode(ctx->active) == MODE_1) {
      if (action == ACTION_TREE_EXPAND)
        action = ACTION_MOVE_DOWN;
      else if (action == ACTION_TREE_COLLAPSE)
        action = ACTION_MOVE_UP;
    }

    if (x_step == 1 &&
        (action == ACTION_MOVE_RIGHT || action == ACTION_MOVE_LEFT)) {
      /* do not reset start_x */
      /*-----------------------------*/

      ; /* do nothing */
    } else {
      /* start at 0 */
      /*----------------*/

      if (start_x) {
        start_x = 0;

        DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                     dir_entry->start_file + dir_entry->cursor_pos, start_x,
                     ctx->ctx_file_window);
      }
    }

    switch (action) {

    case ACTION_CMD_TAGGED_A:
    case ACTION_CMD_TAGGED_O:
    case ACTION_CMD_TAGGED_G:
    case ACTION_TAG:
    case ACTION_UNTAG:
    case ACTION_TAG_ALL:
    case ACTION_UNTAG_ALL:
    case ACTION_TAG_REST:
    case ACTION_UNTAG_REST:
    case ACTION_CMD_TAGGED_V:
    case ACTION_CMD_TAGGED_Y:
    case ACTION_CMD_TAGGED_C:
    case ACTION_CMD_TAGGED_M:
    case ACTION_CMD_TAGGED_D:
    case ACTION_CMD_TAGGED_R:
    case ACTION_CMD_TAGGED_P:
    case ACTION_CMD_TAGGED_S:
    case ACTION_CMD_TAGGED_X:
    case ACTION_TOGGLE_TAGGED_MODE:
    case ACTION_ASTERISK:
      if (handle_tag_file_action(ctx, action, dir_entry, &unput_char,
                                 &need_dsp_help, start_x, s,
                                 &maybe_change_x_step)) {
        break;
      }
      break;

    case ACTION_NONE:
      break;

    case ACTION_VIEW_PREVIEW:
      if (!ctx->preview_mode) {
        /* Turning ON */
        /* INSTRUCTION: Before any redirection, save the current state */
        ctx->preview_return_panel = ctx->active;
        ctx->preview_return_focus = ctx->focused_window;
      }

      ctx->preview_mode = !ctx->preview_mode;

      if (ctx->preview_mode) {
        /* Turning ON */
        /* 1. Save current width setting */
        saved_fixed_width = ctx->fixed_col_width;

        /* 2. Update Window Layout immediately to get new dims */
        ReCreateWindows(ctx);

        /* 3. Force Compact Mode based on new width (width - 2 for
         * borders/padding) */
        /* Accessing ctx->layout.big_file_win_width from init.c logic */
        ctx->fixed_col_width = ctx->layout.big_file_win_width - 2;

        /* 4. Update scrolling metrics (max_column, etc) based on new width/mode
         */
        RereadWindowSize(ctx, dir_entry);
      } else {
        /* Turning OFF */
        /* INSTRUCTION: Restore state */
        ctx->active = ctx->preview_return_panel;
        ctx->active->vol = ctx->active->vol;
        ctx->focused_window = ctx->preview_return_focus;

        /* CRITICAL: Update local context variables for the loop if we stay in
         * File Window */
        s = &ctx->active->vol->vol_stats;
        start_vol = ctx->active->vol;

        /* Refresh dir_entry for the restored panel */
        if (ctx->active->vol->total_dirs > 0) {
          int idx = ctx->active->disp_begin_pos + ctx->active->cursor_pos;
          if (idx >= ctx->active->vol->total_dirs)
            idx = ctx->active->vol->total_dirs - 1;
          if (idx < 0)
            idx = 0;
          dir_entry = ctx->active->vol->dir_entry_list[idx].dir_entry;
        } else {
          dir_entry = s->tree;
        }

        /* Restore Global Context Pointers */
        ctx->ctx_dir_window = ctx->active->pan_dir_window;
        ctx->ctx_small_file_window = ctx->active->pan_small_file_window;
        ctx->ctx_big_file_window = ctx->active->pan_big_file_window;
        ctx->ctx_file_window = ctx->active->pan_file_window;

        /* 1. Restore width setting */
        ctx->fixed_col_width = saved_fixed_width;

        /* 2. Update Layout */
        ReCreateWindows(ctx);

        /* 3. Update metrics */
        RereadWindowSize(ctx, dir_entry);

        /* Call RefreshView immediately after restoration */
        RefreshView(ctx, dir_entry);
      }

      /* 5. Draw Everything (Borders, Tree, Stats, List, Preview) */
      RefreshView(ctx, dir_entry);

      if (ctx->preview_mode) {
        UpdatePreview(ctx, dir_entry);
      }

      /* ADDED INSTRUCTION: Conditional Exit Logic */
      if (!ctx->preview_mode) {
        if (ctx->preview_return_focus == FOCUS_TREE) {
          action = ACTION_ESCAPE;
        } else {
          /* We came from a File Window, stay here */
          action = ACTION_NONE;
        }
      }

      need_dsp_help = TRUE;
      break;

    case ACTION_PREVIEW_SCROLL_DOWN:
      if (ctx->preview_mode) {
        preview_line_offset++;
        UpdatePreview(ctx, dir_entry);
      }
      break;

    case ACTION_PREVIEW_SCROLL_UP:
      if (ctx->preview_mode) {
        if (preview_line_offset > 0)
          preview_line_offset--;
        UpdatePreview(ctx, dir_entry);
      }
      break;

    case ACTION_PREVIEW_HOME:
      if (ctx->preview_mode) {
        preview_line_offset = 0;
        UpdatePreview(ctx, dir_entry);
      }
      break;
    case ACTION_PREVIEW_END:
      if (ctx->preview_mode) {
        /* Set to a large value, renderer handles EOF */
        preview_line_offset = 2000000000L;
        UpdatePreview(ctx, dir_entry);
      }
      break;
    case ACTION_PREVIEW_PAGE_UP:
      if (ctx->preview_mode) {
        preview_line_offset -= (getmaxy(ctx->ctx_preview_window) - 1);
        if (preview_line_offset < 0)
          preview_line_offset = 0;
        UpdatePreview(ctx, dir_entry);
      }
      break;
    case ACTION_PREVIEW_PAGE_DOWN:
      if (ctx->preview_mode) {
        preview_line_offset += (getmaxy(ctx->ctx_preview_window) - 1);
        UpdatePreview(ctx, dir_entry);
      }
      break;

    case ACTION_SPLIT_SCREEN:
      ctx->is_split_screen = !ctx->is_split_screen;
      ReCreateWindows(ctx); /* Force layout update immediately */

      if (ctx->is_split_screen) {
        if (ctx->right && ctx->left) {
          ctx->right->vol = ctx->left->vol;
          ctx->right->cursor_pos = ctx->left->cursor_pos;
          ctx->right->disp_begin_pos = ctx->left->disp_begin_pos;
        }
      } else {
        ctx->active = ctx->left;
      }

      return ESC; /* Return to DirWindow to redraw */

    case ACTION_SWITCH_PANEL:
      if (!ctx->is_split_screen)
        break;
      /* Ensure the CURRENT panel is cleaned up before switching context. */
      SwitchToSmallFileWindow(ctx);

      /* Switch Panel */
      if (ctx->active == ctx->left) {
        ctx->active = ctx->right;
      } else {
        ctx->active = ctx->left;
      }
      /* Update Volume Context */
      ctx->active->vol = ctx->active->vol;

      /* Bug 3 Fix: We trigger a loop exit here.
       * This ensures the cleanup code at the end of HandleFileWindow runs,
       * restoring the small window layout before we return to HandleDirWindow.
       */
      action = ACTION_ESCAPE;
      break;

    case ACTION_MOVE_DOWN:
      fmovedown(ctx, &dir_entry->start_file, &dir_entry->cursor_pos, &start_x,
                dir_entry);
      if (ctx->preview_mode) {
        preview_line_offset = 0;
        UpdatePreview(ctx, dir_entry);
      }
      break;

    case ACTION_MOVE_UP:
      fmoveup(ctx, &dir_entry->start_file, &dir_entry->cursor_pos, &start_x,
              dir_entry);
      if (ctx->preview_mode) {
        preview_line_offset = 0;
        UpdatePreview(ctx, dir_entry);
      }
      break;

    case ACTION_MOVE_RIGHT:
      if (!ctx->highlight_full_line && x_step == 1)
        break; /* No horizontal scroll in name-only mode */
      fmoveright(ctx, &dir_entry->start_file, &dir_entry->cursor_pos, &start_x,
                 dir_entry);
      if (ctx->preview_mode) {
        preview_line_offset = 0;
        UpdatePreview(ctx, dir_entry);
      }
      break;

    case ACTION_MOVE_LEFT:
      if (!ctx->highlight_full_line && x_step == 1)
        break; /* No horizontal scroll in name-only mode */
      fmoveleft(ctx, &dir_entry->start_file, &dir_entry->cursor_pos, &start_x,
                dir_entry);
      if (ctx->preview_mode) {
        preview_line_offset = 0;
        UpdatePreview(ctx, dir_entry);
      }
      break;

    case ACTION_PAGE_DOWN:
      fmovenpage(ctx, &dir_entry->start_file, &dir_entry->cursor_pos, &start_x,
                 dir_entry);
      if (ctx->preview_mode) {
        preview_line_offset = 0;
        UpdatePreview(ctx, dir_entry);
      }
      break;

    case ACTION_PAGE_UP:
      fmoveppage(ctx, &dir_entry->start_file, &dir_entry->cursor_pos, &start_x,
                 dir_entry);
      if (ctx->preview_mode) {
        preview_line_offset = 0;
        UpdatePreview(ctx, dir_entry);
      }
      break;

    case ACTION_END:
      Nav_End(&dir_entry->cursor_pos, &dir_entry->start_file,
              (int)ctx->active->file_count, max_disp_files);

      DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                   dir_entry->start_file + dir_entry->cursor_pos, start_x,
                   ctx->ctx_file_window);
      if (ctx->preview_mode) {
        preview_line_offset = 0;
        UpdatePreview(ctx, dir_entry);
      }
      ctx->active->start_file = dir_entry->start_file;
      break;

    case ACTION_HOME:
      Nav_Home(&dir_entry->cursor_pos, &dir_entry->start_file);

      DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                   dir_entry->start_file + dir_entry->cursor_pos, start_x,
                   ctx->ctx_file_window);
      if (ctx->preview_mode) {
        preview_line_offset = 0;
        UpdatePreview(ctx, dir_entry);
      }
      ctx->active->start_file = dir_entry->start_file;
      break;

    case ACTION_TOGGLE_HIDDEN: {
      ToggleDotFiles(ctx, ctx->active);

      /* Update current dir pointer using the new accessor function */
      dir_entry = GetPanelDirEntry(ctx->active);

      /* Explicitly update the file window (preview) */
      DisplayFileWindow(ctx, ctx->active, dir_entry);
      RefreshWindow(ctx->ctx_file_window);
      if (ctx->preview_mode) {
        preview_line_offset = 0;
        UpdatePreview(ctx, dir_entry);
      }

      need_dsp_help = TRUE;
    } break;

    case ACTION_CMD_A:
      fe_ptr =
          ctx->active
              ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
              .file;

      need_dsp_help = TRUE;

      if (!ChangeFileModus(ctx, fe_ptr)) {
        DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                     dir_entry->start_file + dir_entry->cursor_pos, start_x,
                     ctx->ctx_file_window);
      }
      break;

    case ACTION_CMD_O:
      fe_ptr =
          ctx->active
              ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
              .file;

      need_dsp_help = TRUE;

      if (!ChangeFileOwner(ctx, fe_ptr)) {
        DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                     dir_entry->start_file + dir_entry->cursor_pos, start_x,
                     ctx->ctx_file_window);
      }
      break;

    case ACTION_CMD_G:
      fe_ptr =
          ctx->active
              ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
              .file;

      need_dsp_help = TRUE;

      if (!ChangeFileGroup(ctx, fe_ptr)) {
        DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                     dir_entry->start_file + dir_entry->cursor_pos, start_x,
                     ctx->ctx_file_window);
      }
      break;

    case ACTION_TOGGLE_MODE:
      if (ctx->preview_mode) {
        beep();
        break;
      }
      list_pos = dir_entry->start_file + dir_entry->cursor_pos;

      RotatePanelFileMode(ctx, ctx->active);
      x_step = (GetPanelMaxColumn(ctx->active) > 1)
                   ? getmaxy(ctx->ctx_file_window)
                   : 1;
      max_disp_files =
          getmaxy(ctx->ctx_file_window) * GetPanelMaxColumn(ctx->active);

      if (dir_entry->cursor_pos >= max_disp_files) {
        /* Cursor must be repositioned */
        /*-------------------------------------*/

        dir_entry->cursor_pos = max_disp_files - 1;
      }

      dir_entry->start_file = list_pos - dir_entry->cursor_pos;
      DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                   dir_entry->start_file + dir_entry->cursor_pos, start_x,
                   ctx->ctx_file_window);
      break;

    case ACTION_CMD_V:
      fe_ptr =
          ctx->active
              ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
              .file;
      if (ctx->view_mode == ARCHIVE_MODE) {
        if (View(ctx, dir_entry, fe_ptr->name) != 0) {
          /* Error message already handled in View */
        }
      } else {
        char full_path[PATH_LENGTH + 1];
        GetFileNamePath(fe_ptr, full_path);
        if (View(ctx, dir_entry, full_path) != 0) {
          /* Error message already handled in View */
        }
      }
      break;

    case ACTION_CMD_H:
      fe_ptr =
          ctx->active
              ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
              .file;
      de_ptr = fe_ptr->dir_entry;
      (void)GetRealFileNamePath(fe_ptr, filepath, ctx->view_mode);
      (void)ViewHex(ctx, filepath);
      need_dsp_help = TRUE;
      break;

    case ACTION_CMD_E:
      fe_ptr =
          ctx->active
              ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
              .file;
      de_ptr = fe_ptr->dir_entry;
      (void)GetFileNamePath(fe_ptr, filepath);
      (void)Edit(ctx, de_ptr, filepath);
      break;

    case ACTION_CMD_Y:
    case ACTION_CMD_C:
      fe_ptr =
          ctx->active
              ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
              .file;
      de_ptr = fe_ptr->dir_entry;

      path_copy = FALSE;
      if (action == ACTION_CMD_Y)
        path_copy = TRUE;

      need_dsp_help = TRUE;

      if (GetCopyParameter(ctx, fe_ptr->name, path_copy, to_file, to_dir)) {
        break;
      }

      if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
        if (realpath(to_dir, to_path) == NULL) {
          if (errno == ENOENT) {
            strcpy(to_path, to_dir);
          } else {
            MESSAGE(ctx, "Invalid destination path*\"%s\"*%s", to_dir,
                    strerror(errno));
            break;
          }
        }
        dest_dir_entry = NULL;
      } else {
        get_dir_ret =
            GetDirEntry(ctx, s->tree, de_ptr, to_dir, &dest_dir_entry, to_path);
        if (get_dir_ret == -1) { /* System error */
          break;
        }
        if (get_dir_ret == -3) { /* Directory not found, proceed */
          dest_dir_entry = NULL;
        }
      }

      /* EXPAND WILDCARDS FOR SINGLE FILE COPY */
      BuildFilename(fe_ptr->name, to_file, expanded_to_file);

      {
        int dir_create_mode = 0; /* Local mode for single file op */
        int overwrite_mode = 0;  /* Local mode for single file op */
        CopyFile(ctx, s, fe_ptr, expanded_to_file, dest_dir_entry, to_path,
                 path_copy, &dir_create_mode, &overwrite_mode,
                 (ConflictCallback)UI_ConflictResolverWrapper,
                 (ChoiceCallback)UI_ChoiceResolver);
      }

      RefreshDirWindow(ctx, ctx->active);
      if (ctx->is_split_screen) {
        RefreshDirWindow(ctx,
                         (ctx->active == ctx->left) ? ctx->right : ctx->left);
      }

      RefreshView(ctx, dir_entry);

      need_dsp_help = TRUE;
      break;

    case ACTION_CMD_MKFILE:
      if (ctx->view_mode != DISK_MODE)
        break;

      {
        char file_name[PATH_LENGTH * 2 + 1];
        int mk_result;

        ClearHelp(ctx);
        *file_name = '\0';
        if (UI_ReadString(ctx, ctx->active, "MAKE FILE:", file_name,
                          PATH_LENGTH, HST_FILE) == CR) {
          mk_result = MakeFile(ctx, dir_entry, file_name, s, NULL,
                               (ChoiceCallback)UI_ChoiceResolver);
          if (mk_result == 0) {
            /* If successful, refresh the view */
            BuildFileEntryList(ctx, ctx->active);
            RefreshView(ctx, dir_entry);
          } else if (mk_result == 1) {
            MESSAGE(ctx, "File already exists!");
          } else {
            MESSAGE(ctx, "Can't create File*\"%s\"", file_name);
          }
        }
      }
      need_dsp_help = TRUE;
      break;

    case ACTION_CMD_M:
      if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
        break;
      }

      fe_ptr =
          ctx->active
              ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
              .file;
      de_ptr = fe_ptr->dir_entry;

      need_dsp_help = TRUE;

      if (GetMoveParameter(ctx, fe_ptr->name, to_file, to_dir)) {
        break;
      }

      get_dir_ret =
          GetDirEntry(ctx, s->tree, de_ptr, to_dir, &dest_dir_entry, to_path);
      if (get_dir_ret == -1) {
        break;
      }
      if (get_dir_ret == -3) {
        dest_dir_entry = NULL;
      }

      /* Construct absolute path for checking */
      {
        char abs_check_path[PATH_LENGTH * 2 + 2];
        char current_dir[PATH_LENGTH + 1];
        BOOL created = FALSE;
        int dir_create_mode = 0;

        if (*to_dir == FILE_SEPARATOR_CHAR) {
          strcpy(abs_check_path, to_dir);
        } else {
          GetPath(de_ptr, current_dir);
          snprintf(abs_check_path, sizeof(abs_check_path), "%s%c%s",
                   current_dir, FILE_SEPARATOR_CHAR, to_dir);
        }
        /* FIX: Pass &dest_dir_entry */
        if (EnsureDirectoryExists(ctx, abs_check_path, s->tree, &created,
                                  &dest_dir_entry, &dir_create_mode,
                                  (ChoiceCallback)UI_ChoiceResolver) == -1)
          break;
      }

      /* EXPAND WILDCARDS FOR SINGLE FILE MOVE */
      BuildFilename(fe_ptr->name, to_file, expanded_to_file);

      {
        int dir_create_mode = 0;
        int overwrite_mode = 0;
        if (!MoveFile(ctx, fe_ptr, expanded_to_file, dest_dir_entry, to_path,
                      &new_fe_ptr, &dir_create_mode, &overwrite_mode,
                      (ConflictCallback)UI_ConflictResolverWrapper,
                      (ChoiceCallback)UI_ChoiceResolver)) {
          /* File was moved */
          /*-------------------*/

          /* ... Stats updates ... */
          /* ... BuildFileEntryList ... */

          RefreshView(ctx, dir_entry);

          RefreshDirWindow(ctx, ctx->active);
          if (ctx->is_split_screen) {
            RefreshDirWindow(ctx, (ctx->active == ctx->left) ? ctx->right
                                                             : ctx->left);
          }

          maybe_change_x_step = TRUE;
        }
      }
      need_dsp_help = TRUE;
      break;

    case ACTION_CMD_D:
      if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE &&
          ctx->view_mode != ARCHIVE_MODE) {
        break;
      }

      term = InputChoice(ctx, "Delete this file (Y/N) ? ", "YN\033");

      need_dsp_help = TRUE;

      if (term != 'Y')
        break;

      fe_ptr =
          ctx->active
              ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
              .file;
      de_ptr = fe_ptr->dir_entry;

      {
        int override_mode = 0;
        if (!DeleteFile(ctx, fe_ptr, &override_mode, s,
                        (ChoiceCallback)UI_ChoiceResolver)) {
          /* File was deleted */
          /*----------------------*/

          RefreshView(ctx, dir_entry);
          maybe_change_x_step = TRUE;
        }
      }
      break;

    case ACTION_CMD_R:
      if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE &&
          ctx->view_mode != ARCHIVE_MODE) {
        break;
      }

      fe_ptr =
          ctx->active
              ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
              .file;
      de_ptr = fe_ptr->dir_entry;

      if (!GetRenameParameter(ctx, fe_ptr->name, new_name)) {
        /* EXPAND WILDCARDS FOR SINGLE FILE RENAME */
        BuildFilename(fe_ptr->name, new_name, expanded_new_name);

        if (!RenameFile(ctx, fe_ptr, expanded_new_name, &new_fe_ptr)) {
          /* Rename OK */
          /*-----------*/

          RefreshView(ctx, dir_entry);
          maybe_change_x_step = TRUE;
        }
      }
      need_dsp_help = TRUE;
      break;

    case ACTION_CMD_S:
      UI_HandleSort(ctx, dir_entry, s, start_x);
      need_dsp_help = TRUE;
      break;

    case ACTION_FILTER:
      if (UI_ReadFilter(ctx) == 0) {

        dir_entry->start_file = 0;
        dir_entry->cursor_pos = 0;

        BuildFileEntryList(ctx, ctx->active);

        DisplayFilter(ctx, s);
        DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                     dir_entry->start_file + dir_entry->cursor_pos, start_x,
                     ctx->ctx_file_window);

        if (dir_entry->global_flag)
          DisplayDiskStatistic(ctx, s);
        else
          DisplayDirStatistic(ctx, dir_entry, NULL, s); /* Updated call */

        if (ctx->active->file_count == 0)
          unput_char = ESC;
        maybe_change_x_step = TRUE;
      }
      need_dsp_help = TRUE;
      break;

    case ACTION_LOG:
    case ACTION_LOGIN:
      fe_ptr =
          ctx->active
              ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
              .file;
      if (ctx->view_mode == DISK_MODE || ctx->view_mode == USER_MODE) {
        (void)GetFileNamePath(fe_ptr, new_login_path);
        if (!GetNewLoginPath(ctx, ctx->active, new_login_path)) {
          dir_entry->login_flag = TRUE;

          (void)LogDisk(ctx, ctx->active, new_login_path);
          unput_char = ESC;
        }
        need_dsp_help = TRUE;
      }
      break;

    case ACTION_ENTER:
      if (ctx->preview_mode) {
        action = ACTION_NONE;
        break;
      }
      /* Toggle Big Window */
      if (dir_entry->big_window)
        break; /* Exit loop */

      dir_entry->big_window = TRUE;
      ch = '\0';

      /* Use Global Refresh for clean transition */
      RefreshView(ctx, dir_entry);

      /* Update scrolling metrics for new size */
      RereadWindowSize(ctx, dir_entry);

      action =
          ACTION_NONE; /* Prevent loop termination to stay in file window */
      break;

    case ACTION_CMD_P:
      fe_ptr =
          ctx->active
              ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
              .file;
      de_ptr = fe_ptr->dir_entry;
      {
        char pipe_cmd[PATH_LENGTH + 1];
        pipe_cmd[0] = '\0';
        if (GetPipeCommand(ctx, pipe_cmd) == 0) {
          (void)Pipe(ctx, de_ptr, fe_ptr, pipe_cmd);
        }
      }
      RefreshView(ctx, dir_entry);
      need_dsp_help = TRUE;
      break;

    case ACTION_CMD_X:
      fe_ptr =
          ctx->active
              ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
              .file;
      de_ptr = fe_ptr->dir_entry;
      {
        char command_template[COMMAND_LINE_LENGTH + 1];
        command_template[0] = '\0';
        if (fe_ptr &&
            (fe_ptr->stat_struct.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH))) {
          StrCp(command_template, fe_ptr->name);
        }
        if (GetCommandLine(ctx, command_template) == 0) {
          (void)Execute(ctx, de_ptr, fe_ptr, command_template,
                        &ctx->active->vol->vol_stats, UI_ArchiveCallback);
          if (ctx->view_mode == ARCHIVE_MODE)
            HitReturnToContinue();
        }
      }
      dir_entry = RefreshFileView(ctx, dir_entry);

      /* Insert: Explicit Global Refresh to be safe */
      RefreshView(ctx, dir_entry);
      need_dsp_help = TRUE;
      break;

    case ACTION_QUIT_DIR:
      need_dsp_help = TRUE;
      fe_ptr =
          ctx->active
              ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
              .file;
      de_ptr = fe_ptr->dir_entry;
      QuitTo(ctx, de_ptr);
      break;

    case ACTION_QUIT:
      need_dsp_help = TRUE;
      Quit(ctx);
      action = ACTION_NONE;
      break;

    case ACTION_VOL_MENU:
      /* Panel state isolation: No vol_stats sync */

      if (SelectLoadedVolume(ctx, NULL) == 0) {
        unput_char = '\0'; /* Ensure we return clean ESC, no pending input */
        return ESC;
      } else {
        ch = 0;
      }
      if (ctx->active->vol != start_vol)
        return ESC;
      break;
    case ACTION_VOL_PREV:
      /* Panel state isolation: No vol_stats sync */

      if (CycleLoadedVolume(ctx, ctx->active, -1) == 0) {
        unput_char = '\0'; /* Ensure we return clean ESC, no pending input */
        return ESC;
      } else {
        ch = 0;
      }
      if (ctx->active->vol != start_vol)
        return ESC;
      break;
    case ACTION_VOL_NEXT:
      /* Panel state isolation: No vol_stats sync */

      if (CycleLoadedVolume(ctx, ctx->active, 1) == 0) {
        unput_char = '\0'; /* Ensure we return clean ESC, no pending input */
        return ESC;
      } else {
        ch = 0;
      }
      if (ctx->active->vol != start_vol)
        return ESC;
      break;

    case ACTION_REFRESH: {
      dir_entry = RefreshFileView(ctx, dir_entry);
      need_dsp_help = TRUE;
    } break;

    case ACTION_RESIZE:
      ctx->resize_request = TRUE;
      break;

    case ACTION_TOGGLE_STATS: /* ADDED */
      ctx->show_stats = !ctx->show_stats;
      ctx->resize_request = TRUE;
      break;

    case ACTION_TOGGLE_COMPACT: /* ADDED */
      ctx->fixed_col_width = (ctx->fixed_col_width == 0) ? 32 : 0;
      ctx->resize_request = TRUE;
      break;

    case ACTION_ESCAPE:
      break; /* Handled by loop condition */

    case ACTION_LIST_JUMP:
      ListJump(ctx, dir_entry, "");
      need_dsp_help = TRUE;
      break;

    default:
      break;
    } /* switch */

    /* Centralized check: If directory became empty, we MUST pop out of file
     * window */
    if (ctx->active->file_count == 0) {
      action = ACTION_ESCAPE;
    }

  } while (action != ACTION_QUIT && action != ACTION_ENTER &&
           action != ACTION_ESCAPE && action != ACTION_QUIT);

  if (dir_entry->big_window) {
    SwitchToSmallFileWindow(ctx);
    /* We don't need full refresh here because HandleDirWindow will catch the
     * return */
  }

  /* Ensure all mode flags are cleared when exiting back to the tree.
   * This guarantees that RefreshView returns to small window
   * ctx->layout. */
  dir_entry->global_flag = FALSE;
  dir_entry->tagged_flag = FALSE;
  dir_entry->big_window = FALSE;

  {
    FILE *fp = fopen("/tmp/ytree_debug_exit.log", "a");
    if (fp) {
      fprintf(fp, "DEBUG: HandleFileWindow EXITING with %s\n",
              (action == ACTION_ENTER) ? "CR" : "ESC");
      fclose(fp);
    }
  }
  return ((action == ACTION_ENTER)
              ? CR
              : ESC); /* Return CR or ESC based on action */
}

/* WalkTaggedFiles moved to file_tags.c */

static void RereadWindowSize(ViewContext *ctx, DirEntry *dir_entry) {
  int height, width;
  SetPanelFileMode(ctx, ctx->active, GetPanelFileMode(ctx->active));

  GetMaxYX(ctx->ctx_file_window, &height, &width);
  x_step = (GetPanelMaxColumn(ctx->active) > 1) ? height : 1;
  max_disp_files = height * GetPanelMaxColumn(ctx->active);

  if (dir_entry->start_file + dir_entry->cursor_pos <
      (int)ctx->active->file_count) {
    while (dir_entry->cursor_pos >= max_disp_files) {
      dir_entry->start_file += x_step;
      dir_entry->cursor_pos -= x_step;
    }
  }
  return;
}

static void ListJump(ViewContext *ctx, DirEntry *dir_entry, char *str) {
  char search_buf[256];
  int buf_len = 0;
  int ch;
  int original_start_file = dir_entry->start_file;
  int original_cursor_pos = dir_entry->cursor_pos;
  int i;
  int found_idx;
  int start_x = 0;

  (void)str; /* Unused */

  memset(search_buf, 0, sizeof(search_buf));

  ClearHelp(ctx);
  MvAddStr(Y_PROMPT(ctx), 1, "Search: ");
  RefreshWindow(stdscr);

  while (1) {
    /* Update prompt */
    move(Y_PROMPT(ctx), 9);
    addstr(search_buf);
    clrtoeol();
    refresh();

    ch = Getch(ctx);

    /* Handle Resize or Error */
    if (ch == -1)
      break;

    if (ch == ESC) {
      /* Cancel: Restore */
      dir_entry->start_file = original_start_file;
      dir_entry->cursor_pos = original_cursor_pos;
      DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                   dir_entry->start_file + dir_entry->cursor_pos, start_x,
                   ctx->ctx_file_window);
      break;
    }

    if (ch == CR || ch == LF) {
      /* Confirm: Keep new position */
      break;
    }

    if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b' || ch == KEY_DC) {
      if (buf_len > 0) {
        buf_len--;
        search_buf[buf_len] = '\0';

        /* Re-search or Restore */
        if (buf_len == 0) {
          dir_entry->start_file = original_start_file;
          dir_entry->cursor_pos = original_cursor_pos;
        } else {
          /* Search again from top for the shorter string */
          found_idx = -1;
          for (i = 0; i < (int)ctx->active->file_count; i++) {
            FileEntry *fe = ctx->active->file_entry_list[i].file;
            if (strncasecmp(fe->name, search_buf, buf_len) == 0) {
              found_idx = i;
              break;
            }
          }

          if (found_idx != -1) {
            if (found_idx >= dir_entry->start_file &&
                found_idx < dir_entry->start_file + max_disp_files) {
              dir_entry->cursor_pos = found_idx - dir_entry->start_file;
            } else {
              dir_entry->start_file = found_idx;
              dir_entry->cursor_pos = 0;
            }
          }
        }
        DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                     dir_entry->start_file + dir_entry->cursor_pos, start_x,
                     ctx->ctx_file_window);
        RefreshWindow(ctx->ctx_file_window);
      }
    } else if (isprint(ch)) {
      if (buf_len < (int)sizeof(search_buf) - 1) {
        search_buf[buf_len] = ch;
        search_buf[buf_len + 1] = '\0';

        found_idx = -1;
        for (i = 0; i < (int)ctx->active->file_count; i++) {
          FileEntry *fe = ctx->active->file_entry_list[i].file;
          if (strncasecmp(fe->name, search_buf, buf_len + 1) == 0) {
            found_idx = i;
            break;
          }
        }

        if (found_idx != -1) {
          buf_len++; /* Accept char */
          if (found_idx >= dir_entry->start_file &&
              found_idx < dir_entry->start_file + max_disp_files) {
            dir_entry->cursor_pos = found_idx - dir_entry->start_file;
          } else {
            dir_entry->start_file = found_idx;
            dir_entry->cursor_pos = 0;
          }
          DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                       dir_entry->start_file + dir_entry->cursor_pos, start_x,
                       ctx->ctx_file_window);
          RefreshWindow(ctx->ctx_file_window);
        } else {
          /* No match: Accept char to show user what they typed, but do not move
           * cursor */
          buf_len++;
        }
      }
    }
  }
}

/* Shell utilities and recursive IO moved to prompts.c */

/* UI_ViewTaggedFiles moved to prompts.c */

static void UpdatePreview(ViewContext *ctx, DirEntry *dir_entry) {
  if (ctx->preview_mode && ctx->ctx_preview_window) {
    FileEntry *fe = NULL;
    if (ctx->active && ctx->active->file_entry_list &&
        ctx->active->file_count > 0) {
      int idx = dir_entry->start_file + dir_entry->cursor_pos;
      if (idx >= 0 && (unsigned int)idx < ctx->active->file_count) {
        fe = ctx->active->file_entry_list[idx].file;
      }
    }
    if (fe) {
      char path[PATH_LENGTH];
      GetFileNamePath(fe, path);
      RenderFilePreview(ctx, ctx->ctx_preview_window, path,
                        &preview_line_offset, 0);
    } else {
      RenderFilePreview(ctx, ctx->ctx_preview_window, "", &preview_line_offset,
                        0);
    }
  }
}
