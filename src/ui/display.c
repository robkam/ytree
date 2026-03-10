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
    "DIR (A)ttribute (B)rief (D)elete (F)ilter (G)roup (L)og (M)akedir "
    "(N)ewfile";
static char dir_help_disk_mode_1[] =
    "COMMANDS (O)wner (P)ipe (Q)uit (R)ename "
    "(S)howall (T)ag (U)ntag e(X)ec (`)dotfiles";
static char *dir_help[MAX_MODES][2] = {
    {/* DISK_MODE */
     dir_help_disk_mode_0, dir_help_disk_mode_1},
    {/* LL_FILE_MODE */
     "DIR       (F)ilter (^F)dirmode (L)og re(^L)oad (S)howall (T)ag (U)ntag "
     "(Q)uit",
     "COMMANDS                                                                 "
     "          "},
    {/* ARCHIVE_MODE */
     "ARCHIVE   (C)opy (F)ilter (^F)dirmode (L)og (M)akedir (R)ename (S)howall "
     "(T)ag  ",
     "COMMANDS  (U)ntag (Q)uit                                                 "
     "       "},
    {                      /* USER_MODE */
     dir_help_disk_mode_0, /* Default unless changed by user prefs */
     dir_help_disk_mode_1}};

static char file_help_disk_mode_0[] =
    "FILE      (A)ttribute (B)rief (C)opy/(^K) (D)elete (E)dit (F)ilter "
    "(^F)ilemode "
    "(G)roup";
static char file_help_disk_mode_1[] =
    "COMMANDS (H)ex (L)og (M)ove/(^N) (N)ewfile "
    "(O)wner (P)ipe (Q)uit (R)ename (S)ort";
static char *file_help[MAX_MODES][2] = {
    {/* DISK_MODE */
     file_help_disk_mode_0, file_help_disk_mode_1},
    {/* LL_FILE_MODE */
     "FILE      (F)ilter (^F)ilemode (L)og (^L)redraw (S)ort (T)ag (U)ntag "
     "(Q)uit      ",
     "COMMANDS                                                                 "
     "               "},
    {/* ARCHIVE_MODE */
     "ARCH-FILE (C)opy (D)elete (F)ilter (^F)mode (H)ex (R)ename (S)ort (T)ag "
     "(V)iew     ",
     "COMMANDS  (M)akedir (P)ipe (^R)nm (U)ntag pathcop(Y)                     "
     "       "},
    {                       /* USER_MODE */
     file_help_disk_mode_0, /* Default unless changed by user prefs */
     file_help_disk_mode_1}};

void DisplayDirHelp(ViewContext *ctx) {
  int i;
  char *cptr;

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
  wnoutrefresh(ctx->ctx_menu_window);
}

void DisplayFileHelp(ViewContext *ctx) {
  int i;
  char *cptr;

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
  wnoutrefresh(ctx->ctx_menu_window);
}

static void PrintHelpString(WINDOW *win, int y, int x, const char *str) {
  int ch;
  int color, hi_color, lo_color;

  if (x < 0 || y < 0)
    return;

#ifdef COLOR_SUPPORT
  lo_color = CPAIR_MENU;
  hi_color = CPAIR_HIMENUS;
#else
  lo_color = A_NORMAL;
  hi_color = A_BOLD;
#endif

  color = lo_color;
  wmove(win, y, x);

  for (; *str; str++) {
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
    wattrset(win, COLOR_PAIR(color) | A_BOLD);
#else
    wattrset(win, color);
#endif
    waddch(win, ch);
  }
  wattrset(win, 0);
}

void DisplayPreviewHelp(ViewContext *ctx) {
  /*
   * Help Footer for Preview Mode (F7)
   */
  PrintHelpString(ctx->ctx_border_window, Y_PROMPT(ctx), 0,
                  "PREVIEW   (Up/Down) Select File  (PgUp/PgDn) Scroll Page  "
                  "(Home/End) Jump");
  wmove(ctx->ctx_border_window, Y_PROMPT(ctx), 0);
  wclrtoeol(ctx->ctx_border_window);
  PrintHelpString(ctx->ctx_border_window, Y_PROMPT(ctx) + 1, 0,
                  "COMMANDS  (Shift) Navigate Preview  (F7) Exit Preview");
  wmove(ctx->ctx_border_window, Y_PROMPT(ctx) + 1, 0);
  wclrtoeol(ctx->ctx_border_window);
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
void DisplayHeaderPath(ViewContext *ctx, char *path) {
  char display_buffer[PATH_LENGTH + 1];
  int available_width =
      COLS - ctx->layout.stats_width -
      26; /* COLS - "Path: " (6) - Stats Panel (24) - Clock/Margin (20) */
  if (available_width < 10)
    available_width = 10;

  CutPathname(display_buffer, path, available_width);

  DEBUG_LOG("DisplayHeaderPath: path='%s' cut='%s' avail=%d COLS=%d", path,
            display_buffer, available_width, COLS);

  /* Write path with fixed width to ensure old content is cleared */
  mvwprintw(ctx->ctx_border_window, 0, 6, "%-*s", available_width,
            display_buffer);
  wattrset(ctx->ctx_border_window, A_NORMAL);
  wnoutrefresh(ctx->ctx_border_window);
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

  /* Draw Tree */
  /* Use the window pointers from the panel */
  if (panel->pan_dir_window) {
    DisplayTree(ctx, panel->vol, panel->pan_dir_window, begin, begin + cursor,
                FALSE);
    wnoutrefresh(panel->pan_dir_window);
  }

  /* Draw File List */
  if (panel->pan_file_window && total > 0) {
    int idx = begin + cursor;
    if (idx >= 0 && idx < total) {
      DirEntry *de = panel->vol->dir_entry_list[idx].dir_entry;
      /* Ensure we have a valid entry and matching list to display */
      if (de && panel->file_entry_list) {
        /* Optimization: Render existing list without rebuilding using
         * DisplayFiles directly. This avoids rescanning the directory on the
         * inactive panel. We use start_x = 0 because the window
         * (pan_file_window) is already positioned. We use -1 for hilight_no to
         * avoid highlighting files in the inactive panel.
         */
        DisplayFiles(ctx, panel, de, panel->start_file,
                     -1, /* No highlight in inactive panel */
                     0,  /* start_x inside the window */
                     panel->pan_file_window);

        wnoutrefresh(panel->pan_file_window);
      }
    }
  }
}

/*
 * CENTRALIZED REDRAW FUNCTION
 * Handles the complexities of Split/Big/Preview modes in one place.
 * Use this to ensure all borders, stats, and content are consistent.
 */
void RefreshView(ViewContext *ctx, DirEntry *dir_entry) {
  Statistic *s = &ctx->active->vol->vol_stats;

  if (ctx->active == NULL)
    MESSAGE(ctx, "FATAL: RefreshView called with NULL ctx->active");

  /* 1. Re-evaluate Layout (Geometry) */
  ReCreateWindows(ctx);

  /* 2. BACKGROUND REFRESH (z=-1) */
  touchwin(stdscr);
  wnoutrefresh(stdscr);

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
    /* In tree mode (small window), show parent directory path.
     * In file mode (big window), show current directory path. */
    if (dir_entry->big_window || dir_entry->global_flag ||
        dir_entry->tagged_flag) {
      /* File window mode: show the directory we're viewing files in */
      GetPath(dir_entry, path);
    } else if (dir_entry->up_tree) {
      /* Tree mode: show the parent directory path (where we ARE, not what's
       * selected) */
      GetPath(dir_entry->up_tree, path);
    } else {
      /* Root directory has no parent */
      GetPath(dir_entry, path);
    }
    DisplayHeaderPath(ctx, path);
  }

  /* 7. Draw Content Panels THIRD (z=1) */
  if (ctx->preview_mode) {
    SwitchToBigFileWindow(ctx);
    DisplayFileWindow(ctx, ctx->active, dir_entry);
    if (ctx->ctx_preview_window)
      wnoutrefresh(ctx->ctx_preview_window);
  } else if (dir_entry->big_window || dir_entry->global_flag ||
             dir_entry->tagged_flag) {
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

  if (ctx->is_split_screen && ctx->active) {
    YtreePanel *inactive = (ctx->active == ctx->left) ? ctx->right : ctx->left;
    RenderInactivePanel(ctx, inactive);
  }

  /* 8. Update Footer Help and Refresh Menu Window LAST (z=2) */
  if (ctx->preview_mode) {
    DisplayPreviewHelp(ctx);
  } else {
    if (ctx->focused_window == FOCUS_TREE) {
      DisplayDirHelp(ctx);
    } else {
      DisplayFileHelp(ctx);
    }
    if (ctx->ctx_menu_window)
      wnoutrefresh(ctx->ctx_menu_window);
  }

  UI_Dialog_RefreshAll(ctx);
  doupdate();
}