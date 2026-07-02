#ifndef YTNOVA_APPSTATE_MODAL_H
#define YTNOVA_APPSTATE_MODAL_H

#include "ytnova_defs.h"

BOOL AppStateCommitPreviewMode(ViewContext *ctx, BOOL preview_mode);
BOOL AppStateCommitPreviewReturn(ViewContext *ctx, YtreeNovaPanel *panel,
                                 ViewFocus focus);
BOOL AppStateCommitPreviewEntryFocus(ViewContext *ctx, ViewFocus focus);
BOOL AppStateCommitHistoryViewport(ViewContext *ctx, int disp_begin_pos,
                                   int cursor_pos);
BOOL AppStateCommitCompletionViewport(ViewContext *ctx, int disp_begin_pos,
                                      int cursor_pos);

#endif /* YTNOVA_APPSTATE_MODAL_H */
