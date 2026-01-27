/***************************************************************************
 *
 * src/cmd/chmod.c
 * Change File Modes
 *
 ***************************************************************************/


#include "ytree.h"



static int SetDirModus(DirEntry *de_ptr, WalkingPackage *walking_package);
static int GetNewModus(int old_modus, char *new_modus );
static int InputModeString(char *modus, int y, int x);


int ChangeFileModus(FileEntry *fe_ptr)
{
  char modus[12];
  WalkingPackage walking_package;
  int  result;
  int  x;

  result = -1;

  if( mode != DISK_MODE && mode != USER_MODE )
  {
    return( result );
  }

  (void) GetAttributes( fe_ptr->stat_struct.st_mode, modus );

  ClearHelp();
  MvAddStr( LINES - 2, 1, "MODE:" );
  x = 1 + strlen("MODE:") + UI_INPUT_PADDING;

  if( InputModeString( modus, LINES - 2, x ) == CR )
  {
    (void) strcpy( walking_package.function_data.change_modus.new_modus, modus );
    result = SetFileModus( fe_ptr, &walking_package );
  }

  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}





int ChangeDirModus(DirEntry *de_ptr)
{
  char modus[12];
  WalkingPackage walking_package;
  int  result;
  int  x;

  result = -1;

  if( mode != DISK_MODE && mode != USER_MODE )
  {
    return( result );
  }

  (void) GetAttributes( de_ptr->stat_struct.st_mode, modus );

  ClearHelp();
  MvAddStr( LINES - 2, 1, "MODE:" );
  x = 1 + strlen("MODE:") + UI_INPUT_PADDING;

  if( InputModeString( modus, LINES - 2, x ) == CR )
  {
    (void) strcpy( walking_package.function_data.change_modus.new_modus, modus );
    result = SetDirModus( de_ptr, &walking_package );
  }

  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}


static int InputModeString(char *modus, int y, int x)
{
    int cursor_pos = 1; /* Skip file type char at index 0 */
    int ch;
    char path[PATH_LENGTH];

    curs_set(0); /* Hide hardware cursor, we manage highlighting manually */

    while (1) {
        /* Draw current mode string */
        wmove(stdscr, y, x);
        int i;
        for (i = 0; i < 10; i++) {
            if (i == cursor_pos) wattron(stdscr, A_REVERSE);
            waddch(stdscr, modus[i]);
            if (i == cursor_pos) wattroff(stdscr, A_REVERSE);
        }
        wrefresh(stdscr);

        ch = Getch();

        /* F2 Handling for Volume Switching */
        if (ch == KEY_F(2)) {
             if (KeyF2Get(CurrentVolume->vol_stats.tree,
                          CurrentVolume->vol_stats.disp_begin_pos,
                          CurrentVolume->vol_stats.cursor_pos, path) == 0) {
                 /* Ignore path result as chmod uses specific characters */
             }
             /* Always redraw prompt */
             MvAddStr( LINES - 2, 1, "MODE:" );
             continue;
        }

        if (ch == ESC) return ESC;
        if (ch == '\n' || ch == '\r') return CR;

        switch (ch) {
            case KEY_LEFT:
                if (cursor_pos > 1) cursor_pos--;
                break;
            case KEY_RIGHT:
                if (cursor_pos < 9) cursor_pos++;
                break;
            case KEY_HOME:
                cursor_pos = 1;
                break;
            case KEY_END:
                cursor_pos = 9;
                break;
            default:
                if (strchr("rwx-sStT", ch)) {
                    modus[cursor_pos] = ch;
                    if (cursor_pos < 9) cursor_pos++;
                }
                break;
        }
    }
}


int SetFileModus(FileEntry *fe_ptr, WalkingPackage *walking_package)
{
  struct stat stat_struct;
  char buffer[PATH_LENGTH+1];
  int  result;
  int  new_modus;

  result = -1;

  walking_package->new_fe_ptr = fe_ptr; /* unchanged */

  new_modus = GetNewModus( fe_ptr->stat_struct.st_mode,
			   walking_package->function_data.change_modus.new_modus
			 );

  new_modus = new_modus | ( fe_ptr->stat_struct.st_mode &
	      ~( S_IRWXO | S_IRWXG | S_IRWXU | S_ISGID | S_ISUID ) );

  if( !chmod( GetFileNamePath( fe_ptr, buffer ), new_modus ) )
  {
    /* Successful modification */
    /*-------------------------*/

    if( STAT_( buffer, &stat_struct ) )
    {
      ERROR_MSG( "Stat Failed" );
    }
    else
    {
      fe_ptr->stat_struct = stat_struct;
    }

    result = 0;
  }
  else
  {
    MESSAGE( "Cant't change modus:*%s", strerror(errno) );
  }

  return( result );
}





static int SetDirModus(DirEntry *de_ptr, WalkingPackage *walking_package)
{
  struct stat stat_struct;
  char buffer[PATH_LENGTH+1];
  int  result;
  int  new_modus;

  result = -1;

  new_modus = GetNewModus( de_ptr->stat_struct.st_mode,
			   walking_package->function_data.change_modus.new_modus
			 );

  new_modus = new_modus | ( de_ptr->stat_struct.st_mode &
	      ~( S_IRWXO | S_IRWXG | S_IRWXU | S_ISGID | S_ISUID ) );

  if( !chmod( GetPath( de_ptr, buffer ), new_modus ) )
  {
    /* Successful modification */
    /*-------------------------*/

    if( STAT_( buffer, &stat_struct ) )
    {
      ERROR_MSG( "Stat Failed" );
    }
    else
    {
      de_ptr->stat_struct = stat_struct;
    }

    result = 0;
  }
  else
  {
    MESSAGE( "Cant't change modus:*%s", strerror(errno) );
  }

  return( result );
}




static int GetNewModus(int old_modus, char *modus )
{
  int new_modus;

  new_modus = 0;

  if( *modus == '-' ) new_modus |= S_IFREG;
  if( *modus == 'd' ) new_modus |= S_IFDIR;
#ifdef S_IFLNK
  if( *modus == 'l' ) new_modus |= S_IFLNK;
#endif /* S_IFLNK */
  if( *modus == '?' ) new_modus |= old_modus & S_IFMT;
  modus++;

  if( *modus   == 'r' ) new_modus |= S_IRUSR;
  if( *modus++ == '?' ) new_modus |= old_modus & S_IRUSR;
  if( *modus   == 'w' ) new_modus |= S_IWUSR;
  if( *modus++ == '?' ) new_modus |= old_modus & S_IWUSR;
  if( *modus   == 'x' ) new_modus |= S_IXUSR;
  if( *modus   == 's' ) new_modus |= S_ISUID | S_IXUSR;
  if( *modus++ == '?' ) new_modus |= old_modus & (S_ISUID | S_IXUSR);

  if( *modus   == 'r' ) new_modus |= S_IRGRP;
  if( *modus++ == '?' ) new_modus |= old_modus & S_IRGRP;
  if( *modus   == 'w' ) new_modus |= S_IWGRP;
  if( *modus++ == '?' ) new_modus |= old_modus & S_IWGRP;
  if( *modus   == 'x' ) new_modus |= S_IXGRP;
  if( *modus   == 's' ) new_modus |= S_ISGID | S_IXGRP;
  if( *modus++ == '?' ) new_modus |= old_modus & (S_ISGID | S_IXGRP);

  if( *modus   == 'r' ) new_modus |= S_IROTH;
  if( *modus++ == '?' ) new_modus |= old_modus & S_IROTH;
  if( *modus   == 'w' ) new_modus |= S_IWOTH;
  if( *modus++ == '?' ) new_modus |= old_modus & S_IWOTH;
  if( *modus   == 'x' ) new_modus |= S_IXOTH;
  if( *modus++ == '?' ) new_modus |= old_modus & S_IXOTH;

  return( new_modus );
}




int GetModus(char *modus)
{
  return( GetNewModus( 0, modus ) );
}