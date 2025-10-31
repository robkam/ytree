/***************************************************************************
 *
 * System Call
 *
 ***************************************************************************/


#include "ytree.h"




int SystemCall(char *command_line)
{
  int result;

#ifndef XCURSES
  endwin();
#endif
  result = SilentSystemCall( command_line );
  
  (void) GetAvailBytes( &statistic.disk_space );
  refresh();
  return( result );
}


int QuerySystemCall(char *command_line)
{
  int result;

#ifndef XCURSES
  endwin();
#endif
  result = SilentSystemCall( command_line );
  HitReturnToContinue();
  (void) GetAvailBytes( &statistic.disk_space );
  refresh();

  return( result );
}



extern struct itimerval value, ovalue;

int SilentSystemCall(char *command_line)
{
  return(SilentSystemCallEx(command_line, TRUE));
}

int SilentSystemCallEx(char *command_line, BOOL enable_clock)
{
  int result;
#ifdef XCURSES
  char *xterm=NULL;
#endif

  /* Hier ist die einzige Stelle, in der Kommandos aufgerufen werden! */

#if defined( __NeXT__ )
  nl();
#endif /* linux */

    SuspendClock();

#ifdef XCURSES
  if( ( xterm = malloc( strlen( command_line ) + 10 ) ) == NULL ) {
    ERROR_MSG( "Malloc Failed*ABORT" );
    exit( 1 );
  }
  sprintf(xterm, "xterm -e %s &", command_line);
  result = system( xterm );
  free(xterm);
#else
  result = system( command_line );
#endif

#ifndef XCURSES
  leaveok(stdscr, TRUE);
  curs_set(0);
#if defined( __NeXT__ )
  cbreak();
  nonl();
  noecho();
  clearok( stdscr, TRUE );
#endif /* linux */ 
#endif /* XCURSES */
  if(enable_clock)
    InitClock();
  (void) GetAvailBytes( &statistic.disk_space );
  return( result ); 
}


