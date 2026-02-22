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
typedef int (*ConflictCallback)(const char *src_path, const char *dst_path,
                                int *mode_flags);
typedef int (*ChoiceCallback)(const char *prompt, const char *choices);

/* chgrp.c */
extern int ChangeDirGroup(DirEntry *de_ptr);
extern int ChangeFileGroup(FileEntry *fe_ptr);
extern int SetFileGroup(FileEntry *fe_ptr, WalkingPackage *walking_package);

/* chmod.c */
extern int ChangeDirModus(DirEntry *de_ptr);
extern int ChangeFileModus(FileEntry *fe_ptr);
extern int SetFileModus(FileEntry *fe_ptr, WalkingPackage *walking_package);
extern int SetDirModus(DirEntry *de_ptr, WalkingPackage *walking_package);
extern int GetModus(const char *modus);

/* chown.c */
extern int ChangeDirOwner(DirEntry *de_ptr);
extern int ChangeFileOwner(FileEntry *fe_ptr);
extern int GetNewOwner(int st_uid);
extern int SetFileOwner(FileEntry *fe_ptr, WalkingPackage *walking_package);

/* copy.c */
extern int CopyFile(Statistic *statistic_ptr, FileEntry *fe_ptr, char *to_file,
                    DirEntry *dest_dir_entry, char *to_dir_path, BOOL path_copy,
                    int *dir_create_mode, int *overwrite_mode,
                    ConflictCallback cb, ChoiceCallback choice_cb);
extern int CopyTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package);
extern int GetCopyParameter(char *from_file, BOOL path_copy, char *to_file,
                            char *to_dir);
extern int CopyFileContent(char *to_path, char *from_path, Statistic *s);

/* delete.c */
extern int DeleteFile(FileEntry *fe_ptr, int *auto_override, Statistic *s,
                      ChoiceCallback choice_cb);
extern int DeleteTaggedFiles(FileEntry *fe_ptr,
                             WalkingPackage *walking_package);
extern int RemoveFile(FileEntry *fe_ptr, Statistic *s);

/* execute.c */
extern int Execute(DirEntry *dir_entry, FileEntry *file_entry,
                   char *cmd_template, Statistic *s,
                   ArchiveProgressCallback cb);
extern int ExecuteCommand(FileEntry *fe_ptr, WalkingPackage *walking_package,
                          Statistic *s);

/* filter.c */
extern int ReadFilter(void);
extern int SetFilter(char *filter_spec, Statistic *s);
extern void ApplyFilter(DirEntry *dir_entry, Statistic *s);
extern BOOL Match(FileEntry *fe);

/* log.c */
extern int Log(DirEntry *dir_entry, Statistic *s);
extern int SetLogFile(char *filename);
extern int LogDisk(char *path);
extern int CycleLoadedVolume(int direction);
extern int GetNewLoginPath(char *path);

/* mkdir.c */
extern int MakeDirectory(DirEntry *dir_entry, const char *name, Statistic *s);
extern int EnsureDirectoryExists(char *dir_path, DirEntry *tree, BOOL *created,
                                 DirEntry **result_ptr, int *auto_create,
                                 ChoiceCallback choice_cb);

/* mkfile.c */
extern int MakeFile(DirEntry *dir_entry, const char *name, Statistic *s,
                    int *overwrite_mode, ChoiceCallback choice_cb);

/* move.c */
extern int GetMoveParameter(char *from_file, char *to_folder, char *to_file);
extern int MoveFile(FileEntry *fe_ptr, const char *to_file,
                    DirEntry *dest_dir_entry, const char *to_dir_path,
                    FileEntry **new_fe_ptr, int *dir_create_mode,
                    int *overwrite_mode, ConflictCallback cb,
                    ChoiceCallback choice_cb);
extern int MoveTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package);

/* passwd.c */
extern int Passwd(void);

/* pipe.c */
extern int Pipe(DirEntry *dir_entry, FileEntry *file_entry, char *pipe_command);
extern int PipeDirectory(DirEntry *dir_entry, char *pipe_command);
extern int PipeTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package,
                           Statistic *s);

/* profile.c */
extern void SetProfileValue(char *name, char *value);
extern char *GetProfileValue(const char *name);
extern char *GetUserFileAction(int chkey, int *pchremap);
extern char *GetUserDirAction(int chkey, int *pchremap);
extern BOOL IsUserActionDefined(void);
extern char *GetExtViewer(char *filename);

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
extern int RemoveDirectory(DirEntry *dir_entry, Statistic *s);

/* rename.c */
extern int RenameFile(FileEntry *file_entry, const char *new_name,
                      FileEntry **new_fe_ptr);
extern int RenameDirectory(DirEntry *dir_entry, const char *new_name);
extern int RenameTaggedFiles(FileEntry *fe_ptr,
                             WalkingPackage *walking_package);
extern int GetRenameParameter(const char *old_name, char *new_name);
extern int GetPipeCommand(char *pipe_command);

/* sort.c */
extern void GetKindOfSort(void); /* Obsolete, use UI_GetKindOfSort */
extern void UI_GetKindOfSort(void);
extern void SetKindOfSort(int kind_of_sort, Statistic *s);
extern void UI_SetKindOfSort(int kind_of_sort, Statistic *s);

/* system.c */
extern int QuerySystemCall(char *command_line, Statistic *s);
extern int SilentSystemCall(char *command_line, Statistic *s);
extern int SilentSystemCallEx(char *command_line, BOOL enable_clock,
                              Statistic *s);
extern int SystemCall(char *command_line, Statistic *s);

/* usermode.c */
extern int DirUserMode(DirEntry *dir_entry, int ch, Statistic *s);
extern int FileUserMode(FileEntryList *file_entry_list, int ch, Statistic *s);

/* view.c */
extern int View(DirEntry *dir_entry, char *file_path);
extern int ViewHex(char *file_path);
extern int
ViewTaggedFiles(DirEntry *dir_entry); /* Obsolete, use UI_ViewTaggedFiles */
extern int UI_ViewTaggedFiles(DirEntry *dir_entry);

/* Added for utility decoupling integration */
extern int DeleteDirectory(DirEntry *dir_entry, ChoiceCallback choice_cb);
extern int UI_ChoiceResolver(const char *prompt, const char *choices);
extern int UI_ArchiveCallback(int status, const char *msg, void *user_data);
extern int GetCommandLine(char *command_line);
extern int GetSearchCommandLine(char *command_line, char *raw_pattern);

#endif /* YTREE_CMD_H */
