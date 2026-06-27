/***************************************************************************
 *
 * src/ui/appstate_render.c
 * Render-invalidation transition commits for AppState boundaries.
 *
 ***************************************************************************/

#include "ytnova_appstate_actions.h"
#include "ytnova_appstate_render.h"

BOOL AppStateCommitResizeRequest(ViewContext *ctx, BOOL resize_request) {
  if (!AppStateValidatedOwnerField("ctx.render_dirty_flags"))
    return FALSE;
  if (!ctx)
    return FALSE;

  ctx->resize_request = resize_request ? TRUE : FALSE;
  return TRUE;
}

BOOL AppStateMarkResizeRequest(ViewContext *ctx) {
  return AppStateCommitResizeRequest(ctx, TRUE);
}

BOOL AppStateClearResizeRequest(ViewContext *ctx) {
  return AppStateCommitResizeRequest(ctx, FALSE);
}
