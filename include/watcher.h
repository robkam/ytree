/***************************************************************************
 *
 * watcher.h
 * File System Watcher Interface
 *
 * Provides a platform-independent abstraction for file system monitoring.
 * Currently implements Linux inotify backend.
 *
 ***************************************************************************/

#ifndef WATCHER_H
#define WATCHER_H

#include "ytree_defs.h"

void Watcher_Init(ViewContext *ctx);
void Watcher_SetDir(ViewContext *ctx, const char *path);
int Watcher_GetFD(const ViewContext *ctx);
BOOL Watcher_ProcessEvents(ViewContext *ctx);
void Watcher_Close(ViewContext *ctx);

#endif /* WATCHER_H */
