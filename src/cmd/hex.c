/***************************************************************************
 *
 * src/cmd/hex.c
 * View hex command processing
 *
 ***************************************************************************/

#include "ytree_cmd.h"
#include "ytree_defs.h"
#include "ytree_fs.h"

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define mode (CurrentVolume->vol_stats.login_mode)
extern struct Volume *CurrentVolume;

extern void DrawSpinner(void);
extern int EscapeKeyPressed(void);
extern int InternalView(char *file_path);

static int ViewHexFile(char *file_path);
static int ViewHexArchiveFile(char *file_path);

int ViewHex(char *file_path) {
  switch (mode) {
  case DISK_MODE:
  case USER_MODE:
    return (ViewHexFile(file_path));
  case ARCHIVE_MODE:
    return (ViewHexArchiveFile(file_path));
  default:
    return (-1);
  }
}

static int ViewHexFile(char *file_path) {
  if (access(file_path, R_OK)) {
    MESSAGE("HexView not possible!*\"%s\"*%s", file_path, strerror(errno));
    return -1;
  }

  InternalView(file_path);
  return 0;
}

static int HexProgressCallback(int status, const char *msg, void *user_data) {
  (void)msg;
  (void)user_data;

  if (status == ARCHIVE_STATUS_PROGRESS) {
    DrawSpinner();
    if (EscapeKeyPressed()) {
      return ARCHIVE_CB_ABORT;
    }
  }
  return ARCHIVE_CB_CONTINUE;
}

static int ViewHexArchiveFile(char *file_path) {
  char temp_filename[] = "/tmp/ytree_hex_XXXXXX";
  int fd;
  int result = -1;

  fd = mkstemp(temp_filename);
  if (fd == -1) {
    ERROR_MSG("Could not create temporary file for hex view");
    return -1;
  }

  if (ExtractArchiveEntry(CurrentVolume->vol_stats.login_path, file_path, fd,
                          HexProgressCallback, NULL) != 0) {
    MESSAGE("Could not extract entry*'%s'*from archive", file_path);
    close(fd);
    unlink(temp_filename);
    return -1;
  }
  close(fd);

  InternalView(temp_filename);

  result = 0;
  unlink(temp_filename);

  return result;
}