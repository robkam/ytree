/***************************************************************************
 *
 * ytree_defs.h
 * Global data structures and type definitions
 *
 ***************************************************************************/
#ifndef YTREE_DEFS_H
#define YTREE_DEFS_H

/* Large File Support must be defined before system headers */
#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <locale.h>

#ifdef XCURSES
#include <xcurses.h>
#define HAVE_CURSES 1
#endif

#if !defined(HAVE_CURSES)
#include <curses.h>
#include <term.h>
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
#include <strings.h>
#include <regex.h>
#include <sys/wait.h>
#include <sys/time.h>

#ifdef HAVE_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>
#endif

#ifdef WITH_UTF8
#include <wchar.h>
#endif

#include "uthash.h"

/* --- Macros & Constants --- */

#define LONGLONG		long long
#define HAS_LONGLONG		1

#if !defined(WIN32) && !defined(__DJGPP__)
#include <sys/statvfs.h>
#include <sys/statfs.h>
#define STATFS(a, b, c, d )     statvfs( a, b )
#endif

#if defined(S_IFLNK)
#define STAT_(a, b) lstat(a, b)
#else
#define STAT_(a, b) stat(a, b)
#endif

#define MINIMUM( a, b ) ( ( (a) < (b) ) ? (a) : (b) )
#define MAXIMUM( a, b ) ( ( (a) > (b) ) ? (a) : (b) )

#define UI_INPUT_PADDING 2

/* Win32 / DJGPP compat macros (keep them here as they affect system calls/types) */
#ifdef WIN32
#define  S_IREAD         S_IRUSR
#define  S_IWRITE        S_IWUSR
#define  S_IEXEC         S_IXUSR
#define  popen           _popen
#define  pclose          _pclose
#define  sys_errlist     _sys_errlist
#define  echochar( ch )              { addch( ch ); refresh(); }
#define  putp( str )                 puts( str )
#define  vidattr( attr )
#define  typeahead( file )
#endif

#ifdef __DJGPP__
#define  putp( str )                 puts( str )
#define  vidattr( attr )
#define  typeahead( file )
#endif

#ifndef KEY_END
#define KEY_END   KEY_EOL
#endif

/* S_IS* Macros */
#ifndef S_ISREG
#define S_ISREG( mode )   (((mode) & S_IFMT) == S_IFREG)
#endif
#ifndef S_ISDIR
#define S_ISDIR( mode )   (((mode) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISCHR
#define S_ISCHR( mode )   (((mode) & S_IFMT) == S_IFCHR)
#endif
#ifndef S_ISBLK
#define S_ISBLK( mode )   (((mode) & S_IFMT) == S_IFBLK)
#endif
#ifndef S_ISFIFO
#define S_ISFIFO( mode )   (((mode) & S_IFMT) == S_IFFIFO)
#endif
#ifndef S_ISLNK
#ifdef  S_IFLNK
#define S_ISLNK( mode )   (((mode) & S_IFMT) == S_IFLNK)
#else
#define S_ISLNK( mode )   FALSE
#endif
#endif
#ifndef S_ISSOCK
#ifdef  S_IFSOCK
#define S_ISSOCK( mode )   (((mode) & S_IFMT) == S_IFSOCK)
#else
#define S_ISSOCK( mode )   FALSE
#endif
#endif

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

/* ACS Fallbacks */
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

#define PATH_LENGTH            4096
#define FILE_SPEC_LENGTH       256
#define DISK_NAME_LENGTH       (12 + 1)
#define COMMAND_LINE_LENGTH    4096

#define BOOL       unsigned char
#define LF         10
#define ESC        27
#define LOGIN_ESC  '.'
#define CR         13
#define QUICK_BAUD_RATE      9600

/* Enums */

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
    NUM_UI_COLOR_PAIRS,
    F_COLOR_PAIR_BASE = 32
};

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

typedef enum {
    ACTION_NONE = 0,
    ACTION_MOVE_UP, ACTION_MOVE_DOWN,
    ACTION_MOVE_SIBLING_NEXT,
    ACTION_MOVE_SIBLING_PREV,
    ACTION_MOVE_LEFT, ACTION_MOVE_RIGHT,
    ACTION_PAGE_UP, ACTION_PAGE_DOWN,
    ACTION_HOME, ACTION_END,
    ACTION_TREE_EXPAND,
    ACTION_TREE_COLLAPSE,
    ACTION_TREE_EXPAND_ALL,
    ACTION_ENTER,
    ACTION_ESCAPE,
    ACTION_LOGIN,
    ACTION_QUIT,
    ACTION_QUIT_DIR,
    ACTION_TAG,
    ACTION_UNTAG,
    ACTION_TAG_ALL,
    ACTION_UNTAG_ALL,
    ACTION_TAG_REST,
    ACTION_UNTAG_REST,
    ACTION_FILTER,
    ACTION_TOGGLE_MODE,
    ACTION_REFRESH,
    ACTION_RESIZE,
    ACTION_VOL_MENU,
    ACTION_VOL_PREV,
    ACTION_VOL_NEXT,
    ACTION_CMD_A,
    ACTION_CMD_B,
    ACTION_CMD_C,
    ACTION_CMD_D,
    ACTION_CMD_E,
    ACTION_CMD_G,
    ACTION_CMD_H,
    ACTION_CMD_M,
    ACTION_CMD_O,
    ACTION_CMD_P,
    ACTION_CMD_R,
    ACTION_CMD_S,
    ACTION_CMD_V,
    ACTION_CMD_X,
    ACTION_CMD_Y,
    ACTION_TOGGLE_HIDDEN,
    ACTION_TOGGLE_COMPACT,
    ACTION_CMD_MKFILE,
    ACTION_CMD_TAGGED_A,
    ACTION_CMD_TAGGED_C,
    ACTION_CMD_TAGGED_D,
    ACTION_CMD_TAGGED_G,
    ACTION_CMD_TAGGED_M,
    ACTION_CMD_TAGGED_O,
    ACTION_CMD_TAGGED_P,
    ACTION_CMD_TAGGED_R,
    ACTION_CMD_TAGGED_S,
    ACTION_CMD_TAGGED_V,
    ACTION_CMD_TAGGED_X,
    ACTION_CMD_TAGGED_Y,
    ACTION_LIST_JUMP,
    ACTION_TOGGLE_TAGGED_MODE,
    ACTION_TOGGLE_STATS,
    ACTION_ASTERISK,
    ACTION_SPLIT_SCREEN,
    ACTION_SWITCH_PANEL,
    ACTION_VIEW_PREVIEW,
    ACTION_PREVIEW_SCROLL_UP,
    ACTION_PREVIEW_SCROLL_DOWN,
    ACTION_PREVIEW_HOME,
    ACTION_PREVIEW_END,
    ACTION_PREVIEW_PAGE_UP,
    ACTION_PREVIEW_PAGE_DOWN,
    ACTION_USER_CMD
} YtreeAction;

typedef enum {
    FOCUS_TREE,
    FOCUS_FILE
} ViewFocus;

/* Structs */

#ifdef HAVE_LIBARCHIVE
#define AR_KEEP   0
#define AR_SKIP   1
#define AR_ABORT -1
typedef int (*RewriteCallback)(struct archive *r, struct archive *w, struct archive_entry *entry, void *user_data);
#endif

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
  char               name[];
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
  char               name[];
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
  int           login_mode;
  char          login_path[PATH_LENGTH + 1];
  char          path[PATH_LENGTH + 1];
  char          file_spec[FILE_SPEC_LENGTH + 1];
  char          disk_name[DISK_NAME_LENGTH + 1];
} Statistic;

struct Volume {
    int id;
    Statistic vol_stats;
    Statistic vol_disk_stats;
    DirEntryList *dir_entry_list;
    size_t dir_entry_list_capacity;
    int total_dirs;
    UT_hash_handle hh;
};

typedef union
{
  struct { char      new_modus[12]; } change_modus;
  struct { unsigned  new_owner_id; } change_owner;
  struct { unsigned  new_group_id; } change_group;
  struct { char      *command; } execute;
  struct {
    Statistic *statistic_ptr;
    DirEntry  *dest_dir_entry;
    char      *to_file;
    char      *to_path;
    BOOL      path_copy;
    BOOL      confirm;
    int       dir_create_mode;
    int       overwrite_mode;
    void      *conflict_cb; /* ConflictCallback from ytree_cmd.h */
  } copy;
  struct { char      *new_name; BOOL      confirm; } rename;
  struct {
    DirEntry  *dest_dir_entry;
    char      *to_file;
    char      *to_path;
    BOOL      confirm;
    int       dir_create_mode;
    int       overwrite_mode;
    void      *conflict_cb; /* ConflictCallback from ytree_cmd.h */
  } mv;
  struct { FILE      *pipe_file; } pipe_cmd;
  struct { FILE       *zipfile; int        method; } compress_cmd;
} FunctionData;

typedef struct
{
  FileEntry     *new_fe_ptr;
  FunctionData  function_data;
} WalkingPackage;

typedef struct _PathList {
    char *path;
    struct _PathList *next;
} PathList;

typedef struct {
    WINDOW *pan_dir_window;
    WINDOW *pan_small_file_window;
    WINDOW *pan_big_file_window;
    WINDOW *pan_file_window;
    struct Volume *vol;
    FileEntryList *file_entry_list;
    size_t file_entry_list_capacity;
    unsigned int file_count;
    int dir_x, dir_y, dir_w, dir_h;
    int small_file_x, small_file_y, small_file_w, small_file_h;
    int big_file_x, big_file_y, big_file_w, big_file_h;
    int cursor_pos;
    int disp_begin_pos;
    int start_file;
    int file_mode;
    int max_column;
} YtreePanel;

typedef struct {
  WINDOW *ctx_dir_window;
  WINDOW *ctx_small_file_window;
  WINDOW *ctx_big_file_window;
  WINDOW *ctx_file_window;
  WINDOW *ctx_preview_window;
  int view_mode;
  BOOL show_stats;
  BOOL preview_mode;
  int fixed_col_width;
  int refresh_mode;
  ViewFocus focused_window;
  ViewFocus preview_entry_focus;
  YtreePanel *preview_return_panel;
  ViewFocus preview_return_focus;
} ViewContext;

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

    int preview_win_y;
    int preview_win_x;
    int preview_win_height;
    int preview_win_width;

    int stats_width;
    int main_win_width;

    int header_y;
    int message_y;
    int prompt_y;
    int status_y;
    int bottom_border_y;
} YtreeLayout;

#endif /* YTREE_DEFS_H */