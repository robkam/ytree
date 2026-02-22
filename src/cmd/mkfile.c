/***************************************************************************
 *
 * src/cmd/mkfile.c
 * Creating empty files (Touch command)
 *
 ***************************************************************************/

#include "ytree_cmd.h"
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>

#define FILE_SEPARATOR_STRING "/"

extern int GetPath(DirEntry *dir_entry, char *buffer);

int MakeFile(DirEntry *dir_entry, const char *file_name, Statistic *s,
             int *overwrite_mode, ChoiceCallback choice_cb) {
  char buffer[PATH_LENGTH + 1];
  int result = -1;
  int fd;
  struct stat stat_struct;

  if (!file_name || !*file_name) {
    return -1;
  }
  /* Check if file already exists in the current view to warn user?
     open with O_EXCL will handle the actual FS existence.
  */

  (void)GetPath(dir_entry, buffer);
  (void)strcat(buffer, FILE_SEPARATOR_STRING);
  (void)strcat(buffer, file_name);

  /* Check existence via stat first to give a nicer error or prompt?
     For now, let's just try to create.
  */

  if (stat(buffer, &stat_struct) == 0) {
    /* File already exists! */
    result = 1;
  } else {
    /* Use a default mask of S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH */
    fd = open(buffer, O_CREAT | O_EXCL | O_WRONLY, 0644);

    if (fd == -1) {
      /* Error creating file */
      result = -1;
    } else {
      close(fd);
      result = 0;
    }
  }

  return result;
}
