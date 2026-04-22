/***************************************************************************
 *
 * src/cmd/hex.c
 * View hex command processing
 *
 ***************************************************************************/

#include "ytree_cmd.h"
#include "ytree_fs.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int ViewHexFile(ViewContext *ctx, char *file_path);
static int ViewHexArchiveFile(ViewContext *ctx, char *file_path);
static ViewerGeometry BuildViewerGeometry(const ViewContext *ctx);

static ViewerGeometry BuildViewerGeometry(const ViewContext *ctx) {
  ViewerGeometry geom;

  geom.start_y = ctx->layout.dir_win_y;
  geom.start_x = ctx->layout.dir_win_x;
  geom.height = ctx->layout.message_y - ctx->layout.dir_win_y;
  geom.width = ctx->layout.main_win_width;
  geom.header_y = ctx->layout.header_y;
  geom.message_y = ctx->layout.message_y;
  geom.prompt_y = ctx->layout.prompt_y;
  geom.status_y = ctx->layout.status_y;
  return geom;
}

int ViewHex(ViewContext *ctx, char *file_path) {
  int mode = ctx->active->vol->vol_stats.log_mode;
  switch (mode) {
  case DISK_MODE:
  case USER_MODE:
    return (ViewHexFile(ctx, file_path));
  case ARCHIVE_MODE:
    return (ViewHexArchiveFile(ctx, file_path));
  default:
    return (-1);
  }
}

static int ViewHexFile(ViewContext *ctx, char *file_path) {
  ViewerGeometry geom;

  if (access(file_path, R_OK)) {
    MESSAGE(ctx, "HexView not possible!*\"%s\"*%s", file_path, strerror(errno));
    return -1;
  }

  geom = BuildViewerGeometry(ctx);
  InternalView(ctx, file_path, &geom);
  return 0;
}

static int ViewHexArchiveFile(ViewContext *ctx, char *file_path) {
  char temp_filename[PATH_LENGTH];
  int fd = -1;
  int result = -1;
  ViewerGeometry geom;

  temp_filename[0] = '\0';

  if (!Path_CreateTempFile(temp_filename, sizeof(temp_filename), "ytree_hex_",
                           FALSE, &fd)) {
    UI_Error(ctx, __FILE__, __LINE__,
             "Could not create temporary file for hex view");
    goto cleanup;
  }

  if (ExtractArchiveEntry(ctx->active->vol->vol_stats.log_path, file_path, fd,
                          UI_ArchiveCallback, ctx) != 0) {
    MESSAGE(ctx, "Could not extract entry*'%s'*from archive", file_path);
    goto cleanup;
  }
  close(fd);
  fd = -1;

  geom = BuildViewerGeometry(ctx);
  InternalView(ctx, temp_filename, &geom);

  result = 0;

cleanup:
  if (fd != -1) {
    close(fd);
  }
  if (temp_filename[0] != '\0') {
    unlink(temp_filename);
  }
  return result;
}
