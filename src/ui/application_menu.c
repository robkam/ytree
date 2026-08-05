/***************************************************************************
 *
 * src/ui/application_menu.c
 * Minimal Applications Menu UI
 *
 ***************************************************************************/

#include "ytnova_ui.h"
#include "ytnova_appstate_modal.h"
#include "ytnova_appstate_render.h"

typedef struct {
  const char *label;
} ApplicationMenuEntry;

static const ApplicationMenuEntry kApplicationMenuEntries[] = {
    {"wget fetch preset"},
    {"ssh connect preset"},
    {"format convert preset"},
};

static const UICommandStripCommand applications_menu_commands[] = {
    {UI_COMMAND_LAYOUT_LABEL_FIRST, "Select", "Up", "Down"},
    {UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Close", "Enter", "Esc"},
};

static int ShowApplicationsHelpPopup(ViewContext *ctx) {
  if (ctx == NULL)
    return -1;

  return UI_ShowGeneratedContextHelp(ctx, "dialog.applications", NULL, 0);
}

static void PaintApplicationRow(const ViewContext *ctx, WINDOW *win, int y_pos,
                                int win_width, char *item_text,
                                BOOL selected) {
  chtype base_attr;
  chtype item_attr;

#ifdef COLOR_SUPPORT
  if (ctx->color_enabled) {
    base_attr = COLOR_PAIR(UI_ROLE_PICKER);
    item_attr =
        selected ? UISelectionAttrForBase(ctx, UI_ROLE_PICKER) : base_attr;
  } else {
    base_attr = 0;
    item_attr = selected ? A_REVERSE : 0;
  }
#else
  (void)ctx;
  base_attr = 0;
  item_attr = selected ? A_REVERSE : 0;
#endif

  wattrset(win, base_attr);
  mvwhline(win, y_pos, 1, ' ', win_width - 2);
  wmove(win, y_pos, 2);
  wattrset(win, item_attr);
  WAddStr(win, item_text);
  wattrset(win, base_attr);
}

int UI_OpenApplicationsMenu(ViewContext *ctx) {
  WINDOW *win = NULL;
  const DirEntry *dir_entry = NULL;
  size_t i;
  int selected_index = 0;
  int result = -1;
  int prompt_width;
  int win_height;
  int win_width;
  int win_x;
  int win_y;
  int ch;
  BOOL menu_active;
  BOOL restart_menu;
  const char title[] = "Applications";

  if (ctx == NULL)
    return -1;

  ClearHelp(ctx);

  do {
    int max_label_len = 0;
    int visible_lines;

    restart_menu = FALSE;
    menu_active = TRUE;
    prompt_width = UI_CommandStripVisualLength(
        applications_menu_commands,
        sizeof(applications_menu_commands) / sizeof(applications_menu_commands[0]));

    for (i = 0; i < sizeof(kApplicationMenuEntries) / sizeof(kApplicationMenuEntries[0]);
         i++) {
      int len = StrVisualLength(kApplicationMenuEntries[i].label);
      if (len > max_label_len)
        max_label_len = len;
    }

    win_width = MAXIMUM((int)(strlen(title) + 4), max_label_len + 6);
    win_width = MAXIMUM(win_width, prompt_width + 4);
    win_width = MINIMUM(win_width, COLS - ctx->layout.stats_width - 2);

    win_height = (int)(sizeof(kApplicationMenuEntries) /
                       sizeof(kApplicationMenuEntries[0])) +
                 5;
    win_height = MAXIMUM(win_height, 8);
    win_height = MINIMUM(win_height, ctx->layout.bottom_border_y);

    win_x = ((COLS - ctx->layout.stats_width) - win_width) / 2;
    if (win_x < 1)
      win_x = 1;
    win_y = (LINES - win_height) / 2;

    visible_lines = win_height - 5;
    visible_lines = MAXIMUM(1, visible_lines);
    if (selected_index >= (int)(sizeof(kApplicationMenuEntries) /
                                sizeof(kApplicationMenuEntries[0]))) {
      selected_index = (int)(sizeof(kApplicationMenuEntries) /
                             sizeof(kApplicationMenuEntries[0])) -
                       1;
    }
    if (selected_index < 0)
      selected_index = 0;

    win = newwin(win_height, win_width, win_y, win_x);
    if (win == NULL)
      return -1;

    UI_Dialog_Push(win, UI_TIER_MODAL);
    keypad(win, TRUE);
    WbkgdSet(ctx, win, COLOR_PAIR(UI_ROLE_PICKER));
    curs_set(0);

    while (menu_active) {
      werase(win);
#ifdef COLOR_SUPPORT
      wattron(win, COLOR_PAIR(UI_ROLE_PICKER));
#endif
      box(win, 0, 0);
#ifdef COLOR_SUPPORT
      wattroff(win, COLOR_PAIR(UI_ROLE_PICKER));
#endif
      mvwprintw(win, 1, (win_width - (int)strlen(title)) / 2, "%s", title);
      UI_RenderCommandStrip(
          win, win_height - 2, (win_width - prompt_width) / 2,
          applications_menu_commands,
          sizeof(applications_menu_commands) /
              sizeof(applications_menu_commands[0]),
          UI_ROLE_PICKER, UI_ROLE_KEYBIND);

      for (i = 0; i < sizeof(kApplicationMenuEntries) / sizeof(kApplicationMenuEntries[0]) &&
                  (int)i < visible_lines;
           i++) {
        char item_buf[PATH_LENGTH + 1];
        int max_w = win_width - 4;

        (void)snprintf(item_buf, sizeof(item_buf), "%s",
                       kApplicationMenuEntries[i].label);
        if ((int)strlen(item_buf) > max_w)
          item_buf[max_w] = '\0';
        PaintApplicationRow(ctx, win, 3 + (int)i, win_width, item_buf,
                            (int)i == selected_index);
      }

      wrefresh(win);
      ch = WGetch(ctx, win);

      if (ctx->resize_request) {
        (void)AppStateClearResizeRequest(ctx);
        ReCreateWindows(ctx);
        DisplayMenu(ctx);
        restart_menu = TRUE;
        break;
      }

      switch (ch) {
      case KEY_F(1):
        (void)ShowApplicationsHelpPopup(ctx);
        break;
      case KEY_UP:
        selected_index--;
        if (selected_index < 0) {
          selected_index =
              (int)(sizeof(kApplicationMenuEntries) /
                    sizeof(kApplicationMenuEntries[0])) -
              1;
        }
        break;
      case KEY_DOWN:
        selected_index++;
        if (selected_index >=
            (int)(sizeof(kApplicationMenuEntries) /
                  sizeof(kApplicationMenuEntries[0]))) {
          selected_index = 0;
        }
        break;
      case LF:
      case CR:
      case ESC:
      case 'q':
      case 'Q':
        menu_active = FALSE;
        result = 0;
        break;
      default:
        break;
      }
    }

    if (win != NULL) {
      UI_Dialog_Close(ctx, win);
      win = NULL;
    }
    if (!restart_menu && ctx->active != NULL && ctx->active->vol != NULL) {
      dir_entry = GetSelectedDirEntry(ctx, ctx->active->vol);
      if (dir_entry != NULL)
        RefreshView(ctx, GetSelectedDirEntry(ctx, ctx->active->vol));
    }
  } while (restart_menu);

  curs_set(1);
  return result;
}
