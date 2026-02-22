/***************************************************************************
 *
 * src/cmd/view.c
 * View command processing
 *
 ***************************************************************************/

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

extern char *GetProfileValue(const char *);
#define PAGER GetProfileValue("PAGER")

extern struct Volume *CurrentVolume;
#define mode (CurrentVolume->vol_stats.login_mode)

extern void DrawSpinner(void);
extern int EscapeKeyPressed(void);
extern int SilentSystemCall(char *command_line, Statistic *s);

static int ViewFile(DirEntry *dir_entry, char *file_path);
static int ViewArchiveFile(char *file_path);
static int ArchiveUICallback(int status, const char *msg, void *user_data);

int View(DirEntry *dir_entry, char *file_path) {
  switch (mode) {
  case DISK_MODE:
  case USER_MODE:
    return (ViewFile(dir_entry, file_path));
  case ARCHIVE_MODE:
    return (ViewArchiveFile(file_path));
  default:
    return (-1);
  }
}

static int ViewFile(DirEntry *dir_entry, char *file_path) {
  char command_line[COMMAND_LINE_LENGTH + 1];
  char file_p_aux[PATH_LENGTH + 1];
  int result;

  if (access(file_path, R_OK)) {
    MESSAGE("View not possible!*\"%s\"*%s", file_path, strerror(errno));
    return (-1);
  }

  strcpy(file_p_aux, file_path);

  snprintf(command_line, COMMAND_LINE_LENGTH + 1, "%s %s", PAGER, file_p_aux);

  result = SilentSystemCall(command_line, &CurrentVolume->vol_stats);

  return (result);
}

static int ArchiveUICallback(int status, const char *msg, void *user_data) {
  (void)user_data;
  if (status == ARCHIVE_STATUS_PROGRESS) {
    DrawSpinner();
    if (EscapeKeyPressed()) {
      return ARCHIVE_CB_ABORT;
    }
  } else if (status == ARCHIVE_STATUS_ERROR) {
    if (msg)
      ERROR_MSG("%s", msg);
  } else if (status == ARCHIVE_STATUS_WARNING) {
    if (msg)
      WARNING("%s", msg);
  }
  return ARCHIVE_CB_CONTINUE;
}

static int ViewArchiveFile(char *file_path) {
  char temp_filename[] = "/tmp/ytree_view_XXXXXX";
  char command_line[COMMAND_LINE_LENGTH + 1];
  int fd;
  int result = -1;

  fd = mkstemp(temp_filename);
  if (fd == -1) {
    ERROR_MSG("Could not create temporary file for viewing");
    return -1;
  }

  if (ExtractArchiveEntry(CurrentVolume->vol_stats.login_path, file_path, fd,
                          ArchiveUICallback, NULL) != 0) {
    MESSAGE("Could not extract entry*'%s'*from archive", file_path);
    close(fd);
    unlink(temp_filename);
    return -1;
  }
  close(fd);

  snprintf(command_line, COMMAND_LINE_LENGTH + 1, "%s %s", PAGER,
           temp_filename);

  result = SilentSystemCall(command_line, &CurrentVolume->vol_stats);

  unlink(temp_filename);

  return result;
}

/* ViewTaggedFiles removed from core view.c - moved to UI layer */