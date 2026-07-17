/***************************************************************************
 *
 * src/ui/display_utils.c
 * Functions for formatting and displaying data in the terminal UI
 *
 ***************************************************************************/

#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <ctype.h>
#include <stdio.h>  /* Added for snprintf, etc. */
#include <stdlib.h> /* Added for free, exit */
#include <string.h>
#include <time.h>

/*****************************************************************************
 *                              GetAttributes                                *
 *****************************************************************************/
char *GetAttributes(unsigned short mode, char *buffer) {
  char *save_buffer = buffer;

  if (S_ISREG(mode))
    *buffer++ = '-';
  else if (S_ISDIR(mode))
    *buffer++ = 'd';
  else if (S_ISCHR(mode))
    *buffer++ = 'c';
  else if (S_ISBLK(mode))
    *buffer++ = 'b';
  else if (S_ISFIFO(mode))
    *buffer++ = 'p';
  else if (S_ISLNK(mode))
    *buffer++ = 'l';
  else if (S_ISSOCK(mode))
    *buffer++ = 's'; /* ??? */
  else
    *buffer++ = '?'; /* unknown */

  if (mode & S_IRUSR)
    *buffer++ = 'r';
  else
    *buffer++ = '-';

  if (mode & S_IWUSR)
    *buffer++ = 'w';
  else
    *buffer++ = '-';

  if (mode & S_IXUSR)
    *buffer++ = 'x';
  else
    *buffer++ = '-';

  if (mode & S_ISUID)
    *(buffer - 1) = 's';

  if (mode & S_IRGRP)
    *buffer++ = 'r';
  else
    *buffer++ = '-';

  if (mode & S_IWGRP)
    *buffer++ = 'w';
  else
    *buffer++ = '-';

  if (mode & S_IXGRP)
    *buffer++ = 'x';
  else
    *buffer++ = '-';

  if (mode & S_ISGID)
    *(buffer - 1) = 's';

  if (mode & S_IROTH)
    *buffer++ = 'r';
  else
    *buffer++ = '-';

  if (mode & S_IWOTH)
    *buffer++ = 'w';
  else
    *buffer++ = '-';

  if (mode & S_IXOTH)
    *buffer++ = 'x';
  else
    *buffer++ = '-';

  *buffer =
      '\0'; /* This ensures buffer[10] is null-terminated, safe for char[11] */

  return (save_buffer);
}

/*****************************************************************************
 *                                  CTime                                    *
 * Modernized to use ISO-like format: YYYY-MM-DD HH:MM (16 chars)            *
 *****************************************************************************/
char *CTime(time_t f_time, char *buffer) {
  const struct tm *tm_ptr;

  tm_ptr = localtime(&f_time);

  if (tm_ptr) {
    /* Format: 2025-11-19 14:30 (16 characters + null terminator) */
    /* The caller (BuildUserFileEntry) provides a buffer of size 20. */
    strftime(buffer, 17, "%Y-%m-%d %H:%M", tm_ptr);
  } else {
    /* Fallback for invalid time, 15 characters + null terminator */
    snprintf(buffer, 17, "    ?     ?   ");
  }

  return (buffer);
}

/*****************************************************************************
 *                              FormFilename                                 *
 * Safely formats a filename, truncating with "..." if it exceeds max_len.   *
 * Prioritizes showing the end of the path (like CutPathname) for clarity.  *
 *****************************************************************************/
char *FormFilename(char *dest, char *src, unsigned int max_len) {
  unsigned int l;
  char *src_copy = NULL;
  char *working_src = src;

  /* If dest and src overlap, we must copy src to avoid corruption during write
   */
  if (dest == src) {
    src_copy = xstrdup(src);
    working_src = src_copy;
  }

  l = strlen(working_src);

  if (l <= max_len) {
    /* Safe copy if pointers differ or if they are the same (snprintf handles
     * it) */
    snprintf(dest, max_len + 1, "%s", working_src);
  } else {
    /* Truncate path: "...<suffix>" */
    /* We need the last (max_len - 3) chars from src, plus "..." prefix */
    if (max_len < 4) {
      /* If max_len is too small for "...", just truncate */
      snprintf(dest, max_len + 1, "%.*s", max_len, "...");
    } else {
      const char *suffix_start = working_src + (l - (max_len - 3));
      snprintf(dest, max_len + 1, "...%s", suffix_start);
    }
  }

  if (src_copy)
    free(src_copy);
  return dest;
}

/*****************************************************************************
 *                              CutFilename                                  *
 * Truncates a filename by keeping the prefix and appending "..." if too long.*
 *****************************************************************************/
char *CutFilename(char *dest, const char *src, unsigned int max_len) {
  unsigned int l;

  l = StrVisualLength(src); /* Using visual length as per existing logic */

  if (l <= max_len) {
    snprintf(dest, max_len + 1, "%s", src);
  } else {
    /* Truncate string: keep first (max_len - 3) chars, append "..." */
    if (max_len < 4) {
      snprintf(dest, max_len + 1, "%.*s", max_len, "...");
    } else {
      snprintf(dest, max_len + 1, "%.*s...", max_len - 3, src);
    }
  }
  return dest;
}

/*****************************************************************************
 *                              CutPathname                                  *
 * Truncates a pathname by keeping the suffix and prepending "..." if too long.*
 *****************************************************************************/
char *CutPathname(char *dest, const char *src, unsigned int max_len) {
  unsigned int l;

  l = strlen(src);

  if (l <= max_len) {
    snprintf(dest, max_len + 1, "%s", src);
  } else {
    /* Format: "...<suffix>" */
    /* We need the last (max_len - 3) chars from src */
    if (max_len < 4) {
      snprintf(dest, max_len + 1, "%.*s", max_len, "...");
    } else {
      const char *suffix_start = src + (l - (max_len - 3));
      snprintf(dest, max_len + 1, "...%s", suffix_start);
    }
  }
  return (dest);
}

/*****************************************************************************
 *                              CutName                                      *
 * Truncates a name by keeping the prefix and appending "..." if too long.   *
 * (Identical to CutFilename in behavior, but uses strlen for length)        *
 *****************************************************************************/
char *CutName(char *dest, const char *src, unsigned int max_len) {
  unsigned int l;

  l = strlen(src);

  if (l <= max_len) {
    snprintf(dest, max_len + 1, "%s", src);
  } else {
    /* Truncate string: keep first (max_len - 3) chars, append "..." */
    if (max_len < 4) {
      snprintf(dest, max_len + 1, "%.*s", max_len, "...");
    } else {
      snprintf(dest, max_len + 1, "%.*s...", max_len - 3, src);
    }
  }
  return dest;
}

/*****************************************************************************
 *                           BuildUserFileEntry                              *
 *****************************************************************************/
int BuildUserFileEntry(FileEntry *fe_ptr, int filename_width,
                       int linkname_width, BOOL tagged, char *template,
                       int linelen, char *line) {
  char attributes[11];
  char modify_time[20];
  char change_time[20];
  char access_time[20];
  char format1[60]; /* Increased size for safety with snprintf */
  char format2[60]; /* Increased size for safety with snprintf */
  int written;
  char owner[OWNER_NAME_MAX + 1];
  char group[GROUP_NAME_MAX + 1];
  const char *owner_name_ptr;
  const char *group_name_ptr;
  const char *sym_link_name = NULL;
  char *sptr;
  char tag;

  /* Setup for safe string handling */
  size_t remaining = linelen;
  char *dptr = line;

  if (fe_ptr == NULL || line == NULL || linelen <= 0)
    return -1;

  if (S_ISLNK(fe_ptr->stat_struct.st_mode))
    sym_link_name = &fe_ptr->name[strlen(fe_ptr->name) + 1];
  else
    sym_link_name = "";

  tag = tagged ? TAGGED_SYMBOL : ' ';
  (void)GetAttributes(fe_ptr->stat_struct.st_mode, attributes);

  (void)CTime(fe_ptr->stat_struct.st_mtime, modify_time);
  (void)CTime(fe_ptr->stat_struct.st_ctime, change_time);
  (void)CTime(fe_ptr->stat_struct.st_atime, access_time);

  owner_name_ptr = GetPasswdName(fe_ptr->stat_struct.st_uid);
  group_name_ptr = GetGroupName(fe_ptr->stat_struct.st_gid);

  if (owner_name_ptr == NULL) {
    snprintf(owner, sizeof(owner), "%d", (int)fe_ptr->stat_struct.st_uid);
    owner_name_ptr = owner;
  }
  if (group_name_ptr == NULL) {
    snprintf(group, sizeof(group), "%d", (int)fe_ptr->stat_struct.st_gid);
    group_name_ptr = group;
  }

  /* Safely create format strings */
  snprintf(format1, sizeof(format1), "%%-%ds", filename_width);
  snprintf(format2, sizeof(format2), "%%-%ds", linkname_width);

  for (sptr = template; *sptr && remaining > 0;) { /* Added remaining check */

    if (*sptr == '%') {
      sptr++;
      if (!strncmp(sptr, TAGSYMBOL_VIEWNAME, 3)) {
        written = snprintf(dptr, remaining, "%c", tag);
      } else if (!strncmp(sptr, FILENAME_VIEWNAME, 3)) {
        written = snprintf(dptr, remaining, format1, fe_ptr->name);
      } else if (!strncmp(sptr, ATTRIBUTE_VIEWNAME, 3)) {
        written = snprintf(dptr, remaining, "%10s", attributes);
      } else if (!strncmp(sptr, LINKCOUNT_VIEWNAME, 3)) {
        written =
            snprintf(dptr, remaining, "%3d", (int)fe_ptr->stat_struct.st_nlink);
      } else if (!strncmp(sptr, FILESIZE_VIEWNAME, 3)) {
        written = snprintf(dptr, remaining, "%11lld",
                           (long long)fe_ptr->stat_struct.st_size);
      } else if (!strncmp(sptr, MODTIME_VIEWNAME, 3)) {
        written = snprintf(dptr, remaining, "%16s", modify_time);
      } else if (!strncmp(sptr, SYMLINK_VIEWNAME, 3)) {
        written = snprintf(dptr, remaining, format2, sym_link_name);
      } else if (!strncmp(sptr, UID_VIEWNAME, 3)) {
        written = snprintf(dptr, remaining, "%-8s", owner_name_ptr);
      } else if (!strncmp(sptr, GID_VIEWNAME, 3)) {
        written = snprintf(dptr, remaining, "%-8s", group_name_ptr);
      } else if (!strncmp(sptr, INODE_VIEWNAME, 3)) {
        written = snprintf(dptr, remaining, "%11lld",
                           (long long)fe_ptr->stat_struct.st_ino);
      } else if (!strncmp(sptr, ACCTIME_VIEWNAME, 3)) {
        written = snprintf(dptr, remaining, "%16s", access_time);
      } else if (!strncmp(sptr, SCTIME_VIEWNAME, 3)) {
        written = snprintf(dptr, remaining, "%16s", change_time);
      } else {
        written = -1; /* Indicate no match, will print '%' */
      }

      if (written < 0) { /* Error or no match */
        written = snprintf(dptr, remaining, "%%");
        if (written > 0) {
          if ((size_t)written >= remaining) {
            dptr += remaining - 1;
            remaining = 1;
          } else {
            dptr += written;
            remaining -= written;
          }
        }
        sptr++; /* Advance past the single '%' */
      } else {  /* Successfully wrote something */
        if ((size_t)written >= remaining) {
          dptr += remaining - 1;
          remaining = 1; /* Keep space for NULL */
        } else {
          dptr += written;
          remaining -= written;
        }
        /* Advance sptr past the 3-char viewname */
        if (*sptr)
          sptr++;
        if (*sptr)
          sptr++;
        if (*sptr)
          sptr++;
      }
    } else {               /* Not a '%' placeholder, copy character directly */
      if (remaining > 1) { /* Ensure space for char + null */
        *dptr++ = *sptr++;
        remaining--;
      } else {
        sptr++; /* Consume char but don't write if no space */
      }
    }
  }
  *dptr = '\0'; /* Ensure null termination */
  return (0);
}

int GetVisualUserFileEntryLength(int max_visual_filename_len,
                                 int max_visual_linkname_len, char *template) {
  int len, n;
  char *sptr;

  for (len = 0, sptr = template; *sptr;) {

    if (*sptr == '%') {
      sptr++;
      if (!strncmp(sptr, TAGSYMBOL_VIEWNAME, 3)) {
        n = 1;
      } else if (!strncmp(sptr, FILENAME_VIEWNAME, 3)) {
        n = max_visual_filename_len;
      } else if (!strncmp(sptr, ATTRIBUTE_VIEWNAME, 3)) {
        n = 10;
      } else if (!strncmp(sptr, LINKCOUNT_VIEWNAME, 3)) {
        n = 3;
      } else if (!strncmp(sptr, FILESIZE_VIEWNAME, 3)) {
        n = 11;
      } else if (!strncmp(sptr, MODTIME_VIEWNAME, 3)) {
        n = 16; /* Updated to 16 for YYYY-MM-DD HH:MM */
      } else if (!strncmp(sptr, SYMLINK_VIEWNAME, 3)) {
        n = max_visual_linkname_len;
      } else if (!strncmp(sptr, UID_VIEWNAME, 3)) {
        n = 8;
      } else if (!strncmp(sptr, GID_VIEWNAME, 3)) {
        n = 8;
      } else if (!strncmp(sptr, INODE_VIEWNAME, 3)) {
        n = 11;
      } else if (!strncmp(sptr, ACCTIME_VIEWNAME, 3)) {
        n = 16; /* Updated to 16 */
      } else if (!strncmp(sptr, SCTIME_VIEWNAME, 3)) {
        n = 16; /* Updated to 16 */
      } else {
        n = -1;
      }
      if (n == -1) {
        len++;
        sptr++;
      } else {
        len += n;
        if (*sptr)
          sptr++;
        if (*sptr)
          sptr++;
        if (*sptr)
          sptr++;
      }
    } else {
      sptr++;
      len++;
    }
  }
  return (len);
}

/*****************************************************************************
 *                                  GetMaxYX                                 *
 *****************************************************************************/
void GetMaxYX(WINDOW *win, int *height, int *width) {
  if (win == NULL) {
    /* Cannot use UI_Error here without context - this is a fatal error anyway */
    fprintf(stderr, "FATAL: GetMaxYX called with NULL window\n");
    exit(1);
  }

  getmaxyx(win, *height, *width);

  *height = MAXIMUM(*height, 1);
  *width = MAXIMUM(*width, 1);
}

/***************************************************************************
 *
 * Enhanced Curses Print Functions (from former print.c)
 *
 ***************************************************************************/

int MvAddStr(int y, int x, char *str) {
#ifdef WITH_UTF8
  mvaddstr(y, x, str);
#else
  for (; *str != '\0'; str++)
    mvaddch(y, x++, PRINT(*str));
#endif
  return 0;
}

int MvWAddStr(WINDOW *win, int y, int x, char *str) {
#ifdef WITH_UTF8
  mvwaddstr(win, y, x, str);
#else
  for (; *str != '\0'; str++)
    mvwaddch(win, y, x++, PRINT(*str));
#endif
  return 0;
}

int WAddStr(WINDOW *win, char *str) {
#ifdef WITH_UTF8
  waddstr(win, str);
#else
  for (; *str != '\0'; str++)
    waddch(win, PRINT(*str));
#endif
  return 0;
}

int AddStr(char *str) {
#ifdef WITH_UTF8
  addstr(str);
#else
  for (; *str != '\0'; str++)
    addch(PRINT(*str));
#endif
  return 0;
}

int WAttrAddStr(WINDOW *win, int attr, char *str) {
  int rc;

  wattrset(win, attr);
  rc = WAddStr(win, str);
  wattrset(win, 0);

  return (rc);
}

void PrintSpecialString(WINDOW *win, int y, int x, char *str, int color) {
  int ch;

  if (x < 0 || y < 0) {
    /* screen too small */
    return;
  }

  wmove(win, y, x);

  for (; *str; str++) {
    if ((!iscntrl(*str)) || (!isspace(*str)) || (*str == ' '))
      switch (*str) {
      case '1':
        ch = ACS_ULCORNER;
        break;
      case '2':
        ch = ACS_URCORNER;
        break;
      case '3':
        ch = ACS_LLCORNER;
        break;
      case '4':
        ch = ACS_LRCORNER;
        break;
      case '5':
        ch = ACS_TTEE;
        break;
      case '6':
        ch = ACS_LTEE;
        break;
      case '7':
        ch = ACS_RTEE;
        break;
      case '8':
        ch = ACS_BTEE;
        break;
      case '9':
        ch = ACS_LARROW;
        break;
      case '|':
        ch = ACS_VLINE;
        break;
      case '-':
        ch = ACS_HLINE;
        break;
      default:
        ch = PRINT(*str);
      }
    else
      ch = ACS_BLOCK;

#ifdef COLOR_SUPPORT
    wattrset(win, COLOR_PAIR(color));
#endif /* COLOR_SUPPORT */
    waddch(win, ch);
#ifdef COLOR_SUPPORT
    wattrset(win, 0);
#endif /* COLOR_SUPPORT */
  }
}

/*****************************************************************************
 *                                  PrintLine                                *
 * Generates a line of 'len' characters based on a 2-char pattern:           *
 * start_char, fill_char. The line will consist of start_char followed by    *
 * (len-1) fill_char characters.                                             *
 *****************************************************************************/
void PrintLine(WINDOW *win, int y, int x, const char *line, int len) {
  char *buffer_content;
  int i;

  if (len < 1)
    return;

  // Allocate for 'len' characters plus null terminator
  buffer_content = (char *)xmalloc(len + 1);

  // Set the starting character
  buffer_content[0] = line[0];

  // Fill the remaining characters with the fill character (line[1])
  for (i = 1; i < len; i++) {
    buffer_content[i] = line[1];
  }
  buffer_content[len] = '\0'; // Null terminate

  PrintOptions(win, y, x, buffer_content); // Pass the correctly sized buffer
  free(buffer_content);
}

void Print(WINDOW *win, int y, int x, char *str, int color) {
  int ch;

  if (x < 0 || y < 0) {
    /* screen too small */
    return;
  }
  wmove(win, y, x);
  for (; *str; str++) {
    ch = PRINT((int)*str);

#ifdef COLOR_SUPPORT
    wattrset(win, COLOR_PAIR(color));
#endif /* COLOR_SUPPORT */
    waddch(win, ch);
#ifdef COLOR_SUPPORT
    wattrset(win, 0);
#endif /* COLOR_SUPPORT */
  }
}

void PrintOptions(WINDOW *win, int y, int x, char *str) {
  int ch;
  int color, hi_color, lo_color;
  int max_x;

  if (x < 0 || y < 0) {
    /* screen too small */
    return;
  }
  max_x = getmaxx(win);
  if (max_x <= 0 || x >= max_x)
    return;

#ifdef COLOR_SUPPORT
  lo_color = UI_ROLE_STATIC_TEXT;
  hi_color = UI_ROLE_KEYBIND;
#else
  lo_color = A_NORMAL;
  hi_color = A_BOLD;
#endif

  color = lo_color;

  for (; *str && x < max_x; str++) {
    switch (*str) {
    case '(':
      color = hi_color;
      continue;
    case ')':
      color = lo_color;
      continue;

#ifdef COLOR_SUPPORT
    case ']':
      color = lo_color;
      continue;
    case '[':
      color = hi_color;
      continue;
#else
    case ']':
    case '[': /* ignore */
      continue;
#endif

    case '1':
      ch = ACS_ULCORNER;
      break;
    case '2':
      ch = ACS_URCORNER;
      break;
    case '3':
      ch = ACS_LLCORNER;
      break;
    case '4':
      ch = ACS_LRCORNER;
      break;
    case '5':
      ch = ACS_TTEE;
      break;
    case '6':
      ch = ACS_LTEE;
      break;
    case '7':
      ch = ACS_RTEE;
      break;
    case '8':
      ch = ACS_BTEE;
      break;
    case '9':
      ch = ACS_LARROW;
      break;
    case '|':
      ch = ACS_VLINE;
      break;
    case '-':
      ch = ACS_HLINE;
      break;
    default:
      ch = PRINT(*str);
    }

#ifdef COLOR_SUPPORT
    wattrset(win, COLOR_PAIR(color));
#else
    wattrset(win, color);
#endif
    mvwaddch(win, y, x++, ch);
    wattrset(win, 0);
  }
}

void PrintMenuOptions(WINDOW *win, int y, int x, char *str, int ncolor,
                      int hcolor) {
  int ch;
  int color, hi_color, lo_color;
  int max_x;

  if (x < 0 || y < 0) {
    /* screen too small */
    return;
  }
  max_x = getmaxx(win);
  if (max_x <= 0 || x >= max_x)
    return;

#ifdef COLOR_SUPPORT
  lo_color = ncolor;
  hi_color = hcolor;
#else
  lo_color = A_NORMAL;
  hi_color = A_REVERSE;
#endif

  color = lo_color;

  for (; *str && x < max_x; str++) {
    ch = (int)*str;

    switch (ch) {
    case '(':
      color = hi_color;
      continue;

    case ')':
      color = lo_color;
      continue;

#ifdef COLOR_SUPPORT
    case ']':
      color = lo_color;
      continue;
    case '[':
      color = hi_color;
      continue;
#else
    case ']':
    case '[': /* ignore */
      continue;
#endif
    default:
      ch = PRINT(ch);
      break;
    }
#ifdef COLOR_SUPPORT
    wattrset(win, COLOR_PAIR(color));
#else
    wattrset(win, color);
#endif
    mvwaddch(win, y, x++, ch);
    wattrset(win, 0);
  }
}

static BOOL CommandStripKeyUsesPlainText(const char *key) {
  static const char *plain_keys[] = {"Esc", "Enter", "Up",   "Down", "Home",
                                     "End", "PgUp",  "PgDn", "Shift"};
  size_t i;

  if (key == NULL || *key == '\0')
    return FALSE;
  if (key[0] == 'F' && isdigit((unsigned char)key[1]))
    return TRUE;

  for (i = 0; i < sizeof(plain_keys) / sizeof(plain_keys[0]); ++i) {
    if (strcmp(key, plain_keys[i]) == 0)
      return TRUE;
  }

  return FALSE;
}

static const char *CommandStripFindInlineLabelKey(const char *label,
                                                  const char *primary_key) {
  const char *p;
  unsigned char key;

  if (label == NULL || primary_key == NULL || primary_key[0] == '\0' ||
      primary_key[1] != '\0' || !isalpha((unsigned char)primary_key[0]))
    return NULL;

  key = (unsigned char)tolower((unsigned char)primary_key[0]);
  for (p = label; *p != '\0'; ++p) {
    if ((unsigned char)tolower((unsigned char)*p) == key)
      return p;
  }

  return NULL;
}

static BOOL CommandStripKeyUsesCompactLabel(
    const UICommandStripCommand *command) {
  return command != NULL && command->layout == UI_COMMAND_LAYOUT_KEY_PREFIX &&
         command->primary_key != NULL && command->primary_key[0] != '\0' &&
         command->primary_key[1] == '\0' && command->secondary_key == NULL &&
         !CommandStripKeyUsesPlainText(command->primary_key);
}

static void CommandStripAddLength(int *len, const char *text);

static void CommandStripAddVisibleKeyLength(int *len, const char *key) {
  if (len == NULL || key == NULL || *key == '\0')
    return;

  CommandStripAddLength(len, key);
}

static void CommandStripAddKeySequenceLength(int *len, const char *primary_key,
                                             const char *secondary_key,
                                             BOOL plain_text) {
  (void)plain_text;
  CommandStripAddVisibleKeyLength(len, primary_key);
  if (secondary_key != NULL) {
    CommandStripAddLength(len, "/");
    CommandStripAddVisibleKeyLength(len, secondary_key);
  }
}

static int CommandStripTextLength(const char *text) {
  if (text == NULL)
    return 0;
  return StrVisualLength((char *)text);
}

static void CommandStripAddLength(int *len, const char *text) {
  if (len != NULL)
    *len += CommandStripTextLength(text);
}

static void CommandStripAddMnemonicLabelLength(int *len, const char *label,
                                               const char *key) {
  const char *inline_key;

  if (len == NULL || label == NULL)
    return;

  inline_key = CommandStripFindInlineLabelKey(label, key);
  if (inline_key == NULL) {
    if (key != NULL && *key != '\0') {
      CommandStripAddVisibleKeyLength(len, key);
      if (*label != '\0')
        CommandStripAddLength(len, " ");
    }
    CommandStripAddLength(len, label);
    return;
  }

  while (label < inline_key) {
    char ch[2];

    ch[0] = *label++;
    ch[1] = '\0';
    CommandStripAddLength(len, ch);
  }
  CommandStripAddVisibleKeyLength(len, key);
  CommandStripAddLength(len, inline_key + 1);
}

static void CommandStripMeasureCommandFull(int *len,
                                           const UICommandStripCommand *command) {
  if (len == NULL || command == NULL)
    return;

  switch (command->layout) {
  case UI_COMMAND_LAYOUT_MNEMONIC:
    CommandStripAddMnemonicLabelLength(len, command->label, command->primary_key);
    break;
  case UI_COMMAND_LAYOUT_KEY_PREFIX:
  {
    if (CommandStripKeyUsesCompactLabel(command)) {
      if (CommandStripFindInlineLabelKey(command->label, command->primary_key) !=
          NULL) {
        CommandStripAddLength(len, command->label);
      } else {
        CommandStripAddLength(len, command->primary_key);
        CommandStripAddLength(len, " ");
        CommandStripAddLength(len, command->label);
      }
      break;
    }

    BOOL plain_text = CommandStripKeyUsesPlainText(command->primary_key) &&
                      (command->secondary_key == NULL ||
                       CommandStripKeyUsesPlainText(command->secondary_key));

    CommandStripAddKeySequenceLength(len, command->primary_key,
                                     command->secondary_key, plain_text);
    CommandStripAddLength(len, " ");
    CommandStripAddLength(len, command->label);
    break;
  }
  case UI_COMMAND_LAYOUT_ALT_MNEMONIC:
    CommandStripAddVisibleKeyLength(len, command->primary_key);
    CommandStripAddLength(len, "/");
    CommandStripAddMnemonicLabelLength(len, command->label, command->secondary_key);
    break;
  case UI_COMMAND_LAYOUT_LABEL_FIRST:
    CommandStripAddLength(len, command->label);
    CommandStripAddLength(len, " ");
    CommandStripAddKeySequenceLength(
        len, command->primary_key, command->secondary_key,
        CommandStripKeyUsesPlainText(command->primary_key) &&
            (command->secondary_key == NULL ||
             CommandStripKeyUsesPlainText(command->secondary_key)));
    break;
  }
}

static void CommandStripAppendText(char *buf, size_t buf_size, size_t *offset,
                                   const char *text) {
  size_t text_len;

  if (offset == NULL || text == NULL)
    return;

  text_len = strlen(text);
  if (buf != NULL && buf_size > 0 && *offset < buf_size - 1) {
    size_t copy_len = text_len;

    if (copy_len > buf_size - 1 - *offset)
      copy_len = buf_size - 1 - *offset;
    memcpy(buf + *offset, text, copy_len);
    *offset += copy_len;
    buf[*offset] = '\0';
  }
}

static void CommandStripAppendSpan(char *buf, size_t buf_size, size_t *offset,
                                   const char *start, size_t len) {
  if (offset == NULL || start == NULL || len == 0)
    return;

  if (buf != NULL && buf_size > 0 && *offset < buf_size - 1) {
    size_t copy_len = len;

    if (copy_len > buf_size - 1 - *offset)
      copy_len = buf_size - 1 - *offset;
    memcpy(buf + *offset, start, copy_len);
    *offset += copy_len;
    buf[*offset] = '\0';
  }
}

static void CommandStripAppendChar(char *buf, size_t buf_size, size_t *offset,
                                   char ch) {
  char text[2];

  text[0] = ch;
  text[1] = '\0';
  CommandStripAppendText(buf, buf_size, offset, text);
}

static void CommandStripAppendMnemonicLabel(char *buf, size_t buf_size,
                                            size_t *offset, const char *label,
                                            const char *key) {
  const char *inline_key;

  if (offset == NULL || label == NULL)
    return;

  inline_key = CommandStripFindInlineLabelKey(label, key);
  if (inline_key == NULL) {
    if (key != NULL && *key != '\0') {
      CommandStripAppendText(buf, buf_size, offset, key);
      if (*label != '\0')
        CommandStripAppendText(buf, buf_size, offset, " ");
    }
    CommandStripAppendText(buf, buf_size, offset, label);
    return;
  }

  CommandStripAppendSpan(buf, buf_size, offset, label,
                         (size_t)(inline_key - label));
  CommandStripAppendText(buf, buf_size, offset, key);
  CommandStripAppendText(buf, buf_size, offset, inline_key + 1);
}

int UI_FormatCommandStripEntryText(const UICommandStripCommand *command,
                                   char *buf, size_t buf_size) {
  size_t offset = 0;

  if (buf != NULL && buf_size > 0)
    buf[0] = '\0';
  if (command == NULL)
    return 0;

  switch (command->layout) {
  case UI_COMMAND_LAYOUT_MNEMONIC:
    CommandStripAppendMnemonicLabel(buf, buf_size, &offset, command->label,
                                    command->primary_key);
    break;
  case UI_COMMAND_LAYOUT_KEY_PREFIX:
  {
    if (CommandStripKeyUsesCompactLabel(command)) {
      const char *inline_key =
          CommandStripFindInlineLabelKey(command->label, command->primary_key);

      if (inline_key != NULL) {
        CommandStripAppendSpan(buf, buf_size, &offset, command->label,
                               (size_t)(inline_key - command->label));
        CommandStripAppendChar(buf, buf_size, &offset,
                               (char)toupper((unsigned char)*inline_key));
        CommandStripAppendText(buf, buf_size, &offset, inline_key + 1);
        break;
      }
    }

    CommandStripAppendText(buf, buf_size, &offset, command->primary_key);
    if (command->secondary_key != NULL) {
      CommandStripAppendText(buf, buf_size, &offset, "/");
      CommandStripAppendText(buf, buf_size, &offset, command->secondary_key);
    }
    CommandStripAppendText(buf, buf_size, &offset, " ");
    CommandStripAppendText(buf, buf_size, &offset, command->label);
    break;
  }
  case UI_COMMAND_LAYOUT_ALT_MNEMONIC:
    CommandStripAppendText(buf, buf_size, &offset, command->primary_key);
    CommandStripAppendText(buf, buf_size, &offset, "/");
    CommandStripAppendMnemonicLabel(buf, buf_size, &offset, command->label,
                                    command->secondary_key);
    break;
  case UI_COMMAND_LAYOUT_LABEL_FIRST:
    CommandStripAppendText(buf, buf_size, &offset, command->label);
    CommandStripAppendText(buf, buf_size, &offset, " ");
    CommandStripAppendText(buf, buf_size, &offset, command->primary_key);
    if (command->secondary_key != NULL) {
      CommandStripAppendText(buf, buf_size, &offset, "/");
      CommandStripAppendText(buf, buf_size, &offset, command->secondary_key);
    }
    break;
  }

  if (buf != NULL && buf_size > 0)
    buf[buf_size - 1] = '\0';
  return CommandStripTextLength(buf);
}

int UI_CommandStripVisualLength(const UICommandStripCommand *commands,
                                size_t command_count) {
  size_t i;
  int len = 0;

  if (commands == NULL)
    return 0;

  for (i = 0; i < command_count; ++i) {
    if (i > 0)
      CommandStripAddLength(&len, "  ");
    CommandStripMeasureCommandFull(&len, &commands[i]);
  }

  return len;
}

static void CommandStripRenderText(WINDOW *win, int y, int *x, int max_x,
                                   const char *text, int attr) {
  if (win == NULL || x == NULL || text == NULL)
    return;

  wattrset(win, attr);
  for (; *text && *x < max_x; ++text) {
    int raw = (unsigned char)*text;
    chtype ch = raw < 32 ? ACS_BLOCK : (chtype)raw;

    mvwaddch(win, y, (*x)++, ch);
  }
}

static void CommandStripRenderKeySequence(WINDOW *win, int y, int *x, int max_x,
                                          const char *primary_key,
                                          const char *secondary_key,
                                          int normal_attr, int key_attr,
                                          BOOL plain_text) {
  (void)plain_text;
  CommandStripRenderText(win, y, x, max_x, primary_key, key_attr);
  if (secondary_key != NULL) {
    CommandStripRenderText(win, y, x, max_x, "/", normal_attr);
    CommandStripRenderText(win, y, x, max_x, secondary_key, key_attr);
  }
}

static void CommandStripRenderMnemonicLabel(WINDOW *win, int y, int *x,
                                            int max_x, const char *label,
                                            const char *key, int normal_attr,
                                            int key_attr) {
  const char *inline_key;

  if (label == NULL)
    return;

  inline_key = CommandStripFindInlineLabelKey(label, key);
  if (inline_key == NULL) {
    if (key != NULL && *key != '\0') {
      CommandStripRenderKeySequence(win, y, x, max_x, key, NULL, normal_attr,
                                    key_attr, FALSE);
      if (*label != '\0')
        CommandStripRenderText(win, y, x, max_x, " ", normal_attr);
    }
    CommandStripRenderText(win, y, x, max_x, label, normal_attr);
    return;
  }

  while (label < inline_key && *x < max_x) {
    char ch[2];

    ch[0] = *label++;
    ch[1] = '\0';
    CommandStripRenderText(win, y, x, max_x, ch, normal_attr);
  }
  CommandStripRenderKeySequence(win, y, x, max_x, key, NULL, normal_attr,
                                key_attr, FALSE);
  CommandStripRenderText(win, y, x, max_x, inline_key + 1, normal_attr);
}

static void CommandStripRenderCommandFull(WINDOW *win, int y, int *x, int max_x,
                                          const UICommandStripCommand *command,
                                          int normal_attr, int key_attr) {
  if (command == NULL)
    return;

  switch (command->layout) {
  case UI_COMMAND_LAYOUT_MNEMONIC:
    CommandStripRenderMnemonicLabel(win, y, x, max_x, command->label,
                                    command->primary_key, normal_attr, key_attr);
    break;
  case UI_COMMAND_LAYOUT_KEY_PREFIX:
  {
    if (CommandStripKeyUsesCompactLabel(command)) {
      const char *inline_key =
          CommandStripFindInlineLabelKey(command->label, command->primary_key);

      if (inline_key != NULL) {
        const char *p;

        for (p = command->label; *p != '\0' && *x < max_x; ++p) {
          char ch[2];

          ch[0] = (p == inline_key) ? (char)toupper((unsigned char)*p) : *p;
          ch[1] = '\0';
          CommandStripRenderText(win, y, x, max_x, ch,
                                 p == inline_key ? key_attr : normal_attr);
        }
      } else {
        CommandStripRenderText(win, y, x, max_x, command->primary_key,
                               key_attr);
        CommandStripRenderText(win, y, x, max_x, " ", normal_attr);
        CommandStripRenderText(win, y, x, max_x, command->label, normal_attr);
      }
      break;
    }

    BOOL plain_text = CommandStripKeyUsesPlainText(command->primary_key) &&
                      (command->secondary_key == NULL ||
                       CommandStripKeyUsesPlainText(command->secondary_key));

    CommandStripRenderKeySequence(win, y, x, max_x, command->primary_key,
                                  command->secondary_key, normal_attr,
                                  key_attr, plain_text);
    CommandStripRenderText(win, y, x, max_x, " ", normal_attr);
    CommandStripRenderText(win, y, x, max_x, command->label, normal_attr);
    break;
  }
  case UI_COMMAND_LAYOUT_ALT_MNEMONIC:
    CommandStripRenderKeySequence(win, y, x, max_x, command->primary_key, NULL,
                                  normal_attr, key_attr, FALSE);
    CommandStripRenderText(win, y, x, max_x, "/", normal_attr);
    CommandStripRenderMnemonicLabel(win, y, x, max_x, command->label,
                                    command->secondary_key, normal_attr,
                                    key_attr);
    break;
  case UI_COMMAND_LAYOUT_LABEL_FIRST:
    CommandStripRenderText(win, y, x, max_x, command->label, normal_attr);
    CommandStripRenderText(win, y, x, max_x, " ", normal_attr);
    CommandStripRenderKeySequence(
        win, y, x, max_x, command->primary_key, command->secondary_key,
        normal_attr, key_attr,
        CommandStripKeyUsesPlainText(command->primary_key) &&
            (command->secondary_key == NULL ||
             CommandStripKeyUsesPlainText(command->secondary_key)));
    break;
  }
}

void UI_RenderCommandStrip(WINDOW *win, int y, int x,
                           const UICommandStripCommand *commands,
                           size_t command_count, int ncolor, int hcolor) {
  size_t i;
  int max_x;
  int normal_attr;
  int key_attr;

  if (win == NULL || commands == NULL || x < 0 || y < 0)
    return;

  max_x = getmaxx(win);
  if (max_x <= 0 || x >= max_x)
    return;

#ifdef COLOR_SUPPORT
  normal_attr = COLOR_PAIR(ncolor);
  key_attr = UIKeybindAttrForBase(hcolor, ncolor);
#else
  normal_attr = A_NORMAL;
  key_attr = A_BOLD;
#endif

  for (i = 0; i < command_count && x < max_x; ++i) {
    if (i > 0)
      CommandStripRenderText(win, y, &x, max_x, "  ", normal_attr);
    CommandStripRenderCommandFull(win, y, &x, max_x, &commands[i], normal_attr,
                                  key_attr);
  }

  wattrset(win, 0);
}
