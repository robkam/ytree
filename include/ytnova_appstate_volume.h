/***************************************************************************
 *
 * ytnova_appstate_volume.h
 * Volume AppState transition commits.
 *
 ***************************************************************************/

#ifndef YTNOVA_APPSTATE_VOLUME_H
#define YTNOVA_APPSTATE_VOLUME_H

#include "ytnova_defs.h"

BOOL AppStateCommitDirEntryTotalPayload(DirEntry *dir_entry,
                                        unsigned int total_files,
                                        long long total_bytes);
BOOL AppStateCommitDirEntryMatchingPayload(DirEntry *dir_entry,
                                           unsigned int matching_files,
                                           long long matching_bytes);
BOOL AppStateResetDirEntryPayloadCache(DirEntry *dir_entry);
BOOL AppStateCommitDirEntryLogFlag(DirEntry *dir_entry, BOOL log_flag);
BOOL AppStateCommitDirEntryLoggedState(DirEntry *dir_entry, BOOL not_scanned,
                                       BOOL unlogged_flag);
BOOL AppStateCommitVolumeGeneration(struct Volume *volume);
BOOL AppStateCommitVolumeDirEntryList(struct Volume *volume,
                                      DirEntryList *dir_entry_list,
                                      size_t capacity, int total_dirs);
BOOL AppStateReleaseVolumeDirEntryList(struct Volume *volume);

#endif /* YTNOVA_APPSTATE_VOLUME_H */
