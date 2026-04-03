/***************************************************************************
 *
 * src/fs/watcher_ports.c
 * Core watcher callback provider (fs-side)
 *
 ***************************************************************************/

#include "watcher.h"

static void CoreStorage_WatcherInit(ViewContext *ctx) { Watcher_Init(ctx); }

void CoreWatcherOps_Register(ViewContext *ctx) {
  if (ctx == NULL)
    return;
  ctx->core_storage_ops.watcher_init = CoreStorage_WatcherInit;
}
