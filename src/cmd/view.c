/***************************************************************************
 *
 * src/cmd/view.c
 * View command processing
 *
 ***************************************************************************/

#include "ytree.h"
#include "ytree_cmd.h"
#include "ytree_defs.h"
#include "ytree_fs.h"

#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int ViewFile(ViewContext *ctx, DirEntry *dir_entry, char *file_path);
static int ViewArchiveFile(ViewContext *ctx, char *file_path);
static int ArchiveUICallback(int status, const char *msg, void *user_data);

int View(ViewContext *ctx, DirEntry *dir_entry, char *file_path) {
  int mode = ctx->active->vol->vol_stats.login_mode;
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
  char file_p_aux[PATH_LENGTH + 1];
  char *pager;
  int result;

  if (access(file_path, R_OK)) {
    MESSAGE(ctx, "View not possible!*\"%s\"*%s", file_path, strerror(errno));
    return (-1);
  }

  strcpy(file_p_aux, file_path);

  pager = GetProfileValue(ctx, "PAGER");
  snprintf(command_line, COMMAND_LINE_LENGTH + 1, "%s %s", pager, file_p_aux);

  result = SilentSystemCall(ctx, command_line, &ctx->active->vol->vol_stats);

  return (result);
}

static int ArchiveUICallback(int status, const char *msg, void *user_data) {
  ViewContext *ctx = (ViewContext *)user_data;
  if (status == ARCHIVE_STATUS_PROGRESS) {
    DrawSpinner(ctx);
    if (EscapeKeyPressed()) {
      return ARCHIVE_CB_ABORT;
    }
  } else if (status == ARCHIVE_STATUS_ERROR) {
    if (msg)
      MESSAGE(ctx, "%s", msg);
  } else if (status == ARCHIVE_STATUS_WARNING) {
    if (msg)
      WARNING(ctx, "%s", msg);
  }
  return ARCHIVE_CB_CONTINUE;
}

static int ViewArchiveFile(ViewContext *ctx, char *file_path) {
  char temp_filename[] = "/tmp/ytree_view_XXXXXX";
  char command_line[COMMAND_LINE_LENGTH + 1];
  int fd;
  int result = -1;

  char *pager;

  fd = mkstemp(temp_filename);
  if (fd == -1) {
    MESSAGE(ctx, "Could not create temporary file for viewing");
    return -1;
  }

  if (ExtractArchiveEntry(ctx->active->vol->vol_stats.login_path, file_path, fd,
                          ArchiveUICallback, ctx) != 0) {
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