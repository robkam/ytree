/***************************************************************************
 *
 * execute.c
 * Executing system commands
 *
 ***************************************************************************/


#include "ytree.h"


extern int chdir(const char *);


int Execute(DirEntry *dir_entry, FileEntry *file_entry)
{
    static char command_template[COMMAND_LINE_LENGTH + 1];
    char expanded_command[COMMAND_LINE_LENGTH + 1];
    char *final_command;
    char path[PATH_LENGTH + 1];
    int result = -1;
    int start_dir_fd;

    /* Clear the template before use, except for the executable file case */
    command_template[0] = '\0';
    if (file_entry) {
        if (file_entry->stat_struct.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) {
            /* If the file is executable, default to its name. StrCp handles escaping. */
            StrCp(command_template, file_entry->name);
        }
    }

    MvAddStr(LINES - 2, 1, "COMMAND:");
    if (GetCommandLine(command_template) == 0) {
        /* Check if placeholder expansion is needed */
        if (strstr(command_template, "{}") != NULL) {
            char *in_ptr = command_template;
            char *out_ptr = expanded_command;

            while (*in_ptr) {
                if (*in_ptr == '{' && *(in_ptr + 1) == '}') {
                    char substitution_name[PATH_LENGTH + 1];
                    char escaped_name[PATH_LENGTH * 2 + 1];

                    if (file_entry) {
                        /* In file context, {} is the filename, as we chdir first. */
                        strcpy(substitution_name, file_entry->name);
                    } else {
                        /* In directory context, {} means the current dir "." */
                        strcpy(substitution_name, ".");
                    }

                    /* The name may contain spaces, so escape it for the shell. */
                    StrCp(escaped_name, substitution_name);

                    size_t len = strlen(escaped_name);
                    if ((out_ptr - expanded_command) + len < COMMAND_LINE_LENGTH) {
                        strcpy(out_ptr, escaped_name);
                        out_ptr += len;
                    }
                    in_ptr += 2; /* Skip "{}" */
                } else {
                    *out_ptr++ = *in_ptr++;
                }
            }
            *out_ptr = '\0';
            final_command = expanded_command;
        } else {
            final_command = command_template;
        }

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
            } else {
                refresh();
                result = QuerySystemCall(final_command, &CurrentVolume->vol_stats);

                /* Restore original directory */
                if (fchdir(start_dir_fd) == -1) {
                    snprintf(message, MESSAGE_LENGTH, "Error restoring directory*%s", strerror(errno));
                    MESSAGE(message);
                }
            }
        } else {
            /* Execute is disabled in archive mode, but handle defensively */
            refresh();
            result = QuerySystemCall(final_command, &CurrentVolume->vol_stats);
        }

        close(start_dir_fd);
    }

    return result;
}




int GetCommandLine(char *command_line)
{
  int result;

  result = -1;

  ClearHelp();

  MvAddStr( LINES - 2, 1, "COMMAND: " );
  if( InputString( command_line, LINES - 2, 10, 0, COLS - 11, "\r\033", HST_EXEC ) == CR )
  {
    move( LINES - 2, 1 ); clrtoeol();
    result = 0;
  }

  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}



int GetSearchCommandLine(char *command_line, const char *prompt)
{
  int  result;
  int  pos;
  char *cptr;

  result = -1;

  ClearHelp();

  /* Use the provided prompt string dynamically */
  MvAddStr( LINES - 2, 1, (char *)prompt );
  strcpy( command_line, SEARCHCOMMAND );

  cptr = strstr( command_line, "{}" );
  if(cptr) {
    pos = (cptr - command_line) - 1;
    if(pos < 0)
      pos = 0;
  } else {
    pos = 0;
  }

  /* Calculate dynamic input start position based on prompt length */
  int input_x = 1 + strlen(prompt);

  if( InputString( command_line, LINES - 2, input_x, pos, COLS - input_x - 1, "\r\033", HST_SEARCH ) == CR )
  {
    move( LINES - 2, 1 ); clrtoeol();
    result = 0;
  }

  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}



int ExecuteCommand(FileEntry *fe_ptr, WalkingPackage *walking_package)
{
  char command_line[COMMAND_LINE_LENGTH + 1];
  char raw_path[PATH_LENGTH + 1];
  char quoted_path[PATH_LENGTH * 2 + 1];
  const char *template_ptr;
  int i, dest_idx;
  size_t path_len;

  walking_package->new_fe_ptr = fe_ptr;

  /* 1. Get the raw filename/path */
  (void) GetFileNamePath( fe_ptr, raw_path );

  /* 2. Quote/Escape the path using StrCp (handles spaces/special chars) */
  StrCp( quoted_path, raw_path );
  path_len = strlen( quoted_path );

  /* 3. Build the final command string */
  template_ptr = walking_package->function_data.execute.command;
  dest_idx = 0;

  for( i = 0; template_ptr[i] != '\0' && dest_idx < COMMAND_LINE_LENGTH; )
  {
    /* Check for %s substitution */
    if( template_ptr[i] == '%' && template_ptr[i+1] == 's' )
    {
      if( dest_idx + path_len < COMMAND_LINE_LENGTH )
      {
        strcpy( &command_line[dest_idx], quoted_path );
        dest_idx += path_len;
      }
      i += 2; /* Skip %s */
    }
    /* Check for {} substitution (Legacy support) */
    else if( template_ptr[i] == '{' && template_ptr[i+1] == '}' )
    {
      if( dest_idx + path_len < COMMAND_LINE_LENGTH )
      {
        strcpy( &command_line[dest_idx], quoted_path );
        dest_idx += path_len;
      }
      i += 2; /* Skip {} */
    }
    else
    {
      command_line[dest_idx++] = template_ptr[i++];
    }
  }
  command_line[dest_idx] = '\0';

  return SilentSystemCallEx( command_line, FALSE, &CurrentVolume->vol_stats );
}