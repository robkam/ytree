/***************************************************************************
 *
 * src/cmd/execute.c
 * Executing system commands
 *
 ***************************************************************************/


#include "ytree.h"


extern int chdir(const char *);
extern int String_Replace(char *dest, size_t dest_size, const char *src, const char *token, const char *replacement);


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

    /* 3. Substitute {} with temp path using String_Replace */
    StrCp(quoted_temp, temp_path); /* Escape spaces */

    if (String_Replace(command_line, sizeof(command_line), cmd_template, "{}", quoted_temp) != 0) {
        ERROR_MSG("Command line too long");
        unlink(temp_path);
        return -1;
    }

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
            char substitution_name[PATH_LENGTH + 1];
            char escaped_name[PATH_LENGTH * 2 + 1];

            if (file_entry) {
                /* In file context, {} is the filename, as we chdir first. */
                strncpy(substitution_name, file_entry->name, PATH_LENGTH);
            } else {
                /* In directory context, {} means the current dir "." */
                strcpy(substitution_name, ".");
            }
            substitution_name[PATH_LENGTH] = '\0';

            /* The name may contain spaces, so escape it for the shell. */
            StrCp(escaped_name, substitution_name);

            if (String_Replace(expanded_command, sizeof(expanded_command), command_template, "{}", escaped_name) != 0) {
                MESSAGE("Command line too long");
                return -1;
            }
            final_command = expanded_command;
        } else {
            final_command = command_template;
        }

        /* Robustly save current working directory using a file descriptor */
        start_dir_fd = open(".", O_RDONLY);
        if (start_dir_fd == -1) {
            MESSAGE("Error saving current directory context*%s", strerror(errno));
            return -1;
        }

        if (mode == DISK_MODE || mode == USER_MODE) {
            if (chdir(GetPath(dir_entry, path))) {
                MESSAGE("Can't change directory to*\"%s\"", path);
            } else {
                refresh();
                result = QuerySystemCall(final_command, s);

                /* Restore original directory */
                if (fchdir(start_dir_fd) == -1) {
                    MESSAGE("Error restoring directory*%s", strerror(errno));
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
  char temp_command_line[COMMAND_LINE_LENGTH + 1];
  char raw_path[PATH_LENGTH + 1];
  char quoted_path[PATH_LENGTH * 2 + 1];
  const char *template_ptr;

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

  /* 3. Build the final command string */
  template_ptr = walking_package->function_data.execute.command;

  /* Step 3a: Check for {} in template and replace if present */
  if (strstr(template_ptr, "{}")) {
      if (String_Replace(temp_command_line, sizeof(temp_command_line), template_ptr, "{}", quoted_path) != 0) {
          ERROR_MSG("Command too long");
          return -1;
      }
  } else {
      strncpy(temp_command_line, template_ptr, COMMAND_LINE_LENGTH);
      temp_command_line[COMMAND_LINE_LENGTH] = '\0';
  }

  /* Step 3b: Check for %s in ORIGINAL template and replace in temp buffer if present */
  if (strstr(template_ptr, "%s")) {
      /* Use the result of Step 3a (temp_command_line) as source */
      if (String_Replace(command_line, sizeof(command_line), temp_command_line, "%s", quoted_path) != 0) {
          ERROR_MSG("Command too long");
          return -1;
      }
  } else {
      strncpy(command_line, temp_command_line, COMMAND_LINE_LENGTH);
      command_line[COMMAND_LINE_LENGTH] = '\0';
  }

  return SilentSystemCallEx( command_line, FALSE, s );
}