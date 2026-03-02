/***************************************************************************
 *
 * src/cmd/mkfile.c
 * Creating empty files (Touch command)
 *
 ***************************************************************************/

#include "ytree.h"
#include "ytree_cmd.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define FILE_SEPARATOR_STRING "/"

int MakeFile(ViewContext *ctx, DirEntry *dir_entry, const char *file_name,
             Statistic *s, int *overwrite_mode, ChoiceCallback choice_cb) {
  char buffer[PATH_LENGTH + 1];
  int result = -1;
  int fd;
  struct stat stat_struct;

  DEBUG_LOG("ENTER MakeFile: file_name=%s", file_name);

  if (!file_name || !*file_name) {
    return -1;
  }
  /* Check if file already exists in the current view to warn user?
     open with O_EXCL will handle the actual FS existence.
  */

  (void)GetPath(dir_entry, buffer);
  (void)strcat(buffer, FILE_SEPARATOR_STRING);
  (void)strcat(buffer, file_name);

  DEBUG_LOG("MakeFile: path=%s", buffer);

  if (stat(buffer, &stat_struct) == 0) {
    DEBUG_LOG("MakeFile: File already exists!");
    result = 1;
  } else {
    fd = open(buffer, O_CREAT | O_EXCL | O_WRONLY, 0644);
    if (fd == -1) {
      DEBUG_LOG("MakeFile: open failed, errno=%d (%s)", errno, strerror(errno));
      result = -1;
    } else {
      DEBUG_LOG("MakeFile: successfully created file");
      close(fd);
      result = 0;
    }
  }

  return result;
}
