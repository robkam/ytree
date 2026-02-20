/***************************************************************************
 *
 * src/ui/render_file.c
 * Rendering logic for the file window (List View)
 *
 ***************************************************************************/

#include "ytree.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static char GetTypeOfFile(struct stat fst);
static int GetVisualFileEntryLength(YtreePanel *p);

void SetFileRenderingMetrics(YtreePanel *p, unsigned max_filename,
                             unsigned max_linkname, unsigned max_userview) {
  if (!p)
    return;
  p->max_visual_filename_len = max_filename;
  p->max_visual_linkname_len = max_linkname;
  /* userview len is usually calculated inside GetVisualFileEntryLength,
     but if passed explicitly, update it. */
  if (max_userview > 0)
    p->max_visual_userview_len = max_userview;
}

void SetRenderSortOrder(YtreePanel *p, BOOL reverse) {
  if (!p)
    return;
  p->reverse_sort = reverse;
}

int GetPanelFileMode(YtreePanel *p) {
  if (!p)
    return MODE_1;
  return p->file_mode;
}

int GetPanelMaxColumn(YtreePanel *p) {
  if (!p)
    return 1;
  return p->max_column;
}

void SetPanelFileMode(YtreePanel *p, int new_file_mode) {
  int width;

  if (!p)
    return;

  p->file_mode = new_file_mode;

  /* Use the existing window if available, otherwise calculate from layout or
   * defer */
  if (p->pan_file_window) {
    width = getmaxx(p->pan_file_window);
  } else {
    /* Fallback if window not created yet (e.g. init), use layout hint or
     * default */
    if (p == LeftPanel)
      width = layout.dir_win_width; /* approximation */
    else
      width = COLS - layout.dir_win_width;
    if (width < 10)
      width = 80; /* Safe default */
  }

  p->max_column = width / (GetVisualFileEntryLength(p) + 1);

  if (p->max_column == 0)
    p->max_column = 1;
}

void RotatePanelFileMode(YtreePanel *p) {
  if (!p)
    return;

  switch (p->file_mode) {
  case MODE_1:
    SetPanelFileMode(p, MODE_3);
    break;
  case MODE_2:
    SetPanelFileMode(p, MODE_5);
    break;
  case MODE_3:
    SetPanelFileMode(p, MODE_4);
    break;
  case MODE_4:
    SetPanelFileMode(p, MODE_2);
    break;
  case MODE_5:
    SetPanelFileMode(p, MODE_1);
    break;
  }
  if ((mode != DISK_MODE && mode != USER_MODE) && p->file_mode == MODE_4) {
    RotatePanelFileMode(p);
  } else if (p->file_mode == MODE_5 && !strcmp(USERVIEW, "")) {
    RotatePanelFileMode(p);
  }
}

static int GetVisualFileEntryLength(YtreePanel *p) {
  if (GlobalView->fixed_col_width > 0)
    return GlobalView->fixed_col_width;

  int len = 0;

  switch (p->file_mode) {
  case MODE_1:
    len = (p->max_visual_linkname_len) ? p->max_visual_linkname_len + 4
                                       : 0; /* linkname + " -> " */
    len += p->max_visual_filename_len +
           42; /* filename + format (increased by 4 for 16-char date) */
#ifdef HAS_LONGLONG
    len += 4; /* %11lld instead of %7d */
#endif
    break;

  case MODE_2:
    len = (p->max_visual_linkname_len) ? p->max_visual_linkname_len + 4
                                       : 0; /* linkname + " -> " */
    len += p->max_visual_filename_len + 40; /* filename + format */
#ifdef HAS_LONGLONG
    len += 4; /* %11lld instead of %7d */
#endif
    break;

  case MODE_3:
    len = p->max_visual_filename_len + 2; /* filename + format */
    break;

  case MODE_4:
    len = (p->max_visual_linkname_len) ? p->max_visual_linkname_len + 4
                                       : 0; /* linkname + " -> " */
    len += p->max_visual_filename_len +
           47; /* filename + format (increased by 8 for two 16-char dates) */
    break;

  case MODE_5:
    len = GetVisualUserFileEntryLength(p->max_visual_filename_len,
                                       p->max_visual_linkname_len, USERVIEW);
    p->max_visual_userview_len = len;
    break;
  }

  return len;
}

static char GetTypeOfFile(struct stat fst) {
  if (S_ISLNK(fst.st_mode))
    return '@';
  else if (S_ISSOCK(fst.st_mode))
    return '=';
  else if (S_ISCHR(fst.st_mode))
    return '-';
  else if (S_ISBLK(fst.st_mode))
    return '+';
  else if (S_ISFIFO(fst.st_mode))
    return '|';
  else if (S_ISREG(fst.st_mode))
    return ' ';
  else
    return '?';
}

void PrintFileEntry(YtreePanel *panel, int entry_no, int y, int x,
                    unsigned char hilight, int start_x, WINDOW *win) {
  char attributes[11];
  char modify_time[20];
  char change_time[20];
  char access_time[20];
  char format[60];
  char justify;
  char *line_ptr;
  int n, pos_x;
  FileEntry *fe_ptr;
  static char *line_buffer = NULL;
  static int old_cols = -1;
  static size_t line_buffer_size = 0;
  char owner[OWNER_NAME_MAX + 1];
  char group[GROUP_NAME_MAX + 1];
  char *owner_name_ptr;
  char *group_name_ptr;
  int ef_window_width;
  char *sym_link_name = NULL;
  char type_of_file = ' ';
  int filename_width = 0;
  int linkname_width = 0;
  int base_color_pair;
  int width;

  if (!panel || !panel->file_entry_list)
    return;

  width = getmaxx(win);

  fe_ptr = panel->file_entry_list[entry_no].file;

  if (GlobalView->fixed_col_width > 0) {
    pos_x = x * (GlobalView->fixed_col_width + 1);
    wmove(win, y, pos_x);

    /* Prepare Display Name */
    char display_name[PATH_LENGTH + 1];
    /* Reserve 2 chars for Tag and Type */
    CutFilename(display_name, fe_ptr->name, GlobalView->fixed_col_width - 2);

    /* Set Attributes */
    int color = GetFileTypeColor(fe_ptr);
    wattron(win, COLOR_PAIR(color));
    if (fe_ptr->tagged)
      wattron(win, A_BOLD);
    if (hilight)
      wattron(win, A_REVERSE);

    /* Draw */
    wprintw(win, "%c%c%s", (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
            GetTypeOfFile(fe_ptr->stat_struct), display_name);

    /* Pad remaining width */
    int printed_len = 2 + StrVisualLength(display_name);
    int k;
    for (k = printed_len; k < GlobalView->fixed_col_width; k++) {
      waddch(win, ' ');
    }

    /* Cleanup */
    if (hilight)
      wattroff(win, A_REVERSE);
    if (fe_ptr->tagged)
      wattroff(win, A_BOLD);
    wattroff(win, COLOR_PAIR(color));
    return;
  }

  ef_window_width = width - 2; /* Effective Window Width */

  (panel->reverse_sort) ? (justify = '+') : (justify = '-');

  if (old_cols != COLS) {
    old_cols = COLS;
    if (line_buffer)
      free(line_buffer);

    line_buffer_size = COLS + PATH_LENGTH;
    line_buffer = (char *)xmalloc(line_buffer_size);
  }

  if (fe_ptr && S_ISLNK(fe_ptr->stat_struct.st_mode))
    sym_link_name = &fe_ptr->name[strlen(fe_ptr->name) + 1];
  else
    sym_link_name = "";

#ifdef WITH_UTF8
#if defined(__GNUC__) && __GNUC__ >= 7
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overread"
#endif
  filename_width = panel->max_visual_filename_len +
                   (strlen(fe_ptr->name) - StrVisualLength(fe_ptr->name));
#if defined(__GNUC__) && __GNUC__ >= 7
#pragma GCC diagnostic pop
#endif
  if (fe_ptr && S_ISLNK(fe_ptr->stat_struct.st_mode))
    linkname_width = panel->max_visual_linkname_len +
                     (strlen(sym_link_name) - StrVisualLength(sym_link_name));
#else
  filename_width = panel->max_visual_filename_len;
  linkname_width = panel->max_visual_linkname_len;
#endif

  type_of_file = GetTypeOfFile(fe_ptr->stat_struct);

  /* Calculate starting column position (pos_x) based on column index `x` */
  switch (panel->file_mode) {
  case MODE_1:
    if (panel->max_visual_linkname_len)
      pos_x = x * (panel->max_visual_filename_len +
                   panel->max_visual_linkname_len + 51); /* +47 + 4 = 51 */
    else
      pos_x = x * (panel->max_visual_filename_len + 47); /* +43 + 4 = 47 */
    break;
  case MODE_2:
    if (panel->max_visual_linkname_len)
      pos_x = x * (panel->max_visual_filename_len +
                   panel->max_visual_linkname_len + 43);
    else
      pos_x = x * (panel->max_visual_filename_len + 39);
    break;
  case MODE_3:
    pos_x = x * (panel->max_visual_filename_len + 3);
    break;
  case MODE_4:
    if (panel->max_visual_linkname_len)
      pos_x = x * (panel->max_visual_filename_len +
                   panel->max_visual_linkname_len + 52); /* +44 + 8 = 52 */
    else
      pos_x = x * (panel->max_visual_filename_len + 48); /* +40 + 8 = 48 */
    break;
  case MODE_5:
    pos_x = x * (panel->max_visual_userview_len + 1);
    break;
  default:
    pos_x = x;
    break;
  }

  wmove(win, y, pos_x);
  base_color_pair = GetFileTypeColor(fe_ptr);

  if (highlight_full_line) {
    /* --- RENDER METHOD 1: FULL LINE HIGHLIGHT --- */
    wattron(win, COLOR_PAIR(base_color_pair));
    if (fe_ptr && fe_ptr->tagged)
      wattron(win, A_BOLD);
    if (hilight)
      wattron(win, A_REVERSE);

    /* Build the full line string */
    switch (panel->file_mode) {
    case MODE_1:
      (void)GetAttributes(fe_ptr->stat_struct.st_mode, attributes);
      (void)CTime(fe_ptr->stat_struct.st_mtime, modify_time);
      if (S_ISLNK(fe_ptr->stat_struct.st_mode)) {
#ifdef HAS_LONGLONG
        /* Updated %12s to %16s */
        (void)snprintf(format, sizeof(format),
                       "%%c%%c%%-%ds %%10s %%3d %%11lld %%16s -> %%-%ds",
                       filename_width, linkname_width);
        (void)snprintf(line_buffer, line_buffer_size, format,
                       (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ', type_of_file,
                       fe_ptr->name, attributes, fe_ptr->stat_struct.st_nlink,
                       (LONGLONG)fe_ptr->stat_struct.st_size, modify_time,
                       sym_link_name);
#else
        (void)snprintf(format, sizeof(format),
                       "%%c%%c%%-%ds %%10s %%3d %%7d %%16s -> %%-%ds",
                       filename_width, linkname_width);
        (void)snprintf(line_buffer, line_buffer_size, format,
                       (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ', type_of_file,
                       fe_ptr->name, attributes, fe_ptr->stat_struct.st_nlink,
                       fe_ptr->stat_struct.st_size, modify_time, sym_link_name);
#endif
      } else {
#ifdef HAS_LONGLONG
        (void)snprintf(format, sizeof(format),
                       "%%c%%c%%%c%ds %%10s %%3d %%11lld %%16s", justify,
                       filename_width);
        (void)snprintf(line_buffer, line_buffer_size, format,
                       (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ', type_of_file,
                       fe_ptr->name, attributes, fe_ptr->stat_struct.st_nlink,
                       (LONGLONG)fe_ptr->stat_struct.st_size, modify_time);
#else
        (void)snprintf(format, sizeof(format),
                       "%%c%%c%%%c%ds %%10s %%3d %%7d %%16s", justify,
                       filename_width);
        (void)snprintf(line_buffer, line_buffer_size, format,
                       (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ', type_of_file,
                       fe_ptr->name, attributes, fe_ptr->stat_struct.st_nlink,
                       fe_ptr->stat_struct.st_size, modify_time);
#endif
      }
      break;
    case MODE_2:
      owner_name_ptr = GetDisplayPasswdName(fe_ptr->stat_struct.st_uid);
      group_name_ptr = GetDisplayGroupName(fe_ptr->stat_struct.st_gid);
      if (!owner_name_ptr) {
        snprintf(owner, sizeof(owner), "%d", (int)fe_ptr->stat_struct.st_uid);
        owner_name_ptr = owner;
      }
      if (!group_name_ptr) {
        snprintf(group, sizeof(group), "%d", (int)fe_ptr->stat_struct.st_gid);
        group_name_ptr = group;
      }
      if (S_ISLNK(fe_ptr->stat_struct.st_mode)) {
#ifdef HAS_LONGLONG
        (void)snprintf(format, sizeof(format),
                       "%%c%%c%%%c%ds %%10lld %%-12s %%-12s -> %%-%ds", justify,
                       filename_width, linkname_width);
        (void)snprintf(line_buffer, line_buffer_size, format,
                       (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ', type_of_file,
                       fe_ptr->name, (LONGLONG)fe_ptr->stat_struct.st_ino,
                       owner_name_ptr, group_name_ptr, sym_link_name);
#else
        (void)snprintf(format, sizeof(format),
                       "%%c%%c%%%c%ds  %%8u  %%-12s %%-12s -> %%-%ds", justify,
                       filename_width, linkname_width);
        (void)snprintf(line_buffer, line_buffer_size, format,
                       (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ', type_of_file,
                       fe_ptr->name, (unsigned int)fe_ptr->stat_struct.st_ino,
                       owner_name_ptr, group_name_ptr, sym_link_name);
#endif
      } else {
#ifdef HAS_LONGLONG
        (void)snprintf(format, sizeof(format),
                       "%%c%%c%%%c%ds %%10lld %%-12s %%-12s", justify,
                       filename_width);
        (void)snprintf(line_buffer, line_buffer_size, format,
                       (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ', type_of_file,
                       fe_ptr->name, (LONGLONG)fe_ptr->stat_struct.st_ino,
                       owner_name_ptr, group_name_ptr);
#else
        (void)snprintf(format, sizeof(format),
                       "%%c%%c%%%c%ds  %%8u  %%-12s %%-12s", justify,
                       filename_width);
        (void)snprintf(line_buffer, line_buffer_size, format,
                       (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ', type_of_file,
                       fe_ptr->name, (unsigned int)fe_ptr->stat_struct.st_ino,
                       owner_name_ptr, group_name_ptr);
#endif
      }
      break;
    case MODE_3:
      (void)snprintf(format, sizeof(format), "%%c%%c%%%c%ds", justify,
                     filename_width);
      (void)snprintf(line_buffer, line_buffer_size, format,
                     (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ', type_of_file,
                     fe_ptr->name);
      break;
    case MODE_4:
      (void)CTime(fe_ptr->stat_struct.st_ctime, change_time);
      (void)CTime(fe_ptr->stat_struct.st_atime, access_time);
      if (S_ISLNK(fe_ptr->stat_struct.st_mode)) {
        /* Updated %12s to %16s */
        (void)snprintf(format, sizeof(format),
                       "%%c%%c%%%c%ds Chg: %%16s  Acc: %%16s -> %%-%ds",
                       justify, filename_width, linkname_width);
        (void)snprintf(line_buffer, line_buffer_size, format,
                       (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ', type_of_file,
                       fe_ptr->name, change_time, access_time, sym_link_name);
      } else {
        (void)snprintf(format, sizeof(format),
                       "%%c%%c%%%c%ds Chg: %%16s  Acc: %%16s", justify,
                       filename_width);
        (void)snprintf(line_buffer, line_buffer_size, format,
                       (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ', type_of_file,
                       fe_ptr->name, change_time, access_time);
      }
      break;
    case MODE_5:
      BuildUserFileEntry(fe_ptr, filename_width, linkname_width, USERVIEW, 200,
                         line_buffer);
      break;
    }

    /* Horizontal scrolling logic */
    n = StrVisualLength(line_buffer);
    if (n <= ef_window_width) {
      line_ptr = line_buffer;
    } else {
      int line_end_pos;
      if (n > (start_x + ef_window_width))
        line_ptr =
            &line_buffer[VisualPositionToBytePosition(line_buffer, start_x)];
      else
        line_ptr = &line_buffer[VisualPositionToBytePosition(
            line_buffer, n - ef_window_width)];

      line_end_pos = VisualPositionToBytePosition(line_ptr, ef_window_width);
      line_ptr[line_end_pos] = '\0';
    }
    waddstr(win, line_ptr);

    if (hilight)
      wattroff(win, A_REVERSE);
    if (fe_ptr && fe_ptr->tagged)
      wattroff(win, A_BOLD);

  } else {
    /* --- RENDER METHOD 2: NAME-ONLY HIGHLIGHT --- */
    if (start_x > 0)
      start_x = 0; /* No horizontal scrolling in this mode. */

    wattron(win, COLOR_PAIR(base_color_pair));
    if (fe_ptr && fe_ptr->tagged)
      wattron(win, A_BOLD);

    /* Print tag and type */
    wprintw(win, "%c%c", (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ', type_of_file);

    /* Calculate available width for name and truncate if necessary */
    int overhead = 0;
    if (width >= 80) {
      switch (panel->file_mode) {
      case MODE_1:
        overhead = 44;
        break;
      case MODE_2:
        overhead = 40;
        break;
      case MODE_4:
        overhead = 48;
        break;
      default:
        overhead = 0;
        break;
      }
    }
    if (sym_link_name && *sym_link_name)
      overhead += 4 + linkname_width;

    int max_w = width - pos_x - 3 - overhead;
    /* Prioritize filename: never truncate below 16 chars unless window is tiny
     */
    if (max_w < 16)
      max_w = 16;
    if (max_w > width - pos_x - 3)
      max_w = width - pos_x - 3;

    char display_name[PATH_LENGTH + 1];
    if ((int)strlen(fe_ptr->name) > max_w) {
      CutFilename(display_name, fe_ptr->name, max_w);
    } else {
      strcpy(display_name, fe_ptr->name);
    }

    /* Highlight only the name */
    if (hilight)
      wattron(win, A_REVERSE);
    wprintw(win, "%s", display_name);
    if (hilight)
      wattroff(win, A_REVERSE);

    /* Print attributes for modes other than MODE_3 */
    if (panel->file_mode != MODE_3) {
      int current_x;
      int dummy_y;
      getyx(win, dummy_y, current_x);
      (void)dummy_y;

      /* Adjusted target_x calculation to stay within bounds */
      int target_x = MINIMUM(pos_x + 2 + filename_width, width - overhead);

      /* Fill space between name and attributes */
      for (int i = current_x; i < target_x; i++)
        waddch(win, ' ');

      switch (panel->file_mode) {
      case MODE_1:
        (void)GetAttributes(fe_ptr->stat_struct.st_mode, attributes);
        (void)CTime(fe_ptr->stat_struct.st_mtime, modify_time);
#ifdef HAS_LONGLONG
        /* Updated %12s to %16s */
        wprintw(win, " %10s %3d %11lld %16s", attributes,
                (int)fe_ptr->stat_struct.st_nlink,
                (LONGLONG)fe_ptr->stat_struct.st_size, modify_time);
#else
        wprintw(win, " %10s %3d %7d %16s", attributes,
                (int)fe_ptr->stat_struct.st_nlink,
                (int)fe_ptr->stat_struct.st_size, modify_time);
#endif
        if (sym_link_name && *sym_link_name)
          wprintw(win, " -> %s", sym_link_name);
        break;
      case MODE_2:
        owner_name_ptr = GetDisplayPasswdName(fe_ptr->stat_struct.st_uid);
        group_name_ptr = GetDisplayGroupName(fe_ptr->stat_struct.st_gid);
        if (!owner_name_ptr) {
          snprintf(owner, sizeof(owner), "%d", (int)fe_ptr->stat_struct.st_uid);
          owner_name_ptr = owner;
        }
        if (!group_name_ptr) {
          snprintf(group, sizeof(group), "%d", (int)fe_ptr->stat_struct.st_gid);
          group_name_ptr = group;
        }
#ifdef HAS_LONGLONG
        wprintw(win, " %10lld %-12s %-12s",
                (LONGLONG)fe_ptr->stat_struct.st_ino, owner_name_ptr,
                group_name_ptr);
#else
        wprintw(win, "  %8u  %-12s %-12s",
                (unsigned int)fe_ptr->stat_struct.st_ino, owner_name_ptr,
                group_name_ptr);
#endif
        if (sym_link_name && *sym_link_name)
          wprintw(win, " -> %s", sym_link_name);
        break;
      case MODE_4:
        (void)CTime(fe_ptr->stat_struct.st_ctime, change_time);
        (void)CTime(fe_ptr->stat_struct.st_atime, access_time);
        /* Updated %12s to %16s */
        wprintw(win, " Chg: %16s  Acc: %16s", change_time, access_time);
        if (sym_link_name && *sym_link_name)
          wprintw(win, " -> %s", sym_link_name);
        break;
      case MODE_5:
        break;
      }
    }

    if (fe_ptr && fe_ptr->tagged)
      wattroff(win, A_BOLD);
  }
  wattroff(win, COLOR_PAIR(base_color_pair));
}

void DisplayFiles(YtreePanel *panel, DirEntry *de_ptr, int start_file_no,
                  int hilight_no, int start_x, WINDOW *win) {
  int x, y, p_x, p_y, j;
  int height;

  if (!panel || !panel->file_entry_list)
    return;

  height = getmaxy(win);

#ifdef COLOR_SUPPORT
  WbkgdSet(win, COLOR_PAIR(CPAIR_WINFILE));
#endif
  werase(win);

  if (panel->file_count == 0) {
    mvwaddstr(win, 0, 3,
              (de_ptr->access_denied) ? "Permission Denied!" : "No Files!");
  }

  j = start_file_no;
  p_x = -1;
  p_y = 0;
  for (x = 0; x < panel->max_column; x++) {
    for (y = 0; y < height; y++) {
      if ((unsigned)j < panel->file_count) {
        if (j == hilight_no) {
          p_x = x;
          p_y = y;
        } else {
          PrintFileEntry(panel, j, y, x, FALSE, start_x, win);
        }
      }
      j++;
    }
  }

  if (p_x >= 0)
    PrintFileEntry(panel, hilight_no, p_y, p_x, TRUE, start_x, win);

  wnoutrefresh(win);
}
