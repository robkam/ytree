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

static int GetNewMode(int old_modus, char *new_modus);

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
  int new_modus;

  result = -1;

  walking_package->new_fe_ptr = fe_ptr; /* unchanged */

  new_modus =
      GetNewMode(fe_ptr->stat_struct.st_mode,
                  walking_package->function_data.change_mode.new_mode);

  new_modus = new_modus | (fe_ptr->stat_struct.st_mode &
                           ~(S_IRWXO | S_IRWXG | S_IRWXU | S_ISGID | S_ISUID));

  if (!chmod(GetFileNamePath(fe_ptr, buffer), new_modus)) {
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
  int new_modus;

  result = -1;

  new_modus =
      GetNewMode(de_ptr->stat_struct.st_mode,
                  walking_package->function_data.change_mode.new_mode);

  new_modus = new_modus | (de_ptr->stat_struct.st_mode &
                           ~(S_IRWXO | S_IRWXG | S_IRWXU | S_ISGID | S_ISUID));

  if (!chmod(GetPath(de_ptr, buffer), new_modus)) {
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

static int GetNewMode(int old_modus, char *modus) {
  int new_modus;

  new_modus = 0;

  if (*modus == '-')
    new_modus |= S_IFREG;
  if (*modus == 'd')
    new_modus |= S_IFDIR;
#ifdef S_IFLNK
  if (*modus == 'l')
    new_modus |= S_IFLNK;
#endif /* S_IFLNK */
  if (*modus == '?')
    new_modus |= old_modus & S_IFMT;
  modus++;

  if (*modus == 'r')
    new_modus |= S_IRUSR;
  if (*modus++ == '?')
    new_modus |= old_modus & S_IRUSR;
  if (*modus == 'w')
    new_modus |= S_IWUSR;
  if (*modus++ == '?')
    new_modus |= old_modus & S_IWUSR;
  if (*modus == 'x')
    new_modus |= S_IXUSR;
  if (*modus == 's')
    new_modus |= S_ISUID | S_IXUSR;
  if (*modus++ == '?')
    new_modus |= old_modus & (S_ISUID | S_IXUSR);

  if (*modus == 'r')
    new_modus |= S_IRGRP;
  if (*modus++ == '?')
    new_modus |= old_modus & S_IRGRP;
  if (*modus == 'w')
    new_modus |= S_IWGRP;
  if (*modus++ == '?')
    new_modus |= old_modus & S_IWGRP;
  if (*modus == 'x')
    new_modus |= S_IXGRP;
  if (*modus == 's')
    new_modus |= S_ISGID | S_IXGRP;
  if (*modus++ == '?')
    new_modus |= old_modus & (S_ISGID | S_IXGRP);

  if (*modus == 'r')
    new_modus |= S_IROTH;
  if (*modus++ == '?')
    new_modus |= old_modus & S_IROTH;
  if (*modus == 'w')
    new_modus |= S_IWOTH;
  if (*modus++ == '?')
    new_modus |= old_modus & S_IWOTH;
  if (*modus == 'x')
    new_modus |= S_IXOTH;
  if (*modus++ == '?')
    new_modus |= old_modus & S_IXOTH;

  return (new_modus);
}

int GetModus(const char *modus) { return (GetNewMode(0, (char *)modus)); }
