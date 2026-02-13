/***************************************************************************
 *
 * src/ui/display.c
 * Functions for handling the display
 *
 ***************************************************************************/


#include "ytree.h"
#include "patchlev.h"


/* PrintMenuLine is removed as its functionality for drawing the static stats panel
 * is no longer needed. The stats panel is now fully managed by stats.c. */
/* static void PrintLine(WINDOW *win, int y, int x, char *line, int len); // Removed: PrintLine is now an external function from display_utils.c */
/* static void DisplayVersion(void); // Removed: Unused function */


/* The 'mask' array is removed as the static statistics panel it defined
 * is now entirely managed by stats.c. */

/* 'extended_line' is removed as it was part of the static statistics panel
 * drawing logic, which is now obsolete. */

static char *first_line = "1-"; /* Top horizontal border line */
static char *middle_line_separator = "6-"; /* Separator line between directory and file windows */
static char *last_line = "3-"; /* Bottom horizontal border line */


/*
 * Help line definitions for different modes.
 * NOTE: These strings are manually balanced to fit on a standard 80x24
 * terminal screen. When adding or removing commands, care must be taken
 * to re-balance the two lines to prevent truncation. Commands are ordered
 * alphabetically for easier scanning.
 *
 * Updated: (F)ilespec -> (F)ilter, spacing adjustments.
 */
static char dir_help_disk_mode_0[] = "DIR       (A)ttribute A(b)out (D)elete (F)ilter   (G)roup (L)og (M)akedir";
static char dir_help_disk_mode_1[] = "COMMANDS (O)wner (P)ipe (Q)uit (R)ename (S)how (T)ag (U)ntag e(X)ec (`)dotfiles";
static char *dir_help[MAX_MODES][2] =
  {
    { /* DISK_MODE */
      dir_help_disk_mode_0,
      dir_help_disk_mode_1
    },
    { /* LL_FILE_MODE */
      "DIR       (F)ilter (^F)dirmode (L)og re(^L)oad (S)howall (T)ag (U)ntag (Q)uit",
      "COMMANDS                                                                           "
    },
    { /* ARCHIVE_MODE */
      "ARCHIVE   (C)opy (F)ilter (^F)dirmode (L)og (M)akedir (R)ename (S)howall (T)ag  ",
      "COMMANDS  (U)ntag (Q)uit                                                        "
    },
    { /* USER_MODE */
      dir_help_disk_mode_0,	/* Default unless changed by user prefs */
      dir_help_disk_mode_1
    }
  };


static char file_help_disk_mode_0[] = "FILE      (A)ttribute (C)opy/(^K) (D)elete (E)dit (F)ilter (^F)ilemode (G)roup (H)ex";
static char file_help_disk_mode_1[] = "COMMANDS  (L)og (M)ove/(^N) (O)wner (P)ipe (Q)uit (R)ename (S)ort (V)iew pathcop(Y)";
static char *file_help[MAX_MODES][2] =
  {
    { /* DISK_MODE */
      file_help_disk_mode_0,
      file_help_disk_mode_1
    },
    { /* LL_FILE_MODE */
      "FILE      (F)ilter (^F)ilemode (L)og (^L)redraw (S)ort (T)ag (U)ntag (Q)uit      ",
      "COMMANDS                                                                                "
    },
    { /* ARCHIVE_MODE */
      "ARCH-FILE (C)opy (D)elete (F)ilter (^F)mode (H)ex (R)ename (S)ort (T)ag (V)iew     ",
      "COMMANDS  (M)akedir (P)ipe (^R)nm (U)ntag pathcop(Y)                            "
    },
    { /* USER_MODE */
      file_help_disk_mode_0,	/* Default unless changed by user prefs */
      file_help_disk_mode_1
    }
  };


void DisplayDirHelp(void)
{
  int i;
  char *cptr;

  if (mode == USER_MODE) {
    if (dir_help[mode][0] == dir_help_disk_mode_0 && (cptr = DIR1) != NULL)
      dir_help[mode][0] = cptr;
    if (dir_help[mode][1] == dir_help_disk_mode_1 && (cptr = DIR2) != NULL)
      dir_help[mode][1] = cptr;
  }
  for( i=0; i < (int)(sizeof(dir_help[mode]) / sizeof(dir_help[mode][0])); i++) {
    PrintOptions( stdscr, Y_PROMPT + i, 0, dir_help[mode][i] );
    clrtoeol();
  }
}



void DisplayFileHelp(void)
{
  int i;
  char *cptr;

  if (mode == USER_MODE) {
    if (file_help[mode][0] == file_help_disk_mode_0 && (cptr = FILE1) != NULL)
      file_help[mode][0] = cptr;
    if (file_help[mode][1] == file_help[mode][1] && (cptr = FILE2) != NULL)
      file_help[mode][1] = cptr;
  }
  for( i=0; i < (int)(sizeof(file_help[mode]) / sizeof(file_help[mode][0])); i++) {
    PrintOptions( stdscr, Y_PROMPT + i, 0, file_help[mode][i] );
    clrtoeol();
  }
}

static void PrintHelpString(WINDOW *win, int y, int x, const char *str)
{
    int ch;
    int color, hi_color, lo_color;

    if (x < 0 || y < 0) return;

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
            case '(': color = hi_color; continue;
            case ')': color = lo_color; continue;
            default: break; /* Print normally */
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

void DisplayPreviewHelp(void)
{
   /*
   * Help Footer for Preview Mode (F7)
   */
    PrintHelpString(stdscr, Y_PROMPT, 0,
        "PREVIEW   (Up/Down) Select File  (PgUp/PgDn) Scroll Page  (Home/End) Jump");
    clrtoeol();
    PrintHelpString(stdscr, Y_PROMPT + 1, 0,
      "COMMANDS  (Shift) Navigate Preview  (F7) Exit Preview" );
    clrtoeol();
}

void ClearHelp(void)
{
  int i;

  for( i=0; i < 3; i++ )
  {
    wmove( stdscr, Y_MESSAGE + i, 0 ); clrtoeol();
  }
}


/*
 * DisplayHeaderPath
 * Prints the current path in the top-left header area.
 * This function is designed to be called whenever the path changes,
 * ensuring immediate visual feedback.
 */
void DisplayHeaderPath(char *path) {
    char display_buffer[PATH_LENGTH + 1]; /* Declare buffer for truncated path */
    int available_width = COLS - layout.stats_width - 26; /* COLS - "Path: " (6) - Stats Panel (24) - Clock/Margin (20) */
    if (available_width < 10) available_width = 10; /* Safety minimum */

    attron(COLOR_PAIR(CPAIR_MENU) | A_BOLD);
    mvwhline(stdscr, 0, 6, ' ', available_width); /* Explicitly clear the old path area */

    /* Use CutName for end-truncation (e.g. /home/user/verylong...) in the header */
    CutName(display_buffer, path, available_width);

    mvprintw(0, 6, "%s", display_buffer); /* Print the truncated path */
    attroff(COLOR_PAIR(CPAIR_MENU) | A_BOLD);
    refresh(); /* Ensure it draws immediately */
}


void DisplayMenu(void)
{
  int    y;
  /* int    l, c; // Removed: Unused l and c */
  /* Define L_BORDER_FOR_DISPLAY as the column index where stats.c's left vertical border begins.
   * display.c's lines must end one character before this to allow stats.c to draw its full frame. */
  const int L_BORDER_FOR_DISPLAY = COLS - layout.stats_width - 1;
  int sep_y = layout.dir_win_y + layout.dir_win_height;


  PrintSpecialString( stdscr, 0, 0, "Path: ", CPAIR_MENU );
  /* Print the current path next to the label */
  if (CurrentVolume) {
      DisplayHeaderPath((mode == ARCHIVE_MODE) ? CurrentVolume->vol_stats.login_path : CurrentVolume->vol_stats.path);
  }
  /* The clrtoeol() call was removed to prevent it from erasing the clock display on redraw. */

  werase( dir_window );
  werase( big_file_window );
  werase( small_file_window );
  if (preview_window) werase(preview_window);

  /* Draw the left vertical border for the main application window.
   * The right-hand statistics panel is now drawn entirely by stats.c. */
  for( y=1; y < layout.bottom_border_y; y++ ) { /* From y=1 up to LINES - 5 */
      PrintOptions( stdscr, y, 0, "|");
  }

  if (layout.stats_width == 0) {
      for( y=1; y < layout.bottom_border_y; y++ ) {
          mvaddch(y, COLS - 1, ACS_VLINE);
      }
  }

  /* Draw the top horizontal line of the directory window frame. */
  PrintLine( stdscr, 1, 0, first_line, L_BORDER_FOR_DISPLAY );

  /* Draw the bottom horizontal border of the main content area. */
  PrintLine( stdscr, layout.bottom_border_y, 0, last_line, L_BORDER_FOR_DISPLAY );

  /* PREVIEW MODE SEPARATOR */
  if (GlobalView->preview_mode) {
      int sep_x = layout.preview_win_x - 1;
      int top_y = 1;
      int bottom_y = layout.bottom_border_y;

#ifdef COLOR_SUPPORT
      attron(COLOR_PAIR(CPAIR_MENU) | A_BOLD);
#endif
      /* Draw Vertical Line */
      for (y = top_y + 1; y < bottom_y; y++) {
          mvaddch(y, sep_x, ACS_VLINE);
      }
      /* Draw Junctions */
      mvaddch(top_y, sep_x, ACS_TTEE);
      mvaddch(bottom_y, sep_x, ACS_BTEE);
#ifdef COLOR_SUPPORT
      attroff(COLOR_PAIR(CPAIR_MENU) | A_BOLD);
#endif
  }
  /* Split Screen Visual Separator & Horizontal Dividers */
  else if (!IsSplitScreen) {
      if (file_window == small_file_window) {
          PrintLine( stdscr, sep_y, 0, middle_line_separator, L_BORDER_FOR_DISPLAY );
      }
  } else if (LeftPanel && RightPanel) {
      int split_x = LeftPanel->dir_x + LeftPanel->dir_w;
      int top_y = 1;
      int bottom_y = layout.bottom_border_y;

      /* Determine if horizontal separators are needed for Left/Right panels */
      /* We check the Panel structs directly, not global 'file_window' */
      BOOL draw_left = (LeftPanel->pan_file_window == LeftPanel->pan_small_file_window);
      BOOL draw_right = (RightPanel->pan_file_window == RightPanel->pan_small_file_window);

#ifdef COLOR_SUPPORT
      attron(COLOR_PAIR(CPAIR_MENU) | A_BOLD);
#endif
      /* Draw Vertical Separator Line */
      for (y = top_y + 1; y < bottom_y; y++) {
          mvaddch(y, split_x, ACS_VLINE);
      }

      /* Draw Horizontal Lines first (so junctions overwrite them if needed) */
      if (draw_left) {
          mvwhline(stdscr, sep_y, 1, ACS_HLINE, split_x - 1);
          mvaddch(sep_y, 0, ACS_LTEE);
      }
      if (draw_right) {
          int right_len = L_BORDER_FOR_DISPLAY - split_x - 1;
          if (right_len > 0) {
              mvwhline(stdscr, sep_y, split_x + 1, ACS_HLINE, right_len);
              if (layout.stats_width == 0) {
                  mvaddch(sep_y, L_BORDER_FOR_DISPLAY, ACS_RTEE);
              }
          }
      }

      /* Draw Top/Bottom Junctions */
      mvaddch(top_y, split_x, ACS_TTEE);        /* Top T-Junction */
      mvaddch(bottom_y, split_x, ACS_BTEE);     /* Bottom T-Junction */

      /* Draw Center Junction Logic */
      int junction;
      if (draw_left && draw_right) junction = ACS_PLUS;
      else if (draw_left) junction = ACS_RTEE;
      else if (draw_right) junction = ACS_LTEE;
      else junction = ACS_VLINE;

      mvaddch(sep_y, split_x, junction);

#ifdef COLOR_SUPPORT
      attroff(COLOR_PAIR(CPAIR_MENU) | A_BOLD);
#endif
  }

  if (layout.stats_width == 0) {
      int right_x = COLS - 1;
      int bottom_y = layout.bottom_border_y;
      int sep_y = layout.dir_win_y + layout.dir_win_height;

      /* Draw Right Vertical Line (Content Area) */
      /* Start at y=2 (below top border) to bottom_y-1 */
      for( y=2; y < bottom_y; y++ ) {
          mvaddch(y, right_x, ACS_VLINE);
      }

      /* Explicitly Draw Corners and Junctions */
      mvaddch(1, right_x, ACS_URCORNER);
      mvaddch(bottom_y, right_x, ACS_LRCORNER);
      if (!GlobalView->preview_mode) {
          mvaddch(sep_y, right_x, ACS_RTEE);
      }
  }

  /* The loops and calls related to drawing the static statistics panel (mask, extended_line, last_line)
   * have been removed as per the decoupling objective. */

  /* Only display static logo if animation is disabled - REMOVED */

  /* DisplayVersion(); REMOVED from footer */

  touchwin( dir_window );
  refresh();
}



void SwitchToSmallFileWindow(void)
{
  /* Separator Y calculation: dir_win_y + dir_win_height */
  int separator_y = layout.dir_win_y + layout.dir_win_height;

  werase( file_window );
  /* Adjust PrintLine call to align with the new L_BORDER_FOR_DISPLAY logic */
  PrintLine( stdscr, separator_y, 0, middle_line_separator, COLS - layout.stats_width - 1 );
  /* The RTEE junction character at the stats panel border is now handled by stats.c:DrawBoxFrame */

  if (layout.stats_width == 0) {
      mvaddch(separator_y, COLS - 1, ACS_RTEE);
  }

  /* Restore Split Screen Junction if visible */
  if (IsSplitScreen && LeftPanel) {
      int split_x = LeftPanel->dir_x + LeftPanel->dir_w;
#ifdef COLOR_SUPPORT
      attron(COLOR_PAIR(CPAIR_MENU) | A_BOLD);
#endif
      mvaddch(separator_y, split_x, ACS_PLUS);
#ifdef COLOR_SUPPORT
      attroff(COLOR_PAIR(CPAIR_MENU) | A_BOLD);
#endif
  }

  file_window = small_file_window;
  GlobalView->ctx_file_window = file_window;
  if (ActivePanel) {
      ActivePanel->pan_file_window = ActivePanel->pan_small_file_window;
  }
  RefreshWindow( stdscr );
}


void SwitchToBigFileWindow(void)
{
  /* Separator Y calculation: dir_win_y + dir_win_height */
  int separator_y = layout.dir_win_y + layout.dir_win_height;

  werase( file_window );
  RefreshWindow( file_window );
#ifdef COLOR_SUPPORT
  mvaddch(separator_y, layout.dir_win_x - 1,
        ACS_VLINE | COLOR_PAIR(CPAIR_MENU)| A_BOLD);
  mvaddch(separator_y, layout.dir_win_x + layout.dir_win_width,
           ACS_VLINE | COLOR_PAIR(CPAIR_MENU)| A_BOLD);
  /* The VLINE junction character at the stats panel border is now handled by stats.c:DrawBoxFrame */

#else
  mvwaddch( stdscr, separator_y,
	   layout.dir_win_x - 1,
	   ACS_VLINE
	 );
  mvwaddch( stdscr, separator_y,
	   layout.dir_win_x + layout.dir_win_width,
	   ACS_VLINE
	 );
  /* The VLINE junction character at the stats panel border is now handled by stats.c:DrawBoxFrame */
#endif /* COLOR_SUPPORT */
  file_window = big_file_window;
  GlobalView->ctx_file_window = file_window;
  if (ActivePanel) {
      ActivePanel->pan_file_window = ActivePanel->pan_big_file_window;
  }
  RefreshWindow( stdscr );
}


void MapF2Window(void)
{
  werase( f2_window );
  box(f2_window, 0, 0);
  RefreshWindow( f2_window );
}


void UnmapF2Window(void)
{
  /* Separator Y calculation: dir_win_y + dir_win_height */
  int separator_y = layout.dir_win_y + layout.dir_win_height;

  werase( f2_window );
  if(file_window == big_file_window)
  {
#ifdef COLOR_SUPPORT
  mvaddch(separator_y, layout.dir_win_x - 1,
          ACS_VLINE | COLOR_PAIR(CPAIR_MENU)| A_BOLD);
  mvaddch(separator_y, layout.dir_win_x + layout.dir_win_width,
          ACS_VLINE | COLOR_PAIR(CPAIR_MENU)| A_BOLD);

#else
    mvwaddch( stdscr, separator_y,
	     layout.dir_win_x - 1,
	     ACS_VLINE
	   );

    mvwaddch( stdscr, separator_y,
	     layout.dir_win_x + layout.dir_win_width,
	     ACS_VLINE
	   );
#endif /* COLOR_SUPPORT */
  }
  else
  {
      PrintLine(stdscr, separator_y, 0, middle_line_separator, COLS - layout.stats_width - 1);
  }
  touchwin(stdscr);
  refresh();
}


/* PrintMenuLine function is removed as it is no longer used after decoupling
 * the static stats panel from display.c. */


void RefreshWindow(WINDOW *win)
{
	wnoutrefresh(win);
}

void RenderInactivePanel(YtreePanel *panel)
{
    /* Modified check as per instructions */
    if (!panel || !panel->vol || !panel->pan_dir_window) return;

    /* Clamp cursor */
    int total = panel->vol->total_dirs;
    /* Use Panel state, not Volume state */
    int begin = panel->disp_begin_pos;
    int cursor = panel->cursor_pos;

    if (total > 0 && (begin + cursor >= total)) {
        /* Fix invalid position if any */
        begin = 0; cursor = 0;
    }

    /* Draw Tree */
    /* Use the window pointers from the panel */
    if (panel->pan_dir_window) {
        DisplayTree(panel->vol, panel->pan_dir_window, begin, begin + cursor, FALSE);
        wnoutrefresh(panel->pan_dir_window);
    }

    /* Draw File List */
    if (panel->pan_file_window && total > 0) {
        int idx = begin + cursor;
        if (idx >= 0 && idx < total) {
            DirEntry *de = panel->vol->dir_entry_list[idx].dir_entry;
            /* Ensure we have a valid entry and matching list to display */
            if (de && panel->file_entry_list) {
                /* Optimization: Render existing list without rebuilding using DisplayFiles directly.
                 * This avoids rescanning the directory on the inactive panel.
                 * We use start_x = 0 because the window (pan_file_window) is already positioned.
                 * We use -1 for hilight_no to avoid highlighting files in the inactive panel.
                 */
                DisplayFiles(panel,
                             de,
                             panel->start_file,
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
void RefreshGlobalView(DirEntry *dir_entry)
{
    Statistic *s = &CurrentVolume->vol_stats;

    if (ActivePanel == NULL) ERROR_MSG("FATAL: RefreshGlobalView called with NULL ActivePanel");
    if (ActivePanel->pan_dir_window == NULL) ERROR_MSG("FATAL: ActivePanel window is NULL in Mode: %d", GlobalView->preview_mode);

    /* 1. Re-evaluate Layout (Geometry) */
    ReCreateWindows();

    /* Update Global View Context to match Active Panel immediately after recreation */
    if (ActivePanel) {
        if (GlobalView->preview_mode && LeftPanel) {
            /* In Preview Mode, the directory context (navigation) should always
             * anchor to the LeftPanel. This prevents stale pointers if ActivePanel
             * logic drifts or if focus is ambiguous during mode switches. */
            GlobalView->ctx_dir_window = LeftPanel->pan_dir_window;
        } else {
            GlobalView->ctx_dir_window = ActivePanel->pan_dir_window;
        }

        GlobalView->ctx_small_file_window = ActivePanel->pan_small_file_window;
        GlobalView->ctx_big_file_window = ActivePanel->pan_big_file_window;
        GlobalView->ctx_file_window = ActivePanel->pan_file_window;
    }

    /* 2. Draw Borders and Static Frames */
    DisplayMenu();

    /* 3. Draw Stats Panel (Restores Right Borders) */
    if (!GlobalView->preview_mode) {
        DisplayDiskStatistic(s);
        /* If global mode, stats might be special */
        DisplayDirStatistic(dir_entry, (dir_entry->global_flag) ? "SHOW ALL" : NULL, s);
        DisplayAvailBytes(s);
    }

    /* 4. Update Header Path (Critical Fix for Stale Paths) */
    {
        char path[PATH_LENGTH];
        GetPath(dir_entry, path);
        DisplayHeaderPath(path);
    }

    /* 5. Draw Content based on Mode */
    if (GlobalView->preview_mode) {
        /* PREVIEW MODE */
        SwitchToBigFileWindow();
        DisplayFileWindow(ActivePanel, dir_entry);
        /* Note: UpdatePreview must be called by caller or we need a callback */
    } else if (dir_entry->big_window || dir_entry->global_flag || dir_entry->tagged_flag) {
        /* BIG WINDOW MODE */
        SwitchToBigFileWindow();
        DisplayFileWindow(ActivePanel, dir_entry);
    } else {
        /* STANDARD SPLIT MODE */
        SwitchToSmallFileWindow();

        /* Bug Fix: Explicitly ensure BOTH panels are reset for the small window layout
         * if we are not in a global/big mode. This prevents the "stuck" big window
         * on the inactive panel after a switch. */
        if (IsSplitScreen) {
            LeftPanel->pan_file_window = LeftPanel->pan_small_file_window;
            RightPanel->pan_file_window = RightPanel->pan_small_file_window;
        }

        /* Draw Directory Tree */
        if (ActivePanel && ActivePanel->pan_dir_window) {
            /* Focus Unification: Only highlight tree if focused_window is FOCUS_TREE */
            BOOL tree_highlight = (GlobalView->focused_window == FOCUS_TREE);
            DisplayTree(ActivePanel->vol, ActivePanel->pan_dir_window,
                        ActivePanel->disp_begin_pos,
                        ActivePanel->disp_begin_pos + ActivePanel->cursor_pos,
                        tree_highlight);
        }

        /* Draw File List */
        /* Note: DisplayFileWindow handles focus checking internally via ctrl_file.c logic */
        DisplayFileWindow(ActivePanel, dir_entry);
    }

    /* 6. Render Inactive Panel (Critical Fix for Split Screen Blanking) */
    if (IsSplitScreen && ActivePanel) {
        YtreePanel *inactive = (ActivePanel == LeftPanel) ? RightPanel : LeftPanel;
        RenderInactivePanel(inactive);
    }

    /* 7. Update Footer Help */
    if (GlobalView->preview_mode) {
        DisplayPreviewHelp();
    } else {
        /* In file mode context */
        if (file_window == stdscr || GlobalView->ctx_file_window == file_window) {
             /* Basic clear, specific help drawn by input loop */
             ClearHelp();
        }
    }

    /* 8. Physical Cursor Correction: Ensure cursor stays on active element */
    if (ActivePanel) {
        if (GlobalView->focused_window == FOCUS_TREE && ActivePanel->pan_dir_window) {
            wnoutrefresh(ActivePanel->pan_dir_window);
        } else if (GlobalView->focused_window == FOCUS_FILE && ActivePanel->pan_file_window) {
            wnoutrefresh(ActivePanel->pan_file_window);
        }
    }

    /* 9. Commit Changes */
    doupdate();
}