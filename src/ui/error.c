/***************************************************************************
 *
 * src/ui/error.c
 * Output of error messages
 *
 ***************************************************************************/

#include "patchlev.h"
#include "ytree.h"

static void MapErrorWindow(ViewContext *ctx, char *header);
static void MapNoticeWindow(ViewContext *ctx, char *header);
static void UnmapErrorWindow(ViewContext *ctx);
static void PrintErrorLine(ViewContext *ctx, int y, char *str);
static void DisplayMessage(ViewContext *ctx, char *msg);
static int PrintMessage(ViewContext *ctx, char *msg, BOOL do_beep);

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  char buffer[MESSAGE_LENGTH + 1];
  va_list ap;

  va_start(ap, fmt);
  (void)vsnprintf(buffer, sizeof(buffer), fmt, ap);
  va_end(ap);

  MapErrorWindow(ctx, "E R R O R");
  return PrintMessage(ctx, buffer, TRUE);
}

int UI_Notice(ViewContext *ctx, const char *fmt, ...) {
  char buffer[MESSAGE_LENGTH + 1];
  va_list ap;

  va_start(ap, fmt);
  (void)vsnprintf(buffer, sizeof(buffer), fmt, ap);
  va_end(ap);

  MapNoticeWindow(ctx, "N O T I C E");
  DisplayMessage(ctx, buffer);
  RefreshWindow(ctx->ctx_error_window);
  refresh();
  return 0;
}

int UI_Warning(ViewContext *ctx, const char *fmt, ...) {
  char buffer[MESSAGE_LENGTH + 1];
  va_list ap;

  va_start(ap, fmt);
  (void)vsnprintf(buffer, sizeof(buffer), fmt, ap);
  va_end(ap);

  MapErrorWindow(ctx, "W A R N I N G");
  return PrintMessage(ctx, buffer, FALSE);
}

void AboutBox(ViewContext *ctx) {
  static char version[80];

  (void)snprintf(version, sizeof(version),
#ifdef WITH_UTF8
                 "ytree (UTF8) Version %s %s*",
#else
                 "ytree Version %s %s*",
#endif
                 VERSION, VERSIONDATE);

  MapErrorWindow(ctx, "ABOUT");
  (void)PrintMessage(ctx, version, FALSE);
}

int UI_Error(ViewContext *ctx, const char *module, int line, const char *fmt,
             ...) {
  char msg_buffer[MESSAGE_LENGTH + 1];
  char final_buffer[MESSAGE_LENGTH + 1];
  va_list ap;

  va_start(ap, fmt);
  (void)vsnprintf(msg_buffer, sizeof(msg_buffer), fmt, ap);
  va_end(ap);

  MapErrorWindow(ctx, "INTERNAL ERROR");
  (void)snprintf(final_buffer, sizeof(final_buffer),
                 "%s*In Module \"%s\"*Line %d", msg_buffer, module, line);
  return PrintMessage(ctx, final_buffer, TRUE);
}

static void MapErrorWindow(ViewContext *ctx, char *header) {
  werase(ctx->ctx_error_window);
  wattron(ctx->ctx_error_window, COLOR_PAIR(CPAIR_WINERR) | A_ALTCHARSET);

  /* Box frame with ACS */
  wborder(ctx->ctx_error_window, 0, 0, 0, 0, 0, 0, 0, 0);

  mvwhline(ctx->ctx_error_window, ERROR_WINDOW_HEIGHT - 3, 1, ACS_HLINE,
           ERROR_WINDOW_WIDTH - 2);
  mvwaddch(ctx->ctx_error_window, ERROR_WINDOW_HEIGHT - 3, 0, ACS_LTEE);
  mvwaddch(ctx->ctx_error_window, ERROR_WINDOW_HEIGHT - 3,
           ERROR_WINDOW_WIDTH - 1, ACS_RTEE);
  wattroff(ctx->ctx_error_window, A_ALTCHARSET);
  wattrset(ctx->ctx_error_window, A_NORMAL);

  wattrset(ctx->ctx_error_window, A_REVERSE | A_BLINK);
  MvWAddStr(ctx->ctx_error_window, ERROR_WINDOW_HEIGHT - 2, 1,
            "             PRESS ENTER              ");
  wattrset(ctx->ctx_error_window, A_NORMAL);
  PrintErrorLine(ctx, 1, header);
}

static void MapNoticeWindow(ViewContext *ctx, char *header) {
  werase(ctx->ctx_error_window);
  wattron(ctx->ctx_error_window, COLOR_PAIR(CPAIR_WINERR) | A_ALTCHARSET);

  /* Box frame with ACS */
  wborder(ctx->ctx_error_window, 0, 0, 0, 0, 0, 0, 0, 0);

  mvwhline(ctx->ctx_error_window, ERROR_WINDOW_HEIGHT - 3, 1, ACS_HLINE,
           ERROR_WINDOW_WIDTH - 2);
  mvwaddch(ctx->ctx_error_window, ERROR_WINDOW_HEIGHT - 3, 0, ACS_LTEE);
  mvwaddch(ctx->ctx_error_window, ERROR_WINDOW_HEIGHT - 3,
           ERROR_WINDOW_WIDTH - 1, ACS_RTEE);
  wattroff(ctx->ctx_error_window, A_ALTCHARSET);
  wattrset(ctx->ctx_error_window, A_NORMAL);

  wattrset(ctx->ctx_error_window, A_REVERSE | A_BLINK);
  MvWAddStr(ctx->ctx_error_window, ERROR_WINDOW_HEIGHT - 2, 1,
            "             PLEASE WAIT              ");
  wattrset(ctx->ctx_error_window, A_NORMAL);
  PrintErrorLine(ctx, 1, header);
}

static void UnmapErrorWindow(ViewContext *ctx) {
  werase(ctx->ctx_error_window);

  /* Restore full UI state after error dialog */
  DirEntry *current = GetSelectedDirEntry(ctx, ctx->active->vol);
  RefreshView(ctx, current);

  doupdate();
}

void UnmapNoticeWindow(ViewContext *ctx) {
  werase(ctx->ctx_error_window);
  touchwin(stdscr);
  doupdate();
}

static void PrintErrorLine(ViewContext *ctx, int y, char *str) {
  int l;

  l = strlen(str);

  MvWAddStr(ctx->ctx_error_window, y, (ERROR_WINDOW_WIDTH - l) >> 1, str);
}

static void DisplayMessage(ViewContext *ctx, char *msg) {
  int y, i, j, count;
  char buffer[ERROR_WINDOW_WIDTH - 2 + 1];

  for (i = 0, count = 0; msg[i]; i++)
    if (msg[i] == '*')
      count++;

  if (count > 3)
    y = 2;
  else if (count > 1)
    y = 3;
  else
    y = 4;

  for (i = 0, j = 0; msg[i]; i++) {
    if (msg[i] == '*') {
      buffer[j] = '\0';
      PrintErrorLine(ctx, y++, buffer);
      j = 0;
    } else {
      if (j < (int)((sizeof(buffer) - 1)))
        buffer[j++] = msg[i];
    }
  }
  buffer[j] = '\0';
  PrintErrorLine(ctx, y, buffer);
}

static int PrintMessage(ViewContext *ctx, char *msg, BOOL do_beep) {
  int c;

  DisplayMessage(ctx, msg);
  if (do_beep && (strtol((GetProfileValue)(ctx, "AUDIBLEERROR"), NULL, 0) != 0))
    beep();
  RefreshWindow(ctx->ctx_error_window);
  doupdate();
  c = WGetch(ctx, ctx->ctx_error_window);
  UnmapErrorWindow(ctx);
  touchwin(ctx->ctx_dir_window);
  return (c);
}