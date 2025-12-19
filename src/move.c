/***************************************************************************
 *
 * move.c
 * Move files
 *
 ***************************************************************************/


#include "ytree.h"




static int Move(char *to_path, char *from_path);


int MoveFile(FileEntry *fe_ptr,
	     unsigned char confirm,
	     char *to_file,
	     DirEntry *dest_dir_entry,
	     char *to_dir_path,
	     FileEntry **new_fe_ptr
	    )
{
  DirEntry    *de_ptr;
  LONGLONG    file_size;
  char        from_path[PATH_LENGTH+1];
  char        to_path[PATH_LENGTH+1];
  FileEntry   *dest_file_entry;
  FileEntry   *fen_ptr;
  struct stat stat_struct;
  int         term;
  int         result;
  /* New variables for EnsureDirectoryExists */
  char        abs_path[PATH_LENGTH+1];
  char        from_dir[PATH_LENGTH+1];
  BOOL        refresh_dirwindow = FALSE;

  result = -1;
  *new_fe_ptr = NULL;
  de_ptr = fe_ptr->dir_entry;

  (void) GetPath( de_ptr, from_dir ); /* Get clean source directory path */
  (void) strcpy( from_path, from_dir );
  (void) strcat( from_path, FILE_SEPARATOR_STRING );
  (void) strcat( from_path, fe_ptr->name );

  /* Construct base destination path */
  (void) strcpy( to_path, to_dir_path );
  (void) strcat( to_path, FILE_SEPARATOR_STRING );

  /* Handle relative path: make absolute based on source directory */
  if (*to_path != FILE_SEPARATOR_CHAR) {
       strcpy(abs_path, from_dir);
       strcat(abs_path, FILE_SEPARATOR_STRING);
       strcat(abs_path, to_path);
       strcpy(to_path, abs_path);
  }

  /* Ensure the destination directory exists */
  {
      BOOL created = FALSE;
      /* FIX: Pass &dest_dir_entry to update the pointer */
      if (EnsureDirectoryExists(to_path, CurrentVolume->vol_stats.tree, &created, &dest_dir_entry) == -1) {
          return -1;
      }
      if (created) refresh_dirwindow = TRUE;
  }

  (void) strcat( to_path, to_file );


  if( !strcmp( to_path, from_path ) )
  {
    MESSAGE( "Can't move file into itself" );
    ESCAPE;
  }


  if( access( from_path, W_OK ) )
  {
    (void) snprintf( message,
                     MESSAGE_LENGTH,
		    "Unmoveable file*\"%s\"*%s",
		    from_path,
		    strerror(errno)
		  );
    MESSAGE( message );
    ESCAPE;
  }


  if( dest_dir_entry )
  {
    /* Ziel befindet sich im Sub-Tree */
    /*--------------------------------*/

    (void) GetFileEntry( dest_dir_entry, to_file, &dest_file_entry );

    if( dest_file_entry )
    {
      /* Datei existiert */
      /*-----------------*/

      if( confirm )
      {
	term = InputChoice( "file exist; overwrite (Y/N) ? ", "YN\033" );

        if( term != 'Y' ) {
	  result = (term == 'N' ) ? 0 : -1;  /* Abort on escape */
	  ESCAPE;
	}
      }

      (void) DeleteFile( dest_file_entry );
    }
  }
  else
  {
    /* access benutzen */
    /*-----------------*/

    if( !access( to_path, F_OK ) )
    {
      /* Datei existiert */
      /*-----------------*/

      if( confirm )
      {
	term = InputChoice( "file exist; overwrite (Y/N) ? ", "YN\033" );

        if( term != 'Y' ) {
	  result = (term == 'N' ) ? 0 : -1;  /* Abort on escape */
	  ESCAPE;
	}
      }

      if( unlink( to_path ) )
      {
        (void) snprintf( message,
                         MESSAGE_LENGTH,
		        "Can't unlink*\"%s\"*%s",
		        to_path,
		        strerror(errno)
		      );
        MESSAGE( message );
        ESCAPE;
      }
    }
  }


  if( !Move( to_path, from_path ) )
  {
    /* File wurde bewegt */
    /*-------------------*/

    /* Original aus Baum austragen */
    /*-----------------------------*/

    (void) RemoveFile( fe_ptr );


    if( dest_dir_entry )
    {
      if( STAT_( to_path, &stat_struct ) )
      {
        ERROR_MSG( "Stat Failed*ABORT" );
        exit( 1 );
      }

      file_size = stat_struct.st_size;

      /* Update Total Stats */
      dest_dir_entry->total_bytes += file_size;
      dest_dir_entry->total_files++;
      statistic.disk_total_bytes += file_size;
      statistic.disk_total_files++;

      /* Create File Entry manually */
      /* FIX: Added +1 to allocation for null terminator */
      if( ( fen_ptr = (FileEntry *) malloc( sizeof( FileEntry ) +
					    strlen( to_file ) + 1
					  ) ) == NULL )
      {
        ERROR_MSG( "Malloc Failed*ABORT" );
        exit( 1 );
      }

      (void) strcpy( fen_ptr->name, to_file );

      (void) memcpy( &fen_ptr->stat_struct,
		     &stat_struct,
		     sizeof( stat_struct )
		   );

      fen_ptr->dir_entry   = dest_dir_entry;
      fen_ptr->tagged      = FALSE;
      fen_ptr->matching    = Match( fen_ptr );

      /* Update Matching Stats */
      if (fen_ptr->matching) {
          dest_dir_entry->matching_bytes += file_size;
          dest_dir_entry->matching_files++;
          statistic.disk_matching_bytes += file_size;
          statistic.disk_matching_files++;
      }

      /* Link into list (Head) */
      fen_ptr->next        = dest_dir_entry->file;
      fen_ptr->prev        = NULL;
      if( dest_dir_entry->file ) dest_dir_entry->file->prev = fen_ptr;
      dest_dir_entry->file = fen_ptr;
      *new_fe_ptr          = fen_ptr;

      /* Force refresh if we modified the tree structure or contents */
      refresh_dirwindow = TRUE;
    }

    (void) GetAvailBytes( &statistic.disk_space );

    result = 0;
  }

  if (refresh_dirwindow) {
      RefreshDirWindow();
  }

FNC_XIT:

  move( LINES - 3, 1 ); clrtoeol();
  move( LINES - 2, 1 ); clrtoeol();
  move( LINES - 1, 1 ); clrtoeol();

  return( result );
}





int GetMoveParameter(char *from_file, char *to_file, char *to_dir)
{
  char buffer[PATH_LENGTH * 2 +1];

  if( from_file == NULL )
  {
    from_file = "TAGGED FILES";
    (void) strcpy( to_file, "*" );
  }
  else
  {
    (void) strcpy( to_file, from_file );
  }

  (void) snprintf( buffer, sizeof(buffer), "MOVE: %s", from_file );

  ClearHelp();

  MvAddStr( LINES - 3, 1, buffer );
  MvAddStr( LINES - 2, 1, "AS:   " );
  if( InputString( to_file, LINES - 2, 7, 0, COLS - 7, "\r\033" ) == CR ) {
    MvAddStr( LINES - 1, 1, "TO:   " );
    if( InputString( to_dir, LINES - 1, 7, 0, COLS - 7, "\r\033" ) == CR ) {
        if (to_dir[0] == '\0') {
            strcpy(to_dir, ".");
        }
        return( 0 );
    }
  }
  ClearHelp();
  return( -1 );
}





static int Move(char *to_path, char *from_path)
{
  if( !strcmp( to_path, from_path ) )
  {
    MESSAGE( "Can't move file into itself" );
    return( -1 );
  }

  /* Try atomic rename first */
  if( rename( from_path, to_path ) == 0 )
  {
      return 0;
  }

  /* Handle Cross-Device link error by copying and deleting */
  if (errno == EXDEV) {
      if (CopyFileContent(to_path, from_path) == 0) {
          if (unlink(from_path) == 0) {
              return 0;
          } else {
              (void) snprintf( message, MESSAGE_LENGTH, "Move failed during unlink*\"%s\"*%s", from_path, strerror(errno) );
              MESSAGE( message );
              return -1;
          }
      } else {
          /* Copy failed, error message already shown by CopyFileContent */
          return -1;
      }
  }

  /* Fallback for other errors (e.g. permission denied) */
  (void) snprintf( message, MESSAGE_LENGTH,
          "Can't move (rename) \"%s\"*to \"%s\"*%s",
          from_path,
          to_path,
          strerror(errno)
        );
  MESSAGE( message );
  return( -1 );
}






int MoveTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package)
{
  int  result = -1;
  char new_name[PATH_LENGTH+1];


  if( BuildFilename( fe_ptr->name,
                     walking_package->function_data.mv.to_file,
		     new_name
		   ) == 0 )

  {
    if( *new_name == '\0' )
    {
      MESSAGE( "Can't move file to*empty name" );
    }
    else
    {
      result = MoveFile( fe_ptr,
		         walking_package->function_data.mv.confirm,
		         new_name,
		         walking_package->function_data.mv.dest_dir_entry,
		         walking_package->function_data.mv.to_path,
		         &walking_package->new_fe_ptr
		       );
    }
  }

  return( result );
}