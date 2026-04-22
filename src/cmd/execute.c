/***************************************************************************
 *
 * src/cmd/execute.c
 * Executing system commands
 *
 ***************************************************************************/

#include "ytree_cmd.h"
#include "ytree_fs.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define mode (CurrentVolume->vol_stats.log_mode)

#ifdef HAVE_LIBARCHIVE
static int ExecuteArchiveFile(ViewContext *ctx, DirEntry *dir_entry,
                              const FileEntry *file_entry, const char *cmd_template,
                              Statistic *s, ArchiveProgressCallback cb) {
  char temp_path[PATH_LENGTH];
  char command_line[COMMAND_LINE_LENGTH + 1];
  char dir_path[PATH_LENGTH];
  int fd_tmp = -1;
  int result = -1;
  int written;
  BOOL temp_file_created = FALSE;

  temp_path[0] = '\0';

  if (!Path_CreateTempFile(temp_path, sizeof(temp_path), "ytree_execute_",
                           FALSE, &fd_tmp)) {
    return -1;
  }
  temp_file_created = TRUE;

  if (file_entry) {
    char internal_path[PATH_LENGTH];
    char canonical_internal_path[PATH_LENGTH];
    char root_path[PATH_LENGTH + 1];
    char relative_path[PATH_LENGTH + 1];
    const char *relative_source;
    size_t relative_len;

    GetPath(file_entry->dir_entry, dir_path);

    /* Rebuild the path relative to the archive root (log_path) */
    GetPath(s->tree, root_path);

    if (strncmp(dir_path, root_path, strlen(root_path)) == 0) {
      char *ptr = dir_path + strlen(root_path);
      if (*ptr == FILE_SEPARATOR_CHAR)
        ptr++;
      relative_source = ptr;
    } else {
      relative_source = dir_path;
    }

    written = snprintf(relative_path, sizeof(relative_path), "%s", relative_source);
    if (written < 0 || (size_t)written >= sizeof(relative_path)) {
      goto cleanup;
    }
    relative_len = (size_t)written;

    if (relative_len > 0 &&
        relative_path[relative_len - 1] != FILE_SEPARATOR_CHAR) {
      if (relative_len + 1 >= sizeof(relative_path)) {
        goto cleanup;
      }
      relative_path[relative_len++] = FILE_SEPARATOR_CHAR;
      relative_path[relative_len] = '\0';
    }

    written = snprintf(relative_path + relative_len,
                       sizeof(relative_path) - relative_len, "%s",
                       file_entry->name);
    if (written < 0 ||
        (size_t)written >= (sizeof(relative_path) - relative_len)) {
      goto cleanup;
    }

    written = snprintf(internal_path, sizeof(internal_path), "%s", relative_path);
    if (written < 0 || (size_t)written >= sizeof(internal_path)) {
      goto cleanup;
    }

    if (Archive_ValidateInternalPath(internal_path, canonical_internal_path,
                                     sizeof(canonical_internal_path)) != 0) {
      goto cleanup;
    }

    if (ExtractArchiveEntry(s->log_path, canonical_internal_path, fd_tmp, cb,
                            NULL) !=
        0) {
      goto cleanup;
    }
  } else {
    goto cleanup;
  }
  close(fd_tmp);
  fd_tmp = -1;

  if (Path_BuildCommandLine(cmd_template, NULL, "{}", temp_path, NULL, NULL,
                            command_line, sizeof(command_line)) != 0) {
    goto cleanup;
  }

  result = SilentSystemCall(ctx, command_line, s);

cleanup:
  if (fd_tmp != -1) {
    close(fd_tmp);
  }
  if (temp_file_created && temp_path[0] != '\0') {
    unlink(temp_path);
  }
  return result;
}
#endif

int Execute(ViewContext *ctx, DirEntry *dir_entry, const FileEntry *file_entry,
            const char *cmd_template, Statistic *s, ArchiveProgressCallback cb) {
  char command_line[COMMAND_LINE_LENGTH + 1];
  char path[PATH_LENGTH + 1];
  const char *substitution_name;
  int result = -1;
  int start_dir_fd;

/* ARCHIVE MODE HANDLER */
#ifdef HAVE_LIBARCHIVE
  if (ctx->view_mode == ARCHIVE_MODE) {
    result =
        ExecuteArchiveFile(ctx, dir_entry, file_entry, cmd_template, s, cb);
    return result;
  }
#endif

  substitution_name = file_entry ? file_entry->name : ".";
  if (Path_BuildCommandLine(cmd_template, NULL, "{}", substitution_name, NULL,
                            NULL, command_line, sizeof(command_line)) != 0) {
    return -1;
  }

  /* Robustly save current working directory using a file descriptor */
  start_dir_fd = open(".", O_RDONLY);
  if (start_dir_fd == -1) {
    return -1;
  }

  if (ctx->view_mode == DISK_MODE || ctx->view_mode == USER_MODE) {
    if (chdir(GetPath(dir_entry, path))) {
      result = -1;
    } else {
      result = QuerySystemCall(ctx, command_line, s);

      /* Restore original directory */
      if (fchdir(start_dir_fd) == -1) {
        /* Error restoring directory */
      }
    }
  } else {
    /* Execute is disabled in archive mode, but handle defensively */
    result = QuerySystemCall(ctx, command_line, s);
  }

  close(start_dir_fd);
  return result;
}

/* GetCommandLine and GetSearchCommandLine moved to UI layer */

int ExecuteCommand(ViewContext *ctx, FileEntry *fe_ptr,
                   WalkingPackage *walking_package, Statistic *s) {
  char command_line[COMMAND_LINE_LENGTH + 1];
  char raw_path[PATH_LENGTH + 1];
  const char *template_ptr;

  walking_package->new_fe_ptr = fe_ptr;

#ifdef HAVE_LIBARCHIVE
  /* Archive Mode Tagged Execution */
  if (ctx->view_mode == ARCHIVE_MODE) {
    /* Reconstruct the template to pass to single-file handler */
    return ExecuteArchiveFile(ctx, fe_ptr->dir_entry, fe_ptr,
                              walking_package->function_data.execute.command, s,
                              NULL);
  }
#endif

  /* 1. Get the raw filename/path */
  (void)GetFileNamePath(fe_ptr, raw_path);

  template_ptr = walking_package->function_data.execute.command;
  if (Path_BuildCommandLine(template_ptr, NULL, "{}", raw_path, "%s",
                            raw_path, command_line, sizeof(command_line)) !=
      0) {
    return -1;
  }

  return SilentSystemCallEx(ctx, command_line, FALSE, s);
}
