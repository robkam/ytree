/***************************************************************************
 *
 * src/ui/appstate_focus.c
 * Focus transition commits for AppState panel-local boundaries.
 *
 ***************************************************************************/

#define NO_YTNOVA_MACROS

#include "ytnova_appstate_focus.h"

ViewFocus AppStateResolveActivePanelFocus(const ViewContext *ctx) {
  if (ctx && ctx->active &&
      (ctx->active->saved_focus == FOCUS_TREE ||
       ctx->active->saved_focus == FOCUS_FILE))
    return ctx->active->saved_focus;
  return FOCUS_TREE;
}

BOOL AppStateCommitPanelFocus(const ViewContext *ctx, YtreeNovaPanel *panel,
                              ViewFocus focus) {
  if (!ctx || !panel)
    return FALSE;
  if (focus != FOCUS_TREE && focus != FOCUS_FILE)
    return FALSE;

  panel->saved_focus = focus;
  return TRUE;
}

BOOL AppStateCommitPanelFileShape(YtreeNovaPanel *panel, BOOL big_file_view) {
  if (!panel)
    return FALSE;

  panel->saved_big_file_view = big_file_view ? TRUE : FALSE;
  return TRUE;
}

BOOL AppStateCommitDirEntryFileShape(DirEntry *dir_entry, BOOL big_file_view) {
  if (!dir_entry)
    return FALSE;

  dir_entry->big_window = big_file_view ? TRUE : FALSE;
  return TRUE;
}

BOOL AppStateCommitVolumeFocusMirror(struct Volume *volume, ViewFocus focus) {
  if (!volume)
    return FALSE;
  if (focus != FOCUS_TREE && focus != FOCUS_FILE)
    return FALSE;

  volume->saved_focus = focus;
  return TRUE;
}

BOOL AppStateMirrorActivePanelFocus(const ViewContext *ctx) {
  if (!ctx || !ctx->active)
    return FALSE;
  return AppStateCommitPanelFocus(ctx, ctx->active,
                                  AppStateResolveActivePanelFocus(ctx));
}
