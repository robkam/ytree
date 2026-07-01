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
BOOL AppStateSetPanelVolumeFileStateList(
    YtreeNovaPanel *panel, PanelVolumeFileState *volume_file_state);
BOOL AppStateCommitPanelFileSelection(YtreeNovaPanel *panel,
                                      const char *dir_path,
                                      const char *file_name);
BOOL AppStateCommitPanelTreeSelection(YtreeNovaPanel *panel,
                                      int current_dir_entry);
BOOL AppStateCommitPanelTreeViewportTopPath(YtreeNovaPanel *panel, int slot,
                                            const char *top_path);
BOOL AppStateCommitPanelTreeViewportTopPaths(YtreeNovaPanel *panel,
                                             const YtreeNovaPanel *source);
BOOL AppStateCommitPanelVolumeTreeViewportSnapshot(
    PanelVolumeFileState *state, unsigned int panel_generation,
    unsigned int volume_generation, BOOL has_selected_dir_path,
    const char *selected_dir_path, BOOL has_top_dir_path,
    const char *top_dir_path);
BOOL AppStateCommitPanelVolumeFileSnapshot(
    PanelVolumeFileState *state, int start_file, int file_cursor_pos,
    unsigned int panel_generation, unsigned int volume_generation,
    ViewFocus focus, BOOL big_file_view, const char *file_dir_path,
    const char *file_selection_dir_path, const char *file_selection_name);
BOOL AppStateCommitPanelFileViewport(YtreeNovaPanel *panel, int start_file,
                                     int file_cursor_pos);
BOOL AppStateCommitPanelFileAnchor(YtreeNovaPanel *panel,
                                   DirEntry *file_dir_entry);
BOOL AppStateCommitPanelTreeViewport(YtreeNovaPanel *panel, int disp_begin_pos,
                                     int cursor_pos);

#endif /* YTNOVA_APPSTATE_PANEL_H */
