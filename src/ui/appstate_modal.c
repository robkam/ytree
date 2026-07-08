#include "ytnova_appstate_modal.h"

#include "ytnova_appstate_actions.h"

BOOL AppStateCommitPreviewMode(ViewContext *ctx, BOOL preview_mode) {
  if (!AppStateValidatedGenerationDomain("target.modal-command.session"))
    return FALSE;
  if (!AppStateValidatedOwnerField("ctx.modal_state"))
    return FALSE;
  if (!ctx)
    return FALSE;

  ctx->preview_mode = preview_mode ? TRUE : FALSE;
  return TRUE;
}

BOOL AppStateCommitPreviewReturn(ViewContext *ctx, YtreeNovaPanel *panel,
                                 ViewFocus focus) {
  if (!AppStateValidatedGenerationDomain("target.modal-command.session"))
    return FALSE;
  if (!AppStateValidatedOwnerField("ctx.modal_state"))
    return FALSE;
  if (!ctx)
    return FALSE;

  ctx->preview_return_panel = panel;
  ctx->preview_return_focus = focus;
  return TRUE;
}

BOOL AppStateCommitPreviewEntryFocus(ViewContext *ctx, ViewFocus focus) {
  if (!AppStateValidatedGenerationDomain("target.modal-command.session"))
    return FALSE;
  if (!AppStateValidatedOwnerField("ctx.modal_state"))
    return FALSE;
  if (!ctx)
    return FALSE;

  ctx->preview_entry_focus = focus;
  return TRUE;
}

BOOL AppStateCommitHistoryViewport(ViewContext *ctx, int disp_begin_pos,
                                   int cursor_pos) {
  if (!AppStateValidatedGenerationDomain("target.modal-command.session"))
    return FALSE;
  if (!AppStateValidatedOwnerField("ctx.modal_state"))
    return FALSE;
  if (!ctx)
    return FALSE;

  ctx->disp_begin_pos = disp_begin_pos;
  ctx->cursor_pos = cursor_pos;
  return TRUE;
}

BOOL AppStateCommitCompletionViewport(ViewContext *ctx, int disp_begin_pos,
                                      int cursor_pos) {
  if (!AppStateValidatedGenerationDomain("target.modal-command.session"))
    return FALSE;
  if (!AppStateValidatedOwnerField("ctx.modal_state"))
    return FALSE;
  if (!ctx)
    return FALSE;

  ctx->tab_disp_begin_pos = disp_begin_pos;
  ctx->tab_cursor_pos = cursor_pos;
  return TRUE;
}
