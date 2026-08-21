/***************************************************************************
 *
 * src/cmd/config.c
 * Core init callback provider (cmd-side)
 *
 ***************************************************************************/

#include "ytnova_cmd.h"

static int CoreInit_ReadHistory(ViewContext *ctx, const char *filename) {
  return ReadHistory(ctx, filename);
}

void CoreInitOps_RegisterCmdConfig(CoreInitOps *ops) {
  if (ops == NULL)
    return;

  ops->read_history = CoreInit_ReadHistory;
}
