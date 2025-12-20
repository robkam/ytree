/***************************************************************************
 *
 * chown.c
 * Change Owner
 *
 ***************************************************************************/


#include "ytree.h"

int ChangeFileOwner(FileEntry *fe_ptr)
{
    return HandleFileOwnership(fe_ptr, TRUE, FALSE);
}

int GetNewOwner(int st_uid)
{
  char owner[OWNER_NAME_MAX * 2 +1];
  char *owner_name_ptr;
  int  owner_id;
  int  id;

  owner_id = -1;

  id = (st_uid == -1) ? (int) getuid() : st_uid;

  owner_name_ptr = GetPasswdName( id );
  if( owner_name_ptr == NULL )
  {
    (void) snprintf( owner, sizeof(owner), "%d", id );
  }
  else
  {
    (void) strcpy( owner, owner_name_ptr );
  }

  ClearHelp();

  MvAddStr( LINES - 2, 1, "NEW OWNER:" );

  if( InputString( owner, LINES - 2, 12, 0, OWNER_NAME_MAX, "\r\033", HST_ID ) == CR )
  {
    if( (owner_id = GetPasswdUid( owner )) == -1 )
    {
      (void) snprintf( message, MESSAGE_LENGTH, "Can't read Owner-ID:*%s", owner );
      MESSAGE( message );
    }
  }

  move( LINES - 2, 1 ); clrtoeol();

  return( owner_id );
}

int SetFileOwner(FileEntry *fe_ptr, WalkingPackage *walking_package)
{
    char buffer[PATH_LENGTH + 1];
    uid_t new_uid = (uid_t)walking_package->function_data.change_owner.new_owner_id;

    walking_package->new_fe_ptr = fe_ptr; /* Unchanged */

    GetFileNamePath(fe_ptr, buffer);

    return ChangeOwnership(buffer, new_uid, fe_ptr->stat_struct.st_gid, &fe_ptr->stat_struct);
}

int ChangeDirOwner(DirEntry *de_ptr)
{
    return HandleDirOwnership(de_ptr, TRUE, FALSE);
}