/***************************************************************************
 *
 * src/ui/application_menu.c
 * Applications Menu UI and catalog execution helpers.
 *
 ***************************************************************************/

#include "../core/default_applications_catalog.h"
#include "interactions_panel_paths.h"
#include "ytnova_appstate_focus.h"
#include "ytnova_appstate_modal.h"
#include "ytnova_appstate_render.h"
#include "ytnova_cmd.h"
#include "ytnova_fs.h"
#include "ytnova_ui.h"
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

enum {
  APPLICATIONS_MENU_COMMAND_STRIP_X = 2,
  APPLICATIONS_MENU_MAX_ENTRIES = 64,
  APPLICATIONS_MENU_FIELD_MAX = PATH_LENGTH,
  APPLICATIONS_MENU_PARSE_LINE_MAX = COMMAND_LINE_LENGTH + 256,
  APPLICATIONS_MENU_EXEC_STAY = 0,
  APPLICATIONS_MENU_EXEC_CLOSE = 1
};

typedef struct {
  char label[APPLICATIONS_MENU_FIELD_MAX + 1];
  char prompt[APPLICATIONS_MENU_FIELD_MAX + 1];
  char command[COMMAND_LINE_LENGTH + 1];
} ApplicationMenuEntry;

static const UICommandStripCommand applications_menu_commands[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "help", "F1", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "select", "Enter", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "edit", "E", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "cancel", "Esc", NULL},
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

static char *TrimApplicationField(char *text) {
  char *start;
  char *end;

  if (text == NULL)
    return NULL;

  start = text;
  while (*start == ' ' || *start == '\t' || *start == '\r' || *start == '\n')
    start++;

  end = start + strlen(start);
  while (end > start &&
         (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' ||
          end[-1] == '\n')) {
    end--;
  }
  *end = '\0';
  return start;
}

static int ResolveExistingApplicationsPath(char *path, size_t path_size) {
  int result;

  if (path == NULL || path_size == 0)
    return -1;
  path[0] = '\0';

  if (ConfigPaths_ResolvePreferredPath(CONFIG_SURFACE_APPLICATIONS, path,
                                       path_size) == 0) {
    if (access(path, F_OK) == 0)
      return 0;
    if (errno != ENOENT)
      return -1;
  }

  result = ConfigPaths_ResolveLegacyPath(CONFIG_SURFACE_APPLICATIONS, path,
                                         path_size, FALSE);
  if (result == 0) {
    if (access(path, F_OK) == 0)
      return 0;
    if (errno != ENOENT)
      return -1;
  }

  path[0] = '\0';
  return 1;
}

static FILE *OpenApplicationsCatalog(void) {
  FILE *fp;
  char path[PATH_LENGTH + 1];
  int path_result = ResolveExistingApplicationsPath(path, sizeof(path));

  if (path_result == 0)
    return fopen(path, "r");
  if (path_result < 0)
    return NULL;

  fp = fopen(PACKAGED_APPLICATIONS_PATH, "r");
  if (fp != NULL)
    return fp;

  return fmemopen((void *)default_applications_catalog,
                  strlen(default_applications_catalog), "r");
}

static int LoadApplicationsCatalog(ApplicationMenuEntry *entries,
                                   int max_entries) {
  FILE *fp;
  char line[APPLICATIONS_MENU_PARSE_LINE_MAX + 1];
  int count = 0;

  if (entries == NULL || max_entries <= 0)
    return -1;

  fp = OpenApplicationsCatalog();
  if (fp == NULL)
    return -1;

  while (fgets(line, sizeof(line), fp) != NULL) {
    const char *label_field;
    const char *prompt_field;
    const char *command_field;
    char *separator_one;
    char *separator_two;
    char *comment_start = TrimApplicationField(line);

    if (comment_start == NULL || *comment_start == '\0' || *comment_start == '#')
      continue;

    separator_one = strchr(comment_start, '|');
    if (separator_one == NULL)
      continue;
    *separator_one = '\0';
    separator_two = strchr(separator_one + 1, '|');
    if (separator_two == NULL)
      continue;
    *separator_two = '\0';

    label_field = TrimApplicationField(comment_start);
    prompt_field = TrimApplicationField(separator_one + 1);
    command_field = TrimApplicationField(separator_two + 1);
    if (label_field == NULL || command_field == NULL ||
        !String_HasNonWhitespace(label_field) ||
        !String_HasNonWhitespace(command_field)) {
      continue;
    }

    (void)snprintf(entries[count].label, sizeof(entries[count].label), "%s",
                   label_field);
    (void)snprintf(entries[count].prompt, sizeof(entries[count].prompt), "%s",
                   (prompt_field != NULL) ? prompt_field : "");
    (void)snprintf(entries[count].command, sizeof(entries[count].command), "%s",
                   command_field);
    count++;
    if (count >= max_entries)
      break;
  }

  fclose(fp);
  return count;
}

static BOOL ApplicationCommandUsesToken(const char *command_template,
                                        const char *token) {
  if (command_template == NULL || token == NULL || *token == '\0')
    return FALSE;
  return strstr(command_template, token) != NULL ? TRUE : FALSE;
}

static int ResolveApplicationSelectionPath(ViewContext *ctx, char *path) {
  ViewFocus active_focus;

  if (ctx == NULL || ctx->active == NULL || path == NULL)
    return -1;

  active_focus = AppStateResolveActivePanelFocus(ctx);
  if (active_focus == FOCUS_FILE &&
      UI_GetPanelSelectedFilePath(ctx, ctx->active, path) == 0) {
    return 0;
  }
  if (UI_GetPanelSelectedDirPath(ctx, ctx->active, path) == 0)
    return 0;
  if (UI_GetPanelSelectedFilePath(ctx, ctx->active, path) == 0)
    return 0;
  return -1;
}

static int ResolveApplicationLaunchDirectory(ViewContext *ctx, char *path) {
  ViewFocus active_focus;
  char file_path[PATH_LENGTH + 1];
  const char *separator;
  size_t dir_len;

  if (ctx == NULL || ctx->active == NULL || path == NULL)
    return -1;

  path[0] = '\0';
  active_focus = AppStateResolveActivePanelFocus(ctx);
  if (active_focus != FOCUS_FILE &&
      UI_GetPanelSelectedDirPath(ctx, ctx->active, path) == 0) {
    return 0;
  }
  if (UI_GetPanelSelectedFilePath(ctx, ctx->active, file_path) != 0) {
    if (UI_GetPanelSelectedDirPath(ctx, ctx->active, path) == 0)
      return 0;
    return -1;
  }

  separator = strrchr(file_path, FILE_SEPARATOR_CHAR);
  if (separator == NULL)
    return snprintf(path, PATH_LENGTH + 1, ".") >= 0 ? 0 : -1;
  if (separator == file_path)
    return snprintf(path, PATH_LENGTH + 1, "%s", FILE_SEPARATOR_STRING) >= 0
               ? 0
               : -1;

  dir_len = (size_t)(separator - file_path);
  if (dir_len >= PATH_LENGTH + 1)
    return -1;
  memcpy(path, file_path, dir_len);
  path[dir_len] = '\0';
  return 0;
}

static void ReportApplicationLaunchFailure(ViewContext *ctx,
                                           const ApplicationMenuEntry *entry,
                                           int error_code) {
  if (ctx == NULL || entry == NULL)
    return;
  UI_ShowStatusLineError(ctx, "%s could not start: %s.", entry->label,
                         strerror(error_code != 0 ? error_code : EIO));
}

static int ExecuteApplicationEntry(ViewContext *ctx,
                                   const ApplicationMenuEntry *entry) {
  char launch_dir[PATH_LENGTH + 1];
  char selection_path[PATH_LENGTH + 1];
  char prompt_buffer[COMMAND_LINE_LENGTH + 1];
  char command_line[COMMAND_LINE_LENGTH + 1];
  BOOL needs_selection;
  BOOL needs_input;
  Statistic *stats;

  if (ctx == NULL || entry == NULL || ctx->active == NULL ||
      ctx->active->vol == NULL) {
    return APPLICATIONS_MENU_EXEC_STAY;
  }

  needs_selection = ApplicationCommandUsesToken(entry->command, "{}");
  needs_input = ApplicationCommandUsesToken(entry->command, "{input}");

  if (String_HasNonWhitespace(entry->prompt) && !needs_input) {
    UI_ShowStatusLineError(
        ctx,
        "%s has a prompt but no {input} placeholder. Edit applications.conf.",
        entry->label);
    return APPLICATIONS_MENU_EXEC_STAY;
  }
  if (needs_input && !String_HasNonWhitespace(entry->prompt)) {
    UI_ShowStatusLineError(
        ctx,
        "%s needs prompt text for {input}. Edit applications.conf.",
        entry->label);
    return APPLICATIONS_MENU_EXEC_STAY;
  }

  selection_path[0] = '\0';
  if (needs_selection && ResolveApplicationSelectionPath(ctx, selection_path) != 0) {
    UI_ShowStatusLineError(
        ctx,
        "%s needs a selected path. Choose a file or directory first.",
        entry->label);
    return APPLICATIONS_MENU_EXEC_STAY;
  }
  if (ResolveApplicationLaunchDirectory(ctx, launch_dir) != 0) {
    UI_ShowStatusLineError(ctx,
                           "%s needs an active file or directory context.",
                           entry->label);
    return APPLICATIONS_MENU_EXEC_STAY;
  }

  prompt_buffer[0] = '\0';
  if (String_HasNonWhitespace(entry->prompt)) {
    char prompt_label[APPLICATIONS_MENU_FIELD_MAX + 4];
    size_t prompt_len = strlen(entry->prompt);

    if (prompt_len > 0 && entry->prompt[prompt_len - 1] == ':') {
      (void)snprintf(prompt_label, sizeof(prompt_label), "%s", entry->prompt);
    } else {
      (void)snprintf(prompt_label, sizeof(prompt_label), "%s:", entry->prompt);
    }
    if (UI_ReadString(ctx, ctx->active, prompt_label, prompt_buffer,
                      COMMAND_LINE_LENGTH, HST_EXEC) != CR) {
      return APPLICATIONS_MENU_EXEC_STAY;
    }
    if (!String_HasNonWhitespace(prompt_buffer)) {
      UI_ShowStatusLineError(ctx,
                             "%s requires input. Enter a value or press Esc to "
                             "cancel.",
                             entry->label);
      return APPLICATIONS_MENU_EXEC_STAY;
    }
  }

  if (Path_BuildCommandLine(entry->command, NULL,
                            needs_selection ? "{}" : NULL,
                            needs_selection ? selection_path : NULL,
                            needs_input ? "{input}" : NULL,
                            needs_input ? prompt_buffer : NULL, command_line,
                            sizeof(command_line)) != 0) {
    UI_ShowStatusLineError(
        ctx,
        "%s command is invalid or too long. Edit applications.conf.",
        entry->label);
    return APPLICATIONS_MENU_EXEC_STAY;
  }

  stats = &ctx->active->vol->vol_stats;
  if (LaunchDetachedCommand(ctx, command_line, launch_dir, stats) != 0) {
    ReportApplicationLaunchFailure(ctx, entry, errno);
    return APPLICATIONS_MENU_EXEC_STAY;
  }
  UI_ShowStatusLineNotice(ctx, "launched: %s", entry->label);
  return APPLICATIONS_MENU_EXEC_CLOSE;
}

int UI_OpenApplicationsMenu(ViewContext *ctx) {
  ApplicationMenuEntry entries[APPLICATIONS_MENU_MAX_ENTRIES];
  WINDOW *win = NULL;
  DirEntry *dir_entry = NULL;
  size_t i;
  int entry_count;
  int selected_index = 0;
  int result = -1;
  int win_height;
  int win_width;
  int win_x;
  int win_y;
  int ch;
  int scroll_offset;
  BOOL menu_active;
  BOOL restart_menu;
  const char title[] = "Applications";

  if (ctx == NULL)
    return -1;

  ClearHelp(ctx);
  entry_count =
      LoadApplicationsCatalog(entries, (int)(sizeof(entries) / sizeof(entries[0])));
  if (entry_count <= 0) {
    UI_ShowStatusLineError(ctx,
                           "Applications catalog is empty. Edit applications.conf "
                           "or install the packaged presets.");
    return -1;
  }

  do {
    int max_label_len = 0;
    int visible_lines;

    restart_menu = FALSE;
    menu_active = TRUE;
    scroll_offset = 0;

    for (i = 0; i < (size_t)entry_count; i++) {
      int len = StrVisualLength(entries[i].label);
      if (len > max_label_len)
        max_label_len = len;
    }

    win_width = MAXIMUM((int)(strlen(title) + 4), max_label_len + 6);
    win_width =
        MAXIMUM(win_width, APPLICATIONS_MENU_COMMAND_STRIP_X + 2 +
                               UI_CommandStripVisualLength(
                                   applications_menu_commands,
                                   sizeof(applications_menu_commands) /
                                       sizeof(applications_menu_commands[0])));
    win_width = MINIMUM(win_width, COLS - ctx->layout.stats_width - 2);

    win_height = entry_count + 5;
    win_height = MAXIMUM(win_height, 8);
    win_height = MINIMUM(win_height, ctx->layout.bottom_border_y);

    win_x = ((COLS - ctx->layout.stats_width) - win_width) / 2;
    if (win_x < 1)
      win_x = 1;
    win_y = (LINES - win_height) / 2;

    visible_lines = win_height - 5;
    visible_lines = MAXIMUM(1, visible_lines);
    if (selected_index >= entry_count)
      selected_index = entry_count - 1;
    if (selected_index < 0)
      selected_index = 0;
    if (selected_index >= visible_lines) {
      scroll_offset = selected_index - visible_lines + 1;
      if (scroll_offset > entry_count - visible_lines)
        scroll_offset = entry_count - visible_lines;
    }
    if (scroll_offset < 0)
      scroll_offset = 0;

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
      UI_RenderCommandStrip(win, win_height - 2, APPLICATIONS_MENU_COMMAND_STRIP_X,
                            applications_menu_commands,
                            sizeof(applications_menu_commands) /
                                sizeof(applications_menu_commands[0]),
                            UI_ROLE_PICKER, UI_ROLE_KEYBIND);

      for (i = 0; (int)i < visible_lines; i++) {
        int actual_index = scroll_offset + (int)i;
        char item_buf[PATH_LENGTH + 1];
        int max_w = win_width - 4;

        if (actual_index >= entry_count)
          break;
        (void)snprintf(item_buf, sizeof(item_buf), "%s",
                       entries[actual_index].label);
        if ((int)strlen(item_buf) > max_w)
          item_buf[max_w] = '\0';
        PaintApplicationRow(ctx, win, 3 + (int)i, win_width, item_buf,
                            actual_index == selected_index);
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
          selected_index = entry_count - 1;
          scroll_offset = MAXIMUM(0, entry_count - visible_lines);
        }
        if (selected_index < scroll_offset)
          scroll_offset--;
        break;
      case KEY_DOWN:
        selected_index++;
        if (selected_index >= entry_count) {
          selected_index = 0;
          scroll_offset = 0;
        }
        if (selected_index >= scroll_offset + visible_lines)
          scroll_offset++;
        break;
      case KEY_HOME:
      case KEY_PPAGE:
        selected_index = 0;
        scroll_offset = 0;
        break;
      case KEY_END:
      case KEY_NPAGE:
        selected_index = entry_count - 1;
        scroll_offset = MAXIMUM(0, entry_count - visible_lines);
        break;
      case 'E':
      case 'e':
        UI_EditApplicationsCatalog(ctx,
                                   (ctx->active != NULL && ctx->active->vol != NULL)
                                       ? GetSelectedDirEntry(ctx, ctx->active->vol)
                                       : NULL);
        restart_menu = TRUE;
        menu_active = FALSE;
        break;
      case LF:
      case CR:
        if (ExecuteApplicationEntry(ctx, &entries[selected_index]) ==
            APPLICATIONS_MENU_EXEC_CLOSE) {
          result = 0;
          menu_active = FALSE;
        }
        break;
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

    if (win != NULL) { UI_Dialog_Close(ctx, win); win = NULL; }
    if (!restart_menu && ctx->active != NULL && ctx->active->vol != NULL) {
      dir_entry = GetSelectedDirEntry(ctx, ctx->active->vol);
      if (dir_entry != NULL)
        RefreshView(ctx, GetSelectedDirEntry(ctx, ctx->active->vol));
    }
  } while (restart_menu);

  curs_set(1);
  return result;
}
