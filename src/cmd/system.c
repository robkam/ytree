/***************************************************************************
 *
 * src/cmd/system.c
 * System Call
 *
 ***************************************************************************/

#include "ytree_cmd.h"
#include "ytree_defs.h"
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

extern void SuspendClock(void);
extern void InitClock(void);
extern int GetAvailBytes(Statistic *disk_space, Statistic *s);

/* SystemCall and QuerySystemCall moved to UI layer */

/* Prototypes for functions defined in this file */
int SilentSystemCallEx(char *command_line, BOOL enable_clock, Statistic *s);
int SilentSystemCall(char *command_line, Statistic *s);

int SilentSystemCallEx(char *command_line, BOOL enable_clock, Statistic *s) {
  int result;

  /* Hier ist die einzige Stelle, in der Kommandos aufgerufen werden! */

  SuspendClock();

  result = system(command_line);

  /* Restore terminal settings. If enable_clock is TRUE, InitClock will
     implicitly call refresh() and restore the curses display later.
     If enable_clock is FALSE, the caller (QuerySystemCall) is responsible
     for the display/pause. */

  if (enable_clock)
    InitClock(); /* Re-initializes timer AND calls refresh/restores curses mode
                  */

  (void)GetAvailBytes(&s->disk_space, s);
  return (result);
}

int SilentSystemCall(char *command_line, Statistic *s) {
  return (SilentSystemCallEx(command_line, TRUE, s));
}
