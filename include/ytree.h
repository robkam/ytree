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
#define SMALLWINDOWSKIP GetProfileValue("SMALLWINDOWSKIP")
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

/* ************************************************************************* */
/*                       FUNCTION PROTOTYPES                                 */
/* ************************************************************************* */

/* ctrl_file.c */
extern void FreeFileEntryList(YtreePanel *panel);
extern void InvalidateVolumePanels(ViewContext *ctx, const struct Volume *vol);

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

/* init.c */
extern int Init(ViewContext *ctx, const char *configuration_file,
                const char *history_file);
extern void ReCreateWindows(ViewContext *ctx);
extern void ShutdownCurses(ViewContext *ctx);

/* path_utils.c */
extern const char *GetExtension(const char *filename);
extern char *GetFileNamePath(FileEntry *file_entry, char *buffer);
extern char *GetPath(DirEntry *dir_entry, char *buffer);
extern int Path_Join(char *dest, size_t size, const char *dir,
                     const char *leaf);
extern char *GetRealFileNamePath(FileEntry *file_entry, char *buffer,
                                 int view_mode);
extern void Fnsplit(char *path, char *dir, char *name);
extern void NormPath(char *in_path, char *out_path);

extern int MakePath(const ViewContext *ctx, DirEntry *tree, char *dir_path,
                    DirEntry **dest_dir_entry);
/* quit.c */
extern void Quit(ViewContext *ctx);
extern void QuitTo(ViewContext *ctx, DirEntry *dir_entry);

/* rmdir.c */

/* string_utils.c */
extern int BuildFilename(char *in_filename, char *pattern, char *out_filename);
extern int String_Replace(char *dest, size_t dest_size, const char *src,
                          const char *token, const char *replacement);

extern void Watcher_Init(ViewContext *ctx);
extern void Watcher_SetDir(ViewContext *ctx, const char *path);
extern int Watcher_GetFD(const ViewContext *ctx);
extern BOOL Watcher_ProcessEvents(ViewContext *ctx);
extern void Watcher_Close(ViewContext *ctx);

/* memory.c */
extern void *xmalloc(size_t size);
extern void *xcalloc(size_t nmemb, size_t size);
extern void *xrealloc(void *ptr, size_t size);
extern char *xstrdup(const char *s);

#include "watcher.h"

extern int(ReadProfile)(ViewContext *ctx, const char *filename);
extern void(ReadHistory)(ViewContext *ctx, const char *filename);
extern void(SaveHistory)(ViewContext *ctx, const char *filename);
extern void(InsHistory)(ViewContext *ctx, const char *NewHst, int type);
extern char *(GetHistory)(ViewContext * ctx, int type);

#endif /* YTREE_H */
