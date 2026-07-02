/***************************************************************************
 *
 * ytnova_appstate_visibility.h
 * Visibility-filter transition commits for AppState boundaries.
 *
 ***************************************************************************/

#ifndef YTNOVA_APPSTATE_VISIBILITY_H
#define YTNOVA_APPSTATE_VISIBILITY_H

#include "ytnova_defs.h"

BOOL AppStateCommitPanelVisibilityFilter(YtreeNovaPanel *panel,
                                         BOOL hide_dot_files);
BOOL AppStateSeedPanelVisibilityFilter(YtreeNovaPanel *panel,
                                       BOOL hide_dot_files);
BOOL AppStateCommitDirEntryTaggedFilter(DirEntry *dir_entry, BOOL tagged_only);
BOOL AppStateCommitDirEntryGlobalFilter(DirEntry *dir_entry, BOOL global_filter,
                                        BOOL all_volumes);

#endif /* YTNOVA_APPSTATE_VISIBILITY_H */
