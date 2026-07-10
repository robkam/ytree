/***************************************************************************
 *
 * src/ui/display.c
 * Functions for handling the display
 *
 ***************************************************************************/

#include "../../include/ytnova.h"
#include "../../include/ytnova_cmd.h"
#include "../../include/ytnova_fs.h"
#include "../../include/ytnova_panel_anchor.h"
#include "../../include/ytnova_ui.h"
#include "ytnova_appstate_actions.h"
#include "ytnova_appstate_focus.h"
#include "ytnova_appstate_layout.h"
#include "ytnova_appstate_render.h"
#include "ytnova_appstate_window.h"
#include <assert.h>

/* PrintMenuLine is removed as its functionality for drawing the static stats
 * panel is no longer needed. The stats panel is now fully managed by stats.c.
 */
/* static void PrintLine(WINDOW *win, int y, int x, char *line, int len); //
 * Removed: PrintLine is now an external function from display_utils.c */
/* static void DisplayVersion(void); // Removed: Unused function */

/* The 'mask' array is removed as the static statistics panel it defined
 * is now entirely managed by stats.c. */

/* 'extended_line' is removed as it was part of the static statistics panel
 * drawing logic, which is now obsolete. */

/* Legacy border strings removed. Use ACS_* constants directly. */

typedef struct {
  const char *prefix;
  const UICommandStripCommand *commands;
  size_t command_count;
} HelpCommandStrip;

static const UICommandStripCommand dir_help_disk_mode_0_commands[] = {
    {UI_COMMAND_LAYOUT_MNEMONIC, "Attributes", "A", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Brief", "B", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Copy", "C", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Delete", "D", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Filter", "F", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Global", "G", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Invert", "I", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "compare", "J", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Log", "L", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Makedir", "M", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Newfile", "N", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Only tagged", "O", NULL}};
static const UICommandStripCommand dir_help_disk_mode_1_commands[] = {
    {UI_COMMAND_LAYOUT_MNEMONIC, "Pipe", "P", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Quit", "Q", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Rename", "R", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Showall", "S", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Tag", "T", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Untag", "U", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "movedir", "V", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Write", "W", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "execute", "X", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "archive", "Z", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "jump", "/", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "dotfiles", "`", NULL}};
static const UICommandStripCommand dir_help_ll_mode_0_commands[] = {
    {UI_COMMAND_LAYOUT_MNEMONIC, "Brief", "B", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Filter", "F", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "dirmode", "^F", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Log", "L", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "reload", "^L", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Showall", "S", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Tag", "T", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Untag", "U", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Quit", "Q", NULL}};
static const UICommandStripCommand dir_help_archive_mode_0_commands[] = {
    {UI_COMMAND_LAYOUT_MNEMONIC, "Brief", "B", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Copy", "C", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Delete", "D", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Filter", "F", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "dirmode (^F)", "^F", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Global (G)", "G", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "compare (J)", "J", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Log", "L", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Makedir", "M", NULL}};
static const UICommandStripCommand dir_help_archive_mode_1_commands[] = {
    {UI_COMMAND_LAYOUT_MNEMONIC, "Pipe", "P", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Rename", "R", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Showall", "S", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Tag", "T", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Untag", "U", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "movedir", "V", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Quit", "Q", NULL}};
static const UICommandStripCommand file_help_disk_mode_0_commands[] = {
    {UI_COMMAND_LAYOUT_MNEMONIC, "Attributes", "A", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Brief", "B", NULL},
    {UI_COMMAND_LAYOUT_ALT_MNEMONIC, "copy", "C", "^K"},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Delete", "D", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Edit", "E", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Filter", "F", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "filemode", "^F", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Hex", "H", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Invert", "I", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "compare", "J", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Log", "L", NULL},
    {UI_COMMAND_LAYOUT_ALT_MNEMONIC, "move", "M", "^N"}};
static const UICommandStripCommand file_help_disk_mode_1_commands[] = {
    {UI_COMMAND_LAYOUT_MNEMONIC, "Newfile", "N", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Only tagged", "O", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Pipe", "P", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Quit", "Q", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Rename", "R", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Sort", "S", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Write", "W", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "execute", "X", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "pathcopy", "Y", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "archive", "Z", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "jump", "/", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "dotfiles", "`", NULL}};
static const UICommandStripCommand file_help_ll_mode_0_commands[] = {
    {UI_COMMAND_LAYOUT_MNEMONIC, "Brief", "B", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Filter", "F", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "filemode", "^F", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Log", "L", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "redraw", "^L", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Sort", "S", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Tag", "T", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Untag", "U", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Quit", "Q", NULL}};
static const UICommandStripCommand file_help_archive_mode_0_commands[] = {
    {UI_COMMAND_LAYOUT_MNEMONIC, "Brief", "B", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Copy", "C", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Delete", "D", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Filter", "F", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "filemode", "^F", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Hex", "H", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Invert", "I", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "compare", "J", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Rename", "R", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Sort", "S", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Tag", "T", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "view", "V", NULL}};
static const UICommandStripCommand file_help_archive_mode_1_commands[] = {
    {UI_COMMAND_LAYOUT_MNEMONIC, "Move", "M", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Pipe", "P", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "rename", "^R", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Untag", "U", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "pathcopy", "Y", NULL}};
static const UICommandStripCommand history_help_commands[] = {
    {UI_COMMAND_LAYOUT_MNEMONIC, "Pin/unpin", "P", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "OK", "Enter", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "Cancel", "Esc", NULL}};
static const UICommandStripCommand preview_help_commands[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "Select File", "Up", "Down"},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "Scroll Page", "PgUp", "PgDn"},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "Jump", "Home", "End"}};
static const UICommandStripCommand preview_command_commands[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "Navigate Preview", "Shift", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "Exit Preview", "F7", NULL}};
static const UICommandStripCommand dir_help_nav_commands[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "help", "F1", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "refresh", "F5", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "stats", "F6", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "autoview", "F7", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "split", "F8", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "apps", "F9", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "config", "F10", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "cancel", "Esc", NULL}};
static const UICommandStripCommand dir_help_nav_archive_to_root_commands[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "help", "F1", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "refresh", "F5", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "stats", "F6", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "autoview", "F7", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "split", "F8", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "apps", "F9", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "config", "F10", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "root", "\\", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "cancel", "Esc", NULL}};
static const UICommandStripCommand dir_help_nav_archive_exit_commands[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "help", "F1", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "refresh", "F5", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "stats", "F6", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "autoview", "F7", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "split", "F8", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "apps", "F9", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "config", "F10", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "exit", "\\", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "cancel", "Esc", NULL}};
static const UICommandStripCommand file_help_nav_commands[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "help", "F1", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "refresh", "F5", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "stats", "F6", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "autoview", "F7", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "split", "F8", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "apps", "F9", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "config", "F10", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "cancel", "Esc", NULL}};
static const UICommandStripCommand file_help_nav_to_dir_commands[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "help", "F1", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "refresh", "F5", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "stats", "F6", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "autoview", "F7", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "split", "F8", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "apps", "F9", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "config", "F10", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "to dir", "\\", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "cancel", "Esc", NULL}};
static const HelpCommandStrip dir_help_nav_builtin[] = {
    {"9-4 File ", dir_help_nav_commands,
     sizeof(dir_help_nav_commands) / sizeof(dir_help_nav_commands[0])},
    {"9-4 File ", dir_help_nav_archive_to_root_commands,
     sizeof(dir_help_nav_archive_to_root_commands) /
         sizeof(dir_help_nav_archive_to_root_commands[0])},
    {"9-4 File ", dir_help_nav_archive_exit_commands,
     sizeof(dir_help_nav_archive_exit_commands) /
         sizeof(dir_help_nav_archive_exit_commands[0])}};
static const HelpCommandStrip file_help_nav_builtin[] = {
    {"9-4 Tree ", file_help_nav_commands,
     sizeof(file_help_nav_commands) / sizeof(file_help_nav_commands[0])},
    {"9-4 Tree ", file_help_nav_to_dir_commands,
     sizeof(file_help_nav_to_dir_commands) /
         sizeof(file_help_nav_to_dir_commands[0])}};
static const HelpCommandStrip preview_help_builtin[] = {
    {"PREVIEW   ", preview_help_commands,
     sizeof(preview_help_commands) / sizeof(preview_help_commands[0])},
    {"COMMANDS  ", preview_command_commands,
     sizeof(preview_command_commands) / sizeof(preview_command_commands[0])}};

static const HelpCommandStrip dir_help_builtin[MAX_MODES][2] = {
    {{ "DIR      ", dir_help_disk_mode_0_commands,
       sizeof(dir_help_disk_mode_0_commands) /
           sizeof(dir_help_disk_mode_0_commands[0]) },
     { "COMMANDS ", dir_help_disk_mode_1_commands,
       sizeof(dir_help_disk_mode_1_commands) /
           sizeof(dir_help_disk_mode_1_commands[0]) }},
    {{ "DIR      ", dir_help_ll_mode_0_commands,
       sizeof(dir_help_ll_mode_0_commands) /
           sizeof(dir_help_ll_mode_0_commands[0]) },
     { "", NULL, 0 }},
    {{ "ARCHIVE   ", dir_help_archive_mode_0_commands,
       sizeof(dir_help_archive_mode_0_commands) /
           sizeof(dir_help_archive_mode_0_commands[0]) },
     { "COMMANDS ", dir_help_archive_mode_1_commands,
       sizeof(dir_help_archive_mode_1_commands) /
           sizeof(dir_help_archive_mode_1_commands[0]) }},
    {{ "DIR      ", dir_help_disk_mode_0_commands,
       sizeof(dir_help_disk_mode_0_commands) /
           sizeof(dir_help_disk_mode_0_commands[0]) },
     { "COMMANDS ", dir_help_disk_mode_1_commands,
       sizeof(dir_help_disk_mode_1_commands) /
           sizeof(dir_help_disk_mode_1_commands[0]) }}};

static const HelpCommandStrip file_help_builtin[MAX_MODES][2] = {
    {{ "FILE     ", file_help_disk_mode_0_commands,
       sizeof(file_help_disk_mode_0_commands) /
           sizeof(file_help_disk_mode_0_commands[0]) },
     { "COMMANDS ", file_help_disk_mode_1_commands,
       sizeof(file_help_disk_mode_1_commands) /
           sizeof(file_help_disk_mode_1_commands[0]) }},
    {{ "FILE     ", file_help_ll_mode_0_commands,
       sizeof(file_help_ll_mode_0_commands) /
           sizeof(file_help_ll_mode_0_commands[0]) },
     { "", NULL, 0 }},
    {{ "ARCH-FILE ", file_help_archive_mode_0_commands,
       sizeof(file_help_archive_mode_0_commands) /
           sizeof(file_help_archive_mode_0_commands[0]) },
     { "COMMANDS ", file_help_archive_mode_1_commands,
       sizeof(file_help_archive_mode_1_commands) /
           sizeof(file_help_archive_mode_1_commands[0]) }},
    {{ "FILE     ", file_help_disk_mode_0_commands,
       sizeof(file_help_disk_mode_0_commands) /
           sizeof(file_help_disk_mode_0_commands[0]) },
     { "COMMANDS ", file_help_disk_mode_1_commands,
       sizeof(file_help_disk_mode_1_commands) /
           sizeof(file_help_disk_mode_1_commands[0]) }}};

static void DisplayBuiltInHelpLine(ViewContext *ctx, int y,
                                   const HelpCommandStrip *strip) {
  int prefix_width;

  if (ctx == NULL || ctx->ctx_menu_window == NULL || strip == NULL ||
      strip->prefix == NULL)
    return;

  if (strncmp(strip->prefix, "9-4 ", 4) == 0) {
#ifdef COLOR_SUPPORT
    PrintSpecialString(ctx->ctx_menu_window, y, 0, "9-4", UI_ROLE_KEYBIND);
    PrintSpecialString(ctx->ctx_menu_window, y, 3, (char *)strip->prefix + 3,
                       UI_ROLE_HELP);
#else
    PrintSpecialString(ctx->ctx_menu_window, y, 0, "9-4", A_BOLD);
    PrintSpecialString(ctx->ctx_menu_window, y, 3, (char *)strip->prefix + 3,
                       A_NORMAL);
#endif
  } else {
#ifdef COLOR_SUPPORT
    PrintSpecialString(ctx->ctx_menu_window, y, 0, (char *)strip->prefix,
                       UI_ROLE_HELP);
#else
    PrintSpecialString(ctx->ctx_menu_window, y, 0, (char *)strip->prefix,
                       A_NORMAL);
#endif
  }

  prefix_width = StrVisualLength((char *)strip->prefix);
  UI_RenderCommandStrip(ctx->ctx_menu_window, y, prefix_width, strip->commands,
                        strip->command_count, UI_ROLE_HELP, UI_ROLE_KEYBIND);
}

static void DisplayPreviewHelpLine(ViewContext *ctx, int y,
                                   const HelpCommandStrip *strip) {
  int prefix_width;

  if (ctx == NULL || ctx->ctx_border_window == NULL || strip == NULL ||
      strip->prefix == NULL)
    return;

#ifdef COLOR_SUPPORT
  wattrset(ctx->ctx_border_window, COLOR_PAIR(UI_ROLE_HELP));
#else
  wattrset(ctx->ctx_border_window, A_NORMAL);
#endif
  mvwaddstr(ctx->ctx_border_window, y, 0, strip->prefix);
  wattrset(ctx->ctx_border_window, 0);

  prefix_width = StrVisualLength((char *)strip->prefix);
  UI_RenderCommandStrip(ctx->ctx_border_window, y, prefix_width, strip->commands,
                        strip->command_count, UI_ROLE_HELP, UI_ROLE_KEYBIND);
}

void DisplayDirHelp(ViewContext *ctx, const DirEntry *dir_entry) {
  int i;
  const HelpCommandStrip *nav_strip = &dir_help_nav_builtin[0];

  if (!ctx->ctx_menu_window)
    return;

  werase(ctx->ctx_menu_window);
  for (i = 0; i < 2; i++)
    DisplayBuiltInHelpLine(ctx, i, &dir_help_builtin[ctx->view_mode][i]);
  if (ctx->view_mode == ARCHIVE_MODE && dir_entry != NULL) {
    nav_strip = (dir_entry->up_tree != NULL) ? &dir_help_nav_builtin[1]
                                             : &dir_help_nav_builtin[2];
  }
  DisplayBuiltInHelpLine(ctx, 2, nav_strip);
  UI_RenderStatusLineError(ctx);
  wnoutrefresh(ctx->ctx_menu_window);
}

void DisplayFileHelp(ViewContext *ctx, const DirEntry *dir_entry) {
  int i;
  const HelpCommandStrip *nav_strip;

  if (!ctx->ctx_menu_window)
    return;

  werase(ctx->ctx_menu_window);
  for (i = 0; i < 2; i++)
    DisplayBuiltInHelpLine(ctx, i, &file_help_builtin[ctx->view_mode][i]);
  if (dir_entry && dir_entry->global_flag) {
    nav_strip = &file_help_nav_builtin[1];
  } else {
    nav_strip = &file_help_nav_builtin[0];
  }
  DisplayBuiltInHelpLine(ctx, 2, nav_strip);
  UI_RenderStatusLineError(ctx);
  wnoutrefresh(ctx->ctx_menu_window);
}

void DisplayHistoryHelp(ViewContext *ctx) {
  if (!ctx->ctx_menu_window)
    return;
  werase(ctx->ctx_menu_window);
  {
    static const HelpCommandStrip history_help_strip = {
        "", history_help_commands,
        sizeof(history_help_commands) / sizeof(history_help_commands[0])};
    DisplayBuiltInHelpLine(ctx, 0, &history_help_strip);
  }
  wnoutrefresh(ctx->ctx_menu_window);
}

void DisplayPreviewHelp(ViewContext *ctx) {
  /*
   * Help Footer for Preview Mode (F7)
  */
  wmove(ctx->ctx_border_window, Y_PROMPT(ctx), 0);
  wclrtoeol(ctx->ctx_border_window);
  DisplayPreviewHelpLine(ctx, Y_PROMPT(ctx), &preview_help_builtin[0]);
  wmove(ctx->ctx_border_window, Y_PROMPT(ctx) + 1, 0);
  wclrtoeol(ctx->ctx_border_window);
  DisplayPreviewHelpLine(ctx, Y_PROMPT(ctx) + 1, &preview_help_builtin[1]);
}

void ClearHelp(ViewContext *ctx) {
  if (ctx->ctx_menu_window) {
    werase(ctx->ctx_menu_window);
    wnoutrefresh(ctx->ctx_menu_window);
  }
}

/*
 * DisplayHeaderPath
 * Prints the current path in the top-left header area.
 * This function is designed to be called whenever the path changes,
 * ensuring immediate visual feedback.
 */
void DisplayHeaderPath(ViewContext *ctx, const char *path) {
  char display_buffer[PATH_LENGTH + 1];
  int available_width;

  if (!ctx->ctx_path_window)
    return;

  available_width = getmaxx(ctx->ctx_path_window);

  CutPathname(display_buffer, path, available_width);

  DEBUG_LOG("DisplayHeaderPath: path='%s' cut='%s' avail=%d", path,
            display_buffer, available_width);

  WbkgdSet(ctx, ctx->ctx_path_window, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));
  wattrset(ctx->ctx_path_window, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));
  werase(ctx->ctx_path_window);
  mvwaddstr(ctx->ctx_path_window, 0, 0, display_buffer);
  wnoutrefresh(ctx->ctx_path_window);
}

void DisplayMenu(ViewContext *ctx) {
  const int L_BORDER_FOR_DISPLAY = COLS - ctx->layout.stats_width - 1;
  int bottom_y = ctx->layout.bottom_border_y;

  /* Explicitly Clear all relevant windows - NO wnoutrefresh here */
  werase(ctx->ctx_border_window);

  /* Draw Header Label */
  wattrset(ctx->ctx_border_window, COLOR_PAIR(UI_ROLE_STATIC_TEXT));
  mvwaddstr(ctx->ctx_border_window, 0, 0, "Path: ");
  wattrset(ctx->ctx_border_window, A_NORMAL);

  /* Path will be filled in by caller (RefreshView) using
   * GetPath(dir_entry) */

  /* --- NATIVE ACS BORDERS --- */
  wattron(ctx->ctx_border_window, COLOR_PAIR(UI_ROLE_BOX_LINES) | A_ALTCHARSET);

  /* Outer Box Frame (Data Area) */
  mvwhline(ctx->ctx_border_window, 1, 0, ACS_HLINE, L_BORDER_FOR_DISPLAY);
  mvwhline(ctx->ctx_border_window, bottom_y, 0, ACS_HLINE,
           L_BORDER_FOR_DISPLAY);
  mvwvline(ctx->ctx_border_window, 1, 0, ACS_VLINE, bottom_y - 1);
  mvwvline(ctx->ctx_border_window, 1, L_BORDER_FOR_DISPLAY, ACS_VLINE,
           bottom_y - 1);

  /* Corners */
  mvwaddch(ctx->ctx_border_window, 1, 0, ACS_ULCORNER);
  mvwaddch(ctx->ctx_border_window, 1, L_BORDER_FOR_DISPLAY, ACS_URCORNER);
  mvwaddch(ctx->ctx_border_window, bottom_y, 0, ACS_LLCORNER);
  mvwaddch(ctx->ctx_border_window, bottom_y, L_BORDER_FOR_DISPLAY,
           ACS_LRCORNER);

  /* Sub-window separators */
  if (ctx->preview_mode) {
    int sep_x = ctx->layout.preview_win_x - 1;
    mvwvline(ctx->ctx_border_window, 2, sep_x, ACS_VLINE, bottom_y - 2);
    mvwaddch(ctx->ctx_border_window, 1, sep_x, ACS_TTEE);
    mvwaddch(ctx->ctx_border_window, bottom_y, sep_x, ACS_BTEE);
  } else {
    /* Vertical Split Separator */
    if (ctx->is_split_screen && ctx->left) {
      int split_x = ctx->left->dir_x + ctx->left->dir_w;
      mvwvline(ctx->ctx_border_window, 2, split_x, ACS_VLINE, bottom_y - 2);
      mvwaddch(ctx->ctx_border_window, 1, split_x, ACS_TTEE);
      mvwaddch(ctx->ctx_border_window, bottom_y, split_x, ACS_BTEE);
    }
  }
  wattroff(ctx->ctx_border_window, A_ALTCHARSET);
  wattrset(ctx->ctx_border_window, A_NORMAL);
}

void SwitchToSmallFileWindow(ViewContext *ctx) {
  /* Separator Y calculation: dir_win_y + dir_win_height */
  int separator_y = ctx->layout.dir_win_y + ctx->layout.dir_win_height;

  werase(ctx->ctx_file_window);
  int separator_width = COLS - ctx->layout.stats_width - 1;
  wattron(ctx->ctx_border_window, COLOR_PAIR(UI_ROLE_BOX_LINES) | A_ALTCHARSET);
  mvwhline(ctx->ctx_border_window, separator_y, 1, ACS_HLINE,
           separator_width - 1);
  mvwaddch(ctx->ctx_border_window, separator_y, 0, ACS_LTEE);
  mvwaddch(ctx->ctx_border_window, separator_y, separator_width, ACS_RTEE);

  if (ctx->layout.stats_width == 0) {
    mvwaddch(ctx->ctx_border_window, separator_y, COLS - 1, ACS_RTEE);
  }

  /* Restore Split Screen Junction if visible */
  if (ctx->is_split_screen && ctx->left) {
    int split_x = ctx->left->dir_x + ctx->left->dir_w;
    mvwaddch(ctx->ctx_border_window, separator_y, split_x, ACS_PLUS);
  }
  wattroff(ctx->ctx_border_window, A_ALTCHARSET);
  wattrset(ctx->ctx_border_window, A_NORMAL);

  AppStateSetPanelFileWindowHandle(ctx, ctx->active, FALSE);
}

void SwitchToBigFileWindow(ViewContext *ctx) {
  /* Separator Y calculation: dir_win_y + dir_win_height */
  int separator_y = ctx->layout.dir_win_y + ctx->layout.dir_win_height;

  werase(ctx->ctx_file_window);

  /* Erase the horizontal separator line completely */
  int separator_width = COLS - ctx->layout.stats_width - 1;
  wmove(ctx->ctx_border_window, separator_y, 0);
  whline(ctx->ctx_border_window, ' ', separator_width + 1);

  /* Draw vertical borders at left and right edges of dir window */
  wattron(ctx->ctx_border_window, COLOR_PAIR(UI_ROLE_BOX_LINES) | A_ALTCHARSET);
  mvwaddch(ctx->ctx_border_window, separator_y, ctx->layout.dir_win_x - 1,
           ACS_VLINE);
  mvwaddch(ctx->ctx_border_window, separator_y,
           ctx->layout.dir_win_x + ctx->layout.dir_win_width, ACS_VLINE);
  wattroff(ctx->ctx_border_window, A_ALTCHARSET);
  wattrset(ctx->ctx_border_window, A_NORMAL);

  AppStateSetPanelFileWindowHandle(ctx, ctx->active, TRUE);
}

void MapF2Window(ViewContext *ctx) {
  werase(ctx->ctx_f2_window);
}

void UnmapF2Window(ViewContext *ctx) {
  /* Separator Y calculation: dir_win_y + dir_win_height */
  int separator_y = ctx->layout.dir_win_y + ctx->layout.dir_win_height;

  werase(ctx->ctx_f2_window);
  wattrset(ctx->ctx_border_window, COLOR_PAIR(UI_ROLE_BOX_LINES));
  if (ctx->ctx_file_window == ctx->ctx_big_file_window) {
    mvwaddch(ctx->ctx_border_window, separator_y, ctx->layout.dir_win_x - 1,
             '|');
    mvwaddch(ctx->ctx_border_window, separator_y,
             ctx->layout.dir_win_x + ctx->layout.dir_win_width, '|');
  } else {
    int separator_width = COLS - ctx->layout.stats_width - 1;
    wattron(ctx->ctx_border_window, A_ALTCHARSET);
    mvwhline(ctx->ctx_border_window, separator_y, 1, ACS_HLINE,
             separator_width - 1);
    mvwaddch(ctx->ctx_border_window, separator_y, 0, ACS_LTEE);
    mvwaddch(ctx->ctx_border_window, separator_y, separator_width, ACS_RTEE);
    wattroff(ctx->ctx_border_window, A_ALTCHARSET);
  }
  wattrset(ctx->ctx_border_window, A_NORMAL);
}

/* PrintMenuLine function is removed as it is no longer used after decoupling
 * the static stats panel from display.c. */

void RefreshWindow(WINDOW *win) { wnoutrefresh(win); }

static BOOL IsPanelSavedBigFileMode(const YtreeNovaPanel *panel);

static void ComputePanelRenderPosition(const YtreeNovaPanel *panel, int idx,
                                       int *begin_out, int *cursor_out) {
  int height;

  if (!begin_out || !cursor_out)
    return;
  *begin_out = 0;
  *cursor_out = 0;

  if (!panel || !panel->vol || !panel->vol->dir_entry_list ||
      panel->vol->total_dirs <= 0)
    return;

  if (idx < 0)
    idx = 0;
  if (idx >= panel->vol->total_dirs)
    idx = panel->vol->total_dirs - 1;

  height = panel->pan_dir_window ? getmaxy(panel->pan_dir_window) : 1;
  if (height < 1)
    height = 1;

  *begin_out = panel->disp_begin_pos;
  *cursor_out = panel->cursor_pos;
  if (!PanelComputeViewportPosition(panel, idx, height, begin_out, cursor_out))
    return;
}

static DirEntry *ResolvePanelFileAnchor(const YtreeNovaPanel *panel) {
  if (!panel || !panel->vol || panel->saved_focus != FOCUS_FILE)
    return NULL;
  assert(panel->file_selection_dir_path[0] != '\0');
  if (panel->file_selection_dir_path[0] == '\0')
    return NULL;

  return ResolvePanelAnchorTarget(panel, panel->vol,
                                  panel->file_selection_dir_path);
}

static DirEntry *ResolvePanelFileAnchorForRender(ViewContext *ctx,
                                                 const YtreeNovaPanel *panel) {
  (void)ctx;
  return ResolvePanelFileAnchor(panel);
}

void RenderInactivePanel(ViewContext *ctx, YtreeNovaPanel *panel) {
  if (!panel || !panel->vol || !panel->pan_dir_window)
    return;

  int total = panel->vol->total_dirs;
  int begin = panel->disp_begin_pos;
  int cursor = panel->cursor_pos;
  int selected_idx = GetPanelVisibleSelectionIndex(panel);

  if (total > 0 && (begin + cursor >= total)) {
    begin = 0;
    cursor = 0;
  }

  if (total <= 0)
    return;

  if (panel->saved_focus == FOCUS_FILE &&
      ResolvePanelFileAnchorForRender(ctx, panel)) {
    begin = panel->disp_begin_pos;
    cursor = panel->cursor_pos;
  }

  {
    int render_start = panel->start_file;
    int render_cursor = 0;
    int render_begin = begin;
    int render_tree_cursor = cursor;
    int idx = selected_idx;
    DirEntry *render_dir = NULL;
    const DirEntry *de = NULL;

    if (idx < 0 || idx >= total)
      return;
    ComputePanelRenderPosition(panel, idx, &render_begin, &render_tree_cursor);
    begin = render_begin;

    de = panel->vol->dir_entry_list[idx].dir_entry;
    if (!de)
      return;

    if (panel->saved_focus == FOCUS_FILE) {
      BOOL refresh_file_cache = FALSE;
      char render_dir_path[PATH_LENGTH + 1];

      render_dir = ResolvePanelFileAnchorForRender(ctx, panel);

      if (!render_dir)
        render_dir = (DirEntry *)de;

      GetPath(render_dir, render_dir_path);
      render_dir_path[PATH_LENGTH] = '\0';

      /* Inactive-panel rendering must not rewrite the frozen snapshot. Keep
       * the anchor local and rebuild cache only when the saved directory is
       * genuinely missing or stale. */
      DirOps_ReloadPanelFileAnchorIfMissing(ctx, panel, render_dir);
      if (panel->file_entry_list == NULL ||
          strcmp(panel->file_selection_dir_path, render_dir_path) != 0) {
        refresh_file_cache = TRUE;
      }
      if (refresh_file_cache) {
        FreeFileEntryList(panel);
        BuildFileEntryList(ctx, panel);
      }
    } else if (!panel->file_entry_list) {
      BuildFileEntryList(ctx, panel);
    }

    if (render_dir)
      de = render_dir;

    render_cursor = de->cursor_pos;
    if (panel->saved_focus == FOCUS_FILE) {
      render_start = panel->start_file;
      render_cursor = panel->file_cursor_pos;
    }
    AppStateClampRenderFileViewport(panel->file_count, &render_start,
                                    &render_cursor);

    if (IsPanelSavedBigFileMode(panel) && panel->pan_big_file_window) {
      int file_hilight = -1;

      if (panel->saved_focus == FOCUS_FILE) {
        file_hilight =
            AppStateResolveRenderFileHighlight(panel->file_count, render_start,
                                               render_cursor);
      }
      DEBUG_LOG("RenderInactivePanel:file path='%s' start=%d cursor=%d count=%u",
                panel->file_selection_dir_path[0] ? panel->file_selection_dir_path
                                                  : "<none>",
                render_start, render_cursor, panel->file_count);
      if (panel->pan_dir_window) {
        werase(panel->pan_dir_window);
        wnoutrefresh(panel->pan_dir_window);
      }
      DisplayFiles(ctx, panel, de, render_start, file_hilight, 0,
                   panel->pan_big_file_window);
      wnoutrefresh(panel->pan_big_file_window);
      return;
    }

    if (panel->pan_dir_window) {
      int tree_hilight = selected_idx;
      if (panel->saved_focus == FOCUS_FILE)
        tree_hilight = -1;
      DisplayTree(ctx, panel->vol, panel->pan_dir_window, begin, tree_hilight,
                  FALSE);
      wnoutrefresh(panel->pan_dir_window);
    }

    if (panel->pan_file_window) {
      if (panel->saved_focus != FOCUS_FILE && ctx &&
          AppStateResolveActivePanelFocus(ctx) == FOCUS_FILE &&
          panel->file_count == 0) {
        werase(panel->pan_file_window);
        wnoutrefresh(panel->pan_file_window);
      } else {
        int file_hilight = -1;

        if (panel->saved_focus == FOCUS_FILE) {
          file_hilight =
              AppStateResolveRenderFileHighlight(panel->file_count, render_start,
                                                 render_cursor);
        }
        DisplayFiles(ctx, panel, de, render_start, file_hilight, 0,
                     panel->pan_file_window);
        wnoutrefresh(panel->pan_file_window);
      }
    }
  }
}

static BOOL IsActivePanelBigFileMode(const ViewContext *ctx,
                                     const DirEntry *dir_entry) {
  if (!ctx)
    return FALSE;

  if (!dir_entry)
    return FALSE;

  if (!ctx->active || AppStateResolveActivePanelFocus(ctx) != FOCUS_FILE)
    return FALSE;

  return (ctx->active->saved_big_file_view || dir_entry->global_flag ||
          dir_entry->tagged_flag);
}

static BOOL IsPanelSavedBigFileMode(const YtreeNovaPanel *panel) {
  if (!panel)
    return FALSE;

  return (panel->saved_focus == FOCUS_FILE && panel->saved_big_file_view);
}

static void DrawSplitSeparatorRow(ViewContext *ctx, BOOL left_big,
                                  BOOL right_big) {
  int separator_y;
  int data_right_x;
  int split_x;

  if (!ctx || !ctx->ctx_border_window || !ctx->left)
    return;

  separator_y = ctx->layout.dir_win_y + ctx->layout.dir_win_height;
  data_right_x = COLS - ctx->layout.stats_width - 1;
  split_x = ctx->left->dir_x + ctx->left->dir_w;

  /* Clear the entire separator row before redrawing split-aware junctions. */
  wmove(ctx->ctx_border_window, separator_y, 0);
  whline(ctx->ctx_border_window, ' ', data_right_x + 1);

  wattron(ctx->ctx_border_window, COLOR_PAIR(UI_ROLE_BOX_LINES) | A_ALTCHARSET);

  mvwaddch(ctx->ctx_border_window, separator_y, 0,
           left_big ? ACS_VLINE : ACS_LTEE);
  mvwaddch(ctx->ctx_border_window, separator_y, data_right_x,
           right_big ? ACS_VLINE : ACS_RTEE);

  if (!left_big && split_x > 1) {
    mvwhline(ctx->ctx_border_window, separator_y, 1, ACS_HLINE, split_x - 1);
  }
  if (!right_big && data_right_x - split_x > 1) {
    mvwhline(ctx->ctx_border_window, separator_y, split_x + 1, ACS_HLINE,
             data_right_x - split_x - 1);
  }

  if (!left_big && !right_big) {
    mvwaddch(ctx->ctx_border_window, separator_y, split_x, ACS_PLUS);
  } else if (!left_big && right_big) {
    mvwaddch(ctx->ctx_border_window, separator_y, split_x, ACS_RTEE);
  } else if (left_big && !right_big) {
    mvwaddch(ctx->ctx_border_window, separator_y, split_x, ACS_LTEE);
  } else {
    mvwaddch(ctx->ctx_border_window, separator_y, split_x, ACS_VLINE);
  }

  wattroff(ctx->ctx_border_window, A_ALTCHARSET);
  wattrset(ctx->ctx_border_window, A_NORMAL);
}

/*
 * CENTRALIZED REDRAW FUNCTION
 * Handles the complexities of Split/Big/Preview modes in one place.
 * Use this to ensure all borders, stats, and content are consistent.
 */
void RefreshView(ViewContext *ctx, DirEntry *dir_entry) {
  if (!AppStateValidatedDispatchSurface("surface.render-reflow-projection"))
    return;
  if (!AppStateValidatedEvent("event.render-reflow"))
    return;

  const Statistic *s = &ctx->active->vol->vol_stats;
  BOOL needs_window_recreate = FALSE;
  BOOL active_big_mode;

  if (ctx->active == NULL)
    MESSAGE(ctx, "FATAL: RefreshView called with NULL ctx->active");

  /* 1. Re-evaluate Layout; only recreate windows on actual resize */
  Layout_Recalculate(ctx);
  if (ctx->cached_lines != LINES || ctx->cached_cols != COLS) {
    if (!AppStateCommitTerminalGeometryCache(ctx, LINES, COLS))
      return;
    needs_window_recreate = TRUE;
  }

  /* Preview mode requires a dedicated window topology, not just resized
   * geometry values. Recreate if the preview window lifecycle is out of sync.
   */
  if (ctx->preview_mode && ctx->ctx_preview_window == NULL) {
    needs_window_recreate = TRUE;
  } else if (!ctx->preview_mode && ctx->ctx_preview_window != NULL) {
    needs_window_recreate = TRUE;
  }

  if (needs_window_recreate) {
    ReCreateWindows(ctx);
    touchwin(stdscr);
    wnoutrefresh(stdscr);
  }

  /* 3. Draw Borders and Dynamic Static Frames into ctx_border_window */
  DisplayMenu(ctx);
  touchwin(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);

  /* 4. Render Stats (updates ctx_border_window) */
  if (!ctx->preview_mode) {
    DisplayDiskStatistic(ctx, s);
    UpdateStatsPanel(ctx, dir_entry, s);
  }

  /* 5. Refresh Background/Border Window SECOND (z=0) */

  /* 6. Update Header Path (already drawn to border window) */
  {
    char path[PATH_LENGTH + 1];
    DirEntry *path_dir = dir_entry;
    ViewFocus active_focus = AppStateResolveActivePanelFocus(ctx);

    if (!ctx->preview_mode && dir_entry && active_focus == FOCUS_FILE &&
        ctx->active && ctx->active->file_entry_list && ctx->active->file_count > 0) {
      int idx = dir_entry->start_file + dir_entry->cursor_pos;
      if (idx >= 0 && (unsigned int)idx < ctx->active->file_count) {
        FileEntry *fe = ctx->active->file_entry_list[idx].file;
        if (fe && fe->dir_entry)
          path_dir = fe->dir_entry;
      }
    }

    GetPath(path_dir, path);
    DisplayHeaderPath(ctx, path);
  }

  /* 7. Draw Content Panels THIRD (z=1) */
  active_big_mode = IsActivePanelBigFileMode(ctx, dir_entry);

  if (ctx->preview_mode) {
    /* Preview mode always uses the active panel's big file window as the
     * left list pane. Avoid SwitchToBigFileWindow because its separator
     * surgery assumes standard tree/file geometry and corrupts preview
     * borders.
     */
    AppStateSetPanelFileWindowHandle(ctx, ctx->active, TRUE);
    DisplayFileWindow(ctx, ctx->active, dir_entry);
    if (ctx->ctx_preview_window)
      wnoutrefresh(ctx->ctx_preview_window);
  } else {
    if (ctx->is_split_screen && ctx->left && ctx->right && ctx->active) {
      BOOL left_big_mode;
      BOOL right_big_mode;
      YtreeNovaPanel *inactive;

      inactive = (ctx->active == ctx->left) ? ctx->right : ctx->left;

      left_big_mode = (ctx->active == ctx->left)
                          ? active_big_mode
                          : IsPanelSavedBigFileMode(ctx->left);
      right_big_mode = (ctx->active == ctx->right)
                           ? active_big_mode
                           : IsPanelSavedBigFileMode(ctx->right);

      AppStateSetPanelFileWindowHandle(ctx, ctx->left, left_big_mode);
      AppStateSetPanelFileWindowHandle(ctx, ctx->right, right_big_mode);
      AppStateSetPanelFileWindowHandle(ctx, ctx->active, active_big_mode);

      DrawSplitSeparatorRow(ctx, left_big_mode, right_big_mode);

      if (!active_big_mode && ctx->active->pan_dir_window) {
        BOOL tree_highlight = (AppStateResolveActivePanelFocus(ctx) == FOCUS_TREE);
        DisplayTree(ctx, ctx->active->vol, ctx->active->pan_dir_window,
                    ctx->active->disp_begin_pos,
                    GetPanelVisibleSelectionIndex(ctx->active),
                    tree_highlight);
        wnoutrefresh(ctx->active->pan_dir_window);
      }
      DisplayFileWindow(ctx, ctx->active, dir_entry);
      RenderInactivePanel(ctx, inactive);
    } else {
      if (active_big_mode) {
        SwitchToBigFileWindow(ctx);
        DisplayFileWindow(ctx, ctx->active, dir_entry);
      } else {
        SwitchToSmallFileWindow(ctx);
        if (ctx->active && ctx->active->pan_dir_window) {
          BOOL tree_highlight = (AppStateResolveActivePanelFocus(ctx) == FOCUS_TREE);
          DisplayTree(ctx, ctx->active->vol, ctx->active->pan_dir_window,
                      ctx->active->disp_begin_pos,
                      GetPanelVisibleSelectionIndex(ctx->active),
                      tree_highlight);
          wnoutrefresh(ctx->active->pan_dir_window);
        }
        DisplayFileWindow(ctx, ctx->active, dir_entry);
      }
    }
  }

  /* 8. Update Footer Help and Refresh Menu Window LAST (z=2) */
  if (ctx->preview_mode) {
    DisplayPreviewHelp(ctx);
  } else {
    if (AppStateResolveActivePanelFocus(ctx) == FOCUS_TREE) {
      DisplayDirHelp(ctx, dir_entry);
    } else {
      DisplayFileHelp(ctx, dir_entry);
    }
    if (ctx->ctx_menu_window)
      wnoutrefresh(ctx->ctx_menu_window);
  }

  UI_Dialog_RefreshAll(ctx);
  ClockHandler(ctx, 0);
  doupdate();
}
