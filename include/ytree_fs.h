/***************************************************************************
 *
 * ytree_fs.h
 * Filesystem interaction and archive handling prototypes
 *
 ***************************************************************************/
#ifndef YTREE_FS_H
#define YTREE_FS_H

#include "ytree_defs.h"

/* Define the progress callback type */
typedef void (*ScanProgressCallback)(void *user_data);

/* archive.c */
extern int ExtractArchiveEntry(const char *archive_path, const char *entry_path, int out_fd);
extern int ExtractArchiveNode(const char *archive_path, const char *entry_path, const char *dest_path);
extern int InsertArchiveFileEntry(DirEntry *tree, char *path, struct stat *stat, Statistic *s);
extern int TryInsertArchiveDirEntry(DirEntry *tree, char *dir, struct stat *stat, Statistic *s);
extern void MinimizeArchiveTree(DirEntry **tree_ptr, Statistic *s);
#ifdef HAVE_LIBARCHIVE
extern int Archive_Rewrite(char *archive_path, RewriteCallback cb, void *user_data);
extern int Archive_DeleteEntry(char *archive_path, char *file_path);
extern int Archive_AddFile(char *archive_path, char *src_path, char *dest_name, BOOL is_dir);
extern int Archive_RenameEntry(char *archive_path, char *old_path, char *new_name);
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