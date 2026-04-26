/***************************************************************************
 *
 * include/interactions_panel_paths.h
 * Shared panel path helpers for UI interaction modules.
 *
 ***************************************************************************/

#ifndef INTERACTIONS_PANEL_PATHS_H
#define INTERACTIONS_PANEL_PATHS_H

#include "ytree_ui.h"

extern YtreePanel *UI_GetInactivePanel(ViewContext *ctx);
extern int UI_GetPanelSelectedDirPath(ViewContext *ctx, YtreePanel *panel,
                                      char *out_path);
extern int UI_GetPanelLoggedRootPath(YtreePanel *panel, char *out_path);
extern int UI_GetPanelSelectedFilePath(ViewContext *ctx, YtreePanel *panel,
                                       char *out_path);

#endif
