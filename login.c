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
  char   command_line[COMMAND_LINE_LENGTH + 1];
  char   cat_file_name[PATH_LENGTH+1];
  int    file_method = 0;
  int    pid;
  int    p[2];
  int    depth, l = 0;
  FILE   *f;
  int    status;
  int    result = 0;

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


  if( STAT_( path, &stat_struct ) )
  {
    /* Stat failed */
    /*-------------*/

    (void) sprintf( message, "Can't access*\"%s\"*%s", path, strerror(errno) );
    MESSAGE( message );
    return( -1 );
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
   
    if( pipe( p ) )
    {
      ERROR_MSG( "pipe failed" );
      return( -1 );
    }


    if( file_method == ZOO_COMPRESS )
    {
      /* zoo vom ZOO_FILE */
      /*------------------*/
    
      (void) sprintf( command_line, "%s '%s'", 
                      ZOOLIST, 
                      statistic.login_path
                    );
    }
    else if( file_method == RPM_COMPRESS )
    {
      /* rpm vom RPM_FILE */
      /*------------------*/
    
      (void) sprintf( command_line, "%s '%s'", 
                      RPMLIST, 
                      statistic.login_path
                    );
    }
    else if( file_method == LHA_COMPRESS )
    {
      /* LHA_FILE: now uses 7z listing */
    
      (void) sprintf( command_line, "%s '%s'", 
		      LHALIST, 
		      statistic.login_path
		    );
    }
    else if( file_method == ZIP_COMPRESS )
    {
      /* ZIP_FILE: now uses 7z listing */
    
      (void) sprintf( command_line, "%s '%s'", 
		      ZIPLIST, 
		      statistic.login_path
		    );
    } 
    else if( file_method == SEVENZIP_COMPRESS )
    {
      /* 7z_FILE */
    
      (void) sprintf( command_line, "%s '%s'", 
		      SEVENZIPLIST, 
		      statistic.login_path
		    );
    } 
    else if( file_method == ISO_COMPRESS )
    {
      /* ISO_FILE */
    
      (void) sprintf( command_line, "%s '%s'", 
		      ISOLIST, 
		      statistic.login_path
		    );
    } 
    else if( file_method == ARC_COMPRESS )
    {
      /* ARC_FILE */
    
      (void) sprintf( command_line, "%s '%s'", 
		      ARCLIST, 
		      statistic.login_path
		    );
    }
    else if( file_method == RAR_COMPRESS )
    {
      /* RAR_FILE: now uses 7z listing */
    
      (void) sprintf( command_line, "%s '%s'", 
		      RARLIST, 
		      statistic.login_path
		    );
    }
    else if( file_method == FREEZE_COMPRESS )
    {
      /* melt < TAR_FILE | gtar tvf - */
    
      (void) sprintf( command_line, "%s < '%s' %s | %s", 
		      MELT, 
		      statistic.login_path,
		      ERR_TO_STDOUT,
		      TARLIST
		    );
    }
    else if( file_method == MULTIPLE_FREEZE_COMPRESS )
    {
      (void) strncpy( cat_file_name, statistic.login_path, l - 2 );
      (void) strcpy( &cat_file_name[l - 2], "*" );

      /* cat TAR_FILE | melt | gtar tvf - */
    
      (void) sprintf( command_line, "%s '%s' %s | %s | %s", 
		      CAT,
		      cat_file_name,
		      ERR_TO_STDOUT,
		      MELT, 
		      TARLIST
		    );
    }
    else if( file_method == COMPRESS_COMPRESS )
    {
      /* uncompress < TAR_FILE | gtar tvf - : Now using 7z command */
      
      (void) sprintf( command_line, "%s -so x < '%s' %s | %s", 
		      UNCOMPRESS, /* UNCOMPRESS is now 7z -so x */
		      statistic.login_path,
		      ERR_TO_STDOUT,
		      TARLIST
		    );
    }
    else if( file_method == MULTIPLE_COMPRESS_COMPRESS )
    {
      (void) strncpy( cat_file_name, statistic.login_path, l - 2 );
      (void) strcpy( &cat_file_name[l - 2], "*" );
        
      /* cat TAR_FILE.X* | uncompress | gtar tvf - : Now using 7z command */
    
      (void) sprintf( command_line, "%s %s | %s -so x %s | %s", 
		      CAT,
		      cat_file_name,
		      UNCOMPRESS, /* UNCOMPRESS is now 7z -so x */
		      ERR_TO_STDOUT,
		      TARLIST
		    );
    }
    else if( file_method == GZIP_COMPRESS )
    {
      /* gunzip < TAR_FILE | gtar tvf - : Now using 7z command */
    
      (void) sprintf( command_line, "%s < '%s' %s | %s", 
		      GNUUNZIP, /* GNUUNZIP is now 7z -so x */
		      statistic.login_path,
		      ERR_TO_STDOUT,
		      TARLIST
		    );
    }
    else if( file_method == MULTIPLE_GZIP_COMPRESS )
    {
      (void) strncpy( cat_file_name, statistic.login_path, l - 2 );
      (void) strcpy( &cat_file_name[l - 2], "*" );
        
      /* cat TAR_FILE.X* | gunzip | gtar tvf - : Now using 7z command */
    
      (void) sprintf( command_line, "%s %s | %s < '%s' %s | %s", 
		      CAT,
		      cat_file_name,
		      GNUUNZIP, /* GNUUNZIP is now 7z -so x */
		      statistic.login_path,
		      ERR_TO_STDOUT,
		      TARLIST
		    );
    }
    else if( file_method == BZIP_COMPRESS )
    {
      /* bunzip2 < TAR_FILE | gtar tvf - : Now using 7z command */
    
      (void) sprintf( command_line, "%s < '%s' %s | %s", 
		      BUNZIP, /* BUNZIP is now 7z -so x */
		      statistic.login_path,
		      ERR_TO_STDOUT,
		      TARLIST
		    );
    }
    else if( file_method == ZSTD_COMPRESS )
    {
      /* zstdcat < TAR_FILE | gtar tvf - */
    
      (void) sprintf( command_line, "%s < '%s' %s | %s", 
		      ZSTDCAT, 
		      statistic.login_path,
		      ERR_TO_STDOUT,
		      TARLIST
		    );
    }
    else if( file_method == LZIP_COMPRESS )
    {
      /* lzip -dc < TAR_FILE | gtar tvf - */
    
      (void) sprintf( command_line, "%s < '%s' %s | %s", 
		      LUNZIP, 
		      statistic.login_path,
		      ERR_TO_STDOUT,
		      TARLIST
		    );
    }
    else if( file_method == NO_COMPRESS )
    {
      /* NO_COMPRESS */

      /* gtar tvf - < TAR_FILE */
    
      (void) sprintf( command_line, "%s < '%s'", 
		      TARLIST,
		      statistic.login_path
		    );
    }
    else if( file_method == TAPE_DIR_FREEZE_COMPRESS )
    {
      /* melt < TAR_FILE */
    
      (void) sprintf( command_line, "%s < '%s'", 
		      MELT, 
		      statistic.login_path
		    );
    }
    else if( file_method == TAPE_DIR_COMPRESS_COMPRESS )
    {
      /* uncompress < TAR_FILE: Now using 7z command */
    
      (void) sprintf( command_line, "%s < '%s'", 
		      UNCOMPRESS, /* UNCOMPRESS is now 7z -so x */
		      statistic.login_path
		    );
    }
    else if( file_method == TAPE_DIR_GZIP_COMPRESS )
    {
      /* gunzip < TAR_FILE: Now using 7z command */
    
      (void) sprintf( command_line, "%s < '%s'", 
		      GNUUNZIP, /* GNUUNZIP is now 7z -so x */
		      statistic.login_path
		    );
    }
    else if( file_method == TAPE_DIR_BZIP_COMPRESS )
    {
      /* bunzip2 < TAR_FILE: Now using 7z command */
    
      (void) sprintf( command_line, "%s < '%s'", 
		      BUNZIP, /* BUNZIP is now 7z -so x */
		      statistic.login_path
		    );
    }
    else if( file_method == TAPE_DIR_NO_COMPRESS )
    {
      /* cat < TAR_FILE */
    
      (void) sprintf( command_line, "%s < '%s'", 
		      CAT,
		      statistic.login_path
		    );
    }
    else
    {
      (void) sprintf( message, "unknown file_method %d", file_method );
      ERROR_MSG( message );
      *command_line = '\0';
      close( p[0] );
      close( p[1] );
      return( -1 );
    }
    
    (void) strcat( command_line, ERR_TO_NULL );

#ifdef DEBUG
  fprintf( stderr, "system( \"%s\" )\n", command_line );
#endif

    pid = fork();

    if( pid == -1 )
    {
      ERROR_MSG( "can't fork()" );
      (void) close( p[0] );
      (void) close( p[1] );
      return( -1 );
    }
    else if( pid == 0 )
    {
      /* Sohn */
      /*------*/
 
      (void) close( p[0] );
      (void) close( 1 );
      if(dup( p[1] ) == -1)
      {
	/* (void) fprintf(stderr, "dup failed\n" ); */
      }
      (void) close( p[1] );

      if( result == 0 && SilentSystemCallEx( command_line, FALSE ) )
      {
        result = 1;
	/* (void) fprintf(stderr, "system(%s)*failed\n", command_line ); */
      }
      exit( result );
    }
    else
    {
      /* Vater */
      /*-------*/

      (void) close( p[1] );
      status = 0;

      if( ( f = fdopen( p[0], "r" ) ) == NULL )
      {
	ERROR_MSG( "fdopen failed" );
	return( -1 );
      }
      
      /* Use the single dispatcher function */
      if (ReadTreeFromArchive(statistic.tree, f, mode))
      {
          ERROR_MSG("ReadTreeFromArchive Failed");
          (void)fclose(f);
          (void)wait(&status);
          return(-1);
      }
      
      (void) wait( &status );
      if(status)
      {
        sprintf( message, "ReadTreeFromArchive failed*can't execute*%s", command_line );
        MESSAGE( message );
      }
      (void) fclose( f );
    }
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
  
  return( 0 );
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