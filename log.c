/***************************************************************************
 *
 * log.c
 * Dateibaum lesen
 *
 ***************************************************************************/


#include "ytree.h"
/* #include <sys/wait.h> */  /* maybe wait.h is available */


/* Login Disk liefert
 * -1 bei Fehler
 * 0  bei fehlerfreiem lesen eines neuen Baumes
 * 1  bei Benutzung des Baumes im Speicher
 */


int LoginDisk(char *path)
{
  struct stat stat_struct;
  int    depth;
  int    result = 0;
  char   saved_filter[FILE_SPEC_LENGTH + 1];

  if( STAT_( path, &stat_struct ) )
  {
    /* Stat failed */
    /*-------------*/

    (void) sprintf( message, "Can't access*\"%s\"*%s", path, strerror(errno) );
    MESSAGE( message );
    return( -1 );
  }

  /* Pre-flight check for non-directory files to ensure they are valid archives */
  if( !S_ISDIR(stat_struct.st_mode ) )
  {
#ifdef HAVE_LIBARCHIVE
      struct archive *a_test;
      int r_test;

      a_test = archive_read_new();
      if (a_test == NULL) {
          ERROR_MSG("archive_read_new() failed");
          return -1;
      }
      archive_read_support_filter_all(a_test);
      archive_read_support_format_all(a_test);
      r_test = archive_read_open_filename(a_test, path, 10240);
      archive_read_free(a_test);

      if (r_test != ARCHIVE_OK) {
          (void) sprintf(message, "Not a recognized archive file*or format not supported*\"%s\"", path);
          MESSAGE(message);
          return -1;
      }
#else
      /* Fallback for when libarchive is not available */
      (void) sprintf(message, "Cannot open file as archive*ytree not compiled with*libarchive support");
      MESSAGE(message);
      return -1;
#endif
  }


  if( mode == DISK_MODE || mode == USER_MODE)
  {
    /* Status retten */
    /*---------------*/
    (void) memcpy( (char *) &disk_statistic,
		   (char *) &statistic,
		   sizeof( Statistic )
		 );
  }

  if ( disk_statistic.login_path[0] != 0) {
    if( !strcmp( disk_statistic.login_path, path ) )
    {
      /* Tree is in memory! Use it! */
      /*----------------------------*/

      if( statistic.tree != disk_statistic.tree )
        DeleteTree( statistic.tree );

      if (IsUserActionDefined())
      {
        mode = USER_MODE;
      }
      else
      {
        mode = DISK_MODE;
      }
      (void) memcpy( (char *) &statistic,
                     (char *) &disk_statistic,
                     sizeof( Statistic )
                     );
      (void) SetFilter( statistic.file_spec );
      return( 1 );   /* Return-Wert fuer "alten Baum" */
    }
  }

  /* Save the current filter before wiping statistics.
   * If it's empty (first run), use the default.
   */
  if (strlen(statistic.file_spec) > 0) {
      strncpy(saved_filter, statistic.file_spec, FILE_SPEC_LENGTH);
      saved_filter[FILE_SPEC_LENGTH] = '\0';
  } else {
      strcpy(saved_filter, DEFAULT_FILE_SPEC);
  }


  if( mode != DISK_MODE && mode != USER_MODE )
  {
    DeleteTree( statistic.tree );
  }

  (void) memset( &statistic, 0, sizeof( statistic ) );

  if( ( statistic.tree = (DirEntry *) malloc( sizeof( DirEntry ) +
					      PATH_LENGTH )) == NULL )
  {
    ERROR_MSG( "Malloc failed*ABORT" );
    exit( 1 );
  }

  (void) memset( statistic.tree, 0, sizeof( DirEntry ) + PATH_LENGTH );

  (void) strcpy( statistic.path, path );
  (void) strcpy( statistic.login_path, path );
  /* Restore the user's filter */
  (void) strcpy( statistic.file_spec, saved_filter );

  statistic.kind_of_sort = SORT_BY_NAME + SORT_ASC;
  (void) memcpy( &statistic.tree->stat_struct, &stat_struct, sizeof( stat_struct ) );


  if( !S_ISDIR(stat_struct.st_mode ) )
  {
    /* "root" node is always a directory */
    (void) memset( (char *) &statistic.tree->stat_struct, 0, sizeof( struct stat ) );
    statistic.tree->stat_struct.st_mode = S_IFDIR;
    statistic.disk_total_directories = 1;
    mode = ARCHIVE_MODE;
  }
  else if (IsUserActionDefined())
  {
    mode = USER_MODE;
  }
  else
  {
    mode = DISK_MODE;
  }


  (void) GetDiskParameter( path,
			   statistic.disk_name,
			   &statistic.disk_space,
			   &statistic.disk_capacity
			 );

  RefreshWindow( stdscr );
  RefreshWindow( dir_window );
  DisplayMenu();
  doupdate();


  if( mode == ARCHIVE_MODE)
  {
    (void) strcpy( statistic.tree->name, path );

#ifdef HAVE_LIBARCHIVE
    Notice("Scanning archive...");
    if (ReadTreeFromArchive(statistic.tree, statistic.login_path))
    {
        /* Error message will have been displayed by the function */
        result = -1;
    }
    UnmapNoticeWindow();
#else
    ERROR_MSG("Archive support not compiled.*Please install libarchive-dev*and recompile ytree.");
    result = -1;
#endif

  }
  else
  {
    if( *disk_statistic.login_path )
    {
      /* Alten Baum loeschen */
      /*---------------------*/
      *disk_statistic.login_path = '\0';
      DeleteTree( disk_statistic.tree );
    }

    (void) strcpy( statistic.tree->name, path );
    statistic.tree->next = statistic.tree->prev = NULL;

    depth = strtol(TREEDEPTH, NULL, 0);
    if( ReadTree( statistic.tree, path, depth ) )
    {
      ERROR_MSG( "ReadTree Failed" );
      return( -1 );
    }

    /* Recalculate stats to respect initial hide_dot_files setting,
       as ReadTree now loads everything. */
    RecalculateSysStats();

    (void) memcpy( (char *) &disk_statistic,
		   (char *) &statistic,
		   sizeof( Statistic )
		 );
  }

  (void) SetFilter( statistic.file_spec );
/*  SetKindOfSort( statistic.kind_of_sort ); */

  return( result );
}





int GetNewLoginPath(char *path)
{
    int result;
    char *cptr;
    char user_input[PATH_LENGTH * 2 + 1] = "";
    char current_dir_path[PATH_LENGTH + 1];

    result = -1;

    ClearHelp();

    MvAddStr(LINES - 2, 1, "LOG:");

    /* Save the current directory context and set it as default for user input */
    strcpy(current_dir_path, path);
    strcpy(user_input, path);

    if (mode == LL_FILE_MODE && *path == '<') {
        for (cptr = user_input; (*cptr = *(cptr + 1)); cptr++);
        if (user_input[strlen(user_input) - 1] == '>')
            user_input[strlen(user_input) - 1] = '\0';
    }

    if (InputString(user_input, LINES - 2, 6, 0, COLS - 7, "\r\033") == CR) {
        /*
         * NOTE: The size of temp_path has been increased to prevent potential
         * buffer overflows identified by the -Wformat-truncation compiler warning.
         * The new size accounts for the worst-case concatenation of a base path,
         * a path separator, and the user's input string.
         */
        char temp_path[PATH_LENGTH * 3 + 2];
        char resolved_path[PATH_LENGTH + 1];

        /* InputString expands '~', so check if the result is an absolute path. */
        if (user_input[0] != FILE_SEPARATOR_CHAR) {
            /* It's a relative path. Construct the full path to be resolved. */
            if (strcmp(current_dir_path, FILE_SEPARATOR_STRING) == 0) {
                snprintf(temp_path, sizeof(temp_path), "/%s", user_input);
            } else {
                snprintf(temp_path, sizeof(temp_path), "%s/%s", current_dir_path, user_input);
            }
        } else {
            /* It's an absolute path. */
            strcpy(temp_path, user_input);
        }

        /*
         * Use realpath() to resolve '..' and '.' components. This is robust.
         * If it fails, it could be because the path does not exist (e.g.,
         * logging into a non-existent path for an archive). In that case,
         * fall back to the pure string-based NormPath.
         */
        if (realpath(temp_path, resolved_path) != NULL) {
            strcpy(path, resolved_path);
        } else {
            NormPath(temp_path, path);
        }
        result = 0;
    }

    return (result);
}