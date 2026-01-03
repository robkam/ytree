/***************************************************************************
 *
 * ytree.h
 * Header file for YTREE
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

#include "uthash.h" /* Required for volume management */

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
/* Cast to size_t to silence signed/unsigned comparison warnings when 'b' is sizeof/strlen */
#define MAXIMUM( a, b ) ( ( (a) > (b) ) ? (a) : (b) )

/* Standard Input Padding for Prompts */
#define UI_INPUT_PADDING 2


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
#define S_ISFIFO( mode )   (((mode) & S_IFMT) == S_IFFIFO)
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

#define BOOL       unsigned char
#define LF         10
#define ESC        27
#define LOGIN_ESC  '.'

#define QUICK_BAUD_RATE      9600

#define CR                     13

/* Auto-Refresh Configuration Modes */
#define REFRESH_WATCHER  1
#define REFRESH_ON_NAV   2
#define REFRESH_ON_ENTER 4

/* Window Dimension Definitions */

typedef struct {
    int dir_win_y;
    int dir_win_x;
    int dir_win_height;
    int dir_win_width;

    int small_file_win_y;
    int small_file_win_x;
    int small_file_win_height;
    int small_file_win_width;

    int big_file_win_y;
    int big_file_win_x;
    int big_file_win_height;
    int big_file_win_width;

    int stats_width;
    int main_win_width;
} YtreeLayout;

extern YtreeLayout layout;
extern void Layout_Recalculate(void);

/* Removed dynamic macros to use struct fields */
/* #define STATS_WIDTH          24 */
/* #define STATS_MARGIN         2 */
/* #define MAIN_WIN_WIDTH       (COLS - STATS_WIDTH - STATS_MARGIN) */
/* #define DIR_WINDOW_X         1 */
/* #define DIR_WINDOW_Y         2 */
/* #define DIR_WINDOW_WIDTH     MAIN_WIN_WIDTH */
/* #define DIR_WINDOW_HEIGHT    ((LINES * 8 / 14)-1) */
/* #define FILE_WINDOW_1_X      1 */
/* #define FILE_WINDOW_1_Y      DIR_WINDOW_HEIGHT + 3 */
/* #define FILE_WINDOW_1_WIDTH  MAIN_WIN_WIDTH */
/* #define FILE_WINDOW_1_HEIGHT (LINES - DIR_WINDOW_HEIGHT - 7 ) */
/* #define FILE_WINDOW_2_X      1 */
/* #define FILE_WINDOW_2_Y      2 */
/* #define FILE_WINDOW_2_WIDTH  MAIN_WIN_WIDTH */
/* #define FILE_WINDOW_2_HEIGHT (LINES - 6) */
/* #define STATS_MIDDLE_SEPARATOR_Y (DIR_WINDOW_HEIGHT + 2) */

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


/* Increased to 4096 to match standard Linux PATH_MAX and fix realpath warnings */
#define PATH_LENGTH            4096
#define FILE_SPEC_LENGTH       256
#define DISK_NAME_LENGTH       (12 + 1)
#define COMMAND_LINE_LENGTH    4096
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

/* History Types */
enum HistoryType {
    HST_GENERAL = 0,
    HST_LOGIN,
    HST_EXEC,
    HST_PIPE,
    HST_FILTER,
    HST_SEARCH,
    HST_FILE,
    HST_PATH,
    HST_ID,
    HST_CHANGE_MODUS
};

/* ************************************************************************* */
/*                              STRUCTURES                                   */
/* ************************************************************************* */

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
  char               name[]; /* C99 Flexible Array Member */
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
  char               name[]; /* C99 Flexible Array Member */
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
  int           login_mode; /* Renamed from mode to login_mode */
  char          login_path[PATH_LENGTH + 1];
  char          path[PATH_LENGTH + 1];
  char          file_spec[FILE_SPEC_LENGTH + 1];
  char          disk_name[DISK_NAME_LENGTH + 1];
} Statistic;

/* Volume structure to hold per-volume state */
struct Volume {
    int id;                 /* Key for hash table */
    Statistic vol_stats;    /* Holds tree, path, current dir stats */
    Statistic vol_disk_stats; /* Holds filesystem totals */

    /* Encapsulated Directory State Cache */
    DirEntryList *dir_entry_list;
    size_t dir_entry_list_capacity;
    int total_dirs;

    /* Encapsulated File State Cache */
    FileEntryList *file_entry_list;
    size_t file_entry_list_capacity;
    unsigned int file_count;

    UT_hash_handle hh;      /* For uthash macros */
};


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
    int       dir_create_mode;
    int       overwrite_mode;
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
    int       dir_create_mode;
    int       overwrite_mode;
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

/*
 * YtreeAction enumeration to abstract keys into logical actions.
 * Use generic "Command" actions (e.g., ACTION_CMD_M) for keys that
 * have different meanings in different windows.
 */
typedef enum {
    ACTION_NONE = 0,
    /* Navigation */
    ACTION_MOVE_UP, ACTION_MOVE_DOWN,
    ACTION_MOVE_SIBLING_NEXT, /* Added for ZTree-like TAB behavior */
    ACTION_MOVE_SIBLING_PREV, /* Added for Shift-Tab */
    ACTION_MOVE_LEFT, ACTION_MOVE_RIGHT,
    ACTION_PAGE_UP, ACTION_PAGE_DOWN,
    ACTION_HOME, ACTION_END,
    /* Tree Ops */
    ACTION_TREE_EXPAND,      /* TAB, * */
    ACTION_TREE_COLLAPSE,    /* BTAB, - */
    ACTION_TREE_EXPAND_ALL,  /* + */
    /* Standard Commands */
    ACTION_ENTER,       /* CR, LF */
    ACTION_ESCAPE,      /* ESC (27) */
    ACTION_LOGIN,       /* L, l */
    ACTION_QUIT,        /* q, Q */
    ACTION_QUIT_DIR,    /* ^Q */
    ACTION_TAG,         /* t */
    ACTION_UNTAG,       /* u */
    ACTION_TAG_ALL,     /* ^T, T */
    ACTION_UNTAG_ALL,   /* ^U, U */
    ACTION_TAG_REST,    /* ; */
    ACTION_UNTAG_REST,  /* : */
    ACTION_FILTER,      /* f, F */
    ACTION_TOGGLE_MODE, /* ^F */
    ACTION_REFRESH,     /* ^L */
    ACTION_RESIZE,      /* KEY_RESIZE */
    /* Volume Ops */
    ACTION_VOL_MENU,    /* K (Shift-K) */
    ACTION_VOL_PREV,    /* < , */
    ACTION_VOL_NEXT,    /* > . */
    /* Context Commands (Letters) */
    ACTION_CMD_A,       /* a, A */
    ACTION_CMD_B,       /* b, B */
    ACTION_CMD_C,       /* c, C */
    ACTION_CMD_D,       /* d, D */
    ACTION_CMD_E,       /* e, E */
    ACTION_CMD_G,       /* g, G */
    ACTION_CMD_H,       /* h, H */
    ACTION_CMD_M,       /* m, M */
    ACTION_CMD_O,       /* o, O */
    ACTION_CMD_P,       /* p, P */
    ACTION_CMD_R,       /* r, R */
    ACTION_CMD_S,       /* s, S */
    ACTION_CMD_V,       /* v, V */
    ACTION_CMD_X,       /* x, X */
    ACTION_CMD_Y,       /* y, Y */
    ACTION_TOGGLE_HIDDEN, /* ` */
    ACTION_TOGGLE_COMPACT, /* b, B - Brief Mode */
    /* Tagged/Ctrl Variants */
    ACTION_CMD_TAGGED_A, /* ^A */
    ACTION_CMD_TAGGED_C, /* ^C */
    ACTION_CMD_TAGGED_D, /* ^D */
    ACTION_CMD_TAGGED_G, /* ^G */
    ACTION_CMD_TAGGED_M, /* ^N (Move Tagged legacy) */
    ACTION_CMD_TAGGED_O, /* ^O */
    ACTION_CMD_TAGGED_P, /* ^P */
    ACTION_CMD_TAGGED_R, /* ^R */
    ACTION_CMD_TAGGED_S, /* ^S */
    ACTION_CMD_TAGGED_X, /* ^X */
    ACTION_CMD_TAGGED_Y, /* ^Y, ^K (Copy legacy?) -> Check mapping */
    /* Function Keys */
    ACTION_LIST_JUMP,            /* F12 */
    ACTION_TOGGLE_TAGGED_MODE,   /* 8, Shift-F4 */
    ACTION_TOGGLE_STATS,         /* ADDED */
    ACTION_ASTERISK,             /* * (Expand All / Invert Tags) */
    ACTION_USER_CMD              /* Reserved for future */
} YtreeAction;

/* State Restoration Structure */
typedef struct _PathList {
    char *path;
    struct _PathList *next;
} PathList;


/* View Context Structure for Split Screen Support
 *
 * This structure unifies the main application window pointers.
 *
 * Window Roles:
 * - dir_window:        The top window displaying the Directory Tree.
 * - small_file_window: The bottom window displaying the File List (Split View).
 * - big_file_window:   A full-height window displaying the File List (Zoom View).
 *                      This window physically overlaps both dir_window and small_file_window.
 * - file_window:       A dynamic pointer that points to either small_file_window or
 *                      big_file_window depending on the current view mode.
 */
typedef struct {
  WINDOW *dir_window;
  WINDOW *small_file_window;
  WINDOW *big_file_window;
  WINDOW *file_window;
  int view_mode; /* Operation mode (DISK_MODE, ARCHIVE_MODE, etc.) */
  BOOL show_stats; /* ADDED */
  int fixed_col_width; /* ADDED */
  int refresh_mode; /* ADDED: Auto-refresh configuration */
} ViewContext;

extern ViewContext *GlobalView;

/* Compatibility Macros */
/* These map the legacy global variable names to the new GlobalView structure members,
   allowing existing code to compile without modification. */
#define dir_window (GlobalView->dir_window)
#define small_file_window (GlobalView->small_file_window)
#define big_file_window (GlobalView->big_file_window)
#define file_window (GlobalView->file_window)
#define mode (GlobalView->view_mode)


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

/* Compatibility layer for existing code using global 'statistic' and 'disk_statistic' */
#define disk_statistic (CurrentVolume->vol_disk_stats)

/* extern int mode; */ /* REMOVED: Now macro mapping to GlobalView->view_mode */
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
extern struct Volume *Volume_GetByPath(const char *path); /* Added */

/* main.c */
extern int ytree(int argc, char *argv[]);

/* archive.c */
extern int ExtractArchiveEntry(const char *archive_path, const char *entry_path, int out_fd);
extern int ExtractArchiveNode(const char *archive_path, const char *entry_path, const char *dest_path);
extern int InsertArchiveFileEntry(DirEntry *tree, char *path, struct stat *stat, Statistic *s);
extern int TryInsertArchiveDirEntry(DirEntry *tree, char *dir, struct stat *stat, Statistic *s);
extern void MinimizeArchiveTree(DirEntry **tree_ptr, Statistic *s);

/* readarchive.c */
extern int ReadTreeFromArchive(DirEntry **dir_entry_ptr, const char *filename, Statistic *s);

/* animate.c */
extern void InitAnimation(void);
extern void StopAnimation(void);
extern void DrawAnimationStep(WINDOW *win);
extern void DrawSpinner(void); /* ADDED */

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
extern int CopyFileContent(char *to_path, char *from_path, Statistic *s); /* Added */

/* delete.c */
extern int DeleteFile(FileEntry *fe_ptr, int *auto_override, Statistic *s);
extern int RemoveFile(FileEntry *fe_ptr, Statistic *s);

/* dirwin.c */
extern void DisplayTree(struct Volume *vol, WINDOW *win, int start_entry_no, int hilight_no);
extern int HandleDirWindow(DirEntry *start_dir_entry);
extern int KeyF2Get(DirEntry *start_dir_entry, int disp_begin_pos, int cursor_pos, char *path);
extern int RefreshDirWindow(void);
extern int ScanSubTree( DirEntry *dir_entry, Statistic *s );
extern void ToggleDotFiles(void);
extern DirEntry *GetSelectedDirEntry(struct Volume *vol);
extern void BuildDirEntryList(struct Volume *vol); /* UPDATED: Takes Volume context */
extern void FreeDirEntryList(void); /* Retained for compat, or wraps FreeVolumeCache */
extern void FreeVolumeCache(struct Volume *vol); /* ADDED: Explicit cache cleaner */
extern DirEntry *RefreshTreeSafe(DirEntry *entry); /* ADDED: Safe Non-destructive Refresh */

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
extern void DisplayHeaderPath(char *path); /* ADDED: Prototype for dynamic header path */

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
extern void PrintLine(WINDOW *win, int y, int x, char *line, int len); /* ADDED: PrintLine prototype */
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
extern int GetSearchCommandLine(char *command_line, const char *prompt); /* Updated Prototype */

/* filter.c */
extern int ReadFilter(void);
extern int SetFilter(char *filter_spec, Statistic *s);
extern void ApplyFilter(DirEntry *dir_entry, Statistic *s);
extern BOOL Match(FileEntry *fe);

/* filewin.c */
extern void DisplayFileWindow(DirEntry *dir_entry);
extern int HandleFileWindow(DirEntry *dir_entry);
extern void RotateFileMode(void);
extern void SetFileMode(int new_file_mode);

/* freesp.c */
extern int GetAvailBytes(LONGLONG *avail_bytes, Statistic *s);
extern int GetDiskParameter(char *path, char *volume_name, LONGLONG *avail_bytes, LONGLONG *capacity, Statistic *s);

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
extern YtreeAction GetKeyAction(int ch); /* ADDED: Prototype for GetKeyAction */
extern int GetEventOrKey(void); /* ADDED: Prototype for Event Loop integration */

/* log.c */
extern int GetNewLoginPath(char *path);
extern int LoginDisk(char *path);
extern int SelectLoadedVolume(void);
extern int CycleLoadedVolume(int direction); /* Added for volume cycling */

/* mkdir.c */
extern DirEntry *MakeDirEntry( DirEntry *father_dir_entry, char *dir_name ); /* Return DirEntry* */
extern int MakeDirectory(DirEntry *father_dir_entry);
extern int MakePath( DirEntry *tree, char *dir_path, DirEntry **dest_dir_entry );
extern int EnsureDirectoryExists(char *dir_path, DirEntry *tree, BOOL *created, DirEntry **result_ptr, int *auto_create); /* Updated prototype */

/* move.c */
extern int GetMoveParameter(char *from_file, char *to_file, char *to_dir);
extern int MoveFile(FileEntry *fe_ptr, unsigned char confirm, char *to_file, DirEntry *dest_dir_entry, char *to_dir_path, FileEntry **new_fe_ptr, int *dir_create_mode, int *overwrite_mode);
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
extern char *CutFilename(char *dest, const char *src, unsigned int max_len);
extern char *CutName(char *dest, const char *src, unsigned int max_len);
extern char *CutPathname(char *dest, const char *src, unsigned int max_len);

/* pipe.c */
extern int GetPipeCommand(char *pipe_command);
extern int Pipe(DirEntry *dir_entry, FileEntry *file_entry);
extern int PipeDirectory(DirEntry *dir_entry); /* ADDED */
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
extern int ReadTree(DirEntry *dir_entry, char *path, int depth, Statistic *s);
extern void UnReadTree(DirEntry *dir_entry, Statistic *s);
extern int RescanDir(DirEntry *dir_entry, int depth, Statistic *s);

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
extern void DisplayDirStatistic(DirEntry *dir_entry, const char *title, Statistic *s); /* Changed prototype */
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

/* tree_utils.c */
extern void DeleteTree(DirEntry *tree);
extern int GetDirEntry(DirEntry *tree, DirEntry *current_dir_entry, char *dir_path, DirEntry **dir_entry, char *to_path);
extern int GetFileEntry(DirEntry *de_ptr, char *file_name, FileEntry **file_entry);

/* usermode.c */
extern int DirUserMode(DirEntry *dir_entry, int ch);
extern int FileUserMode(FileEntryList *file_entry_list, int ch);

/* view.c */
extern int View(DirEntry * dir_entry, char *file_path);
extern int InternalView(char *file_path); /* Ensure this is present and exported */

#include "watcher.h"

#endif /* YTREE_H */