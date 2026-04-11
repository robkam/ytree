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

static BOOL CopyBoundedStringChecked(char *dst, size_t dst_size,
                                     const char *src) {
  int written;

  if (!dst || dst_size == 0)
    return FALSE;

  if (!src)
    src = "";

  written = snprintf(dst, dst_size, "%s", src);
  if (written < 0) {
    dst[0] = '\0';
    return FALSE;
  }
  if ((size_t)written >= dst_size) {
    dst[dst_size - 1] = '\0';
    return FALSE;
  }
  return TRUE;
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

static BOOL is_valid_mode_char(int idx, int ch) {
  switch (idx) {
  case 0:
    return (ch == '?' || ch == '-' || ch == 'd' || ch == 'l');
  case 1:
  case 4:
  case 7:
    return (ch == 'r' || ch == '-' || ch == '?');
  case 2:
  case 5:
  case 8:
    return (ch == 'w' || ch == '-' || ch == '?');
  case 3:
    return (ch == 'x' || ch == 's' || ch == 'S' || ch == '-' || ch == '?');
  case 6:
    return (ch == 'x' || ch == 's' || ch == 'S' || ch == '-' || ch == '?');
  case 9:
    return (ch == 'x' || ch == 't' || ch == 'T' || ch == '-' || ch == '?');
  default:
    return FALSE;
  }
}

int UI_ParseModeInput(const char *input, char *out_mode, char *preview_mode) {
  size_t len;
  int i;
  char attributes[11];

  if (!input || !out_mode || !preview_mode)
    return -1;

  len = strlen(input);
  if (len == 3 || len == 4) {
    mode_t mode_bits = 0;
    mode_t synthetic_mode;

    for (i = 0; i < (int)len; i++) {
      if (input[i] < '0' || input[i] > '7')
        return -1;
    }

    mode_bits = (mode_t)strtol(input, NULL, 8);
    synthetic_mode = S_IFREG | (mode_bits & 0777);
    if (mode_bits & 04000)
      synthetic_mode |= S_ISUID;
    if (mode_bits & 02000)
      synthetic_mode |= S_ISGID;
#ifdef S_ISVTX
    if (mode_bits & 01000)
      synthetic_mode |= S_ISVTX;
#endif

    (void)GetAttributes((unsigned short)synthetic_mode, attributes);
    out_mode[0] = '?';
    memcpy(&out_mode[1], &attributes[1], 9);
    out_mode[10] = '\0';
    strncpy(preview_mode, &attributes[1], 9);
    preview_mode[9] = '\0';
    return 0;
  }

  if (len == 9) {
    out_mode[0] = '?';
    for (i = 0; i < 9; i++) {
      if (!is_valid_mode_char(i + 1, input[i]))
        return -1;
      out_mode[i + 1] = (char)input[i];
    }
    out_mode[10] = '\0';
    strncpy(preview_mode, input, 9);
    preview_mode[9] = '\0';
    return 0;
  }

  if (len == 10) {
    for (i = 0; i < 10; i++) {
      if (!is_valid_mode_char(i, input[i]))
        return -1;
      out_mode[i] = (char)input[i];
    }
    out_mode[10] = '\0';
    strncpy(preview_mode, &out_mode[1], 9);
    preview_mode[9] = '\0';
    return 0;
  }

  return -1;
}

static void format_mode_prompt_value(mode_t mode, char *buffer, size_t size) {
  char attributes[11];

  if (!buffer || size == 0)
    return;

  (void)GetAttributes((unsigned short)mode, attributes);
  (void)snprintf(buffer, size, "%s", attributes);
}

static int parse_date_input(const char *input, time_t base_time,
                            time_t *out_time) {
  struct tm tm_value;
  const struct tm *base_tm_ptr = NULL;
  int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
  int matched = 0;
  time_t parsed_time;
  time_t effective_base_time;

  if (!input || !out_time)
    return -1;

  effective_base_time = (base_time > 0) ? base_time : time(NULL);
  base_tm_ptr = localtime(&effective_base_time);

  memset(&tm_value, 0, sizeof(tm_value));
  if (base_tm_ptr) {
    tm_value = *base_tm_ptr;
  } else {
    tm_value.tm_hour = 0;
    tm_value.tm_min = 0;
    tm_value.tm_sec = 0;
  }
  tm_value.tm_isdst = -1;

  matched = sscanf(input, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour,
                   &minute, &second);
  if (matched != 6) {
    matched =
        sscanf(input, "%d-%d-%d %d:%d", &year, &month, &day, &hour, &minute);
    if (matched != 5) {
      matched = sscanf(input, "%d-%d-%d", &year, &month, &day);
    }
  }
  if (matched != 3 && matched != 5 && matched != 6)
    return -1;

  if (month < 1 || month > 12 || day < 1 || day > 31 || hour < 0 || hour > 23 ||
      minute < 0 || minute > 59 || second < 0 || second > 59) {
    return -1;
  }

  tm_value.tm_year = year - 1900;
  tm_value.tm_mon = month - 1;
  tm_value.tm_mday = day;
  if (matched >= 5) {
    tm_value.tm_hour = hour;
    tm_value.tm_min = minute;
  }
  if (matched == 6) {
    tm_value.tm_sec = second;
  }

  parsed_time = mktime(&tm_value);
  if (parsed_time == (time_t)-1)
    return -1;

  *out_time = parsed_time;
  return 0;
}

static int change_path_date(ViewContext *ctx, const char *path,
                            struct stat *stat_buf) {
  struct utimbuf times;
  struct stat updated_stat;
  time_t new_mtime = stat_buf->st_mtime;
  int scope_mask = 0;

  if (UI_GetDateChangeSpec(ctx, &new_mtime, &scope_mask) != 0) {
    return -1;
  }

  times.actime =
      (scope_mask & DATE_SCOPE_ACCESS) ? new_mtime : stat_buf->st_atime;
  times.modtime =
      (scope_mask & DATE_SCOPE_MODIFY) ? new_mtime : stat_buf->st_mtime;

  if (utime(path, &times) != 0) {
    UI_Message(ctx, "Can't change date:*\"%s\"*%s", path, strerror(errno));
    wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
    wclrtoeol(ctx->ctx_border_window);
    wmove(ctx->ctx_border_window, ctx->layout.status_y, 0);
    wclrtoeol(ctx->ctx_border_window);
    wnoutrefresh(ctx->ctx_border_window);
    return -1;
  }

  if (stat(path, &updated_stat) != 0) {
    UI_Message(ctx, "Can't stat after date change:*\"%s\"*%s", path,
               strerror(errno));
    wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
    wclrtoeol(ctx->ctx_border_window);
    wmove(ctx->ctx_border_window, ctx->layout.status_y, 0);
    wclrtoeol(ctx->ctx_border_window);
    wnoutrefresh(ctx->ctx_border_window);
    return -1;
  }
  *stat_buf = updated_stat;

  wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wmove(ctx->ctx_border_window, ctx->layout.status_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);
  return 0;
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

static YtreePanel *GetInactivePanel(ViewContext *ctx) {
  if (!ctx || !ctx->is_split_screen || !ctx->active || !ctx->left ||
      !ctx->right)
    return NULL;
  return (ctx->active == ctx->left) ? ctx->right : ctx->left;
}

static int GetPanelSelectedDirPath(ViewContext *ctx, YtreePanel *panel,
                                   char *out_path) {
  DirEntry *dir_entry = NULL;

  if (!ctx || !panel || !panel->vol || !out_path)
    return -1;

  if (!panel->vol->dir_entry_list || panel->vol->total_dirs <= 0) {
    BuildDirEntryList(ctx, panel->vol, &panel->current_dir_entry);
  }
  if (!panel->vol->dir_entry_list || panel->vol->total_dirs <= 0)
    return -1;

  dir_entry = GetPanelDirEntry(panel);
  if (!dir_entry)
    return -1;

  GetPath(dir_entry, out_path);
  out_path[PATH_LENGTH] = '\0';
  return 0;
}

static int GetPanelLoggedRootPath(YtreePanel *panel, char *out_path) {
  if (!panel || !panel->vol || !panel->vol->vol_stats.tree || !out_path)
    return -1;

  GetPath(panel->vol->vol_stats.tree, out_path);
  out_path[PATH_LENGTH] = '\0';
  return 0;
}

static int GetPanelSelectedFilePath(ViewContext *ctx, YtreePanel *panel,
                                    char *out_path) {
  const DirEntry *dir_entry = NULL;
  FileEntry *file_entry = NULL;
  int file_idx;

  if (!ctx || !panel || !panel->vol || !out_path)
    return -1;

  if (!panel->vol->dir_entry_list || panel->vol->total_dirs <= 0) {
    BuildDirEntryList(ctx, panel->vol, &panel->current_dir_entry);
  }
  if (!panel->vol->dir_entry_list || panel->vol->total_dirs <= 0)
    return -1;

  BuildFileEntryList(ctx, panel);
  if (!panel->file_entry_list || panel->file_count == 0)
    return -1;

  dir_entry = GetPanelDirEntry(panel);
  if (!dir_entry)
    return -1;

  if (panel->saved_focus == FOCUS_FILE && panel->file_dir_entry == dir_entry) {
    file_idx = panel->start_file + panel->file_cursor_pos;
  } else {
    file_idx = dir_entry->start_file + dir_entry->cursor_pos;
  }
  if (file_idx < 0)
    file_idx = 0;
  if ((unsigned int)file_idx >= panel->file_count)
    file_idx = (int)panel->file_count - 1;
  if (file_idx < 0)
    return -1;

  file_entry = panel->file_entry_list[file_idx].file;
  if (!file_entry)
    return -1;

  GetFileNamePath(file_entry, out_path);
  out_path[PATH_LENGTH] = '\0';
  return 0;
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
    } else if (GetPanelSelectedFilePath(ctx, ctx->active, source_path) == 0) {
      source_path[PATH_LENGTH] = '\0';
    } else if (GetPanelSelectedDirPath(ctx, ctx->active, source_path) == 0) {
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
        GetPanelSelectedDirPath(ctx, ctx->active, cwd) == 0) {
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

typedef enum {
  COMPARE_HELP_FILE_TARGET = 0,
  COMPARE_HELP_DIRECTORY_TARGET,
  COMPARE_HELP_TREE_TARGET,
  COMPARE_HELP_SCOPE,
  COMPARE_HELP_EXTERNAL_SCOPE,
  COMPARE_HELP_BASIS,
  COMPARE_HELP_RESULTS
} CompareHelpTopic;

static void ClearComparePromptArea(ViewContext *ctx) {
  if (!ctx || !ctx->ctx_border_window)
    return;

#ifdef COLOR_SUPPORT
  wattrset(ctx->ctx_border_window, COLOR_PAIR(CPAIR_MENU));
#else
  wattrset(ctx->ctx_border_window, A_NORMAL);
#endif
  wattroff(ctx->ctx_border_window, A_ALTCHARSET);

  if (ctx->layout.prompt_y > 0) {
    wmove(ctx->ctx_border_window, ctx->layout.prompt_y - 1, 0);
    wclrtoeol(ctx->ctx_border_window);
  }
  wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wmove(ctx->ctx_border_window, ctx->layout.status_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);
  doupdate();
}

static void DrawComparePrompt(ViewContext *ctx, const char *prompt) {
  if (!ctx || !ctx->ctx_border_window || !prompt)
    return;

  ClearComparePromptArea(ctx);
#ifdef COLOR_SUPPORT
  wattrset(ctx->ctx_border_window, COLOR_PAIR(CPAIR_MENU));
#else
  wattrset(ctx->ctx_border_window, A_NORMAL);
#endif
  wattroff(ctx->ctx_border_window, A_ALTCHARSET);
  PrintMenuOptions(ctx->ctx_border_window, ctx->layout.prompt_y, 1,
                   (char *)prompt, CPAIR_MENU, CPAIR_HIMENUS);
  PrintMenuOptions(ctx->ctx_border_window, ctx->layout.status_y, 1,
                   "COMMANDS  (F1) context help  (Esc) cancel", CPAIR_MENU,
                   CPAIR_HIMENUS);
  wnoutrefresh(ctx->ctx_border_window);
  doupdate();
}

static void GetCompareHelpLines(CompareHelpTopic topic, const char **title,
                                const char **line_0, const char **line_1,
                                const char **line_2) {
  if (!title || !line_0 || !line_1 || !line_2)
    return;

  *title = "Compare Help";

  switch (topic) {
  case COMPARE_HELP_FILE_TARGET:
    *line_0 = "Current file is the compare source.";
    *line_1 = "Enter the target path, or use (F2) browse and (Up) history.";
    *line_2 = "In split view, the inactive panel provides the default target.";
    break;
  case COMPARE_HELP_DIRECTORY_TARGET:
    *line_0 = "Current directory is the compare source.";
    *line_1 = "Enter the target path, or use (F2) browse and (Up) history.";
    *line_2 = "In split view, the inactive panel provides the default target.";
    break;
  case COMPARE_HELP_TREE_TARGET:
    *line_0 = "Current logged tree is the compare source.";
    *line_1 = "Enter the target path, or use (F2) browse and (Up) history.";
    *line_2 = "In split view, the inactive panel provides the default target.";
    break;
  case COMPARE_HELP_SCOPE:
    *line_0 = "Directory only compares the current directory.";
    *line_1 = "Logged tree compares the current logged tree recursively.";
    *line_2 = "Tree compare never auto-logs unopened '+' subdirectories.";
    break;
  case COMPARE_HELP_EXTERNAL_SCOPE:
    *line_0 = "External compare launches DIRDIFF/TREEDIFF viewer commands.";
    *line_1 = "It does not tag files and does not replace internal compare.";
    *line_2 = "Choose directory or logged tree source scope for launch.";
    break;
  case COMPARE_HELP_BASIS:
    *line_0 = "Size checks file length. Date checks the last-modified time.";
    *line_1 =
        "siZe+date marks a difference if size or modification time differs.";
    *line_2 = "Hash opens both files and compares their content exactly, so it "
              "is slower.";
    break;
  case COMPARE_HELP_RESULTS:
  default:
    *line_0 = "Choose which compare result to tag in the active file list.";
    *line_1 =
        "diFferent tags basis mismatches. Unique tags source-only entries.";
    *line_2 =
        "Match/Newer/Older/Type-mismatch/Error each tag only that outcome.";
    break;
  }
}

static void ShowCompareHelpPopup(ViewContext *ctx, CompareHelpTopic topic) {
  WINDOW *win;
  const char *title = NULL;
  const char *line_0 = NULL;
  const char *line_1 = NULL;
  const char *line_2 = NULL;
  const char *close_prompt = "(F1)/(Esc) close help";
  const char *help_lines[3];
  int height;
  int i;
  int width;
  int win_x;
  int win_y;

  if (!ctx)
    return;

  GetCompareHelpLines(topic, &title, &line_0, &line_1, &line_2);
  help_lines[0] = line_0;
  help_lines[1] = line_1;
  help_lines[2] = line_2;

  width = StrVisualLength(title) + 8;
  for (i = 0; i < 3; i++) {
    int line_len = StrVisualLength(help_lines[i]) + 4;
    if (line_len > width)
      width = line_len;
  }
  if (StrVisualLength(close_prompt) + 4 > width)
    width = StrVisualLength(close_prompt) + 4;

  width = MINIMUM(width, COLS - 4);
  width = MAXIMUM(width, 42);
  height = 7;
  win_x = MAXIMUM(1, (COLS - width) / 2);
  win_y = MAXIMUM(1, (LINES - height) / 2);

  win = newwin(height, width, win_y, win_x);
  if (!win)
    return;

  UI_Dialog_Push(win, UI_TIER_MODAL);
  keypad(win, TRUE);
  WbkgdSet(ctx, win, COLOR_PAIR(CPAIR_MENU));
  curs_set(0);

  while (1) {
    int ch;

    werase(win);
    box(win, 0, 0);
    mvwprintw(win, 1, MAXIMUM(2, (width - StrVisualLength(title)) / 2), "%s",
              title);
    for (i = 0; i < 3; i++) {
      mvwprintw(win, 2 + i, 2, "%.*s", width - 4, help_lines[i]);
    }
    PrintMenuOptions(win, height - 2, 2, (char *)close_prompt, CPAIR_MENU,
                     CPAIR_HIMENUS);
    wrefresh(win);

    ch = WGetch(ctx, win);
    if (ch != ERR)
      break;
  }

  UI_Dialog_Close(ctx, win);
}

static int ShowCompareHelpCallback(ViewContext *ctx, void *help_data) {
  CompareHelpTopic topic = COMPARE_HELP_FILE_TARGET;

  if (help_data != NULL)
    topic = *(CompareHelpTopic *)help_data;

  ShowCompareHelpPopup(ctx, topic);
  return 0;
}

static int InputCompareChoice(ViewContext *ctx, const char *prompt,
                              const char *valid_terms, int default_choice,
                              CompareHelpTopic help_topic) {
  if (!ctx || !prompt || !valid_terms)
    return ESC;

  while (1) {
    int ch;

    DrawComparePrompt(ctx, prompt);

    ch = WGetch(ctx, ctx->ctx_border_window);
    if (ch < 0)
      continue;

    if (ch == KEY_F(1)) {
      ShowCompareHelpPopup(ctx, help_topic);
      continue;
    }
    if (ch == ESC) {
      ClearComparePromptArea(ctx);
      return ESC;
    }

    if (ch == CR || ch == LF) {
      if (default_choice > 0) {
        ch = default_choice;
      } else {
        continue;
      }
    }

    if (islower(ch))
      ch = toupper(ch);

    if (strchr(valid_terms, ch) != NULL) {
      ClearComparePromptArea(ctx);
      return ch;
    }
  }
}

static int PromptCompareTargetPath(ViewContext *ctx, const char *prompt,
                                   const char *default_path, char *target_path,
                                   CompareHelpTopic help_topic) {
  static const char compare_target_hints[] =
      "(F1) help  (F2) browse  (Up) history  (Enter) OK  (Esc) cancel";

  if (!ctx || !prompt || !target_path)
    return -1;

  if (default_path) {
    strncpy(target_path, default_path, PATH_LENGTH);
    target_path[PATH_LENGTH] = '\0';
  } else {
    target_path[0] = '\0';
  }

  ClearHelp(ctx);
  if (UI_ReadStringWithHelp(ctx, ctx->active, prompt, target_path, PATH_LENGTH,
                            HST_PATH, compare_target_hints,
                            ShowCompareHelpCallback, &help_topic) != CR) {
    return -1;
  }

  if (target_path[0] == '\0')
    return -1;

  return 0;
}

static int PromptCompareBasis(ViewContext *ctx, CompareBasis *basis) {
  int ch;

  if (!ctx || !basis)
    return -1;

  ch =
      InputCompareChoice(ctx, "COMPARE BASIS: (S)ize (D)ate si(Z)e+date (H)ash",
                         "SDZH", 0, COMPARE_HELP_BASIS);
  if (ch == ESC || ch < 0)
    return -1;

  switch (ch) {
  case 'S':
    *basis = COMPARE_BASIS_SIZE;
    return 0;
  case 'D':
    *basis = COMPARE_BASIS_DATE;
    return 0;
  case 'Z':
    *basis = COMPARE_BASIS_SIZE_AND_DATE;
    return 0;
  case 'H':
    *basis = COMPARE_BASIS_HASH;
    return 0;
  default:
    return -1;
  }
}

static int PromptCompareTagResult(ViewContext *ctx,
                                  CompareTagResult *tag_result) {
  int ch;

  if (!ctx || !tag_result)
    return -1;

  ch = InputCompareChoice(
      ctx,
      "TAG FILE LIST: di(F)ferent (M)atch (N)ewer (O)lder (U)nique "
      "(T)ype-mismatch (E)rror",
      "FMNOUTE", 0, COMPARE_HELP_RESULTS);
  if (ch == ESC || ch < 0)
    return -1;

  switch (ch) {
  case 'F':
    *tag_result = COMPARE_TAG_DIFFERENT;
    return 0;
  case 'M':
    *tag_result = COMPARE_TAG_MATCH;
    return 0;
  case 'N':
    *tag_result = COMPARE_TAG_NEWER;
    return 0;
  case 'O':
    *tag_result = COMPARE_TAG_OLDER;
    return 0;
  case 'U':
    *tag_result = COMPARE_TAG_UNIQUE;
    return 0;
  case 'T':
    *tag_result = COMPARE_TAG_TYPE_MISMATCH;
    return 0;
  case 'E':
    *tag_result = COMPARE_TAG_ERROR;
    return 0;
  default:
    return -1;
  }
}

const char *UI_CompareFlowTypeName(CompareFlowType flow_type) {
  switch (flow_type) {
  case COMPARE_FLOW_FILE:
    return "file";
  case COMPARE_FLOW_DIRECTORY:
    return "directory";
  case COMPARE_FLOW_LOGGED_TREE:
    return "tree";
  default:
    return "unknown";
  }
}

const char *UI_CompareBasisName(CompareBasis basis) {
  switch (basis) {
  case COMPARE_BASIS_SIZE:
    return "size";
  case COMPARE_BASIS_DATE:
    return "date";
  case COMPARE_BASIS_SIZE_AND_DATE:
    return "size+date";
  case COMPARE_BASIS_HASH:
    return "hash";
  default:
    return "none";
  }
}

const char *UI_CompareTagResultName(CompareTagResult tag_result) {
  switch (tag_result) {
  case COMPARE_TAG_DIFFERENT:
    return "different";
  case COMPARE_TAG_MATCH:
    return "match";
  case COMPARE_TAG_NEWER:
    return "newer";
  case COMPARE_TAG_OLDER:
    return "older";
  case COMPARE_TAG_UNIQUE:
    return "unique";
  case COMPARE_TAG_TYPE_MISMATCH:
    return "type-mismatch";
  case COMPARE_TAG_ERROR:
    return "error";
  default:
    return "none";
  }
}

const char *UI_GetCompareHelperCommand(const ViewContext *ctx,
                                       CompareFlowType flow_type) {
  const char *helper;

  if (!ctx)
    return "";

  switch (flow_type) {
  case COMPARE_FLOW_FILE:
    return GetProfileValue(ctx, "FILEDIFF");
  case COMPARE_FLOW_DIRECTORY:
    return GetProfileValue(ctx, "DIRDIFF");
  case COMPARE_FLOW_LOGGED_TREE:
    helper = GetProfileValue(ctx, "TREEDIFF");
    if (!helper || !*helper)
      helper = GetProfileValue(ctx, "DIRDIFF");
    return helper ? helper : "";
  default:
    return "";
  }
}

int UI_SelectCompareMenuChoice(ViewContext *ctx, CompareMenuChoice *choice) {
  int ch;

  if (!ctx || !choice)
    return -1;

  ch = InputCompareChoice(ctx,
                          "COMPARE SCOPE: (D)irectory only  logged (T)ree  "
                          "e(X)ternal viewer",
                          "DTX", 'D', COMPARE_HELP_SCOPE);
  if (ch == ESC || ch < 0)
    return -1;

  if (ch == 'D') {
    *choice = COMPARE_MENU_DIRECTORY_ONLY;
    return 0;
  }
  if (ch == 'T') {
    *choice = COMPARE_MENU_DIRECTORY_PLUS_TREE;
    return 0;
  }
  if (ch == 'X') {
    int scope_ch =
        InputCompareChoice(ctx, "EXTERNAL VIEWER: (D)irectory  logged (T)ree",
                           "DT", 'D', COMPARE_HELP_EXTERNAL_SCOPE);
    if (scope_ch == ESC || scope_ch < 0)
      return -1;
    *choice = (scope_ch == 'T') ? COMPARE_MENU_EXTERNAL_TREE
                                : COMPARE_MENU_EXTERNAL_DIRECTORY;
    return 0;
  }

  return -1;
}

int UI_BuildFileCompareRequest(ViewContext *ctx, FileEntry *source_file,
                               CompareRequest *request) {
  YtreePanel *inactive = NULL;
  const char *default_target = NULL;

  if (!ctx || !source_file || !request)
    return -1;

  memset(request, 0, sizeof(*request));
  request->flow_type = COMPARE_FLOW_FILE;
  request->basis = COMPARE_BASIS_NONE;
  request->tag_result = COMPARE_TAG_NONE;

  GetFileNamePath(source_file, request->source_path);
  request->source_path[PATH_LENGTH] = '\0';

  if (ctx->is_split_screen) {
    inactive = GetInactivePanel(ctx);
    if (inactive &&
        GetPanelSelectedFilePath(ctx, inactive, request->target_path) == 0) {
      default_target = request->target_path;
      request->used_split_default_target = TRUE;
    }
  }

  if (PromptCompareTargetPath(
          ctx, "COMPARE TARGET:",
          default_target ? default_target : request->source_path,
          request->target_path, COMPARE_HELP_FILE_TARGET) != 0) {
    return -1;
  }

  return 0;
}

static int BuildDirectoryCompareRequestInternal(ViewContext *ctx,
                                                DirEntry *source_dir,
                                                CompareFlowType flow_type,
                                                CompareRequest *request,
                                                BOOL include_compare_prompts) {
  YtreePanel *inactive = NULL;
  const char *default_target = NULL;

  if (!ctx || !request)
    return -1;
  if (flow_type != COMPARE_FLOW_DIRECTORY &&
      flow_type != COMPARE_FLOW_LOGGED_TREE) {
    return -1;
  }

  memset(request, 0, sizeof(*request));
  request->flow_type = flow_type;

  if (flow_type == COMPARE_FLOW_DIRECTORY) {
    if (!source_dir)
      return -1;
    GetPath(source_dir, request->source_path);
  } else {
    if (!ctx->active || !ctx->active->vol || !ctx->active->vol->vol_stats.tree)
      return -1;
    GetPath(ctx->active->vol->vol_stats.tree, request->source_path);
  }
  request->source_path[PATH_LENGTH] = '\0';

  if (ctx->is_split_screen) {
    inactive = GetInactivePanel(ctx);
    if (inactive) {
      if (flow_type == COMPARE_FLOW_DIRECTORY) {
        if (GetPanelSelectedDirPath(ctx, inactive, request->target_path) == 0) {
          default_target = request->target_path;
          request->used_split_default_target = TRUE;
        }
      } else if (GetPanelLoggedRootPath(inactive, request->target_path) == 0) {
        default_target = request->target_path;
        request->used_split_default_target = TRUE;
      }
    }
  }

  if (PromptCompareTargetPath(ctx, "COMPARE TARGET:",
                              default_target ? default_target
                                             : request->source_path,
                              request->target_path,
                              flow_type == COMPARE_FLOW_DIRECTORY
                                  ? COMPARE_HELP_DIRECTORY_TARGET
                                  : COMPARE_HELP_TREE_TARGET) != 0) {
    return -1;
  }
  request->target_path[PATH_LENGTH] = '\0';

  if (include_compare_prompts) {
    if (PromptCompareBasis(ctx, &request->basis) != 0)
      return -1;
    if (PromptCompareTagResult(ctx, &request->tag_result) != 0)
      return -1;
  } else {
    request->basis = COMPARE_BASIS_NONE;
    request->tag_result = COMPARE_TAG_NONE;
  }

  return 0;
}

int UI_BuildDirectoryCompareRequest(ViewContext *ctx, DirEntry *source_dir,
                                    CompareFlowType flow_type,
                                    CompareRequest *request) {
  return BuildDirectoryCompareRequestInternal(ctx, source_dir, flow_type,
                                              request, TRUE);
}

int UI_BuildDirectoryCompareLaunchRequest(ViewContext *ctx,
                                          DirEntry *source_dir,
                                          CompareFlowType flow_type,
                                          CompareRequest *request) {
  return BuildDirectoryCompareRequestInternal(ctx, source_dir, flow_type,
                                              request, FALSE);
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

int UI_PromptAttributeAction(ViewContext *ctx, BOOL tagged, BOOL allow_date) {
  int ch;
  const char *menu_text;

  if (tagged) {
    menu_text = "ATTRIBUTES: (M)ode (O)wner (G)roup (D)ate (^D)ate";
  } else if (allow_date) {
    menu_text = "ATTRIBUTES: (M)ode (O)wner (G)roup (D)ate";
  } else {
    menu_text = "ATTRIBUTES: (M)ode (O)wner (G)roup";
  }

  ClearHelp(ctx);
  wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  PrintOptions(ctx->ctx_border_window, ctx->layout.prompt_y, 1,
               (char *)menu_text);
  wnoutrefresh(ctx->ctx_border_window);
  doupdate();

  while (1) {
    ch = WGetch(ctx, ctx->ctx_border_window);
    if (ch == ESC) {
      ch = ESC;
      break;
    }
    if (ch < 0)
      continue;
    if (islower(ch))
      ch = toupper(ch);

    if (ch == 'M' || ch == 'O' || ch == 'G' || (allow_date && ch == 'D') ||
        (tagged && (ch == 'D' || ch == 0x04))) {
      break;
    }
  }

  wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);
  doupdate();
  return ch;
}

int UI_GetDateChangeSpec(ViewContext *ctx, time_t *new_time, int *scope_mask) {
  char date_input[32];
  char display_time[32];
  const struct tm *tm_ptr;
  int which;

  if (!ctx || !new_time || !scope_mask)
    return -1;

  time_t base_time = (*new_time > 0) ? *new_time : time(NULL);

  which = InputChoice(ctx, "DATE FIELD: (M)odified (A)ccessed (B)oth", "MAB");
  if (which == ESC || which < 0)
    return -1;

  switch (which) {
  case 'A':
    *scope_mask = DATE_SCOPE_ACCESS;
    break;
  case 'B':
    *scope_mask = DATE_SCOPE_ACCESS | DATE_SCOPE_MODIFY;
    break;
  case 'M':
  default:
    *scope_mask = DATE_SCOPE_MODIFY;
    break;
  }

  CopyBoundedString(display_time, sizeof(display_time), "1970-01-01 00:00:00");
  tm_ptr = localtime(&base_time);
  if (tm_ptr) {
    (void)strftime(display_time, sizeof(display_time), "%Y-%m-%d %H:%M:%S",
                   tm_ptr);
  }

  strncpy(date_input, display_time, sizeof(date_input) - 1);
  date_input[sizeof(date_input) - 1] = '\0';

  ClearHelp(ctx);
  if (UI_ReadString(ctx, ctx->active, "DATE (YYYY-MM-DD [HH:MM[:SS]]):",
                    date_input, (int)sizeof(date_input), HST_GENERAL) != CR) {
    wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
    wclrtoeol(ctx->ctx_border_window);
    wmove(ctx->ctx_border_window, ctx->layout.status_y, 0);
    wclrtoeol(ctx->ctx_border_window);
    wnoutrefresh(ctx->ctx_border_window);
    return -1;
  }

  if (parse_date_input(date_input, base_time, new_time) != 0) {
    UI_Message(ctx, "Invalid date. Use YYYY-MM-DD [HH:MM[:SS]]");
    wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
    wclrtoeol(ctx->ctx_border_window);
    wmove(ctx->ctx_border_window, ctx->layout.status_y, 0);
    wclrtoeol(ctx->ctx_border_window);
    wnoutrefresh(ctx->ctx_border_window);
    return -1;
  }

  wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wmove(ctx->ctx_border_window, ctx->layout.status_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);
  return 0;
}

int ChangeFileModus(ViewContext *ctx, FileEntry *fe_ptr) {
  char mode_input[16];
  WalkingPackage walking_package;
  int result = -1;

  if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
    return (result);
  }

  format_mode_prompt_value(fe_ptr->stat_struct.st_mode, mode_input,
                           sizeof(mode_input));

  ClearHelp(ctx);
  if (UI_ReadString(ctx, ctx->active, "MODE (octal/rwx):", mode_input,
                    (int)sizeof(mode_input), HST_CHANGE_MODUS) == CR) {
    char parsed_mode[12];
    char preview_mode[10];

    if (UI_ParseModeInput(mode_input, parsed_mode, preview_mode) != 0) {
      UI_Message(ctx, "Invalid mode. Use 3/4-digit octal or -rwxrwxrwx");
      wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
      wclrtoeol(ctx->ctx_border_window);
      wnoutrefresh(ctx->ctx_border_window);
      return -1;
    }
    CopyBoundedString(
        walking_package.function_data.change_mode.new_mode,
        sizeof(walking_package.function_data.change_mode.new_mode),
        parsed_mode);
    result = SetFileModus(ctx, fe_ptr, &walking_package);
  }

  wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);

  return (result);
}

int ChangeDirModus(ViewContext *ctx, DirEntry *de_ptr) {
  char mode_input[16];
  WalkingPackage walking_package;
  int result = -1;

  if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
    return (result);
  }

  format_mode_prompt_value(de_ptr->stat_struct.st_mode, mode_input,
                           sizeof(mode_input));

  ClearHelp(ctx);
  if (UI_ReadString(ctx, ctx->active, "MODE (octal/rwx):", mode_input,
                    (int)sizeof(mode_input), HST_CHANGE_MODUS) == CR) {
    char parsed_mode[12];
    char preview_mode[10];

    if (UI_ParseModeInput(mode_input, parsed_mode, preview_mode) != 0) {
      UI_Message(ctx, "Invalid mode. Use 3/4-digit octal or -rwxrwxrwx");
      wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
      wclrtoeol(ctx->ctx_border_window);
      wnoutrefresh(ctx->ctx_border_window);
      return -1;
    }
    CopyBoundedString(
        walking_package.function_data.change_mode.new_mode,
        sizeof(walking_package.function_data.change_mode.new_mode),
        parsed_mode);
    result = SetDirModus(de_ptr, &walking_package);
  }

  wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);

  return (result);
}

int ChangeFileDate(ViewContext *ctx, FileEntry *fe_ptr) {
  char path[PATH_LENGTH + 1];

  if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
    return -1;
  }

  GetFileNamePath(fe_ptr, path);
  return change_path_date(ctx, path, &fe_ptr->stat_struct);
}

int ChangeDirDate(ViewContext *ctx, DirEntry *de_ptr) {
  char path[PATH_LENGTH + 1];

  if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
    return -1;
  }

  GetPath(de_ptr, path);
  return change_path_date(ctx, path, &de_ptr->stat_struct);
}

int GetNewOwner(ViewContext *ctx, int st_uid) {
  char owner[OWNER_NAME_MAX * 2 + 1];
  const char *owner_name_ptr;
  int owner_id;
  int id;

  owner_id = -1;

  id = (st_uid == -1) ? (int)getuid() : st_uid;

  owner_name_ptr = GetPasswdName(id);
  if (owner_name_ptr == NULL) {
    (void)snprintf(owner, sizeof(owner), "%d", id);
  } else {
    CopyBoundedString(owner, sizeof(owner), owner_name_ptr);
  }

  ClearHelp(ctx);

  if (UI_ReadString(ctx, ctx->active, "OWNER:", owner, OWNER_NAME_MAX,
                    HST_ID) == CR) {
    if ((owner_id = GetPasswdUid(owner)) == -1) {
      UI_Message(ctx, "Can't read Owner-ID:*%s", owner);
    }
  }

  wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);

  return (owner_id);
}

int GetNewGroup(ViewContext *ctx, int st_gid) {
  char group[GROUP_NAME_MAX * 2 + 1];
  const char *group_name_ptr;
  int id;
  int group_id;

  group_id = -1;

  id = (st_gid == -1) ? (int)getgid() : st_gid;

  group_name_ptr = GetGroupName(id);
  if (group_name_ptr == NULL) {
    (void)snprintf(group, sizeof(group), "%d", id);
  } else {
    CopyBoundedString(group, sizeof(group), group_name_ptr);
  }

  ClearHelp(ctx);

  if (UI_ReadString(ctx, ctx->active, "GROUP:", group, GROUP_NAME_MAX,
                    HST_ID) == CR) {
    if ((group_id = GetGroupId(group)) == -1) {
      UI_Message(ctx, "Can't read Group-ID:*\"%s\"", group);
    }
  }

  wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);

  return (group_id);
}

int ChangeFileOrDirOwnership(ViewContext *ctx, const char *path,
                             struct stat *stat_buf, BOOL change_owner,
                             BOOL change_group) {
  uid_t new_uid = stat_buf->st_uid;
  gid_t new_gid = stat_buf->st_gid;
  int result = -1;

  if (change_owner) {
    int owner_id = GetNewOwner(ctx, stat_buf->st_uid);
    if (owner_id < 0)
      return -1; /* User cancelled or error */
    new_uid = (uid_t)owner_id;
  }

  if (change_group) {
    int group_id = GetNewGroup(ctx, stat_buf->st_gid);
    if (group_id < 0)
      return -1; /* User cancelled or error */
    new_gid = (gid_t)group_id;
  }

  /* Use the generic helper to perform the system call and update stat */
  result = ChangeOwnership(path, new_uid, new_gid, stat_buf);

  return result;
}

int HandleDirOwnership(ViewContext *ctx, DirEntry *de_ptr, BOOL change_owner,
                       BOOL change_group) {
  char path[PATH_LENGTH + 1];

  if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
    return -1;
  }

  GetPath(de_ptr, path);
  return ChangeFileOrDirOwnership(ctx, path, &de_ptr->stat_struct, change_owner,
                                  change_group);
}

int HandleFileOwnership(ViewContext *ctx, FileEntry *fe_ptr, BOOL change_owner,
                        BOOL change_group) {
  char path[PATH_LENGTH + 1];

  if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
    return -1;
  }

  GetFileNamePath(fe_ptr, path);
  return ChangeFileOrDirOwnership(ctx, path, &fe_ptr->stat_struct, change_owner,
                                  change_group);
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

int recursive_mkdir(char *path) {
  char *p = path;
  if (*p == '/')
    p++;
  while (*p) {
    if (*p == '/') {
      *p = '\0';
      if (mkdir(path, 0700) != 0 && errno != EEXIST)
        return -1;
      *p = '/';
    }
    p++;
  }
  if (mkdir(path, 0700) != 0 && errno != EEXIST)
    return -1;
  return 0;
}

int recursive_rmdir(const char *path) {
  DIR *d = opendir(path);
  const struct dirent *entry;
  char fullpath[PATH_LENGTH];

  if (!d)
    return -1;

  while ((entry = readdir(d))) {
    if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
      continue;
    snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
    struct stat st;
    if (stat(fullpath, &st) == 0) {
      if (S_ISDIR(st.st_mode)) {
        recursive_rmdir(fullpath);
      } else {
        unlink(fullpath);
      }
    }
  }
  closedir(d);
  return rmdir(path);
}

static void SetupTaggedViewWindow(ViewContext *ctx) {
  int start_y;
  int start_x;
  int available_height;
  int width;

  start_y = ctx->layout.dir_win_y;
  start_x = ctx->layout.dir_win_x;
  available_height = ctx->layout.message_y - start_y;
  width = ctx->layout.main_win_width;

  if (available_height < 3)
    available_height = 3;
  if (width < 20)
    width = 20;

  ctx->viewer.wlines = available_height - 2;
  ctx->viewer.wcols = width - 2;

  if (ctx->viewer.border) {
    delwin(ctx->viewer.border);
    ctx->viewer.border = NULL;
  }
  if (ctx->viewer.view) {
    delwin(ctx->viewer.view);
    ctx->viewer.view = NULL;
  }

  ctx->viewer.border = newwin(available_height, width, start_y, start_x);
  ctx->viewer.view =
      newwin(ctx->viewer.wlines, ctx->viewer.wcols, start_y + 1, start_x + 1);

  keypad(ctx->viewer.view, TRUE);
  scrollok(ctx->viewer.view, FALSE);
  clearok(ctx->viewer.view, TRUE);
  leaveok(ctx->viewer.view, FALSE);

#ifdef COLOR_SUPPORT
  WbkgdSet(ctx, ctx->viewer.view, COLOR_PAIR(CPAIR_WINDIR));
  WbkgdSet(ctx, ctx->viewer.border, COLOR_PAIR(CPAIR_WINDIR) | A_BOLD);
#endif
}

static void DestroyTaggedViewWindow(ViewContext *ctx) {
  if (ctx->viewer.view) {
    delwin(ctx->viewer.view);
    ctx->viewer.view = NULL;
  }
  if (ctx->viewer.border) {
    delwin(ctx->viewer.border);
    ctx->viewer.border = NULL;
  }
}

static long TaggedViewPageStep(ViewContext *ctx) {
  long step = (long)ctx->viewer.wlines - 1;
  if (step < 1)
    step = 1;
  return step;
}

static void DrawTaggedViewHeader(ViewContext *ctx, const char *display_path,
                                 int current_idx, int total_count) {
  int i;
  int available;
  char header_buf[PATH_LENGTH + 64];
  char clipped_header[PATH_LENGTH + 64];

  for (i = ctx->layout.header_y; i <= ctx->layout.status_y; i++) {
    wmove(stdscr, i, 0);
    wclrtoeol(stdscr);
  }

  snprintf(header_buf, sizeof(header_buf), "[%d/%d] %s", current_idx + 1,
           total_count, display_path ? display_path : "");
  available = (COLS > 7) ? (COLS - 7) : 1;

  Print(stdscr, ctx->layout.header_y, 0, "File: ", CPAIR_MENU);
  Print(stdscr, ctx->layout.header_y, 6,
        CutPathname(clipped_header, header_buf, available), CPAIR_HIMENUS);

  PrintOptions(stdscr, ctx->layout.message_y, 0,
               "(Q)uit  (Space/PgDn) next page/file  (PgUp) prev page");
  PrintOptions(stdscr, ctx->layout.prompt_y, 0,
               "(N)ext file  (P)rev file (wrap)  (Up/Down) line  (Home/End)");
  PrintOptions(stdscr, ctx->layout.status_y, 0, "View tagged files");

  box(ctx->viewer.border, 0, 0);
  wnoutrefresh(stdscr);
  wnoutrefresh(ctx->viewer.border);
}

static int RunTaggedViewLoop(ViewContext *ctx, char **view_paths,
                             char **display_paths, int path_count) {
  int current_idx = 0;
  int ch;
  BOOL quit = FALSE;
  long line_offset = 0;

  if (!ctx || !view_paths || !display_paths || path_count <= 0)
    return -1;

  SetupTaggedViewWindow(ctx);

  while (!quit) {
    long page_step;

    DrawTaggedViewHeader(ctx, display_paths[current_idx], current_idx,
                         path_count);
    RenderFilePreview(ctx, ctx->viewer.view, view_paths[current_idx],
                      &line_offset, 0);
    RefreshWindow(ctx->viewer.view);
    doupdate();

    ch = (ctx->resize_request) ? -1 : WGetch(ctx, ctx->viewer.view);

    ch = NormalizeViKey(ctx, ch);

    if (ctx->resize_request) {
      ctx->resize_request = FALSE;
      SetupTaggedViewWindow(ctx);
      continue;
    }

    page_step = TaggedViewPageStep(ctx);

    switch (ch) {
#ifdef KEY_RESIZE
    case KEY_RESIZE:
      ctx->resize_request = TRUE;
      break;
#endif
    case ESC:
    case 'q':
    case 'Q':
      quit = TRUE;
      break;
    case 'n':
    case 'N':
      current_idx = (current_idx + 1) % path_count;
      line_offset = 0;
      break;
    case 'p':
    case 'P':
      current_idx = (current_idx + path_count - 1) % path_count;
      line_offset = 0;
      break;
    case KEY_UP:
      if (line_offset > 0)
        line_offset--;
      break;
    case KEY_DOWN:
      if (line_offset < 2000000000L)
        line_offset++;
      break;
    case KEY_HOME:
      line_offset = 0;
      break;
    case KEY_END:
      line_offset = 2000000000L;
      break;
    case KEY_LEFT:
    case KEY_PPAGE:
      if (line_offset > page_step)
        line_offset -= page_step;
      else
        line_offset = 0;
      break;
    case KEY_RIGHT:
    case KEY_NPAGE:
      if (line_offset < 2000000000L - page_step)
        line_offset += page_step;
      else
        line_offset = 2000000000L;
      break;
    case ' ': {
      long old_offset = line_offset;
      long probe_offset = line_offset;

      if (probe_offset < 2000000000L - page_step)
        probe_offset += page_step;
      else
        probe_offset = 2000000000L;

      RenderFilePreview(ctx, ctx->viewer.view, view_paths[current_idx],
                        &probe_offset, 0);
      if (probe_offset == old_offset) {
        if (current_idx < path_count - 1) {
          current_idx++;
          line_offset = 0;
        }
      } else {
        line_offset = probe_offset;
      }
      break;
    }
    case 'L' & 0x1f:
      clearok(stdscr, TRUE);
      break;
    default:
      break;
    }
  }

  DestroyTaggedViewWindow(ctx);
  touchwin(stdscr);
  wnoutrefresh(stdscr);
  doupdate();
  return 0;
}

static int RunExternalTaggedViewer(ViewContext *ctx, char **view_paths,
                                   int path_count, Statistic *s) {
  const char *viewer;
  char *command_line;
  int i;
  size_t current_len;

  if (!ctx || !view_paths || path_count <= 0 || !s)
    return -1;

  viewer = (GetProfileValue)(ctx, "PAGER");
  if (!viewer || !*viewer)
    viewer = "less";

  command_line = (char *)xmalloc(COMMAND_LINE_LENGTH + 1);
  if (!Path_CommandInit(command_line, COMMAND_LINE_LENGTH + 1, &current_len,
                        viewer)) {
    free(command_line);
    return -1;
  }

  if (ctx->global_search_term[0] != '\0') {
    if (!Path_CommandAppendLiteral(command_line, COMMAND_LINE_LENGTH + 1,
                                   &current_len, " -p ") ||
        !Path_CommandAppendQuotedArg(command_line, COMMAND_LINE_LENGTH + 1,
                                     &current_len, ctx->global_search_term)) {
      UI_Warning(ctx, "Search pattern too long.");
      free(command_line);
      return -1;
    }
  }

  for (i = 0; i < path_count; i++) {
    if (!Path_CommandAppendLiteral(command_line, COMMAND_LINE_LENGTH + 1,
                                   &current_len, " ") ||
        !Path_CommandAppendQuotedArg(command_line, COMMAND_LINE_LENGTH + 1,
                                     &current_len, view_paths[i])) {
      UI_Warning(ctx, "Tagged viewer command too long.");
      free(command_line);
      return -1;
    }
  }

  (void)SystemCall(ctx, command_line, s);
  free(command_line);
  return 0;
}

int UI_ViewTaggedFiles(ViewContext *ctx, DirEntry *dir_entry) {
  FileEntry *fe;
  const char *temp_dir = NULL;
  char **view_paths = NULL;
  char **display_paths = NULL;
  char path[PATH_LENGTH + 1];
  int tagged_count = 0;
  int path_count = 0;
  BOOL use_internal_view = FALSE;
  int i;
  char temp_dir_template[PATH_LENGTH];
  Statistic *s;
  const char *tagged_viewer;
  int result = 0;

  (void)dir_entry;
  if (!ctx || !ctx->active || !ctx->active->vol ||
      !ctx->active->file_entry_list)
    return 0;

  s = &ctx->active->vol->vol_stats;
  tagged_viewer = GetProfileValue(ctx, "TAGGEDVIEWER");
  if (tagged_viewer &&
      (!strcasecmp(tagged_viewer, "internal") ||
       !strcasecmp(tagged_viewer, "builtin") ||
       !strcasecmp(tagged_viewer, "ytree") || !strcmp(tagged_viewer, "1"))) {
    use_internal_view = TRUE;
  }

  if (s->log_mode == ARCHIVE_MODE) {
    if (!Path_BuildTempTemplate(temp_dir_template, sizeof(temp_dir_template),
                                "ytree_view_")) {
      UI_Error(ctx, __FILE__, __LINE__,
               "Could not prepare temp dir template for viewing");
      return -1;
    }
    temp_dir = mkdtemp(temp_dir_template);
    if (!temp_dir) {
      UI_Error(ctx, __FILE__, __LINE__,
               "Could not create temp dir for viewing");
      return -1;
    }
  }

  for (i = 0; i < (int)ctx->active->file_count; i++) {
    fe = ctx->active->file_entry_list[i].file;
    if (fe->tagged && fe->matching)
      tagged_count++;
  }

  if (tagged_count <= 0) {
    goto cleanup;
  }

  view_paths = (char **)xcalloc((size_t)tagged_count, sizeof(char *));
  display_paths = (char **)xcalloc((size_t)tagged_count, sizeof(char *));
  if (!view_paths || !display_paths) {
    UI_Error(ctx, __FILE__, __LINE__,
             "Out of memory while preparing tagged view");
    result = -1;
    goto cleanup;
  }

  for (i = 0; i < (int)ctx->active->file_count; i++) {
    fe = ctx->active->file_entry_list[i].file;

    if (!(fe->tagged && fe->matching))
      continue;

    if (s->log_mode == DISK_MODE || s->log_mode == USER_MODE) {
      GetFileNamePath(fe, path);
      view_paths[path_count] = xstrdup(path);
      display_paths[path_count] = xstrdup(path);
      path_count++;
    } else if (s->log_mode == ARCHIVE_MODE) {
#ifdef HAVE_LIBARCHIVE
      if (path_count >= 100) {
        UI_Warning(ctx, "Archive view limit (100) reached.");
        break;
      }

      {
        char t_filename[PATH_LENGTH];
        char t_dirname[PATH_LENGTH];
        char root_path[PATH_LENGTH + 1];
        char relative_path[PATH_LENGTH + 1];
        char internal_path[PATH_LENGTH + 1];
        char canonical_internal_path[PATH_LENGTH + 1];
        char full_path[PATH_LENGTH + 1];
        size_t root_len;

        GetPath(ctx->active->vol->vol_stats.tree, root_path);
        GetFileNamePath(fe, full_path);

        root_len = strlen(root_path);
        if (strncmp(full_path, root_path, root_len) == 0) {
          char *ptr = full_path + root_len;
          if (*ptr == FILE_SEPARATOR_CHAR) {
            ptr++;
            if (!CopyBoundedStringChecked(relative_path, sizeof(relative_path),
                                          ptr)) {
              UI_Warning(ctx, "Skipped long archive path*\"%s\"", fe->name);
              continue;
            }
          } else if (*ptr == '\0') {
            relative_path[0] = '\0';
          } else {
            if (!CopyBoundedStringChecked(relative_path, sizeof(relative_path),
                                          full_path)) {
              UI_Warning(ctx, "Skipped long archive path*\"%s\"", fe->name);
              continue;
            }
          }
        } else {
          if (!CopyBoundedStringChecked(relative_path, sizeof(relative_path),
                                        full_path)) {
            UI_Warning(ctx, "Skipped long archive path*\"%s\"", fe->name);
            continue;
          }
        }

        if (strlen(relative_path) == 0) {
          if (!CopyBoundedStringChecked(relative_path, sizeof(relative_path),
                                        fe->name)) {
            UI_Warning(ctx, "Skipped long archive path*\"%s\"", fe->name);
            continue;
          }
        }

        if (!CopyBoundedStringChecked(internal_path, sizeof(internal_path),
                                      relative_path)) {
          UI_Warning(ctx, "Skipped long archive path*\"%s\"", fe->name);
          continue;
        }
        if (Archive_ValidateInternalPath(internal_path, canonical_internal_path,
                                         sizeof(canonical_internal_path)) != 0) {
          UI_Warning(ctx, "Skipped unsafe archive member path*\"%s\"",
                     internal_path);
          continue;
        }

        if (strlen(canonical_internal_path) > 0) {
          char *dir_only;

          snprintf(t_dirname, sizeof(t_dirname), "%s/%s", temp_dir,
                   canonical_internal_path);
          dir_only = xstrdup(t_dirname);
          dirname(dir_only);
          if (recursive_mkdir(dir_only) != 0) {
            UI_Warning(ctx, "Failed to prepare temp path*\"%s\"",
                       canonical_internal_path);
            free(dir_only);
            continue;
          }
          free(dir_only);
          snprintf(t_filename, sizeof(t_filename), "%s/%s", temp_dir,
                   canonical_internal_path);
        } else {
          snprintf(t_filename, sizeof(t_filename), "%s/%s", temp_dir, fe->name);
        }

        if (ExtractArchiveNode(s->log_path, canonical_internal_path, t_filename,
                               UI_ArchiveCallback, ctx) == 0) {
          view_paths[path_count] = xstrdup(t_filename);
          display_paths[path_count] = xstrdup(canonical_internal_path);
          path_count++;
        } else {
          UI_Warning(ctx, "Failed to extract*\"%s\"", canonical_internal_path);
        }
      }
#else
      if (path_count == 0) {
        UI_Message(ctx, "Archive view requires libarchive support.");
      }
#endif
    }
  }

  if (path_count > 0) {
    if (use_internal_view) {
      RunTaggedViewLoop(ctx, view_paths, display_paths, path_count);
    } else {
      RunExternalTaggedViewer(ctx, view_paths, path_count, s);
    }
  } else if (s->log_mode == ARCHIVE_MODE) {
    UI_Message(ctx, "No files extracted.");
  }

cleanup:
  for (i = 0; i < tagged_count; i++) {
    if (view_paths && view_paths[i])
      free(view_paths[i]);
    if (display_paths && display_paths[i])
      free(display_paths[i]);
  }
  free(view_paths);
  free(display_paths);

  if (s->log_mode == ARCHIVE_MODE && temp_dir) {
    recursive_rmdir(temp_dir);
  }

  return result;
}
