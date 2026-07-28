/***************************************************************************
 *
 * src/ui/help_popup.c
 * Shared contextual help popup rendering.
 *
 ***************************************************************************/

#include "ytnova_ui.h"

static const UICommandStripCommand help_popup_close_commands[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "close", "Esc", "Q"},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "OK", "Enter", NULL}};

static const UICommandStripCommand help_popup_scroll_commands[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "scroll", "Up", "Down"},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "page", "PgUp", "PgDn"},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "jump", "Home", "End"},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "close", "Esc", "Q"},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "OK", "Enter", NULL}};

static int HelpPopupFooterWidth(const UICommandStripCommand *commands,
                                size_t command_count) {
  return UI_CommandStripVisualLength(commands, command_count);
}

static void RenderHelpPopupFooter(
    WINDOW *win, int y, int start_x, const UICommandStripCommand *commands,
    size_t command_count, const UIHelpPopupFooterSpec *footer_spec) {
  if (win == NULL || commands == NULL)
    return;

  (void)footer_spec;
  UI_RenderCommandStrip(win, y, start_x, commands, command_count, UI_ROLE_HELP,
                        UI_ROLE_KEYBIND);
}

static int HelpPopupRowWidth(const UIHelpPopupRow *row) {
  int width = 0;

  if (row == NULL)
    return 0;

  if (row->prefix != NULL)
    width += StrVisualLength(row->prefix);

  switch (row->kind) {
  case UI_HELP_POPUP_COMMAND_STRIP:
    width += UI_CommandStripVisualLength(row->commands, row->command_count);
    break;
  case UI_HELP_POPUP_TEXT:
  default:
    if (row->text != NULL)
      width += StrVisualLength(row->text);
    break;
  }

  return width;
}

static void RenderHelpPopupRow(WINDOW *win, int y, int content_width,
                               const UIHelpPopupRow *row) {
  int x = 2;

  if (win == NULL || row == NULL)
    return;

  if (row->prefix != NULL && row->prefix[0] != '\0') {
    PrintSpecialString(win, y, x, (char *)row->prefix, UI_ROLE_HELP);
    x += StrVisualLength(row->prefix);
  }

  switch (row->kind) {
  case UI_HELP_POPUP_COMMAND_STRIP:
    UI_RenderCommandStrip(win, y, x, row->commands, row->command_count,
                          UI_ROLE_HELP, UI_ROLE_KEYBIND);
    break;
  case UI_HELP_POPUP_TEXT:
  default:
    if (row->text != NULL)
      mvwprintw(win, y, x, "%.*s", content_width - (x - 2), row->text);
    break;
  }
}

static int ShowHelpPopupInternal(ViewContext *ctx, const char *title,
                                 const UIHelpPopupRow *rows, size_t row_count,
                                 BOOL dismiss_any_key,
                                 const UIHelpPopupFooterSpec *footer_spec) {
  static const UICommandStripCommand *const default_close_commands =
      help_popup_close_commands;
  static const UICommandStripCommand *const default_scroll_commands =
      help_popup_scroll_commands;
  WINDOW *win;
  int content_width;
  const UICommandStripCommand *effective_footer_commands;
  size_t effective_footer_count;
  int footer_width;
  int height;
  int i;
  int max_row_width;
  int max_visible_rows;
  int scroll_offset = 0;
  BOOL scrollable;
  int visible_rows;
  int width;
  int win_x;
  int win_y;
  BOOL use_history_geometry;

  if (ctx == NULL || title == NULL || rows == NULL || row_count == 0)
    return -1;

  max_row_width = 0;
  for (i = 0; i < (int)row_count; ++i) {
    int row_width = HelpPopupRowWidth(&rows[i]);

    if (row_width > max_row_width)
      max_row_width = row_width;
  }

  use_history_geometry =
      ctx != NULL && ctx->layout.main_win_width >= 48 && (LINES - 6) >= 6;
  if (use_history_geometry) {
    width = MINIMUM(ctx->layout.main_win_width, COLS - 2);
    height = MINIMUM(LINES - 6, LINES - 2);
    win_x = 1;
    win_y = 2;
  } else {
    width = StrVisualLength(title) + 8;
    if (max_row_width + 4 > width)
      width = max_row_width + 4;
    width = MAXIMUM(width, 48);
    width = MINIMUM(width, COLS - 4);

    max_visible_rows = LINES - 5;
    if (max_visible_rows < 1)
      max_visible_rows = 1;
    visible_rows = (int)row_count;
    if (visible_rows > max_visible_rows)
      visible_rows = max_visible_rows;

    scrollable = ((int)row_count > visible_rows);
    effective_footer_commands =
        footer_spec != NULL && footer_spec->commands != NULL
            ? footer_spec->commands
            : (scrollable ? default_scroll_commands : default_close_commands);
    effective_footer_count = footer_spec != NULL && footer_spec->commands != NULL
                                 ? footer_spec->command_count
                                 : (scrollable ? sizeof(help_popup_scroll_commands) /
                                                     sizeof(help_popup_scroll_commands[0])
                                               : sizeof(help_popup_close_commands) /
                                                     sizeof(help_popup_close_commands[0]));
    footer_width =
        HelpPopupFooterWidth(effective_footer_commands, effective_footer_count);
    if (footer_width + 4 > width)
      width = MINIMUM(footer_width + 4, COLS - 4);

    height = visible_rows + 3;
    height = MAXIMUM(height, 6);
    height = MINIMUM(height, LINES - 2);
    win_x = MAXIMUM(1, (COLS - width) / 2);
    win_y = MAXIMUM(1, (LINES - height) / 2);
  }

  visible_rows = height - 3;
  if (visible_rows < 1)
    visible_rows = 1;
  scrollable = ((int)row_count > visible_rows);
  effective_footer_commands = footer_spec != NULL && footer_spec->commands != NULL
                                  ? footer_spec->commands
                                  : (scrollable ? default_scroll_commands
                                                : default_close_commands);
  effective_footer_count = footer_spec != NULL && footer_spec->commands != NULL
                               ? footer_spec->command_count
                               : (scrollable ? sizeof(help_popup_scroll_commands) /
                                                   sizeof(help_popup_scroll_commands[0])
                                             : sizeof(help_popup_close_commands) /
                                                   sizeof(help_popup_close_commands[0]));
  footer_width =
      HelpPopupFooterWidth(effective_footer_commands, effective_footer_count);
  content_width = width - 4;

  win = newwin(height, width, win_y, win_x);
  if (win == NULL)
    return -1;

  UI_Dialog_Push(win, UI_TIER_MODAL);
  keypad(win, TRUE);
  WbkgdSet(ctx, win, COLOR_PAIR(UI_ROLE_HELP));
  curs_set(0);

  while (1) {
    int ch;

    effective_footer_commands =
        footer_spec != NULL && footer_spec->commands != NULL
            ? footer_spec->commands
            : (scrollable ? default_scroll_commands : default_close_commands);
    effective_footer_count = footer_spec != NULL && footer_spec->commands != NULL
                                 ? footer_spec->command_count
                                 : (scrollable ? sizeof(help_popup_scroll_commands) /
                                                     sizeof(help_popup_scroll_commands[0])
                                               : sizeof(help_popup_close_commands) /
                                                     sizeof(help_popup_close_commands[0]));

    werase(win);
#ifdef COLOR_SUPPORT
    wattron(win, COLOR_PAIR(UI_ROLE_BOX_LINES));
#endif
    box(win, 0, 0);
#ifdef COLOR_SUPPORT
    wattroff(win, COLOR_PAIR(UI_ROLE_BOX_LINES));
#endif
    mvwprintw(win, 1, MAXIMUM(2, (width - StrVisualLength(title)) / 2), "%s",
              title);

    for (i = 0; i < visible_rows && scroll_offset + i < (int)row_count; ++i)
      RenderHelpPopupRow(win, 2 + i, content_width, &rows[scroll_offset + i]);

    RenderHelpPopupFooter(win, height - 1,
                          MAXIMUM(2, (width - footer_width) / 2),
                          effective_footer_commands, effective_footer_count,
                          footer_spec);
    wrefresh(win);

    ch = WGetch(ctx, win);
    if (ch == ERR)
      continue;

    if (footer_spec != NULL && footer_spec->key_handler != NULL) {
      int handled = footer_spec->key_handler(ctx, ch, footer_spec->key_data);

      if (handled > 0)
        break;
      if (handled < 0)
        continue;
    }

    if (ch == KEY_F(1) || ch == ESC || ch == CR || ch == LF || ch == 'q' ||
        ch == 'Q')
      break;
    if (dismiss_any_key)
      break;

    if (!scrollable)
      continue;

    switch (ch) {
    case KEY_UP:
      if (scroll_offset > 0)
        scroll_offset--;
      break;
    case KEY_DOWN:
      if (scroll_offset + visible_rows < (int)row_count)
        scroll_offset++;
      break;
    case KEY_PPAGE:
      scroll_offset -= visible_rows;
      if (scroll_offset < 0)
        scroll_offset = 0;
      break;
    case KEY_NPAGE:
      scroll_offset += visible_rows;
      if (scroll_offset + visible_rows > (int)row_count)
        scroll_offset = (int)row_count - visible_rows;
      if (scroll_offset < 0)
        scroll_offset = 0;
      break;
    case KEY_HOME:
      scroll_offset = 0;
      break;
    case KEY_END:
      scroll_offset = (int)row_count - visible_rows;
      if (scroll_offset < 0)
        scroll_offset = 0;
      break;
    default:
      break;
    }
  }

  UI_Dialog_Close(ctx, win);
  return 0;
}

int UI_ShowHelpPopupWithFooter(ViewContext *ctx, const char *title,
                               const UIHelpPopupRow *rows, size_t row_count,
                               const UIHelpPopupFooterSpec *footer_spec) {
  return ShowHelpPopupInternal(ctx, title, rows, row_count, FALSE,
                               footer_spec);
}

int UI_ShowHelpPopup(ViewContext *ctx, const char *title,
                     const UIHelpPopupRow *rows, size_t row_count) {
  return ShowHelpPopupInternal(ctx, title, rows, row_count, FALSE, NULL);
}

int UI_ShowHelpPopupDismissAnyKey(ViewContext *ctx, const char *title,
                                  const UIHelpPopupRow *rows,
                                  size_t row_count) {
  return ShowHelpPopupInternal(ctx, title, rows, row_count, TRUE, NULL);
}
