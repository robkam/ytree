/***************************************************************************
 *
 * src/ui/appstate_render.c
 * Render transition commits and projection helpers for AppState boundaries.
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

void AppStateClampRenderFileViewport(unsigned int file_count,
                                     int *render_start_ptr,
                                     int *render_cursor_ptr) {
  int render_start;
  int render_cursor;

  if (!render_start_ptr || !render_cursor_ptr)
    return;

  render_start = *render_start_ptr;
  render_cursor = *render_cursor_ptr;
  if (file_count > 0) {
    if (render_start < 0)
      render_start = 0;
    if ((unsigned int)render_start >= file_count)
      render_start = (int)file_count - 1;
    if (render_cursor < 0)
      render_cursor = 0;
    if ((unsigned int)(render_start + render_cursor) >= file_count) {
      render_cursor = (int)file_count - 1 - render_start;
      if (render_cursor < 0)
        render_cursor = 0;
    }
  } else {
    render_start = 0;
    render_cursor = 0;
  }

  *render_start_ptr = render_start;
  *render_cursor_ptr = render_cursor;
}

int AppStateResolveRenderFileHighlight(unsigned int file_count,
                                       int render_start, int render_cursor) {
  if (file_count == 0)
    return -1;

  return render_start + render_cursor;
}
