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
    char cwd[PATH_LENGTH + 1];
    char path[PATH_LENGTH + 1];
    int result = -1;

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

        if (getcwd(cwd, PATH_LENGTH) == NULL) {
            WARNING("getcwd failed*\".\"assumed");
            strcpy(cwd, ".");
        }

        if (mode == DISK_MODE || mode == USER_MODE) {
            if (chdir(GetPath(dir_entry, path))) {
                snprintf(message, MESSAGE_LENGTH, "Can't change directory to*\"%s\"", path);
                MESSAGE(message);
            } else {
                refresh();
                result = QuerySystemCall(final_command, &CurrentVolume->vol_stats);
            }
            if (chdir(cwd)) {
                snprintf(message, MESSAGE_LENGTH, "Can't change directory to*\"%s\"", cwd);
                MESSAGE(message);
            }
        } else {
            /* Execute is disabled in archive mode, but handle defensively */
            refresh();
            result = QuerySystemCall(final_command, &CurrentVolume->vol_stats);
        }
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



int GetSearchCommandLine(char *command_line)
{
  int  result;
  int  pos;
  char *cptr;

  result = -1;

  ClearHelp();

  MvAddStr( LINES - 2, 1, "SEARCH UNTAG COMMAND: " );
  strcpy( command_line, SEARCHCOMMAND );

  cptr = strstr( command_line, "{}" );
  if(cptr) {
    pos = (cptr - command_line) - 1;
    if(pos < 0)
      pos = 0;
  } else {
    pos = 0;
  }
  if( InputString( command_line, LINES - 2, 23, pos, COLS - 24, "\r\033", HST_SEARCH ) == CR )
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
  int i, result;
  char c;
  char *cptr;

  command_line[0] = '\0';
  cptr = command_line;

  walking_package->new_fe_ptr = fe_ptr;  /* unchanged */

  for( i=0; (c = walking_package->function_data.execute.command[i]); i++ )
  {
    if( c == '{' && walking_package->function_data.execute.command[i+1] == '}' )
    {
      (void) GetFileNamePath( fe_ptr, cptr );
      cptr = &command_line[ strlen( command_line ) ];
      i++;
    }
    else
    {
      *cptr++ = c;
    }
  }
  *cptr = '\0';

  result = SilentSystemCallEx( command_line, FALSE, &CurrentVolume->vol_stats );

  /* Ignore Result */
  /*---------------*/

  return( result );
}