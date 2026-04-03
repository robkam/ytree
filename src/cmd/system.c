/***************************************************************************
 *
 * src/cmd/system.c
 * System Call
 *
 ***************************************************************************/

#include "ytree_cmd.h"
#include "ytree_fs.h"
#include <stdlib.h>

/* SystemCall and QuerySystemCall moved to UI layer */

/* Prototypes for functions defined in this file */
int SilentSystemCallEx(ViewContext *ctx, const char *command_line, BOOL enable_clock, Statistic *s);
int SilentSystemCall(ViewContext *ctx, const char *command_line, Statistic *s);

int SilentSystemCallEx(ViewContext *ctx, const char *command_line, BOOL enable_clock, Statistic *s) {
  int result;

  /* Hier ist die einzige Stelle, in der Kommandos aufgerufen werden! */

  if (ctx->hook_suspend_clock)
    ctx->hook_suspend_clock(ctx);

  result = system(command_line);

  /* Restore terminal settings. If enable_clock is TRUE, InitClock will
     implicitly call refresh() and restore the curses display later.
     If enable_clock is FALSE, the caller (QuerySystemCall) is responsible
     for the display/pause. */

  if (enable_clock && ctx->hook_init_clock)
    ctx->hook_init_clock(ctx); /* Re-initializes timer AND calls refresh/restores
                                * curses mode
                                */

  (void)GetAvailBytes(&s->disk_space, s);
  return (result);
}

int SilentSystemCall(ViewContext *ctx, const char *command_line, Statistic *s) {
  return (SilentSystemCallEx(ctx, command_line, TRUE, s));
}
