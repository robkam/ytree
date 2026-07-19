/***************************************************************************
 *
 * src/cmd/config.c
 * Core init callback provider (cmd-side)
 *
 ***************************************************************************/

#include "ytnova_cmd.h"

static int CoreInit_ReadGroupEntries(void) { return ReadGroupEntries(); }

static int CoreInit_ReadPasswdEntries(void) { return ReadPasswdEntries(); }

static int CoreInit_ReadHistory(ViewContext *ctx, const char *filename) {
  return ReadHistory(ctx, filename);
}

void CoreInitOps_RegisterCmdConfig(CoreInitOps *ops) {
  if (ops == NULL)
    return;

  ops->read_group_entries = CoreInit_ReadGroupEntries;
  ops->read_passwd_entries = CoreInit_ReadPasswdEntries;
  ops->read_history = CoreInit_ReadHistory;
}
