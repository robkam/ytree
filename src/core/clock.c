/***************************************************************************
 *
 * src/core/clock.c
 * Clock Module
 *
 ***************************************************************************/


#include "ytree.h"



void InitClock()
{

#ifdef CLOCK_SUPPORT
  /*
   * Signal-based timer eliminated for safety.
   * Clock updates are now handled in the main event loop (input.c).
   */
  print_time = TRUE;
#endif
}


void ClockHandler(int sig)
{
   (void)sig; /* Unused now */
#ifdef CLOCK_SUPPORT

   char strtm[23];
   time_t HORA;
   struct tm *hora;

   if(COLS > 50 && print_time)
   {
      time(&HORA);
      hora = localtime(&HORA);
      *strtm = '\0';

      strftime(strtm, sizeof(strtm), "%d-%m-%Y %H:%M:%S", hora);

      werase(time_window);
#ifdef COLOR_SUPPORT
      wattrset(time_window, COLOR_PAIR(CPAIR_MENU));
#endif
      mvwaddstr(time_window, 0, 0, strtm);
#ifdef COLOR_SUPPORT
      wattrset(time_window, A_NORMAL);
#endif
      wrefresh(time_window);
   }
   /* Recursive signal call removed */
#endif
}

void SuspendClock(void)
{
#ifdef CLOCK_SUPPORT
  /* No hardware timer to pause anymore */
#endif
  return;
}