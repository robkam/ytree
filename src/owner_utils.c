/***************************************************************************
 *
 * owner_utils.c
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

/*
 * Central helper to change ownership, re-stat the file, and handle errors.
 * Returns 0 on success, -1 on failure.
 */
int ChangeOwnership(const char *path, uid_t new_uid, gid_t new_gid, struct stat *stat_buf)
{
    struct stat new_stat;

    if (chown(path, new_uid, new_gid) != 0)
    {
        (void)snprintf(message, MESSAGE_LENGTH, "Cannot change ownership:*%s", strerror(errno));
        MESSAGE(message);
        return -1;
    }

    if (STAT_(path, &new_stat) != 0)
    {
        ERROR_MSG("Re-stat failed after chown");
        return -1;
    }

    /* Update the caller's stat buffer */
    *stat_buf = new_stat;
    return 0;
}