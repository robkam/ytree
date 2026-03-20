/***************************************************************************
 * src/cmd/attributes.c
 * Attributes Command Implementation
 *
 * Implements the change group, owner, and mode functionality for ytree.
 ***************************************************************************/

#include "ytree.h"
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

#ifndef PATH_LENGTH
#define PATH_LENGTH 4096
#endif

static int GetNewMode(int old_mode, char *new_mode);

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

int SetFileGroup(ViewContext *ctx, FileEntry *fe_ptr,
                 WalkingPackage *walking_package) {
  char buffer[PATH_LENGTH + 1];
  gid_t new_gid =
      (gid_t)walking_package->function_data.change_group.new_group_id;

  walking_package->new_fe_ptr = fe_ptr; /* Unchanged */

  GetFileNamePath(fe_ptr, buffer);

  return ChangeOwnership(buffer, fe_ptr->stat_struct.st_uid, new_gid,
                         &fe_ptr->stat_struct);
}

int ChangeDirGroup(DirEntry *de_ptr) {
  /* Refactored to call UI-based handler or use direct ChangeOwnership if needed
   */
  return -1;
}

int SetFileOwner(ViewContext *ctx, FileEntry *fe_ptr,
                 WalkingPackage *walking_package) {
  char buffer[PATH_LENGTH + 1];
  uid_t new_uid =
      (uid_t)walking_package->function_data.change_owner.new_owner_id;

  walking_package->new_fe_ptr = fe_ptr; /* Unchanged */

  GetFileNamePath(fe_ptr, buffer);

  return ChangeOwnership(buffer, new_uid, fe_ptr->stat_struct.st_gid,
                         &fe_ptr->stat_struct);
}

int ChangeDirOwner(DirEntry *de_ptr) {
  /* Refactored to call UI-based handler or use direct ChangeOwnership if needed
   */
  return -1;
}

int SetFileModus(ViewContext *ctx, FileEntry *fe_ptr,
                 WalkingPackage *walking_package) {
  struct stat stat_struct;
  char buffer[PATH_LENGTH + 1];
  int result;
  int new_mode;

  result = -1;

  walking_package->new_fe_ptr = fe_ptr; /* unchanged */

  new_mode =
      GetNewMode(fe_ptr->stat_struct.st_mode,
                  walking_package->function_data.change_mode.new_mode);

  new_mode = new_mode | (fe_ptr->stat_struct.st_mode &
                           ~(S_IRWXO | S_IRWXG | S_IRWXU | S_ISGID | S_ISUID));

  if (!chmod(GetFileNamePath(fe_ptr, buffer), new_mode)) {
    /* Successful modification */
    /*-------------------------*/

    if (STAT_(buffer, &stat_struct)) {
      return -1;
    } else {
      fe_ptr->stat_struct = stat_struct;
    }

    result = 0;
  } else {
    return -1;
  }

  return (result);
}

int SetDirModus(DirEntry *de_ptr, WalkingPackage *walking_package) {
  struct stat stat_struct;
  char buffer[PATH_LENGTH + 1];
  int result;
  int new_mode;

  result = -1;

  new_mode =
      GetNewMode(de_ptr->stat_struct.st_mode,
                  walking_package->function_data.change_mode.new_mode);

  new_mode = new_mode | (de_ptr->stat_struct.st_mode &
                           ~(S_IRWXO | S_IRWXG | S_IRWXU | S_ISGID | S_ISUID));

  if (!chmod(GetPath(de_ptr, buffer), new_mode)) {
    /* Successful modification */
    /*-------------------------*/

    if (STAT_(buffer, &stat_struct)) {
      return -1;
    } else {
      de_ptr->stat_struct = stat_struct;
    }

    result = 0;
  } else {
    return -1;
  }

  return (result);
}

static int GetNewMode(int old_mode, char *mode) {
  int new_mode;

  new_mode = 0;

  if (*mode == '-')
    new_mode |= S_IFREG;
  if (*mode == 'd')
    new_mode |= S_IFDIR;
#ifdef S_IFLNK
  if (*mode == 'l')
    new_mode |= S_IFLNK;
#endif /* S_IFLNK */
  if (*mode == '?')
    new_mode |= old_mode & S_IFMT;
  mode++;

  if (*mode == 'r')
    new_mode |= S_IRUSR;
  if (*mode++ == '?')
    new_mode |= old_mode & S_IRUSR;
  if (*mode == 'w')
    new_mode |= S_IWUSR;
  if (*mode++ == '?')
    new_mode |= old_mode & S_IWUSR;
  if (*mode == 'x')
    new_mode |= S_IXUSR;
  if (*mode == 's')
    new_mode |= S_ISUID | S_IXUSR;
  if (*mode++ == '?')
    new_mode |= old_mode & (S_ISUID | S_IXUSR);

  if (*mode == 'r')
    new_mode |= S_IRGRP;
  if (*mode++ == '?')
    new_mode |= old_mode & S_IRGRP;
  if (*mode == 'w')
    new_mode |= S_IWGRP;
  if (*mode++ == '?')
    new_mode |= old_mode & S_IWGRP;
  if (*mode == 'x')
    new_mode |= S_IXGRP;
  if (*mode == 's')
    new_mode |= S_ISGID | S_IXGRP;
  if (*mode++ == '?')
    new_mode |= old_mode & (S_ISGID | S_IXGRP);

  if (*mode == 'r')
    new_mode |= S_IROTH;
  if (*mode++ == '?')
    new_mode |= old_mode & S_IROTH;
  if (*mode == 'w')
    new_mode |= S_IWOTH;
  if (*mode++ == '?')
    new_mode |= old_mode & S_IWOTH;
  if (*mode == 'x')
    new_mode |= S_IXOTH;
  if (*mode++ == '?')
    new_mode |= old_mode & S_IXOTH;

  return (new_mode);
}
