/***************************************************************************
 *
 * ytree.h
 * Header-Datei fuer YTREE
 *
 ***************************************************************************/

#ifndef YTREE_H
#define YTREE_H


#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <ctype.h>
#include <math.h>

#include <locale.h> /* Required for modern POSIX (Linux/BSD/macOS) */

#ifdef XCURSES
#include <xcurses.h>
#define HAVE_CURSES 1
#endif

#if !defined(HAVE_CURSES)
#include <curses.h>
#include <term.h> /* ADDED: For tgetstr, tputs */
#endif

#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <pwd.h>
#include <setjmp.h>
#include <signal.h>
#include <string.h>
#include <stdlib.h>
#include <strings.h> /* For strcasecmp, strncasecmp */
#include <regex.h>

#include <sys/wait.h> /* Standard POSIX for wait() */
#include <sys/time.h> /* For setitimer (clock.c) */

#ifdef HAVE_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>
#endif

#ifdef WITH_UTF8
#include <wchar.h>
#endif

/* --- Consolidated Large File / 64-bit Definitions --- */

/* Assume modern POSIX systems use long long for 64-bit ints */
#define LONGLONG		long long
#define HAS_LONGLONG		1

/* Use standard POSIX statvfs headers and macro (most modern systems) */
#if !defined(WIN32) && !defined(__DJGPP__)
#include <sys/statvfs.h>
#include <sys/statfs.h> /* Keep for possible fallback structures */

/* Use statvfs as default, as it's the modern standard */
#define STATFS(a, b, c, d )     statvfs( a, b )
#endif /* !WIN32 && !__DJGPP__ */


/* Prefer lstat() for determining file type (e.g., links), otherwise fallback to stat() */
#if defined(S_IFLNK)
#define STAT_(a, b) lstat(a, b)
#else
#define STAT_(a, b) stat(a, b)
#endif


/* Some handy macros... */

#define MINIMUM( a, b ) ( ( (a) < (b) ) ? (a) : (b) )
#define MAXIMUM( a, b ) ( ( (a) > (b) ) ? (a) : (b) )


#ifdef WIN32
/* Windows/Cygwin/WSL compatibility layer simplifications */
#define  S_IREAD         S_IRUSR
#define  S_IWRITE        S_IWUSR
#define  S_IEXEC         S_IXUSR

#define  popen           _popen
#define  pclose          _pclose
#define  sys_errlist     _sys_errlist

/* No need for termcap emulation/tricks on modern terminals */
#define  echochar( ch )              { addch( ch ); refresh(); }
#define  putp( str )                 puts( str )
#define  vidattr( attr )
#define  typeahead( file )

#endif /* WIN32 */


#ifdef __DJGPP__
/* DJGPP GNU DOS Compiler compatibility layer simplifications */
#define  putp( str )                 puts( str )
#define  vidattr( attr )
#define  typeahead( file )
#endif /* __DJGPP__*/


#ifndef KEY_END
#define KEY_END   KEY_EOL
#endif


/* Standard POSIX S_IS* macros. Keep fallbacks minimal. */
#ifndef S_ISREG
#define S_ISREG( mode )   (((mode) & S_IFMT) == S_IFREG)
#endif /* S_ISREG */

#ifndef S_ISDIR
#define S_ISDIR( mode )   (((mode) & S_IFMT) == S_IFDIR)
#endif /* S_ISDIR */

#ifndef S_ISCHR
#define S_ISCHR( mode )   (((mode) & S_IFMT) == S_IFCHR)
#endif /* S_ISCHR */

#ifndef S_ISBLK
#define S_ISBLK( mode )   (((mode) & S_IFMT) == S_IFBLK)
#endif /* S_ISBLK */

#ifndef S_ISFIFO
#define S_ISFIFO( mode )   (((mode) & S_IFMT) == S_IFIFO)
#endif /* S_ISFIFO */

#ifndef S_ISLNK
#ifdef  S_IFLNK
#define S_ISLNK( mode )   (((mode) & S_IFMT) == S_IFLNK)
#else
#define S_ISLNK( mode )   FALSE
#endif /* S_IFLNK */
#endif /* S_ISLNK */

#ifndef S_ISSOCK
#ifdef  S_IFSOCK
#define S_ISSOCK( mode )   (((mode) & S_IFMT) == S_IFSOCK)
#else
#define S_ISSOCK( mode )   FALSE
#endif /* S_IFSOCK */
#endif /* S_ISSOCK */


/* VI_KEYS definitions remain */
#define VI_KEY_UP    'k'
#define VI_KEY_DOWN  'j'
#define VI_KEY_RIGHT 'l'
#define VI_KEY_LEFT  'h'
#define VI_KEY_NPAGE ( 'D' & 0x1F )
#define VI_KEY_PPAGE ( 'U' & 0x1F )


#define OWNER_NAME_MAX  64
#define GROUP_NAME_MAX  64
#define DISPLAY_OWNER_NAME_MAX  12
#define DISPLAY_GROUP_NAME_MAX  12


/* Sonderzeichen fuer Liniengrafik (using standard ncurses ACS_ macros) */
/* Keep these fallbacks as they are universally simple */
#ifndef ACS_ULCORNER
#define ACS_ULCORNER '+'
#endif
#ifndef ACS_URCORNER
#define ACS_URCORNER '+'
#endif
#ifndef ACS_LLCORNER
#define ACS_LLCORNER '+'
#endif
#ifndef ACS_LRCORNER
#define ACS_LRCORNER '+'
#endif
#ifndef ACS_VLINE
#define ACS_VLINE    '|'
#endif
#ifndef ACS_HLINE
#define ACS_HLINE    '-'
#endif
#ifndef ACS_RTEE
#define ACS_RTEE    '+'
#endif
#ifndef ACS_LTEE
#define ACS_LTEE    '+'
#endif
#ifndef ACS_BTEE
#define ACS_BTEE    '+'
#endif
#ifndef ACS_TTEE
#define ACS_TTEE    '+'
#endif
#ifndef ACS_BLOCK
#define ACS_BLOCK   '?'
#endif
#ifndef ACS_LARROW
#define ACS_LARROW  '<'
#endif


/* Color Definitionen */
enum UI_COLOR_PAIRS {
    CPAIR_DIR = 1,
    CPAIR_HIDIR,
    CPAIR_WINDIR,
    CPAIR_FILE,
    CPAIR_HIFILE,
    CPAIR_WINFILE,
    CPAIR_STATS,
    CPAIR_WINSTATS,
    CPAIR_BORDERS,
    CPAIR_HIMENUS,
    CPAIR_MENU,
    CPAIR_WINERR,
    CPAIR_HST,
    CPAIR_HIHST,
    CPAIR_WINHST,
    CPAIR_GLOBAL,
    CPAIR_HIGLOBAL,
    NUM_UI_COLOR_PAIRS, /* This will be 19, so pairs are 1-18 */
    F_COLOR_PAIR_BASE = 32
};


#define PROFILE_FILENAME	".ytree"
#define HISTORY_FILENAME	".ytree-hst"


/* User-Defines */
/*--------------*/

#include "config.h"


/* Auswahl der benutzten UNIX-Kommandos */
/*--------------------------------------*/

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
#define INODE_VIEWNAME		"ino"
#define ACCTIME_VIEWNAME	"act"
#define SCTIME_VIEWNAME		"sct"


#define CLOCK_INTERVAL	   1

#define FILE_SEPARATOR_CHAR   '/'
#define FILE_SEPARATOR_STRING "/"

#define ERR_TO_NULL           " 2> /dev/null"
#define ERR_TO_STDOUT         " 2>&1 "

#define BOOL       unsigned char
#define LF         10
#define ESC        27
#define LOGIN_ESC  '.'

#define QUICK_BAUD_RATE      9600

#define CR                     13

#define DIR_WINDOW_X         1
#define DIR_WINDOW_Y         2
#define DIR_WINDOW_WIDTH     (COLS - 26)
#define DIR_WINDOW_HEIGHT    ((LINES * 8 / 14)-1)

#define F2_WINDOW_X          DIR_WINDOW_X
#define F2_WINDOW_Y          DIR_WINDOW_Y
#define F2_WINDOW_WIDTH      DIR_WINDOW_WIDTH
#define F2_WINDOW_HEIGHT     (DIR_WINDOW_HEIGHT + 1)

#define FILE_WINDOW_1_X      1
#define FILE_WINDOW_1_Y      DIR_WINDOW_HEIGHT + 3
#define FILE_WINDOW_1_WIDTH  (COLS - 26)
#define FILE_WINDOW_1_HEIGHT (LINES - DIR_WINDOW_HEIGHT - 7 )

#define FILE_WINDOW_2_X      1
#define FILE_WINDOW_2_Y      2
#define FILE_WINDOW_2_WIDTH  (COLS - 26)
#define FILE_WINDOW_2_HEIGHT (LINES - 6)

#define ERROR_WINDOW_WIDTH   40
#define ERROR_WINDOW_HEIGHT  10
#define ERROR_WINDOW_X       ((COLS - ERROR_WINDOW_WIDTH) >> 1)
#define ERROR_WINDOW_Y       ((LINES - ERROR_WINDOW_HEIGHT) >> 1)

#define HISTORY_WINDOW_X       1
#define HISTORY_WINDOW_Y       2
#define HISTORY_WINDOW_WIDTH   (COLS - 26)
#define HISTORY_WINDOW_HEIGHT  (LINES - 6)

#define MATCHES_WINDOW_X       1
#define MATCHES_WINDOW_Y       2
#define MATCHES_WINDOW_WIDTH   (COLS - 26)
#define MATCHES_WINDOW_HEIGHT  (LINES - 6)

#define TIME_WINDOW_X        (COLS - 20)
#define TIME_WINDOW_Y        0
#define TIME_WINDOW_WIDTH    20
#define TIME_WINDOW_HEIGHT   1


#define PATH_LENGTH            1024
#define FILE_SPEC_LENGTH       (12 + 1)
#define DISK_NAME_LENGTH       (12 + 1)
#define MESSAGE_LENGTH         (PATH_LENGTH + 80 + 1)
#define COMMAND_LINE_LENGTH    4096
#define MODE_1                 0
#define MODE_2                 1
#define MODE_3                 2
#define MODE_4                 3
#define MODE_5                 4


#define ESCAPE               goto FNC_XIT

#define PRINT(ch) (iscntrl(ch) && (((unsigned char)(ch)) < ' ')) ? (ACS_BLOCK) : ((unsigned char)(ch))

/* ========================================================================= */
/*                              STRUCTURES                                   */
/* ========================================================================= */

typedef struct
{
    const char *name;
    int id;
    int fg;
    int bg;
} UIColor;

typedef struct _file_color_rule
{
    char *pattern;
    int fg;
    int bg;
    int pair_id;
    struct _file_color_rule *next;
} FileColorRule;


typedef struct _file_entry
{
  struct _file_entry *next;
  struct _file_entry *prev;
  struct _dir_entry  *dir_entry;
  struct stat        stat_struct;
  BOOL               tagged;
  BOOL               matching;
  char               name[1];
/*char               symlink_name[]; */ /* Folgt direkt dem Namen, falls */
					/* Eintrag == symbolischer Link  */
} FileEntry;



typedef struct _dir_entry
{
  struct _file_entry *file;
  struct _dir_entry  *next;
  struct _dir_entry  *prev;
  struct _dir_entry  *sub_tree;
  struct _dir_entry  *up_tree;
  LONGLONG           total_bytes;
  LONGLONG           matching_bytes;
  LONGLONG           tagged_bytes;
  unsigned int       total_files;
  unsigned int       matching_files;
  unsigned int       tagged_files;
  int                cursor_pos;
  int                start_file;
  struct   stat      stat_struct;
  BOOL               access_denied;
  BOOL               global_flag;
  BOOL               tagged_flag;
  BOOL               only_tagged;
  BOOL               not_scanned;
  BOOL               big_window;
  BOOL               login_flag;
  char               name[1];
} DirEntry;



typedef struct
{
  unsigned long      indent;
  DirEntry           *dir_entry;
  unsigned short     level;
} DirEntryList;


typedef struct
{
  FileEntry          *file;
} FileEntryList;


typedef struct
{
  DirEntry      *tree;
  LONGLONG	disk_space;
  LONGLONG	disk_capacity;
  LONGLONG	disk_total_files;
  LONGLONG	disk_total_bytes;
  LONGLONG	disk_matching_files;
  LONGLONG	disk_matching_bytes;
  LONGLONG	disk_tagged_files;
  LONGLONG 	disk_tagged_bytes;
  unsigned int  disk_total_directories;
  int           disp_begin_pos;
  int           cursor_pos;
  int           kind_of_sort;
  char          login_path[PATH_LENGTH + 1];
  char          path[PATH_LENGTH + 1];
  char          file_spec[FILE_SPEC_LENGTH + 1];
  char          disk_name[DISK_NAME_LENGTH + 1];
} Statistic;


typedef union
{
  struct
  {
    char      new_modus[11];
  } change_modus;

  struct
  {
    unsigned  new_owner_id;
  } change_owner;

  struct
  {
    unsigned  new_group_id;
  } change_group;

  struct
  {
    char      *command;
  } execute;

  struct
  {
    Statistic *statistic_ptr;
    DirEntry  *dest_dir_entry;
    char      *to_file;
    char      *to_path;
    BOOL      path_copy;
    BOOL      confirm;
  } copy;

  struct
  {
    char      *new_name;
    BOOL      confirm;
  } rename;

  struct
  {
    DirEntry  *dest_dir_entry;
    char      *to_file;
    char      *to_path;
    BOOL      confirm;
  } mv;

  struct
  {
    FILE      *pipe_file;
  } pipe_cmd;

  struct
  {
   FILE       *zipfile;
   int        method;
   } compress_cmd;

} FunctionData;


typedef struct
{
  FileEntry     *new_fe_ptr;
  FunctionData  function_data;
} WalkingPackage;

/* strerror() is POSIX, and all modern operating systems provide it.  */
#define HAVE_STRERROR 1

#ifndef HAVE_STRERROR
extern const char *StrError(int);
#endif /* HAVE_STRERROR */


extern WINDOW *dir_window;
extern WINDOW *small_file_window;
extern WINDOW *big_file_window;
extern WINDOW *file_window;
extern WINDOW *error_window;
extern WINDOW *history_window;
extern WINDOW *matches_window;
extern WINDOW *f2_window;
extern WINDOW *time_window;

extern Statistic statistic;
extern Statistic disk_statistic;
extern int       mode;
extern int       user_umask;
extern char      message[];
extern BOOL	 print_time;
extern BOOL      resize_request;
extern BOOL      bypass_small_window;
extern BOOL      highlight_full_line;
extern BOOL      hide_dot_files;
extern char      number_seperator;
extern char      *initial_directory;
extern char 	 builtin_hexdump_cmd[];

extern UIColor ui_colors[];
extern int NUM_UI_COLORS;
extern FileColorRule *file_color_rules_head;

extern char *getenv(const char *);

/* ========================================================================= */
/*                       FUNCTION PROTOTYPES                                 */
/* ========================================================================= */

/* main.c */
extern int ytree(int argc, char *argv[]);

/* archive.c */
extern int ExtractArchiveEntry(const char *archive_path, const char *entry_path, int out_fd);
extern int InsertArchiveFileEntry(DirEntry *tree, char *path, struct stat *stat);
extern int TryInsertArchiveDirEntry(DirEntry *tree, char *dir, struct stat *stat);
extern void MinimizeArchiveTree(DirEntry *tree);

/* archive_reader.c */
extern int ReadTreeFromArchive(DirEntry *dir_entry, const char *filename);

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
extern void WbkgdSet(WINDOW *w, chtype c);
extern void ParseColorString(const char *color_str, int *fg, int *bg);
extern void UpdateUIColor(const char *name, int fg, int bg);
extern void AddFileColorRule(const char *pattern, int fg, int bg);
extern int GetFileTypeColor(FileEntry *fe_ptr);
#else
#define StartColors()	;
#define WbkgdSet(a, b)  ;
#endif

/* copy.c */
extern int CopyFile(Statistic *statistic_ptr, FileEntry *fe_ptr, unsigned char confirm, char *to_file, DirEntry *dest_dir_entry, char *to_dir_path, BOOL path_copy);
extern int CopyTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package);
extern int GetCopyParameter(char *from_file, BOOL path_copy, char *to_file, char *to_dir);

/* delete.c */
extern int DeleteFile(FileEntry *fe_ptr);
extern int RemoveFile(FileEntry *fe_ptr);

/* dirwin.c */
extern void DisplayTree(WINDOW *win, int start_entry_no, int hilight_no);
extern int HandleDirWindow(DirEntry *start_dir_entry);
extern int KeyF2Get(DirEntry *start_dir_entry, int disp_begin_pos, int cursor_pos, char *path);
extern int RefreshDirWindow(void);
extern int ScanSubTree( DirEntry *dir_entry );
extern void ToggleDotFiles(void);

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

/* display_utils.c */
extern int AddStr(char *str);
extern int BuildUserFileEntry(FileEntry *fe_ptr, int filename_width, int linkname_width, char *template, int linelen, char *line);
extern char *CTime(time_t f_time, char *buffer);
extern char *CutFilename(char *dest, char *src, unsigned int max_len);
extern char *CutName(char *dest, char *src, unsigned int max_len);
extern char *CutPathname(char *dest, char *src, unsigned int max_len);
extern char *FormFilename(char *dest, char *src, unsigned int max_len);
extern char *GetAttributes(unsigned short modus, char *buffer);
extern void GetMaxYX(WINDOW *win, int *height, int *width);
extern int GetVisualUserFileEntryLength( int max_visual_filename_len, int max_visual_linkname_len, char *template);
extern int MvAddStr(int y, int x, char *str);
extern int MvWAddStr(WINDOW *win, int y, int x, char *str);
extern void Print(WINDOW *, int, int, char *, int);
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
extern int GetSearchCommandLine(char *command_line);

/* filespec.c */
extern int ReadFileSpec(void);
extern int SetFileSpec(char *file_spec);
extern void SetMatchingParam(DirEntry *dir_entry);

/* filewin.c */
extern void DisplayFileWindow(DirEntry *dir_entry);
extern int HandleFileWindow(DirEntry *dir_entry);
extern void RotateFileMode(void);
extern void SetFileMode(int new_file_mode);

/* freesp.c */
extern int GetAvailBytes(LONGLONG *avail_bytes);
extern int GetDiskParameter(char *path, char *volume_name, LONGLONG *avail_bytes, LONGLONG *capacity);

/* group.c */
extern char *GetDisplayGroupName(unsigned int gid);
extern int GetGroupId(char *name);
extern char *GetGroupName(unsigned int gid);
extern int ReadGroupEntries(void);

/* hex.c */
extern int InternalView(char *file_path);
extern int ViewHex(char *file_path);

/* history.c, keyhtab.c */
extern char *GetHistory(void);
extern char *GetMatches(char *);
extern void InsHistory(char *new_hist);
extern void ReadHistory(char *filename);
extern void SaveHistory(char *filename);

/* init.c */
extern int Init(char *configuration_file, char *history_file);
extern void ReCreateWindows(void);

/* input.c */
extern int Getch(void);
extern void HitReturnToContinue(void);
extern int InputChoise(char *msg, char *term);
extern int InputString(char *s, int y, int x, int cursor_pos, int length, char *term);
extern BOOL KeyPressed(void);
extern BOOL EscapeKeyPressed(void);
extern char *StrLeft(const char *str, size_t count);
extern char *StrRight(const char *str, size_t count);
extern int StrVisualLength(const char *str);
extern int ViKey( int ch );
extern int VisualPositionToBytePosition(const char *str, int visual_pos);
extern int WGetch(WINDOW *win);

/* log.c */
extern int GetNewLoginPath(char *path);
extern int LoginDisk(char *path);

/* match.c */
extern BOOL Match(char *file_name);
extern int SetMatchSpec(char *new_spec);

/* mkdir.c */
extern int MakeDirEntry( DirEntry *father_dir_entry, char *dir_name );
extern int MakeDirectory(DirEntry *father_dir_entry);
extern int MakePath( DirEntry *tree, char *dir_path, DirEntry **dest_dir_entry );

/* move.c */
extern int GetMoveParameter(char *from_file, char *to_file, char *to_dir);
extern int MoveFile(FileEntry *fe_ptr, unsigned char confirm, char *to_file, DirEntry *dest_dir_entry, char *to_dir_path, FileEntry **new_fe_ptr);
extern int MoveTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package);

/* owner_utils.c */
extern int ChangeOwnership(const char *path, uid_t new_uid, gid_t new_gid, struct stat *stat_buf);
extern int HandleDirOwnership(DirEntry *de_ptr, BOOL change_owner, BOOL change_group);
extern int HandleFileOwnership(FileEntry *fe_ptr, BOOL change_owner, BOOL change_group);

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
extern int PipeTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package);

/* profile.c */
extern char *GetExtViewer(char *filename);
extern char *GetProfileValue( char *key );
extern char *GetUserDirAction(int chkey, int *pchremap);
extern char *GetUserFileAction(int chkey, int *pchremap);
extern BOOL IsUserActionDefined(void);
extern int ReadProfile( char *filename );

/* quit.c */
extern void Quit(void);
extern void QuitTo(DirEntry * dir_entry);

/* readtree.c */
extern int ReadTree(DirEntry *dir_entry, char *path, int depth);
extern void UnReadTree(DirEntry *dir_entry);
extern int RescanDir(DirEntry *dir_entry, int depth);

/* rename.c */
extern int GetRenameParameter(char *old_name, char *new_name);
extern int RenameDirectory(DirEntry *de_ptr, char *new_name);
extern int RenameFile(FileEntry *fe_ptr, char *new_name, FileEntry **new_fe_ptr);
extern int RenameTaggedFiles(FileEntry *fe_ptr, WalkingPackage *walking_package);

/* rmdir.c */
extern int DeleteDirectory(DirEntry *dir_entry);

/* sort.c */
extern void GetKindOfSort(void);
extern void SetKindOfSort(int new_kind_of_sort);

/* stats.c */
extern void DisplayAvailBytes(void);
extern void DisplayDirParameter(DirEntry *dir_entry);
extern void DisplayDirStatistic(DirEntry *dir_entry);
extern void DisplayDirTagged(DirEntry *dir_entry);
extern void DisplayDiskName(void);
extern void DisplayDiskStatistic(void);
extern void DisplayDiskTagged(void);
extern void DisplayFileParameter(FileEntry *file_entry);
extern void DisplayFileSpec(void);
extern void DisplayGlobalFileParameter(FileEntry *file_entry);
extern void RecalculateSysStats(void);

/* string_utils.c */
extern int BuildFilename( char *in_filename, char *pattern, char *out_filename);
extern void StrCp(char *dest, const char *src);
extern int Strrcmp(char *s1, char* s2);
extern char *SubString(char *dest, char *src, int pos, int len);

/* system.c */
extern int QuerySystemCall(char *command_line);
extern int SilentSystemCall(char *command_line);
extern int SilentSystemCallEx(char *command_line, BOOL enable_clock);
extern int SystemCall(char *command_line);

/* tree_utils.c */
extern void DeleteTree(DirEntry *tree);
extern int GetDirEntry(DirEntry *tree, DirEntry *current_dir_entry, char *dir_path, DirEntry **dir_entry, char *to_path);
extern int GetFileEntry(DirEntry *de_ptr, char *file_name, FileEntry **file_entry);

/* usermode.c */
extern int DirUserMode(DirEntry *dir_entry, int ch);
extern int FileUserMode(FileEntryList *file_entry_list, int ch);

/* view.c */
extern int View(DirEntry * dir_entry, char *file_path);

#endif /* YTREE_H */