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
#include "ytree_cmd.h"
#include "ytree_ui.h"

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

/* clock.c */
extern void ClockHandler(int);
extern void InitClock(void);
extern void SuspendClock(void);

/* dirwin.c */
extern int ScanSubTree( DirEntry *dir_entry, Statistic *s );

/* history.c, tabcompl.c */
extern char *GetHistory(int type);
extern char *GetMatches(char *);
extern void InsHistory(char *new_hist, int type);
extern void ReadHistory(char *filename);
extern void SaveHistory(char *filename);

/* init.c */
extern int Init(char *configuration_file, char *history_file);
extern void ReCreateWindows(void);

/* path_utils.c */
extern char *GetExtension(char *filename);
extern char *GetFileNamePath(FileEntry *file_entry, char *buffer);
extern char *GetPath(DirEntry *dir_entry, char *buffer);
extern char *GetRealFileNamePath(FileEntry *file_entry, char *buffer);
extern void Fnsplit(char *path, char *dir, char *name);
extern void NormPath( char *in_path, char *out_path );

/* quit.c */
extern void Quit(void);
extern void QuitTo(DirEntry * dir_entry);

/* rmdir.c */
extern int DeleteDirectory(DirEntry *dir_entry);

/* string_utils.c */
extern int BuildFilename( char *in_filename, char *pattern, char *out_filename);
extern void StrCp(char *dest, const char *src);
extern int Strrcmp(char *s1, char* s2);
extern char *SubString(char *dest, char *src, int pos, int len);

#include "watcher.h"

#endif /* YTREE_H */