/***************************************************************************
 *
 * src/cmd/system.c
 * System Call
 *
 ***************************************************************************/


#include "ytree.h"


/* Forward declaration for internal use */
extern struct itimerval value, ovalue;


int SystemCall(char *command_line, Statistic *s)
{
  int result;

  endwin(); /* Ensure terminal state is reset before external command */
  result = SilentSystemCall( command_line, s );

  (void) GetAvailBytes( &s->disk_space, s );
  /* Full screen redraw to fully restore the curses UI */
  clearok(stdscr, TRUE);
  refresh();
  return( result );
}


int QuerySystemCall(char *command_line, Statistic *s)
{
  int result;

  endwin(); /* 1. Save state / Exit curses mode */

  /* 2. Execute command (runs outside curses) */
  result = SilentSystemCallEx( command_line, FALSE, s );

  /* The external command has finished. We are still in the raw terminal. */

  HitReturnToContinue(); /* 3. Print message and wait for key in raw terminal */

  /* 4. Aggressive redraw/refresh to restore the curses UI completely */
  clearok(stdscr, TRUE);
  refresh();

  (void) GetAvailBytes( &s->disk_space, s );

  return( result );
}



int SilentSystemCall(char *command_line, Statistic *s)
{
  return(SilentSystemCallEx(command_line, TRUE, s));
}

int SilentSystemCallEx(char *command_line, BOOL enable_clock, Statistic *s)
{
  int result;

  /* Hier ist die einzige Stelle, in der Kommandos aufgerufen werden! */

  SuspendClock();

  result = system( command_line );

  /* Restore terminal settings. If enable_clock is TRUE, InitClock will
     implicitly call refresh() and restore the curses display later.
     If enable_clock is FALSE, the caller (QuerySystemCall) is responsible
     for the display/pause. */

  if(enable_clock)
    InitClock(); /* Re-initializes timer AND calls refresh/restores curses mode */

  (void) GetAvailBytes( &s->disk_space, s );
  return( result );
}
