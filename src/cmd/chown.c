/***************************************************************************
 *
 * src/cmd/chown.c
 * Change Owner
 *
 ***************************************************************************/

#include "ytree_cmd.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

extern char *GetPasswdName(unsigned int uid);
extern int GetPasswdUid(char *name);
extern char *GetFileNamePath(FileEntry *file_entry, char *buffer);
extern int ChangeOwnership(const char *path, uid_t new_uid, gid_t new_gid,
                           struct stat *stat_buf);

/* ChangeFileOwner, ChangeDirOwner, and GetNewOwner moved to UI layer */
/* ChangeFileGroup, ChangeDirGroup, and GetNewGroup moved to UI layer */

int SetFileOwner(FileEntry *fe_ptr, WalkingPackage *walking_package) {
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