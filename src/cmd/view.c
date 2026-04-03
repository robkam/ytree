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
  {
    int n = snprintf(command_line, COMMAND_LINE_LENGTH + 1, "%s %s", pager,
                     file_path);
    if (n < 0 || n >= COMMAND_LINE_LENGTH + 1) {
      MESSAGE(ctx, "View command too long!*\"%s\"", file_path);
      return (-1);
    }
  }

  result = SilentSystemCall(ctx, command_line, &ctx->active->vol->vol_stats);

  return (result);
}

static int ViewArchiveFile(ViewContext *ctx, char *file_path) {
  char temp_filename[] = "/tmp/ytree_view_XXXXXX";
  char command_line[COMMAND_LINE_LENGTH + 1];
  int fd;
  int result = -1;

  const char *pager;

  fd = mkstemp(temp_filename);
  if (fd == -1) {
    MESSAGE(ctx, "Could not create temporary file for viewing");
    return -1;
  }

  if (ExtractArchiveEntry(ctx->active->vol->vol_stats.log_path, file_path, fd,
                          UI_ArchiveCallback, ctx) != 0) {
    MESSAGE(ctx, "Could not extract entry*'%s'*from archive", file_path);
    close(fd);
    unlink(temp_filename);
    return -1;
  }
  close(fd);

  pager = GetProfileValue(ctx, "PAGER");
  snprintf(command_line, COMMAND_LINE_LENGTH + 1, "%s %s", pager,
           temp_filename);

  result = SilentSystemCall(ctx, command_line, &ctx->active->vol->vol_stats);

  unlink(temp_filename);

  return result;
}

/* ViewTaggedFiles removed from core view.c - moved to UI layer */
