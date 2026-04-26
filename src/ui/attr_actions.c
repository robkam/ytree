/***************************************************************************
 *
 * src/ui/attr_actions.c
 * Mode/date/ownership interaction helpers extracted from interactions.c.
 *
 ***************************************************************************/

#include "ytree_cmd.h"
#include "ytree_fs.h"
#include "ytree_ui.h"
#include <ctype.h>
#include <grp.h>
#include <pwd.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <utime.h>

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
