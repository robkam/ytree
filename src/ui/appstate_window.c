/***************************************************************************
 *
 * src/ui/appstate_window.c
 * Window-handle transition commits for AppState boundaries.
 *
 ***************************************************************************/

#include "ytnova_appstate_actions.h"
#include "ytnova_appstate_window.h"

BOOL AppStateSyncActiveWindowHandles(ViewContext *ctx) {
  if (!AppStateValidatedOwnerField("ctx.window_handles"))
    return FALSE;
  if (!ctx || !ctx->active)
    return FALSE;

  ctx->ctx_dir_window = ctx->active->pan_dir_window;
  ctx->ctx_small_file_window = ctx->active->pan_small_file_window;
  ctx->ctx_big_file_window = ctx->active->pan_big_file_window;
  ctx->ctx_file_window = ctx->active->pan_file_window;
  return TRUE;
}
