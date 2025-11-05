/***************************************************************************
 *
 * Umlenken von Datei-Inhalten zu einem Kommando
 *
 ***************************************************************************/


#include "ytree.h"


extern int chdir(const char *);




int Pipe(DirEntry *dir_entry, FileEntry *file_entry)
{
  static char input_buffer[PATH_LENGTH + 3] = "| ";
  char file_name_path[PATH_LENGTH+1];
  char file_name_p_aux[PATH_LENGTH+1];
  char *command_line;
  char *archive;
  char cwd[PATH_LENGTH+1];
  char path[PATH_LENGTH+1];
  int  result;

  result = -1;

  if( ( command_line = (char *)malloc( COMMAND_LINE_LENGTH + 1 ) ) == NULL )
  {
    ERROR_MSG( "Malloc failed*ABORT" );
    exit( 1 );
  }

  (void) GetRealFileNamePath( file_entry, file_name_path );
  (void) StrCp( file_name_p_aux, file_name_path);
  
  ClearHelp();

  MvAddStr( LINES - 2, 1, "Pipe-Command:" );
  if( GetPipeCommand( &input_buffer[2] ) == 0 )
  {
    move( LINES - 2, 1 ); clrtoeol();
    
    if( getcwd( cwd, PATH_LENGTH ) == NULL )
    {
      WARNING( "getcwd failed*\".\"assumed" );
      (void) strcpy( cwd, "." );
    }

    (void) GetPath( dir_entry, path );
      

    if( mode == DISK_MODE || mode == USER_MODE )
    {
      /* Kommandozeile zusammenbasteln */
      /*-------------------------------*/
  
      (void) sprintf( command_line, "%s %s %s", 
				    CAT, 
				    file_name_p_aux, 
				    input_buffer 
		    );
    }
    else
    {
      /* TAR/ZOO/7z/ISO/ZIP_FILE_MODE */
      /*------------------------------*/

      archive = (mode == TAPE_MODE) ? statistic.tape_name : statistic.login_path;

      MakeExtractCommandLine( command_line,
			      archive,
                              file_name_p_aux,
			      input_buffer
			    );

    }
    refresh();
    result = QuerySystemCall( command_line );
  }
  else
  {
    move( LINES - 2, 1 ); clrtoeol();
  }

  free( command_line );

  return( result );
}





int GetPipeCommand(char *pipe_command)
{
  int  result;

  result = -1;

  ClearHelp();

  MvAddStr( LINES - 2, 1, "Pipe-Command: " );
  if( InputString( pipe_command, LINES - 2, 15, 0, COLS - 16, "\r\033" ) == CR )
  {
    result = 0;
  }
  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}





  
int PipeTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package)
{
  int  i, n;
  char from_path[PATH_LENGTH+1];
  char buffer[2048];


  walking_package->new_fe_ptr = fe_ptr;  /* unchanged */

  (void) GetRealFileNamePath( fe_ptr, from_path );
  if( ( i = open( from_path, O_RDONLY ) ) == -1 )
  {
    (void) sprintf( message, 
		    "Can't open file*\"%s\"*%s", 
		    from_path, 
		    strerror(errno) 
		  );
    MESSAGE( message );
    return( -1 );
  }

  while( ( n = read( i, buffer, sizeof( buffer ) ) ) > 0 )
  {
    if( fwrite( buffer, 
		n, 
		1, 
		walking_package->function_data.pipe_cmd.pipe_file ) != 1 
      )
    {
      (void) sprintf( message, "Write-Error!*%s", strerror(errno) );
      MESSAGE( message );
      (void) close( i ); 
      return( -1 );
    }
  }

  (void) close( i );

  return( 0 );
}