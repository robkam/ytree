/***************************************************************************
*
* ytree_fs.h
* Filesystem interaction and archive handling prototypes
*
***************************************************************************/
#ifndef YTREE_FS_H
#define YTREE_FS_H

#include "ytree_defs.h"

/* Status Codes */
#define ARCHIVE_STATUS_PROGRESS 0
#define ARCHIVE_STATUS_WARNING  1
#define ARCHIVE_STATUS_ERROR    2

/* Callback Return Codes */
#define ARCHIVE_CB_CONTINUE     0
#define ARCHIVE_CB_ABORT       -1

/* Callback Type */
typedef int (*ArchiveProgressCallback)(int status, const char *msg, void *user_data);

/* Define the progress callback type for Scans */
typedef void (*ScanProgressCallback)(void *user_data);

/* archive.c */
extern int ExtractArchiveEntry(const char *archive_path, const char *entry_path, int out_fd, ArchiveProgressCallback cb, void *user_data);
extern int ExtractArchiveNode(const char *archive_path, const char *entry_path, const char *dest_path, ArchiveProgressCallback cb, void *user_data);
extern int InsertArchiveFileEntry(DirEntry *tree, char *path, struct stat *stat, Statistic *s);
extern int TryInsertArchiveDirEntry(DirEntry *tree, char *dir, struct stat *stat, Statistic *s);
extern void MinimizeArchiveTree(DirEntry **tree_ptr, Statistic *s);

#ifdef HAVE_LIBARCHIVE
extern int Archive_Rewrite(char *archive_path, RewriteCallback rw_cb, void *rw_data, ArchiveProgressCallback cb, void *user_data);
extern int Archive_DeleteEntry(char *archive_path, char *file_path, ArchiveProgressCallback cb, void *user_data);
extern int Archive_AddFile(char *archive_path, char *src_path, char *dest_name, BOOL is_dir, ArchiveProgressCallback cb, void *user_data);
extern int Archive_RenameEntry(char *archive_path, char *old_path, char *new_name, ArchiveProgressCallback cb, void *user_data);
#endif

/* readarchive.c */
extern int ReadTreeFromArchive(DirEntry **dir_entry_ptr, const char *filename, Statistic *s, ScanProgressCallback cb, void *cb_data);

/* freesp.c */
extern int GetAvailBytes(LONGLONG *avail_bytes, Statistic *s);
extern int GetDiskParameter(char *path, char *volume_name, LONGLONG *avail_bytes, LONGLONG *capacity, Statistic *s);

/* owner_utils.c */
extern int ChangeOwnership(const char *path, uid_t new_uid, gid_t new_gid, struct stat *stat_buf);
extern int HandleDirOwnership(DirEntry *de_ptr, BOOL change_owner, BOOL change_group);
extern int HandleFileOwnership(FileEntry *fe_ptr, BOOL change_owner, BOOL change_group);

/* readtree.c */
extern int ReadTree(DirEntry *dir_entry, char *path, int depth, Statistic *s, ScanProgressCallback cb, void *cb_data);
extern void UnReadTree(DirEntry *dir_entry, Statistic *s);
extern int RescanDir(DirEntry *dir_entry, int depth, Statistic *s, ScanProgressCallback cb, void *cb_data);

/* tree_utils.c */
extern void DeleteTree(DirEntry *tree);
extern int GetDirEntry(DirEntry *tree, DirEntry *current_dir_entry, char *dir_path, DirEntry **dir_entry, char *to_path);
extern int GetFileEntry(DirEntry *de_ptr, char *file_name, FileEntry **file_entry);
extern void SaveTreeState(DirEntry *root, PathList **expanded, PathList **tagged);
extern void RestoreTreeState(DirEntry *root, PathList **expanded, PathList *tagged, Statistic *s);
extern void FreePathList(PathList *list);

#endif /* YTREE_FS_H */
