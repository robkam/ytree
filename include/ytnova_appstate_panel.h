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
BOOL AppStateCommitPanelTreeSelection(YtreeNovaPanel *panel,
                                      int current_dir_entry);
BOOL AppStateCommitPanelTreeViewportTopPath(YtreeNovaPanel *panel, int slot,
                                            const char *top_path);
BOOL AppStateCommitPanelTreeViewportTopPaths(YtreeNovaPanel *panel,
                                             const YtreeNovaPanel *source);
BOOL AppStateCommitPanelFileViewport(YtreeNovaPanel *panel, int start_file,
                                     int file_cursor_pos);
BOOL AppStateCommitPanelFileAnchor(YtreeNovaPanel *panel,
                                   DirEntry *file_dir_entry);
BOOL AppStateCommitPanelTreeViewport(YtreeNovaPanel *panel, int disp_begin_pos,
                                     int cursor_pos);

#endif /* YTNOVA_APPSTATE_PANEL_H */
