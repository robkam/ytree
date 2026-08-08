/***************************************************************************
 *
 * src/ui/render_dir.c
 * Directory tree rendering logic
 *
 ***************************************************************************/

#include "ytnova_appstate_mode.h"
#include "ytnova_cmd.h"
#include "ytnova_ui.h"

static void FormatBinaryDirSize(const ViewContext *ctx, long long value,
                                char *buffer, size_t buffer_size) {
  char temp[64];
  int len;
  int commas;
  int i;
  int j;
  char separator;

  if (!buffer || buffer_size == 0)
    return;

  (void)snprintf(temp, sizeof(temp), "%lld", value);
  len = (int)strlen(temp);
  commas = (len - 1) / 3;
  if ((size_t)(len + commas + 1) > buffer_size) {
    (void)snprintf(buffer, buffer_size, "%lld", value);
    return;
  }

  separator = (ctx && ctx->number_seperator) ? ctx->number_seperator : ',';
  j = len + commas;
  buffer[j] = '\0';
  for (i = len - 1; i >= 0; i--) {
    buffer[--j] = temp[i];
    if (i > 0 && (len - i) % 3 == 0)
      buffer[--j] = separator;
  }
}

static void FormatDirSize(const ViewContext *ctx, const YtreeNovaPanel *panel,
                          long long value, char *buffer, size_t buffer_size) {
  double scaled = (double)value;
  int unit_index = 0;
  static const char *units[] = {"B", "K", "M", "G", "T", "P"};

  if (!buffer || buffer_size == 0)
    return;
  if (!panel || !panel->human_size_units) {
    FormatBinaryDirSize(ctx, value, buffer, buffer_size);
    return;
  }
  if (value < 0) {
    (void)snprintf(buffer, buffer_size, "Err");
    return;
  }
  while (scaled >= 999.5 && unit_index < 5) {
    scaled /= 1024.0;
    unit_index++;
  }
  if (unit_index == 0)
    (void)snprintf(buffer, buffer_size, "%lld%s", value, units[unit_index]);
  else
    (void)snprintf(buffer, buffer_size, "%.1f%s", scaled, units[unit_index]);
}

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
  if (ctx->active && ctx->ctx_f2_window == win)
    return ctx->active;

  return NULL;
}

static BOOL DirEntryVisibleForRenderPanel(const YtreeNovaPanel *panel,
                                          const struct Volume *vol,
                                          const DirEntry *dir_entry) {
  const DirEntry *ancestor;

  if (!panel || !vol || !dir_entry)
    return FALSE;

  if (panel->vol == vol)
    return PanelDirIsVisible(panel, dir_entry);

  if (!panel->hide_dot_files)
    return TRUE;

  if (dir_entry == vol->vol_stats.tree)
    return TRUE;

  if (dir_entry->name[0] == '.')
    return FALSE;

  for (ancestor = dir_entry->up_tree; ancestor && ancestor != vol->vol_stats.tree;
       ancestor = ancestor->up_tree) {
    if (ancestor->name[0] == '.')
      return FALSE;
  }

  return TRUE;
}

static int ResolveDirRenderMode(const ViewContext *ctx, const struct Volume *vol,
                                const WINDOW *win) {
  const YtreeNovaPanel *panel = ResolveDirRenderPanel(ctx, vol, win);

  if (panel)
    return panel->dir_mode;
  if (ctx)
    return ctx->dir_mode;
  return MODE_3;
}

typedef struct DirEntryNameRender {
  char text[PATH_LENGTH + 2];
  BOOL append_expand_suffix;
} DirEntryNameRender;

typedef struct DirEntryRenderState {
  ViewContext *ctx;
  WINDOW *win;
  int y;
  int graph_len;
  int attr_start_col;
  unsigned char hilight;
  BOOL full_line_highlight;
  BOOL is_active;
  chtype margin_attr;
  chtype tree_line_attr;
  chtype name_attr;
  chtype active_name_highlight_attr;
} DirEntryRenderState;

static void BuildDirGraphBuffer(const struct Volume *vol, int entry_no,
                                const DirEntry *de_ptr, char *graph_buffer,
                                size_t graph_buffer_size) {
  unsigned int j;
  size_t graph_used = 0;

  if (!vol || !de_ptr || !graph_buffer || graph_buffer_size == 0)
    return;

  graph_buffer[0] = '\0';
  for (j = 0; j < vol->dir_entry_list[entry_no].level; j++) {
    const char *segment =
        (vol->dir_entry_list[entry_no].indent & (1L << j)) ? "| " : "  ";
    int written = snprintf(graph_buffer + graph_used,
                           graph_buffer_size - graph_used, "%s", segment);

    if (written < 0)
      break;
    if ((size_t)written >= graph_buffer_size - graph_used) {
      graph_used = graph_buffer_size - 1;
      break;
    }
    graph_used += (size_t)written;
  }

  (void)snprintf(graph_buffer + graph_used, graph_buffer_size - graph_used, "%s",
                 de_ptr->next ? "6-" : "3-");
}

static char *BuildDirAttributeLine(const ViewContext *ctx,
                                   const YtreeNovaPanel *render_panel,
                                   const DirEntry *de_ptr, int dir_mode) {
  char attributes[11];
  char modify_time[20];
  char change_time[20];
  char access_time[20];
  char owner[OWNER_NAME_MAX + 1];
  char group[GROUP_NAME_MAX + 1];
  const char *owner_name_ptr;
  const char *group_name_ptr;
  char size_text[32];
  char *line_buffer;

  if (!de_ptr)
    return NULL;

  switch (dir_mode) {
  case MODE_1:
    line_buffer = (char *)xmalloc(96);
    (void)GetAttributes(de_ptr->stat_struct.st_mode, attributes);
    (void)CTime(de_ptr->stat_struct.st_mtime, modify_time);
    FormatDirSize(ctx, render_panel, (long long)de_ptr->stat_struct.st_size,
                  size_text, sizeof(size_text));
    (void)snprintf(line_buffer, 96, "%10s %3d %11s %16s", attributes,
                   (int)de_ptr->stat_struct.st_nlink, size_text, modify_time);
    return line_buffer;
  case MODE_2:
    line_buffer = (char *)xmalloc(160);
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
    (void)snprintf(line_buffer, 160, "%12u  %-12s %-12s",
                   (unsigned int)de_ptr->stat_struct.st_ino, owner_name_ptr,
                   group_name_ptr);
    return line_buffer;
  case MODE_4:
    line_buffer = (char *)xmalloc(80);
    (void)CTime(de_ptr->stat_struct.st_ctime, change_time);
    (void)CTime(de_ptr->stat_struct.st_atime, access_time);
    (void)snprintf(line_buffer, 80, "Chg.: %16s  Acc.: %16s", change_time,
                   access_time);
    return line_buffer;
  case MODE_3:
  default:
    return NULL;
  }
}

static void DrawDirEntryGraph(const DirEntryRenderState *render_state,
                              const DirEntry *de_ptr,
                              const char *graph_buffer) {
  ViewContext *ctx;
  WINDOW *win;
  chtype margin_attr;
  chtype tree_line_attr;
  unsigned int j;

  if (!render_state || !render_state->ctx || !de_ptr || !render_state->win ||
      !graph_buffer)
    return;

  ctx = render_state->ctx;
  win = render_state->win;
  margin_attr = render_state->margin_attr;
  tree_line_attr = render_state->tree_line_attr;

  wattrset(win, margin_attr);
  mvwaddch(win, render_state->y, 0,
           (de_ptr->unlogged_flag || de_ptr->not_scanned) ? '+' : ' ');
  wmove(win, render_state->y, 3);
  wattrset(win, tree_line_attr);
  wattron(win, A_ALTCHARSET);
  for (j = 0; j < (unsigned int)render_state->graph_len; ++j) {
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
}

static DirEntryNameRender PrepareDirNameRender(const ViewContext *ctx,
                                               const DirEntry *de_ptr,
                                               int dir_mode,
                                               const DirEntryRenderState
                                                   *render_state) {
  DirEntryNameRender name_render;
  int max_name_len;

  name_render.text[0] = '\0';
  name_render.append_expand_suffix = FALSE;
  if (!ctx || !de_ptr || !render_state)
    return name_render;

  (void)snprintf(name_render.text, sizeof(name_render.text), "%s",
                 (*de_ptr->name) ? de_ptr->name : ".");
  if (de_ptr->not_scanned) {
    BOOL has_subdirs = (de_ptr->sub_tree != NULL);

    if (!has_subdirs && S_ISDIR(de_ptr->stat_struct.st_mode) &&
        de_ptr->stat_struct.st_nlink > 2) {
      has_subdirs = TRUE;
    }
    if (has_subdirs) {
      size_t name_len = strlen(name_render.text);

      if (name_len < sizeof(name_render.text) - 1) {
        name_render.text[name_len] = '/';
        name_render.text[name_len + 1] = '\0';
        name_render.append_expand_suffix = TRUE;
      }
    }
  }

  max_name_len = (dir_mode == MODE_3)
                     ? ctx->layout.dir_win_width - 3 - render_state->graph_len - 1
                     : render_state->attr_start_col - 3 - render_state->graph_len - 1;
  if (max_name_len < 1)
    max_name_len = 1;
  if ((int)strlen(name_render.text) > max_name_len) {
    char temp_name[PATH_LENGTH + 2];

    CutName(temp_name, name_render.text, max_name_len);
    (void)snprintf(name_render.text, sizeof(name_render.text), "%s", temp_name);
  }
  return name_render;
}

static void DrawDirEntryName(const DirEntryRenderState *render_state,
                             DirEntryNameRender *name_render) {
  const ViewContext *ctx;
  WINDOW *win;
  BOOL is_active;
  size_t highlight_name_len;
  BOOL split_expand_suffix;

  if (!render_state || !render_state->ctx || !render_state->win || !name_render)
    return;

  ctx = render_state->ctx;
  win = render_state->win;
  is_active = render_state->is_active;
  highlight_name_len = strlen(name_render->text);
  split_expand_suffix = (win == ctx->ctx_f2_window &&
                         name_render->append_expand_suffix &&
                         highlight_name_len > 0);
  if (!render_state->hilight || render_state->full_line_highlight) {
    mvwaddstr(win, render_state->y, 3 + render_state->graph_len,
              name_render->text);
    return;
  }

#ifdef COLOR_SUPPORT
  if (is_active)
    wattrset(win, render_state->active_name_highlight_attr);
  else
    wattron(win, A_BOLD | A_UNDERLINE);
#else
  if (is_active)
    wattron(win, A_REVERSE);
  else
    wattron(win, A_BOLD | A_UNDERLINE);
#endif
  if (split_expand_suffix) {
    char saved_ch;

    highlight_name_len--;
    saved_ch = name_render->text[highlight_name_len];
    name_render->text[highlight_name_len] = '\0';
    mvwaddstr(win, render_state->y, 3 + render_state->graph_len,
              name_render->text);
#ifdef COLOR_SUPPORT
    if (is_active)
      wattrset(win, render_state->name_attr);
    else
      wattroff(win, A_BOLD | A_UNDERLINE);
#else
    if (is_active)
      wattroff(win, A_REVERSE);
    else
      wattroff(win, A_BOLD | A_UNDERLINE);
#endif
    name_render->text[highlight_name_len] = saved_ch;
    waddnstr(win, name_render->text + highlight_name_len,
             (int)(strlen(name_render->text) - highlight_name_len));
    return;
  }

  mvwaddstr(win, render_state->y, 3 + render_state->graph_len,
            name_render->text);
#ifdef COLOR_SUPPORT
  if (is_active)
    wattrset(win, render_state->name_attr);
  else
    wattroff(win, A_BOLD | A_UNDERLINE);
#else
  if (is_active)
    wattroff(win, A_REVERSE);
  else
    wattroff(win, A_BOLD | A_UNDERLINE);
#endif
}

static void DrawDirEntryAttributes(const DirEntryRenderState *render_state,
                                   const char *line_buffer) {
  int current_x;
  int i;
  int current_y;

  if (!render_state || !render_state->win || !line_buffer)
    return;

  getyx(render_state->win, current_y, current_x);
  (void)current_y;
  for (i = current_x; i < render_state->attr_start_col; ++i)
    waddch(render_state->win, ' ');
  mvwaddstr(render_state->win, render_state->y, render_state->attr_start_col,
            line_buffer);
}

/*
 * SetDirMode
 * Sets the display mode for directory entries.
 * Avoid using 'mode' as parameter name due to global macro collision.
 */
void SetDirMode(ViewContext *ctx, int new_mode) {
  (void)AppStateCommitDirectoryDisplayMode(ctx, new_mode);
}

void SelectDirMode(ViewContext *ctx, int selection) {
  int new_mode = MODE_3;
  int current_mode;

  if (!ctx)
    return;

  current_mode = (ctx->active) ? ctx->active->dir_mode : ctx->dir_mode;

  switch (selection) {
  case 1:
    new_mode = MODE_3;
    break;
  case 2:
    new_mode = (current_mode == MODE_1) ? MODE_3 : MODE_1;
    break;
  case 3:
    new_mode = (current_mode == MODE_2) ? MODE_3 : MODE_2;
    break;
  case 4:
    new_mode = (current_mode == MODE_4) ? MODE_3 : MODE_4;
    break;
  default:
    return;
  }
  (void)AppStateCommitDirectoryDisplayMode(ctx, new_mode);
}

void RotateDirMode(ViewContext *ctx) {
  int next_mode;
  int current_mode;

  if (!ctx)
    return;

  current_mode = (ctx->active) ? ctx->active->dir_mode : ctx->dir_mode;
  next_mode = current_mode;
  switch (current_mode) {
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
  char graph_buffer[PATH_LENGTH + 1];
  char *line_buffer = NULL;
  const YtreeNovaPanel *render_panel;
  const DirEntry *de_ptr;
  DirEntryNameRender name_render;
  DirEntryRenderState render_state;
  int dir_mode;
  const int attr_start_col = 38;
  chtype inactive_full_line_attr;
  BOOL full_line_highlight;

  if (!ctx || !vol || !win)
    return;

  de_ptr = vol->dir_entry_list[entry_no].dir_entry;
  dir_mode = ResolveDirRenderMode(ctx, vol, win);
  render_panel = ResolveDirRenderPanel(ctx, vol, win);
  BuildDirGraphBuffer(vol, entry_no, de_ptr, graph_buffer, sizeof(graph_buffer));
  line_buffer = BuildDirAttributeLine(ctx, render_panel, de_ptr, dir_mode);
  render_state.ctx = ctx;
  render_state.win = win;
  render_state.y = y;
  render_state.graph_len = (int)strlen(graph_buffer);
  render_state.attr_start_col = attr_start_col;
  render_state.hilight = hilight;
  render_state.full_line_highlight =
      (ctx->highlight_full_line && win != ctx->ctx_f2_window);
  render_state.is_active = is_active;
  full_line_highlight = render_state.full_line_highlight;

  wmove(win, y, 0);
  wclrtoeol(win);

#ifdef COLOR_SUPPORT
  {
    int color;
    int highlight_color;
    int margin_color;
    int tree_line_color;

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

  render_state.name_attr =
      (hilight && render_state.full_line_highlight && is_active)
          ? COLOR_PAIR(highlight_color)
          : COLOR_PAIR(color);
  render_state.margin_attr =
      (hilight && render_state.full_line_highlight && is_active)
          ? COLOR_PAIR(highlight_color)
          : COLOR_PAIR(margin_color);
  render_state.tree_line_attr =
      (hilight && render_state.full_line_highlight && is_active)
          ? COLOR_PAIR(highlight_color)
          : COLOR_PAIR(tree_line_color);
  render_state.active_name_highlight_attr =
      (win == ctx->ctx_f2_window) ? UISelectionAttrForBase(ctx, UI_ROLE_PICKER)
                                  : COLOR_PAIR(highlight_color);
  }
#else
  render_state.name_attr = A_NORMAL;
  render_state.margin_attr = A_NORMAL;
  render_state.tree_line_attr = A_NORMAL;
  render_state.active_name_highlight_attr = A_REVERSE;
#endif
  inactive_full_line_attr = (hilight && full_line_highlight && !is_active)
                                ? (A_BOLD | A_UNDERLINE)
                                : A_NORMAL;
  if (inactive_full_line_attr != A_NORMAL) {
    render_state.margin_attr |= inactive_full_line_attr;
    render_state.tree_line_attr |= inactive_full_line_attr;
    render_state.name_attr |= inactive_full_line_attr;
  }

#ifndef COLOR_SUPPORT
  if (hilight && render_state.full_line_highlight && is_active)
    render_state.name_attr = render_state.margin_attr =
        render_state.tree_line_attr = A_REVERSE;
#endif

  DrawDirEntryGraph(&render_state, de_ptr, graph_buffer);
  wattrset(win, render_state.name_attr);
  name_render = PrepareDirNameRender(ctx, de_ptr, dir_mode, &render_state);
  DrawDirEntryName(&render_state, &name_render);
  DrawDirEntryAttributes(&render_state, line_buffer);

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
    wattron(win, COLOR_PAIR(UI_ROLE_PICKER));
#endif
    box(win, 0, 0);
#ifdef COLOR_SUPPORT
    wattroff(win, COLOR_PAIR(UI_ROLE_PICKER));
#endif
  }

  panel = ResolveDirRenderPanel(ctx, vol, win);
  list_idx = start_entry_no;

  if (list_idx < 0)
    list_idx = 0;

  for (i = 0; i < height && list_idx < vol->total_dirs; i++) {
    while (list_idx < vol->total_dirs) {
      const DirEntry *candidate = vol->dir_entry_list[list_idx].dir_entry;
      if (!panel || DirEntryVisibleForRenderPanel(panel, vol, candidate))
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
