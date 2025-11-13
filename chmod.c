/***************************************************************************
 *
 * chmod.c
 * Change Modus
 *
 ***************************************************************************/


#include "ytree.h"



static int SetDirModus(DirEntry *de_ptr, WalkingPackage *walking_package);
static int GetNewModus(int old_modus, char *new_modus );



int ChangeFileModus(FileEntry *fe_ptr)
{
  char modus[11];
  WalkingPackage walking_package;
  int  result;

  result = -1;

  if( mode != DISK_MODE && mode != USER_MODE )
  {
    return( result );
  }

  (void) GetAttributes( fe_ptr->stat_struct.st_mode, modus );

  if( GetNewFileModus( LINES - 2, 1, modus, "\r\033" ) == CR )
  {
    (void) strcpy( walking_package.function_data.change_modus.new_modus, modus );
    result = SetFileModus( fe_ptr, &walking_package );
  }

  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}





int ChangeDirModus(DirEntry *de_ptr)
{
  char modus[11];
  WalkingPackage walking_package;
  int  result;

  result = -1;

  if( mode != DISK_MODE && mode != USER_MODE )
  {
    return( result );
  }

  (void) GetAttributes( de_ptr->stat_struct.st_mode, modus );

  if( GetNewFileModus( LINES - 2, 1, modus, "\r\033" ) == CR )
  {
    (void) strcpy( walking_package.function_data.change_modus.new_modus, modus );
    result = SetDirModus( de_ptr, &walking_package );
  }

  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}




int GetNewFileModus(int y, int x, char *modus, char *term)
{
  int c, p;
  static char rwx[] = "rwx";

  ClearHelp();
  curs_set(1);
  MvAddStr( y, x, "New Filemodus:" );

  x += 16;

  p = 0;
  MvAddStr(y, x, modus );
  leaveok(stdscr, FALSE);
  do
  {
    move( y, x + p );
    RefreshWindow( stdscr );
    doupdate();

    c = Getch();

#ifdef VI_KEYS
    c = ViKey( c );
#endif /* VI_KEYS */

    if( c == LF ) c = CR;

    if( p > 0 && ( c == '?' || c == '-' || c == rwx[(p-1) % 3] ) )
    {
      /* gueltige Eingabe */
      /*------------------*/

      modus[p] = (char) c;
      addch( c );
      if( p < 9 ) p++;
    }
    else if( c == 's' && ( p == 3 || p == 6 ) )
    {
      /* Set-ID */
      /*--------*/

      if( modus[p] != 'x' && modus[p] != 's' )
      {
	MESSAGE( "Execute-Permission required*for set-ID" );
      }
      else
      {
        modus[p] = (char) c;
        addch( c );
        if( p < 9 ) p++;
      }
    }
    else
    {
      if( c == ' ' && p < 9 ) p++;
      else if( c == KEY_LEFT && p > 0 ) p--;
      else if( c == KEY_RIGHT && p < 9 ) p++;
      else if( strrchr( term, c ) == NULL ) beep();
    }
  } while( c != -1 && strrchr( term, c ) == NULL );
  leaveok(stdscr, TRUE);
  move( y, x ); clrtoeol();
  curs_set(0);

  return( c );
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
    /* Erfolgreich modifiziert */
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
    (void) sprintf( message, "Cant't change modus:*%s", strerror(errno) );
    MESSAGE( message );
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
    /* Erfolgreich modifiziert */
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
    (void) sprintf( message, "Cant't change modus:*%s", strerror(errno) );
    MESSAGE( message );
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