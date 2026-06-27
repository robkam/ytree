/***************************************************************************
 *
 * ytnova_appstate_layout.h
 * Layout transition commits for AppState boundaries.
 *
 ***************************************************************************/

#ifndef YTNOVA_APPSTATE_LAYOUT_H
#define YTNOVA_APPSTATE_LAYOUT_H

#include "ytnova_defs.h"

BOOL AppStateCommitSplitScreenLayout(ViewContext *ctx, BOOL is_split_screen);
BOOL AppStateCommitTerminalGeometryCache(ViewContext *ctx, int terminal_lines,
                                         int terminal_cols);

#endif /* YTNOVA_APPSTATE_LAYOUT_H */
