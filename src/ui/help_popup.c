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
  int content_lines = height - 6;

  if (content_lines < 1)
    return 1;
  return (content_lines + 1) / 2;
}

static int HelpPopupContentLineCapacity(int height) {
  int content_lines = height - 6;

  if (content_lines < 1)
    return 1;
  return content_lines;
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

static void FillHelpPopupBlankLine(WINDOW *win, int y, int start_x, int width,
                                   int role) {
  int x;

  if (win == NULL || width <= 0)
    return;

  for (x = 0; x < width; ++x)
    mvwaddch(win, y, start_x + x, ' ' | COLOR_PAIR(role));
}

static void RenderHelpPopupFooter(
    WINDOW *win, int y, int start_x, const UICommandStripCommand *commands,
    size_t command_count, const UIHelpPopupFooterSpec *footer_spec) {
  int footer_width;

  if (win == NULL)
    return;

  (void)footer_spec;
  footer_width = getmaxx(win) - start_x - 2;
  if (footer_width > 0)
    FillHelpPopupBlankLine(win, y, start_x, footer_width,
                           UI_ROLE_HELP_FOOTER);

  if (commands != NULL && command_count > 0) {
    UI_RenderCommandStrip(win, y, start_x, commands, command_count,
                          UI_ROLE_HELP_FOOTER, UI_ROLE_HELP_KEYBIND);
  }
}

static void ClearHelpPopupContentArea(WINDOW *win, int start_y, int start_x,
                                      int line_count, int width) {
  int y;

  if (win == NULL || line_count <= 0)
    return;

  for (y = 0; y < line_count; ++y)
    FillHelpPopupBlankLine(win, start_y + y, start_x, width, UI_ROLE_HELP);
}

static void RenderHelpPopupFrame(WINDOW *win, int width, const char *title) {
  if (win == NULL || title == NULL)
    return;

  werase(win);
#ifdef COLOR_SUPPORT
  wattron(win, COLOR_PAIR(UI_ROLE_HELP_BOX_LINES));
#endif
  box(win, 0, 0);
#ifdef COLOR_SUPPORT
  wattroff(win, COLOR_PAIR(UI_ROLE_HELP_BOX_LINES));
  wattron(win, COLOR_PAIR(UI_ROLE_HELP_HEADING));
#endif
  mvwprintw(win, 1, MAXIMUM(2, (width - StrVisualLength(title)) / 2), "%s",
            title);
#ifdef COLOR_SUPPORT
  wattroff(win, COLOR_PAIR(UI_ROLE_HELP_HEADING));
#endif
}

static void RenderHelpInlineText(WINDOW *win, int y, int column, int max_width,
                                 const char *text,
                                 UISemanticRolePair base_role) {
  BOOL attention = FALSE;
  int used_width = 0;

  if (win == NULL || text == NULL || max_width <= 0)
    return;

  while (*text != '\0' && used_width < max_width) {
    size_t len = 0;
    int available = max_width - used_width;

    if (text[0] == '*' && text[1] == '*') {
      attention = !attention;
      text += 2;
      continue;
    }

    while (text[len] != '\0' && !(text[len] == '*' && text[len + 1] == '*'))
      ++len;
    if (len == 0)
      break;
    if ((int)len > available)
      len = (size_t)available;

    wattrset(win, COLOR_PAIR(attention ? UI_ROLE_HELP_ATTENTION : base_role));
    mvwprintw(win, y, column, "%.*s", (int)len, text);
    column += (int)len;
    used_width += (int)len;
    text += len;
  }

  wattrset(win, COLOR_PAIR(UI_ROLE_HELP));
}

static void SyncHelpPopupActiveRowScroll(
    const UIHelpPopupFooterSpec *footer_spec, int visible_rows,
    int content_lines, int row_count, int *scroll_row_offset,
    int *scroll_line_phase) {
  int active_row;
  int effective_visible_rows;
  int line_scroll;
  int max_line_scroll;

  if (footer_spec == NULL || footer_spec->active_row_handler == NULL ||
      scroll_row_offset == NULL || scroll_line_phase == NULL ||
      visible_rows <= 0 || content_lines <= 0 || row_count <= 0)
    return;

  active_row = footer_spec->active_row_handler(footer_spec->key_data);
  effective_visible_rows = visible_rows - (*scroll_line_phase != 0 ? 1 : 0);
  if (effective_visible_rows < 1)
    effective_visible_rows = 1;

  line_scroll = (*scroll_row_offset * 2) + (*scroll_line_phase != 0 ? 1 : 0);
  max_line_scroll = ((row_count * 2) - 1) - content_lines;
  if (max_line_scroll < 0)
    max_line_scroll = 0;

  if (active_row >= 0 && active_row < row_count) {
    int visible_start_row =
        *scroll_row_offset + (*scroll_line_phase != 0 ? 1 : 0);

    if (active_row < visible_start_row) {
      *scroll_row_offset = active_row;
      *scroll_line_phase = 0;
    } else if (active_row >= visible_start_row + effective_visible_rows) {
      *scroll_row_offset = active_row - effective_visible_rows + 1;
      *scroll_line_phase = 0;
    }
    line_scroll =
        (*scroll_row_offset * 2) + (*scroll_line_phase != 0 ? 1 : 0);
  }

  if (line_scroll < 0)
    line_scroll = 0;
  if (line_scroll > max_line_scroll)
    line_scroll = max_line_scroll;
  *scroll_row_offset = line_scroll / 2;
  *scroll_line_phase = line_scroll % 2;
}

static void UpdateHelpPopupViewport(const UIHelpPopupFooterSpec *footer_spec,
                                    int scroll_row_offset,
                                    int scroll_line_phase, int visible_rows,
                                    int row_count) {
  int effective_visible_rows;

  if (footer_spec == NULL || footer_spec->viewport_handler == NULL)
    return;

  effective_visible_rows = visible_rows - (scroll_line_phase != 0 ? 1 : 0);
  if (effective_visible_rows < 1)
    effective_visible_rows = 1;
  {
    int visible_start_row = scroll_row_offset + (scroll_line_phase != 0 ? 1 : 0);

    if (visible_start_row < 0)
      visible_start_row = 0;
    if (visible_start_row > row_count)
      visible_start_row = row_count;
    if (visible_start_row + effective_visible_rows > row_count)
      effective_visible_rows = row_count - visible_start_row;
    if (effective_visible_rows < 0)
      effective_visible_rows = 0;

    footer_spec->viewport_handler(footer_spec->key_data, visible_start_row,
                                  effective_visible_rows, row_count);
  }
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
  case UI_HELP_POPUP_TEXT:
    if (row->prefix != NULL && row->prefix[0] != '\0' &&
        row->text != NULL && row->text[0] != '\0')
      width += 2;
    if (row->text != NULL && row->text[0] != '\0')
      width += StrVisualLength(row->text);
    break;
  default:
    break;
  }

  return width;
}

#define HELP_POPUP_CONTENT_START_Y 3
#define HELP_POPUP_CONTENT_START_X 2

static void RenderHelpPopupRow(WINDOW *win, int y, int start_x,
                               int content_width, const UIHelpPopupRow *row) {
  int x = start_x;
  UISemanticRolePair text_role = UI_ROLE_HELP;

  if (win == NULL || row == NULL)
    return;

  if (row->prefix != NULL && row->prefix[0] != '\0') {
    if (row->kind == UI_HELP_POPUP_LINK_TEXT) {
      wattrset(win, COLOR_PAIR(row->selected ? UI_ROLE_HELP_LINK_SELECTION
                                             : UI_ROLE_HELP_LINK));
      mvwprintw(win, y, x, "%s", row->prefix);
      wattrset(win, COLOR_PAIR(UI_ROLE_HELP));
    } else {
      wattrset(win, COLOR_PAIR(UI_ROLE_HELP_TERM));
      mvwprintw(win, y, x, "%s", row->prefix);
      wattrset(win, COLOR_PAIR(UI_ROLE_HELP));
    }
    x += StrVisualLength(row->prefix);
  }

  switch (row->kind) {
  case UI_HELP_POPUP_COMMAND_STRIP:
    UI_RenderCommandStrip(win, y, x, row->commands, row->command_count,
                          UI_ROLE_HELP, UI_ROLE_HELP_KEYBIND);
    break;
  case UI_HELP_POPUP_LINK_TEXT:
  case UI_HELP_POPUP_TEXT:
    if (row->text != NULL && row->text[0] != '\0' &&
        x < start_x + content_width) {
      if (row->prefix != NULL && row->prefix[0] != '\0') {
        mvwprintw(win, y, x, ": ");
        x += 2;
      }
      if (x < start_x + content_width)
        RenderHelpInlineText(win, y, x, content_width - (x - start_x),
                             row->text, text_role);
      wattrset(win, COLOR_PAIR(UI_ROLE_HELP));
    }
    break;
  default:
    break;
  }
}

typedef struct {
  WINDOW *win;
  int height;
  int content_lines;
  int content_width;
  int visible_rows;
  const UIHelpPopupRow *rows;
  size_t row_count;
  const UIHelpPopupFooterSpec *footer_spec;
  const UICommandStripCommand *effective_footer_commands;
  size_t effective_footer_count;
  int *scroll_row_offset;
  int *scroll_line_phase;
} HelpPopupRenderState;

static void RenderHelpPopupPage(const HelpPopupRenderState *state) {
  int active_row = -1;
  int render_visible_rows;
  int i;

  if (state == NULL || state->win == NULL || state->rows == NULL ||
      state->row_count == 0 || state->scroll_row_offset == NULL ||
      state->scroll_line_phase == NULL)
    return;

  UpdateHelpPopupViewport(state->footer_spec, *state->scroll_row_offset,
                          *state->scroll_line_phase, state->visible_rows,
                          (int)state->row_count);
  SyncHelpPopupActiveRowScroll(state->footer_spec, state->visible_rows,
                               state->content_lines, (int)state->row_count,
                               state->scroll_row_offset,
                               state->scroll_line_phase);
  UpdateHelpPopupViewport(state->footer_spec, *state->scroll_row_offset,
                          *state->scroll_line_phase, state->visible_rows,
                          (int)state->row_count);
  if (state->footer_spec != NULL &&
      state->footer_spec->active_row_handler != NULL) {
    active_row =
        state->footer_spec->active_row_handler(state->footer_spec->key_data);
  }

  ClearHelpPopupContentArea(state->win, HELP_POPUP_CONTENT_START_Y,
                            HELP_POPUP_CONTENT_START_X, state->content_lines + 1,
                            state->content_width);
  render_visible_rows =
      state->visible_rows - (*state->scroll_line_phase != 0 ? 1 : 0);
  if (render_visible_rows < 1)
    render_visible_rows = 1;

  for (i = 0; i < render_visible_rows &&
              *state->scroll_row_offset + *state->scroll_line_phase + i <
                  (int)state->row_count;
       ++i) {
    UIHelpPopupRow render_row =
        state->rows[*state->scroll_row_offset + *state->scroll_line_phase + i];

    if (state->footer_spec != NULL &&
        state->footer_spec->active_row_handler != NULL) {
      render_row.selected =
          (*state->scroll_row_offset + *state->scroll_line_phase + i ==
           active_row);
    }
    RenderHelpPopupRow(state->win,
                       HELP_POPUP_CONTENT_START_Y + *state->scroll_line_phase +
                           (i * 2),
                       HELP_POPUP_CONTENT_START_X,
                       state->content_width, &render_row);
  }

  RenderHelpPopupFooter(state->win, state->height - 2, 2,
                        state->effective_footer_commands,
                        state->effective_footer_count, state->footer_spec);
  wredrawln(state->win, HELP_POPUP_CONTENT_START_Y,
            state->content_lines + 1);
  wredrawln(state->win, state->height - 2, 1);
  wrefresh(state->win);
}

static void ScrollHelpPopupLine(int delta, int row_count, int content_lines,
                                int *scroll_row_offset,
                                int *scroll_line_phase) {
  int line_scroll;
  int max_line_scroll;

  if (scroll_row_offset == NULL || scroll_line_phase == NULL || row_count <= 0 ||
      content_lines <= 0 || delta == 0)
    return;

  line_scroll = (*scroll_row_offset * 2) + *scroll_line_phase + delta;
  max_line_scroll = (row_count * 2) - 1 - content_lines;
  if (max_line_scroll < 0)
    max_line_scroll = 0;
  if (line_scroll < 0)
    line_scroll = 0;
  if (line_scroll > max_line_scroll)
    line_scroll = max_line_scroll;
  *scroll_row_offset = line_scroll / 2;
  *scroll_line_phase = line_scroll % 2;
}

static void HandleHelpPopupScrollKey(int ch, int row_count, int content_lines,
                                     int *scroll_row_offset,
                                     int *scroll_line_phase) {
  int page_delta;

  if (scroll_row_offset == NULL || scroll_line_phase == NULL)
    return;

  switch (ch) {
  case KEY_UP:
    ScrollHelpPopupLine(-1, row_count, content_lines, scroll_row_offset,
                        scroll_line_phase);
    break;
  case KEY_DOWN:
    ScrollHelpPopupLine(1, row_count, content_lines, scroll_row_offset,
                        scroll_line_phase);
    break;
  case KEY_PPAGE:
  case KEY_NPAGE:
    page_delta = (ch == KEY_PPAGE) ? -content_lines : content_lines;
    ScrollHelpPopupLine(page_delta, row_count, content_lines, scroll_row_offset,
                        scroll_line_phase);
    break;
  case KEY_HOME:
    *scroll_row_offset = 0;
    *scroll_line_phase = 0;
    break;
  case KEY_END:
    ScrollHelpPopupLine(row_count * 2, row_count, content_lines,
                        scroll_row_offset, scroll_line_phase);
    break;
  default:
    break;
  }
}

static int ShowHelpPopupInternal(ViewContext *ctx, const char *title,
                                 const UIHelpPopupRow *rows, size_t row_count,
                                 BOOL dismiss_any_key,
                                 const UIHelpPopupFooterSpec *footer_spec) {
  HelpPopupRenderState render_state;
  WINDOW *win;
  int content_width;
  const UICommandStripCommand *effective_footer_commands;
  size_t effective_footer_count;
  int height;
  int i;
  int max_row_width;
  int scroll_line_phase = 0;
  int scroll_row_offset = 0;
  BOOL scrollable;
  int content_lines;
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
    int footer_width;
    int max_visible_rows;

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
  content_lines = HelpPopupContentLineCapacity(height);
  scrollable = (((int)row_count * 2) - 1 > content_lines);
  HelpPopupResolveFooterCommands(scrollable, footer_spec,
                                 &effective_footer_commands,
                                 &effective_footer_count);
  content_width = width - 4;

  win = newwin(height, width, win_y, win_x);
  if (win == NULL)
    return -1;

  UI_Dialog_Push(win, UI_TIER_MODAL);
  keypad(win, TRUE);
  idlok(win, FALSE);
  idcok(win, FALSE);
  idlok(stdscr, FALSE);
  idcok(stdscr, FALSE);
  WbkgdSet(ctx, win, COLOR_PAIR(UI_ROLE_HELP));
  curs_set(0);

  wattron(win, COLOR_PAIR(UI_ROLE_HELP_BOX_LINES));
  RenderHelpPopupFrame(win, width, title);
  wattroff(win, COLOR_PAIR(UI_ROLE_HELP_BOX_LINES));
  ClearHelpPopupContentArea(win, HELP_POPUP_CONTENT_START_Y,
                            HELP_POPUP_CONTENT_START_X, content_lines + 1,
                            content_width);
  RenderHelpPopupFooter(win, height - 2, HELP_POPUP_CONTENT_START_X,
                        effective_footer_commands,
                        effective_footer_count, footer_spec);
  wrefresh(win);

  memset(&render_state, 0, sizeof(render_state));
  render_state.win = win;
  render_state.height = height;
  render_state.content_lines = content_lines;
  render_state.content_width = content_width;
  render_state.visible_rows = visible_rows;
  render_state.rows = rows;
  render_state.row_count = row_count;
  render_state.footer_spec = footer_spec;
  render_state.scroll_row_offset = &scroll_row_offset;
  render_state.scroll_line_phase = &scroll_line_phase;

  while (1) {
    int ch;

    HelpPopupResolveFooterCommands(scrollable, footer_spec,
                                   &effective_footer_commands,
                                   &effective_footer_count);
    render_state.effective_footer_commands = effective_footer_commands;
    render_state.effective_footer_count = effective_footer_count;
    RenderHelpPopupPage(&render_state);

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
    HandleHelpPopupScrollKey(ch, (int)row_count, content_lines,
                             &scroll_row_offset, &scroll_line_phase);
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
