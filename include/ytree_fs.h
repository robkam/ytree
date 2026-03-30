/***************************************************************************
 *
 * ytree_fs.h
 * Filesystem interaction and archive handling prototypes
 *
 ***************************************************************************/
#ifndef YTREE_FS_H
#define YTREE_FS_H

#include "ytree_defs.h"

/* ARCHIVE_STATUS_* and ARCHIVE_CB_* moved to ytree_defs.h */
#define UNSUPPORTED_FORMAT_ERROR -2

/* Define the progress callback type for Scans */
typedef void (*ScanProgressCallback)(ViewContext *ctx, void *user_data);

/* path_utils.c */
extern char *GetFileNamePath(FileEntry *file_entry, char *buffer);
extern char *GetPath(DirEntry *dir_entry, char *buffer);
extern void Fnsplit(char *path, char *dir, char *name);
extern void NormPath(char *in_path, char *out_path);

/* archive.c */
extern int ExtractArchiveEntry(const char *archive_path, const char *entry_path,
                               int out_fd, ArchiveProgressCallback cb,
                               void *user_data);
extern int ExtractArchiveNode(const char *archive_path, const char *entry_path,
                              const char *dest_path, ArchiveProgressCallback cb,
                              void *user_data);
extern int InsertArchiveFileEntry(ViewContext *ctx, DirEntry *tree, char *path,
                                  const struct stat *stat, Statistic *s);
extern int TryInsertArchiveDirEntry(ViewContext *ctx, DirEntry *tree, const char *dir,
                                    const struct stat *stat, Statistic *s);
extern void MinimizeArchiveTree(DirEntry **tree_ptr, Statistic *s);

#ifdef HAVE_LIBARCHIVE
extern int Archive_Rewrite(char *archive_path, RewriteCallback rw_cb,
                           void *rw_data, ArchiveProgressCallback cb,
                           void *user_data);
extern int Archive_CreateFromPaths(const char *dest_path,
                                   const char *const *source_paths,
                                   const char *const *archive_paths,
                                   size_t source_count);
extern int Archive_DeleteEntry(char *archive_path, char *file_path,
                               ArchiveProgressCallback cb, void *user_data);
extern int Archive_AddFile(char *archive_path, char *src_path, char *dest_name,
                           BOOL is_dir, ArchiveProgressCallback cb,
                           void *user_data);
extern int Archive_RenameEntry(char *archive_path, char *old_path,
                               char *new_name, ArchiveProgressCallback cb,
                               void *user_data);
#endif

/* readarchive.c */
extern int ReadTreeFromArchive(ViewContext *ctx, DirEntry **dir_entry_ptr,
                               const char *filename, Statistic *s,
                               ScanProgressCallback cb, void *cb_data);

/* freesp.c */
extern int GetAvailBytes(long long *avail_bytes, Statistic *s);
extern int GetDiskParameter(char *path, char *volume_name,
                            long long *avail_bytes, long long *capacity,
                            Statistic *s);

/* readtree.c */
extern int ReadTree(ViewContext *ctx, DirEntry *dir_entry, char *path,
                    int depth, Statistic *s, ScanProgressCallback cb,
                    void *cb_data);
extern void UnReadTree(ViewContext *ctx, DirEntry *dir_entry, Statistic *s);
extern int RescanDir(ViewContext *ctx, DirEntry *dir_entry, int depth,
                     Statistic *s, ScanProgressCallback cb, void *cb_data);

/* path_utils.c */
extern char *GetPath(DirEntry *dir_entry, char *buffer);
extern void Fnsplit(char *path, char *dir, char *name);

/* tree_utils.c */
extern void DeleteTree(DirEntry *tree);
extern int GetDirEntry(const ViewContext *ctx, DirEntry *tree,
                       DirEntry *current_dir_entry, const char *dir_path,
                       DirEntry **dir_entry, char *to_path);
extern int GetFileEntry(DirEntry *de_ptr, const char *file_name,
                        FileEntry **file_entry);
extern void SaveTreeState(DirEntry *root, PathList **expanded,
                          PathList **tagged);
extern void RestoreTreeState(ViewContext *ctx, DirEntry *root,
                             PathList **expanded, PathList *tagged,
                             Statistic *s);
void ApplyFilterToTree(DirEntry *dir_entry, Statistic *s);
extern void FreePathList(PathList *list);

#endif /* YTREE_FS_H */
