/***************************************************************************
 *
 * src/ui/appstate_volume.c
 * Volume AppState transition commit helpers.
 *
 ***************************************************************************/

#include "ytnova_appstate_actions.h"
#include "ytnova_appstate_volume.h"

BOOL AppStateCommitDirEntryTotalPayload(DirEntry *dir_entry,
                                        unsigned int total_files,
                                        long long total_bytes) {
  if (!AppStateValidatedOwnerField("volume.payload_cache"))
    return FALSE;
  if (!dir_entry)
    return FALSE;

  dir_entry->total_files = total_files;
  dir_entry->total_bytes = total_bytes;
  return TRUE;
}

BOOL AppStateCommitDirEntryMatchingPayload(DirEntry *dir_entry,
                                           unsigned int matching_files,
                                           long long matching_bytes) {
  if (!AppStateValidatedOwnerField("volume.payload_cache"))
    return FALSE;
  if (!dir_entry)
    return FALSE;

  dir_entry->matching_files = matching_files;
  dir_entry->matching_bytes = matching_bytes;
  return TRUE;
}

BOOL AppStateCommitDirEntryAccessDenied(DirEntry *dir_entry,
                                        BOOL access_denied) {
  if (!AppStateValidatedOwnerField("volume.payload_cache"))
    return FALSE;
  if (!dir_entry)
    return FALSE;

  dir_entry->access_denied = access_denied ? TRUE : FALSE;
  return TRUE;
}

BOOL AppStateResetDirEntryPayloadCache(DirEntry *dir_entry) {
  if (!AppStateValidatedOwnerField("volume.payload_cache"))
    return FALSE;
  if (!dir_entry)
    return FALSE;

  dir_entry->file = NULL;
  dir_entry->total_bytes = 0L;
  dir_entry->matching_bytes = 0L;
  dir_entry->tagged_bytes = 0L;
  dir_entry->total_files = 0;
  dir_entry->matching_files = 0;
  dir_entry->tagged_files = 0;
  dir_entry->access_denied = FALSE;
  dir_entry->log_flag = FALSE;
  return TRUE;
}

BOOL AppStateCommitDirEntryLogFlag(DirEntry *dir_entry, BOOL log_flag) {
  if (!AppStateValidatedOwnerField("volume.payload_cache"))
    return FALSE;
  if (!dir_entry)
    return FALSE;

  dir_entry->log_flag = log_flag ? TRUE : FALSE;
  return TRUE;
}

BOOL AppStateCommitDirEntryLoggedState(DirEntry *dir_entry, BOOL not_scanned,
                                       BOOL unlogged_flag) {
  if (!AppStateValidatedOwnerField("volume.logged_state"))
    return FALSE;
  if (!dir_entry)
    return FALSE;

  dir_entry->not_scanned = not_scanned ? TRUE : FALSE;
  dir_entry->unlogged_flag = unlogged_flag ? TRUE : FALSE;
  return TRUE;
}

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
