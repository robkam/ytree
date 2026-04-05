/***************************************************************************
 *
 * src/ui/ctrl_dir.c
 * Directory Window Controller (Input & Event Handling)
 *
 ***************************************************************************/

#define NO_YTREE_MACROS
#include "watcher.h"
#include "ytree_cmd.h"
#include "ytree_fs.h"
#include "ytree_ui.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* TREEDEPTH uses GetProfileValue which is 2-arg in NO_YTREE_MACROS context */
#undef TREEDEPTH
#define TREEDEPTH (GetProfileValue)(ctx, "TREEDEPTH")

/* dir_list.c: BuildDirEntryList, FreeDirEntryList, FreeVolumeCache,
 *             GetPanelDirEntry, GetSelectedDirEntry */

/* dir_ops.c */
void HandlePlus(ViewContext *ctx, DirEntry *dir_entry, DirEntry *de_ptr,
                char *new_log_path, BOOL *need_dsp_help, YtreePanel *p);
void HandleReadSubTree(ViewContext *ctx, DirEntry *dir_entry,
                       BOOL *need_dsp_help, YtreePanel *p);
void HandleCollapseSubTree(ViewContext *ctx, DirEntry *dir_entry,
                           BOOL *need_dsp_help, YtreePanel *p);
void HandleUnreadSubTree(ViewContext *ctx, DirEntry *dir_entry,
                         DirEntry *de_ptr, BOOL *need_dsp_help, YtreePanel *p);

void HandleSwitchWindow(ViewContext *ctx, DirEntry *dir_entry,
                        BOOL *need_dsp_help, int *ch, YtreePanel *p);
static void DirListJump(ViewContext *ctx, DirEntry **dir_entry_ptr,
                        const Statistic *s);
static void DrawDirListJumpPrompt(ViewContext *ctx, WINDOW *win,
                                  const char *search_buf);
static void HandleDirectoryCompare(ViewContext *ctx, DirEntry *source_dir);

static BOOL IsPathInside(const char *outer, const char *candidate) {
  size_t outer_len;

  if (!outer || !candidate)
    return FALSE;
  outer_len = strlen(outer);
  if (outer_len == 0)
    return FALSE;
  if (strncmp(outer, candidate, outer_len) != 0)
    return FALSE;
  return (candidate[outer_len] == FILE_SEPARATOR_CHAR ||
          candidate[outer_len] == '\0');
}

static BOOL BuildDirOpCommand(BOOL move_dir, const char *quoted_src,
                              const char *quoted_dst, char *command_line,
                              size_t command_size) {
  const char *prefix = move_dir ? "mv -- " : "cp -a -- ";
  size_t prefix_len;
  size_t src_len;
  size_t dst_len;
  size_t total_len;
  size_t pos = 0;

  if (!quoted_src || !quoted_dst || !command_line || command_size == 0)
    return FALSE;

  prefix_len = strlen(prefix);
  src_len = strlen(quoted_src);
  dst_len = strlen(quoted_dst);
  total_len = prefix_len + src_len + 1 + dst_len + 1;
  if (total_len > command_size)
    return FALSE;

  memcpy(command_line + pos, prefix, prefix_len);
  pos += prefix_len;
  memcpy(command_line + pos, quoted_src, src_len);
  pos += src_len;
  command_line[pos++] = ' ';
  memcpy(command_line + pos, quoted_dst, dst_len);
  pos += dst_len;
  command_line[pos] = '\0';
  return TRUE;
}

static int ResolveDirTargetPath(DirEntry *dir_entry, const char *to_dir_in,
                                const char *to_file, char *src_path,
                                char *dest_path) {
  char resolved_dir[PATH_LENGTH + 1];
  const char *leaf;

  if (!dir_entry || !to_dir_in || !to_file || !*to_file || !src_path ||
      !dest_path)
    return -1;

  GetPath(dir_entry, src_path);
  src_path[PATH_LENGTH] = '\0';

  if (to_dir_in[0] == FILE_SEPARATOR_CHAR) {
    (void)snprintf(resolved_dir, sizeof(resolved_dir), "%s", to_dir_in);
  } else {
    char parent_path[PATH_LENGTH + 1];
    const char *sep;
    sep = strrchr(src_path, FILE_SEPARATOR_CHAR);
    if (!sep) {
      return -1;
    } else if (sep == src_path) {
      parent_path[0] = FILE_SEPARATOR_CHAR;
      parent_path[1] = '\0';
    } else {
      size_t parent_len = (size_t)(sep - src_path);
      if (parent_len >= sizeof(parent_path))
        return -1;
      memcpy(parent_path, src_path, parent_len);
      parent_path[parent_len] = '\0';
    }

    if (Path_Join(resolved_dir, sizeof(resolved_dir), parent_path, to_dir_in) !=
        0)
      return -1;
  }

  leaf = Path_LeafName(to_file);
  if (!leaf || !*leaf)
    return -1;

  if (Path_Join(dest_path, PATH_LENGTH + 1, resolved_dir, leaf) != 0)
    return -1;
  return 0;
}

static DirEntry *HandleDirCopyMove(ViewContext *ctx, DirEntry *dir_entry,
                                   BOOL move_dir, BOOL *need_dsp_help) {
  char to_file[PATH_LENGTH + 1];
  char to_dir[PATH_LENGTH + 1];
  char src_path[PATH_LENGTH + 1];
  char dest_path[PATH_LENGTH + 1];
  char quoted_src[PATH_LENGTH * 4 + 8];
  char quoted_dst[PATH_LENGTH * 4 + 8];
  char command_line[COMMAND_LINE_LENGTH + 1];
  struct stat st;
  int cmd_res;
  DirEntry *anchor;
  const char *prompt;

  if (!ctx || !ctx->active || !ctx->active->vol || !dir_entry)
    return dir_entry;

  if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
    UI_ShowStatusLineError(
        ctx, "Directory copy/move is only supported in filesystem mode");
    if (need_dsp_help)
      *need_dsp_help = TRUE;
    return dir_entry;
  }

  if (move_dir && dir_entry == ctx->active->vol->vol_stats.tree) {
    UI_ShowStatusLineError(ctx, "Cannot move the logged root directory");
    if (need_dsp_help)
      *need_dsp_help = TRUE;
    return dir_entry;
  }

  to_file[0] = '\0';
  to_dir[0] = '\0';
  if (move_dir) {
    if (GetMoveParameter(ctx, dir_entry->name, to_file, to_dir) != 0)
      return dir_entry;
  } else {
    if (GetCopyParameter(ctx, dir_entry->name, FALSE, to_file, to_dir) != 0)
      return dir_entry;
  }

  if (ResolveDirTargetPath(dir_entry, to_dir, to_file, src_path, dest_path) !=
      0) {
    UI_ShowStatusLineError(ctx, "Invalid destination path");
    if (need_dsp_help)
      *need_dsp_help = TRUE;
    return dir_entry;
  }

  if (strcmp(src_path, dest_path) == 0) {
    UI_ShowStatusLineError(ctx, "Source and destination are the same");
    if (need_dsp_help)
      *need_dsp_help = TRUE;
    return dir_entry;
  }
  if (IsPathInside(src_path, dest_path)) {
    UI_ShowStatusLineError(ctx,
                           "Destination cannot be inside the source directory");
    if (need_dsp_help)
      *need_dsp_help = TRUE;
    return dir_entry;
  }
  if (lstat(dest_path, &st) == 0) {
    UI_ShowStatusLineError(ctx, "Destination already exists");
    if (need_dsp_help)
      *need_dsp_help = TRUE;
    return dir_entry;
  } else if (errno != ENOENT) {
    UI_ShowStatusLineError(ctx, "Cannot access destination path");
    if (need_dsp_help)
      *need_dsp_help = TRUE;
    return dir_entry;
  }

  if (!Path_ShellQuote(src_path, quoted_src, sizeof(quoted_src)) ||
      !Path_ShellQuote(dest_path, quoted_dst, sizeof(quoted_dst))) {
    UI_ShowStatusLineError(ctx, "Path too long");
    if (need_dsp_help)
      *need_dsp_help = TRUE;
    return dir_entry;
  }

  if (!BuildDirOpCommand(move_dir, quoted_src, quoted_dst, command_line,
                         sizeof(command_line))) {
    UI_ShowStatusLineError(ctx, "Command too long");
    if (need_dsp_help)
      *need_dsp_help = TRUE;
    return dir_entry;
  }

  if (move_dir) {
    prompt = "Move directory now (Y/N) ? ";
  } else {
    prompt = "Copy directory now (Y/N) ? ";
  }

  if (UI_ChoiceResolver(ctx, prompt, "YN\033") != 'Y')
    return dir_entry;

  cmd_res = SystemCall(ctx, command_line, &ctx->active->vol->vol_stats);
  if (cmd_res != 0) {
    UI_ShowStatusLineError(ctx, move_dir ? "Directory move failed"
                                         : "Directory copy failed");
    if (need_dsp_help)
      *need_dsp_help = TRUE;
    return dir_entry;
  }

  anchor = dir_entry->up_tree ? dir_entry->up_tree
                              : ctx->active->vol->vol_stats.tree;
  dir_entry = RefreshTreeSafe(ctx, ctx->active, anchor);
  RefreshView(ctx, dir_entry);
  if (need_dsp_help)
    *need_dsp_help = TRUE;
  return dir_entry;
}

static BOOL ExitArchiveRootToParent(ViewContext *ctx, DirEntry **dir_entry_ptr,
                                    Statistic **s_ptr,
                                    const struct Volume **start_vol_ptr,
                                    BOOL retain_archive_volume,
                                    BOOL focus_file) {
  struct Volume *old_vol;
  char archive_path[PATH_LENGTH + 1];
  char parent_dir[PATH_LENGTH + 1];
  char archive_name[PATH_LENGTH + 1];
  int target_file_idx = -1;
  int visible_rows = 1;
  int file_idx;

  if (!ctx || !ctx->active || !ctx->active->vol || !dir_entry_ptr ||
      !*dir_entry_ptr || !s_ptr || !*s_ptr || !start_vol_ptr) {
    return FALSE;
  }

  old_vol = ctx->active->vol;

  (void)snprintf(archive_path, sizeof(archive_path), "%s", (*s_ptr)->log_path);
  Fnsplit(archive_path, parent_dir, archive_name);

  if (LogDisk(ctx, ctx->active, parent_dir) != 0) {
    return FALSE;
  }

  if (!retain_archive_volume) {
    BOOL vol_in_use = FALSE;
    if (ctx->is_split_screen) {
      const YtreePanel *other =
          (ctx->active == ctx->left) ? ctx->right : ctx->left;
      if (other && other->vol == old_vol)
        vol_in_use = TRUE;
    }
    if (!vol_in_use)
      Volume_Delete(ctx, old_vol);
  }

  *s_ptr = &ctx->active->vol->vol_stats;
  *start_vol_ptr = ctx->active->vol;
  ctx->global_search_term[0] = '\0';

  if (ctx->active->vol->total_dirs > 0) {
    *dir_entry_ptr = ctx->active->vol
                         ->dir_entry_list[ctx->active->disp_begin_pos +
                                          ctx->active->cursor_pos]
                         .dir_entry;
  } else {
    *dir_entry_ptr = (*s_ptr)->tree;
  }

  (*dir_entry_ptr)->start_file = 0;
  (*dir_entry_ptr)->cursor_pos = focus_file ? 0 : -1;

  BuildFileEntryList(ctx, ctx->active);
  if (ctx->ctx_small_file_window) {
    visible_rows = getmaxy(ctx->ctx_small_file_window);
  } else if (ctx->ctx_file_window) {
    visible_rows = getmaxy(ctx->ctx_file_window);
  }
  if (visible_rows < 1)
    visible_rows = 1;

  for (file_idx = 0; file_idx < (int)ctx->active->file_count; file_idx++) {
    const FileEntry *fe = ctx->active->file_entry_list[file_idx].file;
    if (fe && strcmp(fe->name, archive_name) == 0) {
      target_file_idx = file_idx;
      break;
    }
  }

  if (target_file_idx >= visible_rows) {
    (*dir_entry_ptr)->start_file = target_file_idx - (visible_rows - 1);
  }
  if ((*dir_entry_ptr)->start_file >= (int)ctx->active->file_count) {
    (*dir_entry_ptr)->start_file = MAXIMUM(0, (int)ctx->active->file_count - 1);
  }
  if ((*dir_entry_ptr)->start_file < 0) {
    (*dir_entry_ptr)->start_file = 0;
  }

  if (focus_file && target_file_idx >= 0 && ctx->active->file_count > 0) {
    int file_cursor = target_file_idx - (*dir_entry_ptr)->start_file;
    if (file_cursor < 0)
      file_cursor = 0;
    if (file_cursor >= visible_rows)
      file_cursor = visible_rows - 1;
    (*dir_entry_ptr)->cursor_pos = file_cursor;
    ctx->focused_window = FOCUS_FILE;
    ctx->active->saved_focus = FOCUS_FILE;
  } else {
    (*dir_entry_ptr)->cursor_pos = -1;
    ctx->focused_window = FOCUS_TREE;
    ctx->active->saved_focus = FOCUS_TREE;
  }

  ctx->active->file_dir_entry = *dir_entry_ptr;
  ctx->active->start_file = (*dir_entry_ptr)->start_file;
  ctx->active->file_cursor_pos = (*dir_entry_ptr)->cursor_pos;

  ctx->view_mode = ctx->active->vol->vol_stats.log_mode;
  RefreshView(ctx, *dir_entry_ptr);
  return TRUE;
}

static void RestorePanelFileSelection(DirEntry *dir_entry, YtreePanel *panel) {
  if (!dir_entry || !panel)
    return;

  if (panel->saved_focus != FOCUS_FILE)
    return;

  if (panel->file_dir_entry != dir_entry)
    return;

  dir_entry->start_file = panel->start_file;
  dir_entry->cursor_pos = panel->file_cursor_pos;
  if (dir_entry->start_file < 0)
    dir_entry->start_file = 0;
  if (dir_entry->cursor_pos < 0 && dir_entry->total_files > 0)
    dir_entry->cursor_pos = 0;
}

static void HandleDirectoryCompare(ViewContext *ctx, DirEntry *source_dir) {
  CompareMenuChoice menu_choice = COMPARE_MENU_CANCEL;
  CompareFlowType flow_type = COMPARE_FLOW_DIRECTORY;
  CompareRequest request;
  BOOL external_launch = FALSE;

  if (!ctx || !source_dir)
    return;

  if (UI_SelectCompareMenuChoice(ctx, &menu_choice) != 0)
    return;

  if (menu_choice == COMPARE_MENU_DIRECTORY_PLUS_TREE) {
    flow_type = COMPARE_FLOW_LOGGED_TREE;
  } else if (menu_choice == COMPARE_MENU_EXTERNAL_DIRECTORY) {
    flow_type = COMPARE_FLOW_DIRECTORY;
    external_launch = TRUE;
  } else if (menu_choice == COMPARE_MENU_EXTERNAL_TREE) {
    flow_type = COMPARE_FLOW_LOGGED_TREE;
    external_launch = TRUE;
  } else if (menu_choice != COMPARE_MENU_DIRECTORY_ONLY) {
    return;
  }

  if (external_launch) {
    DirCompare_LaunchExternal(ctx, source_dir, flow_type);
    RefreshView(ctx, source_dir);
    return;
  }

  if (UI_BuildDirectoryCompareRequest(ctx, source_dir, flow_type, &request) !=
      0) {
    return;
  }

  if (flow_type == COMPARE_FLOW_DIRECTORY) {
    DirCompare_RunInternalDirectory(ctx, source_dir, &request);
    return;
  }

  DirCompare_RunInternalLoggedTree(ctx, &request);
}

int HandleDirWindow(ViewContext *ctx, const DirEntry *start_dir_entry) {
  DirEntry *dir_entry, *de_ptr;
  int ch, unput_char;
  BOOL need_dsp_help;
  char new_name[PATH_LENGTH + 1];
  char new_log_path[PATH_LENGTH + 1];
  YtreeAction action; /* Declare YtreeAction variable */
  const struct Volume *start_vol = NULL;
  Statistic *s = NULL;
  int height;
  char watcher_path[PATH_LENGTH + 1];

  DEBUG_LOG("HandleDirWindow: Recalculating layout");
  Layout_Recalculate(ctx);
  DEBUG_LOG("HandleDirWindow: Calling DisplayMenu");
  DisplayMenu(ctx);

  unput_char = 0;
  de_ptr = NULL;

  /* Safety fallback if Init() has not set up panels */
  if (ctx->active == NULL)
    ctx->active = ctx->left;
  if (ctx->active == NULL || ctx->active->vol == NULL)
    return ESC;

  start_vol = ctx->active->vol;
  s = &ctx->active->vol->vol_stats;

  if (ctx->active) {
    DEBUG_LOG("HandleDirWindow: Syncing panel state");
    ctx->focused_window = ctx->active->saved_focus;

    /* Update Global View Context to match Active Panel */
    /* This ensures macros like ctx->ctx_dir_window resolve to the
     * correct ncurses window
     */
    ctx->ctx_dir_window = ctx->active->pan_dir_window;
    ctx->ctx_small_file_window = ctx->active->pan_small_file_window;
    ctx->ctx_big_file_window = ctx->active->pan_big_file_window;
    ctx->ctx_file_window = ctx->active->pan_file_window;
  }

  /* Safety Reset for Preview Mode */
  if (ctx->preview_mode) {
    DEBUG_LOG("HandleDirWindow: Resetting preview mode");
    ctx->preview_mode = FALSE;
    ReCreateWindows(ctx);
    DisplayMenu(ctx);
    /* Update context again after ReCreateWindows */
    if (ctx->active) {
      ctx->ctx_dir_window = ctx->active->pan_dir_window;
      ctx->ctx_small_file_window = ctx->active->pan_small_file_window;
      ctx->ctx_big_file_window = ctx->active->pan_big_file_window;
      ctx->ctx_file_window = ctx->active->pan_file_window;
    }
  }

  height = getmaxy(ctx->ctx_dir_window);
  DEBUG_LOG("HandleDirWindow: Window height=%d", height);

  /* Clear flags */
  /*-----------------*/

  SetDirMode(ctx, MODE_3);

  need_dsp_help = TRUE;

  DEBUG_LOG("HandleDirWindow: Building DirEntryList for vol=%p",
            (void *)ctx->active->vol);
  BuildDirEntryList(ctx, ctx->active->vol, &ctx->active->current_dir_entry);
  if (ctx->initial_directory != NULL) {
    if (!strcmp(ctx->initial_directory, ".")) /* Entry just a single "." */
    {
      ctx->active->disp_begin_pos = 0;
      ctx->active->cursor_pos = 0;
      unput_char = CR;
    } else {
      int log_path_len = -1;
      if (*ctx->initial_directory == '.') { /* Entry of form "./alpha/beta" */
        log_path_len =
            snprintf(new_log_path, sizeof(new_log_path), "%s%s",
                     start_dir_entry->name, ctx->initial_directory + 1);
      } else if (*ctx->initial_directory == '~') {
        const char *home = getenv("HOME");
        if (home) {
          /* Entry of form "~/alpha/beta" */
          log_path_len = snprintf(new_log_path, sizeof(new_log_path), "%s%s",
                                  home, ctx->initial_directory + 1);
        } else {
          /* Entry of form "beta" or "/full/path/alpha/beta" */
          log_path_len = snprintf(new_log_path, sizeof(new_log_path), "%s",
                                  ctx->initial_directory);
        }
      } else { /* Entry of form "beta" or "/full/path/alpha/beta" */
        log_path_len = snprintf(new_log_path, sizeof(new_log_path), "%s",
                                ctx->initial_directory);
      }
      if (log_path_len >= 0 && (size_t)log_path_len < sizeof(new_log_path)) {
        int i;
        for (i = 0; i < ctx->active->vol->total_dirs; i++) {
          if (*new_log_path == FILE_SEPARATOR_CHAR) {
            GetPath(ctx->active->vol->dir_entry_list[i].dir_entry, new_name);
          } else {
            (void)snprintf(new_name, sizeof(new_name), "%s",
                           ctx->active->vol->dir_entry_list[i].dir_entry->name);
          }
          if (!strcmp(new_log_path, new_name)) {
            ctx->active->disp_begin_pos = i;
            ctx->active->cursor_pos = 0;
            unput_char = CR;
            break;
          }
        }
      }
    }
    ctx->initial_directory = NULL;
  }

  {
    int safe_idx = ctx->active->disp_begin_pos + ctx->active->cursor_pos;
    if (ctx->active->vol->total_dirs <= 0) {
      ctx->active->disp_begin_pos = 0;
      ctx->active->cursor_pos = 0;
      safe_idx = 0;
    } else if (safe_idx < 0 || safe_idx >= ctx->active->vol->total_dirs) {
      ctx->active->disp_begin_pos = 0;
      ctx->active->cursor_pos = 0;
      safe_idx = 0;
    }

    if (ctx->active->vol->dir_entry_list) {
      dir_entry = ctx->active->vol->dir_entry_list[safe_idx].dir_entry;
    } else {
      dir_entry = ctx->active->vol->vol_stats.tree;
    }
  }

  /* REPLACEMENT START: Unified Rendering Call */
  if (!dir_entry->log_flag) {
    if (ctx->active && ctx->active->saved_focus == FOCUS_FILE) {
      if (ctx->active->file_dir_entry == dir_entry) {
        dir_entry->start_file = ctx->active->start_file;
        dir_entry->cursor_pos = ctx->active->file_cursor_pos;
      }
      if (dir_entry->start_file < 0)
        dir_entry->start_file = 0;
      if (dir_entry->cursor_pos < 0 && dir_entry->total_files > 0)
        dir_entry->cursor_pos = 0;
    } else {
      dir_entry->start_file = 0;
      dir_entry->cursor_pos = -1;
    }
  }

  RefreshView(ctx, dir_entry);
  /* REPLACEMENT END */

  if (dir_entry->log_flag) {
    if ((dir_entry->global_flag) || (dir_entry->tagged_flag)) {
      unput_char = 'S';
    } else {
      unput_char = CR;
    }
  } else if (ctx->active && ctx->active->saved_focus == FOCUS_FILE &&
             dir_entry->total_files > 0) {
    unput_char = CR;
  }
  do {
    /* Detect Global Volume Change (Split Brain Fix) */
    if (ctx->active == NULL || ctx->active->vol == NULL ||
        ctx->active->vol != start_vol)
      return ESC;

    if (need_dsp_help || ctx->view_mode == ARCHIVE_MODE) {
      need_dsp_help = FALSE;
      DisplayDirHelp(ctx, dir_entry);
    }
    DisplayDirParameter(ctx, dir_entry);
    RefreshWindow(ctx->ctx_dir_window);

    if (ctx->is_split_screen) {
      YtreePanel *inactive =
          (ctx->active == ctx->left) ? ctx->right : ctx->left;
      RenderInactivePanel(ctx, inactive);
    }

    if (s->log_mode == DISK_MODE || s->log_mode == USER_MODE) {
      if (ctx->refresh_mode & REFRESH_WATCHER) {
        GetPath(dir_entry, watcher_path);
        Watcher_SetDir(ctx, watcher_path);
      }
    }

    if (unput_char) {
      ch = unput_char;
      unput_char = '\0';
    } else {
      doupdate();
      ch = (ctx->resize_request) ? -1 : GetEventOrKey(ctx);
      /* LF to CR normalization is now handled by GetKeyAction */
    }

    if (IsUserActionDefined(ctx)) { /* User commands take precedence */
      ch = DirUserMode(ctx, dir_entry, ch, &ctx->active->vol->vol_stats);
    }

    /* ViKey processing is now handled inside GetKeyAction */

    if (ctx->resize_request) {
      /* SIMPLIFIED RESIZE: Just call Global Refresh */
      RefreshView(ctx, dir_entry);
      need_dsp_help = TRUE;
      ctx->resize_request = FALSE;
    }

    action = GetKeyAction(ctx, ch); /* Translate raw input to YtreeAction */

    switch (action) {
    case ACTION_RESIZE:
      ctx->resize_request = TRUE;
      break;

    case ACTION_EDIT_CONFIG:
      UI_OpenConfigProfile(ctx, dir_entry);
      need_dsp_help = TRUE;
      break;

    case ACTION_TOGGLE_STATS:
      ctx->show_stats = !ctx->show_stats;
      ctx->resize_request = TRUE;
      break;

    case ACTION_VIEW_PREVIEW: {
      const YtreePanel *saved_panel = ctx->active;

      /* Save Preview State - BEFORE potential panel switching */
      ctx->preview_return_panel = ctx->active;
      ctx->preview_return_focus = ctx->focused_window;

      ctx->preview_mode = TRUE;
      /* ADDED INSTRUCTION: Track Entry Point */
      ctx->preview_entry_focus = FOCUS_TREE;

      /* Enter File Window loop via HandleSwitchWindow logic */
      /* We use HandleSwitchWindow to encapsulate the setup for HandleFileWindow
       */
      HandleSwitchWindow(ctx, dir_entry, &need_dsp_help, &ch, ctx->active);

      /* Post-Return Cleanup */
      ctx->preview_mode = FALSE; /* Restore cleaning of preview mode flag */
      ctx->focused_window = FOCUS_TREE; /* Restore focus to tree on return */

      if (ctx->active != saved_panel) {
        /* If panel was switched during preview, we REFRESH local state
         * BEFORE calling RefreshView to ensure the correct entry is
         * drawn. */
        if (ctx->active->vol == NULL)
          return ESC;
        start_vol = ctx->active->vol;
        s = &ctx->active->vol->vol_stats;

        if (ctx->active->vol && ctx->active->vol->total_dirs > 0) {
          dir_entry = ctx->active->vol
                          ->dir_entry_list[ctx->active->disp_begin_pos +
                                           ctx->active->cursor_pos]
                          .dir_entry;
        } else {
          dir_entry = s->tree;
        }

        /* Sync Global View Context */
        ctx->ctx_dir_window = ctx->active->pan_dir_window;
        ctx->ctx_small_file_window = ctx->active->pan_small_file_window;
        ctx->ctx_big_file_window = ctx->active->pan_big_file_window;
        ctx->ctx_file_window = ctx->active->pan_file_window;

        /* EXPLICIT REFRESH with the CORRECT dir_entry */
        RefreshView(ctx, dir_entry);

        need_dsp_help = TRUE;
        continue;
      }

      /* Standard refresh if panel didn't change */
      RefreshView(ctx, dir_entry);

      need_dsp_help = TRUE;

      /* Refresh local pointer */
      if (ctx->active->vol->total_dirs > 0) {
        dir_entry = ctx->active->vol
                        ->dir_entry_list[ctx->active->disp_begin_pos +
                                         ctx->active->cursor_pos]
                        .dir_entry;
      } else {
        dir_entry = s->tree;
      }
    } break;

    case ACTION_SPLIT_SCREEN:
      if (ctx->is_split_screen && ctx->active == ctx->right) {
        /* Preserve Right Panel state into Left Panel before unsplitting */
        ctx->left->vol = ctx->right->vol;
        ctx->left->cursor_pos = ctx->right->cursor_pos;
        ctx->left->disp_begin_pos = ctx->right->disp_begin_pos;
        ctx->left->start_file = ctx->right->start_file;
        ctx->left->file_cursor_pos = ctx->right->file_cursor_pos;
        ctx->left->file_dir_entry = ctx->right->file_dir_entry;
        ctx->left->saved_big_file_view = ctx->right->saved_big_file_view;
        ctx->left->saved_focus = ctx->right->saved_focus;
        /* Left panel now follows right panel state; force rebuild to avoid
         * stale file list from an older volume. */
        FreeFileEntryList(ctx->left);
        /* Sync Global Volume */
        ctx->active->vol = ctx->left->vol;
      }

      ctx->is_split_screen = !ctx->is_split_screen;
      ReCreateWindows(ctx); /* Force layout update immediately */

      if (ctx->is_split_screen) {
        if (ctx->right && ctx->left) {
          ctx->right->vol = ctx->left->vol;
          ctx->right->cursor_pos = ctx->left->cursor_pos;
          ctx->right->disp_begin_pos = ctx->left->disp_begin_pos;
          ctx->right->start_file = ctx->left->start_file;
          ctx->right->file_cursor_pos = ctx->left->file_cursor_pos;
          ctx->right->file_dir_entry = ctx->left->file_dir_entry;
          ctx->right->saved_big_file_view = ctx->left->saved_big_file_view;
          /* Start a newly split peer panel in tree focus by default. */
          ctx->right->saved_focus = FOCUS_TREE;
          /* Right panel inherited a new volume/state; drop stale cache so
           * inactive render shows the correct mirror immediately. */
          FreeFileEntryList(ctx->right);
        }
      } else {
        /* Prevent hidden peer cache from leaking across later volume splits. */
        FreeFileEntryList(ctx->right);
        ctx->active = ctx->left;
      }
      ctx->focused_window = ctx->active->saved_focus;
      RefreshView(ctx, dir_entry);
      need_dsp_help = TRUE;
      break;

    case ACTION_SWITCH_PANEL:
      if (!ctx->is_split_screen)
        break;

      /* Save local state to Active Panel for persistence before switching */
      /* Note: Navigation actions already update ctx->active->cursor_pos
       * directly. Do NOT overwrite with stale local_cursor_pos! */

      /* Switch Panel */
      if (ctx->active == ctx->left) {
        ctx->active = ctx->right;
      } else {
        ctx->active = ctx->left;
      }

      /* Restore Volume context */
      ctx->focused_window = ctx->active->saved_focus;
      s = &ctx->active->vol->vol_stats; /* UPDATE S */
      /* Do NOT overwrite ctx->active->cursor_pos here; preserve its
       * independent state */

      /* Recalculate local dir_entry for the new ctx->active */
      if (ctx->active->vol->total_dirs > 0) {
        /* Clamp coordinates just in case */
        if (ctx->active->disp_begin_pos + ctx->active->cursor_pos >=
            ctx->active->vol->total_dirs) {
          ctx->active->cursor_pos =
              ctx->active->vol->total_dirs - 1 - ctx->active->disp_begin_pos;
        }
        dir_entry = ctx->active->vol
                        ->dir_entry_list[ctx->active->disp_begin_pos +
                                         ctx->active->cursor_pos]
                        .dir_entry;
      } else {
        dir_entry = s->tree;
      }
      DEBUG_LOG("ACTION_SWITCH_PANEL: active panel is now %s with "
                "cursor_pos=%d, dir_entry=%s",
                ctx->active == ctx->left ? "LEFT" : "RIGHT",
                ctx->active->cursor_pos, dir_entry ? dir_entry->name : "NULL");

      /* Sync Global View Context */
      ctx->ctx_dir_window = ctx->active->pan_dir_window;
      ctx->ctx_small_file_window = ctx->active->pan_small_file_window;
      ctx->ctx_big_file_window = ctx->active->pan_big_file_window;
      ctx->ctx_file_window = ctx->active->pan_file_window;

      /* REFRESH View for new active panel */
      RestorePanelFileSelection(dir_entry, ctx->active);
      RefreshView(ctx, dir_entry);
      need_dsp_help = TRUE;
      if (ctx->focused_window == FOCUS_FILE && dir_entry->total_files > 0) {
        unput_char = CR;
      }
      continue; /* Stay in loop with new context */

    case ACTION_NONE: /* -1 or unhandled keys */
      if (ch == -1)
        break; /* Ignore -1 (ctx->resize_request handled above) */
      /* Fall through for other unhandled keys to beep */
      UI_Beep(ctx, FALSE);
      break;

    case ACTION_MOVE_DOWN:
      DirNav_Movedown(ctx, &dir_entry, ctx->active);
      break;
    case ACTION_MOVE_UP:
      DirNav_Moveup(ctx, &dir_entry, ctx->active);
      break;
    case ACTION_MOVE_SIBLING_NEXT: {
      const DirEntry *target = dir_entry->next;
      if (target == NULL) {
        /* Wrap to first sibling */
        if (dir_entry->up_tree != NULL) {
          target = dir_entry->up_tree->sub_tree;
        }
      }

      if (target != NULL && target != dir_entry) {
        /* Find the sibling in the linear list to update cursor index */
        int k;
        int found_idx = -1;

        for (k = 0; k < ctx->active->vol->total_dirs; k++) {
          if (ctx->active->vol->dir_entry_list[k].dir_entry == target) {
            found_idx = k;
            break;
          }
        }

        if (found_idx != -1) {
          /* Move cursor to sibling */
          if (found_idx >= ctx->active->disp_begin_pos &&
              found_idx < ctx->active->disp_begin_pos + height) {
            ctx->active->cursor_pos = found_idx - ctx->active->disp_begin_pos;
          } else {
            /* Off screen, center it or move to top */
            ctx->active->disp_begin_pos = found_idx;
            ctx->active->cursor_pos = 0;
            /* Bounds check */
            if (ctx->active->disp_begin_pos + height >
                ctx->active->vol->total_dirs) {
              ctx->active->disp_begin_pos =
                  MAXIMUM(0, ctx->active->vol->total_dirs - height);
              ctx->active->cursor_pos = found_idx - ctx->active->disp_begin_pos;
            }
          }
          /* Sync */
          dir_entry = ctx->active->vol
                          ->dir_entry_list[ctx->active->disp_begin_pos +
                                           ctx->active->cursor_pos]
                          .dir_entry;

          if (0) {
            dir_entry = RefreshTreeSafe(ctx, ctx->active, dir_entry);
            break; /* Skip manual refresh logic below */
          }

          /* Refresh */
          DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                      ctx->active->disp_begin_pos,
                      ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                      TRUE);
          DisplayFileWindow(ctx, ctx->active, dir_entry);
          DisplayDiskStatistic(ctx, s);
          UpdateStatsPanel(ctx, dir_entry, s);
          DisplayAvailBytes(ctx, s);

          char path[PATH_LENGTH];
          GetPath(dir_entry, path);
          DisplayHeaderPath(ctx, path);
        }
      }
    }
      need_dsp_help = TRUE;
      break;
    case ACTION_MOVE_SIBLING_PREV: {
      DirEntry *target = dir_entry->prev;
      if (target == NULL) {
        /* Wrap to last sibling */
        if (dir_entry->up_tree != NULL) {
          target = dir_entry->up_tree->sub_tree;
          while (target && target->next != NULL) {
            target = target->next;
          }
        }
      }

      if (target != NULL && target != dir_entry) {
        /* Find the sibling in the linear list to update cursor index */
        int k;
        int found_idx = -1;

        for (k = 0; k < ctx->active->vol->total_dirs; k++) {
          if (ctx->active->vol->dir_entry_list[k].dir_entry == target) {
            found_idx = k;
            break;
          }
        }

        if (found_idx != -1) {
          /* Move cursor to sibling */
          if (found_idx >= ctx->active->disp_begin_pos &&
              found_idx < ctx->active->disp_begin_pos + height) {
            ctx->active->cursor_pos = found_idx - ctx->active->disp_begin_pos;
          } else {
            /* Off screen, center it or move to top */
            ctx->active->disp_begin_pos = found_idx;
            ctx->active->cursor_pos = 0;
            /* Bounds check */
            if (ctx->active->disp_begin_pos + height >
                ctx->active->vol->total_dirs) {
              ctx->active->disp_begin_pos =
                  MAXIMUM(0, ctx->active->vol->total_dirs - height);
              ctx->active->cursor_pos = found_idx - ctx->active->disp_begin_pos;
            }
          }
          /* Sync */
          dir_entry = ctx->active->vol
                          ->dir_entry_list[ctx->active->disp_begin_pos +
                                           ctx->active->cursor_pos]
                          .dir_entry;

          if (0) {
            dir_entry = RefreshTreeSafe(ctx, ctx->active, dir_entry);
            break; /* Skip manual refresh logic below */
          }

          /* Refresh */
          DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                      ctx->active->disp_begin_pos,
                      ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                      TRUE);
          DisplayFileWindow(ctx, ctx->active, dir_entry);
          DisplayDiskStatistic(ctx, s);
          UpdateStatsPanel(ctx, dir_entry, s);
          DisplayAvailBytes(ctx, s);

          char path[PATH_LENGTH];
          GetPath(dir_entry, path);
          DisplayHeaderPath(ctx, path);
        }
      }
    }
      need_dsp_help = TRUE;
      break;
    case ACTION_PAGE_DOWN:
      DirNav_Movenpage(ctx, &dir_entry, ctx->active);
      break;
    case ACTION_PAGE_UP:
      DirNav_Moveppage(ctx, &dir_entry, ctx->active);
      break;
    case ACTION_HOME:
      DirNav_MoveHome(ctx, &dir_entry, ctx->active);
      break;
    case ACTION_END:
      DirNav_MoveEnd(ctx, &dir_entry, ctx->active);
      break;
    case ACTION_MOVE_RIGHT:
    case ACTION_TREE_EXPAND_ALL:
      HandlePlus(ctx, dir_entry, de_ptr, new_log_path, &need_dsp_help,
                 ctx->active);
      break;
    case ACTION_ASTERISK:
      HandleReadSubTree(ctx, dir_entry, &need_dsp_help, ctx->active);
      break;
    case ACTION_TREE_EXPAND:
      HandleReadSubTree(ctx, dir_entry, &need_dsp_help, ctx->active);
      break;
    case ACTION_MOVE_LEFT:
      if (!dir_entry->not_scanned && dir_entry->sub_tree != NULL) {
        HandleCollapseSubTree(ctx, dir_entry, &need_dsp_help, ctx->active);
        break;
      }

      if (dir_entry->up_tree == NULL) {
        /* FS root boundary no-op. */
        break;
      }

      {
        int p_idx = -1;
        int k;
        for (k = 0; k < ctx->active->vol->total_dirs; k++) {
          if (ctx->active->vol->dir_entry_list[k].dir_entry ==
              dir_entry->up_tree) {
            p_idx = k;
            break;
          }
        }
        if (p_idx != -1) {
          if (p_idx >= ctx->active->disp_begin_pos &&
              p_idx < ctx->active->disp_begin_pos + height) {
            ctx->active->cursor_pos = p_idx - ctx->active->disp_begin_pos;
          } else {
            ctx->active->disp_begin_pos = p_idx;
            ctx->active->cursor_pos = 0;
            if (ctx->active->disp_begin_pos + height >
                ctx->active->vol->total_dirs) {
              ctx->active->disp_begin_pos =
                  MAXIMUM(0, ctx->active->vol->total_dirs - height);
              ctx->active->cursor_pos = p_idx - ctx->active->disp_begin_pos;
            }
          }
          dir_entry = ctx->active->vol
                          ->dir_entry_list[ctx->active->disp_begin_pos +
                                           ctx->active->cursor_pos]
                          .dir_entry;

          DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                      ctx->active->disp_begin_pos,
                      ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                      TRUE);
          DisplayFileWindow(ctx, ctx->active, dir_entry);
          DisplayDiskStatistic(ctx, s);
          UpdateStatsPanel(ctx, dir_entry, s);
          DisplayAvailBytes(ctx, s);
          {
            char path[PATH_LENGTH];
            GetPath(dir_entry, path);
            DisplayHeaderPath(ctx, path);
          }
        }
      }
      break;
    case ACTION_TO_DIR:
      if (ctx->view_mode != ARCHIVE_MODE) {
        break;
      }
      if (dir_entry->up_tree != NULL) {
        DirEntry *archive_root = dir_entry;
        int root_idx = -1;
        int k;

        while (archive_root->up_tree != NULL) {
          int parent_idx = -1;
          for (k = 0; k < ctx->active->vol->total_dirs; k++) {
            if (ctx->active->vol->dir_entry_list[k].dir_entry ==
                archive_root->up_tree) {
              parent_idx = k;
              break;
            }
          }
          if (parent_idx < 0) {
            break;
          }
          archive_root = archive_root->up_tree;
        }
        for (k = 0; k < ctx->active->vol->total_dirs; k++) {
          if (ctx->active->vol->dir_entry_list[k].dir_entry == archive_root) {
            root_idx = k;
            break;
          }
        }
        if (root_idx >= 0) {
          if (root_idx >= ctx->active->disp_begin_pos &&
              root_idx < ctx->active->disp_begin_pos + height) {
            ctx->active->cursor_pos = root_idx - ctx->active->disp_begin_pos;
          } else {
            ctx->active->disp_begin_pos = root_idx;
            ctx->active->cursor_pos = 0;
            if (ctx->active->disp_begin_pos + height >
                ctx->active->vol->total_dirs) {
              ctx->active->disp_begin_pos =
                  MAXIMUM(0, ctx->active->vol->total_dirs - height);
              ctx->active->cursor_pos = root_idx - ctx->active->disp_begin_pos;
            }
          }

          dir_entry = ctx->active->vol
                          ->dir_entry_list[ctx->active->disp_begin_pos +
                                           ctx->active->cursor_pos]
                          .dir_entry;

          DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                      ctx->active->disp_begin_pos,
                      ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                      TRUE);
          DisplayFileWindow(ctx, ctx->active, dir_entry);
          DisplayDiskStatistic(ctx, s);
          UpdateStatsPanel(ctx, dir_entry, s);
          DisplayAvailBytes(ctx, s);
          {
            char path[PATH_LENGTH];
            GetPath(dir_entry, path);
            DisplayHeaderPath(ctx, path);
          }
          DisplayDirHelp(ctx, dir_entry);
        }
        break;
      }
      if (ExitArchiveRootToParent(ctx, &dir_entry, &s, &start_vol, TRUE,
                                  TRUE)) {
        need_dsp_help = TRUE;
        unput_char = CR;
      } else {
        MESSAGE(ctx, "Can't exit archive root.");
        need_dsp_help = TRUE;
      }
      break;
    case ACTION_TREE_COLLAPSE:
      if (!dir_entry->not_scanned && dir_entry->sub_tree != NULL) {
        HandleCollapseSubTree(ctx, dir_entry, &need_dsp_help, ctx->active);
      } else {
        HandleUnreadSubTree(ctx, dir_entry, de_ptr, &need_dsp_help,
                            ctx->active);
      }
      break;
    case ACTION_TOGGLE_HIDDEN: {
      ToggleDotFiles(ctx, ctx->active);

      /* Update current dir pointer using the new accessor function
      because ToggleDotFiles might have changed the list layout */
      if (ctx->active->vol->total_dirs > 0) {
        dir_entry = ctx->active->vol
                        ->dir_entry_list[ctx->active->disp_begin_pos +
                                         ctx->active->cursor_pos]
                        .dir_entry;
      } else {
        dir_entry = s->tree;
      }

      need_dsp_help = TRUE;
    } break;
    case ACTION_FILTER:
      if (UI_ReadFilter(ctx) == 0) {
        RecalculateSysStats(ctx, s);
        dir_entry->start_file = 0;
        dir_entry->cursor_pos = -1;
        RefreshView(ctx, dir_entry);
      }
      need_dsp_help = TRUE;
      break;
    case ACTION_LIST_JUMP:
      DirListJump(ctx, &dir_entry, s);
      need_dsp_help = TRUE;
      break;
    case ACTION_TAG:
    case ACTION_UNTAG:
    case ACTION_TAG_ALL:
    case ACTION_UNTAG_ALL:
    case ACTION_CMD_TAGGED_S:
      if (HandleDirTagActions(ctx, action, &dir_entry, &need_dsp_help, &ch)) {
        break;
      }
      break;

    case ACTION_TOGGLE_MODE:
      RotateDirMode(ctx);
      /*DisplayFileWindow(ctx,  dir_entry, 0, -1 );*/
      DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                  ctx->active->disp_begin_pos,
                  ctx->active->disp_begin_pos + ctx->active->cursor_pos, TRUE);
      /*RefreshWindow( ctx->ctx_file_window );*/
      DisplayDiskStatistic(ctx, s);
      UpdateStatsPanel(ctx, dir_entry, s);
      need_dsp_help = TRUE;
      break;

    case ACTION_CMD_S:
      HandleShowAll(ctx, FALSE, FALSE, dir_entry, &need_dsp_help, &ch,
                    ctx->active);
      break;
    case ACTION_COMPARE_DIR:
      HandleDirectoryCompare(ctx, dir_entry);
      need_dsp_help = TRUE;
      break;
    case ACTION_COMPARE_TREE:
      HandleDirectoryCompare(ctx, dir_entry);
      need_dsp_help = TRUE;
      break;
    case ACTION_ENTER:
      if (dir_entry == NULL) {
        UI_Beep(ctx, FALSE);
        break;
      }

      if (ctx->refresh_mode & REFRESH_ON_ENTER) {
        dir_entry = RefreshTreeSafe(ctx, ctx->active, dir_entry);
        /* Sync pointer from list in case address changed */
        dir_entry = ctx->active->vol
                        ->dir_entry_list[ctx->active->disp_begin_pos +
                                         ctx->active->cursor_pos]
                        .dir_entry;
      }

      DEBUG_LOG("ACTION_ENTER: dir_entry=%p name=%s matching=%u",
                (void *)dir_entry, dir_entry ? dir_entry->name : "NULL",
                dir_entry ? (unsigned int)dir_entry->matching_files : 0U);

      if (dir_entry == NULL || dir_entry->total_files == 0) {
        UI_Beep(ctx, FALSE);
        break;
      }

      /* Fix Context Safety on Return */
      {
        const YtreePanel *saved_panel = ctx->active;

        /* Note: Navigation actions already update active->cursor_pos. */

        HandleSwitchWindow(ctx, dir_entry, &need_dsp_help, &ch, ctx->active);

        /* Restore whichever focus the active panel had before/after the
         * switch. */
        ctx->focused_window = ctx->active->saved_focus;

        if (ctx->active != saved_panel) {
          /* If panel was switched, refresh state locally and continue */
          if (ctx->active->vol == NULL)
            return ESC;
          start_vol = ctx->active->vol;
          s = &ctx->active->vol->vol_stats;

          if (ctx->active->vol && ctx->active->vol->total_dirs > 0) {
            dir_entry = ctx->active->vol
                            ->dir_entry_list[ctx->active->disp_begin_pos +
                                             ctx->active->cursor_pos]
                            .dir_entry;
          } else {
            dir_entry = s->tree;
          }

          ctx->ctx_dir_window = ctx->active->pan_dir_window;
          ctx->ctx_small_file_window = ctx->active->pan_small_file_window;
          ctx->ctx_big_file_window = ctx->active->pan_big_file_window;
          ctx->ctx_file_window = ctx->active->pan_file_window;

          RestorePanelFileSelection(dir_entry, ctx->active);
          RefreshView(ctx, dir_entry);
          need_dsp_help = TRUE;
          if (ctx->focused_window == FOCUS_FILE && dir_entry->total_files > 0) {
            unput_char = CR;
          }
          action = ACTION_NONE; /* Prevent while(...) condition from exiting
                                   HandleDirWindow! */
          continue;
        }

        /* Important: Check for volume changes after returning from File Window
         */
        if (ctx->active->vol != start_vol)
          return ESC;

        /* CENTRALIZED REFRESH: Restore clean layout after return */
        RefreshView(ctx, dir_entry);
      }
      /* Refresh local pointer */
      dir_entry = ctx->active->vol
                      ->dir_entry_list[ctx->active->disp_begin_pos +
                                       ctx->active->cursor_pos]
                      .dir_entry;
      break;
    case ACTION_CMD_X:
      if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
      } else {
        char command_template[COMMAND_LINE_LENGTH + 1];
        command_template[0] = '\0';
        if (GetCommandLine(ctx, command_template) == 0) {
          (void)Execute(ctx, dir_entry, NULL, command_template,
                        &ctx->active->vol->vol_stats, UI_ArchiveCallback);
          dir_entry = RefreshTreeSafe(
              ctx, ctx->active, dir_entry); /* Auto-Refresh after command */
        }
      }
      need_dsp_help = TRUE;
      DisplayAvailBytes(ctx, s);
      DisplayDiskStatistic(ctx, s);
      UpdateStatsPanel(ctx, dir_entry, s);
      break;
    case ACTION_CMD_MKFILE:
      DEBUG_LOG("ACTION_CMD_MKFILE reached in ctrl_dir.c. mode=%d",
                ctx->view_mode);
      if (ctx->view_mode != DISK_MODE)
        break;
      {
        char file_name[PATH_LENGTH * 2 + 1];

        ClearHelp(ctx);
        *file_name = '\0';
        if (UI_ReadString(ctx, ctx->active, "MAKE FILE:", file_name,
                          PATH_LENGTH, HST_FILE) == CR) {
          int mk_result =
              MakeFile(ctx, dir_entry, file_name, s, NULL, UI_ChoiceResolver);
          if (mk_result == 0) {
            /* Determine where the new file should be. */
            if (ctx->active && ctx->active->pan_file_window) {
              /* Force file window refresh */
              DisplayFileWindow(ctx, ctx->active, dir_entry);
            }
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

    case ACTION_CMD_M: {
      char dir_name[PATH_LENGTH * 2 + 1];
      ClearHelp(ctx);
      *dir_name = '\0';
      if (UI_ReadString(ctx, ctx->active, "MAKE DIRECTORY:", dir_name,
                        PATH_LENGTH, HST_FILE) == CR) {
        if (!MakeDirectory(ctx, ctx->active, dir_entry, dir_name, s)) {
          BuildDirEntryList(ctx, ctx->active->vol,
                            &ctx->active->current_dir_entry);
          RefreshView(ctx, dir_entry);
        }
      }
      wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
      wclrtoeol(ctx->ctx_border_window);
      wnoutrefresh(ctx->ctx_border_window);
    }
      need_dsp_help = TRUE;
      break;
    case ACTION_CMD_D:
      if (!DeleteDirectory(ctx, dir_entry, UI_ChoiceResolver)) {
        if (ctx->active->disp_begin_pos + ctx->active->cursor_pos > 0) {
          if (ctx->active->cursor_pos > 0)
            ctx->active->cursor_pos--;
          else
            ctx->active->disp_begin_pos--;
        }
      }
      /* Update regardless of success */
      BuildDirEntryList(ctx, ctx->active->vol, &ctx->active->current_dir_entry);
      dir_entry = ctx->active->vol
                      ->dir_entry_list[ctx->active->disp_begin_pos +
                                       ctx->active->cursor_pos]
                      .dir_entry;
      dir_entry->start_file = 0;
      dir_entry->cursor_pos = -1;

      RefreshView(ctx, dir_entry);
      need_dsp_help = TRUE;
      break;

    case ACTION_CMD_R:
      if (!GetRenameParameter(ctx, dir_entry->name, new_name)) {
        int rename_result = RenameDirectory(ctx, dir_entry, new_name);
        if (!rename_result) {
          /* Rename OK */
          BuildDirEntryList(ctx, ctx->active->vol,
                            &ctx->active->current_dir_entry);
          dir_entry = ctx->active->vol
                          ->dir_entry_list[ctx->active->disp_begin_pos +
                                           ctx->active->cursor_pos]
                          .dir_entry;
        }
        RefreshView(ctx, dir_entry);
      }
      need_dsp_help = TRUE;
      break;
    case ACTION_REFRESH: /* Rescan */
      dir_entry = RefreshTreeSafe(ctx, ctx->active, dir_entry);
      need_dsp_help = TRUE;
      break;
    case ACTION_CMD_G:
      HandleShowAll(ctx, FALSE, TRUE, dir_entry, &need_dsp_help, &ch,
                    ctx->active);
      break;
    case ACTION_CMD_C:
      dir_entry = HandleDirCopyMove(ctx, dir_entry, FALSE, &need_dsp_help);
      break;
    case ACTION_CMD_V:
      dir_entry = HandleDirCopyMove(ctx, dir_entry, TRUE, &need_dsp_help);
      break;
    case ACTION_CMD_O:
      UI_Beep(ctx, FALSE);
      break;
    case ACTION_CMD_A:
      if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
        UI_Beep(ctx, FALSE);
        break;
      }
      need_dsp_help = TRUE;
      switch (UI_PromptAttributeAction(ctx, FALSE, TRUE)) {
      case 'M':
        if (ChangeDirModus(ctx, dir_entry))
          break;
        DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                    ctx->active->disp_begin_pos,
                    ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                    TRUE);
        DisplayDiskStatistic(ctx, s);
        UpdateStatsPanel(ctx, dir_entry, s);
        break;
      case 'O':
        if (HandleDirOwnership(ctx, dir_entry, TRUE, FALSE))
          break;
        DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                    ctx->active->disp_begin_pos,
                    ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                    TRUE);
        DisplayDiskStatistic(ctx, s);
        UpdateStatsPanel(ctx, dir_entry, s);
        break;
      case 'G':
        if (HandleDirOwnership(ctx, dir_entry, FALSE, TRUE))
          break;
        DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                    ctx->active->disp_begin_pos,
                    ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                    TRUE);
        DisplayDiskStatistic(ctx, s);
        UpdateStatsPanel(ctx, dir_entry, s);
        break;
      case 'D':
        if (ChangeDirDate(ctx, dir_entry))
          break;
        DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                    ctx->active->disp_begin_pos,
                    ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                    TRUE);
        DisplayDiskStatistic(ctx, s);
        UpdateStatsPanel(ctx, dir_entry, s);
        break;
      default:
        break;
      }
      break;

    case ACTION_CMD_I: {
      ArchivePayload payload;
      int gather_result;
      payload.original_source_list = NULL;
      payload.expanded_file_list = NULL;

      gather_result = UI_GatherArchivePayload(ctx, dir_entry, NULL, &payload);
      if (gather_result != 0) {
        if (gather_result < 0)
          UI_ShowStatusLineError(ctx, "Nothing to archive");
        need_dsp_help = FALSE;
      } else {
        int create_result;
        create_result = UI_CreateArchiveFromPayload(ctx, &payload);
        if (create_result == 0) {
          dir_entry = RefreshTreeSafe(ctx, ctx->active, dir_entry);
          need_dsp_help = TRUE;
        } else if (create_result < 0) {
          need_dsp_help = FALSE;
        } else {
          need_dsp_help = TRUE;
        }
      }
      UI_FreeArchivePayload(&payload);
    } break;

    case ACTION_TOGGLE_COMPACT:
      ctx->fixed_col_width = (ctx->fixed_col_width == 0) ? 32 : 0;
      ctx->resize_request = TRUE;
      break;

    case ACTION_CMD_P: /* Pipe Directory */
    {
      char pipe_cmd[PATH_LENGTH + 1];
      pipe_cmd[0] = '\0';
      if (GetPipeCommand(ctx, pipe_cmd) == 0) {
        PipeDirectory(ctx, dir_entry, pipe_cmd);
      }
    }
      need_dsp_help = TRUE;
      break;

    case ACTION_CMD_PRINT: /* Print Directory */
      UI_HandlePrintController(ctx, dir_entry, FALSE);
      need_dsp_help = TRUE;
      break;

    /* Volume Cycling and Selection */
    case ACTION_VOL_MENU: /* Shift-K: Select Loaded Volume */
    {
      /* Save current panel state to volume before switching away */
      /* Panel state isolation: No vol_stats sync */
      ctx->active->vol->saved_tree_index =
          ctx->active->disp_begin_pos + ctx->active->cursor_pos;

      int res = SelectLoadedVolume(ctx, NULL);
      if (res == 0) { /* If volume switch was successful */
        if (ctx->active->vol != start_vol)
          return ESC; /* Abort to main loop to handle clean re-entry */

        /* Update loop variables for new volume */
        s = &ctx->active->vol->vol_stats; /* UPDATE S */
        /* Panel isolation: No vol_stats sync */
        ctx->active->disp_begin_pos =
            ctx->active->vol->saved_tree_index /* legacy removed */;

        /* Safety check / Clamping */
        if (ctx->active->vol->total_dirs > 0) {
          /* If saved position is beyond current end, clamp to last item */
          if (ctx->active->disp_begin_pos + ctx->active->cursor_pos >=
              ctx->active->vol->total_dirs) {
            /* Clamp to last valid index */
            int last_idx = ctx->active->vol->total_dirs - 1;
            /* Determine new disp_begin_pos and cursor_pos */
            if (last_idx >= ctx->layout.dir_win_height) {
              ctx->active->disp_begin_pos =
                  last_idx - (ctx->layout.dir_win_height - 1);
              ctx->active->cursor_pos = ctx->layout.dir_win_height - 1;
            } else {
              ctx->active->disp_begin_pos = 0;
              ctx->active->cursor_pos = last_idx;
            }
          }
          /* Now safe to assign dir_entry */
          dir_entry = ctx->active->vol
                          ->dir_entry_list[ctx->active->disp_begin_pos +
                                           ctx->active->cursor_pos]
                          .dir_entry;
        } else {
          dir_entry = s->tree;
        }

        DisplayMenu(ctx); /* Force redraw of frame/separator */
        DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                    ctx->active->disp_begin_pos,
                    ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                    TRUE);
        DisplayFileWindow(
            ctx, ctx->active,
            dir_entry); /* Refresh file window for the new directory */
        RefreshWindow(ctx->ctx_file_window);
        DisplayDiskStatistic(ctx, s);
        UpdateStatsPanel(ctx, dir_entry, s);
        DisplayAvailBytes(ctx, s);
        /* Update header path after volume switch */
        {
          char path[PATH_LENGTH];
          GetPath(dir_entry, path);
          DisplayHeaderPath(ctx, path);
        }
        need_dsp_help = TRUE;
      } else {
        /* Cancelled volume menu clears footer/help; redraw immediately. */
        DisplayMenu(ctx);
        need_dsp_help = TRUE;
      }
    } break;

    case ACTION_VOL_PREV: /* Previous Volume */
    {
      /* Save current panel state to volume before switching away */
      /* Panel isolation: No vol_stats sync */
      ctx->active->vol->saved_tree_index =
          ctx->active->disp_begin_pos + ctx->active->cursor_pos;

      int res = CycleLoadedVolume(ctx, ctx->active, -1);
      if (res == 0) { /* If volume switch was successful */
        if (ctx->active->vol != start_vol)
          return ESC;

        s = &ctx->active->vol->vol_stats; /* UPDATE S */
        /* Panel isolation: No vol_stats sync */
        ctx->active->disp_begin_pos =
            ctx->active->vol->saved_tree_index /* legacy removed */;

        /* Safety check / Clamping */
        if (ctx->active->vol->total_dirs > 0) {
          if (ctx->active->disp_begin_pos + ctx->active->cursor_pos >=
              ctx->active->vol->total_dirs) {
            int last_idx = ctx->active->vol->total_dirs - 1;
            if (last_idx >= ctx->layout.dir_win_height) {
              ctx->active->disp_begin_pos =
                  last_idx - (ctx->layout.dir_win_height - 1);
              ctx->active->cursor_pos = ctx->layout.dir_win_height - 1;
            } else {
              ctx->active->disp_begin_pos = 0;
              ctx->active->cursor_pos = last_idx;
            }
          }
          dir_entry = ctx->active->vol
                          ->dir_entry_list[ctx->active->disp_begin_pos +
                                           ctx->active->cursor_pos]
                          .dir_entry;
        } else {
          dir_entry = s->tree;
        }

        DisplayMenu(ctx);
        DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                    ctx->active->disp_begin_pos,
                    ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                    TRUE);
        DisplayFileWindow(ctx, ctx->active, dir_entry);
        RefreshWindow(ctx->ctx_file_window);
        DisplayDiskStatistic(ctx, s);
        UpdateStatsPanel(ctx, dir_entry, s);
        DisplayAvailBytes(ctx, s);
        {
          char path[PATH_LENGTH];
          GetPath(dir_entry, path);
          DisplayHeaderPath(ctx, path);
        }
        need_dsp_help = TRUE;
      }
    } break;

    case ACTION_VOL_NEXT: /* Next Volume */
    {
      /* Save current panel state to volume before switching away */
      /* Panel isolation: No vol_stats sync */
      ctx->active->vol->saved_tree_index =
          ctx->active->disp_begin_pos + ctx->active->cursor_pos;

      int res = CycleLoadedVolume(ctx, ctx->active, 1);
      if (res == 0) { /* If volume switch was successful */
        if (ctx->active->vol != start_vol)
          return ESC;

        s = &ctx->active->vol->vol_stats; /* UPDATE S */
        /* Panel isolation: No vol_stats sync */
        ctx->active->disp_begin_pos =
            ctx->active->vol->saved_tree_index /* legacy removed */;

        if (ctx->active->vol->total_dirs > 0) {
          if (ctx->active->disp_begin_pos + ctx->active->cursor_pos >=
              ctx->active->vol->total_dirs) {
            int last_idx = ctx->active->vol->total_dirs - 1;
            if (last_idx >= ctx->layout.dir_win_height) {
              ctx->active->disp_begin_pos =
                  last_idx - (ctx->layout.dir_win_height - 1);
              ctx->active->cursor_pos = ctx->layout.dir_win_height - 1;
            } else {
              ctx->active->disp_begin_pos = 0;
              ctx->active->cursor_pos = last_idx;
            }
          }
          dir_entry = ctx->active->vol
                          ->dir_entry_list[ctx->active->disp_begin_pos +
                                           ctx->active->cursor_pos]
                          .dir_entry;
        } else {
          dir_entry = s->tree;
        }

        DisplayMenu(ctx);
        DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                    ctx->active->disp_begin_pos,
                    ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                    TRUE);
        DisplayFileWindow(ctx, ctx->active, dir_entry);
        RefreshWindow(ctx->ctx_file_window);
        DisplayDiskStatistic(ctx, s);
        UpdateStatsPanel(ctx, dir_entry, s);
        DisplayAvailBytes(ctx, s);
        {
          char path[PATH_LENGTH];
          GetPath(dir_entry, path);
          DisplayHeaderPath(ctx, path);
        }
        need_dsp_help = TRUE;
      }
    } break;

    case ACTION_QUIT_DIR:
      need_dsp_help = TRUE;
      QuitTo(ctx, dir_entry);
      break;

    case ACTION_QUIT:
      need_dsp_help = TRUE;
      Quit(ctx);
      action = ACTION_NONE;
      break;

    case ACTION_LOG:
      if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
        if (getcwd(new_log_path, sizeof(new_log_path)) == NULL) {
          (void)snprintf(new_log_path, sizeof(new_log_path), "%s", ".");
        }
      } else {
        (void)GetPath(dir_entry, new_log_path);
      }
      if (!GetNewLogPath(ctx, ctx->active, new_log_path)) {
        int ret; /* DEBUG variable */
        DisplayMenu(ctx);
        doupdate();

        ret = LogDisk(ctx, ctx->active, new_log_path);

        /* Check return value. Only update state if log succeeded (0). */
        if (ret == 0) {
          /* Safety Check: If volume was changed, return to main loop to handle
           * new context */
          if (ctx->active->vol != start_vol)
            return ESC;

          s = &ctx->active->vol
                   ->vol_stats; /* Update stats pointer for new volume */

          /* Safety check / Clamping */
          if (ctx->active->vol->total_dirs > 0) {
            if (ctx->active->disp_begin_pos + ctx->active->cursor_pos >=
                ctx->active->vol->total_dirs) {
              int last_idx = ctx->active->vol->total_dirs - 1;
              if (last_idx >= ctx->layout.dir_win_height) {
                ctx->active->disp_begin_pos =
                    last_idx - (ctx->layout.dir_win_height - 1);
                ctx->active->cursor_pos = ctx->layout.dir_win_height - 1;
              } else {
                ctx->active->disp_begin_pos = 0;
                ctx->active->cursor_pos = last_idx;
              }
            }
            /* Now safe to assign dir_entry */
            dir_entry = ctx->active->vol
                            ->dir_entry_list[ctx->active->disp_begin_pos +
                                             ctx->active->cursor_pos]
                            .dir_entry;
          } else {
            dir_entry = s->tree;
          }

          DisplayMenu(ctx); /* Redraw menu/frame */

          /* Force Full Display Refresh */
          DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                      ctx->active->disp_begin_pos,
                      ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                      TRUE);
          DisplayFileWindow(ctx, ctx->active, dir_entry);
          RefreshWindow(ctx->ctx_file_window);
          DisplayDiskStatistic(ctx, s);
          DisplayDirStatistic(ctx, dir_entry, NULL, s);
          DisplayAvailBytes(ctx, s);
          /* Update header path */
          {
            char path[PATH_LENGTH];
            GetPath(dir_entry, path);
            DisplayHeaderPath(ctx, path);
          }
        }
        need_dsp_help = TRUE;
      }
      break;
    /* Ctrl-L is now ACTION_REFRESH, handled above */
    default: /* Unhandled action, beep */
      UI_Beep(ctx, FALSE);
      break;
    } /* switch */

    /* Refresh dir_entry pointer in case tree was rescanned/reallocated during
     * action */
    if (ctx->active && ctx->active->vol && ctx->active->vol->dir_entry_list) {
      int safe_idx = ctx->active->disp_begin_pos + ctx->active->cursor_pos;
      if (safe_idx < 0)
        safe_idx = 0;
      if (safe_idx >= ctx->active->vol->total_dirs)
        safe_idx = ctx->active->vol->total_dirs - 1;
      if (safe_idx >= 0) {
        dir_entry = ctx->active->vol->dir_entry_list[safe_idx].dir_entry;
      } else {
        dir_entry = ctx->active->vol->vol_stats.tree;
      }
    }

  } while (action != ACTION_QUIT && action != ACTION_ENTER &&
           action != ACTION_ESCAPE &&
           action != ACTION_LOG); /* Loop until explicit quit, escape or log */

  /* Sync state back to Volume on exit */
  /* Removed shared state sync on exit */

  return (ch); /* Return the last raw character that caused exit */
}

static void DirListJump(ViewContext *ctx, DirEntry **dir_entry_ptr,
                        const Statistic *s) {
  char search_buf[256];
  int buf_len = 0;
  int i;
  int found_idx;
  int height;
  int original_disp_begin_pos;
  int original_cursor_pos;
  WINDOW *jump_win =
      (ctx && ctx->ctx_menu_window) ? ctx->ctx_menu_window : stdscr;

  if (!ctx || !ctx->active || !ctx->active->vol ||
      !ctx->active->vol->dir_entry_list || ctx->active->vol->total_dirs <= 0 ||
      !dir_entry_ptr || !*dir_entry_ptr) {
    return;
  }

  height = getmaxy(ctx->ctx_dir_window);
  if (height <= 0)
    return;

  original_disp_begin_pos = ctx->active->disp_begin_pos;
  original_cursor_pos = ctx->active->cursor_pos;

  memset(search_buf, 0, sizeof(search_buf));

  ClearHelp(ctx);
  DrawDirListJumpPrompt(ctx, jump_win, search_buf);

  while (1) {
    int ch;

    DrawDirListJumpPrompt(ctx, jump_win, search_buf);

    ch = Getch(ctx);
    if (ch == -1)
      break;

    if (ch == ESC) {
      ctx->active->disp_begin_pos = original_disp_begin_pos;
      ctx->active->cursor_pos = original_cursor_pos;
    } else if (ch == CR || ch == LF) {
      break;
    } else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b' || ch == KEY_DC) {
      if (buf_len > 0) {
        buf_len--;
        search_buf[buf_len] = '\0';

        if (buf_len == 0) {
          ctx->active->disp_begin_pos = original_disp_begin_pos;
          ctx->active->cursor_pos = original_cursor_pos;
        } else {
          found_idx = -1;
          for (i = 0; i < ctx->active->vol->total_dirs; i++) {
            DirEntry *candidate = ctx->active->vol->dir_entry_list[i].dir_entry;
            if (candidate &&
                strncasecmp(candidate->name, search_buf, buf_len) == 0) {
              found_idx = i;
              break;
            }
          }

          if (found_idx != -1) {
            if (found_idx >= ctx->active->disp_begin_pos &&
                found_idx < ctx->active->disp_begin_pos + height) {
              ctx->active->cursor_pos = found_idx - ctx->active->disp_begin_pos;
            } else {
              ctx->active->disp_begin_pos = found_idx;
              ctx->active->cursor_pos = 0;
              if (ctx->active->disp_begin_pos + height >
                  ctx->active->vol->total_dirs) {
                ctx->active->disp_begin_pos =
                    MAXIMUM(0, ctx->active->vol->total_dirs - height);
                ctx->active->cursor_pos =
                    found_idx - ctx->active->disp_begin_pos;
              }
            }
          }
        }
      }
    } else if (isprint(ch)) {
      if (buf_len < (int)sizeof(search_buf) - 1) {
        search_buf[buf_len] = ch;
        search_buf[buf_len + 1] = '\0';

        found_idx = -1;
        for (i = 0; i < ctx->active->vol->total_dirs; i++) {
          DirEntry *candidate = ctx->active->vol->dir_entry_list[i].dir_entry;
          if (candidate && strncasecmp(candidate->name, search_buf,
                                       (size_t)buf_len + 1) == 0) {
            found_idx = i;
            break;
          }
        }

        if (found_idx != -1) {
          buf_len++;
          if (found_idx >= ctx->active->disp_begin_pos &&
              found_idx < ctx->active->disp_begin_pos + height) {
            ctx->active->cursor_pos = found_idx - ctx->active->disp_begin_pos;
          } else {
            ctx->active->disp_begin_pos = found_idx;
            ctx->active->cursor_pos = 0;
            if (ctx->active->disp_begin_pos + height >
                ctx->active->vol->total_dirs) {
              ctx->active->disp_begin_pos =
                  MAXIMUM(0, ctx->active->vol->total_dirs - height);
              ctx->active->cursor_pos = found_idx - ctx->active->disp_begin_pos;
            }
          }
        } else {
          /* Sticky cursor: keep current selection when input has no match. */
          buf_len++;
        }
      }
    }

    if (ctx->active->cursor_pos < 0)
      ctx->active->cursor_pos = 0;

    if (ctx->active->disp_begin_pos + ctx->active->cursor_pos >=
        ctx->active->vol->total_dirs) {
      int last_idx = ctx->active->vol->total_dirs - 1;
      if (last_idx < 0)
        last_idx = 0;
      if (last_idx >= height) {
        ctx->active->disp_begin_pos = last_idx - (height - 1);
        ctx->active->cursor_pos = height - 1;
      } else {
        ctx->active->disp_begin_pos = 0;
        ctx->active->cursor_pos = last_idx;
      }
    }

    *dir_entry_ptr = ctx->active->vol
                         ->dir_entry_list[ctx->active->disp_begin_pos +
                                          ctx->active->cursor_pos]
                         .dir_entry;

    DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                ctx->active->disp_begin_pos,
                ctx->active->disp_begin_pos + ctx->active->cursor_pos, TRUE);
    DisplayFileWindow(ctx, ctx->active, *dir_entry_ptr);
    DisplayDiskStatistic(ctx, s);
    UpdateStatsPanel(ctx, *dir_entry_ptr, s);
    DisplayAvailBytes(ctx, s);
    {
      char path[PATH_LENGTH];
      GetPath(*dir_entry_ptr, path);
      DisplayHeaderPath(ctx, path);
    }
    RefreshWindow(ctx->ctx_dir_window);
    RefreshWindow(ctx->ctx_file_window);
    doupdate();

    if (ch == ESC)
      break;
  }
}

static void DrawDirListJumpPrompt(ViewContext *ctx, WINDOW *win,
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
