/***************************************************************************
 *
 * ytree_key_record.h
 * Optional key-stream recording helpers
 *
 ***************************************************************************/
#ifndef YTREE_KEY_RECORD_H
#define YTREE_KEY_RECORD_H

#include "ytree_defs.h"

int KeyRecord_Start(ViewContext *ctx, const char *path);
void KeyRecord_Stop(ViewContext *ctx);
void KeyRecord_Log(ViewContext *ctx, int ch);
void KeyRecord_Pause(ViewContext *ctx, BOOL pause);
int KeyRecord_BeginPrompt(ViewContext *ctx);

#endif
