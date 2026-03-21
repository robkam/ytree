/***************************************************************************
 *
 * ytree.h
 * Header file for YTREE
 *
 ***************************************************************************/

#ifndef YTREE_H
#define YTREE_H

#include "ytree_cmd.h"
#include "ytree_defs.h"
#include "ytree_fs.h"
#include <stdio.h>
#define DEBUG_LOG(fmt, ...)                                                    \
  {                                                                            \
    FILE *fp = fopen("/tmp/ytree_debug.log", "a");                             \
    if (fp) {                                                                  \
      fprintf(fp, fmt "\n", ##__VA_ARGS__);                                    \
      fflush(fp);                                                              \
      fclose(fp);                                                              \
    }                                                                          \
  }
#include "ytree_ui.h"
#include <stdarg.h>

/* Some handy macros... */

/* Cast to size_t to silence signed/unsigned comparison warnings when 'b' is
 * sizeof/strlen */
/* MINIMUM/MAXIMUM Moved to ytree_defs.h */

#include "config.h"

/* User-Defines */
/*--------------*/

#define CAT GetProfileValue("CAT")
#define HEXDUMP GetProfileValue("HEXDUMP")
#define EDITOR GetProfileValue("EDITOR")
#define PAGER GetProfileValue("PAGER")
#define MELT GetProfileValue("MELT")
#define UNCOMPRESS GetProfileValue("UNCOMPRESS")
#define GNUUNZIP GetProfileValue("GNUUNZIP")
#define BUNZIP GetProfileValue("BUNZIP")
#define MANROFF GetProfileValue("MANROFF")
#define ZSTDCAT GetProfileValue("ZSTDCAT")
#define LUNZIP GetProfileValue("LUNZIP")
#define TREEDEPTH GetProfileValue("TREEDEPTH")
#define USERVIEW GetProfileValue("USERVIEW")
#define FILEMODE GetProfileValue("FILEMODE")
#define NUMBERSEP GetProfileValue("NUMBERSEP")
#define NOSMALLWINDOW GetProfileValue("NOSMALLWINDOW")
#define INITIALDIR GetProfileValue("INITIALDIR")
#define DIR1 GetProfileValue("DIR1")
#define DIR2 GetProfileValue("DIR2")
#define FILE1 GetProfileValue("FILE1")
#define FILE2 GetProfileValue("FILE2")
#define SEARCHCOMMAND GetProfileValue("SEARCHCOMMAND")
#define HEXEDITOFFSET GetProfileValue("HEXEDITOFFSET")
#define LISTJUMPSEARCH GetProfileValue("LISTJUMPSEARCH")
#define AUDIBLEERROR GetProfileValue("AUDIBLEERROR")
#define CONFIRMQUIT GetProfileValue("CONFIRMQUIT")
#define HIDEDOTFILES GetProfileValue("HIDEDOTFILES")
#define AUTO_REFRESH GetProfileValue("AUTO_REFRESH")

#define DEFAULT_TREE "."

/* UI Message Macros moved to ytree_defs.h */

#define TAGGED_SYMBOL '*'

/* Obsolete compression method definitions removed */
#define FREEZE_COMPRESS 1
#define COMPRESS_COMPRESS 3
#define GZIP_COMPRESS 5
#define BZIP_COMPRESS 6
#define LZIP_COMPRESS 21
#define ZSTD_COMPRESS 22

#define SORT_BY_NAME 1
#define SORT_BY_MOD_TIME 2
#define SORT_BY_CHG_TIME 3
#define SORT_BY_ACC_TIME 4
#define SORT_BY_SIZE 5
#define SORT_BY_OWNER 6
#define SORT_BY_GROUP 7
#define SORT_BY_EXTENSION 8
#define SORT_ASC 10
#define SORT_DSC 20
#define SORT_CASE 40
#define SORT_ICASE 80

#define DEFAULT_FILE_SPEC "*"

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

#define CLOCK_INTERVAL 1

#define FILE_SEPARATOR_CHAR '/'
#define FILE_SEPARATOR_STRING "/"

#define ERR_TO_NULL " 2> /dev/null"
#define ERR_TO_STDOUT " 2>&1 "

/* Auto-Refresh Configuration Modes */
#define REFRESH_WATCHER 1
#define REFRESH_ON_NAV 2
#define REFRESH_ON_ENTER 4

/* View Return Codes */
#define VIEW_EXIT 0
#define VIEW_NEXT 1
#define VIEW_PREV 2

extern void Layout_Recalculate(ViewContext *ctx);

/* Standard UI Vertical Layout */
#define Y_HEADER(ctx) ((ctx)->layout.header_y)
#define Y_PROMPT(ctx) ((ctx)->layout.prompt_y)
#define Y_STATUS(ctx) ((ctx)->layout.status_y)
#define Y_MESSAGE(ctx) ((ctx)->layout.message_y)

#define F2_WINDOW_X(ctx) ((ctx)->layout.dir_win_x)
#define F2_WINDOW_Y(ctx) ((ctx)->layout.dir_win_y)
#define F2_WINDOW_WIDTH(ctx) ((ctx)->layout.dir_win_width)
#define F2_WINDOW_HEIGHT(ctx) ((ctx)->layout.dir_win_height + 1)

#define ERROR_WINDOW_WIDTH 40
#define ERROR_WINDOW_HEIGHT 10
#define ERROR_WINDOW_X ((COLS - ERROR_WINDOW_WIDTH) >> 1)
#define ERROR_WINDOW_Y ((LINES - ERROR_WINDOW_HEIGHT) >> 1)

#define HISTORY_WINDOW_X 1
#define HISTORY_WINDOW_Y 2
#define HISTORY_WINDOW_WIDTH(ctx) ((ctx)->layout.main_win_width)
#define HISTORY_WINDOW_HEIGHT (LINES - 6)

#define MATCHES_WINDOW_X 1
#define MATCHES_WINDOW_Y 2
#define MATCHES_WINDOW_WIDTH(ctx) ((ctx)->layout.main_win_width)
#define MATCHES_WINDOW_HEIGHT (LINES - 6)

#define TIME_WINDOW_X (COLS - 20)
#define TIME_WINDOW_Y 0
#define TIME_WINDOW_WIDTH 20
#define TIME_WINDOW_HEIGHT 1

#define MODE_1 0
#define MODE_2 1
#define MODE_3 2
#define MODE_4 3
#define MODE_5 4

/*
 * Message length increased to accommodate worst-case scenarios with two paths
 * (e.g. "Can't copy <path1> to <path2>") to prevent format-truncation warnings
 * and buffer overflows. (2 * 4096 + 256 for error text overhead)
 */
#define MESSAGE_LENGTH ((PATH_LENGTH * 2) + 256)

#define ESCAPE goto FNC_XIT

#ifdef WITH_UTF8
/* In UTF-8 mode, let ncurses handle the bytes. Only filter standard low control
 * codes. */
#define PRINT(ch) ((unsigned char)(ch) < 32 && (ch) != 0 ? ACS_BLOCK : (ch))
#else
#define PRINT(ch)                                                              \
  (iscntrl(ch) && (((unsigned char)(ch)) < ' ')) ? (ACS_BLOCK)                 \
                                                 : ((unsigned char)(ch))
#endif

#define PROFILE_FILENAME ".ytree"
#define HISTORY_FILENAME ".ytree-hst"

/* ************************************************************************* */
/*                       EXTERNS                                             */
/* ************************************************************************* */

/* strerror() is POSIX, and all modern operating systems provide it.  */
#define HAVE_STRERROR 1

#ifndef HAVE_STRERROR
extern const char *StrError(int);
#endif /* HAVE_STRERROR */

/* Window Externs - The main view windows are now in ViewContext */

/* Global volume management */

extern UIColor ui_colors[];
extern int NUM_UI_COLORS;

extern char *getenv(const char *);
extern volatile sig_atomic_t ytree_shutdown_flag;

/* ************************************************************************* */
/*                       FUNCTION PROTOTYPES                                 */
/* ************************************************************************* */

/* ctrl_dir.c */
extern int HandleDirWindow(ViewContext *ctx, DirEntry *start_dir_entry);

/* ctrl_file.c */
extern void BuildFileEntryList(ViewContext *ctx, YtreePanel *panel);
extern void FreeFileEntryList(YtreePanel *panel);
extern void InvalidateVolumePanels(ViewContext *ctx, struct Volume *vol);

/* volume.c */
extern struct Volume *Volume_Create(ViewContext *ctx);
extern void Volume_Delete(ViewContext *ctx, struct Volume *vol);
extern void Volume_FreeAll(ViewContext *ctx);
extern struct Volume *Volume_GetByPath(ViewContext *ctx, const char *path);
extern struct Volume *Volume_Load(ViewContext *ctx, const char *path,
                                  struct Volume *reuse_vol,
                                  ScanProgressCallback cb, void *cb_user_data);
extern void SetKindOfSort(int kind_of_sort, Statistic *s);

/* main.c */
extern int ytree(int argc, char *argv[]);

/* clock.c */
extern void ClockHandler(ViewContext *ctx, int sig);
extern void InitClock(ViewContext *ctx);
extern void SuspendClock(ViewContext *ctx);

/* dirwin.c */
extern int ScanSubTree(ViewContext *ctx, DirEntry *dir_entry, Statistic *s);

/* history.c, tabcompl.c */
extern char *GetMatches(ViewContext *ctx, char *base);

/* init.c */
extern int Init(ViewContext *ctx, char *configuration_file, char *history_file);
extern void ReCreateWindows(ViewContext *ctx);
extern void ShutdownCurses(ViewContext *ctx);

/* path_utils.c */
extern char *GetExtension(char *filename);
extern char *GetFileNamePath(FileEntry *file_entry, char *buffer);
extern char *GetPath(DirEntry *dir_entry, char *buffer);
extern char *GetRealFileNamePath(FileEntry *file_entry, char *buffer,
                                 int view_mode);
extern void Fnsplit(char *path, char *dir, char *name);
extern void NormPath(char *in_path, char *out_path);

extern int MakePath(ViewContext *ctx, DirEntry *tree, char *dir_path,
                    DirEntry **dest);
/* quit.c */
extern void Quit(ViewContext *ctx);
extern void QuitTo(ViewContext *ctx, DirEntry *dir_entry);

/* rmdir.c */

/* string_utils.c */
extern int BuildFilename(char *in_filename, char *pattern, char *out_filename);
extern void StrCp(char *dest, const char *src);
extern int Strrcmp(char *s1, char *s2);
extern char *SubString(char *dest, char *src, int pos, int len);
extern int String_Replace(char *dest, size_t dest_size, const char *src,
                          const char *token, const char *replacement);

/* error.c */
extern int UI_Error(ViewContext *ctx, const char *file, int line,
                    const char *fmt, ...);
extern int UI_Message(ViewContext *ctx, const char *fmt, ...);
extern int UI_Warning(ViewContext *ctx, const char *fmt, ...);
extern int UI_Notice(ViewContext *ctx, const char *fmt, ...);
extern int(UI_ReadString)(ViewContext *ctx, YtreePanel *panel,
                          const char *prompt, char *buffer, int max_len,
                          int history_type);
extern void AboutBox(ViewContext *ctx);
extern void UnmapNoticeWindow(ViewContext *ctx);

extern void Watcher_Init(ViewContext *ctx);
extern void Watcher_SetDir(ViewContext *ctx, const char *path);
extern int Watcher_GetFD(ViewContext *ctx);
extern BOOL Watcher_ProcessEvents(ViewContext *ctx);
extern void Watcher_Close(ViewContext *ctx);

/* memory.c */
extern void *xmalloc(size_t size);
extern void *xcalloc(size_t nmemb, size_t size);
extern void *xrealloc(void *ptr, size_t size);
extern char *xstrdup(const char *s);

#include "watcher.h"

extern int(ReadProfile)(ViewContext *ctx, char *filename);
extern void(ReadHistory)(ViewContext *ctx, char *filename);
extern void(SaveHistory)(ViewContext *ctx, char *filename);
extern void(InsHistory)(ViewContext *ctx, char *NewHst, int type);
extern char *(GetHistory)(ViewContext * ctx, int type);
extern int ReadGroupEntries(void);
extern int ReadPasswdEntries(void);

extern char *(GetProfileValue)(ViewContext * ctx, const char *name);
extern void(SetProfileValue)(ViewContext *ctx, char *name, char *value);

#endif /* YTREE_H */
