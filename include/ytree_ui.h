/***************************************************************************
 *
 * ytree_ui.h
 * User Interface (Ncurses) rendering and input handling
 *
 ***************************************************************************/

#ifndef YTREE_UI_H
#define YTREE_UI_H

#include "ytree_defs.h"
#include "ytree_dialog.h"

/* animate.c */
extern void InitAnimation(ViewContext *ctx);
extern void StopAnimation(ViewContext *ctx);
extern void DrawAnimationStep(ViewContext *ctx, WINDOW *win);
extern void DrawSpinner(ViewContext *ctx);

/* color.c */
#ifdef COLOR_SUPPORT
extern void StartColors(ViewContext *ctx);
extern void ReinitColorPairs(ViewContext *ctx);
extern void WbkgdSet(ViewContext *ctx, WINDOW *w, chtype c);
extern void ParseColorString(const char *color_str, int *fg, int *bg);
extern void UpdateUIColor(const char *name, int fg, int bg);
extern void AddFileColorRule(ViewContext *ctx, const char *pattern, int fg,
                             int bg);
extern int GetFileTypeColor(ViewContext *ctx, FileEntry *fe_ptr);

#else
#define StartColors(ctx) ;
#define ReinitColorPairs(ctx) ;
#define WbkgdSet(ctx, a, b) ;
#endif

/* dirwin.c */
extern int HandleDirWindow(ViewContext *ctx, DirEntry *start_dir_entry);
extern int KeyF2Get(ViewContext *ctx, YtreePanel *panel, char *path);
extern int RefreshDirWindow(ViewContext *ctx, YtreePanel *panel);
extern void ToggleDotFiles(ViewContext *ctx, YtreePanel *panel);
extern DirEntry *GetSelectedDirEntry(ViewContext *ctx, struct Volume *vol);
extern DirEntry *GetPanelDirEntry(YtreePanel *p);
extern void BuildDirEntryList(ViewContext *ctx, struct Volume *vol,
                              int *index_ptr);
extern void FreeDirEntryList(ViewContext *ctx);
extern void FreeVolumeCache(struct Volume *vol);
extern DirEntry *RefreshTreeSafe(ViewContext *ctx, YtreePanel *panel,
                                 DirEntry *entry);

/* render_dir.c */
extern void DisplayTree(ViewContext *ctx, struct Volume *vol, WINDOW *win,
                        int start_entry_no, int hilight_no, BOOL is_active);
extern void PrintDirEntry(ViewContext *ctx, struct Volume *vol, WINDOW *win,
                          int entry_no, int y, unsigned char hilight,
                          BOOL is_active);
extern void SetDirMode(ViewContext *ctx, int new_mode);
extern void RotateDirMode(ViewContext *ctx);

/* display.c */
extern void ClearHelp(ViewContext *ctx);
extern void DisplayDirHelp(ViewContext *ctx);
extern void DisplayFileHelp(ViewContext *ctx);
extern void DisplayMenu(ViewContext *ctx);
extern void MapF2Window(ViewContext *ctx);
extern void RefreshWindow(WINDOW *win);
extern void SwitchToBigFileWindow(ViewContext *ctx);
extern void SwitchToSmallFileWindow(ViewContext *ctx);
extern void UnmapF2Window(ViewContext *ctx);
extern void DisplayHeaderPath(ViewContext *ctx, char *path);
extern void RenderInactivePanel(ViewContext *ctx, YtreePanel *panel);
extern void RefreshView(ViewContext *ctx, DirEntry *dir_entry);
extern void DisplayPreviewHelp(ViewContext *ctx);

/* display_utils.c */
extern int AddStr(char *str);
extern int BuildUserFileEntry(FileEntry *fe_ptr, int filename_width,
                              int linkname_width, char *pattern, int linelen,
                              char *line);
extern char *CTime(time_t f_time, char *buffer);
extern char *CutFilename(char *dest, const char *src, unsigned int max_len);
extern char *CutName(char *dest, const char *src, unsigned int max_len);
extern char *CutPathname(char *dest, const char *src, unsigned int max_len);
extern char *FormFilename(char *dest, char *src, unsigned int max_len);
extern char *GetAttributes(unsigned short modus, char *buffer);
extern void GetMaxYX(WINDOW *win, int *height, int *width);
extern int GetVisualUserFileEntryLength(int max_visual_filename_len,
                                        int max_visual_linkname_len,
                                        char *pattern);
extern int MvAddStr(int y, int x, char *str);
extern int MvWAddStr(WINDOW *win, int y, int x, char *str);
extern void Print(WINDOW *, int, int, char *, int);
extern void PrintLine(WINDOW *win, int y, int x, char *line, int len);
extern void PrintMenuOptions(WINDOW *, int, int, char *, int, int);
extern void PrintOptions(WINDOW *, int, int, char *);
extern void PrintSpecialString(WINDOW *win, int y, int x, char *str, int color);
extern int WAddStr(WINDOW *win, char *str);
extern int WAttrAddStr(WINDOW *win, int attr, char *str);

/* edit.c */
extern int Edit(ViewContext *ctx, DirEntry *dir_entry, char *file_path);

/* error.c */
extern void AboutBox(ViewContext *ctx);
extern void Error(char *msg, char *module, int line);
extern void Message(char *msg);
extern void Notice(char *msg);
extern void UnmapNoticeWindow(ViewContext *ctx);
extern void Warning(char *msg);
extern int UI_Error(ViewContext *ctx, const char *file, int line,
                    const char *fmt, ...);
extern int UI_Warning(ViewContext *ctx, const char *fmt, ...);
extern int UI_Message(ViewContext *ctx, const char *fmt, ...);
extern int UI_Notice(ViewContext *ctx, const char *fmt, ...);

/* filewin.c */
extern void BuildFileEntryList(ViewContext *ctx, YtreePanel *panel);
extern void DisplayFileWindow(ViewContext *ctx, YtreePanel *panel,
                              DirEntry *dir_entry);
extern int HandleFileWindow(ViewContext *ctx, DirEntry *dir_entry);

/* render_file.c */
extern void SetPanelFileMode(ViewContext *ctx, YtreePanel *p,
                             int new_file_mode);
extern int GetPanelFileMode(YtreePanel *p);
extern void RotatePanelFileMode(ViewContext *ctx, YtreePanel *p);
extern int GetPanelMaxColumn(YtreePanel *p);
extern void SetFileRenderingMetrics(YtreePanel *p, unsigned max_filename,
                                    unsigned max_linkname,
                                    unsigned max_userview);
extern void SetRenderSortOrder(YtreePanel *p, BOOL reverse);
extern void DisplayFiles(ViewContext *ctx, YtreePanel *panel,
                         DirEntry *dir_entry, int start_file_no, int hilight_no,
                         int start_x, WINDOW *win);
extern void PrintFileEntry(ViewContext *ctx, YtreePanel *panel, int entry_no,
                           int y, int x, unsigned char hilight, int start_x,
                           WINDOW *win);

/* hex.c has moved to cmd/hex.c and prototypes to ytree_cmd.h */

/* key_engine.c */
extern int Getch(ViewContext *ctx);
extern void HitReturnToContinue(void);
extern int InputChoice(ViewContext *ctx, const char *msg, const char *term);
extern int UI_AskConflict(ViewContext *ctx, const char *src_path,
                          const char *dst_path, int *mode_flags);

/* input_line.c */
extern int UI_ReadString(ViewContext *ctx, YtreePanel *panel,
                         const char *prompt, char *buffer, int max_len,
                         int history_type);

extern BOOL KeyPressed(void);
extern BOOL EscapeKeyPressed(void);
extern char *StrLeft(const char *str, size_t count);
extern char *StrRight(const char *str, size_t count);
extern int StrVisualLength(const char *str);
extern int ViKey(int ch);
extern int VisualPositionToBytePosition(const char *str, int visual_pos);
extern int WGetch(ViewContext *ctx, WINDOW *win);
extern YtreeAction GetKeyAction(ViewContext *ctx, int ch);
extern int GetEventOrKey(ViewContext *ctx);

/* stats.c */
extern void DisplayAvailBytes(ViewContext *ctx, Statistic *s);
extern void DisplayDirParameter(ViewContext *ctx, DirEntry *dir_entry);
extern void DisplayDirStatistic(ViewContext *ctx, DirEntry *dir_entry,
                                const char *title, Statistic *s);
extern void DisplayDirTagged(ViewContext *ctx, DirEntry *dir_entry,
                             Statistic *s);
extern void DisplayDiskName(ViewContext *ctx, Statistic *s);
extern void DisplayDiskStatistic(ViewContext *ctx, Statistic *s);
extern void DisplayDiskTagged(ViewContext *ctx, Statistic *s);
extern void DisplayFileParameter(ViewContext *ctx, FileEntry *file_entry);
extern void DisplayFileStatistic(ViewContext *ctx, FileEntry *file_entry,
                                 Statistic *s);
extern void UpdateStatsPanel(ViewContext *ctx, DirEntry *dir_entry,
                             Statistic *s);
extern void DisplayFilter(ViewContext *ctx, Statistic *s);
extern void DisplayGlobalFileParameter(ViewContext *ctx, FileEntry *file_entry);
extern void RecalculateSysStats(ViewContext *ctx, Statistic *s);

/* ui_nav.c */
extern void Nav_MoveDown(int *cursor, int *offset, int total_items,
                         int page_height, int step);
extern void Nav_MoveUp(int *cursor, int *offset);
extern void Nav_PageDown(int *cursor, int *offset, int total_items,
                         int page_height);
extern void Nav_PageUp(int *cursor, int *offset, int page_height);
extern void Nav_Home(int *cursor, int *offset);
extern void Nav_End(int *cursor, int *offset, int total_items, int page_height);

/* vol_menu.c */
extern int SelectLoadedVolume(ViewContext *ctx, int *return_key);

/* view_internal.c */
extern int InternalView(ViewContext *ctx, char *file_path);

/* view_preview.c */
extern void RenderFilePreview(ViewContext *ctx, WINDOW *win, char *filename,
                              long *line_offset_ptr, int col_offset);
extern void RenderArchivePreview(ViewContext *ctx, WINDOW *win,
                                 char *archive_path, char *internal_path,
                                 long *line_offset_ptr);

/* interactions.c */
extern int ChangeFileModus(ViewContext *ctx, FileEntry *fe_ptr);
extern int ChangeDirModus(ViewContext *ctx, DirEntry *de_ptr);
extern int HandleDirOwnership(ViewContext *ctx, DirEntry *de_ptr,
                              BOOL change_owner, unsigned char change_group);
extern int HandleFileOwnership(ViewContext *ctx, FileEntry *fe_ptr,
                               BOOL change_owner, unsigned char change_group);
extern int GetNewOwner(ViewContext *ctx, int st_uid);
extern int GetNewGroup(ViewContext *ctx, int st_gid);
extern int ChangeFileOrDirOwnership(ViewContext *ctx, const char *path,
                                    struct stat *stat_buf, BOOL change_owner,
                                    BOOL change_group);
extern int UI_ArchiveCallback(int status, const char *msg, void *user_data);
extern void shell_quote(char *dest, const char *src);
extern int recursive_mkdir(char *path);
extern int recursive_rmdir(const char *path);
extern int UI_ConflictResolverWrapper(ViewContext *ctx, const char *src_path,
                                      const char *dst_path, int *mode_flags);
extern void BuildFileEntryList(ViewContext *ctx, YtreePanel *panel);

#endif /* YTREE_UI_H */