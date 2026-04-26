/***************************************************************************
 *
 * src/ui/interactions.c
 * UI Prompts and Interaction Functions
 *
 ***************************************************************************/

#include "sort.h"
#include "watcher.h"
#include "ytree_cmd.h"
#include "ytree_fs.h"
#include "ytree_ui.h"
#include "interactions_panel_paths.h"
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <libgen.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

static char move_prompt_header[PATH_LENGTH + 50];
static char move_prompt_as[PATH_LENGTH + 1];

static void CopyBoundedString(char *dst, size_t dst_size, const char *src) {
  int written;

  if (!dst || dst_size == 0)
    return;

  if (!src)
    src = "";

  written = snprintf(dst, dst_size, "%s", src);
  if (written < 0) {
    dst[0] = '\0';
  } else if ((size_t)written >= dst_size) {
    dst[dst_size - 1] = '\0';
  }
}

static void DrawSortPrompt(ViewContext *ctx, WINDOW *win, BOOL ascending) {
  int y0;

  if (!ctx || !win)
    return;

  if (win == stdscr) {
    y0 = Y_PROMPT(ctx);
    wmove(win, y0, 0);
    wclrtoeol(win);
    wmove(win, y0 + 1, 0);
    wclrtoeol(win);
    if (y0 > 0) {
      wmove(win, y0 - 1, 0);
      wclrtoeol(win);
    }
  } else {
    y0 = 0;
    werase(win);
  }

  PrintOptions(
      win, y0, 0,
      "SORT by   (A)ccTime (C)hgTime (E)xtension (G)roup (M)odTime (N)ame "
      "(S)ize");
  PrintOptions(win, y0 + 1, 0,
               ascending ? "COMMANDS  o(W)ner   (O)rder: [ascending]"
                         : "COMMANDS  o(W)ner   (O)rder: [descending]");
  wnoutrefresh(win);
  doupdate();
}

int UI_ConflictResolverWrapper(ViewContext *ctx, const char *src_path,
                               const char *dst_path, int *mode_flags) {
  return UI_AskConflict(ctx, src_path, dst_path, mode_flags);
}

int UI_ChoiceResolver(ViewContext *ctx, const char *prompt,
                      const char *choices) {
  return InputChoice(ctx, prompt, choices);
}

int UI_CoreQuitConfirm(ViewContext *ctx, const char *msg, const char *choices) {
  return InputChoice(ctx, msg, choices);
}

void UI_CoreQuitSaveHistory(ViewContext *ctx, const char *path_for_history) {
  SaveHistory(ctx, path_for_history);
}

void UI_CoreQuitCloseWatcher(ViewContext *ctx) { Watcher_Close(ctx); }

void UI_CoreQuitCleanupVolumeTree(ViewContext *ctx) {
  Volume_FreeAll(ctx);
  FreeDirEntryList(ctx);
}

void UI_CoreQuitSuspendClock(ViewContext *ctx) { SuspendClock(ctx); }

void UI_CoreQuitShutdownTerminal(ViewContext *ctx) {
  attrset(0);  /* Reset attributes */
  clear();     /* Clear internal buffer */
  refresh();   /* Push clear to screen */
  curs_set(1); /* Restore visible cursor */
  ShutdownCurses(ctx);
#ifdef XCURSES
  XCursesExit();
#endif
}

int GetMoveParameter(ViewContext *ctx, const char *from_file, char *to_file,
                     char *to_dir) {
  if (from_file == NULL) {
    from_file = "TAGGED FILES";
    CopyBoundedString(to_file, PATH_LENGTH + 1, "*");
  } else {
    CopyBoundedString(to_file, PATH_LENGTH + 1, from_file);
  }

  (void)snprintf(move_prompt_header, sizeof(move_prompt_header), "MOVE: %s",
                 from_file);

  ClearHelp(ctx);

  if (UI_ReadString(ctx, ctx->active, move_prompt_header, to_file, PATH_LENGTH,
                    HST_FILE) == CR) {

    strncpy(move_prompt_as, to_file, PATH_LENGTH);
    move_prompt_as[PATH_LENGTH] = '\0';

    if (ctx->is_split_screen && ctx->active) {
      YtreePanel *target = (ctx->active == ctx->left) ? ctx->right : ctx->left;
      if (target && target->vol && target->vol->total_dirs > 0) {
        int idx = target->disp_begin_pos + target->cursor_pos;
        /* Safety bounds check */
        if (idx < 0)
          idx = 0;
        if (idx >= target->vol->total_dirs)
          idx = target->vol->total_dirs - 1;

        GetPath(target->vol->dir_entry_list[idx].dir_entry, to_dir);
      }
    }

    if (UI_ReadString(ctx, ctx->active, "To Directory:", to_dir, PATH_LENGTH,
                      HST_PATH) == CR) {
      if (to_dir[0] == '\0') {
        CopyBoundedString(to_dir, PATH_LENGTH + 1, ".");
      }
      return (0);
    }
  }
  ClearHelp(ctx);
  return (-1);
}

int GetCopyParameter(ViewContext *ctx, const char *from_file, BOOL path_copy,
                     char *to_file, char *to_dir) {
  char prompt_header[PATH_LENGTH + 50];

  if (from_file == NULL) {
    from_file = "TAGGED FILES";
    CopyBoundedString(to_file, PATH_LENGTH + 1, "*");
  } else {
    CopyBoundedString(to_file, PATH_LENGTH + 1, from_file);
  }

  if (path_copy) {
    (void)snprintf(prompt_header, sizeof(prompt_header), "PATHCOPY: %s",
                   from_file);
  } else {
    (void)snprintf(prompt_header, sizeof(prompt_header), "COPY: %s", from_file);
  }

  ClearHelp(ctx);

  if (UI_ReadString(ctx, ctx->active, prompt_header, to_file, PATH_LENGTH,
                    HST_FILE) == CR) {
    if (ctx->is_split_screen && ctx->active) {
      YtreePanel *target = (ctx->active == ctx->left) ? ctx->right : ctx->left;
      if (target && target->vol && target->vol->total_dirs > 0) {
        int idx = target->disp_begin_pos + target->cursor_pos;
        /* Safety bounds check */
        if (idx < 0)
          idx = 0;
        if (idx >= target->vol->total_dirs)
          idx = target->vol->total_dirs - 1;

        GetPath(target->vol->dir_entry_list[idx].dir_entry, to_dir);
      }
    }

    if (UI_ReadString(ctx, ctx->active, "To Directory:", to_dir, PATH_LENGTH,
                      HST_PATH) == CR) {
      if (to_dir[0] == '\0') {
        CopyBoundedString(to_dir, PATH_LENGTH + 1, ".");
      }
      return (0);
    }
  }
  ClearHelp(ctx);
  return (-1);
}

int UI_GatherArchivePayload(ViewContext *ctx, DirEntry *selected_dir,
                            FileEntry *selected_file,
                            ArchivePayload *payload) {
  char **selected_paths = NULL;
  size_t selected_count = 0;
  BOOL recursive_directories = TRUE;
  int rc = -1;

  if (!ctx || !ctx->active || !payload)
    return -1;

  UI_FreeArchivePayload(payload);

  BuildFileEntryList(ctx, ctx->active);
  if (ctx->active->file_entry_list && ctx->active->file_count > 0) {
    size_t i;
    size_t tagged_count = 0;

    for (i = 0; i < (size_t)ctx->active->file_count; ++i) {
      if (ctx->active->file_entry_list[i].file &&
          ctx->active->file_entry_list[i].file->tagged)
        tagged_count++;
    }

    if (tagged_count > 0) {
      selected_paths = (char **)xcalloc(tagged_count, sizeof(*selected_paths));
      if (!selected_paths)
        return -1;

      for (i = 0; i < (size_t)ctx->active->file_count; ++i) {
        char file_path[PATH_LENGTH + 1];
        FileEntry *fe = ctx->active->file_entry_list[i].file;

        if (!fe || !fe->tagged)
          continue;

        GetFileNamePath(fe, file_path);
        file_path[PATH_LENGTH] = '\0';
        selected_paths[selected_count] = xstrdup(file_path);
        if (!selected_paths[selected_count])
          goto cleanup;

        selected_count++;
      }
    }
  }

  if (selected_count == 0) {
    char source_path[PATH_LENGTH + 1];
    struct stat source_st;
    int recursive_choice;

    selected_paths = (char **)xcalloc(1, sizeof(*selected_paths));
    if (!selected_paths)
      return -1;

    if (selected_file) {
      GetFileNamePath(selected_file, source_path);
      source_path[PATH_LENGTH] = '\0';
    } else if (selected_dir) {
      GetPath(selected_dir, source_path);
      source_path[PATH_LENGTH] = '\0';
    } else if (UI_GetPanelSelectedFilePath(ctx, ctx->active, source_path) == 0) {
      source_path[PATH_LENGTH] = '\0';
    } else if (UI_GetPanelSelectedDirPath(ctx, ctx->active, source_path) == 0) {
      source_path[PATH_LENGTH] = '\0';
    } else {
      goto cleanup;
    }

    selected_paths[0] = xstrdup(source_path);
    if (!selected_paths[0])
      goto cleanup;
    selected_count = 1;

    if (lstat(source_path, &source_st) != 0) {
      UI_ShowStatusLineError(ctx, "Cannot access selected source path");
      goto cleanup;
    }
    if (S_ISDIR(source_st.st_mode)) {
      recursive_choice = InputChoiceLiteral(ctx, "Recursive? (Y/n)", "YN\033");
      if (recursive_choice == ESC) {
        rc = 1;
        goto cleanup;
      }
      if (recursive_choice == 'N')
        recursive_directories = FALSE;
    }
  }

  rc = UI_BuildArchivePayloadFromPaths((const char *const *)selected_paths,
                                       selected_count, recursive_directories,
                                       payload);

cleanup:
  if (selected_paths) {
    size_t i;
    for (i = 0; i < selected_count; ++i)
      free(selected_paths[i]);
    free(selected_paths);
  }
  if (rc < 0)
    UI_FreeArchivePayload(payload);
  return rc;
}

static int ResolveArchiveDestinationPath(ViewContext *ctx, const char *input_path,
                                         char *resolved_path,
                                         size_t resolved_size) {
  char absolute_input[PATH_LENGTH + 1];
  char normalized_input[PATH_LENGTH + 1];
  char parent_path[PATH_LENGTH + 1];
  char resolved_parent[PATH_LENGTH + 1];
  const char *slash;
  const char *file_name;
  int written;
  int parent_written;

  if (!input_path || !resolved_path || resolved_size == 0)
    return -1;
  if (input_path[0] == '\0')
    return -1;

  if (input_path[0] == FILE_SEPARATOR_CHAR) {
    written = snprintf(absolute_input, sizeof(absolute_input), "%s", input_path);
  } else {
    char cwd[PATH_LENGTH + 1];
    if (ctx && ctx->active &&
        UI_GetPanelSelectedDirPath(ctx, ctx->active, cwd) == 0) {
      cwd[PATH_LENGTH] = '\0';
    } else if (!getcwd(cwd, sizeof(cwd))) {
      return -1;
    }
    written = snprintf(absolute_input, sizeof(absolute_input), "%s%c%s", cwd,
                       FILE_SEPARATOR_CHAR, input_path);
  }
  if (written < 0 || (size_t)written >= sizeof(absolute_input))
    return -1;

  NormPath(absolute_input, normalized_input);
  if (realpath(normalized_input, resolved_path) != NULL)
    return 0;

  if (errno != ENOENT)
    return -1;

  slash = strrchr(normalized_input, FILE_SEPARATOR_CHAR);
  if (!slash)
    return -1;

  file_name = slash + 1;
  if (file_name[0] == '\0')
    return -1;

  if (slash == normalized_input) {
    parent_written = snprintf(parent_path, sizeof(parent_path), "%s",
                              FILE_SEPARATOR_STRING);
  } else {
    size_t parent_len = (size_t)(slash - normalized_input);
    parent_written = snprintf(parent_path, sizeof(parent_path), "%.*s",
                              (int)parent_len, normalized_input);
  }
  if (parent_written < 0 || (size_t)parent_written >= sizeof(parent_path))
    return -1;

  if (realpath(parent_path, resolved_parent) == NULL)
    return -1;

  if (strcmp(resolved_parent, FILE_SEPARATOR_STRING) == 0) {
    written = snprintf(resolved_path, resolved_size, "%s%s", resolved_parent,
                       file_name);
  } else {
    written = snprintf(resolved_path, resolved_size, "%s%c%s", resolved_parent,
                       FILE_SEPARATOR_CHAR, file_name);
  }
  if (written < 0 || (size_t)written >= resolved_size)
    return -1;

  return 0;
}

static BOOL SourcePathMatchesArchiveDestination(const char *source_path,
                                                const char *dest_path) {
  char resolved_source[PATH_LENGTH + 1];

  if (!source_path || !dest_path)
    return FALSE;

  if (realpath(source_path, resolved_source) == NULL)
    return FALSE;

  return strcmp(resolved_source, dest_path) == 0;
}

static const char *UnsupportedArchiveLabel(const char *dest_path) {
  const char *dot;

  if (!dest_path)
    return "(none)";

  dot = strrchr(dest_path, '.');
  if (!dot || dot[1] == '\0')
    return "(none)";
  return dot;
}

int UI_CreateArchiveFromPayload(ViewContext *ctx, const ArchivePayload *payload) {
  char destination_input[PATH_LENGTH + 1];
  char destination_path[PATH_LENGTH + 1];
  const char *filename = NULL;
  int input_result;
  struct stat dest_stat;
  int prompt_written;
  char overwrite_prompt[PATH_LENGTH + 64];

  if (!ctx || !ctx->active || !payload)
    return -1;
  if (!payload->original_source_list)
    return -1;

  destination_input[0] = '\0';
  input_result = UI_ReadString(
      ctx, ctx->active,
      "Create archive: (suffix .tar .tar.gz/.tgz .tar.bz2/.tbz2 .tar.xz/.txz .zip) ",
      destination_input, PATH_LENGTH, HST_FILE);
  if (input_result != CR || destination_input[0] == '\0')
    return 1;

  if (ResolveArchiveDestinationPath(ctx, destination_input, destination_path,
                                    sizeof(destination_path)) != 0) {
    UI_ShowStatusLineError(ctx, "Invalid archive destination path");
    return -1;
  }

  if (lstat(destination_path, &dest_stat) == 0) {
    filename = strrchr(destination_path, FILE_SEPARATOR_CHAR);
    if (filename && filename[1] != '\0')
      filename++;
    else
      filename = destination_path;

    prompt_written = snprintf(overwrite_prompt, sizeof(overwrite_prompt),
                              "Overwrite %s? (y/n)", filename);
    if (prompt_written < 0 || (size_t)prompt_written >= sizeof(overwrite_prompt))
      return -1;
    if (InputChoiceLiteral(ctx, overwrite_prompt, "YN\033") != 'Y')
      return 1;
  } else if (errno != ENOENT) {
    UI_ShowStatusLineError(ctx, "Cannot access destination path");
    return -1;
  }

#ifdef HAVE_LIBARCHIVE
  ArchiveExpandedEntry *entry;
  const char **source_paths = NULL;
  const char **archive_paths = NULL;
  size_t source_count = 0;
  size_t idx = 0;
  int rc;

  for (entry = payload->expanded_file_list; entry; entry = entry->next) {
    if (SourcePathMatchesArchiveDestination(entry->source_path, destination_path))
      continue;
    source_count++;
  }

  if (source_count == 0) {
    UI_ShowStatusLineError(ctx, "Nothing to archive");
    return -1;
  }

  source_paths = (const char **)xcalloc(source_count, sizeof(*source_paths));
  archive_paths = (const char **)xcalloc(source_count, sizeof(*archive_paths));
  if (!source_paths || !archive_paths) {
    free((void *)source_paths);
    free((void *)archive_paths);
    UI_ShowStatusLineError(ctx, "Out of memory while creating archive");
    return -1;
  }
  for (entry = payload->expanded_file_list; entry; entry = entry->next) {
    if (SourcePathMatchesArchiveDestination(entry->source_path, destination_path))
      continue;
    source_paths[idx++] = entry->source_path;
    archive_paths[idx - 1] = entry->archive_path;
  }

  rc = Archive_CreateFromPaths(destination_path, source_paths, archive_paths,
                               source_count);
  free((void *)source_paths);
  free((void *)archive_paths);
  if (rc == 0)
    return 0;
  if (rc == UNSUPPORTED_FORMAT_ERROR) {
    UI_ShowStatusLineError(ctx, "Unsupported archive format: %s",
                           UnsupportedArchiveLabel(destination_path));
  } else {
    UI_ShowStatusLineError(ctx, "Failed to create archive");
  }
  return -1;
#else
  UI_ShowStatusLineError(ctx, "Archive creation requires libarchive support");
  return -1;
#endif
}

int GetRenameParameter(ViewContext *ctx, const char *old_name, char *new_name) {
  const char *prompt;

  if (old_name == NULL) {
    prompt = "RENAME TAGGED FILES TO:";
  } else {
    prompt = "RENAME TO:";
  }

  CopyBoundedString(new_name, PATH_LENGTH + 1, (old_name) ? old_name : "*");

  if (UI_ReadString(ctx, ctx->active, prompt, new_name, PATH_LENGTH,
                    HST_FILE) != CR)
    return (-1);

  if (!strlen(new_name))
    return (-1);

  if (old_name && !strcmp(old_name, new_name)) {
    UI_Message(ctx, "Can't rename: New name same as old name.");
    return (-1);
  }

  if (strrchr(new_name, FILE_SEPARATOR_CHAR) != NULL) {
    UI_Message(ctx, "Invalid new name:*No slashes when renaming!");
    return (-1);
  }

  return (0);
}

int UI_ArchiveCallback(int status, const char *msg, void *user_data) {
  ViewContext *ctx = (ViewContext *)user_data;
  if (status == ARCHIVE_STATUS_PROGRESS) {
    if (ctx)
      DrawSpinner(ctx);
    if (EscapeKeyPressed()) {
      return ARCHIVE_CB_ABORT;
    }
  } else if (status == ARCHIVE_STATUS_ERROR) {
    if (msg && ctx)
      UI_Message(ctx, "%s", msg);
  } else if (status == ARCHIVE_STATUS_WARNING) {
    if (msg && ctx)
      UI_Warning(ctx, "%s", msg);
  }
  return ARCHIVE_CB_CONTINUE;
}

int GetCommandLine(ViewContext *ctx, char *command_line) {
  int result = -1;

  ClearHelp(ctx);

  if (UI_ReadString(ctx, ctx->active, "COMMAND:", command_line,
                    COMMAND_LINE_LENGTH, HST_EXEC) == CR) {
    result = 0;
  }

  wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);

  return (result);
}

int GetSearchCommandLine(ViewContext *ctx, char *command_line,
                         char *raw_pattern) {
  int result = -1;
  char input_buf[256];

  ClearHelp(ctx);

  input_buf[0] = '\0';

  if (UI_ReadString(ctx, ctx->active, "SEARCH TAGGED:", input_buf, 256,
                    HST_SEARCH) == CR) {
    size_t command_len;

    if (raw_pattern) {
      strncpy(raw_pattern, input_buf, 255);
      raw_pattern[255] = '\0';
    }

    if (!Path_CommandInit(command_line, COMMAND_LINE_LENGTH + 1, &command_len,
                          "grep -i --") ||
        !Path_CommandAppendLiteral(command_line, COMMAND_LINE_LENGTH + 1,
                                   &command_len, " ") ||
        !Path_CommandAppendQuotedArg(command_line, COMMAND_LINE_LENGTH + 1,
                                     &command_len, input_buf) ||
        !Path_CommandAppendLiteral(command_line, COMMAND_LINE_LENGTH + 1,
                                   &command_len, " {}")) {
      UI_Warning(ctx, "Search command too long.");
    } else {
      result = 0;
    }
  }

  wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);

  return (result);
}

int GetPipeCommand(ViewContext *ctx, char *pipe_command) {
  int result = -1;

  ClearHelp(ctx);

  if (UI_ReadString(ctx, ctx->active, "Pipe-Command:", pipe_command,
                    PATH_LENGTH, HST_PIPE) == CR) {
    result = 0;
  }

  wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);

  return (result);
}

int SystemCall(ViewContext *ctx, const char *command_line, Statistic *s) {
  int result;

  endwin(); /* Ensure terminal state is reset before external command */
  result = SilentSystemCall(ctx, command_line, s);

  (void)GetAvailBytes(&s->disk_space, s);
  /* Full screen redraw to fully restore the curses UI */
  touchwin(stdscr);
  wnoutrefresh(stdscr);
  doupdate();
  return (result);
}

int QuerySystemCall(ViewContext *ctx, const char *command_line, Statistic *s) {
  int result;

  endwin(); /* 1. Save state / Exit curses mode */

  /* 2. Execute command (runs outside curses) */
  result = SilentSystemCallEx(ctx, command_line, FALSE, s);

  /* The external command has finished. We are still in the raw terminal. */

  HitReturnToContinue(); /* 3. Print message and wait for key in raw terminal */

  /* 4. Aggressive redraw/refresh to restore the curses UI completely */
  touchwin(stdscr);
  wnoutrefresh(stdscr);
  doupdate();

  (void)GetAvailBytes(&s->disk_space, s);

  return (result);
}

int UI_ReadFilter(ViewContext *ctx) {
  int result = -1;
  char buffer[FILE_SPEC_LENGTH * 2 + 1];

  ClearHelp(ctx);
  /* Pre-fill with current filter value */
  CopyBoundedString(buffer, sizeof(buffer),
                    ctx->active->vol->vol_stats.file_spec);

  if (UI_ReadString(ctx, ctx->active, "FILTER:", buffer, FILE_SPEC_LENGTH,
                    HST_FILTER) == CR) {
    if (SetFilter(buffer, &ctx->active->vol->vol_stats)) {
      UI_Message(ctx, "Invalid Filter Spec");
    } else {
      CopyBoundedString(ctx->active->vol->vol_stats.file_spec,
                        sizeof(ctx->active->vol->vol_stats.file_spec), buffer);
      result = 0;
    }
  }
  wmove(ctx->ctx_border_window, ctx->layout.message_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);
  return (result);
}

void UI_HandleSort(ViewContext *ctx, DirEntry *dir_entry, Statistic *s,
                   int start_x) {
  int c;
  int sort_kind = 0;
  int order = SORT_ASC;
  WINDOW *sort_win =
      (ctx && ctx->ctx_menu_window) ? ctx->ctx_menu_window : stdscr;

  if (!ctx || !ctx->active || !s)
    return;

  DrawSortPrompt(ctx, sort_win, TRUE);

  do {
    c = Getch(ctx);
    if (c == -1 || c == ESC)
      return;
    c = toupper(c);
    if (c == 'Q')
      return;

    switch (c) {
    case 'N':
      sort_kind = SORT_BY_NAME;
      break;
    case 'E':
      sort_kind = SORT_BY_EXTENSION;
      break;
    case 'M':
      sort_kind = SORT_BY_MOD_TIME;
      break;
    case 'A':
      sort_kind = SORT_BY_ACC_TIME;
      break;
    case 'C':
      sort_kind = SORT_BY_CHG_TIME;
      break;
    case 'G':
      sort_kind = SORT_BY_GROUP;
      break;
    case 'W':
      sort_kind = SORT_BY_OWNER;
      break;
    case 'S':
      sort_kind = SORT_BY_SIZE;
      break;
    case 'O':
      if (order == SORT_ASC) {
        order = SORT_DSC;
      } else {
        order = SORT_ASC;
      }
      DrawSortPrompt(ctx, sort_win, (order == SORT_ASC));
      break;
    }
  } while (!strchr("ACEGMNWS", c));

  SetKindOfSort(sort_kind + order, s);

  if (dir_entry) {
    dir_entry->start_file = 0;
    dir_entry->cursor_pos = 0;
  }

  if (ctx->active) {
    Panel_Sort(ctx->active, s->kind_of_sort);
    if (dir_entry) {
      DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                   dir_entry->start_file + dir_entry->cursor_pos, start_x,
                   ctx->ctx_file_window);
    }
  }
}

