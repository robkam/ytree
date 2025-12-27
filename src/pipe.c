/***************************************************************************
 *
 * pipe.c
 * Redirecting file and directory contents to a command
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
  char path[PATH_LENGTH + 1];
  int start_dir_fd;

  (void) GetRealFileNamePath( file_entry, file_name_path );

  ClearHelp();

  MvAddStr( LINES - 2, 1, "Pipe-Command:" );
  if( GetPipeCommand( &input_buffer[2] ) == 0 )
  {
    move( LINES - 2, 1 ); clrtoeol();

    /* Robustly save current working directory using a file descriptor */
    start_dir_fd = open(".", O_RDONLY);
    if (start_dir_fd == -1) {
        snprintf(message, MESSAGE_LENGTH, "Error saving current directory context*%s", strerror(errno));
        MESSAGE(message);
        return -1;
    }

    if (mode == DISK_MODE || mode == USER_MODE) {
        if (chdir(GetPath(dir_entry, path))) {
            snprintf(message, MESSAGE_LENGTH, "Can't change directory to*\"%s\"", path);
            MESSAGE(message);
            close(start_dir_fd);
            return -1;
        }
    } else { /* ARCHIVE_MODE */
        char archive_dir[PATH_LENGTH + 1];
        char *last_slash;
        strcpy(archive_dir, CurrentVolume->vol_stats.login_path);
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
                 close(start_dir_fd);
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

        /* Restore CWD before returning */
        if (fchdir(start_dir_fd) == -1) {
             /* Failed to restore, but we are aborting anyway. */
        }
        close(start_dir_fd);

        /* Restore screen before showing message */
        InitClock(); /* Re-enables curses */
        clearok(stdscr, TRUE);
        refresh();
        MESSAGE(message);
        result = -1;
        return result;
    } else {
        if( mode == DISK_MODE || mode == USER_MODE )
        {
            int in_fd;
            char buffer[4096];
            ssize_t bytes_read;
            in_fd = open(file_name_path, O_RDONLY);
            if (in_fd != -1) {
                while ((bytes_read = read(in_fd, buffer, sizeof(buffer))) > 0) {
                    /* FIX: Cast bytes_read to size_t to avoid signed/unsigned comparison warning */
                    if (fwrite(buffer, 1, bytes_read, pipe_fp) < (size_t)bytes_read) {
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
            char *archive = CurrentVolume->vol_stats.login_path;
            ExtractArchiveEntry(archive, file_name_path, fileno(pipe_fp));
    #endif
        }
        result = pclose(pipe_fp);
    }

    /* Change back to original directory before interacting with user */
    if (fchdir(start_dir_fd) == -1) {
        fprintf(stderr, "Error restoring directory: %s\n", strerror(errno));
    }
    close(start_dir_fd);

    /* Update stats as the command might have modified the disk */
    (void) GetAvailBytes( &CurrentVolume->vol_stats.disk_space, &CurrentVolume->vol_stats );

    /* Let user see output, then restore screen */
    HitReturnToContinue();
    InitClock(); /* Restores curses mode */
    clearok(stdscr, TRUE);
    refresh();
  }
  else
  {
    move( LINES - 2, 1 ); clrtoeol();
  }

  return( result );
}

int PipeDirectory(DirEntry *dir_entry)
{
  static char input_buffer[PATH_LENGTH + 3] = "| ";
  char path[PATH_LENGTH + 1];
  FILE *pipe_fp;
  FileEntry *fe;
  int result = -1;
  int start_dir_fd;

  (void) GetPath( dir_entry, path );

  ClearHelp();
  MvAddStr( LINES - 2, 1, "Pipe-Command:" );

  if( GetPipeCommand( &input_buffer[2] ) == 0 )
  {
    move( LINES - 2, 1 ); clrtoeol();

    /* Robustly save current working directory using a file descriptor */
    start_dir_fd = open(".", O_RDONLY);
    if (start_dir_fd == -1) {
        snprintf(message, MESSAGE_LENGTH, "Error saving current directory context*%s", strerror(errno));
        MESSAGE(message);
        return -1;
    }

    if( mode == DISK_MODE || mode == USER_MODE )
    {
      if( chdir( path ) )
      {
        (void) snprintf( message, MESSAGE_LENGTH, "Can't change directory to*\"%s\"", path );
        MESSAGE( message );
        close(start_dir_fd);
        return( -1 );
      }
    }

    endwin();
    SuspendClock();

    if( ( pipe_fp = popen( &input_buffer[2], "w" ) ) == NULL )
    {
      (void) snprintf( message, MESSAGE_LENGTH, "Could not execute pipe command*\"%s\"*%s",
                       &input_buffer[2], strerror(errno) );

      /* Restore CWD */
      if (fchdir(start_dir_fd) == -1) { }
      close(start_dir_fd);

      InitClock();
      clearok( stdscr, TRUE );
      refresh();
      MESSAGE( message );
      return( -1 );
    }

    for( fe = dir_entry->file; fe; fe = fe->next )
    {
      if( fe->matching )
      {
        if( !hide_dot_files || fe->name[0] != '.' )
        {
          fprintf( pipe_fp, "%s\n", fe->name );
        }
      }
    }

    pclose( pipe_fp );
    result = 0;

    /* Restore CWD */
    if (fchdir(start_dir_fd) == -1) {
        fprintf(stderr, "Error restoring directory: %s\n", strerror(errno));
    }
    close(start_dir_fd);

    HitReturnToContinue();
    InitClock();
    clearok( stdscr, TRUE );
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
  if( InputString( pipe_command, LINES - 2, 15, 0, COLS - 16, "\r\033", HST_PIPE ) == CR )
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