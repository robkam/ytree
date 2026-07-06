/***************************************************************************
 *
 * src/ui/appstate_focus.c
 * Focus transition commits for AppState compatibility boundaries.
 *
 ***************************************************************************/

#define NO_YTNOVA_MACROS

#include "ytnova_appstate_actions.h"
#include "ytnova_appstate_focus.h"

ViewFocus AppStateResolveActivePanelFocus(const ViewContext *ctx) {
  if (ctx && ctx->active &&
      (ctx->active->saved_focus == FOCUS_TREE ||
       ctx->active->saved_focus == FOCUS_FILE))
    return ctx->active->saved_focus;
  if (ctx && (ctx->focused_window == FOCUS_TREE ||
              ctx->focused_window == FOCUS_FILE))
    return ctx->focused_window;
  return FOCUS_TREE;
}

BOOL AppStateCommitPanelFocus(ViewContext *ctx, YtreeNovaPanel *panel,
                              ViewFocus focus) {
  if (!AppStateValidatedCompatibilityShim("shim.focused-window-session-flag"))
    return FALSE;
  if (!ctx || !panel)
    return FALSE;
  if (focus != FOCUS_TREE && focus != FOCUS_FILE)
    return FALSE;

  panel->saved_focus = focus;
  if (ctx->active == panel)
    ctx->focused_window = focus;
  return TRUE;
}

BOOL AppStateCommitPanelFileShape(YtreeNovaPanel *panel, BOOL big_file_view) {
  if (!AppStateValidatedCompatibilityShim("shim.focused-window-session-flag"))
    return FALSE;
  if (!panel)
    return FALSE;

  panel->saved_big_file_view = big_file_view ? TRUE : FALSE;
  return TRUE;
}

BOOL AppStateCommitDirEntryFileShape(DirEntry *dir_entry, BOOL big_file_view) {
  if (!AppStateValidatedCompatibilityShim("shim.focused-window-session-flag"))
    return FALSE;
  if (!dir_entry)
    return FALSE;

  dir_entry->big_window = big_file_view ? TRUE : FALSE;
  return TRUE;
}

BOOL AppStateCommitVolumeFocusMirror(struct Volume *volume, ViewFocus focus) {
  if (!AppStateValidatedCompatibilityShim("shim.focused-window-session-flag"))
    return FALSE;
  if (!volume)
    return FALSE;
  if (focus != FOCUS_TREE && focus != FOCUS_FILE)
    return FALSE;

  volume->saved_focus = focus;
  return TRUE;
}

BOOL AppStateMirrorActivePanelFocus(ViewContext *ctx) {
  if (!ctx || !ctx->active)
    return FALSE;
  return AppStateCommitPanelFocus(ctx, ctx->active, ctx->active->saved_focus);
}
