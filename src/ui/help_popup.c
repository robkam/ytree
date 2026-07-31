/***************************************************************************
 *
 * src/ui/help_popup.c
 * Shared contextual help popup rendering.
 *
 ***************************************************************************/

#include "ytnova_ui.h"

#define HELP_POPUP_HISTORY_MIN_WIDTH 48
#define HELP_POPUP_HISTORY_VERTICAL_MARGIN 6
#define HELP_POPUP_MIN_HEIGHT 7
#define HELP_POPUP_CENTERED_HORIZONTAL_MARGIN 4

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

static int HelpPopupVisibleRowCapacity(int height) {
  int content_lines = height - 5;

  if (content_lines < 1)
    return 1;
  return (content_lines + 1) / 2;
}

static int HelpPopupHeightForVisibleRows(int visible_rows) {
  if (visible_rows < 1)
    visible_rows = 1;
  return (visible_rows * 2) + 5;
}

static void HelpPopupResolveFooterCommands(
    BOOL scrollable, const UIHelpPopupFooterSpec *footer_spec,
    const UICommandStripCommand **commands_out, size_t *command_count_out) {
  if (commands_out == NULL || command_count_out == NULL)
    return;

  if (footer_spec != NULL && footer_spec->commands != NULL) {
    *commands_out = footer_spec->commands;
    *command_count_out = footer_spec->command_count;
    return;
  }

  *commands_out = scrollable ? help_popup_scroll_commands
                             : help_popup_close_commands;
  *command_count_out =
      scrollable ? sizeof(help_popup_scroll_commands) /
                       sizeof(help_popup_scroll_commands[0])
                 : sizeof(help_popup_close_commands) /
                       sizeof(help_popup_close_commands[0]);
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

static void SyncHelpPopupActiveRowScroll(
    const UIHelpPopupFooterSpec *footer_spec, int visible_rows, int row_count,
    int *scroll_offset) {
  int active_row;

  if (footer_spec == NULL || footer_spec->active_row_handler == NULL ||
      scroll_offset == NULL || visible_rows <= 0 || row_count <= 0)
    return;

  active_row = footer_spec->active_row_handler(footer_spec->key_data);
  if (active_row < 0 || active_row >= row_count)
    return;

  if (active_row < *scroll_offset)
    *scroll_offset = active_row;
  else if (active_row >= *scroll_offset + visible_rows)
    *scroll_offset = active_row - visible_rows + 1;

  if (*scroll_offset < 0)
    *scroll_offset = 0;
  if (*scroll_offset + visible_rows > row_count)
    *scroll_offset = row_count - visible_rows;
  if (*scroll_offset < 0)
    *scroll_offset = 0;
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
  case UI_HELP_POPUP_LINK_TEXT:
    if (row->prefix != NULL)
      width += StrVisualLength(row->prefix);
    if (row->text != NULL && row->text[0] != '\0')
      width += 2 + StrVisualLength(row->text);
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
  int max_text;

  if (win == NULL || row == NULL)
    return;

  max_text = content_width;
  if (max_text < 0)
    max_text = 0;

  if (row->prefix != NULL && row->prefix[0] != '\0') {
    if (row->kind == UI_HELP_POPUP_LINK_TEXT) {
      wattrset(win, COLOR_PAIR(row->selected ? UI_ROLE_HELP_LINK_SELECTION
                                             : UI_ROLE_HELP_LINK));
      mvwprintw(win, y, x, "%s", row->prefix);
      wattrset(win, COLOR_PAIR(UI_ROLE_HELP));
    } else {
      PrintSpecialString(win, y, x, (char *)row->prefix, UI_ROLE_HELP);
    }
    x += StrVisualLength(row->prefix);
  }

  switch (row->kind) {
  case UI_HELP_POPUP_COMMAND_STRIP:
    UI_RenderCommandStrip(win, y, x, row->commands, row->command_count,
                          UI_ROLE_HELP, UI_ROLE_HELP_KEYBIND);
    break;
  case UI_HELP_POPUP_LINK_TEXT:
    if (row->text != NULL && row->text[0] != '\0' && x < content_width + 2) {
      mvwprintw(win, y, x, ": ");
      x += 2;
      if (x < content_width + 2)
        mvwprintw(win, y, x, "%.*s", content_width - (x - 2), row->text);
    }
    break;
  case UI_HELP_POPUP_TEXT:
  default:
    if (row->text != NULL)
      mvwprintw(win, y, x, "%.*s", max_text, row->text);
    break;
  }
}

static int ShowHelpPopupInternal(ViewContext *ctx, const char *title,
                                 const UIHelpPopupRow *rows, size_t row_count,
                                 BOOL dismiss_any_key,
                                 const UIHelpPopupFooterSpec *footer_spec) {
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
      ctx->layout.main_win_width >= HELP_POPUP_HISTORY_MIN_WIDTH &&
      (LINES - HELP_POPUP_HISTORY_VERTICAL_MARGIN) >=
          HELP_POPUP_HISTORY_VERTICAL_MARGIN;
  if (use_history_geometry) {
    width = MINIMUM(ctx->layout.main_win_width, COLS - 2);
    height = MINIMUM(LINES - HELP_POPUP_HISTORY_VERTICAL_MARGIN, LINES - 2);
    if ((height % 2) == 0)
      height--;
    if (height < HELP_POPUP_MIN_HEIGHT)
      height = HELP_POPUP_MIN_HEIGHT;
    win_x = 1;
    win_y = 2;
  } else {
    width = StrVisualLength(title) + 8;
    if (max_row_width + 4 > width)
      width = max_row_width + 4;
    width = MAXIMUM(width, HELP_POPUP_HISTORY_MIN_WIDTH);
    width = MINIMUM(width, COLS - HELP_POPUP_CENTERED_HORIZONTAL_MARGIN);

    max_visible_rows = HelpPopupVisibleRowCapacity(LINES - 2);
    visible_rows = (int)row_count;
    if (visible_rows > max_visible_rows)
      visible_rows = max_visible_rows;

    scrollable = ((int)row_count > visible_rows);
    HelpPopupResolveFooterCommands(scrollable, footer_spec,
                                   &effective_footer_commands,
                                   &effective_footer_count);
    footer_width =
        HelpPopupFooterWidth(effective_footer_commands, effective_footer_count);
    if (footer_width + 4 > width)
      width = MINIMUM(footer_width + 4,
                      COLS - HELP_POPUP_CENTERED_HORIZONTAL_MARGIN);

    height = HelpPopupHeightForVisibleRows(visible_rows);
    height = MAXIMUM(height, HELP_POPUP_MIN_HEIGHT);
    height = MINIMUM(height, LINES - 2);
    win_x = MAXIMUM(1, (COLS - width) / 2);
    win_y = MAXIMUM(1, (LINES - height) / 2);
  }

  visible_rows = HelpPopupVisibleRowCapacity(height);
  scrollable = ((int)row_count > visible_rows);
  HelpPopupResolveFooterCommands(scrollable, footer_spec,
                                 &effective_footer_commands,
                                 &effective_footer_count);
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

    HelpPopupResolveFooterCommands(scrollable, footer_spec,
                                   &effective_footer_commands,
                                   &effective_footer_count);
    SyncHelpPopupActiveRowScroll(footer_spec, visible_rows, (int)row_count,
                                 &scroll_offset);

    werase(win);
#ifdef COLOR_SUPPORT
    wattron(win, COLOR_PAIR(UI_ROLE_HELP_BOX_LINES));
#endif
    box(win, 0, 0);
#ifdef COLOR_SUPPORT
    wattroff(win, COLOR_PAIR(UI_ROLE_HELP_BOX_LINES));
#endif
    mvwprintw(win, 1, MAXIMUM(2, (width - StrVisualLength(title)) / 2), "%s",
              title);

    for (i = 0; i < visible_rows && scroll_offset + i < (int)row_count; ++i)
      RenderHelpPopupRow(win, 3 + (i * 2), content_width,
                         &rows[scroll_offset + i]);

    RenderHelpPopupFooter(win, height - 2, 2, effective_footer_commands,
                          effective_footer_count, footer_spec);
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
