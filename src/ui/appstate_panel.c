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
