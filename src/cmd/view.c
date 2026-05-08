/***************************************************************************
 *
 * src/cmd/view.c
 * View command processing
 *
 ***************************************************************************/

#include "ytree_cmd.h"
#include "ytree_fs.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int ViewFile(ViewContext *ctx, DirEntry *dir_entry, char *file_path);
static int ViewArchiveFile(ViewContext *ctx, char *file_path);

int View(ViewContext *ctx, DirEntry *dir_entry, char *file_path) {
  int mode = ctx->active->vol->vol_stats.log_mode;
  switch (mode) {
  case DISK_MODE:
  case USER_MODE:
    return (ViewFile(ctx, dir_entry, file_path));
  case ARCHIVE_MODE:
    return (ViewArchiveFile(ctx, file_path));
  default:
    return (-1);
  }
}

static int ViewFile(ViewContext *ctx, DirEntry *dir_entry, char *file_path) {
  char command_line[COMMAND_LINE_LENGTH + 1];
  const char *pager;
  int result;

  if (access(file_path, R_OK)) {
    MESSAGE(ctx, "View not possible!*\"%s\"*%s", file_path, strerror(errno));
    return (-1);
  }

  pager = GetProfileValue(ctx, "PAGER");
  if (Path_BuildCommandLine(pager, file_path, NULL, NULL, NULL, NULL,
                            command_line, sizeof(command_line)) != 0) {
    MESSAGE(ctx, "View command too long!*\"%s\"", file_path);
    return (-1);
  }

  result = SystemCall(ctx, command_line, &ctx->active->vol->vol_stats);
  if (dir_entry != NULL)
    RefreshView(ctx, dir_entry);

  return (result);
}

static int ViewArchiveFile(ViewContext *ctx, char *file_path) {
  char temp_filename[PATH_LENGTH];
  char command_line[COMMAND_LINE_LENGTH + 1];
  int fd = -1;
  int result = -1;

  const char *pager;

  temp_filename[0] = '\0';

  if (!Path_CreateTempFile(temp_filename, sizeof(temp_filename), "ytree_view_",
                           FALSE, &fd)) {
    MESSAGE(ctx, "Could not create temporary file for viewing");
    goto cleanup;
  }

  if (ExtractArchiveEntry(ctx->active->vol->vol_stats.log_path, file_path, fd,
                          UI_ArchiveCallback, ctx) != 0) {
    MESSAGE(ctx, "Could not extract entry*'%s'*from archive", file_path);
    goto cleanup;
  }
  close(fd);
  fd = -1;

  pager = GetProfileValue(ctx, "PAGER");
  if (Path_BuildCommandLine(pager, temp_filename, NULL, NULL, NULL, NULL,
                            command_line, sizeof(command_line)) != 0) {
    MESSAGE(ctx, "View command too long!*\"%s\"", file_path);
    goto cleanup;
  }

  result = SystemCall(ctx, command_line, &ctx->active->vol->vol_stats);

cleanup:
  if (fd != -1) {
    close(fd);
  }
  if (temp_filename[0] != '\0') {
    unlink(temp_filename);
  }
  return result;
}

/* ViewTaggedFiles removed from core view.c - moved to UI layer */
