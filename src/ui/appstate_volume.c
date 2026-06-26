/***************************************************************************
 *
 * src/ui/appstate_volume.c
 * Volume AppState transition commit helpers.
 *
 ***************************************************************************/

#include "ytnova_appstate_actions.h"
#include "ytnova_appstate_volume.h"

BOOL AppStateCommitVolumeGeneration(struct Volume *volume) {
  if (!AppStateValidatedOwnerField("volume.volume_generation"))
    return FALSE;
  if (!volume)
    return FALSE;

  volume->volume_generation++;
  return TRUE;
}
