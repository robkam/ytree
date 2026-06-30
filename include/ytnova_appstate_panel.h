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
BOOL AppStateCommitPanelVolume(YtreeNovaPanel *panel, struct Volume *vol);
BOOL AppStateCommitPanelFileSelection(YtreeNovaPanel *panel,
                                      const char *dir_path,
                                      const char *file_name);
BOOL AppStateCommitPanelFileViewport(YtreeNovaPanel *panel, int start_file,
                                     int file_cursor_pos);
BOOL AppStateCommitPanelTreeViewport(YtreeNovaPanel *panel, int disp_begin_pos,
                                     int cursor_pos);

#endif /* YTNOVA_APPSTATE_PANEL_H */
