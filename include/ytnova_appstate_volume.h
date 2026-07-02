/***************************************************************************
 *
 * ytnova_appstate_volume.h
 * Volume AppState transition commits.
 *
 ***************************************************************************/

#ifndef YTNOVA_APPSTATE_VOLUME_H
#define YTNOVA_APPSTATE_VOLUME_H

#include "ytnova_defs.h"

BOOL AppStateCommitVolumeGeneration(struct Volume *volume);
BOOL AppStateCommitVolumeDirEntryList(struct Volume *volume,
                                      DirEntryList *dir_entry_list,
                                      size_t capacity, int total_dirs);
BOOL AppStateReleaseVolumeDirEntryList(struct Volume *volume);

#endif /* YTNOVA_APPSTATE_VOLUME_H */
