/***************************************************************************
 *
 * src/core/clock.c
 * Clock Module
 *
 ***************************************************************************/

#include "ytree.h"

void InitClock(ViewContext *ctx) {

#ifdef CLOCK_SUPPORT
  /*
   * Signal-based timer eliminated for safety.
   * Clock updates are now handled in the main event loop (input.c).
   */
  ctx->clock_print_time = TRUE;
#endif
}

void ClockHandler(ViewContext *ctx, int sig) {
  (void)sig; /* Unused now */
#ifdef CLOCK_SUPPORT

  if (!ctx)
    return;

  char strtm[23];
  time_t HORA;
  struct tm *hora;

  if (COLS > 50 && ctx->clock_print_time) {
    time(&HORA);
    hora = localtime(&HORA);
    *strtm = '\0';

    strftime(strtm, sizeof(strtm), "%d-%m-%Y %H:%M:%S", hora);

    werase(ctx->ctx_time_window);
#ifdef COLOR_SUPPORT
    wattrset(ctx->ctx_time_window, COLOR_PAIR(CPAIR_MENU));
#endif
    mvwaddstr(ctx->ctx_time_window, 0, 0, strtm);
#ifdef COLOR_SUPPORT
    wattrset(ctx->ctx_time_window, A_NORMAL);
#endif
    wnoutrefresh(ctx->ctx_time_window);
  }
  /* Recursive signal call removed */
#endif
}

void SuspendClock(ViewContext *ctx) {
  (void)ctx;
#ifdef CLOCK_SUPPORT
  /* No hardware timer to pause anymore */
#endif
  return;
}