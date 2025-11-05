/***************************************************************************
 *
 * Edit-Kommando-Bearbeitung
 *
 ***************************************************************************/


#include "ytree.h"



int Edit(DirEntry * dir_entry, char *file_path)
{
  char *command_line;
  int  result = -1;
  char *file_p_aux=NULL;
  char cwd[PATH_LENGTH + 1];
  char path[PATH_LENGTH + 1];


  if( mode != DISK_MODE && mode != USER_MODE )
  {
    beep();
    return( -1 );
  }

  if( access( file_path, R_OK ) )
  {
    (void) sprintf( message, 
		    "Edit not possible!*\"%s\"*%s", 
		    file_path, 
		    strerror(errno) 
		  );
    MESSAGE( message );
    ESCAPE;
  }

  if( ( file_p_aux = (char *)malloc( COMMAND_LINE_LENGTH ) ) == NULL )
  {
    ERROR_MSG( "Malloc failed*ABORT" );
    exit( 1 );
  }
  StrCp(file_p_aux, file_path);

  if( ( command_line = (char *)malloc( COMMAND_LINE_LENGTH ) ) == NULL )
  {
    ERROR_MSG( "Malloc failed*ABORT" );
    exit( 1 );
  }
  (void) strcpy( command_line, EDITOR );
  (void) strcat( command_line, " \"" );
  (void) strcat( command_line, file_p_aux );
  (void) strcat( command_line, "\"" );
  free( file_p_aux);

  /*  result = SystemCall(command_line);
    --crb3 29apr02: perhaps finally eliminate the problem with jstar writing new
    files to the ytree starting cwd. new code grabbed from execute.c.
    --crb3 01oct02: move getcwd operation within the IF DISKMODE stuff.
  */                                                                              

  if (mode == DISK_MODE)
  {
    if (getcwd(cwd, PATH_LENGTH) == NULL)
    {
            WARNING("getcwd failed*\".\"assumed");
            (void) strcpy(cwd, ".");
    }

    if (chdir(GetPath(dir_entry, path)))
    {
            (void) sprintf(message, "Can't change directory to*\"%s\"", path);
            MESSAGE(message);
    }else{
            result = SystemCall(command_line);
    }
    if(chdir(cwd))
    {
            (void) sprintf(message, "Can't change directory to*\"%s\"", cwd);
            MESSAGE(message);
    }
  }else{
    result = SystemCall(command_line);
  }                                                                         
  free( command_line );
 
FNC_XIT:

  return( result );
}