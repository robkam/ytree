/***************************************************************************
 *
 * src/ui/error.c
 * Output of error messages
 *
 ***************************************************************************/

#include "ytree_ui.h"

#include <stdarg.h>
#include <ctype.h>

typedef enum ModalSeverity {
  MODAL_SEVERITY_INFO = 0,
  MODAL_SEVERITY_WARNING,
  MODAL_SEVERITY_ERROR
} ModalSeverity;

static short ModalSeverityColorPair(ModalSeverity severity);
static void MapModalWindow(ViewContext *ctx, char *header, char *prompt,
                           ModalSeverity severity);
static void UnmapErrorWindow(ViewContext *ctx);
static void PrintErrorLine(ViewContext *ctx, int y, char *str);
static int GetWrappedChunk(char *segment, int segment_len, int line_start,
                           int body_width, int *chunk_offset, int *chunk_len);
static int CountWrappedSegmentLines(char *segment, int segment_len,
                                    int body_width);
static int CountWrappedBodyLines(char *msg, int body_width);
static void DisplayMessage(ViewContext *ctx, char *msg);
static int PrintMessage(ViewContext *ctx, char *msg);
static void ClearStatusLineErrorLine(ViewContext *ctx);

void UI_Beep(ViewContext *ctx, BOOL critical) {
  (void)ctx;
  (void)critical;
  /* Audible bell behavior is permanently disabled. */
}

void UI_RenderStatusLineError(ViewContext *ctx) {
  if (!ctx || !ctx->ctx_menu_window || !ctx->status_line_error_pending)
    return;

  wmove(ctx->ctx_menu_window, 2, 0);
  wclrtoeol(ctx->ctx_menu_window);
  PrintMenuOptions(ctx->ctx_menu_window, 2, 0, ctx->status_line_error_text,
                   CPAIR_MENU, CPAIR_WINERR);
  wnoutrefresh(ctx->ctx_menu_window);
}

void UI_ShowStatusLineError(ViewContext *ctx, const char *fmt, ...) {
  va_list ap;

  if (!ctx || !fmt)
    return;

  va_start(ap, fmt);
  (void)vsnprintf(ctx->status_line_error_text, sizeof(ctx->status_line_error_text),
                  fmt, ap);
  va_end(ap);

  ctx->status_line_error_pending = TRUE;
  UI_RenderStatusLineError(ctx);
  doupdate();
}

void UI_ClearStatusLineError(ViewContext *ctx) {
  const DirEntry *dir_entry = NULL;

  if (!ctx || !ctx->status_line_error_pending)
    return;

  ctx->status_line_error_pending = FALSE;
  ctx->status_line_error_text[0] = '\0';
  ClearStatusLineErrorLine(ctx);
  if (!ctx->ctx_menu_window)
    return;

  if (ctx->preview_mode) {
    DisplayPreviewHelp(ctx);
  } else {
    if (ctx->active)
      dir_entry = GetPanelDirEntry(ctx->active);
    if (ctx->focused_window == FOCUS_TREE)
      DisplayDirHelp(ctx, dir_entry);
    else
      DisplayFileHelp(ctx, dir_entry);
  }
}

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  char buffer[MESSAGE_LENGTH + 1];
  va_list ap;

  va_start(ap, fmt);
  (void)vsnprintf(buffer, sizeof(buffer), fmt, ap);
  va_end(ap);

  if (ctx == NULL || ctx->ctx_error_window == NULL) {
    fprintf(stderr, "%s\n", buffer);
    return 0;
  }

  MapModalWindow(ctx, "I N F O", "             PRESS ENTER              ",
                 MODAL_SEVERITY_INFO);
  return PrintMessage(ctx, buffer);
}

int UI_Notice(ViewContext *ctx, const char *fmt, ...) {
  char buffer[MESSAGE_LENGTH + 1];
  va_list ap;

  va_start(ap, fmt);
  (void)vsnprintf(buffer, sizeof(buffer), fmt, ap);
  va_end(ap);

  if (ctx == NULL || ctx->ctx_error_window == NULL) {
    fprintf(stderr, "%s\n", buffer);
    return 0;
  }

  MapModalWindow(ctx, "N O T I C E", "             PLEASE WAIT              ",
                 MODAL_SEVERITY_INFO);
  DisplayMessage(ctx, buffer);
  RefreshWindow(ctx->ctx_error_window);
  doupdate();
  return 0;
}

int UI_Warning(ViewContext *ctx, const char *fmt, ...) {
  char buffer[MESSAGE_LENGTH + 1];
  va_list ap;

  va_start(ap, fmt);
  (void)vsnprintf(buffer, sizeof(buffer), fmt, ap);
  va_end(ap);

  if (ctx == NULL || ctx->ctx_error_window == NULL) {
    fprintf(stderr, "%s\n", buffer);
    return 0;
  }

  MapModalWindow(ctx, "W A R N I N G", "             PRESS ENTER              ",
                 MODAL_SEVERITY_WARNING);
  return PrintMessage(ctx, buffer);
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

  MapModalWindow(ctx, "ABOUT", "             PRESS ENTER              ",
                 MODAL_SEVERITY_INFO);
  (void)PrintMessage(ctx, version);
}

int UI_Error(ViewContext *ctx, const char *module, int line, const char *fmt,
             ...) {
  char msg_buffer[MESSAGE_LENGTH + 1];
  char final_buffer[MESSAGE_LENGTH + 1];
  va_list ap;

  va_start(ap, fmt);
  (void)vsnprintf(msg_buffer, sizeof(msg_buffer), fmt, ap);
  va_end(ap);

  if (ctx == NULL || ctx->ctx_error_window == NULL) {
    fprintf(stderr, "%s (module=%s line=%d)\n", msg_buffer, module, line);
    return -1;
  }

  MapModalWindow(ctx, "INTERNAL ERROR", "             PRESS ENTER              ",
                 MODAL_SEVERITY_ERROR);
  (void)snprintf(final_buffer, sizeof(final_buffer),
                 "%s*In Module \"%s\"*Line %d", msg_buffer, module, line);
  return PrintMessage(ctx, final_buffer);
}

static short ModalSeverityColorPair(ModalSeverity severity) {
  switch (severity) {
    case MODAL_SEVERITY_INFO:
      return CPAIR_INFO;
    case MODAL_SEVERITY_WARNING:
      return CPAIR_WARN;
    case MODAL_SEVERITY_ERROR:
    default:
      return CPAIR_ERR;
  }
}

static void MapModalWindow(ViewContext *ctx, char *header, char *prompt,
                           ModalSeverity severity) {
  short color_pair = ModalSeverityColorPair(severity);

  WbkgdSet(ctx, ctx->ctx_error_window, COLOR_PAIR(color_pair));
  werase(ctx->ctx_error_window);
  wattron(ctx->ctx_error_window, COLOR_PAIR(color_pair) | A_ALTCHARSET);

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
  MvWAddStr(ctx->ctx_error_window, ERROR_WINDOW_HEIGHT - 2, 1, prompt);
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

static int GetWrappedChunk(char *segment, int segment_len, int line_start,
                           int body_width, int *chunk_offset, int *chunk_len) {
  int remaining, wrap_end, next_start;

  while (line_start < segment_len &&
         isspace((unsigned char)segment[line_start]))
    line_start++;

  if (line_start >= segment_len) {
    *chunk_offset = segment_len;
    *chunk_len = 0;
    return segment_len;
  }

  remaining = segment_len - line_start;
  if (remaining <= body_width) {
    *chunk_offset = line_start;
    *chunk_len = remaining;
    return segment_len;
  }

  wrap_end = line_start + body_width;
  while (wrap_end > line_start && !isspace((unsigned char)segment[wrap_end]))
    wrap_end--;

  if (wrap_end == line_start)
    wrap_end = line_start + body_width;

  next_start = wrap_end;
  while (next_start < segment_len &&
         isspace((unsigned char)segment[next_start]))
    next_start++;

  while (wrap_end > line_start && isspace((unsigned char)segment[wrap_end - 1]))
    wrap_end--;

  if (wrap_end <= line_start)
    wrap_end = line_start + body_width;

  *chunk_offset = line_start;
  *chunk_len = wrap_end - line_start;
  return next_start;
}

static int CountWrappedSegmentLines(char *segment, int segment_len, int body_width) {
  int line_start, line_count;
  int chunk_offset, chunk_len;

  if (segment_len <= 0)
    return 1;

  line_start = 0;
  line_count = 0;
  while (line_start < segment_len) {
    line_start = GetWrappedChunk(segment, segment_len, line_start, body_width,
                                 &chunk_offset, &chunk_len);
    if (chunk_len <= 0)
      break;
    line_count++;
  }

  if (line_count <= 0)
    return 1;
  return line_count;
}

static int CountWrappedBodyLines(char *msg, int body_width) {
  int i, segment_len, line_count;
  char segment[MESSAGE_LENGTH + 1];

  line_count = 0;
  segment_len = 0;
  for (i = 0;; i++) {
    if (msg[i] == '*' || msg[i] == '\0') {
      segment[segment_len] = '\0';
      line_count += CountWrappedSegmentLines(segment, segment_len, body_width);
      segment_len = 0;
      if (msg[i] == '\0')
        break;
    } else {
      if (segment_len < MESSAGE_LENGTH)
        segment[segment_len++] = msg[i];
    }
  }

  return line_count;
}

static void DisplayMessage(ViewContext *ctx, char *msg) {
  int y, i, segment_len, line_start, chunk_offset, chunk_len, next_start;
  int body_top, body_bottom, body_rows, body_width;
  int total_lines, rendered_lines;
  BOOL center_body;
  char buffer[ERROR_WINDOW_WIDTH - 2 + 1];
  char segment[MESSAGE_LENGTH + 1];

  body_top = 2;
  body_bottom = ERROR_WINDOW_HEIGHT - 4;
  body_rows = body_bottom - body_top + 1;
  body_width = ERROR_WINDOW_WIDTH - 2;

  total_lines = CountWrappedBodyLines(msg, body_width);
  rendered_lines = total_lines;
  if (rendered_lines > body_rows)
    rendered_lines = body_rows;
  y = body_top;
  if (rendered_lines < body_rows)
    y += (body_rows - rendered_lines) >> 1;
  center_body = (total_lines == 1);

  segment_len = 0;
  for (i = 0;; i++) {
    if (msg[i] == '*' || msg[i] == '\0') {
      segment[segment_len] = '\0';
      if (segment_len == 0) {
        buffer[0] = '\0';
        if (y > body_bottom)
          break;
        if (center_body)
          PrintErrorLine(ctx, y, buffer);
        else
          MvWAddStr(ctx->ctx_error_window, y, 1, buffer);
        y++;
      } else {
        line_start = 0;
        while (line_start < segment_len) {
          if (y > body_bottom)
            break;
          next_start = GetWrappedChunk(segment, segment_len, line_start,
                                       body_width, &chunk_offset, &chunk_len);
          if (chunk_len <= 0)
            break;
          memcpy(buffer, segment + chunk_offset, chunk_len);
          buffer[chunk_len] = '\0';
          if (center_body)
            PrintErrorLine(ctx, y, buffer);
          else
            MvWAddStr(ctx->ctx_error_window, y, 1, buffer);
          y++;
          line_start = next_start;
        }
      }

      if (y > body_bottom || msg[i] == '\0')
        break;
      segment_len = 0;
    } else {
      if (segment_len < MESSAGE_LENGTH)
        segment[segment_len++] = msg[i];
    }
  }
}

static int PrintMessage(ViewContext *ctx, char *msg) {
  int c;

  DisplayMessage(ctx, msg);
  RefreshWindow(ctx->ctx_error_window);
  doupdate();
  c = WGetch(ctx, ctx->ctx_error_window);
  UnmapErrorWindow(ctx);
  touchwin(ctx->ctx_dir_window);
  return (c);
}

static void ClearStatusLineErrorLine(ViewContext *ctx) {
  if (!ctx || !ctx->ctx_menu_window)
    return;
  wmove(ctx->ctx_menu_window, 2, 0);
  wclrtoeol(ctx->ctx_menu_window);
}
