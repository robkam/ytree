/***************************************************************************
 *
 * src/ui/appstate_mode.c
 * View-mode transition commits for AppState boundaries.
 *
 ***************************************************************************/

#include "ytnova_appstate_actions.h"
#include "ytnova_appstate_mode.h"

BOOL AppStateCommitViewMode(ViewContext *ctx, int view_mode) {
  if (!AppStateValidatedOwnerField("ctx.view_mode"))
    return FALSE;
  if (!ctx)
    return FALSE;
  if (view_mode < DISK_MODE || view_mode >= MAX_MODES)
    return FALSE;

  ctx->view_mode = view_mode;
  return TRUE;
}

BOOL AppStateCommitDirectoryDisplayMode(ViewContext *ctx, int dir_mode) {
  if (!AppStateValidatedOwnerField("ctx.dir_mode"))
    return FALSE;
  if (!ctx)
    return FALSE;
  if (dir_mode < MODE_1 || dir_mode > MODE_4)
    return FALSE;

  ctx->dir_mode = dir_mode;
  return TRUE;
}
