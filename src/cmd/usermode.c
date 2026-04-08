/***************************************************************************
 *
 * src/cmd/usermode.c
 * Functions for executing User Defined Commands.
 *
 ***************************************************************************/

#include "ytree_cmd.h"
#include "ytree_fs.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

int DirUserMode(ViewContext *ctx, DirEntry *dir_entry, int ch, Statistic *s) {
  int chremap;
  char *command_str;
  int command_was_run = 0;
  int current_key = ch;

  while (1) {
    command_str = GetUserDirAction(ctx, current_key, &chremap);

    if (command_str != NULL) {
      command_was_run = 1;

      char command_line[COMMAND_LINE_LENGTH + 1];
      char path[PATH_LENGTH + 1];
      char filepath[PATH_LENGTH + 1];
      int start_dir_fd;

      GetPath(dir_entry, filepath);
      if (!Path_BuildUserActionCommand(command_str, filepath, command_line,
                                       sizeof(command_line))) {
        WARNING(ctx, "Invalid command template or command too long.");
        return -1;
      }

      /* Robustly save current working directory using a file descriptor */
      start_dir_fd = open(".", O_RDONLY);
      if (start_dir_fd == -1) {
        MESSAGE(ctx, "Error saving current directory context*%s",
                strerror(errno));
        /* Continue loop processing? Usually we consume key here. */
        return -1;
      }

      if (chdir(GetPath(dir_entry, path))) {
        MESSAGE(ctx, "Can't change directory to*\"%s\"", path);
      } else {
        QuerySystemCall(ctx, command_line, s);

        /* Restore original directory */
        if (fchdir(start_dir_fd) == -1) {
          MESSAGE(ctx, "Error restoring directory*%s", strerror(errno));
        }
      }
      close(start_dir_fd);
    }

    /* Check if the chain ends */
    if (chremap == current_key || chremap <= 0) {
      break;
    } else {
      /* Follow the remap chain */
      current_key = chremap;
    }
  }

  if (command_was_run) {
    return -1; /* Command was executed, consume the keypress. */
  }

  /* No command was run, return the final result of remapping. */
  return chremap;
}

int FileUserMode(ViewContext *ctx, FileEntryList *file_entry_list, int ch,
                 Statistic *s) {
  int chremap;
  char *command_str;
  int command_was_run = 0;
  int current_key = ch;

  while (1) {
    command_str = GetUserFileAction(ctx, current_key, &chremap);

    if (command_str != NULL) {
      command_was_run = 1;

      char command_line[COMMAND_LINE_LENGTH + 1];
      char path[PATH_LENGTH + 1];
      char filepath[PATH_LENGTH + 1];
      int start_dir_fd;
      DirEntry *dir_entry = file_entry_list->file->dir_entry;

      GetRealFileNamePath(file_entry_list->file, filepath, ctx->view_mode);
      if (!Path_BuildUserActionCommand(command_str, filepath, command_line,
                                       sizeof(command_line))) {
        WARNING(ctx, "Invalid command template or command too long.");
        return -1;
      }

      /* Robustly save current working directory using a file descriptor */
      start_dir_fd = open(".", O_RDONLY);
      if (start_dir_fd == -1) {
        MESSAGE(ctx, "Error saving current directory context*%s",
                strerror(errno));
        return -1;
      }

      if (chdir(GetPath(dir_entry, path))) {
        MESSAGE(ctx, "Can't change directory to*\"%s\"", path);
      } else {
        QuerySystemCall(ctx, command_line, s);

        /* Restore original directory */
        if (fchdir(start_dir_fd) == -1) {
          MESSAGE(ctx, "Error restoring directory*%s", strerror(errno));
        }
      }
      close(start_dir_fd);
    }

    /* Check if the chain ends */
    if (chremap == current_key || chremap <= 0) {
      break;
    } else {
      /* Follow the remap chain */
      current_key = chremap;
    }
  }

  if (command_was_run) {
    return -1; /* Command was executed, consume the keypress. */
  }

  /* No command was run, return the final result of remapping. */
  return chremap;
}
