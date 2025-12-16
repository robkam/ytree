/***************************************************************************
 *
 * pipe.c
 * Umlenken von Datei-Inhalten zu einem Kommando
 *
 ***************************************************************************/


#include "ytree.h"


extern int chdir(const char *);




int Pipe(DirEntry *dir_entry, FileEntry *file_entry)
{
  static char input_buffer[PATH_LENGTH + 3] = "| ";
  char file_name_path[PATH_LENGTH+1];
  int  result = -1;
  FILE *pipe_fp;
  char cwd[PATH_LENGTH + 1];
  char path[PATH_LENGTH + 1];

  (void) GetRealFileNamePath( file_entry, file_name_path );

  ClearHelp();

  MvAddStr( LINES - 2, 1, "Pipe-Command:" );
  if( GetPipeCommand( &input_buffer[2] ) == 0 )
  {
    move( LINES - 2, 1 ); clrtoeol();

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        WARNING("getcwd failed*\".\"assumed");
        strcpy(cwd, ".");
    }

    if (mode == DISK_MODE || mode == USER_MODE) {
        if (chdir(GetPath(dir_entry, path))) {
            snprintf(message, MESSAGE_LENGTH, "Can't change directory to*\"%s\"", path);
            MESSAGE(message);
            if (chdir(cwd) != 0) { /* Attempt to restore CWD */
                /* Handle restore failure if needed, though mostly fatal/unrecoverable context */
            }
            return -1;
        }
    } else { /* ARCHIVE_MODE */
        char archive_dir[PATH_LENGTH + 1];
        char *last_slash;
        strcpy(archive_dir, statistic.login_path);
        last_slash = strrchr(archive_dir, '/');
        if (last_slash) {
            if (last_slash == archive_dir) { /* Root directory, e.g., "/archive.zip" */
                *(last_slash + 1) = '\0';
            } else {
                *last_slash = '\0';
            }
            if (chdir(archive_dir) != 0) {
                 snprintf(message, MESSAGE_LENGTH, "Can't change directory to*\"%s\"", archive_dir);
                 MESSAGE(message);
                 if (chdir(cwd) != 0) { /* Attempt to restore CWD */
                 }
                 return -1;
            }
        }
    }


    /* Suspend curses to allow external command to run correctly */
    endwin();
    SuspendClock();

    pipe_fp = popen(&input_buffer[2], "w");
    if (pipe_fp == NULL) {
        (void)snprintf(message, MESSAGE_LENGTH, "Could not execute pipe command*\"%s\"*%s", &input_buffer[2], strerror(errno));
        /* Restore screen before showing message */
        clearok(stdscr, TRUE);
        refresh();
        MESSAGE(message);
        result = -1;
    } else {
        if( mode == DISK_MODE || mode == USER_MODE )
        {
            int in_fd;
            char buffer[4096];
            ssize_t bytes_read;
            in_fd = open(file_name_path, O_RDONLY);
            if (in_fd != -1) {
                while ((bytes_read = read(in_fd, buffer, sizeof(buffer))) > 0) {
                    if (fwrite(buffer, 1, bytes_read, pipe_fp) < bytes_read) {
                        /* Handle pipe write error */
                        break;
                    }
                }
                close(in_fd);
            }
        }
        else
        {
          /* ARCHIVE_MODE */
          /*--------------*/
    #ifdef HAVE_LIBARCHIVE
            char *archive = statistic.login_path;
            ExtractArchiveEntry(archive, file_name_path, fileno(pipe_fp));
    #endif
        }
        result = pclose(pipe_fp);
    }

    /* Change back to original directory before interacting with user */
    if (chdir(cwd) != 0) {
        /* This is a critical error, but we're outside curses mode. */
        /* We can't use MESSAGE(). We'll proceed to HitReturnToContinue() */
        /* which will at least alert the user something is wrong. */
    }

    /* Let user see output, then restore screen */
    HitReturnToContinue();
    clearok(stdscr, TRUE);
    refresh();
  }
  else
  {
    move( LINES - 2, 1 ); clrtoeol();
  }

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
    (void) snprintf( message, MESSAGE_LENGTH,
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
      (void) snprintf( message, MESSAGE_LENGTH, "Write-Error!*%s", strerror(errno) );
      MESSAGE( message );
      (void) close( i );
      return( -1 );
    }
  }

  (void) close( i );

  return( 0 );
}