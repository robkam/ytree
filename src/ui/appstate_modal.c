#include "ytnova_appstate_modal.h"

#include "ytnova_appstate_actions.h"

BOOL AppStateCommitPreviewMode(ViewContext *ctx, BOOL preview_mode) {
  if (!AppStateValidatedOwnerField("ctx.modal_state"))
    return FALSE;
  if (!ctx)
    return FALSE;

  ctx->preview_mode = preview_mode ? TRUE : FALSE;
  return TRUE;
}

BOOL AppStateCommitPreviewReturn(ViewContext *ctx, YtreeNovaPanel *panel,
                                 ViewFocus focus) {
  if (!AppStateValidatedOwnerField("ctx.modal_state"))
    return FALSE;
  if (!ctx)
    return FALSE;

  ctx->preview_return_panel = panel;
  ctx->preview_return_focus = focus;
  return TRUE;
}

BOOL AppStateCommitPreviewEntryFocus(ViewContext *ctx, ViewFocus focus) {
  if (!AppStateValidatedOwnerField("ctx.modal_state"))
    return FALSE;
  if (!ctx)
    return FALSE;

  ctx->preview_entry_focus = focus;
  return TRUE;
}
