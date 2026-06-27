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
