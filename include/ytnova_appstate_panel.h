/***************************************************************************
 *
 * ytnova_appstate_panel.h
 * Panel-local AppState transition commits.
 *
 ***************************************************************************/

#ifndef YTNOVA_APPSTATE_PANEL_H
#define YTNOVA_APPSTATE_PANEL_H

#include "ytnova_defs.h"

BOOL AppStateCommitPanelGeneration(YtreeNovaPanel *panel);
BOOL AppStateRestorePanelGeneration(YtreeNovaPanel *panel,
                                    unsigned int panel_generation);

#endif /* YTNOVA_APPSTATE_PANEL_H */
