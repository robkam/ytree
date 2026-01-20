/***************************************************************************
 *
 * ytree.h
 * Header file for YTREE
 *
 ***************************************************************************/

#ifndef YTREE_H
#define YTREE_H

#include "ytree_defs.h"
#include "ytree_fs.h"

/* Some handy macros... */

/* Cast to size_t to silence signed/unsigned comparison warnings when 'b' is sizeof/strlen */
/* MINIMUM/MAXIMUM Moved to ytree_defs.h */

#include "config.h"

/* User-Defines */
/*--------------*/

#define CAT             GetProfileValue( "CAT" )
#define HEXDUMP         GetProfileValue( "HEXDUMP" )
#define EDITOR          GetProfileValue( "EDITOR" )
#define PAGER           GetProfileValue( "PAGER" )
#define MELT            GetProfileValue( "MELT" )
#define UNCOMPRESS      GetProfileValue( "UNCOMPRESS" )
#define GNUUNZIP        GetProfileValue( "GNUUNZIP" )
#define BUNZIP          GetProfileValue( "BUNZIP" )
#define MANROFF         GetProfileValue( "MANROFF" )
#define ZSTDCAT         GetProfileValue( "ZSTDCAT" )
#define LUNZIP          GetProfileValue( "LUNZIP" )
#define TREEDEPTH       GetProfileValue( "TREEDEPTH" )
#define USERVIEW        GetProfileValue( "USERVIEW" )
#define FILEMODE        GetProfileValue( "FILEMODE" )
#define NUMBERSEP       GetProfileValue( "NUMBERSEP" )
#define NOSMALLWINDOW   GetProfileValue( "NOSMALLWINDOW" )
#define INITIALDIR      GetProfileValue( "INITIALDIR" )
#define DIR1            GetProfileValue( "DIR1" )
#define DIR2            GetProfileValue( "DIR2" )
#define FILE1           GetProfileValue( "FILE1" )
#define FILE2           GetProfileValue( "FILE2" )
#define SEARCHCOMMAND   GetProfileValue( "SEARCHCOMMAND" )
#define HEXEDITOFFSET   GetProfileValue( "HEXEDITOFFSET" )
#define LISTJUMPSEARCH  GetProfileValue( "LISTJUMPSEARCH" )
#define AUDIBLEERROR    GetProfileValue( "AUDIBLEERROR" )
#define CONFIRMQUIT     GetProfileValue( "CONFIRMQUIT" )
#define HIDEDOTFILES    GetProfileValue( "HIDEDOTFILES" )
#define AUTO_REFRESH    GetProfileValue("AUTO_REFRESH")

#define DEFAULT_TREE       "."

#define ERROR_MSG( msg )   Error( msg, __FILE__, __LINE__ )
#define WARNING( msg )     Warning( msg )
#define MESSAGE( msg )     Message( msg )

#define TAGGED_SYMBOL       '*'
#define MAX_MODES            4
#define DISK_MODE            0
#define LL_FILE_MODE         1 /* Legacy, may be removed */
#define ARCHIVE_MODE         2
#define USER_MODE            3

/* Obsolete compression method definitions removed */
#define FREEZE_COMPRESS             1
#define COMPRESS_COMPRESS           3
#define GZIP_COMPRESS               5
#define BZIP_COMPRESS               6
#define LZIP_COMPRESS               21
#define ZSTD_COMPRESS               22

#define SORT_BY_NAME       1
#define SORT_BY_MOD_TIME   2
#define SORT_BY_CHG_TIME   3
#define SORT_BY_ACC_TIME   4
#define SORT_BY_SIZE       5
#define SORT_BY_OWNER      6
#define SORT_BY_GROUP      7
#define SORT_BY_EXTENSION  8
#define SORT_ASC           10
#define SORT_DSC           20
#define SORT_CASE          40
#define SORT_ICASE         80

#define DEFAULT_FILE_SPEC "*"

#define TAGSYMBOL_VIEWNAME	"tag"
#define FILENAME_VIEWNAME	"fnm"
#define ATTRIBUTE_VIEWNAME	"atr"
#define LINKCOUNT_VIEWNAME	"lct"
#define FILESIZE_VIEWNAME	"fsz"
#define MODTIME_VIEWNAME	"mot"
#define SYMLINK_VIEWNAME	"lnm"
#define UID_VIEWNAME		"uid"
#define GID_VIEWNAME		"gid"
#define INODE_VIEWNAME	"ino"
#define ACCTIME_VIEWNAME	"act"
#define SCTIME_VIEWNAME	"sct"


#define CLOCK_INTERVAL	   1

#define FILE_SEPARATOR_CHAR   '/'
#define FILE_SEPARATOR_STRING "/"

#define ERR_TO_NULL           " 2> /dev/null"
#define ERR_TO_STDOUT         " 2>&1 "


/* Auto-Refresh Configuration Modes */
#define REFRESH_WATCHER  1
#define REFRESH_ON_NAV   2
#define REFRESH_ON_ENTER 4

/* View Return Codes */
#define VIEW_EXIT 0
#define VIEW_NEXT 1
#define VIEW_PREV 2

extern YtreeLayout layout;
extern void Layout_Recalculate(void);

/* Standard UI Vertical Layout */
#define Y_HEADER      0            /* Top header row */
#define Y_PROMPT      (LINES - 2)  /* Standard line for User Input / Prompts */
#define Y_STATUS      (LINES - 1)  /* Bottom line for Help/Status hints */
#define Y_MESSAGE     (LINES - 3)  /* Top of the 3-line Help/Message area */

/* Dependent Macros Updated to use Layout Struct */
#define F2_WINDOW_X          layout.dir_win_x
#define F2_WINDOW_Y          layout.dir_win_y
#define F2_WINDOW_WIDTH      layout.dir_win_width
#define F2_WINDOW_HEIGHT     (layout.dir_win_height + 1)

#define ERROR_WINDOW_WIDTH   40
#define ERROR_WINDOW_HEIGHT  10
#define ERROR_WINDOW_X       ((COLS - ERROR_WINDOW_WIDTH) >> 1)
#define ERROR_WINDOW_Y       ((LINES - ERROR_WINDOW_HEIGHT) >> 1)

#define HISTORY_WINDOW_X       1
#define HISTORY_WINDOW_Y       2
#define HISTORY_WINDOW_WIDTH   layout.main_win_width
#define HISTORY_WINDOW_HEIGHT  (LINES - 6)

#define MATCHES_WINDOW_X       1
#define MATCHES_WINDOW_Y       2
#define MATCHES_WINDOW_WIDTH   layout.main_win_width
#define MATCHES_WINDOW_HEIGHT  (LINES - 6)

#define TIME_WINDOW_X        (COLS - 20)
#define TIME_WINDOW_Y        0
#define TIME_WINDOW_WIDTH    20
#define TIME_WINDOW_HEIGHT   1

#define MODE_1                 0
#define MODE_2                 1
#define MODE_3                 2
#define MODE_4                 3
#define MODE_5                 4

/*
 * Message length increased to accommodate worst-case scenarios with two paths
 * (e.g. "Can't copy <path1> to <path2>") to prevent format-truncation warnings
 * and buffer overflows. (2 * 4096 + 256 for error text overhead)
 */
#define MESSAGE_LENGTH         ((PATH_LENGTH * 2) + 256)


#define ESCAPE               goto FNC_XIT

#ifdef WITH_UTF8
/* In UTF-8 mode, let ncurses handle the bytes. Only filter standard low control codes. */
#define PRINT(ch) ((unsigned char)(ch) < 32 && (ch) != 0 ? ACS_BLOCK : (ch))
#else
#define PRINT(ch) (iscntrl(ch) && (((unsigned char)(ch)) < ' ')) ? (ACS_BLOCK) : ((unsigned char)(ch))
#endif

#define PROFILE_FILENAME	".ytree"
#define HISTORY_FILENAME	".ytree-hst"


/* ************************************************************************* */
/*                       EXTERNS                                             */
/* ************************************************************************* */

extern ViewContext *GlobalView;

/* Compatibility Macros */
#define dir_window (GlobalView->ctx_dir_window)
#define small_file_window (GlobalView->ctx_small_file_window)
#define big_file_window (GlobalView->ctx_big_file_window)
#define file_window (GlobalView->ctx_file_window)
#define preview_window (GlobalView->ctx_preview_window)
#define mode (GlobalView->view_mode)

extern YtreePanel *LeftPanel;
extern YtreePanel *RightPanel;
extern YtreePanel *ActivePanel;
extern BOOL IsSplitScreen;

/* strerror() is POSIX, and all modern operating systems provide it.  */
#define HAVE_STRERROR 1

#ifndef HAVE_STRERROR
extern const char *StrError(int);
#endif /* HAVE_STRERROR */


/* Window Externs - The main view windows are now in ViewContext */
extern WINDOW *error_window;
extern WINDOW *history_window;
extern WINDOW *matches_window;
extern WINDOW *f2_window;
extern WINDOW *time_window;

/* Global volume management */
extern struct Volume *CurrentVolume;
extern struct Volume *VolumeList;
extern char GlobalSearchTerm[256];

/* Compatibility layer for existing code using global 'statistic' and 'disk_statistic' */
#define disk_statistic (CurrentVolume->vol_disk_stats)

extern int       user_umask;
extern char      message[];
extern BOOL	 print_time;
extern BOOL      resize_request;
extern BOOL      bypass_small_window;
extern BOOL      highlight_full_line;
extern BOOL      hide_dot_files;
extern int       animation_method;
extern char      number_seperator;
extern char      *initial_directory;
extern char 	 builtin_hexdump_cmd[];

extern UIColor ui_colors[];
extern int NUM_UI_COLORS;
extern FileColorRule *file_color_rules_head;

extern char *getenv(const char *);

/* ************************************************************************* */
/*                       FUNCTION PROTOTYPES                                 */
/* ************************************************************************* */

/* volume.c */
extern struct Volume *Volume_Create(void);
extern void Volume_Delete(struct Volume *vol);
extern void Volume_FreeAll(void);
extern struct Volume *Volume_GetByPath(const char *path);

/* main.c */
extern int ytree(int argc, char *argv[]);

/* animate.c */
extern void InitAnimation(void);
extern void StopAnimation(void);
extern void DrawAnimationStep(WINDOW *win);
extern void DrawSpinner(void);

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

/* clock.c */
extern void ClockHandler(int);
extern void InitClock(void);
extern void SuspendClock(void);

/* color.c */
#ifdef COLOR_SUPPORT
extern void StartColors(void);
extern void ReinitColorPairs(void);
extern void WbkgdSet(WINDOW *w, chtype c);
extern void ParseColorString(const char *color_str, int *fg, int *bg);
extern void UpdateUIColor(const char *name, int fg, int bg);
extern void AddFileColorRule(const char *pattern, int fg, int bg);
extern int GetFileTypeColor(FileEntry *fe_ptr);
#else
#define StartColors()	;
#define ReinitColorPairs() ;
#define WbkgdSet(a, b)  ;
#endif

/* copy.c */
extern int CopyFile(Statistic *statistic_ptr, FileEntry *fe_ptr, unsigned char confirm, char *to_file, DirEntry *dest_dir_entry, char *to_dir_path, BOOL path_copy, int *dir_create_mode, int *overwrite_mode);
extern int CopyTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package);
extern int GetCopyParameter(char *from_file, BOOL path_copy, char *to_file, char *to_dir);
extern int CopyFileContent(char *to_path, char *from_path, Statistic *s);

/* delete.c */
extern int DeleteFile(FileEntry *fe_ptr, int *auto_override, Statistic *s);
extern int RemoveFile(FileEntry *fe_ptr, Statistic *s);

/* dirwin.c */
extern void DisplayTree(struct Volume *vol, WINDOW *win, int start_entry_no, int hilight_no, BOOL is_active);
extern int HandleDirWindow(DirEntry *start_dir_entry);
extern int KeyF2Get(DirEntry *start_dir_entry, int disp_begin_pos, int cursor_pos, char *path);
extern int RefreshDirWindow(void);
extern int ScanSubTree( DirEntry *dir_entry, Statistic *s );
extern void ToggleDotFiles(void);
extern DirEntry *GetSelectedDirEntry(struct Volume *vol);
extern void BuildDirEntryList(struct Volume *vol);
extern void FreeDirEntryList(void);
extern void FreeVolumeCache(struct Volume *vol);
extern DirEntry *RefreshTreeSafe(DirEntry *entry);

/* display.c */
extern void ClearHelp(void);
extern void DisplayDirHelp(void);
extern void DisplayFileHelp(void);
extern void DisplayMenu(void);
extern void MapF2Window(void);
extern void RefreshWindow(WINDOW *win);
extern void SwitchToBigFileWindow(void);
extern void SwitchToSmallFileWindow(void);
extern void UnmapF2Window(void);
extern void DisplayHeaderPath(char *path);
extern void RenderInactivePanel(YtreePanel *panel);
extern void RefreshGlobalView(DirEntry *dir_entry);
extern void DisplayPreviewHelp(void);

/* display_utils.c */
extern int AddStr(char *str);
extern int BuildUserFileEntry(FileEntry *fe_ptr, int filename_width, int linkname_width, char *template, int linelen, char *line);
extern char *CTime(time_t f_time, char *buffer);
extern char *CutFilename(char *dest, const char *src, unsigned int max_len);
extern char *CutName(char *dest, const char *src, unsigned int max_len);
extern char *CutPathname(char *dest, const char *src, unsigned int max_len);
extern char *FormFilename(char *dest, char *src, unsigned int max_len);
extern char *GetAttributes(unsigned short modus, char *buffer);
extern void GetMaxYX(WINDOW *win, int *height, int *width);
extern int GetVisualUserFileEntryLength( int max_visual_filename_len, int max_visual_linkname_len, char *template);
extern int MvAddStr(int y, int x, char *str);
extern int MvWAddStr(WINDOW *win, int y, int x, char *str);
extern void Print(WINDOW *, int, int, char *, int);
extern void PrintLine(WINDOW *win, int y, int x, char *line, int len);
extern void PrintMenuOptions(WINDOW *,int, int, char *, int, int);
extern void PrintOptions(WINDOW *,int, int, char *);
extern void PrintSpecialString(WINDOW *win, int y, int x, char *str, int color);
extern int WAddStr(WINDOW *win, char *str);
extern int WAttrAddStr(WINDOW *win, int attr, char *str);

/* edit.c */
extern int Edit(DirEntry * dir_entry, char *file_path);

/* error.c */
extern void AboutBox(void);
extern void Error(char *msg, char *module, int line);
extern void Message(char *msg);
extern void Notice(char *msg);
extern void UnmapNoticeWindow(void);
extern void Warning(char *msg);

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

/* filewin.c */
extern void DisplayFileWindow(DirEntry *dir_entry, WINDOW *win);
extern int HandleFileWindow(DirEntry *dir_entry);
extern void RotateFileMode(void);
extern void SetFileMode(int new_file_mode);
extern void DisplayFiles(DirEntry *de_ptr, int start_file_no, int hilight_no, int start_x, WINDOW *win);
extern int ViewTaggedFiles(DirEntry *dir_entry);

/* group.c */
extern char *GetDisplayGroupName(unsigned int gid);
extern int GetGroupId(char *name);
extern char *GetGroupName(unsigned int gid);
extern int ReadGroupEntries(void);

/* hex.c */
extern int InternalView(char *file_path);
extern int ViewHex(char *file_path);

/* history.c, tabcompl.c */
extern char *GetHistory(int type);
extern char *GetMatches(char *);
extern void InsHistory(char *new_hist, int type);
extern void ReadHistory(char *filename);
extern void SaveHistory(char *filename);

/* init.c */
extern int Init(char *configuration_file, char *history_file);
extern void ReCreateWindows(void);

/* input.c */
extern int Getch(void);
extern void HitReturnToContinue(void);
extern int InputChoice(char *msg, char *term);
extern int InputString(char *s, int y, int x, int cursor_pos, int length, char *term, int history_type);
extern int InputStringEx(char *s, int y, int x, int cursor_pos, int display_width, int max_len, char *term, int history_type);
extern BOOL KeyPressed(void);
extern BOOL EscapeKeyPressed(void);
extern char *StrLeft(const char *str, size_t count);
extern char *StrRight(const char *str, size_t count);
extern int StrVisualLength(const char *str);
extern int ViKey( int ch );
extern int VisualPositionToBytePosition(const char *str, int visual_pos);
extern int WGetch(WINDOW *win);
extern YtreeAction GetKeyAction(int ch);
extern int GetEventOrKey(void);

/* log.c */
extern int GetNewLoginPath(char *path);
extern int LoginDisk(char *path);
extern int SelectLoadedVolume(void);
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

/* path_utils.c */
extern char *GetExtension(char *filename);
extern char *GetFileNamePath(FileEntry *file_entry, char *buffer);
extern char *GetPath(DirEntry *dir_entry, char *buffer);
extern char *GetRealFileNamePath(FileEntry *file_entry, char *buffer);
extern void Fnsplit(char *path, char *dir, char *name);
extern void NormPath( char *in_path, char *out_path );

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

/* quit.c */
extern void Quit(void);
extern void QuitTo(DirEntry * dir_entry);

/* rename.c */
extern int GetRenameParameter(char *old_name, char *new_name);
extern int RenameDirectory(DirEntry *de_ptr, char *new_name);
extern int RenameFile(FileEntry *fe_ptr, char *new_name, FileEntry **new_fe_ptr);
extern int RenameTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package);

/* rmdir.c */
extern int DeleteDirectory(DirEntry *dir_entry);

/* sort.c */
extern void GetKindOfSort(void);
extern void SetKindOfSort(int new_kind_of_sort, Statistic *s);

/* stats.c */
extern void DisplayAvailBytes(Statistic *s);
extern void DisplayDirParameter(DirEntry *dir_entry);
extern void DisplayDirStatistic(DirEntry *dir_entry, const char *title, Statistic *s);
extern void DisplayDirTagged(DirEntry *dir_entry, Statistic *s);
extern void DisplayDiskName(Statistic *s);
extern void DisplayDiskStatistic(Statistic *s);
extern void DisplayDiskTagged(Statistic *s);
extern void DisplayFileParameter(FileEntry *file_entry);
extern void DisplayFilter(Statistic *s);
extern void DisplayGlobalFileParameter(FileEntry *file_entry);
extern void RecalculateSysStats(Statistic *s);

/* string_utils.c */
extern int BuildFilename( char *in_filename, char *pattern, char *out_filename);
extern void StrCp(char *dest, const char *src);
extern int Strrcmp(char *s1, char* s2);
extern char *SubString(char *dest, char *src, int pos, int len);

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
extern void RenderFilePreview(WINDOW *win, char *filename, long *line_offset_ptr, int col_offset);
extern void RenderArchivePreview(WINDOW *win, char *archive_path, char *internal_path, long *line_offset_ptr);

#include "watcher.h"

#endif /* YTREE_H */