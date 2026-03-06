/***************************************************************************
 *
 * src/ui/edit.c
 * Edit command processing
 *
 ***************************************************************************/

#include "ytree.h"

int Edit(ViewContext *ctx, DirEntry *dir_entry, char *file_path) {
  char *command_line;
  int result = -1;
  char *file_p_aux = NULL;
  char path[PATH_LENGTH + 1];
  int start_dir_fd;

  if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
    return (-1);
  }

  if (access(file_path, R_OK)) {
    UI_Message(ctx, "Edit not possible!*\"%s\"*%s", file_path, strerror(errno));
    ESCAPE;
  }

  if ((file_p_aux = (char *)malloc(COMMAND_LINE_LENGTH)) == NULL) {
    UI_Message(ctx, "Malloc failed*ABORT");
    exit(1);
  }
  StrCp(file_p_aux, file_path);

  if ((command_line = (char *)malloc(COMMAND_LINE_LENGTH)) == NULL) {
    UI_Message(ctx, "Malloc failed*ABORT");
    exit(1);
  }
  /*
   * Replaced unsafe strcpy/strcat construction with snprintf
   * to ensure bounds checking and secure string handling.
   */
  (void)snprintf(command_line, COMMAND_LINE_LENGTH, "%s \"%s\"",
                 (GetProfileValue)(ctx, "EDITOR"), file_p_aux);

  free(file_p_aux);

  /*  result = SystemCall(command_line);
    --crb3 29apr02: perhaps finally eliminate the problem with jstar writing new
    files to the ytree starting cwd. new code grabbed from execute.c.
    --crb3 01oct02: move getcwd operation within the IF DISKMODE stuff.
  */

  if (ctx->view_mode == DISK_MODE || ctx->view_mode == USER_MODE) {
    /* Robustly save current working directory using a file descriptor */
    start_dir_fd = open(".", O_RDONLY);
    if (start_dir_fd == -1) {
      UI_Message(ctx, "Error saving current directory context*%s",
                 strerror(errno));
      free(command_line);
      return -1;
    }

    if (chdir(GetPath(dir_entry, path))) {
      UI_Message(ctx, "Can't change directory to*\"%s\"", path);
    } else {
      result = SystemCall(ctx, command_line, &ctx->active->vol->vol_stats);

      /* Restore original directory */
      if (fchdir(start_dir_fd) == -1) {
        UI_Message(ctx, "Error restoring directory*%s", strerror(errno));
      }
    }
    close(start_dir_fd);
  } else {
    result = SystemCall(ctx, command_line, &ctx->active->vol->vol_stats);
  }
  free(command_line);

FNC_XIT:

  return (result);
}