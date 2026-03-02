/***************************************************************************
 *
 * src/cmd/system.c
 * System Call
 *
 ***************************************************************************/

#include "ytree.h"
#include "ytree_cmd.h"
#include "ytree_defs.h"
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

/* SystemCall and QuerySystemCall moved to UI layer */

/* Prototypes for functions defined in this file */
int SilentSystemCallEx(ViewContext *ctx, char *command_line, BOOL enable_clock, Statistic *s);
int SilentSystemCall(ViewContext *ctx, char *command_line, Statistic *s);

int SilentSystemCallEx(ViewContext *ctx, char *command_line, BOOL enable_clock, Statistic *s) {
  int result;

  /* Hier ist die einzige Stelle, in der Kommandos aufgerufen werden! */

  SuspendClock(ctx);

  result = system(command_line);

  /* Restore terminal settings. If enable_clock is TRUE, InitClock will
     implicitly call refresh() and restore the curses display later.
     If enable_clock is FALSE, the caller (QuerySystemCall) is responsible
     for the display/pause. */

  if (enable_clock)
    InitClock(ctx); /* Re-initializes timer AND calls refresh/restores
                            * curses mode
                            */

  (void)GetAvailBytes(&s->disk_space, s);
  return (result);
}

int SilentSystemCall(ViewContext *ctx, char *command_line, Statistic *s) {
  return (SilentSystemCallEx(ctx, command_line, TRUE, s));
}
