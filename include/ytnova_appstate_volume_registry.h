/***************************************************************************
 *
 * ytnova_appstate_volume_registry.h
 * Volume-registry transition commits for AppState boundaries.
 *
 ***************************************************************************/

#ifndef YTNOVA_APPSTATE_VOLUME_REGISTRY_H
#define YTNOVA_APPSTATE_VOLUME_REGISTRY_H

#include "ytnova_defs.h"

BOOL AppStateRegisterVolume(ViewContext *ctx, struct Volume *volume);
BOOL AppStateUnregisterVolume(ViewContext *ctx, struct Volume *volume);
BOOL AppStateClearVolumeRegistry(ViewContext *ctx);

#endif /* YTNOVA_APPSTATE_VOLUME_REGISTRY_H */
