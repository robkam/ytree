/***************************************************************************
 *
 * src/cmd/execute.c
 * Executing system commands
 *
 ***************************************************************************/


#include "ytree.h"


extern int chdir(const char *);


#ifdef HAVE_LIBARCHIVE
static int ExecuteArchiveFile(DirEntry *dir_entry, FileEntry *file_entry, char *cmd_template, Statistic *s) {
    char temp_path[PATH_LENGTH];
    char command_line[COMMAND_LINE_LENGTH];
    char quoted_temp[PATH_LENGTH * 2 + 1];
    char internal_path[PATH_LENGTH];
    char dir_path[PATH_LENGTH];
    char root_path[PATH_LENGTH+1];
    char relative_path[PATH_LENGTH+1];
    int fd_tmp;
    int result = -1;
    char *in_ptr, *out_ptr;
    size_t len;

    /* 1. Create Temporary File */
    strcpy(temp_path, "/tmp/ytree_XXXXXX");
    fd_tmp = mkstemp(temp_path);
    if (fd_tmp == -1) {
        ERROR_MSG("Could not create temporary file for execution");
        return -1;
    }

    /* 2. Extract File Content */
    if (file_entry) {
        GetPath(file_entry->dir_entry, dir_path);

        /* Rebuild the path relative to the archive root (login_path) */
        GetPath(s->tree, root_path);

        if (strncmp(dir_path, root_path, strlen(root_path)) == 0) {
            char *ptr = dir_path + strlen(root_path);
            if (*ptr == FILE_SEPARATOR_CHAR) ptr++;
            strcpy(relative_path, ptr);
        } else {
            strcpy(relative_path, dir_path);
        }

        if (relative_path[0] != '\0' && relative_path[strlen(relative_path)-1] != FILE_SEPARATOR_CHAR) {
            strcat(relative_path, FILE_SEPARATOR_STRING);
        }
        strcat(relative_path, file_entry->name);

        strcpy(internal_path, relative_path);

        if (ExtractArchiveEntry(s->login_path, internal_path, fd_tmp) != 0) {
            ERROR_MSG("Failed to extract file for execution");
            close(fd_tmp);
            unlink(temp_path);
            return -1;
        }
    } else {
        close(fd_tmp);
        unlink(temp_path);
        return -1;
    }
    close(fd_tmp);

    /* 3. Substitute {} with temp path */
    StrCp(quoted_temp, temp_path); /* Escape spaces */

    in_ptr = cmd_template;
    out_ptr = command_line;

    while (*in_ptr) {
        if (*in_ptr == '{' && *(in_ptr + 1) == '}') {
            len = strlen(quoted_temp);
            if ((out_ptr - command_line) + len < COMMAND_LINE_LENGTH) {
                strcpy(out_ptr, quoted_temp);
                out_ptr += len;
            }
            in_ptr += 2;
        } else {
            *out_ptr++ = *in_ptr++;
        }
    }
    *out_ptr = '\0';

    /* 4. Execute */
    refresh();
    result = SilentSystemCall(command_line, s);

    /* 5. Cleanup */
    unlink(temp_path);

    return result;
}
#endif


int Execute(DirEntry *dir_entry, FileEntry *file_entry, Statistic *s)
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

    if (GetCommandLine(command_template) == 0) {
        /* ARCHIVE MODE HANDLER */
#ifdef HAVE_LIBARCHIVE
        if (mode == ARCHIVE_MODE) {
            result = ExecuteArchiveFile(dir_entry, file_entry, command_template, s);
            /* Single file execute usually expects a pause to see output */
            /* SilentSystemCall (used in ExecuteArchiveFile) doesn't pause. */
            /* We should pause here manually if needed. */
            HitReturnToContinue();
            return result;
        }
#endif

        /* Check if placeholder expansion is needed */
        if (strstr(command_template, "{}") != NULL) {
            char *in_ptr = command_template;
            char *out_ptr = expanded_command;

            while (*in_ptr) {
                if (*in_ptr == '{' && *(in_ptr + 1) == '}') {
                    char substitution_name[PATH_LENGTH + 1];
                    char escaped_name[PATH_LENGTH * 2 + 1];
                    size_t len;

                    if (file_entry) {
                        /* In file context, {} is the filename, as we chdir first. */
                        strcpy(substitution_name, file_entry->name);
                    } else {
                        /* In directory context, {} means the current dir "." */
                        strcpy(substitution_name, ".");
                    }

                    /* The name may contain spaces, so escape it for the shell. */
                    StrCp(escaped_name, substitution_name);

                    len = strlen(escaped_name);
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
                result = QuerySystemCall(final_command, s);

                /* Restore original directory */
                if (fchdir(start_dir_fd) == -1) {
                    snprintf(message, MESSAGE_LENGTH, "Error restoring directory*%s", strerror(errno));
                    MESSAGE(message);
                }
            }
        } else {
            /* Execute is disabled in archive mode, but handle defensively */
            refresh();
            result = QuerySystemCall(final_command, s);
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

  if( UI_ReadString( "COMMAND:", command_line, COMMAND_LINE_LENGTH, HST_EXEC ) == CR )
  {
    move( LINES - 2, 1 ); clrtoeol();
    result = 0;
  }

  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}



int GetSearchCommandLine(char *command_line, char *raw_pattern)
{
  int  result;
  char input_buf[256];

  result = -1;

  ClearHelp();

  input_buf[0] = '\0';

  if( UI_ReadString( "SEARCH TAGGED:", input_buf, 256, HST_SEARCH ) == CR )
  {
    if( raw_pattern ) {
        strncpy(raw_pattern, input_buf, 255);
        raw_pattern[255] = '\0';
    }

    /* Construct command line: grep -i "pattern" {} */
    snprintf(command_line, COMMAND_LINE_LENGTH, "grep -i \"%s\" {}", input_buf);

    move( LINES - 2, 1 ); clrtoeol();
    result = 0;
  }

  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}



int ExecuteCommand(FileEntry *fe_ptr, WalkingPackage *walking_package, Statistic *s)
{
  char command_line[COMMAND_LINE_LENGTH + 1];
  char raw_path[PATH_LENGTH + 1];
  char quoted_path[PATH_LENGTH * 2 + 1];
  const char *template_ptr;
  int i, dest_idx;
  size_t path_len;

  walking_package->new_fe_ptr = fe_ptr;

#ifdef HAVE_LIBARCHIVE
  /* Archive Mode Tagged Execution */
  if (mode == ARCHIVE_MODE) {
      /* Reconstruct the template to pass to single-file handler */
      return ExecuteArchiveFile(fe_ptr->dir_entry, fe_ptr, walking_package->function_data.execute.command, s);
  }
#endif

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

  return SilentSystemCallEx( command_line, FALSE, s );
}