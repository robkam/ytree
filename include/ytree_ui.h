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
extern void WbkgdSet(const ViewContext *ctx, WINDOW *w, chtype c);
extern void ParseColorString(const char *color_str, int *fg, int *bg);
extern void UpdateUIColor(const char *name, int fg, int bg);
extern void AddFileColorRule(ViewContext *ctx, const char *pattern, int fg,
                             int bg);
extern int GetFileTypeColor(const ViewContext *ctx, const FileEntry *fe_ptr);

#else
#define StartColors(ctx) ;
#define ReinitColorPairs(ctx) ;
#define WbkgdSet(ctx, a, b) ;
#endif

/* dirwin.c */
extern int HandleDirWindow(ViewContext *ctx, const DirEntry *start_dir_entry);
extern int KeyF2Get(ViewContext *ctx, YtreePanel *panel, char *path);
extern int RefreshDirWindow(ViewContext *ctx, YtreePanel *p);
extern void ToggleDotFiles(ViewContext *ctx, YtreePanel *p);
extern DirEntry *GetSelectedDirEntry(ViewContext *ctx, struct Volume *vol);
extern DirEntry *GetPanelDirEntry(YtreePanel *p);
extern void BuildDirEntryList(ViewContext *ctx, struct Volume *vol,
                              int *index_ptr);
extern void FreeDirEntryList(ViewContext *ctx);
extern void FreeVolumeCache(struct Volume *vol);
extern DirEntry *RefreshTreeSafe(ViewContext *ctx, YtreePanel *p,
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
extern void DisplayDirHelp(ViewContext *ctx, const DirEntry *dir_entry);
extern void DisplayFileHelp(ViewContext *ctx, const DirEntry *dir_entry);
extern void DisplayMenu(ViewContext *ctx);
extern void MapF2Window(ViewContext *ctx);
extern void RefreshWindow(WINDOW *win);
extern void SwitchToBigFileWindow(ViewContext *ctx);
extern void SwitchToSmallFileWindow(ViewContext *ctx);
extern void UnmapF2Window(ViewContext *ctx);
extern void DisplayHeaderPath(ViewContext *ctx, const char *path);
extern void RenderInactivePanel(ViewContext *ctx, YtreePanel *panel);
extern void RefreshView(ViewContext *ctx, DirEntry *dir_entry);
extern void DisplayPreviewHelp(ViewContext *ctx);
extern void DisplayHistoryHelp(ViewContext *ctx);

/* display_utils.c */
extern int AddStr(char *str);
extern int BuildUserFileEntry(FileEntry *fe_ptr, int filename_width,
                              int linkname_width, char *template, int linelen,
                              char *line);
extern char *CTime(time_t f_time, char *buffer);
extern char *CutFilename(char *dest, const char *src, unsigned int max_len);
extern char *CutName(char *dest, const char *src, unsigned int max_len);
extern char *CutPathname(char *dest, const char *src, unsigned int max_len);
extern char *FormFilename(char *dest, char *src, unsigned int max_len);
extern char *GetAttributes(unsigned short mode, char *buffer);
extern void GetMaxYX(WINDOW *win, int *height, int *width);
extern int GetVisualUserFileEntryLength(int max_visual_filename_len,
                                        int max_visual_linkname_len,
                                        char *template);
extern int MvAddStr(int y, int x, char *str);
extern int MvWAddStr(WINDOW *win, int y, int x, char *str);
extern void Print(WINDOW *, int, int, char *, int);
extern void PrintLine(WINDOW *win, int y, int x, const char *line, int len);
extern void PrintMenuOptions(WINDOW *, int, int, char *, int, int);
extern void PrintOptions(WINDOW *, int, int, char *);
extern void PrintSpecialString(WINDOW *win, int y, int x, char *str, int color);
extern int WAddStr(WINDOW *win, char *str);
extern int WAttrAddStr(WINDOW *win, int attr, char *str);

/* error.c */
extern void AboutBox(ViewContext *ctx);
extern void Error(char *msg, char *module, int line);
extern void Message(char *msg);
extern void Notice(char *msg);
extern void UnmapNoticeWindow(ViewContext *ctx);
extern void Warning(char *msg);
extern int UI_Error(ViewContext *ctx, const char *module, int line,
                    const char *fmt, ...);
extern int UI_Warning(ViewContext *ctx, const char *fmt, ...);
extern int UI_Message(ViewContext *ctx, const char *fmt, ...);
extern int UI_Notice(ViewContext *ctx, const char *fmt, ...);
extern void UI_Beep(ViewContext *ctx, BOOL critical);
extern void UI_ShowStatusLineError(ViewContext *ctx, const char *fmt, ...);
extern void UI_RenderStatusLineError(ViewContext *ctx);
extern void UI_ClearStatusLineError(ViewContext *ctx);

/* filewin.c / ctrl_file.c / ctrl_file_ops.c */
extern void BuildFileEntryList(ViewContext *ctx, YtreePanel *panel);
extern void DisplayFileWindow(ViewContext *ctx, YtreePanel *panel,
                              const DirEntry *dir_entry);
extern int HandleFileWindow(ViewContext *ctx, DirEntry *dir_entry);
extern DirEntry *RefreshFileView(ViewContext *ctx, DirEntry *dir_entry);
extern BOOL handle_tag_file_action(ViewContext *ctx, int action,
                                   DirEntry *dir_entry, int *unput_char_ptr,
                                   BOOL *need_dsp_help_ptr, int start_x,
                                   Statistic *s, BOOL *maybe_change_x_step_ptr);

/* render_file.c */
extern void SetPanelFileMode(ViewContext *ctx, YtreePanel *p,
                             int new_file_mode);
extern int GetPanelFileMode(const YtreePanel *p);
extern void RotatePanelFileMode(ViewContext *ctx, YtreePanel *p);
extern int GetPanelMaxColumn(const YtreePanel *p);
extern void SetFileRenderingMetrics(YtreePanel *p, unsigned max_filename,
                                    unsigned max_linkname,
                                    unsigned max_userview);
extern void SetRenderSortOrder(YtreePanel *p, BOOL reverse);
extern void DisplayFiles(ViewContext *ctx, YtreePanel *panel,
                         const DirEntry *de_ptr,
                         int start_file_no, int hilight_no, int start_x,
                         WINDOW *win);
extern void PrintFileEntry(ViewContext *ctx, YtreePanel *panel, int entry_no,
                           int y, int x, unsigned char hilight, int start_x,
                           WINDOW *win);

/* hex.c has moved to cmd/hex.c and prototypes to ytree_cmd.h */

/* key_engine.c */
extern int Getch(ViewContext *ctx);
extern void HitReturnToContinue(void);
extern int InputChoice(ViewContext *ctx, const char *msg, const char *term);
extern int InputChoiceLiteral(ViewContext *ctx, const char *msg,
                              const char *term);
extern int UI_AskConflict(ViewContext *ctx, const char *src_path,
                          const char *dst_path, int *mode_flags);

/* input_line.c */
extern int UI_ReadString(ViewContext *ctx, YtreePanel *panel,
                         const char *prompt, char *buffer, int max_len,
                         int history_type);
extern int UI_ReadStringWithHelp(ViewContext *ctx, YtreePanel *panel,
                                 const char *prompt, char *buffer, int max_len,
                                 int history_type, const char *hints_override,
                                 int (*help_callback)(ViewContext *, void *),
                                 void *help_data);

extern BOOL KeyPressed(void);
extern BOOL EscapeKeyPressed(void);
extern char *StrLeft(const char *str, size_t visible_count);
extern char *StrRight(const char *str, size_t visible_count);
extern int StrVisualLength(const char *str);
extern BOOL IsViKeysEnabled(const ViewContext *ctx);
extern int ViKey(int ch);
extern int NormalizeViKey(const ViewContext *ctx, int ch);
extern int VisualPositionToBytePosition(const char *str, int visual_pos);
extern int WGetch(ViewContext *ctx, WINDOW *win);
extern YtreeAction GetKeyAction(const ViewContext *ctx, int ch);
extern int GetEventOrKey(ViewContext *ctx);

/* stats.c */
extern void DisplayAvailBytes(ViewContext *ctx, const Statistic *s);
extern void DisplayDirParameter(ViewContext *ctx, DirEntry *de);
extern void DisplayDirStatistic(ViewContext *ctx, const DirEntry *de,
                                const char *title, const Statistic *s);
extern void DisplayDirTagged(ViewContext *ctx, const DirEntry *de,
                             const Statistic *s);
extern void DisplayDiskName(ViewContext *ctx, const Statistic *s);
extern void DisplayDiskStatistic(ViewContext *ctx, const Statistic *s);
extern void DisplayDiskTagged(ViewContext *ctx, const Statistic *s);
extern void DisplayFileParameter(ViewContext *ctx, FileEntry *fe);
extern void DisplayFileStatistic(ViewContext *ctx, const FileEntry *fe,
                                 const Statistic *s);
extern void UpdateStatsPanel(ViewContext *ctx, DirEntry *dir_entry,
                             const Statistic *s);
extern void DisplayFilter(ViewContext *ctx, const Statistic *s);
extern void DisplayGlobalFileParameter(ViewContext *ctx, FileEntry *fe);
extern void RecalculateSysStats(ViewContext *ctx, Statistic *s);

/* ctrl_dir.c / dir_tags.c */
extern void HandleShowAll(ViewContext *ctx, BOOL tagged_only, BOOL all_volumes,
                          DirEntry *dir_entry, BOOL *need_dsp_help, int *ch,
                          YtreePanel *p);
extern BOOL HandleDirTagActions(ViewContext *ctx, int action,
                                DirEntry **dir_entry_ptr, BOOL *need_dsp_help,
                                int *ch);

/* dir_nav.c */
extern void DirNav_Movedown(ViewContext *ctx, DirEntry **dir_entry,
                            YtreePanel *p);
extern void DirNav_Moveup(ViewContext *ctx, DirEntry **dir_entry,
                          YtreePanel *p);
extern void DirNav_Movenpage(ViewContext *ctx, DirEntry **dir_entry,
                             YtreePanel *p);
extern void DirNav_Moveppage(ViewContext *ctx, DirEntry **dir_entry,
                             YtreePanel *p);
extern void DirNav_MoveEnd(ViewContext *ctx, DirEntry **dir_entry,
                           YtreePanel *p);
extern void DirNav_MoveHome(ViewContext *ctx, DirEntry **dir_entry,
                            YtreePanel *p);

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
extern int InternalView(ViewContext *ctx, char *file_path,
                        const ViewerGeometry *geom);

/* view_preview.c */
extern void RenderFilePreview(ViewContext *ctx, WINDOW *win, char *filename,
                              long *line_offset_ptr, int col_offset);
extern void RenderArchivePreview(ViewContext *ctx, WINDOW *win,
                                 const char *archive_path,
                                 const char *internal_path,
                                 long *line_offset_ptr);

/* quit.c */
extern void Quit(ViewContext *ctx);
extern void QuitTo(ViewContext *ctx, DirEntry *dir_entry);

/* interactions.c */
/* Date-change scopes (only fields POSIX allows us to set). */
#define DATE_SCOPE_ACCESS 1
#define DATE_SCOPE_MODIFY 2

extern int UI_PromptAttributeAction(ViewContext *ctx, BOOL tagged,
                                    BOOL allow_date);
extern int UI_ParseModeInput(const char *input, char *out_mode,
                             char *preview_mode);
extern int UI_GetDateChangeSpec(ViewContext *ctx, time_t *new_time,
                                int *scope_mask);
extern int ChangeFileModus(ViewContext *ctx, FileEntry *fe_ptr);
extern int ChangeDirModus(ViewContext *ctx, DirEntry *de_ptr);
extern int ChangeFileDate(ViewContext *ctx, FileEntry *fe_ptr);
extern int ChangeDirDate(ViewContext *ctx, DirEntry *de_ptr);
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
extern int UI_BuildArchivePayloadFromPaths(const char *const *source_paths,
                                           size_t source_count,
                                           BOOL recursive_directories,
                                           ArchivePayload *payload);
extern int UI_GatherArchivePayload(ViewContext *ctx, DirEntry *selected_dir,
                                   FileEntry *selected_file,
                                   ArchivePayload *payload);
extern void UI_FreeArchivePayload(ArchivePayload *payload);
extern int UI_CreateArchiveFromPayload(ViewContext *ctx,
                                       const ArchivePayload *payload);
extern int UI_SelectCompareMenuChoice(ViewContext *ctx,
                                      CompareMenuChoice *choice);
extern int UI_BuildFileCompareRequest(ViewContext *ctx, FileEntry *source_file,
                                      CompareRequest *request);
extern int UI_BuildDirectoryCompareRequest(ViewContext *ctx,
                                           DirEntry *source_dir,
                                           CompareFlowType flow_type,
                                           CompareRequest *request);
extern int UI_BuildDirectoryCompareLaunchRequest(ViewContext *ctx,
                                                 DirEntry *source_dir,
                                                 CompareFlowType flow_type,
                                                 CompareRequest *request);
extern const char *UI_CompareFlowTypeName(CompareFlowType flow_type);
extern const char *UI_CompareBasisName(CompareBasis basis);
extern const char *UI_CompareTagResultName(CompareTagResult tag_result);
extern const char *UI_GetCompareHelperCommand(const ViewContext *ctx,
                                              CompareFlowType flow_type);
extern int UI_ConflictResolverWrapper(ViewContext *ctx, const char *src_path,
                                      const char *dst_path, int *mode_flags);
extern void BuildFileEntryList(ViewContext *ctx, YtreePanel *panel);

/* file_tags.c */
extern void FileTags_WalkTaggedFiles(ViewContext *ctx, int start_file,
                                     int cursor_pos,
                                     int (*fkt)(ViewContext *, FileEntry *,
                                                WalkingPackage *),
                                     WalkingPackage *walking_package);
extern BOOL FileTags_IsMatchingTaggedFiles(ViewContext *ctx);
extern int FileTags_UI_DeleteTaggedFiles(ViewContext *ctx, int max_disp_files,
                                         Statistic *s);
extern void FileTags_SilentWalkTaggedFiles(
    ViewContext *ctx,
    int (*fkt)(ViewContext *, FileEntry *, WalkingPackage *, Statistic *),
    WalkingPackage *walking_package);
extern void FileTags_SilentTagWalkTaggedFiles(
    ViewContext *ctx,
    int (*fkt)(ViewContext *, FileEntry *, WalkingPackage *, Statistic *),
    WalkingPackage *walking_package);
extern void FileTags_HandleInvertTags(ViewContext *ctx, DirEntry *dir_entry,
                                      Statistic *s);

/* file_list.c */
extern void FileList_RemoveFileEntry(ViewContext *ctx, int entry_no);
extern void FileList_ChangeFileEntry(ViewContext *ctx);

/* progress.c */
extern void Progress_Start(ViewContext *ctx, const char *operation,
                           const char *source_path, const char *dest_path,
                           long long bytes_total, unsigned int items_total);
extern BOOL Progress_Update(ViewContext *ctx, long long bytes_done,
                            unsigned int items_done);
extern void Progress_Finish(ViewContext *ctx);
extern void Progress_Render(ViewContext *ctx);

#endif /* YTREE_UI_H */
