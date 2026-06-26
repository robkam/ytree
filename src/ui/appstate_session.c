/***************************************************************************
 *
 * src/ui/appstate_session.c
 * Session-routing transition commits for AppState boundaries.
 *
 ***************************************************************************/

#define NO_YTNOVA_MACROS

#include "ytnova_appstate_actions.h"
#include "ytnova_appstate_session.h"

BOOL AppStateCommitActivePanel(ViewContext *ctx, YtreeNovaPanel *panel) {
  if (!AppStateValidatedOwnerField("ctx.active"))
    return FALSE;
  if (!ctx || !panel)
    return FALSE;
  if (panel != ctx->left && panel != ctx->right)
    return FALSE;

  ctx->active = panel;
  return TRUE;
}
