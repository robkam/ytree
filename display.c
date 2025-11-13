/***************************************************************************
 *
 * display.c
 * Menue-Geruest anzeigen
 *
 ***************************************************************************/


#include "ytree.h"
#include "patchlev.h"


static void PrintMenuLine(WINDOW *win, int y, int x, char *line);
static void PrintLine(WINDOW *win, int y, int x, char *line, int len);
static void DisplayVersion(void);



static char *mask[] = {
"5-----------------------2",
"|FILE:                  |",
"6-----------------------7",
"|DISK:                  |",
"| Avail                 |",
"6-----------------------7",
"|DISK Statistics        |",
"|[Total]                  |",
"| Files:                |",
"| Bytes:                |",
"|[Matching]               |",
"| Files:                |",
"| Bytes:                |",
"|[Tagged]                 |",
"| Files:                |",
"| Bytes:                |",
"|[Current Directory]      |",
"|                       |",
"| Bytes:                |"
};


static char *logo[] = {
                        "                  #                       ",
                        "                ##                        ",
                        "     ##  ##   #####  ## ###   ####    ####",
                        "    ##  ##    ##     ##  ## ##  ##  ##  ##",
                        "   ##  ##    ##     ##  ## ######  ###### ",
                        "   #####    ## #   ##     ##      ##      ",
                        "     ##     ###  ####     ####    ####    ",
                        "#####                                     "
		      };

static char *extended_line = "| |                       |";


static char *last_line = "3-8-----------------------4";
static char *first_line ="1-5";


static char dir_help_disk_mode_0[] = "DIR       (A)ttribute (D)elete  (F)ilespec  (G)roup (L)og (M)akedir A(b)out         (Q)uit";
static char dir_help_disk_mode_1[] = "COMMANDS  (O)wner (R)ename (S)howall (^S)how-tagged (T)ag (U)ntag e(X)ecute   (^F) dirmode";
static char *dir_help[MAX_MODES][2] =
  {
    { /* DISK_MODE */
      dir_help_disk_mode_0,
      dir_help_disk_mode_1
    },
    { /* LL_FILE_MODE */
      "DIR       (F)ilespec (L)ogin (S)howall (T)ag (U)ntag (^F) dirmode  (Q)uit                 ",
      "COMMANDS                                                                    "
    },
    { /* ARCHIVE_MODE */
      "ARCHIVE   (F)ilespec (L)og (S)howall (T)ag (U)ntag (^F) dirmode  (Q)uit                 ",
      "COMMANDS                                                                    "
    },
    { /* USER_MODE */
      dir_help_disk_mode_0,	/* Default unless changed by user prefs */
      dir_help_disk_mode_1
    }
  };


static char file_help_disk_mode_0[] = "FILE      (A)ttribute (C)opy (D)elete (E)dit (F)ilespec (G)roup (H)ex (L)ogin (M)ove    (Q)uit ";
static char file_help_disk_mode_1[] = "COMMANDS  (O)wner (P)ipe (R)ename (S)ort (T)ag (U)ntag (V)iew e(X)ecute pathcop(Y) (^F)ilemode ";
static char *file_help[MAX_MODES][2] =
  {
    { /* DISK_MODE */
      file_help_disk_mode_0,
      file_help_disk_mode_1
    },
    { /* LL_FILE_MODE */
      "FILE      (F)ilespec (L)ogin (S)ort (T)ag (U)ntag (^F)ilemode      (Q)uit                ",
      "COMMANDS                                                                   "
    },
    { /* ARCHIVE_MODE */
      "ARCH-FILE (C)opy (F)ilespec (H)ex (P)ipe (S)ort (T)ag (U)ntag (V)iew pathcop(Y) (Q)uit         ",
      "COMMANDS  (^F)ilemode                                                        "
    },
    { /* USER_MODE */
      file_help_disk_mode_0,	/* Default unless changed by user prefs */
      file_help_disk_mode_1
    }
  };





static void DisplayVersion(void)
{
  static char version[80];

  ClearHelp();
  (void) sprintf( version,
		  "ytree Version %sPL%d %s (Werner Bregulla)",
		  VERSION,
		  PATCHLEVEL,
		  VERSIONDATE
		);
  MvAddStr( LINES - 2, (unsigned) (COLS - strlen( version )) >> 1, version );
}




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
    PrintOptions( stdscr, LINES - 2 + i, 0, dir_help[mode][i] );
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
    if (file_help[mode][1] == file_help_disk_mode_1 && (cptr = FILE2) != NULL)
      file_help[mode][1] = cptr;
  }
  for( i=0; i < (int)(sizeof(file_help[mode]) / sizeof(file_help[mode][0])); i++) {
    PrintOptions( stdscr, LINES - 2 + i, 0, file_help[mode][i] );
    clrtoeol();
  }
}



void ClearHelp(void)
{
  int i;

  for( i=0; i < 3; i++ )
  {
    wmove( stdscr, LINES - 3 + i, 0 ); clrtoeol();
  }
}



void DisplayMenu(void)
{
  int    y;
  int    l, c;


  PrintSpecialString( stdscr, 0, 0, "Path: ", CPAIR_MENU );
#ifdef COLOR_SUPPORT
  clrtoeol();
#endif

  werase( dir_window );
  werase( big_file_window );
  werase( small_file_window );

  for( y=1; y <= (int)(sizeof(mask) / sizeof(mask[0])); y++){
    PrintOptions( stdscr, y, 0, "|");
    PrintOptions( stdscr, y, COLS - 25 , mask[y-1] );
  }
  for( ; y < LINES - 4; y++ )
    PrintMenuLine( stdscr, y, 0, extended_line );
  PrintLine( stdscr, DIR_WINDOW_HEIGHT + 2, 0, "6-7", COLS - 25 );
  PrintLine( stdscr, 1, 0, first_line, COLS - 25);
  PrintMenuLine( stdscr, y, 0, last_line );

  l = sizeof(logo) / sizeof(logo[0]);
  c = strlen( logo[0] );

  for( y=0; y < l; y++ )
  {
    MvWAddStr( dir_window,
	       y + ((DIR_WINDOW_HEIGHT - l) >> 1),
	       (DIR_WINDOW_WIDTH - c) >> 1,
	       logo[y]
	     );
  }
  DisplayVersion();

  touchwin( dir_window );
  /* refresh(); */
}



void SwitchToSmallFileWindow(void)
{
  werase( file_window );
  PrintLine( stdscr, DIR_WINDOW_HEIGHT + 2, 0, "6-7", COLS - 25 );
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
  file_window = big_file_window;
  RefreshWindow( stdscr );
}


void MapF2Window(void)
{
  char *buffer;

  if( ( buffer = (char *)malloc( F2_WINDOW_WIDTH + 1 ) ) == NULL )
  {
     ERROR_MSG( "Malloc failed*ABORT" );
     exit( 1 );
  }
  werase( f2_window );
  box(f2_window, 0, 0);
  memset(buffer, '=', F2_WINDOW_WIDTH);
  buffer[F2_WINDOW_WIDTH] = '\0';

  PrintSpecialString( f2_window, F2_WINDOW_HEIGHT - 1, 0, buffer, CPAIR_HST );
  RefreshWindow( f2_window );
  free(buffer);
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
}



static void PrintMenuLine(WINDOW *win, int y, int x, char *line)
{
  int  i;
  int p, l;
  char *buffer;
  if (strchr(line,'('))
     p = 2;
  else
     p = 0;
  l = COLS + 2 + p;
  if( ( buffer = (char *)malloc( l ) ) == NULL )
  {
     ERROR_MSG( "Malloc failed*ABORT" );
     exit( 1 );
  }
  buffer[0] = line[0];
  if (p == 0)
     p = COLS - 25;
  else
     p = COLS - 27;
  for(i=1; i < p; i++)
      buffer[i] = line[1];
  (void) strncpy( &buffer[i], &line[2], l - i );
  buffer[l-1] = '\0';
  PrintOptions( stdscr, y, x , buffer );
  free( buffer );
}



static void PrintLine(WINDOW *win, int y, int x, char *line, int len)
{
  int  i;
  char *buffer;

  if(len > 0) {
    if( ( buffer = (char *)malloc( len + 2 ) ) == NULL )
    {
       ERROR_MSG( "Malloc failed*ABORT" );
       exit( 1 );
    }
    buffer[0] = line[0];
    for(i=1; i < (len); i++)
        buffer[i] = line[1];
    (void) strcpy( &buffer[i], &line[2] );
    PrintOptions( stdscr, y, x , buffer );
    free( buffer );
  }
}

void RefreshWindow(WINDOW *win)
{
	wnoutrefresh(win);
}