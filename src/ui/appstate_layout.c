/***************************************************************************
 *
 * src/ui/appstate_layout.c
 * Layout transition commits for AppState boundaries.
 *
 ***************************************************************************/

#include "ytnova_appstate_actions.h"
#include "ytnova_appstate_layout.h"

BOOL AppStateCommitSplitScreenLayout(ViewContext *ctx, BOOL is_split_screen) {
  if (!AppStateValidatedOwnerField("ctx.layout"))
    return FALSE;
  if (!ctx)
    return FALSE;

  ctx->is_split_screen = is_split_screen ? TRUE : FALSE;
  return TRUE;
}

BOOL AppStateCommitTerminalGeometryCache(ViewContext *ctx, int terminal_lines,
                                         int terminal_cols) {
  if (!AppStateValidatedOwnerField("ctx.layout"))
    return FALSE;
  if (!ctx)
    return FALSE;

  ctx->cached_lines = terminal_lines;
  ctx->cached_cols = terminal_cols;
  return TRUE;
}

BOOL AppStateCommitLayoutGeometry(ViewContext *ctx,
                                  const YtreeNovaLayout *layout) {
  if (!AppStateValidatedOwnerField("ctx.layout"))
    return FALSE;
  if (!ctx || !layout)
    return FALSE;

  ctx->layout = *layout;
  return TRUE;
}

BOOL AppStateCommitFixedColumnWidth(ViewContext *ctx, int fixed_col_width) {
  if (!AppStateValidatedOwnerField("ctx.layout"))
    return FALSE;
  if (!ctx)
    return FALSE;

  ctx->fixed_col_width = fixed_col_width;
  return TRUE;
}

BOOL AppStateCommitSmallWindowBypass(ViewContext *ctx,
                                     BOOL bypass_small_window) {
  if (!AppStateValidatedOwnerField("ctx.layout"))
    return FALSE;
  if (!ctx)
    return FALSE;

  ctx->bypass_small_window = bypass_small_window ? TRUE : FALSE;
  return TRUE;
}

BOOL AppStateCommitFullLineHighlight(ViewContext *ctx,
                                     BOOL highlight_full_line) {
  if (!AppStateValidatedOwnerField("ctx.layout"))
    return FALSE;
  if (!ctx)
    return FALSE;

  ctx->highlight_full_line = highlight_full_line ? TRUE : FALSE;
  return TRUE;
}
