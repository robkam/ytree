/***************************************************************************
 *
 * src/ui/ctrl_file_ops.c
 * File Action Handlers (Tagging, Move, Copy, Delete dispatch)
 *
 ***************************************************************************/

#define NO_YTREE_MACROS
#include "ytree.h"
#include <utime.h>

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
  WalkingPackage walking_package;
  mode_t mask = 0;
  char mode[16] = {0};
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
  (void)mask;
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
        mask = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

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

          DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                       dir_entry->start_file + dir_entry->cursor_pos, start_x,
                       ctx->ctx_file_window);
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

          DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                       dir_entry->start_file + dir_entry->cursor_pos, start_x,
                       ctx->ctx_file_window);
        }
      } else if (attr_action == 'G') {
        if ((group_id = GetNewGroup(ctx, -1)) >= 0) {
          walking_package.function_data.change_group.new_group_id = group_id;
          FileTags_WalkTaggedFiles(ctx, dir_entry->start_file,
                                   dir_entry->cursor_pos, SetFileGroup,
                                   &walking_package);

          DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                       dir_entry->start_file + dir_entry->cursor_pos, start_x,
                       ctx->ctx_file_window);
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

        DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                     dir_entry->start_file + dir_entry->cursor_pos, start_x,
                     ctx->ctx_file_window);
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
    DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                 dir_entry->start_file + dir_entry->cursor_pos, start_x,
                 ctx->ctx_file_window);
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
    DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                 dir_entry->start_file + dir_entry->cursor_pos, start_x,
                 ctx->ctx_file_window);
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

    DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                 dir_entry->start_file + dir_entry->cursor_pos, start_x,
                 ctx->ctx_file_window);
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

    DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                 dir_entry->start_file + dir_entry->cursor_pos, start_x,
                 ctx->ctx_file_window);
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

    DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                 dir_entry->start_file + dir_entry->cursor_pos, start_x,
                 ctx->ctx_file_window);
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

    DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                 dir_entry->start_file + dir_entry->cursor_pos, start_x,
                 ctx->ctx_file_window);
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

      RefreshDirWindow(ctx, ctx->active);
      if (ctx->is_split_screen) {
        RefreshDirWindow(ctx,
                         (ctx->active == ctx->left) ? ctx->right : ctx->left);
      }

      RefreshView(ctx, dir_entry);
    }
    need_dsp_help = TRUE;
    return TRUE;

  case ACTION_CMD_TAGGED_M:
    if ((ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) ||
        !FileTags_IsMatchingTaggedFiles(ctx)) {
    } else {
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

      RefreshDirWindow(ctx, ctx->active);
      if (ctx->is_split_screen) {
        RefreshDirWindow(ctx,
                         (ctx->active == ctx->left) ? ctx->right : ctx->left);
      }

      BuildFileEntryList(ctx, ctx->active);

      if (ctx->active->file_count == 0)
        unput_char = ESC;

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

      if ((command_line = (char *)malloc(COLS + 1)) == NULL) {
        UI_Error(ctx, "", 0, "Malloc failed*ABORT");
        exit(1);
      }

      /* Allocate new buffer for silent command */
      size_t silent_cmd_size = (size_t)COLS + 20U;
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
      DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                   dir_entry->start_file + dir_entry->cursor_pos, start_x,
                   ctx->ctx_file_window);
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

      if ((command_line = (char *)malloc(COLS + 1)) == NULL) {
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
    DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                 dir_entry->start_file + dir_entry->cursor_pos, start_x,
                 ctx->ctx_file_window);
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
