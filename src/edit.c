/***************************************************************************
 *
 * edit.c
 * Edit command processing
 *
 ***************************************************************************/


#include "ytree.h"



int Edit(DirEntry * dir_entry, char *file_path)
{
  char *command_line;
  int  result = -1;
  char *file_p_aux=NULL;
  char path[PATH_LENGTH + 1];
  int start_dir_fd;


  if( mode != DISK_MODE && mode != USER_MODE )
  {
    return( -1 );
  }

  if( access( file_path, R_OK ) )
  {
    (void) snprintf( message, MESSAGE_LENGTH,
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
  /*
   * Replaced unsafe strcpy/strcat construction with snprintf
   * to ensure bounds checking and secure string handling.
   */
  (void) snprintf(command_line, COMMAND_LINE_LENGTH, "%s \"%s\"", EDITOR, file_p_aux);

  free( file_p_aux);

  /*  result = SystemCall(command_line);
    --crb3 29apr02: perhaps finally eliminate the problem with jstar writing new
    files to the ytree starting cwd. new code grabbed from execute.c.
    --crb3 01oct02: move getcwd operation within the IF DISKMODE stuff.
  */

  if (mode == DISK_MODE || mode == USER_MODE)
  {
    /* Robustly save current working directory using a file descriptor */
    start_dir_fd = open(".", O_RDONLY);
    if (start_dir_fd == -1) {
        snprintf(message, MESSAGE_LENGTH, "Error saving current directory context*%s", strerror(errno));
        MESSAGE(message);
        free(command_line);
        return -1;
    }

    if (chdir(GetPath(dir_entry, path)))
    {
            (void) snprintf(message, MESSAGE_LENGTH, "Can't change directory to*\"%s\"", path);
            MESSAGE(message);
    }else{
            result = SystemCall(command_line, &CurrentVolume->vol_stats);

            /* Restore original directory */
            if (fchdir(start_dir_fd) == -1) {
                snprintf(message, MESSAGE_LENGTH, "Error restoring directory*%s", strerror(errno));
                MESSAGE(message);
            }
    }
    close(start_dir_fd);
  }else{
    result = SystemCall(command_line, &CurrentVolume->vol_stats);
  }
  free( command_line );

FNC_XIT:

  return( result );
}