/***************************************************************************
 *
 * ytree_defs.h
 * Global data structures and type definitions
 *
 ***************************************************************************/
#ifndef YTREE_DEFS_H
#define YTREE_DEFS_H

typedef struct _ViewContext ViewContext;

/* Large File Support must be defined before system headers */
#define _LARGEFILE64_SOURCE 1
#define _FILE_OFFSET_BITS 64

#include <ctype.h>
#include <locale.h>
#include <math.h>
#include <stdio.h>

#ifdef XCURSES
#include <xcurses.h>
#define HAVE_CURSES 1
#endif

#if !defined(HAVE_CURSES)
#include <curses.h>
#include <term.h>
#endif

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <grp.h>
#include <limits.h>
#include <pwd.h>
#include <regex.h>
#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#ifdef HAVE_LIBARCHIVE
#include <archive.h>
#include <archive_entry.h>
#endif

#ifdef WITH_UTF8
#include <wchar.h>
#endif

#include "uthash.h"

/* --- Macros & Constants --- */

#if !defined(WIN32) && !defined(__DJGPP__)
#include <sys/statfs.h>
#include <sys/statvfs.h>

#define STATFS(a, b, c, d) statvfs(a, b)
#endif

#if defined(S_IFLNK)
#define STAT_(a, b) lstat(a, b)
#else
#define STAT_(a, b) stat(a, b)
#endif

#define MINIMUM(a, b) (((a) < (b)) ? (a) : (b))
#define MAXIMUM(a, b) (((a) > (b)) ? (a) : (b))

#define UI_INPUT_PADDING 2

#define MAX_MODES 4
#define DISK_MODE 0
#define LL_FILE_MODE 1 /* Legacy, may be removed */
#define ARCHIVE_MODE 2
#define USER_MODE 3

/* Win32 / DJGPP compat macros (keep them here as they affect system
 * calls/types) */
#ifdef WIN32
#define S_IREAD S_IRUSR
#define S_IWRITE S_IWUSR
#define S_IEXEC S_IXUSR
#define popen _popen
#define pclose _pclose
#define sys_errlist _sys_errlist
#define echochar(ch)                                                           \
  {                                                                            \
    addch(ch);                                                                 \
    refresh();                                                                 \
  }
#define putp(str) puts(str)
#define vidattr(attr)
#define typeahead(file)
#endif

#ifdef __DJGPP__
#define putp(str) puts(str)
#define vidattr(attr)
#define typeahead(file)
#endif

#ifndef KEY_END
#define KEY_END KEY_EOL
#endif

/* S_IS* Macros */
#ifndef S_ISREG
#define S_ISREG(mode) (((mode) & S_IFMT) == S_IFREG)
#endif
#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & S_IFMT) == S_IFDIR)
#endif
#ifndef S_ISCHR
#define S_ISCHR(mode) (((mode) & S_IFMT) == S_IFCHR)
#endif
#ifndef S_ISBLK
#define S_ISBLK(mode) (((mode) & S_IFMT) == S_IFBLK)
#endif
#ifndef S_ISFIFO
#define S_ISFIFO(mode) (((mode) & S_IFMT) == S_IFFIFO)
#endif
#ifndef S_ISLNK
#ifdef S_IFLNK
#define S_ISLNK(mode) (((mode) & S_IFMT) == S_IFLNK)
#else
#define S_ISLNK(mode) FALSE
#endif
#endif
#ifndef S_ISSOCK
#ifdef S_IFSOCK
#define S_ISSOCK(mode) (((mode) & S_IFMT) == S_IFSOCK)
#else
#define S_ISSOCK(mode) FALSE
#endif
#endif

#define VI_KEY_UP 'k'
#define VI_KEY_DOWN 'j'
#define VI_KEY_RIGHT 'l'
#define VI_KEY_LEFT 'h'
#define VI_KEY_NPAGE ('D' & 0x1F)
#define VI_KEY_PPAGE ('U' & 0x1F)

#define OWNER_NAME_MAX 64
#define GROUP_NAME_MAX 64
#define DISPLAY_OWNER_NAME_MAX 12
#define DISPLAY_GROUP_NAME_MAX 12

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
#define ACS_VLINE '|'
#endif
#ifndef ACS_HLINE
#define ACS_HLINE '-'
#endif
#ifndef ACS_RTEE
#define ACS_RTEE '+'
#endif
#ifndef ACS_LTEE
#define ACS_LTEE '+'
#endif
#ifndef ACS_BTEE
#define ACS_BTEE '+'
#endif
#ifndef ACS_TTEE
#define ACS_TTEE '+'
#endif
#ifndef ACS_BLOCK
#define ACS_BLOCK '?'
#endif
#ifndef ACS_LARROW
#define ACS_LARROW '<'
#endif

#define PATH_LENGTH 4096
#define FILE_SPEC_LENGTH 256
#define DISK_NAME_LENGTH (12 + 1)
#define COMMAND_LINE_LENGTH 4096

#define FILE_SEPARATOR_CHAR '/'
#define FILE_SEPARATOR_STRING "/"

#define BOOL unsigned char
#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif
#define LF 10
#define ESC 27
#define LOGIN_ESC '.'
#define CR 13
#define QUICK_BAUD_RATE 9600

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
  CPAIR_INFO,
  CPAIR_WARN,
  CPAIR_ERR,
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
  ACTION_MOVE_UP,
  ACTION_MOVE_DOWN,
  ACTION_MOVE_SIBLING_NEXT,
  ACTION_MOVE_SIBLING_PREV,
  ACTION_MOVE_LEFT,
  ACTION_MOVE_RIGHT,
  ACTION_PAGE_UP,
  ACTION_PAGE_DOWN,
  ACTION_HOME,
  ACTION_END,
  ACTION_TREE_EXPAND,
  ACTION_TREE_COLLAPSE,
  ACTION_TREE_EXPAND_ALL,
  ACTION_ENTER,
  ACTION_ESCAPE,
  ACTION_LOGIN,
  ACTION_LOG,
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
  ACTION_TO_DIR,
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
  ACTION_COMPARE_FILE,
  ACTION_COMPARE_DIR,
  ACTION_COMPARE_TREE,
  ACTION_USER_CMD
} YtreeAction;

typedef enum { FOCUS_TREE, FOCUS_FILE } ViewFocus;

typedef enum {
  COMPARE_FLOW_FILE = 0,
  COMPARE_FLOW_DIRECTORY,
  COMPARE_FLOW_LOGGED_TREE
} CompareFlowType;

typedef enum {
  COMPARE_BASIS_NONE = 0,
  COMPARE_BASIS_SIZE,
  COMPARE_BASIS_DATE,
  COMPARE_BASIS_SIZE_AND_DATE,
  COMPARE_BASIS_HASH
} CompareBasis;

typedef enum {
  COMPARE_TAG_NONE = 0,
  COMPARE_TAG_DIFFERENT,
  COMPARE_TAG_MATCH,
  COMPARE_TAG_NEWER,
  COMPARE_TAG_OLDER,
  COMPARE_TAG_UNIQUE,
  COMPARE_TAG_TYPE_MISMATCH,
  COMPARE_TAG_ERROR
} CompareTagResult;

typedef enum {
  COMPARE_MENU_CANCEL = 0,
  COMPARE_MENU_DIRECTORY_ONLY,
  COMPARE_MENU_DIRECTORY_PLUS_TREE
} CompareMenuChoice;

typedef struct {
  CompareFlowType flow_type;
  CompareBasis basis;
  CompareTagResult tag_result;
  BOOL used_split_default_target;
  char source_path[PATH_LENGTH + 1];
  char target_path[PATH_LENGTH + 1];
} CompareRequest;

/* Structs */

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

  int stats_y_filter_val;
  int stats_y_vol_sep;
  int stats_y_vol_info;
  int stats_y_vstat_sep;
  int stats_y_vstat_val;
  int stats_y_dstat_sep;
  int stats_y_dstat_val;
  int stats_y_attr_sep;
  int stats_y_attr_val;

  int header_y;
  int message_y;
  int prompt_y;
  int status_y;
  int bottom_border_y;
} YtreeLayout;

#ifdef HAVE_LIBARCHIVE
#define AR_KEEP 0
#define AR_SKIP 1
#define AR_ABORT -1
typedef int (*ArchiveProgressCallback)(int status, const char *msg,
                                       void *user_data);
typedef int (*RewriteCallback)(struct archive *r, struct archive *w,
                               struct archive_entry *entry, void *user_data);
#else
typedef int (*ArchiveProgressCallback)(int status, const char *msg,
                                       void *user_data);
#endif

/* UI Callback Status Codes (for ArchiveProgressCallback) */
#define ARCHIVE_STATUS_PROGRESS 1
#define ARCHIVE_STATUS_ERROR 2
#define ARCHIVE_STATUS_WARNING 3
#define ARCHIVE_CB_CONTINUE 0
#define ARCHIVE_CB_ABORT -1

/* Sort methods */
#define TAGSYMBOL_VIEWNAME "tag"
#define FILENAME_VIEWNAME "fnm"
#define ATTRIBUTE_VIEWNAME "atr"
#define LINKCOUNT_VIEWNAME "lct"
#define FILESIZE_VIEWNAME "fsz"
#define MODTIME_VIEWNAME "mot"
#define SYMLINK_VIEWNAME "lnm"
#define UID_VIEWNAME "uid"
#define GID_VIEWNAME "gid"
#define INODE_VIEWNAME "ino"
#define ACCTIME_VIEWNAME "act"
#define SCTIME_VIEWNAME "sct"

/* Compression methods */
#define FREEZE_COMPRESS 1
#define COMPRESS_COMPRESS 3
#define GZIP_COMPRESS 5
#define BZIP_COMPRESS 6
#define LZIP_COMPRESS 21
#define ZSTD_COMPRESS 22

/* UI Message Macros (Headless-compatible) */
extern int UI_Error(ViewContext *ctx, const char *file, int line,
                    const char *format, ...);
extern int UI_Warning(ViewContext *ctx, const char *format, ...);
extern int UI_Message(ViewContext *ctx, const char *format, ...);
extern int UI_Notice(ViewContext *ctx, const char *format, ...);

#define ERROR(ctx, ...) UI_Error(ctx, __FILE__, __LINE__, __VA_ARGS__)
#define WARNING(ctx, ...) UI_Warning(ctx, __VA_ARGS__)
#define MESSAGE(ctx, ...) UI_Message(ctx, __VA_ARGS__)
#define NOTICE(ctx, ...) UI_Notice(ctx, __VA_ARGS__)

/* Input Debugging */

typedef struct {
  const char *name;
  int id;
  int fg;
  int bg;
} UIColor;

typedef struct _file_color_rule {
  char *pattern;
  int fg;
  int bg;
  int pair_id;
  struct _file_color_rule *next;
} FileColorRule;

typedef struct _file_entry {
  struct _file_entry *next;
  struct _file_entry *prev;
  struct _dir_entry *dir_entry;
  struct stat stat_struct;
  BOOL tagged;
  BOOL matching;
  char name[];
} FileEntry;

typedef struct _dir_entry {
  struct _file_entry *file;
  struct _dir_entry *next;
  struct _dir_entry *prev;
  struct _dir_entry *sub_tree;
  struct _dir_entry *up_tree;
  long long total_bytes;
  long long matching_bytes;
  long long tagged_bytes;
  unsigned int total_files;
  unsigned int matching_files;
  unsigned int tagged_files;
  int cursor_pos;
  int start_file;
  struct stat stat_struct;
  BOOL access_denied;
  BOOL global_flag;
  BOOL global_all_volumes;
  BOOL tagged_flag;
  BOOL only_tagged;
  BOOL not_scanned;
  BOOL big_window;
  BOOL login_flag;
  char name[];
} DirEntry;

typedef struct {
  unsigned long indent;
  DirEntry *dir_entry;
  unsigned short level;
} DirEntryList;

typedef struct {
  FileEntry *file;
} FileEntryList;

typedef struct {
  DirEntry *tree;
  long long disk_space;
  long long disk_capacity;
  long long disk_total_files;
  long long disk_total_bytes;
  long long disk_matching_files;
  long long disk_matching_bytes;
  long long disk_tagged_files;
  long long disk_tagged_bytes;
  unsigned int disk_total_directories;
  int kind_of_sort;
  int login_mode;
  char login_path[PATH_LENGTH + 1];
  char path[PATH_LENGTH + 1];
  char file_spec[FILE_SPEC_LENGTH + 1];
  char disk_name[DISK_NAME_LENGTH + 1];
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

typedef union {
  struct {
    char new_mode[12];
  } change_mode;
  struct {
    unsigned new_owner_id;
  } change_owner;
  struct {
    unsigned new_group_id;
  } change_group;
  struct {
    char *command;
  } execute;
  struct {
    Statistic *statistic_ptr;
    DirEntry *dest_dir_entry;
    char *to_file;
    char *to_path;
    BOOL path_copy;
    BOOL confirm;
    int dir_create_mode;
    int overwrite_mode;
    void *conflict_cb; /* ConflictCallback from ytree_cmd.h */
    void *choice_cb;   /* ChoiceCallback from ytree_cmd.h */
  } copy;
  struct {
    char *new_name;
    BOOL confirm;
  } rename;
  struct {
    DirEntry *dest_dir_entry;
    char *to_file;
    char *to_path;
    BOOL confirm;
    int dir_create_mode;
    int overwrite_mode;
    void *conflict_cb; /* ConflictCallback from ytree_cmd.h */
    void *choice_cb;   /* ChoiceCallback from ytree_cmd.h */
  } mv;
  struct {
    FILE *pipe_file;
  } pipe_cmd;
  struct {
    FILE *zipfile;
    int method;
  } compress_cmd;
  struct {
    Statistic *statistic_ptr;
    int auto_override;
    void *choice_cb;
  } del;
} FunctionData;

typedef struct {
  FileEntry *new_fe_ptr;
  FunctionData function_data;
} WalkingPackage;

typedef struct _PathList {
  char *path;
  struct _PathList *next;
} PathList;

typedef struct {
#ifdef YTREE_TUI
  WINDOW *pan_dir_window;
  WINDOW *pan_small_file_window;
  WINDOW *pan_big_file_window;
  WINDOW *pan_file_window;
#else
  void *pan_dir_window;
  void *pan_small_file_window;
  void *pan_big_file_window;
  void *pan_file_window;
#endif
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
  int current_dir_entry;
  unsigned int max_visual_filename_len;
  unsigned int max_visual_linkname_len;
  unsigned int max_visual_userview_len;
  BOOL reverse_sort;
} YtreePanel;

typedef struct _history {
  char *hst;
  int type;
  int pinned;
  struct _history *next;
  struct _history *prev;
} History;

typedef struct {
  int wlines;
  int wcols;
  int bytes;
  WINDOW *view;
  WINDOW *border;
  BOOL resize_done;
  BOOL inhex;
  BOOL inedit;
  BOOL hexoffset;
} ViewerState;

typedef struct _ViewContext {
  WINDOW *ctx_dir_window;
  WINDOW *ctx_small_file_window;
  WINDOW *ctx_big_file_window;
  WINDOW *ctx_file_window;
  WINDOW *ctx_preview_window;
  WINDOW *ctx_border_window;
  WINDOW *ctx_path_window;
  WINDOW *ctx_time_window;
  WINDOW *ctx_menu_window;
  WINDOW *ctx_error_window;
  WINDOW *ctx_history_window;
  WINDOW *ctx_matches_window;
  WINDOW *ctx_f2_window;

  YtreePanel *left;
  YtreePanel *right;
  YtreePanel *active;
  YtreeLayout layout;

  int view_mode;
  int dir_mode;
  BOOL show_stats;
  BOOL preview_mode;
  BOOL clock_print_time;
  int fixed_col_width;
  int refresh_mode;
  ViewFocus focused_window;
  ViewFocus preview_entry_focus;
  YtreePanel *preview_return_panel;
  ViewFocus preview_return_focus;

  ViewerState viewer;

  /* Animation State */
  BOOL anim_is_initialized;
  void *anim_stars;
  int spin_counter;

  /* Color State */
  BOOL color_enabled;

  /* Global state moved to context */
  int animation_method;
  char number_seperator;
  BOOL is_split_screen;
  char global_search_term[256];
  int user_umask;
  BOOL resize_request;
  int cached_lines; /* Last known LINES for resize detection */
  int cached_cols;  /* Last known COLS for resize detection */
  BOOL bypass_small_window;
  BOOL highlight_full_line;
  BOOL hide_dot_files;
  char *initial_directory;
  char *confirm_quit;
  void *file_color_rules_head;

  /* ctrl_file state */
  int ctrl_file_max_disp_files;
  int ctrl_file_x_step;
  int ctrl_file_my_x_step;
  int ctrl_file_hide_right;

  unsigned ctrl_file_max_visual_filename_len;
  unsigned ctrl_file_max_visual_linkname_len;
  unsigned ctrl_file_max_visual_userview_len;
  unsigned ctrl_file_global_max_visual_filename_len;
  unsigned ctrl_file_global_max_visual_linkname_len;

  long ctrl_file_preview_line_offset;
  int ctrl_file_saved_fixed_width;

  char ctrl_file_to_dir[PATH_LENGTH + 1];
  char ctrl_file_to_path[PATH_LENGTH + 1];
  char ctrl_file_to_file[PATH_LENGTH + 1];

  /* profile.c state */
  void *profile_data;  /* Pointer to the profile array */
  void *viewer_list;   /* Pointer to head of Viewer list */
  void *dirmenu_list;  /* Pointer to head of Dirmenu list */
  void *filemenu_list; /* Pointer to head of Filemenu list */

  /* history.c state */
  int total_hist;
  int cursor_pos;
  int disp_begin_pos;
  History *history_head;
  History **history_view_list;
  int history_view_count;
  /* volume.c state */
  struct Volume *volumes_head;
  int volume_serial;

  /* watcher.c state */
  int inotify_fd;
  int current_wd;
  char current_watch_path[PATH_LENGTH + 1];

  /* tabcompl.c state */
  char **tab_mtchs;
  int tab_total_matches;
  int tab_cursor_pos;
  int tab_disp_begin_pos;

} ViewContext;

#endif /* YTREE_DEFS_H */
