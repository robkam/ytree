/***************************************************************************
 *
 * src/ui/display.c
 * Functions for handling the display
 *
 ***************************************************************************/

#include "../../include/patchlev.h"
#include "../../include/ytree.h"

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

static void PrintHelpString(WINDOW *win, int y, int x, const char *str);
static void PrintNavLine(WINDOW *win, int y, const char *str);

/*
 * Help line definitions for different modes.
 * NOTE: These strings are manually balanced to fit on a 120x36
 * terminal screen. When adding or removing commands, care must be taken
 * to re-balance the two lines to prevent truncation. Commands are ordered
 * alphabetically for easier scanning.
 *
 * Updated: (F)ilespec -> (F)ilter, spacing adjustments.
 */
static char dir_help_disk_mode_0[] =
    "DIR      (A)ttributes (B)rief (C)ompare (D)elete (F)ilter (G)lobal "
    "(L)og (M)akedir (N)ewfile";
static char dir_help_disk_mode_1[] =
    "COMMANDS (O) archive (P)ipe (Q)uit (R)ename (S)howall (T)ag (U)ntag "
    "e(X)ecute (/) "
    "jump (`) dotfiles";
static char dir_help_nav[] =
    "Tree  (F1) help  (F5) refresh  (F6) stats  (F7) autoview  "
    "(F8) split  (F9) menu  (F10) config  (Esc) cancel";
static char dir_help_nav_archive_to_root[] =
    "Tree  (F1) help  (F5) refresh  (F6) stats  (F7) autoview  "
    "(F8) split  (F9) menu  (F10) config  (\\) root  (Esc) cancel";
static char dir_help_nav_archive_exit[] =
    "Tree  (F1) help  (F5) refresh  (F6) stats  (F7) autoview  "
    "(F8) split  (F9) menu  (F10) config  (\\) exit  (Esc) cancel";
static char *dir_help[MAX_MODES][2] = {
    {/* DISK_MODE */
     dir_help_disk_mode_0, dir_help_disk_mode_1},
    {/* LL_FILE_MODE */
     "DIR       (B)rief (F)ilter (^F) dirmode (L)og re(^L)oad (S)howall (T)ag "
     "(U)ntag (Q)uit",
     "COMMANDS                                                                 "
     "          "},
    {/* ARCHIVE_MODE */
     "ARCHIVE   (B)rief (C)ompare (D)elete (F)ilter (^F) dirmode (G)lobal "
     "(L)og (M)akedir ",
     "COMMANDS  (R)ename (S)howall (T)ag (U)ntag (Q)uit                        "
     "       "},
    {                      /* USER_MODE */
     dir_help_disk_mode_0, /* Default unless changed by user prefs */
     dir_help_disk_mode_1}};

static char file_help_disk_mode_0[] =
    "FILE     (A)ttributes (B)rief (C)opy/(^K) (D)elete (E)dit (F)ilter "
    "(^F)ilemode (H)ex (I)nvert (J) compare (L)og";
static char file_help_disk_mode_1[] =
    "COMMANDS (M)ove/(^N) (N)ewfile "
    "(O) archive (P)ipe (Q)uit (R)ename (S)ort e(X)ecute (/) jump (`) dotfiles";
static char file_help_nav[] =
    "Dir   (F1) help  (F5) refresh  (F6) stats  (F7) autoview  "
    "(F8) split  (F9) menu  (F10) config  (Esc) cancel";
static char file_help_nav_to_dir[] =
    "Dir   (F1) help  (F5) refresh  (F6) stats  (F7) autoview  "
    "(F8) split  (F9) menu  (F10) config  (\\) to dir  (Esc) cancel";
static char *file_help[MAX_MODES][2] = {
    {/* DISK_MODE */
     file_help_disk_mode_0, file_help_disk_mode_1},
    {/* LL_FILE_MODE */
     "FILE      (B)rief (F)ilter (^F)ilemode (L)og (^L) redraw (S)ort (T)ag "
     "(U)ntag (Q)uit      ",
     "COMMANDS                                                                 "
     "               "},
    {/* ARCHIVE_MODE */
     "ARCH-FILE (B)rief (C)opy (D)elete (F)ilter (^F)ilemode (H)ex (I)nvert "
     "(J) compare (R)ename (S)ort (T)ag (V)iew",
     "COMMANDS  (M)ove (P)ipe (^R)ename (U)ntag pathcop(Y)"},
    {                       /* USER_MODE */
     file_help_disk_mode_0, /* Default unless changed by user prefs */
     file_help_disk_mode_1}};

void DisplayDirHelp(ViewContext *ctx, const DirEntry *dir_entry) {
  int i;
  char *cptr;
  const char *nav_line = dir_help_nav;

  if (!ctx->ctx_menu_window)
    return;

  if (ctx->view_mode == USER_MODE) {
    if (dir_help[ctx->view_mode][0] == dir_help_disk_mode_0 &&
        (cptr = (GetProfileValue)(ctx, "DIR1")) != NULL)
      dir_help[ctx->view_mode][0] = cptr;
    if (dir_help[ctx->view_mode][1] == dir_help_disk_mode_1 &&
        (cptr = (GetProfileValue)(ctx, "DIR2")) != NULL)
      dir_help[ctx->view_mode][1] = cptr;
  }
  werase(ctx->ctx_menu_window);
  for (i = 0; i < 2; i++) {
    PrintOptions(ctx->ctx_menu_window, i, 0, dir_help[ctx->view_mode][i]);
  }
  if (ctx->view_mode == ARCHIVE_MODE && dir_entry != NULL) {
    nav_line = (dir_entry->up_tree != NULL) ? dir_help_nav_archive_to_root
                                            : dir_help_nav_archive_exit;
  }
  PrintNavLine(ctx->ctx_menu_window, 2, nav_line);
  UI_RenderStatusLineError(ctx);
  wnoutrefresh(ctx->ctx_menu_window);
}

void DisplayFileHelp(ViewContext *ctx, const DirEntry *dir_entry) {
  int i;
  char *cptr;
  const char *nav_line;

  if (!ctx->ctx_menu_window)
    return;

  if (ctx->view_mode == USER_MODE) {
    if (file_help[ctx->view_mode][0] == file_help_disk_mode_0 &&
        (cptr = (GetProfileValue)(ctx, "FILE1")) != NULL)
      file_help[ctx->view_mode][0] = cptr;
    if (file_help[ctx->view_mode][1] == file_help_disk_mode_1 &&
        (cptr = (GetProfileValue)(ctx, "FILE2")) != NULL)
      file_help[ctx->view_mode][1] = cptr;
  }
  werase(ctx->ctx_menu_window);
  for (i = 0; i < 2; i++) {
    PrintOptions(ctx->ctx_menu_window, i, 0, file_help[ctx->view_mode][i]);
  }
  nav_line = (dir_entry && dir_entry->global_flag) ? file_help_nav_to_dir
                                                   : file_help_nav;
  PrintNavLine(ctx->ctx_menu_window, 2, nav_line);
  UI_RenderStatusLineError(ctx);
  wnoutrefresh(ctx->ctx_menu_window);
}

static void PrintHelpString(WINDOW *win, int y, int x, const char *str) {
  int ch;
  int color, hi_color, lo_color;
  int max_x;
  int draw_x;

  if (x < 0 || y < 0)
    return;
  max_x = getmaxx(win);
  if (max_x <= 0 || x >= max_x)
    return;

#ifdef COLOR_SUPPORT
  lo_color = CPAIR_MENU;
  hi_color = CPAIR_HIMENUS;
#else
  lo_color = A_NORMAL;
  hi_color = A_BOLD;
#endif

  color = lo_color;
  draw_x = x;

  for (; *str; str++) {
    if (draw_x >= max_x)
      break;
    ch = (int)*str;
    switch (*str) {
    case '(':
      color = hi_color;
      continue;
    case ')':
      color = lo_color;
      continue;
    default:
      break; /* Print normally */
    }

#ifdef COLOR_SUPPORT
    if (color == hi_color)
      wattrset(win, COLOR_PAIR(color) | A_BOLD);
    else
      wattrset(win, COLOR_PAIR(color));
#else
    wattrset(win, color);
#endif
    mvwaddch(win, y, draw_x++, ch);
  }
  wattrset(win, 0);
}

static void PrintNavLine(WINDOW *win, int y, const char *str) {
#ifdef COLOR_SUPPORT
  wattrset(win, COLOR_PAIR(CPAIR_HIMENUS) | A_BOLD);
#else
  wattrset(win, A_BOLD);
#endif
  mvwaddch(win, y, 0, ACS_LARROW);
  mvwaddch(win, y, 1, ACS_LRCORNER);
  wattrset(win, 0);
  mvwaddch(win, y, 2, ' ');
  PrintHelpString(win, y, 3, str);
}

void DisplayHistoryHelp(ViewContext *ctx) {
  if (!ctx->ctx_menu_window)
    return;
  werase(ctx->ctx_menu_window);
  PrintOptions(ctx->ctx_menu_window, 0, 0,
               "History   (P)in/unpin    (Enter) OK    (Esc) Cancel");
  wnoutrefresh(ctx->ctx_menu_window);
}

void DisplayPreviewHelp(ViewContext *ctx) {
  /*
   * Help Footer for Preview Mode (F7)
   */
  wmove(ctx->ctx_border_window, Y_PROMPT(ctx), 0);
  wclrtoeol(ctx->ctx_border_window);
  PrintHelpString(ctx->ctx_border_window, Y_PROMPT(ctx), 0,
                  "PREVIEW   (Up/Down) Select File  (PgUp/PgDn) Scroll Page  "
                  "(Home/End) Jump");
  wmove(ctx->ctx_border_window, Y_PROMPT(ctx) + 1, 0);
  wclrtoeol(ctx->ctx_border_window);
  PrintHelpString(ctx->ctx_border_window, Y_PROMPT(ctx) + 1, 0,
                  "COMMANDS  (Shift) Navigate Preview  (F7) Exit Preview");
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
  wattrset(ctx->ctx_border_window, COLOR_PAIR(CPAIR_MENU) | A_BOLD);
  mvwaddstr(ctx->ctx_border_window, 0, 0, "Path: ");
  wattrset(ctx->ctx_border_window, A_NORMAL);

  /* Path will be filled in by caller (RefreshView) using
   * GetPath(dir_entry) */

  /* --- NATIVE ACS BORDERS --- */
  wattron(ctx->ctx_border_window, COLOR_PAIR(CPAIR_BORDERS) | A_ALTCHARSET);

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
  wattron(ctx->ctx_border_window, COLOR_PAIR(CPAIR_BORDERS) | A_ALTCHARSET);
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

  ctx->ctx_file_window = ctx->ctx_small_file_window;
  if (ctx->active) {
    ctx->active->pan_file_window = ctx->active->pan_small_file_window;
  }
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
  wattron(ctx->ctx_border_window, COLOR_PAIR(CPAIR_BORDERS) | A_ALTCHARSET);
  mvwaddch(ctx->ctx_border_window, separator_y, ctx->layout.dir_win_x - 1,
           ACS_VLINE);
  mvwaddch(ctx->ctx_border_window, separator_y,
           ctx->layout.dir_win_x + ctx->layout.dir_win_width, ACS_VLINE);
  wattroff(ctx->ctx_border_window, A_ALTCHARSET);
  wattrset(ctx->ctx_border_window, A_NORMAL);

  ctx->ctx_file_window = ctx->ctx_big_file_window;
  if (ctx->active) {
    ctx->active->pan_file_window = ctx->active->pan_big_file_window;
  }
}

void MapF2Window(ViewContext *ctx) {
  werase(ctx->ctx_f2_window);
  box(ctx->ctx_f2_window, 0, 0);
  RefreshWindow(ctx->ctx_f2_window);
}

void UnmapF2Window(ViewContext *ctx) {
  /* Separator Y calculation: dir_win_y + dir_win_height */
  int separator_y = ctx->layout.dir_win_y + ctx->layout.dir_win_height;

  werase(ctx->ctx_f2_window);
  wattrset(ctx->ctx_border_window, COLOR_PAIR(CPAIR_BORDERS));
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

void RenderInactivePanel(ViewContext *ctx, YtreePanel *panel) {
  /* Modified check as per instructions */
  if (!panel || !panel->vol || !panel->pan_dir_window)
    return;

  /* Clamp cursor */
  int total = panel->vol->total_dirs;
  /* Use Panel state, not Volume state */
  int begin = panel->disp_begin_pos;
  int cursor = panel->cursor_pos;

  if (total > 0 && (begin + cursor >= total)) {
    /* Fix invalid position if any */
    begin = 0;
    cursor = 0;
  }

  if (total <= 0)
    return;

  {
    int idx = begin + cursor;
    int render_start = panel->start_file;
    int render_cursor = 0;
    const DirEntry *de = NULL;
    BOOL has_file_list = FALSE;

    if (idx < 0 || idx >= total)
      return;

    de = panel->vol->dir_entry_list[idx].dir_entry;
    if (!de)
      return;

    if (!panel->file_entry_list) {
      BuildFileEntryList(ctx, panel);
    }
    has_file_list = (panel->file_entry_list != NULL);

    render_cursor = de->cursor_pos;
    if (panel->file_dir_entry == de) {
      render_start = panel->start_file;
      render_cursor = panel->file_cursor_pos;
    }

    if (panel->saved_focus == FOCUS_FILE && panel->pan_big_file_window) {
      /* Preserve file-centric state for inactive panels: when a panel was left
       * in file mode, render it on files, not on directory tree.
       */
      if (panel->pan_dir_window) {
        werase(panel->pan_dir_window);
        wnoutrefresh(panel->pan_dir_window);
      }
      if (has_file_list) {
        DisplayFiles(ctx, panel, de, render_start, render_start + render_cursor,
                     0, panel->pan_big_file_window);
      } else {
        werase(panel->pan_big_file_window);
      }
      wnoutrefresh(panel->pan_big_file_window);
      return;
    }

    /* Tree-focused inactive panel rendering */
    if (panel->pan_dir_window) {
      DisplayTree(ctx, panel->vol, panel->pan_dir_window, begin, begin + cursor,
                  FALSE);
      wnoutrefresh(panel->pan_dir_window);
    }

    if (panel->pan_file_window) {
      if (has_file_list) {
        DisplayFiles(ctx, panel, de, render_start, -1, 0,
                     panel->pan_file_window);
      } else {
        werase(panel->pan_file_window);
      }
      wnoutrefresh(panel->pan_file_window);
    }
  }
}

static BOOL IsActivePanelBigFileMode(const ViewContext *ctx,
                                     const DirEntry *dir_entry) {
  if (!ctx)
    return FALSE;

  if (ctx->focused_window == FOCUS_FILE)
    return TRUE;

  if (!dir_entry)
    return FALSE;

  return (dir_entry->big_window || dir_entry->global_flag ||
          dir_entry->tagged_flag);
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

  wattron(ctx->ctx_border_window, COLOR_PAIR(CPAIR_BORDERS) | A_ALTCHARSET);

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
  const Statistic *s = &ctx->active->vol->vol_stats;
  BOOL needs_window_recreate = FALSE;
  BOOL active_big_mode;

  if (ctx->active == NULL)
    MESSAGE(ctx, "FATAL: RefreshView called with NULL ctx->active");

  /* 1. Re-evaluate Layout; only recreate windows on actual resize */
  Layout_Recalculate(ctx);
  if (ctx->cached_lines != LINES || ctx->cached_cols != COLS) {
    ctx->cached_lines = LINES;
    ctx->cached_cols = COLS;
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

    if (!ctx->preview_mode && ctx->focused_window == FOCUS_FILE &&
        ctx->active && ctx->active->file_entry_list &&
        ctx->active->file_count > 0) {
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
    if (ctx->active && ctx->active->pan_big_file_window) {
      ctx->active->pan_file_window = ctx->active->pan_big_file_window;
      ctx->ctx_file_window = ctx->active->pan_big_file_window;
    }
    DisplayFileWindow(ctx, ctx->active, dir_entry);
    if (ctx->ctx_preview_window)
      wnoutrefresh(ctx->ctx_preview_window);
  } else {
    if (ctx->is_split_screen && ctx->left && ctx->right && ctx->active) {
      BOOL left_big_mode;
      BOOL right_big_mode;
      YtreePanel *inactive;

      inactive = (ctx->active == ctx->left) ? ctx->right : ctx->left;

      left_big_mode = (ctx->active == ctx->left)
                          ? active_big_mode
                          : (ctx->left->saved_focus == FOCUS_FILE);
      right_big_mode = (ctx->active == ctx->right)
                           ? active_big_mode
                           : (ctx->right->saved_focus == FOCUS_FILE);

      ctx->active->pan_file_window = active_big_mode
                                         ? ctx->active->pan_big_file_window
                                         : ctx->active->pan_small_file_window;
      ctx->ctx_file_window = ctx->active->pan_file_window;

      DrawSplitSeparatorRow(ctx, left_big_mode, right_big_mode);

      if (!active_big_mode && ctx->active->pan_dir_window) {
        BOOL tree_highlight = (ctx->focused_window == FOCUS_TREE);
        DisplayTree(ctx, ctx->active->vol, ctx->active->pan_dir_window,
                    ctx->active->disp_begin_pos,
                    ctx->active->disp_begin_pos + ctx->active->cursor_pos,
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
          BOOL tree_highlight = (ctx->focused_window == FOCUS_TREE);
          DisplayTree(ctx, ctx->active->vol, ctx->active->pan_dir_window,
                      ctx->active->disp_begin_pos,
                      ctx->active->disp_begin_pos + ctx->active->cursor_pos,
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
    if (ctx->focused_window == FOCUS_TREE) {
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
