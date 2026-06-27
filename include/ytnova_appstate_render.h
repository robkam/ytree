/***************************************************************************
 *
 * ytnova_appstate_render.h
 * Render-invalidation transition commits for AppState boundaries.
 *
 ***************************************************************************/

#ifndef YTNOVA_APPSTATE_RENDER_H
#define YTNOVA_APPSTATE_RENDER_H

#include "ytnova_defs.h"

BOOL AppStateCommitResizeRequest(ViewContext *ctx, BOOL resize_request);
BOOL AppStateMarkResizeRequest(ViewContext *ctx);
BOOL AppStateClearResizeRequest(ViewContext *ctx);

#endif /* YTNOVA_APPSTATE_RENDER_H */
