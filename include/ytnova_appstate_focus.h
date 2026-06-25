/***************************************************************************
 *
 * ytnova_appstate_focus.h
 * Focus transition commits for AppState compatibility boundaries.
 *
 ***************************************************************************/

#ifndef YTNOVA_APPSTATE_FOCUS_H
#define YTNOVA_APPSTATE_FOCUS_H

#include "ytnova_defs.h"

BOOL AppStateCommitPanelFocus(ViewContext *ctx, YtreeNovaPanel *panel,
                              ViewFocus focus);
BOOL AppStateMirrorActivePanelFocus(ViewContext *ctx);

#endif /* YTNOVA_APPSTATE_FOCUS_H */
