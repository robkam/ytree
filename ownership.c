/***************************************************************************
 *
 * Centralized ownership and group modification functions
 *
 ***************************************************************************/

#include "ytree.h"

/* Internal function to handle the core logic of changing ownership. */
static int ChangeFileOrDirOwnership(const char *path,
                                    struct stat *stat_buf,
                                    BOOL change_owner,
                                    BOOL change_group)
{
    uid_t new_uid = stat_buf->st_uid;
    gid_t new_gid = stat_buf->st_gid;
    int result = -1;

    if (change_owner)
    {
        int owner_id = GetNewOwner(stat_buf->st_uid);
        if (owner_id < 0) return -1; /* User cancelled or error */
        new_uid = (uid_t)owner_id;
    }

    if (change_group)
    {
        int group_id = GetNewGroup(stat_buf->st_gid);
        if (group_id < 0) return -1; /* User cancelled or error */
        new_gid = (gid_t)group_id;
    }

    /* Use the generic helper to perform the system call and update stat */
    result = ChangeOwnership(path, new_uid, new_gid, stat_buf);

    return result;
}

/* Public function for handling DirEntry ownership changes. */
int HandleDirOwnership(DirEntry *de_ptr, BOOL change_owner, BOOL change_group)
{
    char path[PATH_LENGTH + 1];

    if (mode != DISK_MODE && mode != USER_MODE)
    {
        return -1;
    }
    
    GetPath(de_ptr, path);
    return ChangeFileOrDirOwnership(path, &de_ptr->stat_struct, change_owner, change_group);
}

/* Public function for handling FileEntry ownership changes. */
int HandleFileOwnership(FileEntry *fe_ptr, BOOL change_owner, BOOL change_group)
{
    char path[PATH_LENGTH + 1];

    if (mode != DISK_MODE && mode != USER_MODE)
    {
        return -1;
    }
    
    GetFileNamePath(fe_ptr, path);
    return ChangeFileOrDirOwnership(path, &fe_ptr->stat_struct, change_owner, change_group);
}