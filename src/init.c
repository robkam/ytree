/***************************************************************************
 *
 * init.c
 * Application Initialization and Layout Management
 *
 * Handles ncurses startup, configuration loading (profile/history),
 * and dynamic window geometry calculation.
 *
 ***************************************************************************/


#include "ytree.h"
#include "watcher.h"


static WINDOW *Subwin(WINDOW *orig, int nlines, int ncols,
                      int begin_y, int begin_x);
static WINDOW *Newwin(int nlines, int ncols,
                      int begin_y, int begin_x);

#ifdef XCURSES
char *XCursesProgramName = "ytree";
#endif

void Layout_Recalculate(void)
{
    /*
     * Calculate available vertical space for windows.
     * Top Border is at row 1. Windows start at row 2.
     * Bottom Border is at row LINES-4. Windows end at row LINES-5.
     * Available height = (LINES - 4) - 2 = LINES - 6.
     */
    int available_height = LINES - 6;
    if (available_height < 1) available_height = 1;

    if (GlobalView && GlobalView->show_stats) {
        layout.stats_width = 24;
    } else {
        layout.stats_width = 0;
    }
    int stats_margin = 2;
    layout.main_win_width = COLS - layout.stats_width - stats_margin;

    layout.dir_win_x = 1;
    layout.dir_win_y = 2;
    layout.dir_win_width = layout.main_win_width;

    /* Approx 60% of available height */
    layout.dir_win_height = (available_height * 6) / 10;
    if (layout.dir_win_height < 1) layout.dir_win_height = 1;

    /* Calculate Separator Y (virtual) to position small file window */
    int separator_y = layout.dir_win_y + layout.dir_win_height;

    layout.small_file_win_x = 1;
    layout.small_file_win_y = separator_y + 1;
    layout.small_file_win_width = layout.main_win_width;

    /* Remaining height for file window (Available - DirHeight - Separator(1)) */
    layout.small_file_win_height = available_height - layout.dir_win_height - 1;
    if (layout.small_file_win_height < 1) layout.small_file_win_height = 1;

    layout.big_file_win_x = 1;
    layout.big_file_win_y = 2;
    layout.big_file_win_width = layout.main_win_width;
    layout.big_file_win_height = available_height; /* Uses full available height */
}

int Init(char *configuration_file, char *history_file)
{
  char buffer[PATH_LENGTH + 1];
  char *home = NULL;

  /* Allocate and initialize the first volume using the dedicated module */
  CurrentVolume = Volume_Create();

  /* Allocate Global ViewContext for window management */
  GlobalView = (ViewContext *)calloc(1, sizeof(ViewContext));
  if (!GlobalView) {
      fprintf(stderr, "Fatal Error: Memory allocation failed for GlobalView.\n");
      exit(1);
  }
  GlobalView->show_stats = TRUE;
  GlobalView->fixed_col_width = 0; /* ADDED */

  /* Initialize global mode default */
  GlobalView->view_mode = DISK_MODE;

  /* Use setlocale to correctly initialize for WITH_UTF8 or system locale */
  setlocale(LC_ALL, "");

  user_umask = umask(0);
  initscr();
  Layout_Recalculate();
  StartColors(); /* even on b/w terminals... */

  cbreak();
  noecho();
  nonl();
  raw();
  keypad( stdscr, TRUE );
  clearok(stdscr, TRUE);
  leaveok(stdscr,FALSE);
  curs_set(0);


  WbkgdSet( stdscr, COLOR_PAIR(CPAIR_WINDIR)|A_BOLD);

  ReCreateWindows();

  /* Use the simpler constant value */
  if( baudrate() >= QUICK_BAUD_RATE ) typeahead( -1 );

  file_window = small_file_window;

  if( ReadGroupEntries() )
  {
    ERROR_MSG( "ReadGroupEntries failed*ABORT" );
    exit( 1 );
  }

  if( ReadPasswdEntries() )
  {
    ERROR_MSG( "ReadPasswdEntries failed*ABORT" );
    exit( 1 );
  }

  if (configuration_file != NULL) {
    ReadProfile(configuration_file);
  }
  else if( ( home = getenv("HOME") ) ) {
    snprintf(buffer, sizeof(buffer), "%s%c%s", home, FILE_SEPARATOR_CHAR, PROFILE_FILENAME);
    ReadProfile(buffer);
  }
  if (history_file != NULL) {
    ReadHistory(history_file);
  }
  else if ( home ) {
    snprintf(buffer, sizeof(buffer), "%s%c%s", home, FILE_SEPARATOR_CHAR, HISTORY_FILENAME);
    ReadHistory(buffer);
  }

  ReinitColorPairs();

  SetFileMode( strtol(FILEMODE, NULL, 0) );
  SetKindOfSort( SORT_BY_NAME, &CurrentVolume->vol_stats );
  /* Use System Locale for number separator */
  struct lconv *lc = localeconv();
  if (lc && lc->thousands_sep && *lc->thousands_sep)
      number_seperator = *lc->thousands_sep;
  else
      number_seperator = ','; /* Fallback to English/Comma */
  bypass_small_window = (strtol(NOSMALLWINDOW, NULL, 0 )) ? TRUE : FALSE;
  highlight_full_line = (strtol(GetProfileValue("HIGHLIGHT_FULL_LINE"), NULL, 0)) ? TRUE : FALSE;
  hide_dot_files = (strtol(HIDEDOTFILES, NULL, 0)) ? TRUE : FALSE;
  animation_method = strtol(GetProfileValue("ANIMATION"), NULL, 0);
  initial_directory = INITIALDIR;

  InitClock();
  Watcher_Init();

  return( 0 );
}



void ReCreateWindows()
{
  BOOL is_small;

  Layout_Recalculate();

  is_small = (file_window == small_file_window) ? TRUE : FALSE;


  if(dir_window)
    delwin(dir_window);

  dir_window = Subwin( stdscr,
		       layout.dir_win_height,
		       layout.dir_win_width,
		       layout.dir_win_y,
		       layout.dir_win_x
		      );

  keypad( dir_window, TRUE );
  scrollok( dir_window, TRUE );
  clearok( dir_window, TRUE);
  leaveok(dir_window, TRUE);
  WbkgdSet( dir_window, COLOR_PAIR(CPAIR_WINDIR) );

  if(small_file_window)
    delwin(small_file_window);

  small_file_window = Subwin( stdscr,
			      layout.small_file_win_height,
			      layout.small_file_win_width,
			      layout.small_file_win_y,
		              layout.small_file_win_x
		           );

  if(!small_file_window)
    beep();

  keypad( small_file_window, TRUE );
  clearok(small_file_window, TRUE);
  leaveok(small_file_window, TRUE);

  WbkgdSet(small_file_window, COLOR_PAIR(CPAIR_WINFILE));

  if(big_file_window)
    delwin(big_file_window);

  big_file_window = Subwin( stdscr,
			    layout.big_file_win_height,
			    layout.big_file_win_width,
			    layout.big_file_win_y,
		            layout.big_file_win_x
		          );

  keypad( big_file_window, TRUE );
  clearok(big_file_window, TRUE);
  leaveok(big_file_window, TRUE);
  WbkgdSet(big_file_window, COLOR_PAIR(CPAIR_WINFILE));

  if(error_window)
    delwin(error_window);

  error_window = Newwin(
		       ERROR_WINDOW_HEIGHT,
		       ERROR_WINDOW_WIDTH,
		       ERROR_WINDOW_Y,
		       ERROR_WINDOW_X
		      );
  WbkgdSet(error_window, COLOR_PAIR(CPAIR_WINERR));
  clearok(error_window, TRUE);
  leaveok(error_window, TRUE);


#ifdef CLOCK_SUPPORT

  if(time_window)
    delwin(time_window);

  time_window = Subwin( stdscr,
                      TIME_WINDOW_HEIGHT,
                      TIME_WINDOW_WIDTH,
                      TIME_WINDOW_Y,
                      TIME_WINDOW_X
                    );
  clearok( time_window, TRUE );
  scrollok( time_window, FALSE );
  leaveok( time_window, TRUE );
  WbkgdSet( time_window, COLOR_PAIR(CPAIR_WINDIR|A_BOLD) );
  immedok(time_window, TRUE);
#endif


  if(history_window)
    delwin(history_window);

  history_window = Newwin(
                       HISTORY_WINDOW_HEIGHT,
                       HISTORY_WINDOW_WIDTH,
                       HISTORY_WINDOW_Y,
                       HISTORY_WINDOW_X
                      );
  scrollok(history_window, TRUE);
  clearok(history_window, TRUE );
  leaveok(history_window, TRUE);
  WbkgdSet(history_window, COLOR_PAIR(CPAIR_WINHST));

  matches_window = history_window;

  if(f2_window)
    delwin(f2_window);

  f2_window = Newwin( F2_WINDOW_HEIGHT,
                      F2_WINDOW_WIDTH,
                      F2_WINDOW_Y,
                      F2_WINDOW_X
                    );

  keypad( f2_window, TRUE );
  scrollok( f2_window, FALSE );
  clearok( f2_window, TRUE);
  leaveok( f2_window, TRUE );
  WbkgdSet( f2_window, COLOR_PAIR(CPAIR_WINHST) );

  file_window = (is_small) ? small_file_window : big_file_window;

  clear();
}


static WINDOW *Subwin(WINDOW *orig, int nlines, int ncols,
                      int begin_y, int begin_x)
{
  int x, y, h, w;
  WINDOW *win;

  if(nlines > LINES)   nlines = LINES;
  if(ncols > COLS)     ncols = COLS;

  h = MAXIMUM(nlines, 1);
  w = MAXIMUM(ncols, 1);
  x = MAXIMUM(begin_x, 0);
  y = MAXIMUM(begin_y, 0);

  if(x+w > COLS)  x = COLS - w;
  if(y+h > LINES) y = LINES - h;

  win = subwin(orig, h, w, y, x);

  return(win);
}



static WINDOW *Newwin(int nlines, int ncols,
                      int begin_y, int begin_x)
{
  int x, y, h, w;
  WINDOW *win;

  if(nlines > LINES)   nlines = LINES;
  if(ncols > COLS)     ncols = COLS;

  h = MAXIMUM(nlines, 1);
  w = MAXIMUM(ncols, 1);
  x = MAXIMUM(begin_x, 0);
  y = MAXIMUM(begin_y, 0);

  if(x+w > COLS)  x = COLS - w;
  if(y+h > LINES) y = LINES - h;

  win = newwin(h, w, y, x);

  return(win);
}