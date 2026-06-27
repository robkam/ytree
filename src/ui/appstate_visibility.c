/***************************************************************************
 *
 * src/ui/appstate_visibility.c
 * Visibility-filter transition commits for AppState boundaries.
 *
 ***************************************************************************/

#define NO_YTNOVA_MACROS

#include "ytnova_appstate_actions.h"
#include "ytnova_appstate_volume.h"
#include "ytnova_appstate_visibility.h"

BOOL AppStateCommitPanelVisibilityFilter(YtreeNovaPanel *panel,
                                         BOOL hide_dot_files) {
  if (!AppStateValidatedGenerationDomain("state.visibility-filter.panel-volume"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.panel_generation"))
    return FALSE;
  if (!AppStateValidatedOwnerField("volume.volume_generation"))
    return FALSE;
  if (!panel || !panel->vol)
    return FALSE;

  panel->hide_dot_files = hide_dot_files ? TRUE : FALSE;
  panel->panel_generation++;
  if (!AppStateCommitVolumeGeneration(panel->vol))
    return FALSE;
  return TRUE;
}

BOOL AppStateSeedPanelVisibilityFilter(YtreeNovaPanel *panel,
                                       BOOL hide_dot_files) {
  if (!AppStateValidatedGenerationDomain("state.visibility-filter.panel-volume"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.panel_generation"))
    return FALSE;
  if (!panel)
    return FALSE;

  panel->hide_dot_files = hide_dot_files ? TRUE : FALSE;
  return TRUE;
}
