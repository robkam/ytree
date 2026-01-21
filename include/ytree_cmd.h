/***************************************************************************
 *
 * ytree_cmd.h
 * User commands and action handler prototypes
 *
 ***************************************************************************/
#ifndef YTREE_CMD_H
#define YTREE_CMD_H

#include "ytree_defs.h"

/* chgrp.c */
extern int ChangeDirGroup(DirEntry *de_ptr);
extern int ChangeFileGroup(FileEntry *fe_ptr);
extern int GetNewGroup(int st_gid);
extern int SetFileGroup(FileEntry *fe_ptr, WalkingPackage *walking_package);

/* chmod.c */
extern int ChangeDirModus(DirEntry *de_ptr);
extern int ChangeFileModus(FileEntry *fe_ptr);
extern int GetModus(char *modus);
extern int GetNewFileModus(int y, int x, char *modus, char *term);
extern int SetFileModus(FileEntry *fe_ptr, WalkingPackage *walking_package);

/* chown.c */
extern int ChangeDirOwner(DirEntry *de_ptr);
extern int ChangeFileOwner(FileEntry *fe_ptr);
extern int GetNewOwner(int st_uid);
extern int SetFileOwner(FileEntry *fe_ptr, WalkingPackage *walking_package);

/* copy.c */
extern int CopyFile(Statistic *statistic_ptr, FileEntry *fe_ptr, unsigned char confirm, char *to_file, DirEntry *dest_dir_entry, char *to_dir_path, BOOL path_copy, int *dir_create_mode, int *overwrite_mode);
extern int CopyTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package);
extern int GetCopyParameter(char *from_file, BOOL path_copy, char *to_file, char *to_dir);
extern int CopyFileContent(char *to_path, char *from_path, Statistic *s);

/* delete.c */
extern int DeleteFile(FileEntry *fe_ptr, int *auto_override, Statistic *s);
extern int RemoveFile(FileEntry *fe_ptr, Statistic *s);

/* execute.c */
extern int Execute(DirEntry *dir_entry, FileEntry *file_entry);
extern int ExecuteCommand(FileEntry *fe_ptr, WalkingPackage *walking_package);
extern int GetCommandLine(char *command_line);
extern int GetSearchCommandLine(char *command_line, char *raw_pattern);

/* filter.c */
extern int ReadFilter(void);
extern int SetFilter(char *filter_spec, Statistic *s);
extern void ApplyFilter(DirEntry *dir_entry, Statistic *s);
extern BOOL Match(FileEntry *fe);

/* group.c */
extern char *GetDisplayGroupName(unsigned int gid);
extern int GetGroupId(char *name);
extern char *GetGroupName(unsigned int gid);
extern int ReadGroupEntries(void);

/* hex.c */
extern int ViewHex(char *file_path);

/* log.c */
extern int GetNewLoginPath(char *path);
extern int LoginDisk(char *path);
extern int CycleLoadedVolume(int direction);

/* mkdir.c */
extern DirEntry *MakeDirEntry( DirEntry *father_dir_entry, char *dir_name );
extern int MakeDirectory(DirEntry *father_dir_entry);
extern int MakePath( DirEntry *tree, char *dir_path, DirEntry **dest_dir_entry );
extern int EnsureDirectoryExists(char *dir_path, DirEntry *tree, BOOL *created, DirEntry **result_ptr, int *auto_create);

/* move.c */
extern int GetMoveParameter(char *from_file, char *to_file, char *to_dir);
extern int MoveFile(FileEntry *fe_ptr, unsigned char confirm, char *to_file, DirEntry *dest_dir_entry, char *to_dir_path, FileEntry **new_fe_ptr, int *dir_create_mode, int *overwrite_mode);
extern int MoveTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package);

/* passwd.c */
extern char *GetDisplayPasswdName(unsigned int uid);
extern char *GetPasswdName(unsigned int uid);
extern int GetPasswdUid(char *name);
extern int ReadPasswdEntries(void);

/* pipe.c */
extern int GetPipeCommand(char *pipe_command);
extern int Pipe(DirEntry *dir_entry, FileEntry *file_entry);
extern int PipeDirectory(DirEntry *dir_entry);
extern int PipeTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package);

/* profile.c */
extern char *GetExtViewer(char *filename);
extern char *GetProfileValue( char *key );
extern void SetProfileValue(char *name, char *value);
extern char *GetUserDirAction(int chkey, int *pchremap);
extern char *GetUserFileAction(int chkey, int *pchremap);
extern BOOL IsUserActionDefined(void);
extern int ReadProfile( char *filename );

/* rename.c */
extern int GetRenameParameter(char *old_name, char *new_name);
extern int RenameDirectory(DirEntry *de_ptr, char *new_name);
extern int RenameFile(FileEntry *fe_ptr, char *new_name, FileEntry **new_fe_ptr);
extern int RenameTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package);

/* sort.c */
extern void GetKindOfSort(void);
extern void SetKindOfSort(int new_kind_of_sort, Statistic *s);

/* system.c */
extern int QuerySystemCall(char *command_line, Statistic *s);
extern int SilentSystemCall(char *command_line, Statistic *s);
extern int SilentSystemCallEx(char *command_line, BOOL enable_clock, Statistic *s);
extern int SystemCall(char *command_line, Statistic *s);

/* usermode.c */
extern int DirUserMode(DirEntry *dir_entry, int ch);
extern int FileUserMode(FileEntryList *file_entry_list, int ch);

/* view.c */
extern int View(DirEntry * dir_entry, char *file_path);
extern int ViewTaggedFiles(DirEntry *dir_entry);

#endif /* YTREE_CMD_H */