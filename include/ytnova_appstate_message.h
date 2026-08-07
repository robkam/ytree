/***************************************************************************
 *
 * ytnova_appstate_message.h
 * Message-state transition commits for AppState boundaries.
 *
 ***************************************************************************/

#ifndef YTNOVA_APPSTATE_MESSAGE_H
#define YTNOVA_APPSTATE_MESSAGE_H

#include "ytnova_defs.h"

BOOL AppStateCommitStatusLineError(ViewContext *ctx, const char *message);
BOOL AppStateClearStatusLineError(ViewContext *ctx);
BOOL AppStateCommitStatusLineNotice(ViewContext *ctx, const char *message);
BOOL AppStateClearStatusLineNotice(ViewContext *ctx);

#endif /* YTNOVA_APPSTATE_MESSAGE_H */
