/***************************************************************************
 *
 * ytree_cmd.h
 * User commands and action handler prototypes
 *
 ***************************************************************************/
#ifndef YTREE_CMD_H
#define YTREE_CMD_H

#include "ytree_defs.h"

/* Conflict Resolution Codes */
#define CONFLICT_SKIP 0
#define CONFLICT_OVERWRITE 1
#define CONFLICT_ALL 2
#define CONFLICT_ABORT -1

/* Callback Type Definition */
typedef int (*ConflictCallback)(ViewContext *ctx, const char *src_path,
                                const char *dst_path, int *mode_flags);
typedef int (*ChoiceCallback)(ViewContext *ctx, const char *prompt,
                              const char *choices);

/* attributes.c */
extern int ChangeOwnership(const char *path, uid_t new_uid, gid_t new_gid,
                           struct stat *stat_buf);
extern int ChangeDirGroup(DirEntry *de_ptr);
extern int ChangeFileGroup(ViewContext *ctx, FileEntry *fe_ptr);
extern int SetFileGroup(ViewContext *ctx, FileEntry *fe_ptr,
                        WalkingPackage *walking_package);
extern int SetFileModus(ViewContext *ctx, FileEntry *fe_ptr,
                        WalkingPackage *walking_package);
extern int SetDirModus(DirEntry *de_ptr, WalkingPackage *walking_package);
extern int GetMode(const char *modus);
extern int ChangeDirOwner(DirEntry *de_ptr);
extern int ChangeFileOwner(ViewContext *ctx, FileEntry *fe_ptr);
extern int SetFileOwner(ViewContext *ctx, FileEntry *fe_ptr,
                        WalkingPackage *walking_package);

/* copy.c */
extern int CopyFile(ViewContext *ctx, Statistic *statistic_ptr,
                    FileEntry *fe_ptr, char *to_file, DirEntry *dest_dir_entry,
                    char *to_dir_path, BOOL path_copy, int *dir_create_mode,
                    int *overwrite_mode, ConflictCallback cb,
                    ChoiceCallback choice_cb);
extern int CopyTaggedFiles(ViewContext *ctx, FileEntry *fe_ptr,
                           WalkingPackage *walking_package);
extern int GetCopyParameter(ViewContext *ctx, char *from_file, BOOL path_copy,
                            char *to_file, char *to_dir);
extern int CopyFileContent(ViewContext *ctx, char *to_path, char *from_path,
                           Statistic *s);

/* delete.c */
extern int DeleteFile(ViewContext *ctx, FileEntry *fe_ptr, int *auto_override,
                      Statistic *s, ChoiceCallback choice_cb);
extern int DeleteTaggedFiles(ViewContext *ctx, FileEntry *fe_ptr,
                             WalkingPackage *walking_package);
extern int RemoveFile(ViewContext *ctx, FileEntry *fe_ptr, Statistic *s);

/* edit.c */
extern int Edit(ViewContext *ctx, DirEntry *dir_entry, char *file_path);

/* execute.c */
extern int Execute(ViewContext *ctx, DirEntry *dir_entry, FileEntry *file_entry,
                   char *cmd_template, Statistic *s,
                   ArchiveProgressCallback cb);
extern int ExecuteCommand(ViewContext *ctx, FileEntry *fe_ptr,
                          WalkingPackage *walking_package, Statistic *s);

/* filter.c */
extern int UI_ReadFilter(ViewContext *ctx);
extern int SetFilter(char *filter_spec, Statistic *s);
extern void ApplyFilter(DirEntry *dir_entry, Statistic *s);
extern BOOL Match(FileEntry *fe, Statistic *s);

/* log.c */
extern int Log(DirEntry *dir_entry, Statistic *s);
extern int SetLogFile(char *filename);
extern int LogDisk(ViewContext *ctx, YtreePanel *panel, char *path);
extern int CycleLoadedVolume(ViewContext *ctx, YtreePanel *panel,
                             int direction);
extern int GetNewLoginPath(ViewContext *ctx, YtreePanel *panel, char *path);

/* mkdir.c */
extern int MakeDirectory(ViewContext *ctx, YtreePanel *panel,
                         DirEntry *father_dir_entry, const char *dir_name,
                         Statistic *s);
extern int EnsureDirectoryExists(ViewContext *ctx, char *dir_path,
                                 DirEntry *tree, BOOL *created,
                                 DirEntry **result_ptr, int *auto_create,
                                 ChoiceCallback choice_cb);

/* mkfile.c */
extern int MakeFile(ViewContext *ctx, DirEntry *dir_entry, const char *name,
                    Statistic *s, int *overwrite_mode,
                    ChoiceCallback choice_cb);

/* move.c */
extern int GetMoveParameter(ViewContext *ctx, char *from_file, char *to_file,
                            char *to_dir);
extern int MoveFile(ViewContext *ctx, FileEntry *fe_ptr, const char *to_file,
                    DirEntry *dest_dir_entry, const char *to_dir_path,
                    FileEntry **new_fe_ptr, int *dir_create_mode,
                    int *overwrite_mode, ConflictCallback cb,
                    ChoiceCallback choice_cb);
extern int MoveTaggedFiles(ViewContext *ctx, FileEntry *fe_ptr,
                           WalkingPackage *walking_package);

/* passwd.c */
extern int Passwd(void);

/* pipe.c */
extern int Pipe(ViewContext *ctx, DirEntry *dir_entry, FileEntry *file_entry,
                char *pipe_command);
extern int PipeDirectory(ViewContext *ctx, DirEntry *dir_entry,
                         char *pipe_command);
extern int PipeTaggedFiles(ViewContext *ctx, FileEntry *fe_ptr,
                           WalkingPackage *walking_package, Statistic *s);

/* profile.c */
extern void SetProfileValue(ViewContext *ctx, char *name, char *value);
extern char *GetProfileValue(ViewContext *ctx, const char *name);
extern char *GetUserFileAction(ViewContext *ctx, int chkey, int *pchremap);
extern char *GetUserDirAction(ViewContext *ctx, int chkey, int *pchremap);
extern BOOL IsUserActionDefined(ViewContext *ctx);
extern char *GetExtViewer(ViewContext *ctx, char *filename);

/* passwd.c */
extern int ReadPasswdEntries(void);
extern char *GetPasswdName(unsigned int uid);
extern char *GetDisplayPasswdName(unsigned int uid);
extern int GetPasswdUid(char *name);

/* group.c */
extern int ReadGroupEntries(void);
extern char *GetGroupName(unsigned int gid);
extern char *GetDisplayGroupName(unsigned int gid);
extern int GetGroupId(char *name);

/* rmdir.c */
extern int RemoveDirectory(ViewContext *ctx, DirEntry *dir_entry, Statistic *s);

/* rename.c */
extern int RenameFile(ViewContext *ctx, FileEntry *file_entry,
                      const char *new_name, FileEntry **new_fe_ptr);
extern int RenameDirectory(ViewContext *ctx, DirEntry *dir_entry,
                           const char *new_name);
extern int RenameTaggedFiles(ViewContext *ctx, FileEntry *fe_ptr,
                             WalkingPackage *walking_package);
extern int GetRenameParameter(ViewContext *ctx, const char *old_name,
                              char *new_name);
extern int GetPipeCommand(ViewContext *ctx, char *pipe_command);

/* sort.c */
extern void GetKindOfSort(void); /* Obsolete, use UI_HandleSort */
extern void UI_HandleSort(ViewContext *ctx, DirEntry *dir_entry, Statistic *s,
                          int start_x);
extern void UI_SetKindOfSort(int kind_of_sort, Statistic *s);

/* system.c */
extern int QuerySystemCall(ViewContext *ctx, char *command_line, Statistic *s);
extern int SilentSystemCall(ViewContext *ctx, char *command_line, Statistic *s);
extern int SilentSystemCallEx(ViewContext *ctx, char *command_line,
                              BOOL enable_clock, Statistic *s);
extern int SystemCall(ViewContext *ctx, char *command_line, Statistic *s);

/* usermode.c */
extern int DirUserMode(ViewContext *ctx, DirEntry *dir_entry, int ch,
                       Statistic *s);
extern int FileUserMode(ViewContext *ctx, FileEntryList *file_entry_list,
                        int ch, Statistic *s);

/* view.c */
extern int View(ViewContext *ctx, DirEntry *dir_entry, char *file_path);
extern int ViewHex(ViewContext *ctx, char *file_path);
extern int
ViewTaggedFiles(DirEntry *dir_entry); /* Obsolete, use UI_ViewTaggedFiles */
extern int UI_ViewTaggedFiles(ViewContext *ctx, DirEntry *dir_entry);

/* Added for utility decoupling integration */
extern int DeleteDirectory(ViewContext *ctx, DirEntry *dir_entry,
                           ChoiceCallback choice_cb);
extern int UI_ChoiceResolver(ViewContext *ctx, const char *prompt,
                             const char *choices);
extern int UI_ArchiveCallback(int status, const char *msg, void *user_data);
extern int GetCommandLine(ViewContext *ctx, char *command_line);
extern int GetSearchCommandLine(ViewContext *ctx, char *command_line,
                                char *raw_pattern);

#endif /* YTREE_CMD_H */
