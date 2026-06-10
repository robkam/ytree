/***************************************************************************
 *
 * include/interactions_panel_paths.h
 * Shared panel path helpers for UI interaction modules.
 *
 ***************************************************************************/

#ifndef INTERACTIONS_PANEL_PATHS_H
#define INTERACTIONS_PANEL_PATHS_H

#include "ytnova_ui.h"

extern YtreeNovaPanel *UI_GetInactivePanel(ViewContext *ctx);
extern int UI_GetPanelSelectedDirPath(ViewContext *ctx, YtreeNovaPanel *panel,
                                      char *out_path);
extern int UI_GetPanelLoggedRootPath(YtreeNovaPanel *panel, char *out_path);
extern int UI_GetPanelSelectedFilePath(ViewContext *ctx, YtreeNovaPanel *panel,
                                       char *out_path);

#endif
