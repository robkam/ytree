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

BOOL AppStateCommitVolumeDirEntryList(struct Volume *volume,
                                      DirEntryList *dir_entry_list,
                                      size_t capacity, int total_dirs) {
  if (!AppStateValidatedOwnerField("volume.dir_tree"))
    return FALSE;
  if (!volume)
    return FALSE;

  volume->dir_entry_list = dir_entry_list;
  volume->dir_entry_list_capacity = capacity;
  volume->total_dirs = total_dirs;
  return TRUE;
}

BOOL AppStateReleaseVolumeDirEntryList(struct Volume *volume) {
  if (!AppStateValidatedOwnerField("volume.dir_tree"))
    return FALSE;
  if (!volume)
    return FALSE;

  free(volume->dir_entry_list);
  volume->dir_entry_list = NULL;
  volume->dir_entry_list_capacity = 0;
  volume->total_dirs = 0;
  return TRUE;
}
