/***************************************************************************
 *
 * ytree_ui.h
 * User Interface (Ncurses) rendering and input handling
 *
 ***************************************************************************/

#ifndef YTREE_UI_H
#define YTREE_UI_H

#include "ytree_defs.h"

/* animate.c */
extern void InitAnimation(void);
extern void StopAnimation(void);
extern void DrawAnimationStep(WINDOW *win);
extern void DrawSpinner(void);

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

/* dirwin.c */
extern int HandleDirWindow(DirEntry *start_dir_entry);
extern int KeyF2Get(DirEntry *start_dir_entry, int disp_begin_pos, int cursor_pos, char *path);
extern int RefreshDirWindow(YtreePanel *panel);
extern void ToggleDotFiles(YtreePanel *panel);
extern DirEntry *GetSelectedDirEntry(struct Volume *vol);
extern void BuildDirEntryList(struct Volume *vol);
extern void FreeDirEntryList(void);
extern void FreeVolumeCache(struct Volume *vol);
extern DirEntry *RefreshTreeSafe(YtreePanel *panel, DirEntry *entry);

/* render_dir.c */
extern void DisplayTree(struct Volume *vol, WINDOW *win, int start_entry_no, int hilight_no, BOOL is_active);
extern void PrintDirEntry(struct Volume *vol, WINDOW *win, int entry_no, int y, unsigned char hilight, BOOL is_active);
extern void SetDirMode(int new_mode);
extern int GetDirMode(void);
extern void RotateDirMode(void);

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

/* filewin.c */
extern void BuildFileEntryList(YtreePanel *panel);
extern void DisplayFileWindow(YtreePanel *panel, DirEntry *dir_entry);
extern int HandleFileWindow(DirEntry *dir_entry);

/* render_file.c */
extern void SetFileMode(int new_file_mode);
extern int GetFileMode(void);
extern void RotateFileMode(void);
extern int GetMaxColumn(void);
extern void SetFileRenderingMetrics(unsigned max_filename, unsigned max_linkname, unsigned max_userview);
extern void SetRenderSortOrder(BOOL reverse);
extern void DisplayFiles(YtreePanel *panel, DirEntry *de_ptr, int start_file_no, int hilight_no, int start_x, WINDOW *win);
extern void PrintFileEntry(YtreePanel *panel, int entry_no, int y, int x, unsigned char hilight, int start_x, WINDOW *win);

/* hex.c has moved to cmd/hex.c and prototypes to ytree_cmd.h */

/* input.c */
extern int Getch(void);
extern void HitReturnToContinue(void);
extern int InputChoice(char *msg, char *term);
extern int UI_AskConflict(const char *src_path, const char *dst_path, int *mode_flags);

/* prompt.c */
extern int UI_ReadString(const char *prompt, char *buffer, int max_len, int history_type);

/* Legacy Macro for compatibility - Ignores y, x, and callback args for now */
#define InputString(s,y,x,cp,len,term,hst) UI_ReadString("Input:", s, len, hst)
#define InputStringEx(s,y,x,cp,dw,len,term,hst,cb) UI_ReadString("Input:", s, len, hst)

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

/* ui_nav.c */
extern void Nav_MoveDown(int *cursor, int *offset, int total_items, int page_height, int step);
extern void Nav_MoveUp(int *cursor, int *offset);
extern void Nav_PageDown(int *cursor, int *offset, int total_items, int page_height);
extern void Nav_PageUp(int *cursor, int *offset, int page_height);
extern void Nav_Home(int *cursor, int *offset);
extern void Nav_End(int *cursor, int *offset, int total_items, int page_height);

/* vol_menu.c */
extern int SelectLoadedVolume(void);

/* view_internal.c */
extern int InternalView(char *file_path);

/* view_preview.c */
extern void RenderFilePreview(WINDOW *win, char *filename, long *line_offset_ptr, int col_offset);
extern void RenderArchivePreview(WINDOW *win, char *archive_path, char *internal_path, long *line_offset_ptr);

#endif /* YTREE_UI_H */