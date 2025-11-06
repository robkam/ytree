/***************************************************************************
 *
 * Kopieren von Dateien / Verzeichnissen
 *
 ***************************************************************************/


#include "ytree.h"



static int Copy(char *to_path, char *from_path);
static int CopyArchiveFile(char *to_path, char *from_path);



int CopyFile(Statistic *statistic_ptr, 
             FileEntry *fe_ptr, 
             unsigned char confirm, 
             char *to_file, 
             DirEntry *dest_dir_entry, 
             char *to_dir_path,       /* absoluter Pfad */
             BOOL path_copy
	    )
{
  LONGLONG    file_size;
  char        from_path[PATH_LENGTH+1];
  char        from_dir[PATH_LENGTH+1];
  char        to_path[PATH_LENGTH+1];
  char        abs_path[PATH_LENGTH+1];
  char        buffer[20];
  FileEntry   *dest_file_entry;
  FileEntry   *fen_ptr;
  struct stat stat_struct;
  int         term;
  int         result;
  DIR         *tmpdir = NULL;
  int	      refresh_dirwindow = FALSE;


  result = -1;

  (void) GetRealFileNamePath( fe_ptr, from_path );
  (void) GetPath(fe_ptr->dir_entry, from_dir);
  
  if (mode != DISK_MODE && mode != USER_MODE) {
      /* When copying FROM an archive, the destination is always a real path. */
      /* We bypass GetDirEntry and construct the path directly. */
      strcpy(to_path, to_dir_path);
      dest_dir_entry = NULL; /* Destination is not in the virtual tree */
      path_copy = FALSE; /* Path copy is meaningless from a virtual path */
  } else {
      *to_path = '\0';
      if( strcmp( to_dir_path, FILE_SEPARATOR_STRING ) )
      {
        /* not ROOT */
        /*----------*/
        (void) strcat( to_path, to_dir_path );
      }
  }


  if( path_copy )
  {
    (void) GetPath( fe_ptr->dir_entry, &to_path[strlen(to_path)] );

    /* Create destination folder (if neccessary) */
    /*-------------------------------------------*/
    strcat(to_path, FILE_SEPARATOR_STRING );

    if(*to_path != FILE_SEPARATOR_CHAR) {
         strcpy(abs_path, from_dir);
	 strcat(abs_path, FILE_SEPARATOR_STRING);
	 strcat(abs_path, to_path);
	 strcpy(to_path, abs_path);
    }

    if( MakePath( statistic_ptr->tree, to_path, &dest_dir_entry ) )
    {
     	 (void) sprintf( message, 
                         "Can't create path*\"%s\"*%s", 
                         to_path, 
                         strerror(errno) 
                         );
         MESSAGE( message );
         return( result );
    }
  }
  (void) strcat( to_path, FILE_SEPARATOR_STRING );
  if ((tmpdir = opendir(to_path)) == NULL)
    if (errno == ENOENT) {
     if ( (term =InputChoise( "Directory does not exist; create (y/N) ? ", "YN\033" ))== 'Y') 
     {
        if(*to_path != FILE_SEPARATOR_CHAR) {
          strcpy(abs_path, from_dir);
	  strcat(abs_path, FILE_SEPARATOR_STRING);
	  strcat(abs_path, to_path);
	  strcpy(to_path, abs_path);
        }
        if (MakePath(statistic_ptr->tree, to_path, &dest_dir_entry ) )
        {
                if(tmpdir) closedir(tmpdir);
                (void) sprintf( message, 
                                "Can't create path*\"%s\"*%s", 
                                to_path, 
                                strerror(errno) 
                                );
                MESSAGE( message );
                return( result );
        }
	else
	{
		refresh_dirwindow = TRUE;
	}
     }
     else
     {
        if( tmpdir)
	  closedir(tmpdir);

        return ( result );
     }
  }
  if (tmpdir) closedir(tmpdir);
  (void) strcat( to_path, to_file );


#ifdef DEBUG
  fprintf( stderr, "Copy: \"%s\" --> \"%s\"\n", from_path, to_path );
#endif /* DEBUG */

  if( !strcmp( to_path, from_path ) )
  {
    MESSAGE( "Can't copy file into itself" );
    return( result );
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
	term = InputChoise( "file exist; overwrite (Y/N) ? ", "YN\033" );
    
        if( term != 'Y' ) 
        {
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
	term = InputChoise( "file exist; overwrite (Y/N) ? ", "YN\033" );
      
        if( term != 'Y' ) 
        {
	  result = (term == 'N' ) ? 0 : -1;  /* Abort on escape */
          ESCAPE;
        }
      }
    }
  }
    

  if( !Copy( to_path, from_path ) )
  {
    /* File wurde kopiert */
    /*--------------------*/

    if( chmod( to_path, fe_ptr->stat_struct.st_mode ) == -1 ) 
    {
      sprintf( message, "Can't chmod file*\"%s\"*to mode %s*IGNORED",
               to_path, GetAttributes(fe_ptr->stat_struct.st_mode, buffer) );
      WARNING( message );
    }

    if( dest_dir_entry )
    {
      if( STAT_( to_path, &stat_struct ) )
      {
        ERROR_MSG( "Stat Failed*ABORT" );
        exit( 1 );
      }
  
      file_size = stat_struct.st_size;
    
      dest_dir_entry->total_bytes += file_size;
      dest_dir_entry->total_files++;
      statistic_ptr->disk_total_bytes += file_size;
      statistic_ptr->disk_total_files++;
      dest_dir_entry->matching_bytes += file_size;
      dest_dir_entry->matching_files++;
      statistic_ptr->disk_matching_bytes += file_size;
      statistic_ptr->disk_matching_files++;

      /* File eintragen */
      /*----------------*/

      if( ( fen_ptr = (FileEntry *) malloc( sizeof( FileEntry ) + strlen( to_file ) ) ) == NULL )
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
      fen_ptr->matching    = Match( fen_ptr->name );
      fen_ptr->next        = dest_dir_entry->file;
      fen_ptr->prev        = NULL;
      if( dest_dir_entry->file ) dest_dir_entry->file->prev = fen_ptr;
      dest_dir_entry->file = fen_ptr;
    }

    (void) GetAvailBytes( &statistic_ptr->disk_space );

    result = 0;
  }

  if( refresh_dirwindow) 
  {
  	RefreshDirWindow();
  }

FNC_XIT:
 
  move( LINES - 3, 1 ); clrtoeol();
  move( LINES - 2, 1 ); clrtoeol();
  move( LINES - 1, 1 ); clrtoeol();

  return( result );
}





int GetCopyParameter(char *from_file, BOOL path_copy, char *to_file, char *to_dir)
{
  char buffer[PATH_LENGTH + 1];

  if( from_file == NULL )
  {
    from_file = "TAGGED FILES";
    (void) strcpy( to_file, "*" );
  }
  else
  {
    (void) strcpy( to_file, from_file );
  }

  if( path_copy )
  {
    (void) sprintf( buffer, "PATHCOPY %s", from_file );
  }
  else
  {
    (void) sprintf( buffer, "COPY %s", from_file );
  }

  ClearHelp();

  MvAddStr( LINES - 3, 1, buffer );
  MvAddStr( LINES - 2, 1, "AS   ");

  if( InputString(to_file, LINES - 2, 6, 0, COLS - 6, "\r\033" ) == CR){
    MvAddStr( LINES - 1, 1, "TO   " ); 
    if( InputString( to_dir, LINES - 1, 6, 0, COLS - 6, "\r\033" ) == CR ) 
    return( 0 );
  }
  ClearHelp();
  return( -1 );
}





static int Copy(char *to_path, char *from_path)
{
  int         i, o, n;
  char        buffer[2048];

  if( mode != DISK_MODE && mode != USER_MODE )
  {
    return( CopyArchiveFile( to_path, from_path ) );
  }

#ifdef DEBUG
  fprintf( stderr, "Copy: \"%s\" --> \"%s\"\n", from_path, to_path );
#endif /* DEBUG */

  if( !strcmp( to_path, from_path ) )
  {
    MESSAGE( "Can't copy file into itself" );
    return( -1 );
  }

  if( ( i = open( from_path, O_RDONLY ) ) == -1 )
  {
    (void) sprintf( message, "Can't open file*\"%s\"*%s", from_path, strerror(errno) );
    MESSAGE( message );
    return( -1 );
  }

  if( ( o = open( to_path, 
		  O_CREAT | O_TRUNC | O_WRONLY, 
                  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH
      ) ) == -1 )
  {
    (void) sprintf( message, 
		    "Can't open file*\"%s\"*%s", 
		    to_path, 
		    strerror(errno) 
		  );
    MESSAGE( message );
    (void) close( i );
    return( -1 );
  }

  while( ( n = read( i, buffer, sizeof( buffer ) ) ) > 0 )
  {
    if( write( o, buffer, n ) != n )
    {
      (void) sprintf( message, "Write-Error!*%s", strerror(errno) );
      MESSAGE( message );
      (void) close( i ); (void) close( o );
      (void) unlink( to_path );
      return( -1 );
    }
  }

  (void) close( i ); (void) close( o );

  return( 0 );
}





  
int CopyTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package)
{
  char new_name[PATH_LENGTH+1];
  int  result = -1;

  walking_package->new_fe_ptr = fe_ptr;  /* unchanged */
 
  if( BuildFilename( fe_ptr->name,
		     walking_package->function_data.copy.to_file,
		     new_name
		   ) == 0 )
  {
    if( *new_name == '\0' )
    {
      MESSAGE( "Can't copy file to*empty name" );
    }
  
    result = CopyFile( walking_package->function_data.copy.statistic_ptr,
		       fe_ptr, 
		       walking_package->function_data.copy.confirm,
		       new_name,
		       walking_package->function_data.copy.dest_dir_entry,
		       walking_package->function_data.copy.to_path,
		       walking_package->function_data.copy.path_copy
		     );
  }

  return( result );
}





static int CopyArchiveFile(char *to_path, char *from_path)
{
  int out_fd;
  int result = -1;
  char *archive_path;

  archive_path = (mode == TAPE_MODE) ? statistic.tape_name : statistic.login_path;

  out_fd = open(to_path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if (out_fd == -1) {
    (void)sprintf(message, "Can't create destination file*\"%s\"*%s", to_path, strerror(errno));
    MESSAGE(message);
    return -1;
  }
  
#ifdef HAVE_LIBARCHIVE
  result = ExtractArchiveEntry(archive_path, from_path, out_fd);
#endif
  
  close(out_fd);

  if (result != 0) {
    (void)sprintf(message, "Can't copy file*%s*to file*%s", from_path, to_path);
    WARNING(message);
    unlink(to_path); /* Clean up partial file on failure */
  }

  return result;
}