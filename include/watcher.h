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

void Watcher_Init(void);
void Watcher_SetDir(const char *path);
int Watcher_GetFD(void);
BOOL Watcher_ProcessEvents(void);
void Watcher_Close(void);

#endif /* WATCHER_H */