/***************************************************************************
 *
 * src/ui/appstate_panel.c
 * Panel-local transition commits for AppState boundaries.
 *
 ***************************************************************************/

#define NO_YTNOVA_MACROS

#include "ytnova_appstate_actions.h"
#include "ytnova_appstate_panel.h"

BOOL AppStateCommitPanelGeneration(YtreeNovaPanel *panel) {
  if (!AppStateValidatedOwnerField("panel.panel_generation"))
    return FALSE;
  if (!panel)
    return FALSE;

  panel->panel_generation++;
  return TRUE;
}

BOOL AppStateRestorePanelGeneration(YtreeNovaPanel *panel,
                                    unsigned int panel_generation) {
  if (!AppStateValidatedOwnerField("panel.panel_generation"))
    return FALSE;
  if (!panel)
    return FALSE;

  panel->panel_generation = panel_generation;
  return TRUE;
}

BOOL AppStateCommitPanelVolume(YtreeNovaPanel *panel, struct Volume *vol) {
  if (!AppStateValidatedOwnerField("panel.volume_key"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.panel_generation"))
    return FALSE;
  if (!panel)
    return FALSE;

  panel->vol = vol;
  panel->panel_generation++;
  return TRUE;
}

BOOL AppStateCommitPanelFileSelection(YtreeNovaPanel *panel,
                                      const char *dir_path,
                                      const char *file_name) {
  if (!AppStateValidatedOwnerField("panel.file_selection_key"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.panel_generation"))
    return FALSE;
  if (!panel)
    return FALSE;

  panel->file_selection_dir_path[0] = '\0';
  if (dir_path) {
    (void)snprintf(panel->file_selection_dir_path,
                   sizeof(panel->file_selection_dir_path), "%s", dir_path);
    panel->file_selection_dir_path[PATH_LENGTH] = '\0';
  }

  panel->file_selection_name[0] = '\0';
  if (file_name) {
    (void)snprintf(panel->file_selection_name,
                   sizeof(panel->file_selection_name), "%s", file_name);
    panel->file_selection_name[PATH_LENGTH] = '\0';
  }

  panel->panel_generation++;
  return TRUE;
}

BOOL AppStateCommitPanelTreeSelection(YtreeNovaPanel *panel,
                                      int current_dir_entry) {
  if (!AppStateValidatedOwnerField("panel.tree_selection_key"))
    return FALSE;
  if (!panel)
    return FALSE;

  panel->current_dir_entry = current_dir_entry;
  return TRUE;
}

BOOL AppStateCommitPanelFileViewport(YtreeNovaPanel *panel, int start_file,
                                     int file_cursor_pos) {
  if (!AppStateValidatedOwnerField("panel.file_viewport_origin"))
    return FALSE;
  if (!panel)
    return FALSE;

  panel->start_file = start_file;
  panel->file_cursor_pos = file_cursor_pos;
  return TRUE;
}

BOOL AppStateCommitPanelTreeViewport(YtreeNovaPanel *panel, int disp_begin_pos,
                                     int cursor_pos) {
  if (!AppStateValidatedOwnerField("panel.tree_viewport_origin"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.tree_cursor_pos"))
    return FALSE;
  if (!panel)
    return FALSE;

  panel->disp_begin_pos = disp_begin_pos;
  panel->cursor_pos = cursor_pos;
  return TRUE;
}
