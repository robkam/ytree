/***************************************************************************
 *
 * Dateibaum lesen
 *
 ***************************************************************************/


#include "ytree.h"
/* #include <sys/wait.h> */  /* maybe wait.h is available */




static void DeleteTree(DirEntry *tree)
{
  DirEntry  *de_ptr, *next_de_ptr;
  FileEntry *fe_ptr, *next_fe_ptr;
  
  for( de_ptr=tree; de_ptr; de_ptr=next_de_ptr)
  {
    next_de_ptr = de_ptr->next;
    
    for( fe_ptr=de_ptr->file; fe_ptr; fe_ptr=next_fe_ptr)
    {
      next_fe_ptr=fe_ptr->next;
      free( fe_ptr );
    }
    
    if( de_ptr->sub_tree ) DeleteTree( de_ptr->sub_tree );

    free( de_ptr );
  }
}




/* Login Disk liefert
 * -1 bei Fehler
 * 0  bei fehlerfreiem lesen eines neuen Baumes
 * 1  bei Benutzung des Baumes im Speicher
 */


int LoginDisk(char *path)
{
  struct stat stat_struct;
  int    file_method = 0;
  int    depth, l = 0;
  int    result = 0;

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
      (void) SetFileSpec( statistic.file_spec );
      return( 1 );   /* Return-Wert fuer "alten Baum" */
    }
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
  (void) strcpy( statistic.file_spec, DEFAULT_FILE_SPEC );
  (void) strcpy( statistic.tape_name, DEFAULT_TAPEDEV );
  statistic.kind_of_sort = SORT_BY_NAME + SORT_ASC;
  (void) memcpy( &statistic.tree->stat_struct, &stat_struct, sizeof( stat_struct ) );
  
 
  if( !S_ISDIR(stat_struct.st_mode ) )
  {
    /* "root" node is always a directory */
    (void) memset( (char *) &statistic.tree->stat_struct, 0, sizeof( struct stat ) );
    statistic.tree->stat_struct.st_mode = S_IFDIR;
    statistic.disk_total_directories = 1;

    /* No Directory ==> TAR_FILE/RPM/ZOO/ZIP/LHA/ARC_FILE */
    /*----------------------------------------------------*/

    file_method = GetFileMethod( statistic.login_path );
    l = strlen( statistic.login_path );

    switch( file_method )
    {
      case ZOO_COMPRESS:         mode = ZOO_FILE_MODE; break;
      case ARC_COMPRESS:         mode = ARC_FILE_MODE; break;
      case LHA_COMPRESS:         mode = LHA_FILE_MODE; break;
      case ZIP_COMPRESS:         mode = ZIP_FILE_MODE; break;
      case SEVENZIP_COMPRESS:    mode = SEVENZIP_FILE_MODE; break;
      case ISO_COMPRESS:         mode = ISO_FILE_MODE; break;
      case RPM_COMPRESS:         mode = RPM_FILE_MODE; break;
      case RAR_COMPRESS:         mode = RAR_FILE_MODE; break;
      case TAPE_DIR_NO_COMPRESS:
      case TAPE_DIR_COMPRESS_COMPRESS:
      case TAPE_DIR_FREEZE_COMPRESS:
      case TAPE_DIR_GZIP_COMPRESS:
      case TAPE_DIR_BZIP_COMPRESS:
                                 mode = TAPE_MODE;     break;
      default:                   mode = TAR_FILE_MODE; break;
    }
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


  if( mode == TAPE_MODE )
  {
    /* zugehoeriges tape-device ermitteln */
    /*------------------------------------*/

    if( GetTapeDeviceName() )
    {
      return( -1 );
    }
  }


  if( mode != DISK_MODE && mode != USER_MODE)
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
    /* This fallback can be removed once libarchive is a hard dependency */
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
    (void) memcpy( (char *) &disk_statistic, 
		   (char *) &statistic,
		   sizeof( Statistic )
		 );
  } 
    
  (void) SetFileSpec( statistic.file_spec );
/*  SetKindOfSort( statistic.kind_of_sort ); */
  
  return( result );
}





int GetNewLoginPath(char *path)
{
  int result;
  char *cptr;
  char aux[PATH_LENGTH * 2 + 1]= "";
  
  result = -1;

  ClearHelp();

  MvAddStr( LINES - 2, 1, "NEW LOGIN-PATH:" );

  strcpy(aux,path);
  if( mode == LL_FILE_MODE && *path == '<' )
  {
    for( cptr = aux; (*cptr = *(cptr + 1)); cptr++ ) 
      ;
    if( aux[strlen(aux) - 1] == '>' ) aux[strlen(aux) - 1 ] = '\0';
  }

  if( InputString( aux, LINES - 2, 17, 0, COLS - 18, "\r\033" ) == CR )
  {
    NormPath(aux, path);
    result = 0;
  }
 
return( result );
}