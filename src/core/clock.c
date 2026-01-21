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

  struct itimerval value, ovalue;
  print_time = TRUE;

  if (getitimer(ITIMER_REAL, &value)!= 0) {
      snprintf(message, MESSAGE_LENGTH, "getitimer() failed: %s", strerror(errno));
      ERROR_MSG(message);
  }
  value.it_interval.tv_sec = CLOCK_INTERVAL;
  value.it_value.tv_sec = CLOCK_INTERVAL;
  value.it_interval.tv_usec = 0;

  if (setitimer(ITIMER_REAL, &value, &ovalue)!= 0) {
      snprintf(message, MESSAGE_LENGTH, "setitimer() failed: %s", strerror(errno));
      ERROR_MSG(message);
  }
  ClockHandler(0);
#endif
}




void ClockHandler(int sig)
{
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
   }
   signal(SIGALRM,  ClockHandler );
#endif
}

void SuspendClock(void)
{
#ifdef CLOCK_SUPPORT
  struct itimerval value, ovalue;
  value.it_interval.tv_sec = 0;
  value.it_value.tv_sec = 0;
  value.it_interval.tv_usec = 0;
  setitimer(ITIMER_REAL, &value, &ovalue);
  signal(SIGALRM, SIG_IGN);
#endif
  return;
  }
