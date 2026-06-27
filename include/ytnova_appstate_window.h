/***************************************************************************
 *
 * ytnova_appstate_window.h
 * Window-handle transition commits for AppState boundaries.
 *
 ***************************************************************************/

#ifndef YTNOVA_APPSTATE_WINDOW_H
#define YTNOVA_APPSTATE_WINDOW_H

#include "ytnova_defs.h"

BOOL AppStateSyncActiveWindowHandles(ViewContext *ctx);
BOOL AppStateSetPanelFileWindowHandle(ViewContext *ctx, YtreeNovaPanel *panel,
                                      BOOL big_file_window);
BOOL AppStateSetPreviewWindowHandle(ViewContext *ctx, WINDOW *preview_window);
BOOL AppStateSetBorderWindowHandle(ViewContext *ctx, WINDOW *window);
BOOL AppStateSetPathWindowHandle(ViewContext *ctx, WINDOW *window);
BOOL AppStateSetErrorWindowHandle(ViewContext *ctx, WINDOW *window);
BOOL AppStateSetTimeWindowHandle(ViewContext *ctx, WINDOW *window);
BOOL AppStateSetHistoryWindowHandle(ViewContext *ctx, WINDOW *window);
BOOL AppStateSetMatchesWindowHandle(ViewContext *ctx, WINDOW *window);
BOOL AppStateSetMenuWindowHandle(ViewContext *ctx, WINDOW *window);
BOOL AppStateSetF2WindowHandle(ViewContext *ctx, WINDOW *window);

#endif /* YTNOVA_APPSTATE_WINDOW_H */
