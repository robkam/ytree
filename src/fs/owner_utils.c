/***************************************************************************
 * src/fs/owner_utils.c
 * Filesystem Ownership Utilities
 *
 * Contains low-level helpers for changing file and directory ownership.
 ***************************************************************************/

#include "ytree_cmd.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(S_IFLNK)
#define STAT_(a, b) lstat(a, b)
#else
#define STAT_(a, b) stat(a, b)
#endif

extern char *GetPath(DirEntry *dir_entry, char *buffer);
extern char *GetFileNamePath(FileEntry *file_entry, char *buffer);

/* ChangeFileOrDirOwnership, HandleDirOwnership and HandleFileOwnership moved to
 * UI layer */

/*
 * Central helper to change ownership, re-stat the file, and handle errors.
 * Returns 0 on success, -1 on failure.
 */
int ChangeOwnership(const char *path, uid_t new_uid, gid_t new_gid,
                    struct stat *stat_buf) {
  struct stat new_stat;

  if (chown(path, new_uid, new_gid) != 0) {
    return -1;
  }

  if (STAT_(path, &new_stat) != 0) {
    return -1;
  }

  /* Update the caller's stat buffer */
  *stat_buf = new_stat;
  return 0;
}