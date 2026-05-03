/***************************************************************************
 *
 * src/ui/render_file.c
 * Rendering logic for the file window (List View)
 *
 ***************************************************************************/

#include "ytree_cmd.h"
#include "ytree_ui.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static char GetTypeOfFile(struct stat fst);
static int GetVisualFileEntryLength(ViewContext *ctx, YtreePanel *p);

static void AddClippedAtCursor(WINDOW *win, const char *text, int width) {
  int y, x, remaining;

  if (!win || !text || width <= 0)
    return;

  getyx(win, y, x);
  (void)y;
  remaining = width - x;
  if (remaining <= 0)
    return;

  waddnstr(win, text, remaining);
}

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

int GetPanelFileMode(const YtreePanel *p) {
  if (!p)
    return MODE_1;
  return p->file_mode;
}

int GetPanelMaxColumn(const YtreePanel *p) {
  if (!p)
    return 1;
  return p->max_column;
}

void SetPanelFileMode(ViewContext *ctx, YtreePanel *p, int new_file_mode) {
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
    if (p == ctx->left)
      width = ctx->layout.dir_win_width; /* approximation */
    else
      width = COLS - ctx->layout.dir_win_width;
    if (width < 10)
      width = 80; /* Safe default */
  }

  p->max_column = width / (GetVisualFileEntryLength(ctx, p) + 1);

  if (p->max_column == 0)
    p->max_column = 1;
}

void RotatePanelFileMode(ViewContext *ctx, YtreePanel *p) {
  if (!p)
    return;

  switch (p->file_mode) {
  case MODE_1:
    SetPanelFileMode(ctx, p, MODE_3);
    break;
  case MODE_2:
    SetPanelFileMode(ctx, p, MODE_5);
    break;
  case MODE_3:
    SetPanelFileMode(ctx, p, MODE_4);
    break;
  case MODE_4:
    SetPanelFileMode(ctx, p, MODE_2);
    break;
  case MODE_5:
    SetPanelFileMode(ctx, p, MODE_1);
    break;
  }
  if ((ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) &&
      p->file_mode == MODE_4) {
    RotatePanelFileMode(ctx, p);
  } else if (p->file_mode == MODE_5 &&
             !strcmp((GetProfileValue)(ctx, "USERVIEW"), "")) {
    RotatePanelFileMode(ctx, p);
  }
}

static int GetVisualFileEntryLength(ViewContext *ctx, YtreePanel *p) {
  int filename_len = p->max_visual_filename_len;
  if (filename_len == 0 &&
      !strcmp((GetProfileValue)(ctx, "USERVIEW"), ""))
    filename_len = 14; /* Sensible default for small windows */

  if (ctx->fixed_col_width > 0)
    return ctx->fixed_col_width;

  int len = 0;

  switch (p->file_mode) {
  case MODE_1:
    len = (p->max_visual_linkname_len) ? p->max_visual_linkname_len + 4
                                       : 0; /* linkname + " -> " */
    len +=
        filename_len + 46; /* filename + format (increased for 11-digit size) */
    break;

  case MODE_2:
    len = filename_len + 44; /* filename + format (11-digit size) */
    break;

  case MODE_3:
    len = filename_len + 2; /* filename + format */
    break;

  case MODE_4:
    len = (p->max_visual_linkname_len) ? p->max_visual_linkname_len + 4
                                       : 0; /* linkname + " -> " */
    len += filename_len +
           47; /* filename + format (increased by 8 for two 16-char dates) */
    break;

  case MODE_5:
    len = GetVisualUserFileEntryLength(filename_len, p->max_visual_linkname_len,
                                       (GetProfileValue)(ctx, "USERVIEW"));
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

void PrintFileEntry(ViewContext *ctx, YtreePanel *panel, int entry_no, int y,
                    int x, unsigned char hilight, int start_x, WINDOW *win) {
  char attributes[11];

  if (!ctx || !panel || !panel->vol || !win)
    return;
  char modify_time[20];
  char change_time[20];
  char access_time[20];
  char format[60];
  char justify;
  char *line_ptr;
  int n, pos_x = 0;
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
  char type_of_file;
  int filename_width = 0;
  int linkname_width = 0;
  int base_color_pair;
  int width;
  BOOL is_tagged;
  int highlight_attr;

  if (!panel->file_entry_list)
    return;

  width = getmaxx(win);

  fe_ptr = panel->file_entry_list[entry_no].file;
  if (fe_ptr == NULL)
    return;
  is_tagged = PanelTags_FileIsTagged(panel, fe_ptr);
  highlight_attr = (ctx->is_split_screen && panel != ctx->active)
                       ? (A_BOLD | A_UNDERLINE)
                       : A_REVERSE;

  if (ctx->fixed_col_width > 0) {
    pos_x = x * (ctx->fixed_col_width + 1);
    wmove(win, y, pos_x);

    /* Prepare Display Name */
    char display_name[PATH_LENGTH + 1];
    /* Reserve 2 chars for Tag and Type */
    CutFilename(display_name, fe_ptr->name, ctx->fixed_col_width - 2);

    /* Set Attributes */
    int color = GetFileTypeColor(ctx, fe_ptr);
    wattron(win, COLOR_PAIR(color));
    if (is_tagged)
      wattron(win, A_BOLD);
    if (hilight)
      wattron(win, highlight_attr);

    /* Draw */
    wprintw(win, "%c%c%s", (is_tagged) ? TAGGED_SYMBOL : ' ',
            GetTypeOfFile(fe_ptr->stat_struct), display_name);

    /* Pad remaining width */
    int printed_len = 2 + StrVisualLength(display_name);
    int k;
    for (k = printed_len; k < ctx->fixed_col_width; k++) {
      waddch(win, ' ');
    }

    /* Cleanup */
    if (hilight)
      wattroff(win, highlight_attr);
    if (is_tagged)
      wattroff(win, A_BOLD);
    wattroff(win, COLOR_PAIR(color));
    return;
  }

  (panel->reverse_sort) ? (justify = '+') : (justify = '-');

  if (old_cols != COLS) {
    old_cols = COLS;
    if (line_buffer)
      free(line_buffer);

    line_buffer_size = COLS + PATH_LENGTH;
    line_buffer = (char *)xmalloc(line_buffer_size);
  }
  if (line_buffer == NULL || line_buffer_size == 0)
    return;

  if (S_ISLNK(fe_ptr->stat_struct.st_mode))
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
  if (S_ISLNK(fe_ptr->stat_struct.st_mode))
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

  ef_window_width = width - pos_x - 1; /* Effective width for this column */
  if (ef_window_width < 0)
    ef_window_width = 0;

  wmove(win, y, pos_x);
  base_color_pair = GetFileTypeColor(ctx, fe_ptr);

  if (ctx->highlight_full_line) {
    /* --- RENDER METHOD 1: FULL LINE HIGHLIGHT --- */
    wattron(win, COLOR_PAIR(base_color_pair));
    if (is_tagged)
      wattron(win, A_BOLD);
    if (hilight)
      wattron(win, highlight_attr);

    /* Build the full line string */
    switch (panel->file_mode) {
    case MODE_1:
      (void)GetAttributes(fe_ptr->stat_struct.st_mode, attributes);
      (void)CTime(fe_ptr->stat_struct.st_mtime, modify_time);
      if (S_ISLNK(fe_ptr->stat_struct.st_mode)) {
        /* Updated %12s to %16s */
        (void)snprintf(format, sizeof(format),
                       "%%c%%c%%-%ds %%10s %%3d %%11lld %%16s -> %%-%ds",
                       filename_width, linkname_width);
        (void)snprintf(line_buffer, line_buffer_size, format,
                       (is_tagged) ? TAGGED_SYMBOL : ' ', type_of_file,
                       fe_ptr->name, attributes, fe_ptr->stat_struct.st_nlink,
                       (long long)fe_ptr->stat_struct.st_size, modify_time,
                       sym_link_name);
      } else {
        (void)snprintf(format, sizeof(format),
                       "%%c%%c%%%c%ds %%10s %%3d %%11lld %%16s", justify,
                       filename_width);
        (void)snprintf(line_buffer, line_buffer_size, format,
                       (is_tagged) ? TAGGED_SYMBOL : ' ', type_of_file,
                       fe_ptr->name, attributes, fe_ptr->stat_struct.st_nlink,
                       (long long)fe_ptr->stat_struct.st_size, modify_time);
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
        (void)snprintf(format, sizeof(format),
                       "%%c%%c%%%c%ds %%10lld %%-12s %%-12s -> %%-%ds", justify,
                       filename_width, linkname_width);
        (void)snprintf(line_buffer, line_buffer_size, format,
                       (is_tagged) ? TAGGED_SYMBOL : ' ', type_of_file,
                       fe_ptr->name, (long long)fe_ptr->stat_struct.st_ino,
                       owner_name_ptr, group_name_ptr, sym_link_name);
      } else {
        (void)snprintf(format, sizeof(format),
                       "%%c%%c%%%c%ds %%10lld %%-12s %%-12s", justify,
                       filename_width);
        (void)snprintf(line_buffer, line_buffer_size, format,
                       (is_tagged) ? TAGGED_SYMBOL : ' ', type_of_file,
                       fe_ptr->name, (long long)fe_ptr->stat_struct.st_ino,
                       owner_name_ptr, group_name_ptr);
      }
      break;
    case MODE_3:
      (void)snprintf(format, sizeof(format), "%%c%%c%%%c%ds", justify,
                     filename_width);
      (void)snprintf(line_buffer, line_buffer_size, format,
                     (is_tagged) ? TAGGED_SYMBOL : ' ', type_of_file,
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
                       (is_tagged) ? TAGGED_SYMBOL : ' ', type_of_file,
                       fe_ptr->name, change_time, access_time, sym_link_name);
      } else {
        (void)snprintf(format, sizeof(format),
                       "%%c%%c%%%c%ds Chg: %%16s  Acc: %%16s", justify,
                       filename_width);
        (void)snprintf(line_buffer, line_buffer_size, format,
                       (is_tagged) ? TAGGED_SYMBOL : ' ', type_of_file,
                       fe_ptr->name, change_time, access_time);
      }
      break;
    case MODE_5:
      BuildUserFileEntry(fe_ptr, filename_width, linkname_width, is_tagged,
                         (GetProfileValue)(ctx, "USERVIEW"), 200, line_buffer);
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

      if (line_ptr == NULL)
        return;
      line_end_pos = VisualPositionToBytePosition(line_ptr, ef_window_width);
      if (line_end_pos < 0)
        return;
      line_ptr[line_end_pos] = '\0';
    }
    waddstr(win, line_ptr);

    if (hilight)
      wattroff(win, highlight_attr);
    if (is_tagged)
      wattroff(win, A_BOLD);

  } else {
    /* --- RENDER METHOD 2: NAME-ONLY HIGHLIGHT --- */

    wattron(win, COLOR_PAIR(base_color_pair));
    if (is_tagged)
      wattron(win, A_BOLD);

    /* Print tag and type */
    {
      char prefix[3];
      prefix[0] = (is_tagged) ? TAGGED_SYMBOL : ' ';
      prefix[1] = type_of_file;
      prefix[2] = '\0';
      AddClippedAtCursor(win, prefix, width);
    }

    /* Calculate available width for name and truncate if necessary */
    int overhead = 0;
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
      int copied_len =
          snprintf(display_name, sizeof(display_name), "%s", fe_ptr->name);
      if (copied_len < 0) {
        display_name[0] = '\0';
      } else if ((size_t)copied_len >= sizeof(display_name)) {
        display_name[sizeof(display_name) - 1] = '\0';
      }
    }

    /* Highlight only the name */
    if (hilight)
      wattron(win, highlight_attr);
    AddClippedAtCursor(win, display_name, width);
    if (hilight)
      wattroff(win, highlight_attr);

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
        {
          char detail[PATH_LENGTH + 128];
          /* Updated %12s to %16s */
          if (sym_link_name && *sym_link_name) {
            (void)snprintf(detail, sizeof(detail),
                           " %10s %3d %11lld %16s -> %s", attributes,
                           (int)fe_ptr->stat_struct.st_nlink,
                           (long long)fe_ptr->stat_struct.st_size, modify_time,
                           sym_link_name);
          } else {
            (void)snprintf(detail, sizeof(detail), " %10s %3d %11lld %16s",
                           attributes, (int)fe_ptr->stat_struct.st_nlink,
                           (long long)fe_ptr->stat_struct.st_size, modify_time);
          }
          AddClippedAtCursor(win, detail, width);
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
        {
          char detail[PATH_LENGTH + 128];
          if (sym_link_name && *sym_link_name) {
            (void)snprintf(detail, sizeof(detail), " %10lld %-12s %-12s -> %s",
                           (long long)fe_ptr->stat_struct.st_ino,
                           owner_name_ptr, group_name_ptr, sym_link_name);
          } else {
            (void)snprintf(detail, sizeof(detail), " %10lld %-12s %-12s",
                           (long long)fe_ptr->stat_struct.st_ino,
                           owner_name_ptr, group_name_ptr);
          }
          AddClippedAtCursor(win, detail, width);
        }
        break;
      case MODE_4:
        (void)CTime(fe_ptr->stat_struct.st_ctime, change_time);
        (void)CTime(fe_ptr->stat_struct.st_atime, access_time);
        {
          char detail[PATH_LENGTH + 128];
          /* Updated %12s to %16s */
          if (sym_link_name && *sym_link_name) {
            (void)snprintf(detail, sizeof(detail), " Chg: %16s  Acc: %16s -> %s",
                           change_time, access_time, sym_link_name);
          } else {
            (void)snprintf(detail, sizeof(detail), " Chg: %16s  Acc: %16s",
                           change_time, access_time);
          }
          AddClippedAtCursor(win, detail, width);
        }
        break;
      case MODE_5:
        break;
      }
    }

    if (is_tagged)
      wattroff(win, A_BOLD);
  }
  wattroff(win, COLOR_PAIR(base_color_pair));
}

void DisplayFiles(ViewContext *ctx, YtreePanel *panel, const DirEntry *de_ptr,
                  int start_file_no, int hilight_no, int start_x, WINDOW *win) {
  int x, y, p_x, p_y, j;
  BOOL show_empty_label;

  if (!ctx || !panel || !panel->vol || !win)
    return;
  int height;

  height = getmaxy(win);

#ifdef COLOR_SUPPORT
  WbkgdSet(ctx, win, COLOR_PAIR(CPAIR_WINFILE));
#endif
  werase(win);

  show_empty_label = (panel->file_entry_list == NULL || panel->file_count == 0 ||
                      start_file_no >= (int)panel->file_count);
  if (show_empty_label) {
    const int first_filename_col = 2;
    const char *empty_label = "No files";
    if (de_ptr->access_denied) {
      empty_label = "Permission Denied!";
    } else if (de_ptr->unlogged_flag || de_ptr->not_scanned) {
      empty_label = "Unlogged";
    }
    mvwaddstr(win, 0, first_filename_col, empty_label);
  }

  if (!panel->file_entry_list || panel->file_count == 0) {
    wnoutrefresh(win);
    return;
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
          PrintFileEntry(ctx, panel, j, y, x, FALSE, start_x, win);
        }
      }
      j++;
    }
  }

  if (p_x >= 0)
    PrintFileEntry(ctx, panel, hilight_no, p_y, p_x, TRUE, start_x, win);

  wnoutrefresh(win);
}
