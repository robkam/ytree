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

#endif /* YTNOVA_APPSTATE_WINDOW_H */
