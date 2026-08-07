/***************************************************************************
 *
 * src/ui/appstate_message.c
 * Message-state transition commits for AppState boundaries.
 *
 ***************************************************************************/

#include "ytnova_appstate_actions.h"
#include "ytnova_appstate_message.h"

BOOL AppStateCommitStatusLineError(ViewContext *ctx, const char *message) {
  if (!AppStateValidatedOwnerField("ctx.message_state"))
    return FALSE;
  if (!AppStateValidatedGenerationDomain("target.modal-command.session"))
    return FALSE;
  if (!ctx || !message)
    return FALSE;

  (void)snprintf(ctx->status_line_error_text,
                 sizeof(ctx->status_line_error_text), "%s", message);
  ctx->status_line_error_pending = TRUE;
  return TRUE;
}

BOOL AppStateClearStatusLineError(ViewContext *ctx) {
  if (!AppStateValidatedOwnerField("ctx.message_state"))
    return FALSE;
  if (!AppStateValidatedGenerationDomain("target.modal-command.session"))
    return FALSE;
  if (!ctx)
    return FALSE;

  ctx->status_line_error_pending = FALSE;
  ctx->status_line_error_text[0] = '\0';
  return TRUE;
}

BOOL AppStateCommitStatusLineNotice(ViewContext *ctx, const char *message) {
  if (!AppStateValidatedOwnerField("ctx.message_state"))
    return FALSE;
  if (!AppStateValidatedGenerationDomain("target.modal-command.session"))
    return FALSE;
  if (!ctx || !message)
    return FALSE;

  (void)snprintf(ctx->status_line_notice_text,
                 sizeof(ctx->status_line_notice_text), "%s", message);
  ctx->status_line_notice_pending = TRUE;
  return TRUE;
}

BOOL AppStateClearStatusLineNotice(ViewContext *ctx) {
  if (!AppStateValidatedOwnerField("ctx.message_state"))
    return FALSE;
  if (!AppStateValidatedGenerationDomain("target.modal-command.session"))
    return FALSE;
  if (!ctx)
    return FALSE;

  ctx->status_line_notice_pending = FALSE;
  ctx->status_line_notice_text[0] = '\0';
  return TRUE;
}
