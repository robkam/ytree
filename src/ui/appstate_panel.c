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
