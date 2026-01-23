/***************************************************************************
 *
 * src/cmd/chgrp.c
 * Change Group
 *
 ***************************************************************************/


#include "ytree.h"


int ChangeFileGroup(FileEntry *fe_ptr)
{
    return HandleFileOwnership(fe_ptr, FALSE, TRUE);
}

static void RedrawGroupPrompt(void) {
    MvAddStr( LINES - 2, 1, "GROUP:" );
}

int GetNewGroup(int st_gid)
{
  char group[GROUP_NAME_MAX * 2 +1];
  char *group_name_ptr;
  int  id;
  int  group_id;
  int  x;

  group_id = -1;

  id = ( st_gid == -1 ) ? (int) getgid() : st_gid;

  group_name_ptr = GetGroupName( id );
  if( group_name_ptr == NULL )
  {
    (void) snprintf( group, sizeof(group), "%d", id );
  }
  else
  {
    (void) strcpy( group, group_name_ptr );
  }

  ClearHelp();

  MvAddStr( LINES - 2, 1, "GROUP:" );
  x = 1 + strlen("GROUP:") + UI_INPUT_PADDING;

  if( InputStringEx( group, LINES - 2, x, 0, GROUP_NAME_MAX, GROUP_NAME_MAX, "\r\033", HST_ID, RedrawGroupPrompt ) == CR )
  {
    if( (group_id = GetGroupId( group )) == -1 )
    {
      (void) snprintf( message, MESSAGE_LENGTH, "Can't read Group-ID:*\"%s\"", group );
      MESSAGE( message );
    }
  }

  move( LINES - 2, 1 ); clrtoeol();

  return( group_id );
}

int SetFileGroup(FileEntry *fe_ptr, WalkingPackage *walking_package)
{
    char buffer[PATH_LENGTH + 1];
    gid_t new_gid = (gid_t)walking_package->function_data.change_group.new_group_id;

    walking_package->new_fe_ptr = fe_ptr; /* Unchanged */

    GetFileNamePath(fe_ptr, buffer);

    return ChangeOwnership(buffer, fe_ptr->stat_struct.st_uid, new_gid, &fe_ptr->stat_struct);
}

int ChangeDirGroup(DirEntry *de_ptr)
{
    return HandleDirOwnership(de_ptr, FALSE, TRUE);
}