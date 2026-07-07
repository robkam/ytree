/***************************************************************************
 *
 * ytnova_appstate_render.h
 * Render transition commits and projection helpers for AppState boundaries.
 *
 ***************************************************************************/

#ifndef YTNOVA_APPSTATE_RENDER_H
#define YTNOVA_APPSTATE_RENDER_H

#include "ytnova_defs.h"

BOOL AppStateCommitResizeRequest(ViewContext *ctx, BOOL resize_request);
BOOL AppStateMarkResizeRequest(ViewContext *ctx);
BOOL AppStateClearResizeRequest(ViewContext *ctx);
void AppStateClampRenderFileViewport(unsigned int file_count,
                                     int *render_start_ptr,
                                     int *render_cursor_ptr);
int AppStateResolveRenderFileHighlight(unsigned int file_count,
                                       int render_start, int render_cursor);

#endif /* YTNOVA_APPSTATE_RENDER_H */
