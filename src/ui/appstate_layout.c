/***************************************************************************
 *
 * src/ui/appstate_layout.c
 * Layout transition commits for AppState boundaries.
 *
 ***************************************************************************/

#include "ytnova_appstate_actions.h"
#include "ytnova_appstate_layout.h"
#include "ytnova_appstate_panel.h"

BOOL AppStateCommitSplitScreenLayout(ViewContext *ctx, BOOL is_split_screen) {
  if (!AppStateValidatedGenerationDomain("reflow.layout.projection"))
    return FALSE;
  if (!AppStateValidatedOwnerField("ctx.layout"))
    return FALSE;
  if (!ctx)
    return FALSE;

  ctx->is_split_screen = is_split_screen ? TRUE : FALSE;
  return TRUE;
}

BOOL AppStateCommitTerminalGeometryCache(ViewContext *ctx, int terminal_lines,
                                         int terminal_cols) {
  if (!AppStateValidatedGenerationDomain("reflow.layout.projection"))
    return FALSE;
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
  if (!AppStateValidatedGenerationDomain("reflow.layout.projection"))
    return FALSE;
  if (!AppStateValidatedOwnerField("ctx.layout"))
    return FALSE;
  if (!ctx || !layout)
    return FALSE;

  ctx->layout = *layout;
  return TRUE;
}

BOOL AppStateCommitPanelWindowGeometry(
    YtreeNovaPanel *panel, const YtreeNovaPanelWindowGeometry *geometry) {
  if (!AppStateValidatedGenerationDomain("reflow.layout.projection"))
    return FALSE;
  if (!AppStateValidatedOwnerField("ctx.layout"))
    return FALSE;
  if (!panel || !geometry)
    return FALSE;

  panel->dir_x = geometry->dir_x;
  panel->dir_y = geometry->dir_y;
  panel->dir_w = geometry->dir_w;
  panel->dir_h = geometry->dir_h;
  panel->small_file_x = geometry->small_file_x;
  panel->small_file_y = geometry->small_file_y;
  panel->small_file_w = geometry->small_file_w;
  panel->small_file_h = geometry->small_file_h;
  panel->big_file_x = geometry->big_file_x;
  panel->big_file_y = geometry->big_file_y;
  panel->big_file_w = geometry->big_file_w;
  panel->big_file_h = geometry->big_file_h;
  panel->stats_x = geometry->stats_x;
  panel->stats_width = geometry->stats_width;
  return TRUE;
}

BOOL AppStateCommitFixedColumnWidth(ViewContext *ctx, int fixed_col_width) {
  if (!AppStateValidatedGenerationDomain("reflow.layout.projection"))
    return FALSE;
  if (!AppStateValidatedOwnerField("ctx.layout"))
    return FALSE;
  if (!ctx)
    return FALSE;

  if (ctx->active &&
      !AppStateCommitPanelFixedColumnWidth(ctx->active, fixed_col_width))
    return FALSE;
  ctx->fixed_col_width = fixed_col_width;
  return TRUE;
}

BOOL AppStateCommitSmallWindowBypass(ViewContext *ctx,
                                     BOOL bypass_small_window) {
  if (!AppStateValidatedGenerationDomain("reflow.layout.projection"))
    return FALSE;
  if (!AppStateValidatedOwnerField("ctx.layout"))
    return FALSE;
  if (!ctx)
    return FALSE;

  ctx->bypass_small_window = bypass_small_window ? TRUE : FALSE;
  return TRUE;
}

BOOL AppStateCommitFullLineHighlight(ViewContext *ctx,
                                     BOOL highlight_full_line) {
  if (!AppStateValidatedGenerationDomain("reflow.layout.projection"))
    return FALSE;
  if (!AppStateValidatedOwnerField("ctx.layout"))
    return FALSE;
  if (!ctx)
    return FALSE;

  ctx->highlight_full_line = highlight_full_line ? TRUE : FALSE;
  return TRUE;
}
