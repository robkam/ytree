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

#endif /* YTNOVA_APPSTATE_VISIBILITY_H */
