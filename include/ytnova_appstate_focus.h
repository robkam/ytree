/***************************************************************************
 *
 * ytnova_appstate_focus.h
 * Focus transition commits for AppState compatibility boundaries.
 *
 ***************************************************************************/

#ifndef YTNOVA_APPSTATE_FOCUS_H
#define YTNOVA_APPSTATE_FOCUS_H

#include "ytnova_defs.h"

ViewFocus AppStateResolveActivePanelFocus(const ViewContext *ctx);
BOOL AppStateCommitPanelFocus(ViewContext *ctx, YtreeNovaPanel *panel,
                              ViewFocus focus);
BOOL AppStateCommitPanelFileShape(YtreeNovaPanel *panel, BOOL big_file_view);
BOOL AppStateCommitDirEntryFileShape(DirEntry *dir_entry, BOOL big_file_view);
BOOL AppStateCommitVolumeFocusMirror(struct Volume *volume, ViewFocus focus);
BOOL AppStateMirrorActivePanelFocus(ViewContext *ctx);

#endif /* YTNOVA_APPSTATE_FOCUS_H */
