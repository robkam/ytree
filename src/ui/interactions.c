/***************************************************************************
 *
 * src/ui/interactions.c
 * UI Prompts and Interaction Functions
 *
 ***************************************************************************/

#include "sort.h"
#include "watcher.h"
#include "ytree.h"
#include "ytree_ui.h"
#include <ctype.h>
#include <errno.h>
#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <libgen.h>
#include <pwd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <utime.h>
#include <unistd.h>

static char move_prompt_header[PATH_LENGTH + 50];
static char move_prompt_as[PATH_LENGTH + 1];

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
  struct tm *base_tm_ptr = NULL;
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
    tm_value.tm_year = 70;
    tm_value.tm_mon = 0;
    tm_value.tm_mday = 1;
    tm_value.tm_hour = 0;
    tm_value.tm_min = 0;
    tm_value.tm_sec = 0;
  }
  tm_value.tm_isdst = -1;

  matched = sscanf(input, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour,
                   &minute, &second);
  if (matched != 6) {
    matched = sscanf(input, "%d-%d-%d %d:%d", &year, &month, &day, &hour,
                     &minute);
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

int GetMoveParameter(ViewContext *ctx, char *from_file, char *to_file,
                     char *to_dir) {
  if (from_file == NULL) {
    from_file = "TAGGED FILES";
    (void)strcpy(to_file, "*");
  } else {
    (void)strcpy(to_file, from_file);
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
        strcpy(to_dir, ".");
      }
      return (0);
    }
  }
  ClearHelp(ctx);
  return (-1);
}

int GetCopyParameter(ViewContext *ctx, char *from_file, BOOL path_copy,
                     char *to_file, char *to_dir) {
  char prompt_header[PATH_LENGTH + 50];
  char prompt_as[PATH_LENGTH + 1];

  if (from_file == NULL) {
    from_file = "TAGGED FILES";
    (void)strcpy(to_file, "*");
  } else {
    (void)strcpy(to_file, from_file);
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

    strncpy(prompt_as, to_file, PATH_LENGTH);
    prompt_as[PATH_LENGTH] = '\0';

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
        strcpy(to_dir, ".");
      }
      return (0);
    }
  }
  ClearHelp(ctx);
  return (-1);
}

int GetRenameParameter(ViewContext *ctx, const char *old_name, char *new_name) {
  const char *prompt;

  if (old_name == NULL) {
    prompt = "RENAME TAGGED FILES TO:";
  } else {
    prompt = "RENAME TO:";
  }

  (void)strcpy(new_name, (old_name) ? old_name : "*");

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
  PrintOptions(ctx->ctx_border_window, ctx->layout.prompt_y, 1, (char *)menu_text);
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
  struct tm *tm_ptr;
  int which;
  time_t base_time = (*new_time > 0) ? *new_time : time(NULL);

  if (!ctx || !new_time || !scope_mask)
    return -1;

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

  (void)strcpy(display_time, "1970-01-01 00:00:00");
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
  char parsed_mode[12];
  char preview_mode[10];
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
    if (UI_ParseModeInput(mode_input, parsed_mode, preview_mode) != 0) {
      UI_Message(ctx, "Invalid mode. Use 3/4-digit octal or -rwxrwxrwx");
      wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
      wclrtoeol(ctx->ctx_border_window);
      wnoutrefresh(ctx->ctx_border_window);
      return -1;
    }
    (void)strcpy(walking_package.function_data.change_mode.new_mode, parsed_mode);
    result = SetFileModus(ctx, fe_ptr, &walking_package);
  }

  wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);

  return (result);
}

int ChangeDirModus(ViewContext *ctx, DirEntry *de_ptr) {
  char mode_input[16];
  char parsed_mode[12];
  char preview_mode[10];
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
    if (UI_ParseModeInput(mode_input, parsed_mode, preview_mode) != 0) {
      UI_Message(ctx, "Invalid mode. Use 3/4-digit octal or -rwxrwxrwx");
      wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
      wclrtoeol(ctx->ctx_border_window);
      wnoutrefresh(ctx->ctx_border_window);
      return -1;
    }
    (void)strcpy(walking_package.function_data.change_mode.new_mode, parsed_mode);
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
  char *owner_name_ptr;
  int owner_id;
  int id;

  owner_id = -1;

  id = (st_uid == -1) ? (int)getuid() : st_uid;

  owner_name_ptr = GetPasswdName(id);
  if (owner_name_ptr == NULL) {
    (void)snprintf(owner, sizeof(owner), "%d", id);
  } else {
    (void)strcpy(owner, owner_name_ptr);
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
  char *group_name_ptr;
  int id;
  int group_id;

  group_id = -1;

  id = (st_gid == -1) ? (int)getgid() : st_gid;

  group_name_ptr = GetGroupName(id);
  if (group_name_ptr == NULL) {
    (void)snprintf(group, sizeof(group), "%d", id);
  } else {
    (void)strcpy(group, group_name_ptr);
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
    if (raw_pattern) {
      strncpy(raw_pattern, input_buf, 255);
      raw_pattern[255] = '\0';
    }

    /* Construct command line: grep -i "pattern" {} */
    snprintf(command_line, COMMAND_LINE_LENGTH, "grep -i \"%s\" {}", input_buf);

    result = 0;
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

int SystemCall(ViewContext *ctx, char *command_line, Statistic *s) {
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

int QuerySystemCall(ViewContext *ctx, char *command_line, Statistic *s) {
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
  (void)strcpy(buffer, ctx->active->vol->vol_stats.file_spec);

  if (UI_ReadString(ctx, ctx->active, "FILTER:", buffer, FILE_SPEC_LENGTH,
                    HST_FILTER) == CR) {
    if (SetFilter(buffer, &ctx->active->vol->vol_stats)) {
      UI_Message(ctx, "Invalid Filter Spec");
    } else {
      (void)strcpy(ctx->active->vol->vol_stats.file_spec, buffer);
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

  wmove(stdscr, Y_PROMPT(ctx), 0);
  clrtoeol();
  PrintOptions(
      stdscr, Y_PROMPT(ctx), 0,
      "SORT by   (A)ccTime (C)hgTime (E)xtension (G)roup (M)odTime (N)ame "
      "(S)ize");
  wmove(stdscr, Y_PROMPT(ctx) + 1, 0);
  clrtoeol();
  PrintOptions(stdscr, Y_PROMPT(ctx) + 1, 0,
               "COMMANDS  o(W)ner   (O)rder: [ascending]");
  doupdate();

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
      wmove(stdscr, Y_PROMPT(ctx) + 1, 0);
      clrtoeol();
      PrintOptions(stdscr, Y_PROMPT(ctx) + 1, 0,
                   (order == SORT_ASC)
                       ? "COMMANDS  o(W)ner   (O)rder: [ascending] "
                       : "COMMANDS  o(W)ner   (O)rder: [descending]");
      doupdate();
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

void shell_quote(char *dest, const char *src) {
  *dest++ = '\'';
  while (*src) {
    if (*src == '\'') {
      *dest++ = '\'';
      *dest++ = '\\';
      *dest++ = '\'';
      *dest++ = '\'';
    } else {
      *dest++ = *src;
    }
    src++;
  }
  *dest++ = '\'';
  *dest = '\0';
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
  struct dirent *entry;
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

    DrawTaggedViewHeader(ctx, display_paths[current_idx], current_idx, path_count);
    RenderFilePreview(ctx, ctx->viewer.view, view_paths[current_idx], &line_offset,
                      0);
    RefreshWindow(ctx->viewer.view);
    doupdate();

    ch = (ctx->resize_request) ? -1 : WGetch(ctx, ctx->viewer.view);

#ifdef VI_KEYS
    ch = ViKey(ch);
#endif

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
  char quoted_path[PATH_LENGTH * 2 + 1];
  int i;
  int current_len;

  if (!ctx || !view_paths || path_count <= 0 || !s)
    return -1;

  viewer = (GetProfileValue)(ctx, "PAGER");
  if (!viewer || !*viewer)
    viewer = "less";

  command_line = (char *)xmalloc(COMMAND_LINE_LENGTH + 1);
  snprintf(command_line, COMMAND_LINE_LENGTH + 1, "%s", viewer);
  current_len = strlen(command_line);

  if (ctx->global_search_term[0] != '\0') {
    char search_arg[300];
    snprintf(search_arg, sizeof(search_arg), " -p \"%s\"", ctx->global_search_term);
    if ((int)(current_len + strlen(search_arg)) < COMMAND_LINE_LENGTH) {
      strcat(command_line, search_arg);
      current_len += strlen(search_arg);
    }
  }

  for (i = 0; i < path_count; i++) {
    shell_quote(quoted_path, view_paths[i]);
    if ((int)(current_len + strlen(quoted_path) + 1) >= COMMAND_LINE_LENGTH) {
      UI_Warning(ctx, "Too many tagged files. Truncated list.");
      break;
    }
    strcat(command_line, " ");
    strcat(command_line, quoted_path);
    current_len += strlen(quoted_path) + 1;
  }

  (void)SystemCall(ctx, command_line, s);
  free(command_line);
  return 0;
}

int UI_ViewTaggedFiles(ViewContext *ctx, DirEntry *dir_entry) {
  FileEntry *fe;
  char *temp_dir = NULL;
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

  (void)dir_entry;
  if (!ctx || !ctx->active || !ctx->active->vol || !ctx->active->file_entry_list)
    return 0;

  s = &ctx->active->vol->vol_stats;
  tagged_viewer = GetProfileValue(ctx, "TAGGEDVIEWER");
  if (tagged_viewer &&
      (!strcasecmp(tagged_viewer, "internal") ||
       !strcasecmp(tagged_viewer, "builtin") || !strcasecmp(tagged_viewer, "ytree") ||
       !strcmp(tagged_viewer, "1"))) {
    use_internal_view = TRUE;
  }

  if (s->login_mode == ARCHIVE_MODE) {
    strcpy(temp_dir_template, "/tmp/ytree_view_XXXXXX");
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
    if (s->login_mode == ARCHIVE_MODE && temp_dir)
      recursive_rmdir(temp_dir);
    return 0;
  }

  view_paths = (char **)xcalloc((size_t)tagged_count, sizeof(char *));
  display_paths = (char **)xcalloc((size_t)tagged_count, sizeof(char *));

  for (i = 0; i < (int)ctx->active->file_count; i++) {
    fe = ctx->active->file_entry_list[i].file;

    if (!(fe->tagged && fe->matching))
      continue;

    if (s->login_mode == DISK_MODE || s->login_mode == USER_MODE) {
      GetFileNamePath(fe, path);
      view_paths[path_count] = xstrdup(path);
      display_paths[path_count] = xstrdup(path);
      path_count++;
    } else if (s->login_mode == ARCHIVE_MODE) {
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
        char full_path[PATH_LENGTH + 1];
        char *dir_only;
        size_t root_len;

        GetPath(ctx->active->vol->vol_stats.tree, root_path);
        GetFileNamePath(fe, full_path);

        root_len = strlen(root_path);
        if (strncmp(full_path, root_path, root_len) == 0) {
          char *ptr = full_path + root_len;
          if (*ptr == FILE_SEPARATOR_CHAR) {
            ptr++;
            strcpy(relative_path, ptr);
          } else if (*ptr == '\0') {
            relative_path[0] = '\0';
          } else {
            strcpy(relative_path, full_path);
          }
        } else {
          strcpy(relative_path, full_path);
        }

        if (strlen(relative_path) == 0) {
          strcpy(relative_path, fe->name);
        }

        if (strlen(relative_path) > 0) {
          snprintf(t_dirname, sizeof(t_dirname), "%s/%s", temp_dir,
                   relative_path);
          dir_only = xstrdup(t_dirname);
          dirname(dir_only);
          recursive_mkdir(dir_only);
          free(dir_only);
          snprintf(t_filename, sizeof(t_filename), "%s/%s", temp_dir,
                   relative_path);
        } else {
          snprintf(t_filename, sizeof(t_filename), "%s/%s", temp_dir, fe->name);
        }

        strcpy(internal_path, relative_path);

        if (ExtractArchiveNode(s->login_path, internal_path, t_filename,
                               UI_ArchiveCallback, ctx) == 0) {
          view_paths[path_count] = xstrdup(t_filename);
          display_paths[path_count] = xstrdup(internal_path);
          path_count++;
        } else {
          UI_Warning(ctx, "Failed to extract*\"%s\"", internal_path);
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
  } else if (s->login_mode == ARCHIVE_MODE) {
    UI_Message(ctx, "No files extracted.");
  }

  for (i = 0; i < tagged_count; i++) {
    if (view_paths && view_paths[i])
      free(view_paths[i]);
    if (display_paths && display_paths[i])
      free(display_paths[i]);
  }
  free(view_paths);
  free(display_paths);

  if (s->login_mode == ARCHIVE_MODE && temp_dir) {
    recursive_rmdir(temp_dir);
  }

  return 0;
}
