/***************************************************************************
 * src/cmd/chgrp.c
 * chgrp Command Implementation
 *
 * Implements the change group functionality for ytree.
 ***************************************************************************/

#include "ytree.h"
#include "ytree_cmd.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>


/* ChangeFileGroup, ChangeDirGroup, and GetNewGroup moved to UI layer */

int SetFileGroup(ViewContext *ctx, FileEntry *fe_ptr, WalkingPackage *walking_package) {
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