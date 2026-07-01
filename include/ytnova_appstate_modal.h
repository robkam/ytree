#ifndef YTNOVA_APPSTATE_MODAL_H
#define YTNOVA_APPSTATE_MODAL_H

#include "ytnova_defs.h"

BOOL AppStateCommitPreviewMode(ViewContext *ctx, BOOL preview_mode);
BOOL AppStateCommitPreviewReturn(ViewContext *ctx, YtreeNovaPanel *panel,
                                 ViewFocus focus);
BOOL AppStateCommitPreviewEntryFocus(ViewContext *ctx, ViewFocus focus);

#endif /* YTNOVA_APPSTATE_MODAL_H */
