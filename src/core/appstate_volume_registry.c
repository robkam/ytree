/***************************************************************************
 *
 * src/core/appstate_volume_registry.c
 * Volume-registry transition commits for AppState boundaries.
 *
 ***************************************************************************/

#include "ytnova_appstate_actions.h"
#include "ytnova_appstate_volume_registry.h"

BOOL AppStateRegisterVolume(ViewContext *ctx, struct Volume *volume) {
  if (!AppStateValidatedOwnerField("ctx.volumes_head"))
    return FALSE;
  if (!AppStateValidatedGenerationDomain("lifecycle.volume.registry"))
    return FALSE;
  if (!ctx || !volume)
    return FALSE;

  HASH_ADD_INT(ctx->volumes_head, id, volume);
  return TRUE;
}

BOOL AppStateUnregisterVolume(ViewContext *ctx, struct Volume *volume) {
  if (!AppStateValidatedOwnerField("ctx.volumes_head"))
    return FALSE;
  if (!AppStateValidatedGenerationDomain("lifecycle.volume.registry"))
    return FALSE;
  if (!ctx || !volume)
    return FALSE;

  HASH_DEL(ctx->volumes_head, volume);
  return TRUE;
}

BOOL AppStateClearVolumeRegistry(ViewContext *ctx) {
  if (!AppStateValidatedOwnerField("ctx.volumes_head"))
    return FALSE;
  if (!AppStateValidatedGenerationDomain("lifecycle.volume.registry"))
    return FALSE;
  if (!ctx)
    return FALSE;

  ctx->volumes_head = NULL;
  return TRUE;
}
