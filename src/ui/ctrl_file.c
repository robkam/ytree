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
#include "../../include/ytree_cmd.h"
#include "../../include/ytree_fs.h"
#include "../../include/ytree_ui.h"
#include <libgen.h>
#include <stdlib.h>

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#if !defined(__NeXT__) && !defined(ultrix)
#endif /* __NeXT__ ultrix */

static long preview_line_offset = 0;
static int saved_fixed_width = 0;

/* --- Forward Declarations --- */
/* file_list.c: ReadFileList, ReadGlobalFileList, BuildFileEntryList,
 *              FreeFileEntryList, InvalidateVolumePanels, DisplayFileWindow */
static void ListJump(ViewContext *ctx, DirEntry *dir_entry, char *str);
static void DrawFileListJumpPrompt(ViewContext *ctx, WINDOW *win,
                                   const char *search_buf);
static void UpdatePreview(ViewContext *ctx, const DirEntry *dir_entry);
static int FindDirIndexInVolume(const struct Volume *vol,
                                const DirEntry *target);
static struct Volume *FindVolumeForDir(ViewContext *ctx, const DirEntry *target,
                                       int *dir_idx_out);
static void PositionOwnerFileCursor(ViewContext *ctx, DirEntry *owner_dir,
                                    const FileEntry *target_file);
static BOOL JumpToOwnerDirectory(ViewContext *ctx,
                                 const DirEntry *global_dir_entry);

static YtreeAction FilterPreviewAction(YtreeAction action) {
  switch (action) {
  case ACTION_NONE:
  case ACTION_ESCAPE:
  case ACTION_VIEW_PREVIEW:
  case ACTION_MOVE_UP:
  case ACTION_MOVE_DOWN:
  case ACTION_PAGE_UP:
  case ACTION_PAGE_DOWN:
  case ACTION_HOME:
  case ACTION_END:
  case ACTION_PREVIEW_SCROLL_UP:
  case ACTION_PREVIEW_SCROLL_DOWN:
  case ACTION_PREVIEW_HOME:
  case ACTION_PREVIEW_END:
  case ACTION_PREVIEW_PAGE_UP:
  case ACTION_PREVIEW_PAGE_DOWN:
  case ACTION_RESIZE:
    return action;
  default:
    return ACTION_NONE;
  }
}

/*
 * UpdateStatsPanel
 * Centralized stats panel update that shows the correct information based on
 * context.
 * - In file window mode: Shows stats for currently selected file
 * - In directory mode: Shows stats for current directory
 */
void UpdateStatsPanel(ViewContext *ctx, DirEntry *dir_entry,
                      const Statistic *s) {
  FileEntry *fe_ptr = NULL;
  BOOL show_file_stats = FALSE;

  /* Safety checks */
  if (!ctx || !dir_entry || !s || !ctx->active)
    return;

  /* Check if we're in file window with files available */
  if (ctx->focused_window == FOCUS_FILE && ctx->active->file_entry_list &&
      ctx->active->file_count > 0) {
    int file_index = dir_entry->start_file + dir_entry->cursor_pos;
    if (file_index >= 0 && (unsigned int)file_index < ctx->active->file_count) {
      fe_ptr = ctx->active->file_entry_list[file_index].file;
      show_file_stats = (fe_ptr != NULL);
    }
  }

  if (show_file_stats) {
    /* File Window Mode - show individual file stats */
    DisplayFileStatistic(ctx, fe_ptr, s);

    /* Also update attributes section */
    if (dir_entry->global_flag)
      DisplayGlobalFileParameter(ctx, fe_ptr);
    else
      DisplayFileParameter(ctx, fe_ptr);
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

/* Local helper to refresh file window, maintaining file cursor */
DirEntry *RefreshFileView(ViewContext *ctx, DirEntry *dir_entry) {
  char *saved_name = NULL;
  const Statistic *s = &ctx->active->vol->vol_stats;
  int found_idx = -1;
  int start_x = 0;

  /* 1. Save current filename */
  if (ctx->active->file_count > 0) {
    /* Check bounds before access */
    if (dir_entry->start_file + dir_entry->cursor_pos <
        (int)ctx->active->file_count) {
      const FileEntry *curr =
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
        found_idx < dir_entry->start_file + FileNav_GetMaxDispFiles(ctx)) {
      /* Still on screen, just move cursor */
      dir_entry->cursor_pos = found_idx - dir_entry->start_file;
    } else {
      /* Off screen, recenter or scroll */
      dir_entry->start_file = found_idx;
      dir_entry->cursor_pos = 0;
      /* Bounds check */
      if (dir_entry->start_file + FileNav_GetMaxDispFiles(ctx) >
          (int)ctx->active->file_count) {
        dir_entry->start_file = MAXIMUM(0, (int)ctx->active->file_count -
                                               FileNav_GetMaxDispFiles(ctx));
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
  int ch;
  int unput_char;
  int start_x = 0;
  BOOL need_dsp_help;
  BOOL maybe_change_x_step;
  BOOL volume_changed = FALSE;
  YtreeAction action = ACTION_NONE; /* Initialize action */
  BOOL jumped_to_owner_dir = FALSE;
  BOOL switched_panel = FALSE;
  BOOL return_esc = FALSE;
  YtreePanel *owner_panel = ctx->active;
  DirEntry *tracked_file_dir = ctx->active->file_dir_entry;
  const DirEntry *last_stats_dir = NULL; /* Track context changes */
  struct Volume *start_vol = ctx->active->vol; /* Safety Check Variable */
  Statistic *s = &ctx->active->vol->vol_stats;
  char watcher_path[PATH_LENGTH + 1];

  /* ADDED INSTRUCTION: Focus Unification */
  ctx->focused_window = FOCUS_FILE;
  ctx->active->saved_focus = FOCUS_FILE;
  if (tracked_file_dir == dir_entry) {
    dir_entry->start_file = ctx->active->start_file;
    dir_entry->cursor_pos = ctx->active->file_cursor_pos;
  }
  ctx->active->file_dir_entry = dir_entry;

  unput_char = '\0';
  fe_ptr = NULL;

  /* Reset cursor position flags */
  /*--------------------------------------*/

  need_dsp_help = TRUE;
  maybe_change_x_step = TRUE;

  PanelTags_ApplyToTree(ctx, ctx->active);
  BuildFileEntryList(ctx, ctx->active);
  FileNav_SyncGridMetrics(ctx);
  ctx->ctrl_file_hide_right = 0;

  /* Sanitize cursor position immediately after BuildFileEntryList */
  if (ctx->active->file_count > 0) {
    if (dir_entry->cursor_pos < 0)
      dir_entry->cursor_pos = 0;
    if (dir_entry->start_file < 0)
      dir_entry->start_file = 0;

    /* Bounds Check: ensure we aren't past the end */
    if ((unsigned int)(dir_entry->start_file + dir_entry->cursor_pos) >=
        ctx->active->file_count) {
      dir_entry->start_file = MAXIMUM(0, (int)ctx->active->file_count -
                                             FileNav_GetMaxDispFiles(ctx));
      dir_entry->cursor_pos =
          (int)ctx->active->file_count - 1 - dir_entry->start_file;
    }
  } else {
    dir_entry->cursor_pos = 0;
    dir_entry->start_file = 0;
  }
  ctx->active->start_file = dir_entry->start_file;
  ctx->active->file_cursor_pos = dir_entry->cursor_pos;

  /* Initial Display using Centralized Function if applicable */
  if (ctx->preview_mode) {
    /* F7 entered from tree mode arrives here with preview already enabled.
     * Mirror the same initialization contract used by ACTION_VIEW_PREVIEW.
     */
    saved_fixed_width = ctx->fixed_col_width;
    ReCreateWindows(ctx);
    ctx->fixed_col_width = ctx->layout.big_file_win_width - 2;
    if (ctx->fixed_col_width < 1)
      ctx->fixed_col_width = 1;
    FileNav_RereadWindowSize(ctx, dir_entry);

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

  if (s->log_mode == DISK_MODE || s->log_mode == USER_MODE) {
    GetPath(dir_entry, watcher_path);
    Watcher_SetDir(ctx, watcher_path);
  }

  do {
    /* Critical Safety: If volume was deleted (e.g. via K menu), abort
     * immediately */
    if (ctx->active->vol != start_vol) {
      volume_changed = TRUE;
      action = ACTION_ESCAPE;
      goto file_window_done;
    }

    /* Keep grid metrics in sync with the current file list/rendering width.
     * This prevents stale x_step/max_disp_files after refresh/toggle/resize
     * paths that rebuild the list but do not pass through mode-change logic.
     */
    FileNav_SyncGridMetrics(ctx);
    maybe_change_x_step = FALSE;

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
    if (ctx->active->vol != start_vol) {
      volume_changed = TRUE;
      action = ACTION_ESCAPE;
      goto file_window_done;
    }

    if (IsUserActionDefined(ctx)) { /* User commands take precedence */
      ch = FileUserMode(ctx,
                        &(ctx->active->file_entry_list[dir_entry->start_file +
                                                       dir_entry->cursor_pos]),
                        ch, &ctx->active->vol->vol_stats);
      if (ctx->active->vol != start_vol) {
        volume_changed = TRUE;
        action = ACTION_ESCAPE;
        goto file_window_done;
      }
    }

    if (ctx->resize_request) {
      /* Simplified Resize using Centralized Function */
      FileNav_RereadWindowSize(ctx, dir_entry);
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

    if (ctx->preview_mode) {
      action = FilterPreviewAction(action);
    }

    if (FileNav_GetXStep(ctx) == 1 &&
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
    case ACTION_CMD_TAGGED_PRINT:
    case ACTION_CMD_TAGGED_S:
    case ACTION_CMD_TAGGED_X:
    case ACTION_TOGGLE_TAGGED_MODE:
    case ACTION_INVERT:
      if (handle_tag_file_action(ctx, action, dir_entry, &unput_char,
                                 &need_dsp_help, start_x, s,
                                 &maybe_change_x_step)) {
        break;
      }
      break;

    case ACTION_NONE:
      break;

    case ACTION_VIEW_PREVIEW:
    case ACTION_PREVIEW_SCROLL_DOWN:
    case ACTION_PREVIEW_SCROLL_UP:
    case ACTION_PREVIEW_HOME:
    case ACTION_PREVIEW_END:
    case ACTION_PREVIEW_PAGE_UP:
    case ACTION_PREVIEW_PAGE_DOWN:
      (void)handle_file_window_preview_action(
          ctx, action, &dir_entry, &action, &s, &start_vol, &need_dsp_help,
          &preview_line_offset, &saved_fixed_width, UpdatePreview);
      break;

    case ACTION_SPLIT_SCREEN:
    case ACTION_SWITCH_PANEL:
      if (handle_file_window_split_switch_action(ctx, action, dir_entry,
                                                 owner_panel, &switched_panel,
                                                 &action, &return_esc)) {
        if (return_esc)
          return ESC;
        break;
      }
      break;

    case ACTION_MOVE_DOWN:
    case ACTION_MOVE_UP:
    case ACTION_MOVE_RIGHT:
    case ACTION_MOVE_LEFT:
    case ACTION_PAGE_DOWN:
    case ACTION_PAGE_UP:
    case ACTION_END:
    case ACTION_HOME:
      (void)handle_file_window_navigation_action(
          ctx, action, dir_entry, &start_x, &need_dsp_help,
          &preview_line_offset, UpdatePreview, ListJump);
      break;

    case ACTION_CMD_A:
      if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
        UI_Beep(ctx, FALSE);
        break;
      }
      fe_ptr =
          ctx->active
              ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
              .file;

      need_dsp_help = TRUE;
      switch (UI_PromptAttributeAction(ctx, FALSE, TRUE)) {
      case 'M':
        if (ChangeFileModus(ctx, fe_ptr))
          break;
        DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                     dir_entry->start_file + dir_entry->cursor_pos, start_x,
                     ctx->ctx_file_window);
        break;
      case 'O':
        if (ChangeFileOwner(ctx, fe_ptr))
          break;
        DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                     dir_entry->start_file + dir_entry->cursor_pos, start_x,
                     ctx->ctx_file_window);
        break;
      case 'G':
        if (ChangeFileGroup(ctx, fe_ptr))
          break;
        DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                     dir_entry->start_file + dir_entry->cursor_pos, start_x,
                     ctx->ctx_file_window);
        break;
      case 'D':
        if (ChangeFileDate(ctx, fe_ptr))
          break;
        DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                     dir_entry->start_file + dir_entry->cursor_pos, start_x,
                     ctx->ctx_file_window);
        break;
      default:
        break;
      }
      break;

    case ACTION_CMD_Y:
    case ACTION_CMD_C:
    case ACTION_CMD_M:
    case ACTION_CMD_D:
    case ACTION_CMD_R:
    case ACTION_CMD_P:
    case ACTION_CMD_PRINT:
    case ACTION_CMD_X:
      (void)handle_file_window_command_action(
          ctx, action, &dir_entry, &need_dsp_help, &maybe_change_x_step, s);
      break;

    case ACTION_VOL_MENU:
    case ACTION_VOL_PREV:
    case ACTION_VOL_NEXT:
      (void)handle_file_window_volume_action(ctx, action, start_vol,
                                             &unput_char, &return_esc);
      if (return_esc)
        return ESC;
      break;

    case ACTION_TOGGLE_HIDDEN:
    case ACTION_CMD_I:
    case ACTION_CMD_O:
    case ACTION_CMD_G:
    case ACTION_TOGGLE_MODE:
    case ACTION_CMD_V:
    case ACTION_CMD_H:
    case ACTION_CMD_E:
    case ACTION_COMPARE_FILE:
    case ACTION_CMD_MKFILE:
    case ACTION_CMD_S:
    case ACTION_FILTER:
    case ACTION_LOG:
    case ACTION_ENTER:
    case ACTION_QUIT_DIR:
    case ACTION_QUIT:
    case ACTION_REFRESH:
    case ACTION_EDIT_CONFIG:
    case ACTION_RESIZE:
    case ACTION_TOGGLE_STATS:
    case ACTION_TOGGLE_COMPACT:
    case ACTION_ESCAPE:
      (void)handle_file_window_misc_dispatch_action(
          ctx, action, &dir_entry, &action, &unput_char, &start_x,
          &need_dsp_help, &maybe_change_x_step, s, &preview_line_offset,
          UpdatePreview);
      break;

    case ACTION_LIST_JUMP:
      (void)handle_file_window_navigation_action(
          ctx, action, dir_entry, &start_x, &need_dsp_help,
          &preview_line_offset, UpdatePreview, ListJump);
      break;

    case ACTION_TO_DIR:
      if (!dir_entry->global_flag) {
        break;
      }
      if (JumpToOwnerDirectory(ctx, dir_entry)) {
        jumped_to_owner_dir = TRUE;
        action = ACTION_ESCAPE;
      } else {
        UI_Beep(ctx, FALSE);
      }
      need_dsp_help = TRUE;
      break;

    default:
      break;
    } /* switch */

    if (ctx->active == owner_panel) {
      owner_panel->file_dir_entry = dir_entry;
      owner_panel->start_file = dir_entry->start_file;
      owner_panel->file_cursor_pos = dir_entry->cursor_pos;
      ctx->active->start_file = dir_entry->start_file;
      ctx->active->file_cursor_pos = dir_entry->cursor_pos;
    }

    /* Centralized check: If directory became empty, we MUST pop out of file
     * window */
    if (ctx->active->file_count == 0) {
      action = ACTION_ESCAPE;
    }

  } while (action != ACTION_QUIT && action != ACTION_ENTER &&
           action != ACTION_ESCAPE);

file_window_done:
  if (!volume_changed && dir_entry->big_window) {
    SwitchToSmallFileWindow(ctx);
    /* We don't need full refresh here because HandleDirWindow will catch the
     * return */
  }

  if (!switched_panel) {
    owner_panel->saved_focus = FOCUS_TREE;
  }
  if (!volume_changed) {
    owner_panel->file_dir_entry = dir_entry;
    owner_panel->start_file = dir_entry->start_file;
    owner_panel->file_cursor_pos = dir_entry->cursor_pos;

    /* Ensure all mode flags are cleared when exiting back to the tree.
     * This guarantees that RefreshView returns to small window
     * ctx->layout. */
    dir_entry->global_flag = FALSE;
    dir_entry->global_all_volumes = FALSE;
    dir_entry->tagged_flag = FALSE;
    dir_entry->big_window = FALSE;
  } else {
    owner_panel->file_dir_entry = NULL;
    owner_panel->start_file = 0;
    owner_panel->file_cursor_pos = 0;
  }

  DEBUG_LOG("DEBUG: HandleFileWindow EXITING with %s",
            (action == ACTION_ENTER) ? "CR" : "ESC");
  return ((action == ACTION_ENTER)
              ? CR
              : (jumped_to_owner_dir ? '\\' : ESC)); /* Return CR/ESC/ToDir */
}

/* WalkTaggedFiles moved to file_tags.c */

static int FindDirIndexInVolume(const struct Volume *vol,
                                const DirEntry *target) {
  int i;

  if (!vol || !target || !vol->dir_entry_list)
    return -1;

  for (i = 0; i < vol->total_dirs; i++) {
    if (vol->dir_entry_list[i].dir_entry == target)
      return i;
  }

  return -1;
}

static struct Volume *FindVolumeForDir(ViewContext *ctx, const DirEntry *target,
                                       int *dir_idx_out) {
  struct Volume *vol_iter;
  struct Volume *vol_tmp;
  int idx;

  if (!ctx || !ctx->active || !target)
    return NULL;

  if (!ctx->active->vol->dir_entry_list || ctx->active->vol->total_dirs <= 0) {
    BuildDirEntryList(ctx, ctx->active->vol, &ctx->active->current_dir_entry);
  }
  idx = FindDirIndexInVolume(ctx->active->vol, target);
  if (idx >= 0) {
    if (dir_idx_out)
      *dir_idx_out = idx;
    return ctx->active->vol;
  }

  HASH_ITER(hh, ctx->volumes_head, vol_iter, vol_tmp) {
    int rebuild_idx = 0;

    if (vol_iter == ctx->active->vol)
      continue;

    if (!vol_iter->dir_entry_list || vol_iter->total_dirs <= 0) {
      BuildDirEntryList(ctx, vol_iter, &rebuild_idx);
    }

    idx = FindDirIndexInVolume(vol_iter, target);
    if (idx >= 0) {
      if (dir_idx_out)
        *dir_idx_out = idx;
      return vol_iter;
    }
  }

  return NULL;
}

static void PositionOwnerFileCursor(ViewContext *ctx, DirEntry *owner_dir,
                                    const FileEntry *target_file) {
  int i;
  int file_idx = -1;
  int page_height;
  int page_size;
  int column_count;
  int file_count;

  if (!ctx || !ctx->active || !owner_dir || !target_file)
    return;

  BuildFileEntryList(ctx, ctx->active);

  file_count = (int)ctx->active->file_count;
  if (file_count <= 0) {
    owner_dir->start_file = 0;
    owner_dir->cursor_pos = 0;
    ctx->active->start_file = owner_dir->start_file;
    return;
  }

  for (i = 0; i < file_count; i++) {
    if (ctx->active->file_entry_list[i].file == target_file) {
      file_idx = i;
      break;
    }
  }

  if (file_idx < 0) {
    owner_dir->start_file = 0;
    owner_dir->cursor_pos = 0;
    ctx->active->start_file = owner_dir->start_file;
    return;
  }

  SetPanelFileMode(ctx, ctx->active, GetPanelFileMode(ctx->active));
  page_height = getmaxy(ctx->ctx_file_window);
  if (page_height < 1)
    page_height = 1;
  column_count = GetPanelMaxColumn(ctx->active);
  if (column_count < 1)
    column_count = 1;
  page_size = page_height * column_count;
  if (page_size < 1)
    page_size = 1;

  owner_dir->start_file = file_idx;
  owner_dir->cursor_pos = 0;

  if (owner_dir->start_file + page_size > file_count) {
    owner_dir->start_file = MAXIMUM(0, file_count - page_size);
    owner_dir->cursor_pos = file_idx - owner_dir->start_file;
  }

  if (owner_dir->cursor_pos < 0)
    owner_dir->cursor_pos = 0;
  if (owner_dir->cursor_pos >= page_size)
    owner_dir->cursor_pos = page_size - 1;

  ctx->active->start_file = owner_dir->start_file;
}

static BOOL JumpToOwnerDirectory(ViewContext *ctx,
                                 const DirEntry *global_dir_entry) {
  YtreePanel *panel;
  int selected_idx;
  FileEntry *selected_file;
  DirEntry *owner_dir;
  int owner_dir_idx;
  struct Volume *owner_vol;
  int win_height;

  if (!ctx || !ctx->active || !global_dir_entry)
    return FALSE;
  if (!global_dir_entry->global_flag)
    return FALSE;
  if (!ctx->active->file_entry_list || ctx->active->file_count == 0)
    return FALSE;

  selected_idx = global_dir_entry->start_file + global_dir_entry->cursor_pos;
  if (selected_idx < 0 || (unsigned int)selected_idx >= ctx->active->file_count)
    return FALSE;

  selected_file = ctx->active->file_entry_list[selected_idx].file;
  if (!selected_file || !selected_file->dir_entry)
    return FALSE;
  owner_dir = selected_file->dir_entry;

  panel = ctx->active;
  owner_vol = FindVolumeForDir(ctx, owner_dir, &owner_dir_idx);
  if (!owner_vol || owner_dir_idx < 0)
    return FALSE;
  panel->vol = owner_vol;

  if (!panel->vol->dir_entry_list || panel->vol->total_dirs <= 0) {
    BuildDirEntryList(ctx, panel->vol, &panel->current_dir_entry);
  }

  win_height = getmaxy(ctx->ctx_dir_window);
  if (win_height < 1)
    win_height = ctx->layout.dir_win_height;
  if (win_height < 1)
    win_height = 1;

  if (owner_dir_idx >= panel->disp_begin_pos &&
      owner_dir_idx < panel->disp_begin_pos + win_height) {
    panel->cursor_pos = owner_dir_idx - panel->disp_begin_pos;
  } else if (owner_dir_idx < win_height) {
    panel->disp_begin_pos = 0;
    panel->cursor_pos = owner_dir_idx;
  } else {
    panel->disp_begin_pos = owner_dir_idx - (win_height / 2);
    if (panel->disp_begin_pos > panel->vol->total_dirs - win_height) {
      panel->disp_begin_pos = panel->vol->total_dirs - win_height;
    }
    if (panel->disp_begin_pos < 0)
      panel->disp_begin_pos = 0;
    panel->cursor_pos = owner_dir_idx - panel->disp_begin_pos;
  }

  if (panel->cursor_pos < 0)
    panel->cursor_pos = 0;
  if (panel->disp_begin_pos + panel->cursor_pos >= panel->vol->total_dirs) {
    panel->cursor_pos =
        (panel->vol->total_dirs > 0)
            ? (panel->vol->total_dirs - 1 - panel->disp_begin_pos)
            : 0;
  }

  owner_dir->global_flag = FALSE;
  owner_dir->global_all_volumes = FALSE;
  owner_dir->tagged_flag = FALSE;
  owner_dir->big_window = FALSE;

  PositionOwnerFileCursor(ctx, owner_dir, selected_file);

  return TRUE;
}

static void DrawFileListJumpPrompt(ViewContext *ctx, WINDOW *win,
                                   const char *search_buf) {
  int y = 0;

  if (!ctx || !win || !search_buf)
    return;

  if (win == stdscr) {
    if (ctx->layout.prompt_y > 0) {
      wmove(win, ctx->layout.prompt_y - 1, 0);
      wclrtoeol(win);
    }
    wmove(win, ctx->layout.prompt_y, 0);
    wclrtoeol(win);
    wmove(win, ctx->layout.status_y, 0);
    wclrtoeol(win);
    y = ctx->layout.prompt_y;
  } else {
    werase(win);
    y = 1; /* Center line in 3-line footer window */
  }

  mvwaddstr(win, y, 1, "Jump to: ");
  mvwaddstr(win, y, 10, search_buf);
  wclrtoeol(win);
  wnoutrefresh(win);
  doupdate();
}

static void ListJump(ViewContext *ctx, DirEntry *dir_entry, char *str) {
  char search_buf[256];
  int buf_len = 0;
  int max_disp_files;
  int original_start_file;
  int original_cursor_pos;
  int i;
  int found_idx;
  int start_x = 0;
  WINDOW *jump_win;

  if (!ctx || !ctx->active || !dir_entry)
    return;

  max_disp_files = FileNav_GetMaxDispFiles(ctx);
  original_start_file = dir_entry->start_file;
  original_cursor_pos = dir_entry->cursor_pos;
  jump_win = ctx->ctx_menu_window ? ctx->ctx_menu_window : stdscr;

  (void)str; /* Unused */

  memset(search_buf, 0, sizeof(search_buf));

  ClearHelp(ctx);
  DrawFileListJumpPrompt(ctx, jump_win, search_buf);

  while (1) {
    int ch;

    DrawFileListJumpPrompt(ctx, jump_win, search_buf);

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

static void UpdatePreview(ViewContext *ctx, const DirEntry *dir_entry) {
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
      const Statistic *s = NULL;
      GetFileNamePath(fe, path);
      if (ctx->active->vol) {
        s = &ctx->active->vol->vol_stats;
      }

      if (s && s->log_mode == ARCHIVE_MODE) {
        char archive_root[PATH_LENGTH + 1];
        char internal_path[PATH_LENGTH + 1];
        const char *relative_path = path;
        size_t root_len;
        int written;

        GetPath(ctx->active->vol->vol_stats.tree, archive_root);
        root_len = strlen(archive_root);

        if (root_len > 0 && strncmp(path, archive_root, root_len) == 0) {
          relative_path = path + root_len;
          if (*relative_path == FILE_SEPARATOR_CHAR) {
            relative_path++;
          }
        }
        if (*relative_path == '\0') {
          relative_path = fe->name;
        }

        written = snprintf(internal_path, sizeof(internal_path), "%s",
                           relative_path);
        if (written < 0 || (size_t)written >= sizeof(internal_path)) {
          werase(ctx->ctx_preview_window);
          mvwprintw(ctx->ctx_preview_window, 0, 0, "Preview path too long.");
          wnoutrefresh(ctx->ctx_preview_window);
        } else {
          RenderArchivePreview(ctx, ctx->ctx_preview_window,
                               ctx->active->vol->vol_stats.log_path,
                               internal_path, &preview_line_offset);
        }
      } else {
        RenderFilePreview(ctx, ctx->ctx_preview_window, path,
                          &preview_line_offset, 0);
      }
    } else {
      RenderFilePreview(ctx, ctx->ctx_preview_window, "", &preview_line_offset,
                        0);
    }
  }
}
