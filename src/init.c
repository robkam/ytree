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

    /*
     * Stats Panel Logic:
     * If SplitScreen is active, force stats off to save space.
     * Otherwise respect user preference.
     */
    if (IsSplitScreen) {
        layout.stats_width = 0;
    } else {
        if (GlobalView && GlobalView->show_stats) {
            layout.stats_width = 24;
        } else {
            layout.stats_width = 0;
        }
    }

    int stats_margin = (layout.stats_width > 0) ? 2 : 0;
    layout.main_win_width = (layout.stats_width > 0) ? (COLS - layout.stats_width - 2) : (COLS - 2);

    /* Left Panel Geometry (Always active) */
    int panel_width;
    if (IsSplitScreen) {
        /* Reserve space for separator (the -1) */
        panel_width = (layout.main_win_width - 1) / 2;
    } else {
        panel_width = layout.main_win_width;
    }

    /* Common Heights */
    int dir_h = (available_height * 6) / 10;
    if (dir_h < 1) dir_h = 1;
    int small_file_h = available_height - dir_h - 1;
    if (small_file_h < 1) small_file_h = 1;

    /* Update Global Layout struct (used by display.c etc) */
    layout.dir_win_x = 1;
    layout.dir_win_y = 2;
    layout.dir_win_width = panel_width;
    layout.dir_win_height = dir_h;

    layout.small_file_win_x = 1;
    layout.small_file_win_y = layout.dir_win_y + dir_h + 1;
    layout.small_file_win_width = panel_width;
    layout.small_file_win_height = small_file_h;

    layout.big_file_win_x = 1;
    layout.big_file_win_y = 2;
    layout.big_file_win_width = panel_width;
    layout.big_file_win_height = available_height;

    /* Update LeftPanel Geometry */
    if (LeftPanel) {
        LeftPanel->dir_x = 1;
        LeftPanel->dir_y = 2;
        LeftPanel->dir_w = panel_width;
        LeftPanel->dir_h = dir_h;

        LeftPanel->small_file_x = 1;
        LeftPanel->small_file_y = LeftPanel->dir_y + dir_h + 1;
        LeftPanel->small_file_w = panel_width;
        LeftPanel->small_file_h = small_file_h;

        LeftPanel->big_file_x = 1;
        LeftPanel->big_file_y = 2;
        LeftPanel->big_file_w = panel_width;
        LeftPanel->big_file_h = available_height;
    }

    /* Update RightPanel Geometry */
    if (RightPanel && IsSplitScreen) {
        /* Right panel starts after left panel + separator */
        int right_x = layout.dir_win_x + panel_width + 1;

        RightPanel->dir_x = right_x;
        RightPanel->dir_y = 2;
        RightPanel->dir_w = layout.main_win_width - panel_width - 1; /* Remainder */
        RightPanel->dir_h = dir_h;

        RightPanel->small_file_x = right_x;
        RightPanel->small_file_y = RightPanel->dir_y + dir_h + 1;
        RightPanel->small_file_w = RightPanel->dir_w;
        RightPanel->small_file_h = small_file_h;

        RightPanel->big_file_x = right_x;
        RightPanel->big_file_y = 2;
        RightPanel->big_file_w = RightPanel->dir_w;
        RightPanel->big_file_h = available_height;
    }
}

int Init(char *configuration_file, char *history_file)
{
  char buffer[PATH_LENGTH + 1];
  char *home = NULL;

  /* Allocate Global ViewContext for window management */
  GlobalView = (ViewContext *)calloc(1, sizeof(ViewContext));
  if (!GlobalView) {
      fprintf(stderr, "Fatal Error: Memory allocation failed for GlobalView.\n");
      exit(1);
  }

  /* Allocate Panels */
  LeftPanel = (YtreePanel *)calloc(1, sizeof(YtreePanel));
  RightPanel = (YtreePanel *)calloc(1, sizeof(YtreePanel));
  if (!LeftPanel || !RightPanel) {
      fprintf(stderr, "Fatal Error: Memory allocation failed for Panels.\n");
      exit(1);
  }

  /* Initialize Panel Defaults */
  IsSplitScreen = FALSE;
  ActivePanel = LeftPanel;

  /* Allocate and initialize the first volume using the dedicated module */
  CurrentVolume = Volume_Create();
  /* Assign initial volume to ActivePanel */
  ActivePanel->vol = CurrentVolume;


  GlobalView->show_stats = TRUE;
  GlobalView->fixed_col_width = 0; /* ADDED */
  GlobalView->refresh_mode = strtol(AUTO_REFRESH, NULL, 0); /* ADDED */

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
  if (GlobalView->refresh_mode & REFRESH_WATCHER) Watcher_Init();

  return( 0 );
}



void ReCreateWindows()
{
  BOOL is_small = TRUE;

  /* 1. Recalculate Layout based on current SplitScreen state */
  Layout_Recalculate();

  /* Determine if we are in 'small' (split vertical) or 'big' (zoom) file view mode */
  if (ActivePanel && ActivePanel->pan_file_window && ActivePanel->pan_big_file_window) {
      if (ActivePanel->pan_file_window == ActivePanel->pan_big_file_window) is_small = FALSE;
  }

  /* 2. Cleanup: Destroy ALL existing panel windows */
  if (LeftPanel->pan_dir_window) { delwin(LeftPanel->pan_dir_window); LeftPanel->pan_dir_window = NULL; }
  if (LeftPanel->pan_small_file_window) { delwin(LeftPanel->pan_small_file_window); LeftPanel->pan_small_file_window = NULL; }
  if (LeftPanel->pan_big_file_window) { delwin(LeftPanel->pan_big_file_window); LeftPanel->pan_big_file_window = NULL; }

  if (RightPanel->pan_dir_window) { delwin(RightPanel->pan_dir_window); RightPanel->pan_dir_window = NULL; }
  if (RightPanel->pan_small_file_window) { delwin(RightPanel->pan_small_file_window); RightPanel->pan_small_file_window = NULL; }
  if (RightPanel->pan_big_file_window) { delwin(RightPanel->pan_big_file_window); RightPanel->pan_big_file_window = NULL; }

  /* 3. Create Left Panel Windows (Always Created) */
  LeftPanel->pan_dir_window = Subwin(stdscr, LeftPanel->dir_h, LeftPanel->dir_w, LeftPanel->dir_y, LeftPanel->dir_x);
  keypad(LeftPanel->pan_dir_window, TRUE);
  scrollok(LeftPanel->pan_dir_window, TRUE);
  clearok(LeftPanel->pan_dir_window, TRUE);
  leaveok(LeftPanel->pan_dir_window, TRUE);
  WbkgdSet(LeftPanel->pan_dir_window, COLOR_PAIR(CPAIR_WINDIR));

  LeftPanel->pan_small_file_window = Subwin(stdscr, LeftPanel->small_file_h, LeftPanel->small_file_w, LeftPanel->small_file_y, LeftPanel->small_file_x);
  keypad(LeftPanel->pan_small_file_window, TRUE);
  clearok(LeftPanel->pan_small_file_window, TRUE);
  leaveok(LeftPanel->pan_small_file_window, TRUE);
  WbkgdSet(LeftPanel->pan_small_file_window, COLOR_PAIR(CPAIR_WINFILE));

  LeftPanel->pan_big_file_window = Subwin(stdscr, LeftPanel->big_file_h, LeftPanel->big_file_w, LeftPanel->big_file_y, LeftPanel->big_file_x);
  keypad(LeftPanel->pan_big_file_window, TRUE);
  clearok(LeftPanel->pan_big_file_window, TRUE);
  leaveok(LeftPanel->pan_big_file_window, TRUE);
  WbkgdSet(LeftPanel->pan_big_file_window, COLOR_PAIR(CPAIR_WINFILE));

  /* Determine current file window for LeftPanel */
  LeftPanel->pan_file_window = (is_small) ? LeftPanel->pan_small_file_window : LeftPanel->pan_big_file_window;

  /* 4. Create Right Panel Windows (Only if Split Screen) */
  if (IsSplitScreen) {
      RightPanel->pan_dir_window = Subwin(stdscr, RightPanel->dir_h, RightPanel->dir_w, RightPanel->dir_y, RightPanel->dir_x);
      keypad(RightPanel->pan_dir_window, TRUE);
      scrollok(RightPanel->pan_dir_window, TRUE);
      clearok(RightPanel->pan_dir_window, TRUE);
      leaveok(RightPanel->pan_dir_window, TRUE);
      WbkgdSet(RightPanel->pan_dir_window, COLOR_PAIR(CPAIR_WINDIR));

      RightPanel->pan_small_file_window = Subwin(stdscr, RightPanel->small_file_h, RightPanel->small_file_w, RightPanel->small_file_y, RightPanel->small_file_x);
      keypad(RightPanel->pan_small_file_window, TRUE);
      clearok(RightPanel->pan_small_file_window, TRUE);
      leaveok(RightPanel->pan_small_file_window, TRUE);
      WbkgdSet(RightPanel->pan_small_file_window, COLOR_PAIR(CPAIR_WINFILE));

      RightPanel->pan_big_file_window = Subwin(stdscr, RightPanel->big_file_h, RightPanel->big_file_w, RightPanel->big_file_y, RightPanel->big_file_x);
      keypad(RightPanel->pan_big_file_window, TRUE);
      clearok(RightPanel->pan_big_file_window, TRUE);
      leaveok(RightPanel->pan_big_file_window, TRUE);
      WbkgdSet(RightPanel->pan_big_file_window, COLOR_PAIR(CPAIR_WINFILE));

      /* Default RightPanel to small file window initially or match state */
      RightPanel->pan_file_window = (is_small) ? RightPanel->pan_small_file_window : RightPanel->pan_big_file_window;
  }

  /* 5. Sync ActivePanel Pointers */
  if (ActivePanel == NULL) ActivePanel = LeftPanel;

  if (ActivePanel == LeftPanel) {
      ActivePanel->pan_dir_window = LeftPanel->pan_dir_window;
      ActivePanel->pan_small_file_window = LeftPanel->pan_small_file_window;
      ActivePanel->pan_big_file_window = LeftPanel->pan_big_file_window;
      ActivePanel->pan_file_window = LeftPanel->pan_file_window;
  } else if (ActivePanel == RightPanel && IsSplitScreen) {
      ActivePanel->pan_dir_window = RightPanel->pan_dir_window;
      ActivePanel->pan_small_file_window = RightPanel->pan_small_file_window;
      ActivePanel->pan_big_file_window = RightPanel->pan_big_file_window;
      ActivePanel->pan_file_window = RightPanel->pan_file_window;
  } else {
      /* Fallback if something went wrong (e.g. RightPanel active but SplitScreen disabled) */
      ActivePanel = LeftPanel;
      ActivePanel->pan_dir_window = LeftPanel->pan_dir_window;
      ActivePanel->pan_small_file_window = LeftPanel->pan_small_file_window;
      ActivePanel->pan_big_file_window = LeftPanel->pan_big_file_window;
      ActivePanel->pan_file_window = LeftPanel->pan_file_window;
  }

  /* 6. Sync Global View Context */
  GlobalView->ctx_dir_window = ActivePanel->pan_dir_window;
  GlobalView->ctx_small_file_window = ActivePanel->pan_small_file_window;
  GlobalView->ctx_big_file_window = ActivePanel->pan_big_file_window;
  GlobalView->ctx_file_window = ActivePanel->pan_file_window;

  /* 7. Update Legacy Global file_window */
  file_window = GlobalView->ctx_file_window;

  /* 8. Utility Windows */
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

  clear();

  fprintf(stderr, "DEBUG: ReCreateWindows Done. Split=%d\n", IsSplitScreen);
  fprintf(stderr, "DEBUG: Left Win=%p Right Win=%p\n", (void*)LeftPanel->pan_dir_window, (void*)RightPanel->pan_dir_window);
  fprintf(stderr, "DEBUG: ActivePanel=%p (%s)\n", (void*)ActivePanel, (ActivePanel==LeftPanel)?"LEFT":"RIGHT");
  fprintf(stderr, "DEBUG: Global dir_window=%p\n", (void*)dir_window);
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