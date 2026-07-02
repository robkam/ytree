/***************************************************************************
 *
 * ytnova_appstate_layout.h
 * Layout transition commits for AppState boundaries.
 *
 ***************************************************************************/

#ifndef YTNOVA_APPSTATE_LAYOUT_H
#define YTNOVA_APPSTATE_LAYOUT_H

#include "ytnova_defs.h"

typedef struct {
  int dir_x;
  int dir_y;
  int dir_w;
  int dir_h;
  int small_file_x;
  int small_file_y;
  int small_file_w;
  int small_file_h;
  int big_file_x;
  int big_file_y;
  int big_file_w;
  int big_file_h;
} YtreeNovaPanelWindowGeometry;

BOOL AppStateCommitSplitScreenLayout(ViewContext *ctx, BOOL is_split_screen);
BOOL AppStateCommitTerminalGeometryCache(ViewContext *ctx, int terminal_lines,
                                         int terminal_cols);
BOOL AppStateCommitLayoutGeometry(ViewContext *ctx,
                                  const YtreeNovaLayout *layout);
BOOL AppStateCommitPanelWindowGeometry(
    YtreeNovaPanel *panel, const YtreeNovaPanelWindowGeometry *geometry);
BOOL AppStateCommitFixedColumnWidth(ViewContext *ctx, int fixed_col_width);
BOOL AppStateCommitSmallWindowBypass(ViewContext *ctx, BOOL bypass_small_window);
BOOL AppStateCommitFullLineHighlight(ViewContext *ctx, BOOL highlight_full_line);

#endif /* YTNOVA_APPSTATE_LAYOUT_H */
