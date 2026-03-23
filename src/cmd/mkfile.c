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
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define FILE_SEPARATOR_STRING "/"

int MakeFile(ViewContext *ctx, DirEntry *dir_entry, const char *name,
             Statistic *s, int *overwrite_mode, ChoiceCallback choice_cb) {
  char parent_path[PATH_LENGTH + 1];
  char buffer[PATH_LENGTH + 1];
  int result = -1;
  struct stat stat_struct;

  DEBUG_LOG("ENTER MakeFile: file_name=%s", name);

  if (!name || !*name) {
    return -1;
  }
  /* Check if file already exists in the current view to warn user?
     open with O_EXCL will handle the actual FS existence.
  */

  (void)GetPath(dir_entry, parent_path);
  {
    int n = snprintf(buffer, sizeof(buffer), "%s%s%s", parent_path,
                     FILE_SEPARATOR_STRING, name);
    if (n < 0 || n >= (int)sizeof(buffer)) {
      DEBUG_LOG("MakeFile: path too long, parent=%s file_name=%s", parent_path,
                name);
      return -1;
    }
  }

  DEBUG_LOG("MakeFile: path=%s", buffer);

  if (stat(buffer, &stat_struct) == 0) {
    DEBUG_LOG("MakeFile: File already exists!");
    result = 1;
  } else {
    int fd;
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
