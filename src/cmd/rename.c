/***************************************************************************
 *
 * src/cmd/rename.c
 * Renaming files/directories
 *
 ***************************************************************************/


#include "ytree.h"


static int RenameDirEntry(char *to_path, char *from_path);
static int RenameFileEntry(char *to_path, char *from_path);


int RenameDirectory(DirEntry *de_ptr, char *new_name)
{
  DirEntry    *den_ptr;
  DirEntry    *sde_ptr;
  DirEntry    *ude_ptr;
  FileEntry   *fe_ptr;
  char        from_path[PATH_LENGTH+1];
  char        to_path[PATH_LENGTH+1];
  struct stat stat_struct;
  int         result;
  char        *cptr;
  int         len;

  result = -1;

  /* Get the full path of the directory to be renamed */
  (void) GetPath( de_ptr, from_path );

  /* ARCHIVE MODE HANDLER */
#ifdef HAVE_LIBARCHIVE
  if (mode == ARCHIVE_MODE) {
      if (Archive_RenameEntry(CurrentVolume->vol_stats.login_path, from_path, new_name) == 0) {
           RefreshTreeSafe(CurrentVolume->vol_stats.tree);
           return 0;
      }
      return -1;
  }
#endif

  /*
   * Safety Fix: Construct the new path using snprintf instead of modifying
   * the string in place with strcpy/strcat.
   */

  /* Find the parent directory by locating the last separator */
  cptr = strrchr( from_path, FILE_SEPARATOR_CHAR );

  if( !cptr )
  {
    /* Should not happen for absolute paths, but safety first */
    WARNING( "Invalid Path!*\"%s\"", from_path );
    ESCAPE;
  }

  if( cptr == from_path )
  {
    /* Parent is root '/' */
    len = snprintf(to_path, sizeof(to_path), "%c%s", FILE_SEPARATOR_CHAR, new_name);
  }
  else
  {
    /* Calculate length of parent directory string */
    int parent_len = cptr - from_path;
    /* Construct new path: parent_dir + separator + new_name */
    len = snprintf(to_path, sizeof(to_path), "%.*s%c%s",
                   parent_len, from_path, FILE_SEPARATOR_CHAR, new_name);
  }

  if (len >= (int)sizeof(to_path)) {
      WARNING( "Path too long! Rename aborted." );
      ESCAPE;
  }

  if( access( from_path, W_OK ) )
  {
    MESSAGE( "Rename not possible!*\"%s\"*%s",
		    from_path,
		    strerror(errno)
		  );
    ESCAPE;
  }



  if( !RenameDirEntry( to_path, from_path ) )
  {
    /* Rename successful */
    /*--------------------*/


    if( STAT_( to_path, &stat_struct ) )
    {
      ERROR_MSG( "Stat Failed*ABORT" );
      exit( 1 );
    }

    /* FIX: Added +1 to allocation for null terminator */
    if( ( den_ptr = (DirEntry *) malloc( sizeof( DirEntry ) +
					 strlen( new_name ) + 1
				       ) ) == NULL )
    {
      ERROR_MSG( "Malloc Failed*ABORT" );
      exit( 1 );
    }

    (void) memcpy( den_ptr, de_ptr, sizeof( DirEntry ) );

    (void) strcpy( den_ptr->name, new_name );

    (void) memcpy( &den_ptr->stat_struct,
		   &stat_struct,
		   sizeof( stat_struct )
		 );

    /* Link structure */
    /*---------------------*/

    if( den_ptr->prev ) den_ptr->prev->next = den_ptr;
    if( den_ptr->next ) den_ptr->next->prev = den_ptr;

    /* Subtree */
    /*---------*/

    for( sde_ptr=den_ptr->sub_tree; sde_ptr; sde_ptr = sde_ptr->next )
      sde_ptr->up_tree = den_ptr;

    /* Files */
    /*-------*/

    for( fe_ptr=den_ptr->file; fe_ptr; fe_ptr=fe_ptr->next )
      fe_ptr->dir_entry = den_ptr;

    /* Uptree */
    /*--------*/

    for( ude_ptr=den_ptr->up_tree; ude_ptr; ude_ptr = ude_ptr->next )
      if( ude_ptr->sub_tree == de_ptr ) ude_ptr->sub_tree = den_ptr;

    /* Free old structure */
    /*-------------------------*/

    free( de_ptr );

    /* Warning: de_ptr is invalid from now on !!! */
    /*--------------------------------------------*/

    result = 0;
  }

FNC_XIT:

  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}






int RenameFile(FileEntry *fe_ptr, char *new_name, FileEntry **new_fe_ptr )
{
  DirEntry    *de_ptr;
  FileEntry   *fen_ptr;
  char        from_path[PATH_LENGTH+1];
  char        to_path[PATH_LENGTH+1];
  char        parent_path[PATH_LENGTH+1];
  struct stat stat_struct;
  int         result;
  int         len;

  result = -1;

  *new_fe_ptr = fe_ptr;

  de_ptr = fe_ptr->dir_entry;

  (void) GetFileNamePath( fe_ptr, from_path );

  /* ARCHIVE MODE HANDLER */
#ifdef HAVE_LIBARCHIVE
  if (mode == ARCHIVE_MODE) {
      if (Archive_RenameEntry(CurrentVolume->vol_stats.login_path, from_path, new_name) == 0) {
           RefreshTreeSafe(CurrentVolume->vol_stats.tree);
           return 0;
      }
      return -1;
  }
#endif

  /* Safety Fix: Use snprintf to construct to_path */
  (void) GetPath( de_ptr, parent_path );

  /* Handle root path case correctly (avoid double slash if parent is "/") */
  if (strcmp(parent_path, FILE_SEPARATOR_STRING) == 0) {
      len = snprintf(to_path, sizeof(to_path), "%c%s", FILE_SEPARATOR_CHAR, new_name);
  } else {
      len = snprintf(to_path, sizeof(to_path), "%s%c%s", parent_path, FILE_SEPARATOR_CHAR, new_name);
  }

  if (len >= (int)sizeof(to_path)) {
      WARNING( "Path too long! Rename aborted." );
      ESCAPE;
  }

  if( access( from_path, W_OK ) )
  {
    MESSAGE( "Rename not possible!*\"%s\"*%s",
		    from_path,
		    strerror(errno)
		  );
    ESCAPE;
  }



  if( !RenameFileEntry( to_path, from_path ) )
  {
    /* Rename successful */
    /*--------------------*/

    if( STAT_( to_path, &stat_struct ) )
    {
      ERROR_MSG( "Stat Failed*ABORT" );
      exit( 1 );
    }


    /* FIX: Added +1 to allocation for null terminator */
    if( ( fen_ptr = (FileEntry *) malloc( sizeof( FileEntry ) + strlen( new_name ) + 1 ) ) == NULL )
    {
      ERROR_MSG( "Malloc Failed*ABORT" );
      exit( 1 );
    }

    (void) memcpy( fen_ptr, fe_ptr, sizeof( FileEntry ) );

    (void) strcpy( fen_ptr->name, new_name );

    (void) memcpy( &fen_ptr->stat_struct,
		   &stat_struct,
		   sizeof( stat_struct )
		 );

    /* Link structure */
    /*---------------------*/

    if( fen_ptr->prev ) fen_ptr->prev->next = fen_ptr;
    if( fen_ptr->next ) fen_ptr->next->prev = fen_ptr;
    if( fen_ptr->dir_entry->file == fe_ptr ) fen_ptr->dir_entry->file = fen_ptr;

    /* Free old structure */
    /*-------------------------*/

    free( fe_ptr );

    /* Warning: fe_ptr is invalid from now on !!! */
    /*--------------------------------------------*/

    result = 0;

    *new_fe_ptr = fen_ptr;
  }

FNC_XIT:

  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}





int GetRenameParameter(char *old_name, char *new_name)
{
  const char *prompt;

  /* MODIFIED: Also allow ARCHIVE_MODE */
  if( mode != DISK_MODE && mode != USER_MODE && mode != ARCHIVE_MODE )
  {
    return( -1 );
  }

  ClearHelp();

  if( old_name == NULL )
  {
    prompt = "RENAME TAGGED FILES TO:";
  }
  else
  {
    prompt = "RENAME TO:";
  }

  (void) strcpy( new_name, (old_name) ? old_name : "*" );


  if( UI_ReadString(prompt, new_name, PATH_LENGTH, HST_FILE) != CR )
    return( -1 );

  if(!strlen(new_name))
    return( -1 );

  if(old_name && !strcmp(old_name, new_name))
  {
    MESSAGE("Can't rename: New name same as old name.");
    return( -1 );
  }

  if(strrchr(new_name, FILE_SEPARATOR_CHAR) != NULL)
  {
    MESSAGE("Invalid new name:*No slashes when renaming!");
    return( -1 );
  }

  return( 0 );
}





static int RenameDirEntry(char *to_path, char *from_path)
{
  struct stat fdstat;

  if( !strcmp( to_path, from_path ) )
  {
    MESSAGE( "Can't rename directory:*New Name == Old Name" );
    return( 0 );
  }

  if( stat( to_path, &fdstat ) == 0 )
  {
    MESSAGE( "Can't rename directory:*Destination object already exist!" );
    return( -1 );
  }

  /*
   * Modernized: Always use rename().
   * Removed obsolete link()/unlink() fallback which fails on directories.
   */
  if( rename( from_path, to_path ) )
  {
    MESSAGE( "Can't rename \"%s\"*to \"%s\"*%s",
		    from_path,
		    to_path,
		    strerror(errno)
		  );
    return( -1 );
  }

  return( 0 );
}





static int RenameFileEntry(char *to_path, char *from_path)
{
  if( !strcmp( to_path, from_path ) )
  {
    MESSAGE( "Can't rename!*New Name == Old Name" );
    return( -1 );
  }

  /*
   * Modernized: Use rename() instead of link()/unlink().
   * rename() is atomic and safer.
   */
  if( rename( from_path, to_path ) )
  {
    MESSAGE( "Can't rename \"%s\"*to \"%s\"*%s",
		    from_path,
		    to_path,
		    strerror(errno)
		  );
    return( -1 );
  }

  return( 0 );
}





int RenameTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package)
{
  int  result = -1;
  char new_name[PATH_LENGTH+1];


  if( BuildFilename( fe_ptr->name,
                     walking_package->function_data.rename.new_name,
		     new_name
		   ) == 0 )
  {
    if( *new_name == '\0' )
    {
      MESSAGE( "Can't rename file to*empty name" );
    }
    else
    {
      result = RenameFile( fe_ptr, new_name, &walking_package->new_fe_ptr );
    }
  }
  return( result );
}