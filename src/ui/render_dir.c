/***************************************************************************
 *
 * src/ui/render_dir.c
 * Directory tree rendering logic
 *
 ***************************************************************************/

#include "ytree.h"

/*
 * SetDirMode
 * Sets the display mode for directory entries.
 * Avoid using 'mode' as parameter name due to global macro collision.
 */
void SetDirMode(ViewContext *ctx, int new_mode) { ctx->dir_mode = new_mode; }

void RotateDirMode(ViewContext *ctx) {
  /* For now, just rotate modes without the view_mode check.
   * The check will need to be done at call site with proper context. */
  switch (ctx->dir_mode) {
  case MODE_1:
    ctx->dir_mode = MODE_2;
    break;
  case MODE_2:
    ctx->dir_mode = MODE_4;
    break;
  case MODE_3:
    ctx->dir_mode = MODE_1;
    break;
  case MODE_4:
    ctx->dir_mode = MODE_3;
    break;
  }
}

void PrintDirEntry(ViewContext *ctx, struct Volume *vol, WINDOW *win,
                   int entry_no, int y, unsigned char hilight, BOOL is_active) {
  unsigned int j;
  int color;

  if (!ctx || !vol || !win)
    return;
  char graph_buffer[PATH_LENGTH + 1];
  const char *format = NULL;
  char *line_buffer = NULL;
  char *dir_name;
  char attributes[11];
  char modify_time[20]; /* Increased from 13 to 20 for "YYYY-MM-DD HH:MM" */
  char change_time[20]; /* Increased from 13 to 20 */
  char access_time[20]; /* Increased to 20 */
  char owner[OWNER_NAME_MAX + 1];
  char group[GROUP_NAME_MAX + 1];
  const char *owner_name_ptr;
  const char *group_name_ptr;
  DirEntry *de_ptr;

  if (win == ctx->ctx_f2_window) {
    color = CPAIR_HST;
  } else {
    color = CPAIR_DIR;
  }

  /* Build the tree graph string (e.g., "| 6- ") */
  graph_buffer[0] = '\0';
  size_t graph_used = 0;
  for (j = 0; j < vol->dir_entry_list[entry_no].level; j++) {
    const char *segment =
        (vol->dir_entry_list[entry_no].indent & (1L << j)) ? "| " : "  ";
    int written = snprintf(graph_buffer + graph_used,
                           sizeof(graph_buffer) - graph_used, "%s", segment);
    if (written < 0)
      break;
    if ((size_t)written >= sizeof(graph_buffer) - graph_used) {
      graph_used = sizeof(graph_buffer) - 1;
      break;
    }
    graph_used += (size_t)written;
  }

  de_ptr = vol->dir_entry_list[entry_no].dir_entry;
  {
    const char *branch_marker = de_ptr->next ? "6-" : "3-";
    (void)snprintf(graph_buffer + graph_used, sizeof(graph_buffer) - graph_used,
                   "%s", branch_marker);
  }

  /* Build the attribute string based on the current directory mode */
  switch (ctx->dir_mode) {
  case MODE_1:
    (void)GetAttributes(de_ptr->stat_struct.st_mode, attributes);
    (void)CTime(de_ptr->stat_struct.st_mtime, modify_time);
    /* Increased buffer size from 38 to 42 to accommodate 16-char date */
    line_buffer = (char *)xmalloc(42);

    /* Updated %12s to %16s for date */
    format = "%10s %3d %8lld %16s";

    (void)snprintf(line_buffer, 42, format, attributes,
                   (int)de_ptr->stat_struct.st_nlink,
                   (long long)de_ptr->stat_struct.st_size, modify_time);
    break;
  case MODE_2:
    (void)GetAttributes(de_ptr->stat_struct.st_mode, attributes);
    owner_name_ptr = GetDisplayPasswdName(de_ptr->stat_struct.st_uid);
    group_name_ptr = GetDisplayGroupName(de_ptr->stat_struct.st_gid);
    if (owner_name_ptr == NULL) {
      (void)snprintf(owner, sizeof(owner), "%d",
                     (int)de_ptr->stat_struct.st_uid);
      owner_name_ptr = owner;
    }
    if (group_name_ptr == NULL) {
      (void)snprintf(group, sizeof(group), "%d",
                     (int)de_ptr->stat_struct.st_gid);
      group_name_ptr = group;
    }
    line_buffer = (char *)xmalloc(40);

    format = "%12u  %-12s %-12s";
    (void)snprintf(line_buffer, 40, format,
                   (unsigned int)de_ptr->stat_struct.st_ino, owner_name_ptr,
                   group_name_ptr);
    break;
  case MODE_3: /* No attributes, line_buffer remains NULL */
    break;
  case MODE_4:
    (void)CTime(de_ptr->stat_struct.st_ctime, change_time);
    (void)CTime(de_ptr->stat_struct.st_atime, access_time);
    /* Increased buffer size from 40 to 50 to accommodate two 16-char dates */
    /* Format: "Chg.: " (6) + 16 + "  Acc.: " (8) + 16 = 46 chars. 50 is safe.
     */
    format = "Chg.: %16s  Acc.: %16s";
    line_buffer = (char *)xmalloc(50);

    (void)snprintf(line_buffer, 50, format, change_time, access_time);
    break;
  }

  /* --- Redesigned Drawing Logic --- */

  int attr_start_col = 38; /* Column where attributes begin */
  int graph_len = strlen(graph_buffer);
  chtype line_attr;

  wmove(win, y, 0);
  wclrtoeol(win);

  /* Set the base attribute for the line */
#ifdef COLOR_SUPPORT
  line_attr = COLOR_PAIR(color);
#else
  line_attr = A_NORMAL;
#endif
  wattron(win, line_attr);

  /* If full line highlight is enabled, turn on reverse now. */
  if (hilight && ctx->highlight_full_line) {
    if (is_active)
      wattron(win, A_REVERSE);
    else
      wattron(win, A_BOLD | A_UNDERLINE);
  }

  /* Part 1: Draw the tree graph characters manually */
  wmove(win, y, 0);
  wattron(win, A_ALTCHARSET);
  for (j = 0; j < (unsigned int)graph_len; ++j) {
    int ch;
    switch (graph_buffer[j]) {
    case '6':
      ch = ACS_LTEE;
      break;
    case '3':
      ch = ACS_LLCORNER;
      break;
    case '|':
      ch = ACS_VLINE;
      break;
    case '-':
      ch = ACS_HLINE;
      break;
    default:
      ch = graph_buffer[j];
      break;
    }
    waddch(win, (chtype)ch | A_BOLD); /* Keep graph characters bold */
  }
  wattroff(win, A_ALTCHARSET);

  /* Part 2: Prepare and draw the directory name */
  char name_buffer[PATH_LENGTH + 2];
  dir_name = de_ptr->name;
  (void)snprintf(name_buffer, sizeof(name_buffer), "%s",
                 (*dir_name) ? dir_name : ".");
  if (de_ptr->not_scanned) {
    size_t name_len = strlen(name_buffer);
    if (name_len < sizeof(name_buffer) - 1) {
      name_buffer[name_len] = de_ptr->unlogged_flag ? '+' : '/';
      name_buffer[name_len + 1] = '\0';
    }
  }

  /* Calculate maximum allowed name length based on mode and window width */
  int max_name_len;
  if (ctx->dir_mode == MODE_3) {
    /* In MODE_3 (name-only), truncate based on full window width */
    max_name_len = ctx->layout.dir_win_width - graph_len - 1;
  } else {
    /* In other modes, truncate to prevent overlap with attributes */
    max_name_len = attr_start_col - graph_len - 1;
  }
  /* Safety: Ensure max_name_len is at least 1 to avoid issues with CutName */
  if (max_name_len < 1) {
    max_name_len = 1;
  }

  /* Apply truncation if the name is too long */
  if ((int)strlen(name_buffer) > max_name_len) {
    char temp_name[PATH_LENGTH + 2];
    CutName(temp_name, name_buffer, max_name_len);
    (void)snprintf(name_buffer, sizeof(name_buffer), "%s", temp_name);
  }

  /* If name-only highlight, toggle reverse just for the name. */
  if (hilight && !ctx->highlight_full_line) {
    if (is_active)
      wattron(win, A_REVERSE);
    else
      wattron(win, A_BOLD | A_UNDERLINE);
  }
  mvwaddstr(win, y, graph_len, name_buffer);
  if (hilight && !ctx->highlight_full_line) {
    if (is_active)
      wattroff(win, A_REVERSE);
    else
      wattroff(win, A_BOLD | A_UNDERLINE);
  }

  /* Part 3: Draw attributes and fill the gap in between */
  if (line_buffer) {
    int current_x;
    getyx(win, y, current_x); /* Use y (parameter) as dummy output */
    for (int i = current_x; i < attr_start_col; ++i) {
      waddch(win, ' ');
    }
    mvwaddstr(win, y, attr_start_col, line_buffer);
  }

  /* Turn off attributes */
  if (hilight && ctx->highlight_full_line) {
    if (is_active)
      wattroff(win, A_REVERSE);
    else
      wattroff(win, A_BOLD | A_UNDERLINE);
  }
  wattroff(win, line_attr);

  if (line_buffer)
    free(line_buffer);
}

void DisplayTree(ViewContext *ctx, struct Volume *vol, WINDOW *win,
                 int start_entry_no, int hilight_no, BOOL is_active) {
  int i, y;
  int height;

  if (!ctx || !vol || !win)
    return;

  y = -1;
  getmaxyx(win, height, i); /* Use i for width to avoid unused var warning */
  (void)i;

#ifdef COLOR_SUPPORT
  if (win == ctx->ctx_f2_window) {
    WbkgdSet(ctx, win, COLOR_PAIR(CPAIR_WINHST));
  } else {
    WbkgdSet(ctx, win, COLOR_PAIR(CPAIR_WINDIR));
  }
#endif
  werase(win);

  if (win == ctx->ctx_f2_window) {
    box(win, 0, 0);
  }

  for (i = 0; i < height; i++) {
    if (start_entry_no + i >= vol->total_dirs)
      break;

    if (start_entry_no + i != hilight_no)
      PrintDirEntry(ctx, vol, win, start_entry_no + i, i, FALSE, is_active);
    else
      y = i;
  }

  if (y >= 0)
    PrintDirEntry(ctx, vol, win, start_entry_no + y, y, TRUE, is_active);
}
