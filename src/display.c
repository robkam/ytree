/***************************************************************************
 *
 * display.c
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

/* Mapped '██' to '#' and '░░' to ':' for ncurses ACS character rendering */
static char *logo[] = {
"             ###                                 ",
"            :###                                 ",
" ###  ###  #######    ######   ######    ######  ",
":### :### :::###     :###::## :###::### :###::###",
":### :###   :###     ###  ::  #######:  #######: ",
":### :###   :### ## :###     :###:::   :###:::   ",
"::#######   ::##### :###     ::######  ::######  ",
" :::::###    :::::  :::       ::::::    ::::::   ",
"  ## :###                                        ",
" :######                                         ",
" ::::::                                          "
		      };

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
static char dir_help_disk_mode_1[] = "COMMANDS  (O)wner (Q)uit (R)ename (S)howall (T)ag (U)ntag e(X)ecute (`)dotfiles";
static char *dir_help[MAX_MODES][2] =
  {
    { /* DISK_MODE */
      dir_help_disk_mode_0,
      dir_help_disk_mode_1
    },
    { /* LL_FILE_MODE */
      "DIR       (F)ilter   (^F)dirmode (L)og (^L)redraw (S)howall (T)ag (U)ntag (Q)uit",
      "COMMANDS                                                                           "
    },
    { /* ARCHIVE_MODE */
      "ARCHIVE   (F)ilter   (^F)dirmode (L)og (^L)redraw (S)howall (T)ag (U)ntag (Q)uit",
      "COMMANDS                                                                           "
    },
    { /* USER_MODE */
      dir_help_disk_mode_0,	/* Default unless changed by user prefs */
      dir_help_disk_mode_1
    }
  };


static char file_help_disk_mode_0[] = "FILE      (A)ttribute (C)opy/(^K) (D)elete (E)dit (F)ilter   (^F)ilemode (G)roup (H)ex";
static char file_help_disk_mode_1[] = "COMMANDS  (L)og (M)ove/(^N) (O)wner (P)ipe (Q)uit (R)ename (S)ort (V)iew pathcop(Y)";
static char *file_help[MAX_MODES][2] =
  {
    { /* DISK_MODE */
      file_help_disk_mode_0,
      file_help_disk_mode_1
    },
    { /* LL_FILE_MODE */
      "FILE      (F)ilter   (^F)ilemode (L)og (^L)redraw (S)ort (T)ag (U)ntag (Q)uit      ",
      "COMMANDS                                                                                "
    },
    { /* ARCHIVE_MODE */
      "ARCH-FILE (C)opy (F)ilter   (^F)ilemode (H)ex (P)ipe (S)ort (T)ag (U)ntag (V)iew (Q)uit",
      "COMMANDS                                                                               "
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
    int available_width = COLS - STATS_WIDTH - 26; /* COLS - "Path: " (6) - Stats Panel (24) - Clock/Margin (20) */
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
  /* int    l, c; // Removed: Unused l */
  int    c;
  /* Define L_BORDER_FOR_DISPLAY as the column index where stats.c's left vertical border begins.
   * display.c's lines must end one character before this to allow stats.c to draw its full frame. */
  const int L_BORDER_FOR_DISPLAY = COLS - STATS_WIDTH - 1;


  PrintSpecialString( stdscr, 0, 0, "Path: ", CPAIR_MENU );
  /* Print the current path next to the label */
  DisplayHeaderPath((mode == ARCHIVE_MODE) ? statistic.login_path : statistic.path);
  /* The clrtoeol() call was removed to prevent it from erasing the clock display on redraw. */

  werase( dir_window );
  werase( big_file_window );
  werase( small_file_window );

  /* Draw the left vertical border for the main application window.
   * The right-hand statistics panel is now drawn entirely by stats.c. */
  for( y=1; y < LINES - 4; y++ ) { /* From y=1 up to LINES - 5 */
      PrintOptions( stdscr, y, 0, "|");
  }

  /* Draw the top horizontal line of the directory window frame. */
  PrintLine( stdscr, 1, 0, first_line, L_BORDER_FOR_DISPLAY );

  /* Draw the horizontal separator line between the directory and file windows. */
  PrintLine( stdscr, DIR_WINDOW_HEIGHT + 2, 0, middle_line_separator, L_BORDER_FOR_DISPLAY );

  /* Draw the bottom horizontal border of the main content area. */
  PrintLine( stdscr, LINES - 4, 0, last_line, L_BORDER_FOR_DISPLAY );

  /* The loops and calls related to drawing the static statistics panel (mask, extended_line, last_line)
   * have been removed as per the decoupling objective. */

  /* Only display static logo if animation is disabled */
  if (animation_method == 0) {
      char version[80];
      int logo_rows = sizeof(logo) / sizeof(logo[0]);
      int start_y = (DIR_WINDOW_HEIGHT - logo_rows) / 2;

      /* Print Logo Centered */
      for( y=0; y < logo_rows; y++ )
      {
        int start_x = (DIR_WINDOW_WIDTH - strlen(logo[y])) / 2;
        char *p = logo[y];
        wmove(dir_window, start_y + y, start_x);
        while (*p != '\0') {
            if (*p == '#') {
                wattron(dir_window, A_REVERSE);
                waddch(dir_window, ' ');
                wattroff(dir_window, A_REVERSE);
            } else if (*p == ':') {
                waddch(dir_window, ACS_CKBOARD);
            } else {
                waddch(dir_window, ' ');
            }
            p++;
        }
      }

      /* Print Version Below Logo */
      (void) snprintf( version, sizeof(version),
              "ytree Version %sPL%d, %s",
              VERSION,
              PATCHLEVEL,
              VERSIONDATE
            );
      c = strlen(version);

      /* Ensure it fits and position it 2 lines below logo */
      if (start_y + logo_rows + 2 < DIR_WINDOW_HEIGHT) {
          MvWAddStr( dir_window,
             start_y + logo_rows + 2,
             (DIR_WINDOW_WIDTH - c) / 2,
             version
           );
      }
  }

  /* DisplayVersion(); REMOVED from footer */

  touchwin( dir_window );
  /* refresh(); */
}



void SwitchToSmallFileWindow(void)
{
  werase( file_window );
  /* Adjust PrintLine call to align with the new L_BORDER_FOR_DISPLAY logic */
  PrintLine( stdscr, DIR_WINDOW_HEIGHT + 2, 0, middle_line_separator, COLS - STATS_WIDTH - 1 );
  /* The RTEE junction character at the stats panel border is now handled by stats.c:DrawBoxFrame */
  file_window = small_file_window;
  RefreshWindow( stdscr );
}


void SwitchToBigFileWindow(void)
{
  werase( file_window );
  RefreshWindow( file_window );
#ifdef COLOR_SUPPORT
  mvaddch(DIR_WINDOW_Y + DIR_WINDOW_HEIGHT, DIR_WINDOW_X - 1,
        ACS_VLINE | COLOR_PAIR(CPAIR_MENU)| A_BOLD);
  mvaddch(DIR_WINDOW_Y + DIR_WINDOW_HEIGHT, DIR_WINDOW_X + DIR_WINDOW_WIDTH,
           ACS_VLINE | COLOR_PAIR(CPAIR_MENU)| A_BOLD);
  /* The VLINE junction character at the stats panel border is now handled by stats.c:DrawBoxFrame */

#else
  mvwaddch( stdscr, DIR_WINDOW_Y + DIR_WINDOW_HEIGHT,
	   DIR_WINDOW_X - 1,
	   ACS_VLINE
	 );
  mvwaddch( stdscr, DIR_WINDOW_Y + DIR_WINDOW_HEIGHT,
	   DIR_WINDOW_X + DIR_WINDOW_WIDTH,
	   ACS_VLINE
	 );
  /* The VLINE junction character at the stats panel border is now handled by stats.c:DrawBoxFrame */
#endif /* COLOR_SUPPORT */
  file_window = big_file_window;
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
  werase( f2_window );
  if(file_window == big_file_window)
  {
#ifdef COLOR_SUPPORT
  mvaddch(DIR_WINDOW_Y + DIR_WINDOW_HEIGHT, DIR_WINDOW_X - 1,
          ACS_VLINE | COLOR_PAIR(CPAIR_MENU)| A_BOLD);
  mvaddch(DIR_WINDOW_Y + DIR_WINDOW_HEIGHT, DIR_WINDOW_X + DIR_WINDOW_WIDTH,
          ACS_VLINE | COLOR_PAIR(CPAIR_MENU)| A_BOLD);

#else
    mvwaddch( stdscr, DIR_WINDOW_Y + DIR_WINDOW_HEIGHT,
	     DIR_WINDOW_X - 1,
	     ACS_VLINE
	   );

    mvwaddch( stdscr, DIR_WINDOW_Y + DIR_WINDOW_HEIGHT,
	     DIR_WINDOW_X + DIR_WINDOW_WIDTH,
	     ACS_VLINE
	   );
#endif /* COLOR_SUPPORT */
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