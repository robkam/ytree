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
static void DrawFileListJumpPrompt(ViewContext *ctx, WINDOW *win,
                                   const char *search_buf);
static void UpdatePreview(ViewContext *ctx, const DirEntry *dir_entry);
static void SyncFileGridMetrics(ViewContext *ctx);
static void UpdateFileHeaderPath(ViewContext *ctx, DirEntry *dir_entry);
static int FindDirIndexInVolume(const struct Volume *vol,
                                const DirEntry *target);
static struct Volume *FindVolumeForDir(ViewContext *ctx,
                                       const DirEntry *target,
                                       int *dir_idx_out);
static void PositionOwnerFileCursor(ViewContext *ctx, DirEntry *owner_dir,
                                    const FileEntry *target_file);
static BOOL JumpToOwnerDirectory(ViewContext *ctx,
                                 const DirEntry *global_dir_entry);
static void HandleFileCompare(ViewContext *ctx, FileEntry *source_file);

static BOOL HasNonWhitespace(const char *text) {
  if (!text)
    return FALSE;
  while (*text) {
    if (!isspace((unsigned char)*text))
      return TRUE;
    text++;
  }
  return FALSE;
}

static int BuildFileCompareCommand(const char *command_template,
                                   const char *source_path,
                                   const char *target_path, char *command_line,
                                   size_t command_line_size) {
  char escaped_source[PATH_LENGTH * 2 + 1];
  char escaped_target[PATH_LENGTH * 2 + 1];
  char expanded_command[COMMAND_LINE_LENGTH + 1];
  int written;
  BOOL has_placeholders;

  if (!command_template || !source_path || !target_path || !command_line ||
      command_line_size == 0) {
    return -1;
  }

  StrCp(escaped_source, source_path);
  StrCp(escaped_target, target_path);
  has_placeholders = (strstr(command_template, "%1") != NULL ||
                      strstr(command_template, "%2") != NULL);

  if (has_placeholders) {
    if (String_Replace(expanded_command, sizeof(expanded_command),
                       command_template, "%1", escaped_source) != 0) {
      return -1;
    }
    if (String_Replace(command_line, command_line_size, expanded_command, "%2",
                       escaped_target) != 0) {
      return -1;
    }
    return 0;
  }

  written = snprintf(command_line, command_line_size, "%s %s %s",
                     command_template, escaped_source, escaped_target);
  if (written < 0 || (size_t)written >= command_line_size)
    return -1;

  return 0;
}

static int ResolveFileCompareTargetPath(FileEntry *source_file,
                                        const char *target_input,
                                        char *resolved_target_path) {
  char source_dir[PATH_LENGTH + 1];
  char raw_target_path[(PATH_LENGTH * 2) + 2];
  const char *home = NULL;
  int written;

  if (!source_file || !source_file->dir_entry || !target_input ||
      !resolved_target_path) {
    return -1;
  }

  if (!HasNonWhitespace(target_input))
    return -1;

  GetPath(source_file->dir_entry, source_dir);
  source_dir[PATH_LENGTH] = '\0';

  if (target_input[0] == FILE_SEPARATOR_CHAR) {
    written = snprintf(raw_target_path, sizeof(raw_target_path), "%s",
                       target_input);
  } else if (target_input[0] == '~' &&
             (target_input[1] == '\0' || target_input[1] == FILE_SEPARATOR_CHAR) &&
             (home = getenv("HOME")) != NULL) {
    written = snprintf(raw_target_path, sizeof(raw_target_path), "%s%s", home,
                       target_input + 1);
  } else {
    written = snprintf(raw_target_path, sizeof(raw_target_path), "%s%c%s",
                       source_dir, FILE_SEPARATOR_CHAR, target_input);
  }

  if (written < 0 || (size_t)written >= sizeof(raw_target_path))
    return -1;

  NormPath(raw_target_path, resolved_target_path);
  resolved_target_path[PATH_LENGTH] = '\0';
  return 0;
}

static void GetCommandDisplayName(const char *command_template,
                                  char *command_name, size_t command_name_size) {
  const char *cursor;
  size_t idx = 0;

  if (!command_name || command_name_size == 0)
    return;

  command_name[0] = '\0';
  if (!command_template)
    return;

  cursor = command_template;
  while (*cursor && isspace((unsigned char)*cursor))
    cursor++;
  if (*cursor == '\0')
    return;

  if (*cursor == '"' || *cursor == '\'') {
    char quote = *cursor++;
    while (*cursor && *cursor != quote && idx + 1 < command_name_size) {
      command_name[idx++] = *cursor++;
    }
  } else {
    while (*cursor && !isspace((unsigned char)*cursor) &&
           idx + 1 < command_name_size) {
      command_name[idx++] = *cursor++;
    }
  }
  command_name[idx] = '\0';
}

static void DrainPendingInput(ViewContext *ctx) {
  int ch;

  if (!ctx)
    return;

  nodelay(stdscr, TRUE);
  do {
    ch = WGetch(ctx, stdscr);
  } while (ch != ERR);
  nodelay(stdscr, FALSE);
}

static void HandleFileCompare(ViewContext *ctx, FileEntry *source_file) {
  CompareRequest request;
  struct stat source_stat;
  struct stat target_stat;
  char source_path[PATH_LENGTH + 1];
  char target_path[PATH_LENGTH + 1];
  char source_dir[PATH_LENGTH + 1];
  char command_line[COMMAND_LINE_LENGTH + 1];
  char command_name[PATH_LENGTH + 1];
  const char *filediff = NULL;
  int start_dir_fd = -1;
  int result = -1;
  int exit_status;

  if (!ctx || !source_file)
    return;

  if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
    UI_Message(ctx,
               "File compare is not supported in this view mode.*"
               "Use disk/user mode.");
    return;
  }
  if (!source_file->dir_entry || source_file->dir_entry->global_flag) {
    UI_Message(ctx,
               "File compare is not supported in this file context.*"
               "Use a normal file list entry.");
    return;
  }

  if (UI_BuildFileCompareRequest(ctx, source_file, &request) != 0)
    return;

  filediff = UI_GetCompareHelperCommand(ctx, COMPARE_FLOW_FILE);
  if (!HasNonWhitespace(filediff)) {
    UI_Message(
        ctx,
        "FILEDIFF helper is not configured.*Set FILEDIFF in ~/.ytree "
        "(or .ytree).");
    return;
  }

  strncpy(source_path, request.source_path, PATH_LENGTH);
  source_path[PATH_LENGTH] = '\0';
  if (ResolveFileCompareTargetPath(source_file, request.target_path,
                                   target_path) != 0) {
    UI_Message(ctx, "Compare target is empty or invalid.*Choose a file target.");
    return;
  }

  if (stat(source_path, &source_stat) != 0) {
    UI_Message(ctx, "Compare source is not accessible:*\"%s\"*%s", source_path,
               strerror(errno));
    return;
  }
  if (stat(target_path, &target_stat) != 0) {
    UI_Message(ctx, "Compare target does not exist:*\"%s\"*Select a valid file.",
               target_path);
    return;
  }
  if (S_ISDIR(source_stat.st_mode) || S_ISDIR(target_stat.st_mode)) {
    UI_Message(ctx, "File compare requires two files.*Directory targets are unsupported.");
    return;
  }
  if ((source_stat.st_dev == target_stat.st_dev &&
       source_stat.st_ino == target_stat.st_ino) ||
      !strcmp(source_path, target_path)) {
    UI_Message(ctx, "Can't compare: source and target are the same file.");
    return;
  }

  if (BuildFileCompareCommand(filediff, source_path, target_path, command_line,
                              sizeof(command_line)) != 0) {
    UI_Message(
        ctx,
        "FILEDIFF command is invalid or too long.*Check FILEDIFF in ~/.ytree.");
    return;
  }

  GetPath(source_file->dir_entry, source_dir);
  source_dir[PATH_LENGTH] = '\0';

  start_dir_fd = open(".", O_RDONLY);
  if (start_dir_fd == -1) {
    UI_Message(ctx, "Error preparing compare launch:*%s", strerror(errno));
    return;
  }
  if (chdir(source_dir) != 0) {
    UI_Message(ctx, "Can't change directory to*\"%s\"*%s", source_dir,
               strerror(errno));
    close(start_dir_fd);
    return;
  }

  DrainPendingInput(ctx);
  result = QuerySystemCall(ctx, command_line, &ctx->active->vol->vol_stats);

  if (fchdir(start_dir_fd) == -1) {
    UI_Message(ctx, "Error restoring directory*%s", strerror(errno));
  }
  close(start_dir_fd);

  if (result == -1) {
    UI_Message(
        ctx,
        "Failed to launch FILEDIFF helper.*Install/configure FILEDIFF in "
        "~/.ytree.");
    return;
  }

  if (WIFEXITED(result)) {
    exit_status = WEXITSTATUS(result);
    if (exit_status == 126 || exit_status == 127) {
      GetCommandDisplayName(filediff, command_name, sizeof(command_name));
      UI_Message(ctx,
                 "FILEDIFF helper not available:*\"%s\"*"
                 "Install it or update FILEDIFF in ~/.ytree.",
                 command_name[0] ? command_name : filediff);
    }
  }
}

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

static void fmovedown(ViewContext *ctx, int *start_file, int *cursor_pos,
                      const int *start_x, DirEntry *dir_entry) {
  Nav_MoveDown(cursor_pos, start_file, (int)ctx->active->file_count,
               max_disp_files, 1);

  DisplayFiles(ctx, ctx->active, dir_entry, *start_file,
               *start_file + *cursor_pos, *start_x,
               ctx->ctx_file_window); /* Update dynamic header path */
  UpdateFileHeaderPath(ctx, dir_entry);
  ctx->active->start_file = *start_file;
}

static void fmoveup(ViewContext *ctx, int *start_file, int *cursor_pos,
                    const int *start_x, DirEntry *dir_entry) {
  Nav_MoveUp(cursor_pos, start_file);

  DisplayFiles(ctx, ctx->active, dir_entry, *start_file,
               *start_file + *cursor_pos, *start_x,
               ctx->ctx_file_window); /* Update dynamic header path */
  UpdateFileHeaderPath(ctx, dir_entry);
  ctx->active->start_file = *start_file;
}

static void fmoveright(ViewContext *ctx, int *start_file, int *cursor_pos,
                       int *start_x, DirEntry *dir_entry) {
  int current_index;
  int file_count;

  if (x_step == 1) {
    /* Special case: scroll entire file window */
    /*------------------------*/
    (*start_x)++;
    DisplayFiles(ctx, ctx->active, dir_entry, *start_file,
                 *start_file + *cursor_pos, *start_x, ctx->ctx_file_window);
    if (hide_right <= 0)
      (*start_x)--;
  } else {
    current_index = *start_file + *cursor_pos;
    file_count = (int)ctx->active->file_count;

    if (current_index + x_step < file_count) {
      if (*cursor_pos + x_step < max_disp_files) {
        /* RIGHT possible without scrolling */
        *cursor_pos += x_step;
      } else {
        /* Scrolling keeps row and moves to next visible column window */
        *start_file += x_step;
      }

      DisplayFiles(ctx, ctx->active, dir_entry, *start_file,
                   *start_file + *cursor_pos, *start_x, ctx->ctx_file_window);
    }
  } /* Update dynamic header path */
  UpdateFileHeaderPath(ctx, dir_entry);
  ctx->active->start_file = *start_file;
  return;
}

static void fmoveleft(ViewContext *ctx, int *start_file, int *cursor_pos,
                      int *start_x, DirEntry *dir_entry) {
  int current_index;

  if (x_step == 1) {
    /* Special case: scroll entire file window */
    /*----------------------------------------*/
    if (*start_x > 0)
      (*start_x)--;
    DisplayFiles(ctx, ctx->active, dir_entry, *start_file,
                 *start_file + *cursor_pos, *start_x, ctx->ctx_file_window);
  } else {
    current_index = *start_file + *cursor_pos;

    if (current_index - x_step >= 0) {
      if (*cursor_pos - x_step >= 0) {
        /* LEFT possible without scrolling */
        *cursor_pos -= x_step;
      } else {
        /* Scrolling keeps row and moves to previous visible column window */
        if ((*start_file -= x_step) < 0)
          *start_file = 0;
      }

      DisplayFiles(ctx, ctx->active, dir_entry, *start_file,
                   *start_file + *cursor_pos, *start_x, ctx->ctx_file_window);
    }
  } /* Update dynamic header path */
  UpdateFileHeaderPath(ctx, dir_entry);
  ctx->active->start_file = *start_file;
  return;
}

static void fmovenpage(ViewContext *ctx, int *start_file, int *cursor_pos,
                       const int *start_x, DirEntry *dir_entry) {
  Nav_PageDown(cursor_pos, start_file, (int)ctx->active->file_count,
               max_disp_files);

  DisplayFiles(ctx, ctx->active, dir_entry, *start_file,
               *start_file + *cursor_pos, *start_x,
               ctx->ctx_file_window); /* Update dynamic header path */
  UpdateFileHeaderPath(ctx, dir_entry);
  ctx->active->start_file = *start_file;
}

static void fmoveppage(ViewContext *ctx, int *start_file, int *cursor_pos,
                       const int *start_x, DirEntry *dir_entry) {
  Nav_PageUp(cursor_pos, start_file, max_disp_files);

  DisplayFiles(ctx, ctx->active, dir_entry, *start_file,
               *start_file + *cursor_pos, *start_x,
               ctx->ctx_file_window); /* Update dynamic header path */
  UpdateFileHeaderPath(ctx, dir_entry);
  ctx->active->start_file = *start_file;
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
  BOOL volume_changed = FALSE;
  char new_name[PATH_LENGTH + 1];
  char expanded_new_name[PATH_LENGTH + 1];
  char expanded_to_file[PATH_LENGTH + 1];
  char new_login_path[PATH_LENGTH + 1];
  int get_dir_ret;
  YtreeAction action = ACTION_NONE;            /* Initialize action */
  BOOL jumped_to_owner_dir = FALSE;
  BOOL switched_panel = FALSE;
  YtreePanel *owner_panel = ctx->active;
  DirEntry *tracked_file_dir = ctx->active->file_dir_entry;
  DirEntry *last_stats_dir = NULL;             /* Track context changes */
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
    RereadWindowSize(ctx, dir_entry);

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
    if (ctx->active->vol != start_vol) {
      volume_changed = TRUE;
      action = ACTION_ESCAPE;
      goto file_window_done;
    }

    /* Keep grid metrics in sync with the current file list/rendering width.
     * This prevents stale x_step/max_disp_files after refresh/toggle/resize
     * paths that rebuild the list but do not pass through mode-change logic.
     */
    SyncFileGridMetrics(ctx);
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

    if (ctx->preview_mode) {
      action = FilterPreviewAction(action);
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
      /* Persist current file focus before cloning panel state. */
      owner_panel->file_dir_entry = dir_entry;
      owner_panel->start_file = dir_entry->start_file;
      owner_panel->file_cursor_pos = dir_entry->cursor_pos;

      if (ctx->is_split_screen && ctx->active == ctx->right) {
        ctx->left->vol = ctx->right->vol;
        ctx->left->cursor_pos = ctx->right->cursor_pos;
        ctx->left->disp_begin_pos = ctx->right->disp_begin_pos;
        ctx->left->start_file = ctx->right->start_file;
        ctx->left->file_cursor_pos = ctx->right->file_cursor_pos;
        ctx->left->file_dir_entry = ctx->right->file_dir_entry;
        ctx->left->saved_focus = ctx->right->saved_focus;
        /* Left panel now points at right panel's volume/state. Force list rebuild
         * to avoid stale file cache from a previous volume. */
        FreeFileEntryList(ctx->left);
      }
      ctx->is_split_screen = !ctx->is_split_screen;
      ReCreateWindows(ctx); /* Force layout update immediately */

      if (ctx->is_split_screen) {
        if (ctx->right && ctx->left) {
          ctx->right->vol = ctx->left->vol;
          ctx->right->cursor_pos = ctx->left->cursor_pos;
          ctx->right->disp_begin_pos = ctx->left->disp_begin_pos;
          ctx->right->start_file = dir_entry->start_file;
          ctx->right->file_cursor_pos = dir_entry->cursor_pos;
          ctx->right->file_dir_entry = dir_entry;
          /* Split from file view should preserve file focus on the new peer. */
          ctx->right->saved_focus = ctx->left->saved_focus;
          /* Right panel inherited a new volume/state; invalidate old cache so
           * first render mirrors the active panel immediately. */
          FreeFileEntryList(ctx->right);
        }
      } else {
        /* Hidden panel cache must not leak into a later re-split on another
         * volume. */
        FreeFileEntryList(ctx->right);
        ctx->active = ctx->left;
      }

      return ESC; /* Return to DirWindow to redraw */

    case ACTION_SWITCH_PANEL:
      if (!ctx->is_split_screen)
        break;
      owner_panel->file_dir_entry = dir_entry;
      owner_panel->start_file = dir_entry->start_file;
      owner_panel->file_cursor_pos = dir_entry->cursor_pos;
      ctx->active->saved_focus = FOCUS_FILE;
      switched_panel = TRUE;
      /* Ensure the CURRENT panel is cleaned up before switching context. */
      SwitchToSmallFileWindow(ctx);

      /* Switch Panel */
      if (ctx->active == ctx->left) {
        ctx->active = ctx->right;
      } else {
        ctx->active = ctx->left;
      }
      /* Update Volume Context */
      ctx->focused_window = ctx->active->saved_focus;

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
      if (x_step == 1) {
        /* In one-column layouts, LEFT/RIGHT page through the list. */
        start_x = 0;
        fmovenpage(ctx, &dir_entry->start_file, &dir_entry->cursor_pos,
                   &start_x, dir_entry);
      } else {
        fmoveright(ctx, &dir_entry->start_file, &dir_entry->cursor_pos,
                   &start_x, dir_entry);
      }
      if (ctx->preview_mode) {
        preview_line_offset = 0;
        UpdatePreview(ctx, dir_entry);
      }
      break;

    case ACTION_MOVE_LEFT:
      if (x_step == 1) {
        start_x = 0;
        fmoveppage(ctx, &dir_entry->start_file, &dir_entry->cursor_pos,
                   &start_x, dir_entry);
      } else {
        fmoveleft(ctx, &dir_entry->start_file, &dir_entry->cursor_pos,
                  &start_x, dir_entry);
      }
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
      UpdateFileHeaderPath(ctx, dir_entry);
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
      UpdateFileHeaderPath(ctx, dir_entry);
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

    case ACTION_CMD_O:
      UI_Beep(ctx, FALSE);
      break;

    case ACTION_CMD_G:
      UI_Beep(ctx, FALSE);
      break;

    case ACTION_TOGGLE_MODE:
      if (ctx->preview_mode) {
        UI_Beep(ctx, FALSE);
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

    case ACTION_COMPARE_FILE:
      if (ctx->active->file_count > 0 && ctx->active->file_entry_list) {
        int compare_idx = dir_entry->start_file + dir_entry->cursor_pos;
        if (compare_idx < 0)
          compare_idx = 0;
        if ((unsigned int)compare_idx >= ctx->active->file_count)
          compare_idx = (int)ctx->active->file_count - 1;
        if (compare_idx >= 0) {
          fe_ptr = ctx->active->file_entry_list[compare_idx].file;
          HandleFileCompare(ctx, fe_ptr);
        }
      }
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

      if (!GetRenameParameter(ctx, fe_ptr->name, new_name)) {
        /* EXPAND WILDCARDS FOR SINGLE FILE RENAME */
        BuildFilename(fe_ptr->name, new_name, expanded_new_name);

        int rename_result =
            RenameFile(ctx, fe_ptr, expanded_new_name, &new_fe_ptr);
        RefreshView(ctx, dir_entry);
        if (!rename_result) {
          /* Rename OK */
          /*-----------*/

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
      }
      if (ctx->active->vol != start_vol)
        return ESC;
      break;
    case ACTION_VOL_PREV:
      /* Panel state isolation: No vol_stats sync */

      if (CycleLoadedVolume(ctx, ctx->active, -1) == 0) {
        unput_char = '\0'; /* Ensure we return clean ESC, no pending input */
        return ESC;
      }
      if (ctx->active->vol != start_vol)
        return ESC;
      break;
    case ACTION_VOL_NEXT:
      /* Panel state isolation: No vol_stats sync */

      if (CycleLoadedVolume(ctx, ctx->active, 1) == 0) {
        unput_char = '\0'; /* Ensure we return clean ESC, no pending input */
        return ESC;
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

    case ACTION_TO_DIR:
      if (!dir_entry->global_flag) {
        UI_Beep(ctx, FALSE);
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
              : (jumped_to_owner_dir ? '\\' : ESC)); /* Return CR/ESC/ToDir */
}

/* WalkTaggedFiles moved to file_tags.c */

static void RereadWindowSize(ViewContext *ctx, DirEntry *dir_entry) {
  SyncFileGridMetrics(ctx);

  if (dir_entry->start_file + dir_entry->cursor_pos <
      (int)ctx->active->file_count) {
    while (dir_entry->cursor_pos >= max_disp_files) {
      dir_entry->start_file += x_step;
      dir_entry->cursor_pos -= x_step;
    }
  }
  return;
}

static void SyncFileGridMetrics(ViewContext *ctx) {
  int height;

  if (!ctx || !ctx->active || !ctx->ctx_file_window)
    return;

  SetPanelFileMode(ctx, ctx->active, GetPanelFileMode(ctx->active));
  height = getmaxy(ctx->ctx_file_window);
  x_step = (GetPanelMaxColumn(ctx->active) > 1) ? height : 1;
  max_disp_files = height * GetPanelMaxColumn(ctx->active);
}

static void UpdateFileHeaderPath(ViewContext *ctx, DirEntry *dir_entry) {
  char path[PATH_LENGTH];
  DirEntry *path_dir;
  int idx;

  if (!ctx || !ctx->active || !dir_entry)
    return;

  path_dir = dir_entry;

  if (ctx->active->file_count > 0 && ctx->active->file_entry_list) {
    idx = dir_entry->start_file + dir_entry->cursor_pos;
    if (idx >= 0 && (unsigned int)idx < ctx->active->file_count) {
      FileEntry *fe = ctx->active->file_entry_list[idx].file;
      if (fe && fe->dir_entry)
        path_dir = fe->dir_entry;
    }
  }

  GetPath(path_dir, path);
  DisplayHeaderPath(ctx, path);
}

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

static struct Volume *FindVolumeForDir(ViewContext *ctx,
                                       const DirEntry *target,
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
  int ch;
  int original_start_file = dir_entry->start_file;
  int original_cursor_pos = dir_entry->cursor_pos;
  int i;
  int found_idx;
  int start_x = 0;
  WINDOW *jump_win = (ctx && ctx->ctx_menu_window) ? ctx->ctx_menu_window : stdscr;

  (void)str; /* Unused */

  memset(search_buf, 0, sizeof(search_buf));

  ClearHelp(ctx);
  DrawFileListJumpPrompt(ctx, jump_win, search_buf);

  while (1) {
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
      GetFileNamePath(fe, path);
      RenderFilePreview(ctx, ctx->ctx_preview_window, path,
                        &preview_line_offset, 0);
    } else {
      RenderFilePreview(ctx, ctx->ctx_preview_window, "", &preview_line_offset,
                        0);
    }
  }
}
