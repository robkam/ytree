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

BOOL AppStateSetPanelFileWindowHandle(ViewContext *ctx, YtreeNovaPanel *panel,
                                      BOOL big_file_window) {
  if (!AppStateValidatedOwnerField("ctx.window_handles"))
    return FALSE;
  if (!ctx || !panel)
    return FALSE;

  panel->pan_file_window = big_file_window ? panel->pan_big_file_window
                                           : panel->pan_small_file_window;
  if (panel == ctx->active)
    ctx->ctx_file_window = panel->pan_file_window;
  return TRUE;
}

BOOL AppStateSetPreviewWindowHandle(ViewContext *ctx, WINDOW *preview_window) {
  if (!AppStateValidatedOwnerField("ctx.window_handles"))
    return FALSE;
  if (!ctx)
    return FALSE;

  ctx->ctx_preview_window = preview_window;
  return TRUE;
}
