/***************************************************************************
 *
 * src/ui/render_dir.c
 * Directory tree rendering logic
 *
 ***************************************************************************/

#include "ytnova_appstate_mode.h"
#include "ytnova_cmd.h"
#include "ytnova_ui.h"

static const YtreeNovaPanel *ResolveDirRenderPanel(const ViewContext *ctx,
                                               const struct Volume *vol,
                                               const WINDOW *win) {
  if (!ctx || !vol || !win)
    return NULL;

  if (ctx->left && ctx->left->vol == vol && ctx->left->pan_dir_window == win)
    return ctx->left;
  if (ctx->right && ctx->right->vol == vol && ctx->right->pan_dir_window == win)
    return ctx->right;
  if (ctx->active && ctx->active->vol == vol && ctx->ctx_dir_window == win)
    return ctx->active;

  return NULL;
}

/*
 * SetDirMode
 * Sets the display mode for directory entries.
 * Avoid using 'mode' as parameter name due to global macro collision.
 */
void SetDirMode(ViewContext *ctx, int new_mode) {
  (void)AppStateCommitDirectoryDisplayMode(ctx, new_mode);
}

void RotateDirMode(ViewContext *ctx) {
  int next_mode;

  if (!ctx)
    return;

  next_mode = ctx->dir_mode;
  switch (ctx->dir_mode) {
  case MODE_1:
    next_mode = MODE_2;
    break;
  case MODE_2:
    next_mode = MODE_4;
    break;
  case MODE_3:
    next_mode = MODE_1;
    break;
  case MODE_4:
    next_mode = MODE_3;
    break;
  }
  (void)AppStateCommitDirectoryDisplayMode(ctx, next_mode);
}

void PrintDirEntry(ViewContext *ctx, struct Volume *vol, WINDOW *win,
                   int entry_no, int y, unsigned char hilight, BOOL is_active) {
  unsigned int j;
  int color;
  int highlight_color;
  int margin_color;
  int tree_line_color;

  if (!ctx || !vol || !win)
    return;
  char graph_buffer[PATH_LENGTH + 1];
  const char *format = NULL;
  char *line_buffer = NULL;
  size_t line_buffer_capacity = 0;
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
  BOOL append_expand_suffix = FALSE;

  if (win == ctx->ctx_f2_window) {
    color = UI_ROLE_PICKER;
    highlight_color = UI_ROLE_SELECTION;
    margin_color = UI_ROLE_PICKER;
    tree_line_color = UI_ROLE_PICKER;
  } else {
    color = UI_ROLE_DYNAMIC_TEXT;
    highlight_color = UI_ROLE_SELECTION;
    margin_color = UI_ROLE_MARGIN;
    tree_line_color = UI_ROLE_TREE_LINES;
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
    line_buffer_capacity = 96;
    line_buffer = (char *)xmalloc(line_buffer_capacity);
    format = "%10s %3d %8lld %16s";

    (void)snprintf(line_buffer, line_buffer_capacity, format, attributes,
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
    line_buffer_capacity = 160;
    line_buffer = (char *)xmalloc(line_buffer_capacity);

    format = "%12u  %-12s %-12s";
    (void)snprintf(line_buffer, line_buffer_capacity, format,
                   (unsigned int)de_ptr->stat_struct.st_ino, owner_name_ptr,
                   group_name_ptr);
    break;
  case MODE_3: /* No attributes, line_buffer remains NULL */
    break;
  case MODE_4:
    (void)CTime(de_ptr->stat_struct.st_ctime, change_time);
    (void)CTime(de_ptr->stat_struct.st_atime, access_time);
    format = "Chg.: %16s  Acc.: %16s";
    line_buffer_capacity = 80;
    line_buffer = (char *)xmalloc(line_buffer_capacity);

    (void)snprintf(line_buffer, line_buffer_capacity, format, change_time,
                   access_time);
    break;
  }

  const int status_col = 0;
  const int graph_col = 3;
  int attr_start_col = 38; /* Column where attributes begin */
  int graph_len = strlen(graph_buffer);
  BOOL full_line_highlight =
      (ctx->highlight_full_line && win != ctx->ctx_f2_window);
  chtype line_attr;
  chtype margin_attr;
  chtype tree_line_attr;
  chtype name_attr;
  chtype inactive_full_line_attr;
  chtype active_name_highlight_attr;

  wmove(win, y, 0);
  wclrtoeol(win);

  /* Set the base attribute for the line */
#ifdef COLOR_SUPPORT
  line_attr = (hilight && full_line_highlight && is_active)
                  ? COLOR_PAIR(highlight_color)
                  : COLOR_PAIR(color);
  margin_attr = (hilight && full_line_highlight && is_active)
                    ? COLOR_PAIR(highlight_color)
                    : COLOR_PAIR(margin_color);
  tree_line_attr = (hilight && full_line_highlight && is_active)
                       ? COLOR_PAIR(highlight_color)
                       : COLOR_PAIR(tree_line_color);
  active_name_highlight_attr =
      (win == ctx->ctx_f2_window) ? UISelectionAttrForBase(ctx, UI_ROLE_PICKER)
                                  : COLOR_PAIR(highlight_color);
#else
  line_attr = A_NORMAL;
  margin_attr = A_NORMAL;
  tree_line_attr = A_NORMAL;
  active_name_highlight_attr = A_REVERSE;
#endif
  name_attr = line_attr;
  inactive_full_line_attr = (hilight && full_line_highlight && !is_active)
                                ? (A_BOLD | A_UNDERLINE)
                                : A_NORMAL;
  if (inactive_full_line_attr != A_NORMAL) {
    margin_attr |= inactive_full_line_attr;
    tree_line_attr |= inactive_full_line_attr;
    name_attr |= inactive_full_line_attr;
  }

#ifndef COLOR_SUPPORT
  if (hilight && full_line_highlight && is_active)
    name_attr = margin_attr = tree_line_attr = A_REVERSE;
#endif

  /* Part 1: Draw status marker and tree graph characters manually */
  wattrset(win, margin_attr);
  mvwaddch(win, y, status_col,
           (de_ptr->unlogged_flag || de_ptr->not_scanned) ? '+' : ' ');
  wmove(win, y, graph_col);
  wattrset(win, tree_line_attr);
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
    waddch(win, (chtype)ch | ((win == ctx->ctx_f2_window) ? 0 : A_BOLD));
  }
  wattroff(win, A_ALTCHARSET);
  wattrset(win, name_attr);

  /* Part 2: Prepare and draw the directory name */
  char name_buffer[PATH_LENGTH + 2];
  dir_name = de_ptr->name;
  (void)snprintf(name_buffer, sizeof(name_buffer), "%s",
                 (*dir_name) ? dir_name : ".");
  if (de_ptr->not_scanned) {
    BOOL has_subdirs = (de_ptr->sub_tree != NULL);
    if (!has_subdirs && S_ISDIR(de_ptr->stat_struct.st_mode) &&
        de_ptr->stat_struct.st_nlink > 2) {
      has_subdirs = TRUE;
    }
    if (has_subdirs) {
      size_t name_len = strlen(name_buffer);
      if (name_len < sizeof(name_buffer) - 1) {
        name_buffer[name_len] = '/';
        name_buffer[name_len + 1] = '\0';
        append_expand_suffix = TRUE;
      }
    }
  }

  /* Calculate maximum allowed name length based on mode and window width */
  int max_name_len;
  if (ctx->dir_mode == MODE_3) {
    /* In MODE_3 (name-only), truncate based on full window width */
    max_name_len = ctx->layout.dir_win_width - graph_col - graph_len - 1;
  } else {
    /* In other modes, truncate to prevent overlap with attributes */
    max_name_len = attr_start_col - graph_col - graph_len - 1;
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

  /* If name-only highlight is active, select just the directory name. */
  if (hilight && !full_line_highlight) {
    size_t highlight_name_len = strlen(name_buffer);
    BOOL split_expand_suffix =
        (win == ctx->ctx_f2_window && append_expand_suffix &&
         highlight_name_len > 0);
#ifdef COLOR_SUPPORT
    if (is_active)
      wattrset(win, active_name_highlight_attr);
    else
      wattron(win, A_BOLD | A_UNDERLINE);
#else
    if (is_active)
      wattron(win, A_REVERSE);
    else
      wattron(win, A_BOLD | A_UNDERLINE);
#endif
    if (split_expand_suffix) {
      highlight_name_len--;
      char saved_ch = name_buffer[highlight_name_len];

      name_buffer[highlight_name_len] = '\0';
      mvwaddstr(win, y, graph_col + graph_len, name_buffer);
#ifdef COLOR_SUPPORT
      if (is_active)
        wattrset(win, name_attr);
      else
        wattroff(win, A_BOLD | A_UNDERLINE);
#else
      if (is_active)
        wattroff(win, A_REVERSE);
      else
        wattroff(win, A_BOLD | A_UNDERLINE);
#endif
      name_buffer[highlight_name_len] = saved_ch;
      waddnstr(win, name_buffer + highlight_name_len,
               (int)(strlen(name_buffer) - highlight_name_len));
    } else {
      mvwaddstr(win, y, graph_col + graph_len, name_buffer);
#ifdef COLOR_SUPPORT
      if (is_active)
        wattrset(win, name_attr);
      else
        wattroff(win, A_BOLD | A_UNDERLINE);
#else
      if (is_active)
        wattroff(win, A_REVERSE);
      else
        wattroff(win, A_BOLD | A_UNDERLINE);
#endif
    }
  } else {
    mvwaddstr(win, y, graph_col + graph_len, name_buffer);
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

  wattrset(win, 0);

  if (line_buffer)
    free(line_buffer);
}

void DisplayTree(ViewContext *ctx, struct Volume *vol, WINDOW *win,
                 int start_entry_no, int hilight_no, BOOL is_active) {
  int i, y;
  int list_idx;
  int height;
  const YtreeNovaPanel *panel;

  if (!ctx || !vol || !win)
    return;

  y = -1;
  getmaxyx(win, height, i); /* Use i for width to avoid unused var warning */
  (void)i;

#ifdef COLOR_SUPPORT
  if (win == ctx->ctx_f2_window) {
    WbkgdSet(ctx, win, COLOR_PAIR(UI_ROLE_PICKER));
  } else {
    WbkgdSet(ctx, win, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));
  }
#endif
  werase(win);

  if (win == ctx->ctx_f2_window) {
#ifdef COLOR_SUPPORT
    wattron(win, COLOR_PAIR(UI_ROLE_BOX_LINES));
#endif
    box(win, 0, 0);
#ifdef COLOR_SUPPORT
    wattroff(win, COLOR_PAIR(UI_ROLE_BOX_LINES));
#endif
  }

  panel = ResolveDirRenderPanel(ctx, vol, win);
  list_idx = start_entry_no;

  if (list_idx < 0)
    list_idx = 0;

  for (i = 0; i < height && list_idx < vol->total_dirs; i++) {
    while (list_idx < vol->total_dirs) {
      const DirEntry *candidate = vol->dir_entry_list[list_idx].dir_entry;
      if (!panel || PanelDirIsVisible(panel, candidate))
        break;
      list_idx++;
    }
    if (list_idx >= vol->total_dirs)
      break;

    if (list_idx != hilight_no)
      PrintDirEntry(ctx, vol, win, list_idx, i, FALSE, is_active);
    else
      y = i;
    list_idx++;
  }

  if (y >= 0)
    PrintDirEntry(ctx, vol, win, hilight_no, y, TRUE, is_active);
}
