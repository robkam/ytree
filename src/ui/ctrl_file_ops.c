/***************************************************************************
 *
 * src/ui/ctrl_file_ops.c
 * File Action Handlers (Tagging, Move, Copy, Delete dispatch)
 *
 ***************************************************************************/

#define NO_YTREE_MACROS
#include "ytree_cmd.h"
#include "ytree_fs.h"
#include "ytree_ui.h"
#include <utime.h>

static void ResetPreviewAfterNavigation(
    ViewContext *ctx, DirEntry *dir_entry, long *preview_line_offset_ptr,
    void (*update_preview)(ViewContext *, const DirEntry *)) {
  if (!ctx->preview_mode)
    return;
  *preview_line_offset_ptr = 0;
  if (update_preview)
    update_preview(ctx, dir_entry);
}

/* =========================================================================
 * Shared UI Helpers for Post-Action Refresh, Sync, and Render
 * ========================================================================= */

void UI_RefreshSyncPanels(ViewContext *ctx, DirEntry *dir_entry) {
  RefreshDirWindow(ctx, ctx->active);
  if (ctx->is_split_screen) {
    RefreshDirWindow(ctx, (ctx->active == ctx->left) ? ctx->right : ctx->left);
  }
  RefreshView(ctx, dir_entry);
}

void UI_RenderFilePanel(ViewContext *ctx, const DirEntry *dir_entry,
                        int start_x) {
  DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
               dir_entry->start_file + dir_entry->cursor_pos, start_x,
               ctx->ctx_file_window);
}

static void RebuildActiveFileListAfterMutation(ViewContext *ctx,
                                               DirEntry *dir_entry) {
  int file_count;

  if (!ctx || !ctx->active || !dir_entry)
    return;

  BuildFileEntryList(ctx, ctx->active);

  file_count = (int)ctx->active->file_count;
  if (file_count <= 0) {
    dir_entry->start_file = 0;
    dir_entry->cursor_pos = 0;
  } else {
    int max_disp_files;

    max_disp_files = FileNav_GetMaxDispFiles(ctx);
    if (max_disp_files < 1)
      max_disp_files = 1;

    if (dir_entry->start_file < 0)
      dir_entry->start_file = 0;
    if (dir_entry->start_file >= file_count)
      dir_entry->start_file = file_count - 1;
    if (dir_entry->cursor_pos < 0)
      dir_entry->cursor_pos = 0;
    if (dir_entry->cursor_pos >= max_disp_files)
      dir_entry->cursor_pos = max_disp_files - 1;

    if (dir_entry->start_file + dir_entry->cursor_pos >= file_count) {
      dir_entry->start_file = MAXIMUM(0, file_count - max_disp_files);
      dir_entry->cursor_pos = file_count - 1 - dir_entry->start_file;
    }
  }

  ctx->active->start_file = dir_entry->start_file;
  ctx->active->file_cursor_pos = dir_entry->cursor_pos;
}

BOOL handle_file_window_preview_action(
    ViewContext *ctx, YtreeAction action, DirEntry **dir_entry_ptr,
    YtreeAction *loop_action_ptr, Statistic **stats_ptr,
    struct Volume **start_vol_ptr, BOOL *need_dsp_help_ptr,
    long *preview_line_offset_ptr, int *saved_fixed_width_ptr,
    void (*update_preview)(ViewContext *, const DirEntry *)) {
  DirEntry *dir_entry;

  if (!ctx || !dir_entry_ptr)
    return FALSE;

  dir_entry = *dir_entry_ptr;
  switch (action) {
  case ACTION_VIEW_PREVIEW:
    if (!ctx->preview_mode) {
      ctx->preview_return_panel = ctx->active;
      ctx->preview_return_focus = ctx->focused_window;
    }

    ctx->preview_mode = !ctx->preview_mode;
    if (ctx->preview_mode) {
      *saved_fixed_width_ptr = ctx->fixed_col_width;
      ReCreateWindows(ctx);
      ctx->fixed_col_width = ctx->layout.big_file_win_width - 2;
      if (ctx->fixed_col_width < 1)
        ctx->fixed_col_width = 1;
      FileNav_RereadWindowSize(ctx, dir_entry);
    } else {
      Statistic *stats_local;

      ctx->active = ctx->preview_return_panel;
      ctx->focused_window = ctx->preview_return_focus;
      stats_local = &ctx->active->vol->vol_stats;
      if (stats_ptr)
        *stats_ptr = stats_local;
      if (start_vol_ptr)
        *start_vol_ptr = ctx->active->vol;

      if (ctx->active->vol->total_dirs > 0) {
        int idx = ctx->active->disp_begin_pos + ctx->active->cursor_pos;
        if (idx >= ctx->active->vol->total_dirs)
          idx = ctx->active->vol->total_dirs - 1;
        if (idx < 0)
          idx = 0;
        dir_entry = ctx->active->vol->dir_entry_list[idx].dir_entry;
      } else {
        dir_entry = stats_local->tree;
      }

      ctx->ctx_dir_window = ctx->active->pan_dir_window;
      ctx->ctx_small_file_window = ctx->active->pan_small_file_window;
      ctx->ctx_big_file_window = ctx->active->pan_big_file_window;
      ctx->ctx_file_window = ctx->active->pan_file_window;

      ctx->fixed_col_width = *saved_fixed_width_ptr;
      ReCreateWindows(ctx);
      FileNav_RereadWindowSize(ctx, dir_entry);
      RefreshView(ctx, dir_entry);
    }

    RefreshView(ctx, dir_entry);
    if (ctx->preview_mode && update_preview)
      update_preview(ctx, dir_entry);

    if (!ctx->preview_mode && loop_action_ptr) {
      if (ctx->preview_return_focus == FOCUS_TREE)
        *loop_action_ptr = ACTION_ESCAPE;
      else
        *loop_action_ptr = ACTION_NONE;
    }

    *need_dsp_help_ptr = TRUE;
    *dir_entry_ptr = dir_entry;
    return TRUE;

  case ACTION_PREVIEW_SCROLL_DOWN:
    if (ctx->preview_mode) {
      (*preview_line_offset_ptr)++;
      if (update_preview)
        update_preview(ctx, dir_entry);
    }
    return TRUE;

  case ACTION_PREVIEW_SCROLL_UP:
    if (ctx->preview_mode) {
      if (*preview_line_offset_ptr > 0)
        (*preview_line_offset_ptr)--;
      if (update_preview)
        update_preview(ctx, dir_entry);
    }
    return TRUE;

  case ACTION_PREVIEW_HOME:
    if (ctx->preview_mode) {
      *preview_line_offset_ptr = 0;
      if (update_preview)
        update_preview(ctx, dir_entry);
    }
    return TRUE;

  case ACTION_PREVIEW_END:
    if (ctx->preview_mode) {
      *preview_line_offset_ptr = 2000000000L;
      if (update_preview)
        update_preview(ctx, dir_entry);
    }
    return TRUE;

  case ACTION_PREVIEW_PAGE_UP:
    if (ctx->preview_mode) {
      *preview_line_offset_ptr -= (getmaxy(ctx->ctx_preview_window) - 1);
      if (*preview_line_offset_ptr < 0)
        *preview_line_offset_ptr = 0;
      if (update_preview)
        update_preview(ctx, dir_entry);
    }
    return TRUE;

  case ACTION_PREVIEW_PAGE_DOWN:
    if (ctx->preview_mode) {
      *preview_line_offset_ptr += (getmaxy(ctx->ctx_preview_window) - 1);
      if (update_preview)
        update_preview(ctx, dir_entry);
    }
    return TRUE;

  default:
    return FALSE;
  }
}

BOOL handle_file_window_navigation_action(
    ViewContext *ctx, YtreeAction action, DirEntry *dir_entry, int *start_x_ptr,
    BOOL *need_dsp_help_ptr, long *preview_line_offset_ptr,
    void (*update_preview)(ViewContext *, const DirEntry *),
    void (*list_jump)(ViewContext *, DirEntry *, char *)) {
  if (!ctx || !dir_entry || !start_x_ptr)
    return FALSE;

  switch (action) {
  case ACTION_MOVE_DOWN:
    FileNav_MoveDown(ctx, dir_entry, *start_x_ptr);
    ResetPreviewAfterNavigation(ctx, dir_entry, preview_line_offset_ptr,
                                update_preview);
    return TRUE;

  case ACTION_MOVE_UP:
    FileNav_MoveUp(ctx, dir_entry, *start_x_ptr);
    ResetPreviewAfterNavigation(ctx, dir_entry, preview_line_offset_ptr,
                                update_preview);
    return TRUE;

  case ACTION_MOVE_RIGHT:
    if (FileNav_GetXStep(ctx) == 1) {
      *start_x_ptr = 0;
      FileNav_PageDown(ctx, dir_entry, *start_x_ptr);
    } else {
      FileNav_MoveRight(ctx, dir_entry, start_x_ptr);
    }
    ResetPreviewAfterNavigation(ctx, dir_entry, preview_line_offset_ptr,
                                update_preview);
    return TRUE;

  case ACTION_MOVE_LEFT:
    if (FileNav_GetXStep(ctx) == 1) {
      *start_x_ptr = 0;
      FileNav_PageUp(ctx, dir_entry, *start_x_ptr);
    } else {
      FileNav_MoveLeft(ctx, dir_entry, start_x_ptr);
    }
    ResetPreviewAfterNavigation(ctx, dir_entry, preview_line_offset_ptr,
                                update_preview);
    return TRUE;

  case ACTION_PAGE_DOWN:
    FileNav_PageDown(ctx, dir_entry, *start_x_ptr);
    ResetPreviewAfterNavigation(ctx, dir_entry, preview_line_offset_ptr,
                                update_preview);
    return TRUE;

  case ACTION_PAGE_UP:
    FileNav_PageUp(ctx, dir_entry, *start_x_ptr);
    ResetPreviewAfterNavigation(ctx, dir_entry, preview_line_offset_ptr,
                                update_preview);
    return TRUE;

  case ACTION_END:
    Nav_End(&dir_entry->cursor_pos, &dir_entry->start_file,
            (int)ctx->active->file_count, FileNav_GetMaxDispFiles(ctx));
    UI_RenderFilePanel(ctx, dir_entry, *start_x_ptr);
    FileNav_UpdateHeaderPath(ctx, dir_entry);
    ResetPreviewAfterNavigation(ctx, dir_entry, preview_line_offset_ptr,
                                update_preview);
    ctx->active->start_file = dir_entry->start_file;
    return TRUE;

  case ACTION_HOME:
    Nav_Home(&dir_entry->cursor_pos, &dir_entry->start_file);
    UI_RenderFilePanel(ctx, dir_entry, *start_x_ptr);
    FileNav_UpdateHeaderPath(ctx, dir_entry);
    ResetPreviewAfterNavigation(ctx, dir_entry, preview_line_offset_ptr,
                                update_preview);
    ctx->active->start_file = dir_entry->start_file;
    return TRUE;

  case ACTION_LIST_JUMP:
    if (list_jump)
      list_jump(ctx, dir_entry, "");
    *need_dsp_help_ptr = TRUE;
    return TRUE;

  default:
    return FALSE;
  }
}

BOOL handle_file_window_split_switch_action(
    ViewContext *ctx, YtreeAction action, DirEntry *dir_entry,
    YtreePanel *owner_panel, BOOL *switched_panel_ptr,
    YtreeAction *loop_action_ptr, BOOL *return_esc_ptr) {
  if (!ctx || !dir_entry || !owner_panel || !switched_panel_ptr ||
      !loop_action_ptr || !return_esc_ptr)
    return FALSE;

  *return_esc_ptr = FALSE;

  switch (action) {
  case ACTION_SPLIT_SCREEN:
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
      FreeFileEntryList(ctx->left);
    }
    ctx->is_split_screen = !ctx->is_split_screen;
    ReCreateWindows(ctx);

    if (ctx->is_split_screen) {
      if (ctx->right && ctx->left) {
        ctx->right->vol = ctx->left->vol;
        ctx->right->cursor_pos = ctx->left->cursor_pos;
        ctx->right->disp_begin_pos = ctx->left->disp_begin_pos;
        ctx->right->start_file = dir_entry->start_file;
        ctx->right->file_cursor_pos = dir_entry->cursor_pos;
        ctx->right->file_dir_entry = dir_entry;
        ctx->right->saved_focus = ctx->left->saved_focus;
        FreeFileEntryList(ctx->right);
      }
    } else {
      FreeFileEntryList(ctx->right);
      ctx->active = ctx->left;
    }

    *return_esc_ptr = TRUE;
    return TRUE;

  case ACTION_SWITCH_PANEL:
    if (!ctx->is_split_screen)
      return TRUE;
    owner_panel->file_dir_entry = dir_entry;
    owner_panel->start_file = dir_entry->start_file;
    owner_panel->file_cursor_pos = dir_entry->cursor_pos;
    ctx->active->saved_focus = FOCUS_FILE;
    *switched_panel_ptr = TRUE;
    SwitchToSmallFileWindow(ctx);

    if (ctx->active == ctx->left) {
      ctx->active = ctx->right;
    } else {
      ctx->active = ctx->left;
    }
    ctx->focused_window = ctx->active->saved_focus;
    *loop_action_ptr = ACTION_ESCAPE;
    return TRUE;

  default:
    return FALSE;
  }
}

BOOL handle_file_window_volume_action(ViewContext *ctx, YtreeAction action,
                                      const struct Volume *start_vol,
                                      int *unput_char_ptr,
                                      BOOL *return_esc_ptr) {
  if (!ctx || !unput_char_ptr || !return_esc_ptr)
    return FALSE;

  *return_esc_ptr = FALSE;
  switch (action) {
  case ACTION_VOL_MENU:
    if (SelectLoadedVolume(ctx, NULL) == 0) {
      *unput_char_ptr = '\0';
      *return_esc_ptr = TRUE;
      return TRUE;
    }
    if (ctx->active->vol != start_vol) {
      *return_esc_ptr = TRUE;
    }
    return TRUE;

  case ACTION_VOL_PREV:
    if (CycleLoadedVolume(ctx, ctx->active, -1) == 0) {
      *unput_char_ptr = '\0';
      *return_esc_ptr = TRUE;
      return TRUE;
    }
    if (ctx->active->vol != start_vol) {
      *return_esc_ptr = TRUE;
    }
    return TRUE;

  case ACTION_VOL_NEXT:
    if (CycleLoadedVolume(ctx, ctx->active, 1) == 0) {
      *unput_char_ptr = '\0';
      *return_esc_ptr = TRUE;
      return TRUE;
    }
    if (ctx->active->vol != start_vol) {
      *return_esc_ptr = TRUE;
    }
    return TRUE;

  default:
    return FALSE;
  }
}

BOOL handle_file_window_command_action(ViewContext *ctx, YtreeAction action,
                                       DirEntry **dir_entry_ptr,
                                       BOOL *need_dsp_help_ptr,
                                       BOOL *maybe_change_x_step_ptr,
                                       Statistic *s) {
  FileEntry *fe_ptr = NULL;
  FileEntry *new_fe_ptr = NULL;
  DirEntry *de_ptr = NULL;
  DirEntry *dest_dir_entry = NULL;
  DirEntry *dir_entry = *dir_entry_ptr;
  BOOL path_copy = FALSE;
  int term = 0;
  int get_dir_ret = 0;
  static char to_dir[PATH_LENGTH + 1];
  static char to_path[PATH_LENGTH + 1];
  static char to_file[PATH_LENGTH + 1];
  char expanded_to_file[PATH_LENGTH + 1];

#define need_dsp_help (*need_dsp_help_ptr)
#define maybe_change_x_step (*maybe_change_x_step_ptr)

  switch (action) {
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
          int copied_len = snprintf(to_path, sizeof(to_path), "%s", to_dir);
          if (copied_len < 0 || (size_t)copied_len >= sizeof(to_path)) {
            MESSAGE(ctx, "Invalid destination path*\"%s\"*path too long",
                    to_dir);
            break;
          }
        } else {
          MESSAGE(ctx, "Invalid destination path*\"%s\"*%s", to_dir,
                  strerror(errno));
          break;
        }
      }
      dest_dir_entry = NULL;
    } else {
      struct stat target_stat;
      get_dir_ret =
          GetDirEntry(ctx, s->tree, de_ptr, to_dir, &dest_dir_entry, to_path);
      if (get_dir_ret == -1) { /* System error */
        if (realpath(to_dir, to_path) == NULL ||
            STAT_(to_path, &target_stat) != 0 ||
            !S_ISREG(target_stat.st_mode)) {
          break;
        }
        dest_dir_entry = NULL;
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

    UI_RefreshSyncPanels(ctx, dir_entry);
    need_dsp_help = TRUE;
    break;

  case ACTION_CMD_M:
    if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE &&
        ctx->view_mode != ARCHIVE_MODE) {
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

    if (ctx->view_mode == ARCHIVE_MODE) {
      if (realpath(to_dir, to_path) == NULL) {
        if (errno == ENOENT) {
          int copied_len = snprintf(to_path, sizeof(to_path), "%s", to_dir);
          if (copied_len < 0 || (size_t)copied_len >= sizeof(to_path)) {
            MESSAGE(ctx, "Invalid destination path*\"%s\"*path too long",
                    to_dir);
            break;
          }
        } else {
          MESSAGE(ctx, "Invalid destination path*\"%s\"*%s", to_dir,
                  strerror(errno));
          break;
        }
      }
      dest_dir_entry = NULL;
    } else {
      BOOL target_is_regular_file = FALSE;
      struct stat target_stat;

      get_dir_ret =
          GetDirEntry(ctx, s->tree, de_ptr, to_dir, &dest_dir_entry, to_path);
      if (get_dir_ret == -1) {
        if (realpath(to_dir, to_path) != NULL &&
            STAT_(to_path, &target_stat) == 0 && S_ISREG(target_stat.st_mode)) {
          dest_dir_entry = NULL;
          target_is_regular_file = TRUE;
        } else {
          break;
        }
      }
      if (get_dir_ret == -3) {
        dest_dir_entry = NULL;
      }

      if (!target_is_regular_file) {
        /* Construct absolute path for checking */
        {
          char abs_check_path[PATH_LENGTH * 2 + 2];
          BOOL created = FALSE;
          int dir_create_mode = 0;

          if (*to_dir == FILE_SEPARATOR_CHAR) {
            int copied_len =
                snprintf(abs_check_path, sizeof(abs_check_path), "%s", to_dir);
            if (copied_len < 0 ||
                (size_t)copied_len >= sizeof(abs_check_path)) {
              MESSAGE(ctx, "Invalid destination path*\"%s\"*path too long",
                      to_dir);
              break;
            }
          } else {
            char current_dir[PATH_LENGTH + 1];

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
      }
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
        RebuildActiveFileListAfterMutation(ctx, dir_entry);

        UI_RefreshSyncPanels(ctx, dir_entry);

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
        RebuildActiveFileListAfterMutation(ctx, dir_entry);

        UI_RefreshSyncPanels(ctx, dir_entry);
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

    {
      char new_name[PATH_LENGTH + 1];
      if (!GetRenameParameter(ctx, fe_ptr->name, new_name)) {
        char expanded_new_name[PATH_LENGTH + 1];

        /* EXPAND WILDCARDS FOR SINGLE FILE RENAME */
        BuildFilename(fe_ptr->name, new_name, expanded_new_name);

        if (!RenameFile(ctx, fe_ptr, expanded_new_name, &new_fe_ptr)) {
          /* Rename OK */
          /*-----------*/

          maybe_change_x_step = TRUE;
        }
        RefreshView(ctx, dir_entry);
      }
    }
    need_dsp_help = TRUE;
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

  case ACTION_CMD_PRINT:
    UI_HandlePrintController(ctx, dir_entry, FALSE);
    RefreshView(ctx, dir_entry);
    need_dsp_help = TRUE;
    break;

  case ACTION_CMD_X:
    fe_ptr =
        ctx->active
            ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
            .file;
    if (!fe_ptr)
      break;
    de_ptr = fe_ptr->dir_entry;
    {
      char command_template[COMMAND_LINE_LENGTH + 1];
      command_template[0] = '\0';
      if (fe_ptr->stat_struct.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
        if (!Path_ShellQuote(fe_ptr->name, command_template,
                             sizeof(command_template))) {
          WARNING(ctx, "Command line too long.");
          break;
        }
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

  default:
#undef need_dsp_help
#undef maybe_change_x_step
    return FALSE;
  }

  *dir_entry_ptr = dir_entry;
#undef need_dsp_help
#undef maybe_change_x_step
  return TRUE;
}

BOOL handle_tag_file_action(ViewContext *ctx, int action, DirEntry *dir_entry,
                            int *unput_char_ptr, BOOL *need_dsp_help_ptr,
                            int start_x, Statistic *s,
                            BOOL *maybe_change_x_step_ptr) {
  FileEntry *fe_ptr = NULL;
  FileEntry *new_fe_ptr = NULL;
  DirEntry *de_ptr = NULL;
  DirEntry *dest_dir_entry = NULL;
  int i = 0, list_pos = 0, term = 0, get_dir_ret = 0, owner_id = 0,
      group_id = 0;
  off_t file_size = 0;
  WalkingPackage walking_package = {0};
  int pclose_ret = 0;
  char to_dir[PATH_LENGTH * 2 + 1] = {0};
  char to_file[PATH_LENGTH + 1] = {0};
  char to_path[PATH_LENGTH + 1] = {0};
  char new_name[PATH_LENGTH + 1] = {0};
  BOOL path_copy = FALSE;

/* Macros for local pointers */
#define unput_char (*unput_char_ptr)
#define need_dsp_help (*need_dsp_help_ptr)
#define maybe_change_x_step (*maybe_change_x_step_ptr)

  int max_disp_files =
      getmaxy(ctx->ctx_file_window) * GetPanelMaxColumn(ctx->active);
  int x_step =
      (GetPanelMaxColumn(ctx->active) > 1) ? getmaxy(ctx->ctx_file_window) : 1;
  (void)x_step;
  (void)new_fe_ptr;
  (void)i;
  (void)list_pos;
  (void)term;
  (void)get_dir_ret;
  (void)owner_id;
  (void)group_id;
  (void)file_size;
  (void)pclose_ret;
  (void)path_copy;

  switch (action) {
  case ACTION_CMD_TAGGED_A:
    if ((ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) ||
        !FileTags_IsMatchingTaggedFiles(ctx)) {
    } else {
      int attr_action;
      need_dsp_help = TRUE;
      attr_action = UI_PromptAttributeAction(ctx, TRUE, FALSE);

      if (attr_action == 'M') {
        mode_t mask = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
        char mode[16] = {0};

        (void)GetAttributes(mask, mode);

        /* Updated to use InputString instead of GetNewFileModus */
        ClearHelp(ctx);
        if (UI_ReadString(ctx, ctx->active, "MODE (octal/rwx):", mode,
                          (int)sizeof(mode), HST_CHANGE_MODUS) == CR) {
          char parsed_mode[12] = {0};
          char preview_mode[10] = {0};
          if (UI_ParseModeInput(mode, parsed_mode, preview_mode) != 0) {
            UI_Message(ctx, "Invalid mode. Use 3/4-digit octal or -rwxrwxrwx");
            wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
            wclrtoeol(ctx->ctx_border_window);
            wnoutrefresh(ctx->ctx_border_window);
            return TRUE;
          }
          if (snprintf(
                  walking_package.function_data.change_mode.new_mode,
                  sizeof(walking_package.function_data.change_mode.new_mode),
                  "%s", parsed_mode) >=
              (int)sizeof(walking_package.function_data.change_mode.new_mode)) {
            UI_Message(ctx, "Mode value too long.");
            wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
            wclrtoeol(ctx->ctx_border_window);
            wnoutrefresh(ctx->ctx_border_window);
            return TRUE;
          }
          FileTags_WalkTaggedFiles(ctx, dir_entry->start_file,
                                   dir_entry->cursor_pos, SetFileModus,
                                   &walking_package);

          UI_RenderFilePanel(ctx, dir_entry, start_x);
        }
        wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
        wclrtoeol(ctx->ctx_border_window);
        wnoutrefresh(ctx->ctx_border_window); /* Cleanup prompt line */
      } else if (attr_action == 'O') {
        if ((owner_id = GetNewOwner(ctx, -1)) >= 0) {
          walking_package.function_data.change_owner.new_owner_id = owner_id;
          FileTags_WalkTaggedFiles(ctx, dir_entry->start_file,
                                   dir_entry->cursor_pos, SetFileOwner,
                                   &walking_package);

          UI_RenderFilePanel(ctx, dir_entry, start_x);
        }
      } else if (attr_action == 'G') {
        if ((group_id = GetNewGroup(ctx, -1)) >= 0) {
          walking_package.function_data.change_group.new_group_id = group_id;
          FileTags_WalkTaggedFiles(ctx, dir_entry->start_file,
                                   dir_entry->cursor_pos, SetFileGroup,
                                   &walking_package);

          UI_RenderFilePanel(ctx, dir_entry, start_x);
        }
      } else if (attr_action == 'D' || attr_action == 0x04) {
        time_t new_time = time(NULL);
        int scope_mask = 0;
        int idx;

        if (UI_GetDateChangeSpec(ctx, &new_time, &scope_mask) != 0) {
          return TRUE;
        }

        for (idx = 0; idx < (int)ctx->active->file_count; idx++) {
          struct utimbuf times;
          struct stat updated_stat;
          char path[PATH_LENGTH + 1];

          fe_ptr = ctx->active->file_entry_list[idx].file;
          if (!fe_ptr || !fe_ptr->tagged)
            continue;

          GetFileNamePath(fe_ptr, path);
          times.actime = (scope_mask & DATE_SCOPE_ACCESS)
                             ? new_time
                             : fe_ptr->stat_struct.st_atime;
          times.modtime = (scope_mask & DATE_SCOPE_MODIFY)
                              ? new_time
                              : fe_ptr->stat_struct.st_mtime;

          if (utime(path, &times) != 0)
            continue;
          if (stat(path, &updated_stat) == 0)
            fe_ptr->stat_struct = updated_stat;
        }

        UI_RenderFilePanel(ctx, dir_entry, start_x);
      }
    }
    return TRUE;

  case ACTION_CMD_TAGGED_O:
    UI_Beep(ctx, FALSE);
    return TRUE;

  case ACTION_CMD_TAGGED_G:
    UI_Beep(ctx, FALSE);
    return TRUE;

  case ACTION_TAG:
    fe_ptr =
        ctx->active
            ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
            .file;
    de_ptr = fe_ptr->dir_entry;

    if (!fe_ptr->tagged) {
      fe_ptr->tagged = TRUE;
      de_ptr->tagged_files++;
      de_ptr->tagged_bytes += fe_ptr->stat_struct.st_size;
      s->disk_tagged_files++;
      s->disk_tagged_bytes += fe_ptr->stat_struct.st_size;
    }
    UI_RenderFilePanel(ctx, dir_entry, start_x);
    DisplayDiskStatistic(ctx, s); /* Always update global disk stats */
    DisplayDirStatistic(
        ctx, dir_entry, NULL,
        s); /* Always update current list stats (even in Showall) */
    unput_char = KEY_DOWN;

    return TRUE;
  case ACTION_UNTAG:
    fe_ptr =
        ctx->active
            ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
            .file;
    de_ptr = fe_ptr->dir_entry;
    if (fe_ptr->tagged) {
      fe_ptr->tagged = FALSE;

      de_ptr->tagged_files--;
      de_ptr->tagged_bytes -= fe_ptr->stat_struct.st_size;
      s->disk_tagged_files--;
      s->disk_tagged_bytes -= fe_ptr->stat_struct.st_size;
    }
    UI_RenderFilePanel(ctx, dir_entry, start_x);
    DisplayDiskStatistic(ctx, s); /* Always update global disk stats */
    DisplayDirStatistic(
        ctx, dir_entry, NULL,
        s); /* Always update current list stats (even in Showall) */
    unput_char = KEY_DOWN;

    return TRUE;

  case ACTION_TAG_ALL:
    for (i = 0; i < (int)ctx->active->file_count; i++) {
      fe_ptr = ctx->active->file_entry_list[i].file;
      de_ptr = fe_ptr->dir_entry;

      if (!fe_ptr->tagged) {
        file_size = fe_ptr->stat_struct.st_size;

        fe_ptr->tagged = TRUE;
        de_ptr->tagged_files++;
        de_ptr->tagged_bytes += file_size;
        s->disk_tagged_files++;
        s->disk_tagged_bytes += file_size;
      }
    }

    UI_RenderFilePanel(ctx, dir_entry, start_x);
    DisplayDiskStatistic(ctx, s); /* Always update global disk stats */
    DisplayDirStatistic(
        ctx, dir_entry, NULL,
        s); /* Always update current list stats (even in Showall) */
    return TRUE;

  case ACTION_UNTAG_ALL:
    for (i = 0; i < (int)ctx->active->file_count; i++) {
      fe_ptr = ctx->active->file_entry_list[i].file;
      de_ptr = fe_ptr->dir_entry;

      if (fe_ptr->tagged) {
        file_size = fe_ptr->stat_struct.st_size;

        fe_ptr->tagged = FALSE;
        de_ptr->tagged_files--;
        de_ptr->tagged_bytes -= file_size;
        s->disk_tagged_files--;
        s->disk_tagged_bytes -= file_size;
      }
    }

    UI_RenderFilePanel(ctx, dir_entry, start_x);
    DisplayDiskStatistic(ctx, s); /* Always update global disk stats */
    DisplayDirStatistic(
        ctx, dir_entry, NULL,
        s); /* Always update current list stats (even in Showall) */
    return TRUE;

  case ACTION_TAG_REST:
    for (i = dir_entry->start_file + dir_entry->cursor_pos;
         i < (int)ctx->active->file_count; i++) {
      fe_ptr = ctx->active->file_entry_list[i].file;
      de_ptr = fe_ptr->dir_entry;

      if (!fe_ptr->tagged) {
        file_size = fe_ptr->stat_struct.st_size;

        fe_ptr->tagged = TRUE;
        de_ptr->tagged_files++;
        de_ptr->tagged_bytes += file_size;
        s->disk_tagged_files++;
        s->disk_tagged_bytes += file_size;
      }
    }

    UI_RenderFilePanel(ctx, dir_entry, start_x);
    DisplayDiskStatistic(ctx, s); /* Always update global disk stats */
    DisplayDirStatistic(
        ctx, dir_entry, NULL,
        s); /* Always update current list stats (even in Showall) */
    return TRUE;

  case ACTION_UNTAG_REST:
    for (i = dir_entry->start_file + dir_entry->cursor_pos;
         i < (int)ctx->active->file_count; i++) {
      fe_ptr = ctx->active->file_entry_list[i].file;
      de_ptr = fe_ptr->dir_entry;

      if (fe_ptr->tagged) {
        file_size = fe_ptr->stat_struct.st_size;

        fe_ptr->tagged = FALSE;
        de_ptr->tagged_files--;
        de_ptr->tagged_bytes -= file_size;
        s->disk_tagged_files--;
        s->disk_tagged_bytes -= file_size;
      }
    }

    UI_RenderFilePanel(ctx, dir_entry, start_x);
    DisplayDiskStatistic(ctx, s); /* Always update global disk stats */
    DisplayDirStatistic(
        ctx, dir_entry, NULL,
        s); /* Always update current list stats (even in Showall) */
    return TRUE;

  case ACTION_CMD_TAGGED_V:
    if (!FileTags_IsMatchingTaggedFiles(ctx)) {
      /* STRICT FILTER MODE: No tags = no action */
    } else {
      UI_ViewTaggedFiles(ctx, dir_entry);
      need_dsp_help = TRUE;
    }
    return TRUE;

  case ACTION_CMD_TAGGED_Y:
  case ACTION_CMD_TAGGED_C:
    de_ptr = dir_entry;

    path_copy = FALSE;
    if (action == ACTION_CMD_TAGGED_Y)
      path_copy = TRUE;

    if (!FileTags_IsMatchingTaggedFiles(ctx)) {
    } else {
      need_dsp_help = TRUE;

      if (GetCopyParameter(ctx, NULL, path_copy, to_file, to_dir)) {
        break;
      }

      if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
        if (realpath(to_dir, to_path) == NULL) {
          if (errno == ENOENT) {
            if (snprintf(to_path, sizeof(to_path), "%s", to_dir) >=
                (int)sizeof(to_path)) {
              MESSAGE(ctx, "Invalid destination path*\"%s\"*Path too long",
                      to_dir);
              break;
            }
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

      term = InputChoice(
          ctx, "Ask for confirmation for each overwrite (Y/N) ? ", "YN\033");
      if (term == ESC) {
        break;
      }

      walking_package.function_data.copy.statistic_ptr = s;
      walking_package.function_data.copy.dest_dir_entry = dest_dir_entry;
      walking_package.function_data.copy.to_file = to_file;
      walking_package.function_data.copy.to_path =
          to_path; /* Fixed struct access */
      walking_package.function_data.copy.path_copy =
          path_copy; /* Fixed struct access */
      walking_package.function_data.copy.conflict_cb =
          (void *)(ConflictCallback)UI_ConflictResolverWrapper;
      walking_package.function_data.copy.dir_create_mode =
          0; /* Reset auto-create mode */
      walking_package.function_data.copy.overwrite_mode =
          (term == 'N') ? 1 : 0; /* Reset overwrite mode */
      walking_package.function_data.copy.choice_cb =
          (void *)(ChoiceCallback)UI_ChoiceResolver;

      FileTags_WalkTaggedFiles(ctx, dir_entry->start_file,
                               dir_entry->cursor_pos, CopyTaggedFiles,
                               &walking_package);

      UI_RefreshSyncPanels(ctx, dir_entry);
    }
    need_dsp_help = TRUE;
    return TRUE;

  case ACTION_CMD_TAGGED_M:
    if ((ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) ||
        !FileTags_IsMatchingTaggedFiles(ctx)) {
    } else {
      de_ptr = dir_entry;
      need_dsp_help = TRUE;

      if (GetMoveParameter(ctx, NULL, to_file, to_dir)) {
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
        BOOL created = FALSE;
        int dir_create_mode = 0;

        if (*to_dir == FILE_SEPARATOR_CHAR) {
          if (snprintf(abs_check_path, sizeof(abs_check_path), "%s", to_dir) >=
              (int)sizeof(abs_check_path)) {
            MESSAGE(ctx, "Invalid destination path*\"%s\"*Path too long",
                    to_dir);
            break;
          }
        } else {
          char current_dir[PATH_LENGTH + 1];
          GetPath(dir_entry, current_dir);
          snprintf(abs_check_path, sizeof(abs_check_path), "%s%c%s",
                   current_dir, FILE_SEPARATOR_CHAR, to_dir);
        }
        /* FIX: Pass &dest_dir_entry */
        if (EnsureDirectoryExists(ctx, abs_check_path, s->tree, &created,
                                  &dest_dir_entry, &dir_create_mode,
                                  (ChoiceCallback)UI_ChoiceResolver) == -1)
          break;
      }

      term = InputChoice(
          ctx, "Ask for confirmation for each overwrite (Y/N) ? ", "YN\033");
      if (term == ESC) {
        break;
      }

      walking_package.function_data.mv.dest_dir_entry = dest_dir_entry;
      walking_package.function_data.mv.to_file = to_file;
      walking_package.function_data.mv.to_path = to_path;
      walking_package.function_data.mv.conflict_cb =
          (void *)(ConflictCallback)UI_ConflictResolverWrapper;
      walking_package.function_data.mv.dir_create_mode =
          0; /* Reset auto-create mode */
      walking_package.function_data.mv.overwrite_mode =
          (term == 'N') ? 1 : 0; /* Reset overwrite mode */
      walking_package.function_data.mv.choice_cb =
          (void *)(ChoiceCallback)UI_ChoiceResolver;

      FileTags_WalkTaggedFiles(ctx, dir_entry->start_file,
                               dir_entry->cursor_pos, MoveTaggedFiles,
                               &walking_package);

      UI_RefreshSyncPanels(ctx, dir_entry);

      BuildFileEntryList(ctx, ctx->active);

      dir_entry->start_file = 0;
      dir_entry->cursor_pos = 0;

      RefreshView(ctx, dir_entry);
      maybe_change_x_step = TRUE;
    }
    return TRUE;

  case ACTION_CMD_TAGGED_D:
    if ((ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE &&
         ctx->view_mode != ARCHIVE_MODE) ||
        !FileTags_IsMatchingTaggedFiles(ctx)) {
    } else {
      need_dsp_help = TRUE;
      (void)FileTags_UI_DeleteTaggedFiles(ctx, max_disp_files, s);
      /* ... */

      RefreshView(ctx, dir_entry);
      maybe_change_x_step = TRUE;
    }
    return TRUE;

  case ACTION_CMD_TAGGED_R:
    if ((ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE &&
         ctx->view_mode != ARCHIVE_MODE) ||
        !FileTags_IsMatchingTaggedFiles(ctx)) {
    } else {
      need_dsp_help = TRUE;

      if (GetRenameParameter(ctx, NULL, new_name)) {
        break;
      }

      walking_package.function_data.rename.new_name = new_name;
      walking_package.function_data.rename.confirm = FALSE;

      FileTags_WalkTaggedFiles(ctx, dir_entry->start_file,
                               dir_entry->cursor_pos, RenameTaggedFiles,
                               &walking_package);

      BuildFileEntryList(ctx, ctx->active);

      RefreshView(ctx, dir_entry);
      maybe_change_x_step = TRUE;
    }
    return TRUE;

  case ACTION_CMD_TAGGED_P:
    if (!FileTags_IsMatchingTaggedFiles(ctx)) {
    } else if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
      UI_Message(ctx, "^P is not available in archive mode");
    } else {
      char filepath[PATH_LENGTH + 1] = {0};
      need_dsp_help = TRUE;

      filepath[0] = '\0'; /* Initialize buffer to prevent garbage prompt */
      if (GetPipeCommand(ctx, filepath) == 0) {
        /* Exit ncurses mode */
        endwin();
        SuspendClock(ctx);

        if ((walking_package.function_data.pipe_cmd.pipe_file =
                 popen(filepath, "w")) == NULL) {
          /* Restore ncurses mode if popen fails */
          InitClock(ctx);
          touchwin(stdscr);
          wnoutrefresh(stdscr);
          doupdate();
          UI_Message(ctx, "execution of command*%s*failed", filepath);
        } else {
          FileTags_SilentWalkTaggedFiles(ctx, PipeTaggedFiles,
                                         &walking_package);

          /* Close pipe and capture return value */
          pclose_ret = pclose(walking_package.function_data.pipe_cmd
                                  .pipe_file); /* Fixed struct access */

          /* Wait for user input */
          HitReturnToContinue();

          /* Restore ncurses mode */
          InitClock(ctx);
          touchwin(stdscr);
          wnoutrefresh(stdscr);
          doupdate(); /* Restore screen */

          if (pclose_ret) {
            UI_Warning(ctx, "pclose failed");
          }
        }
      }
      RefreshView(ctx, dir_entry);
    }
    return TRUE;

  case ACTION_CMD_TAGGED_PRINT:
    if (!FileTags_IsMatchingTaggedFiles(ctx)) {
    } else if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
      UI_Message(ctx, "^P is not available in archive mode");
    } else {
      need_dsp_help = TRUE;
      UI_HandlePrint(ctx, dir_entry, TRUE);
      RefreshView(ctx, dir_entry);
    }
    return TRUE;

  case ACTION_CMD_TAGGED_S:
    /* STRICT FILTER MODE: Only allow if tags exist */
    if (!FileTags_IsMatchingTaggedFiles(ctx)) {
      /* If no tags, this command does nothing (or shows error) */
      MESSAGE(ctx, "No tagged files");
    } else if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE &&
               ctx->view_mode != ARCHIVE_MODE) {
      MESSAGE(ctx, "^S is not available in archive mode");
    } else {
      char *command_line;

      if ((command_line = (char *)malloc(COMMAND_LINE_LENGTH + 1)) == NULL) {
        UI_Error(ctx, "", 0, "Malloc failed*ABORT");
        exit(1);
      }

      /* Allocate new buffer for silent command */
      size_t silent_cmd_size =
          (size_t)COMMAND_LINE_LENGTH + sizeof(" > /dev/null 2>&1");
      char *silent_cmd = (char *)malloc(silent_cmd_size);
      if (!silent_cmd) {
        UI_Error(ctx, "", 0, "Malloc failed*ABORT");
        exit(1);
      }

      need_dsp_help = TRUE;
      *command_line = '\0';

      /* Filter Mode */
      if (!GetSearchCommandLine(ctx, command_line, ctx->global_search_term)) {
        /* Construct Silent Command */
        int n = snprintf(silent_cmd, silent_cmd_size, "%s > /dev/null 2>&1",
                         command_line);
        if (n < 0 || (size_t)n >= silent_cmd_size) {
          UI_Message(ctx, "Command too long");
        } else {
          walking_package.function_data.execute.command = silent_cmd;
          /* Use modified SilentTagWalk (Untag on Fail) */
          FileTags_SilentTagWalkTaggedFiles(ctx, ExecuteCommand,
                                            &walking_package);
          /* No HitReturnToContinue - purely silent */
        }
      }

      /* Refresh Display */
      UI_RenderFilePanel(ctx, dir_entry, start_x);
      DisplayDiskStatistic(ctx, s);
      if (dir_entry->global_flag)
        DisplayDiskStatistic(ctx, s);
      else
        DisplayDirStatistic(ctx, dir_entry, NULL, s);

      free(command_line);
      free(silent_cmd);
    }
    return TRUE;

  case ACTION_CMD_TAGGED_X:
    if (!FileTags_IsMatchingTaggedFiles(ctx)) {
    } else if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE &&
               ctx->view_mode != ARCHIVE_MODE) {
      UI_Message(ctx, "^X is not available in archive mode");
    } else {
      char *command_line;

      if ((command_line = (char *)malloc(COMMAND_LINE_LENGTH + 1)) == NULL) {
        UI_Error(ctx, __FILE__, __LINE__, "Malloc failed*ABORT");
        exit(1);
      }

      need_dsp_help = TRUE;
      *command_line = '\0';
      if (GetCommandLine(ctx, command_line) == 0) {
        endwin();
        SuspendClock(ctx);
        walking_package.function_data.execute.command = command_line;
        FileTags_SilentWalkTaggedFiles(ctx, ExecuteCommand, &walking_package);
        HitReturnToContinue();

        InitClock(ctx);
        touchwin(stdscr);
        wnoutrefresh(stdscr);
        doupdate();

        dir_entry = RefreshFileView(ctx, dir_entry);

        /* Insert: Explicit Global Refresh */
        RefreshView(ctx, dir_entry);
      }
      free(command_line);
    }
    return TRUE;

  case ACTION_TOGGLE_TAGGED_MODE:

    /* Toggle mode (if possible) */
    if (dir_entry->tagged_files)
      dir_entry->tagged_flag = !dir_entry->tagged_flag;
    else
      dir_entry->tagged_flag = FALSE;

    BuildFileEntryList(ctx, ctx->active);

    dir_entry->start_file = 0;
    dir_entry->cursor_pos = 0;
    UI_RenderFilePanel(ctx, dir_entry, start_x);
    maybe_change_x_step = TRUE;
    return TRUE;

  case ACTION_INVERT: /* Mapped to 'i'/'I' for Invert Tags in File Window */
    FileTags_HandleInvertTags(ctx, dir_entry, s);
    need_dsp_help = TRUE;
    return TRUE;
  }
#undef unput_char
#undef need_dsp_help
#undef maybe_change_x_step
  return FALSE;
}
