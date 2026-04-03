/***************************************************************************
 *
 * src/ui/completion_dialog.c
 * Filename completion dialog rendering and input loop
 *
 ***************************************************************************/

#include "ytree_ui.h"
#include "ytree_cmd.h"
#include <string.h>

#define MATCHES_WINDOW_HEIGHT (LINES - 6)
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static void PrintMtchEntry(ViewContext *ctx, int entry_no, int y, int color,
                           int start_x, int *hide_left, int *hide_right);
static int DisplayMatches(ViewContext *ctx);

static void PrintMtchEntry(ViewContext *ctx, int entry_no, int y, int color,
                           int start_x, int *hide_left, int *hide_right) {
  int n;
  char buffer[BUFSIZ];
  char *line_ptr;
  int window_width;
  int window_height;
  int ef_window_width;

  GetMaxYX(ctx->ctx_matches_window, &window_height, &window_width);

#ifdef NO_HIGHLIGHT
  ef_window_width = window_width - 3; /* Effektive Window-Width */
#else
  ef_window_width = window_width - 2; /* Effektive Window-Width */
#endif

  *hide_left = *hide_right = 0;

  if (ctx->tab_mtchs[entry_no]) {
    (void)strncpy(buffer, (char *)ctx->tab_mtchs[entry_no], BUFSIZ - 3);
    buffer[BUFSIZ - 3] = '\0';
    n = strlen(buffer);
    wmove(ctx->ctx_matches_window, y, 1);

    if (n <= ef_window_width) {

      /* will completely fit into window */
      /*---------------------------------*/

      line_ptr = buffer;
    } else {
      /* does not completely fit into window;
       * ==> use start_x
       */

      if (n > (start_x + ef_window_width))
        line_ptr = &buffer[start_x];
      else
        line_ptr = &buffer[n - ef_window_width];

      *hide_left = start_x;
      *hide_right = n - start_x - ef_window_width;

      line_ptr[ef_window_width] = '\0';
    }

#ifdef NO_HIGHLIGHT
    {
      const char *suffix = (color == CPAIR_HIHST) ? " <" : "  ";
      size_t line_len = strlen(line_ptr);
      size_t suffix_len = strlen(suffix);
      if (line_len + suffix_len + 1 <= BUFSIZ) {
        memcpy(line_ptr + line_len, suffix, suffix_len + 1);
      }
    }
    WAddStr(ctx->ctx_matches_window, line_ptr);
#else
#ifdef COLOR_SUPPORT
    WbkgdSet(ctx, ctx->ctx_matches_window, COLOR_PAIR(color));
#else
    if (color == CPAIR_HIHST)
      wattrset(ctx->ctx_matches_window, A_REVERSE);
#endif /* COLOR_SUPPORT */
    WAddStr(ctx->ctx_matches_window, line_ptr);
#ifdef COLOR_SUPPORT
    WbkgdSet(ctx, ctx->ctx_matches_window, COLOR_PAIR(CPAIR_WINHST));
#else
    if (color == CPAIR_HIHST)
      wattrset(ctx->ctx_matches_window, 0);
#endif /* COLOR_SUPPORT */
#endif /* NO_HIGHLIGHT */
  }
  return;
}

static int DisplayMatches(ViewContext *ctx) {
  int i, hilight_no, p_y;
  int hide_left, hide_right;

  hilight_no = ctx->tab_disp_begin_pos + ctx->tab_cursor_pos;
  p_y = -1;
  werase(ctx->ctx_matches_window);
  for (i = 0; i < MATCHES_WINDOW_HEIGHT; i++) {
    if (ctx->tab_disp_begin_pos + i >= ctx->tab_total_matches)
      break;
    if (ctx->tab_disp_begin_pos + i != hilight_no)
      PrintMtchEntry(ctx, ctx->tab_disp_begin_pos + i, i, CPAIR_HST, 0,
                     &hide_left, &hide_right);
    else
      p_y = i;
  }
  if (p_y >= 0) {
    PrintMtchEntry(ctx, ctx->tab_disp_begin_pos + p_y, p_y, CPAIR_HIHST, 0,
                   &hide_left, &hide_right);
  }
  return 0;
}

char *GetMatches(ViewContext *ctx, char *base) {
  int ch;
  int start_x;
  int show_dialog;
  char *RetVal = NULL;
  char *TMP;
  int hide_left, hide_right;

  RetVal = PrepareCompletionMatches(ctx, base, &show_dialog);
  if (!show_dialog)
    return RetVal;

  start_x = 0;
  (void)DisplayMatches(ctx);

  do {
    RefreshWindow(ctx->ctx_matches_window);
    doupdate();
    ch = Getch(ctx);

    if (ch != -1 && ch != KEY_RIGHT && ch != KEY_LEFT) {
      if (start_x) {
        start_x = 0;
        PrintMtchEntry(ctx, ctx->tab_disp_begin_pos + ctx->tab_cursor_pos,
                       ctx->tab_cursor_pos, CPAIR_HIHST, start_x, &hide_left,
                       &hide_right);
      }
    }

    switch (ch) {
    case -1:
      RetVal = NULL;
      break;

    case ' ':
      break; /* Quick-Key */

    case KEY_RIGHT:
      start_x++;
      PrintMtchEntry(ctx, ctx->tab_disp_begin_pos + ctx->tab_cursor_pos,
                     ctx->tab_cursor_pos, CPAIR_HIHST, start_x, &hide_left,
                     &hide_right);
      if (hide_right < 0)
        start_x--;
      break;

    case KEY_LEFT:
      if (start_x > 0)
        start_x--;
      PrintMtchEntry(ctx, ctx->tab_disp_begin_pos + ctx->tab_cursor_pos,
                     ctx->tab_cursor_pos, CPAIR_HIHST, start_x, &hide_left,
                     &hide_right);
      break;

    case '\t':
    case KEY_DOWN:
      if (ctx->tab_disp_begin_pos + ctx->tab_cursor_pos + 1 >=
          ctx->tab_total_matches) {
        UI_Beep(ctx, FALSE);
      } else {
        if (ctx->tab_cursor_pos + 1 < MATCHES_WINDOW_HEIGHT) {
          PrintMtchEntry(ctx, ctx->tab_disp_begin_pos + ctx->tab_cursor_pos,
                         ctx->tab_cursor_pos, CPAIR_HST, start_x, &hide_left,
                         &hide_right);
          ctx->tab_cursor_pos++;
          PrintMtchEntry(ctx, ctx->tab_disp_begin_pos + ctx->tab_cursor_pos,
                         ctx->tab_cursor_pos, CPAIR_HIHST, start_x, &hide_left,
                         &hide_right);
        } else {
          PrintMtchEntry(ctx, ctx->tab_disp_begin_pos + ctx->tab_cursor_pos,
                         ctx->tab_cursor_pos, CPAIR_HST, start_x, &hide_left,
                         &hide_right);
          scroll(ctx->ctx_matches_window);
          ctx->tab_disp_begin_pos++;
          PrintMtchEntry(ctx, ctx->tab_disp_begin_pos + ctx->tab_cursor_pos,
                         ctx->tab_cursor_pos, CPAIR_HIHST, start_x, &hide_left,
                         &hide_right);
        }
      }
      break;
    case KEY_BTAB:
    case KEY_UP:
      if (ctx->tab_disp_begin_pos + ctx->tab_cursor_pos - 1 < 1) {
        UI_Beep(ctx, FALSE);
      } else {
        if (ctx->tab_cursor_pos - 1 >= 0) {
          PrintMtchEntry(ctx, ctx->tab_disp_begin_pos + ctx->tab_cursor_pos,
                         ctx->tab_cursor_pos, CPAIR_HST, start_x, &hide_left,
                         &hide_right);
          ctx->tab_cursor_pos--;
          PrintMtchEntry(ctx, ctx->tab_disp_begin_pos + ctx->tab_cursor_pos,
                         ctx->tab_cursor_pos, CPAIR_HIHST, start_x, &hide_left,
                         &hide_right);
        } else {
          PrintMtchEntry(ctx, ctx->tab_disp_begin_pos + ctx->tab_cursor_pos,
                         ctx->tab_cursor_pos, CPAIR_HST, start_x, &hide_left,
                         &hide_right);
          wmove(ctx->ctx_matches_window, 0, 0);
          winsertln(ctx->ctx_matches_window);
          ctx->tab_disp_begin_pos--;
          PrintMtchEntry(ctx, ctx->tab_disp_begin_pos + ctx->tab_cursor_pos,
                         ctx->tab_cursor_pos, CPAIR_HIHST, start_x, &hide_left,
                         &hide_right);
        }
      }
      break;
    case KEY_NPAGE:
      if (ctx->tab_disp_begin_pos + ctx->tab_cursor_pos >=
          ctx->tab_total_matches - 1) {
        UI_Beep(ctx, FALSE);
      } else {
        if (ctx->tab_cursor_pos < MATCHES_WINDOW_HEIGHT - 1) {
          PrintMtchEntry(ctx, ctx->tab_disp_begin_pos + ctx->tab_cursor_pos,
                         ctx->tab_cursor_pos, CPAIR_HST, start_x, &hide_left,
                         &hide_right);
          if (ctx->tab_disp_begin_pos + MATCHES_WINDOW_HEIGHT >
              ctx->tab_total_matches - 1)
            ctx->tab_cursor_pos =
                ctx->tab_total_matches - ctx->tab_disp_begin_pos - 1;
          else
            ctx->tab_cursor_pos = MATCHES_WINDOW_HEIGHT - 1;
          PrintMtchEntry(ctx, ctx->tab_disp_begin_pos + ctx->tab_cursor_pos,
                         ctx->tab_cursor_pos, CPAIR_HIHST, start_x, &hide_left,
                         &hide_right);
        } else {
          if (ctx->tab_disp_begin_pos + ctx->tab_cursor_pos +
                  MATCHES_WINDOW_HEIGHT <
              ctx->tab_total_matches) {
            ctx->tab_disp_begin_pos += MATCHES_WINDOW_HEIGHT;
            ctx->tab_cursor_pos = MATCHES_WINDOW_HEIGHT - 1;
          } else {
            ctx->tab_disp_begin_pos =
                ctx->tab_total_matches - MATCHES_WINDOW_HEIGHT;
            if (ctx->tab_disp_begin_pos < 1)
              ctx->tab_disp_begin_pos = 1;
            ctx->tab_cursor_pos =
                ctx->tab_total_matches - ctx->tab_disp_begin_pos - 1;
          }
          DisplayMatches(ctx);
        }
      }
      break;
    case KEY_PPAGE:
      if (ctx->tab_disp_begin_pos + ctx->tab_cursor_pos <= 1) {
        UI_Beep(ctx, FALSE);
      } else {
        if (ctx->tab_cursor_pos > 0) {
          PrintMtchEntry(ctx, ctx->tab_disp_begin_pos + ctx->tab_cursor_pos,
                         ctx->tab_cursor_pos, CPAIR_HST, start_x, &hide_left,
                         &hide_right);
          ctx->tab_cursor_pos = 0;
          PrintMtchEntry(ctx, ctx->tab_disp_begin_pos + ctx->tab_cursor_pos,
                         ctx->tab_cursor_pos, CPAIR_HIHST, start_x, &hide_left,
                         &hide_right);
        } else {
          if ((ctx->tab_disp_begin_pos -= MATCHES_WINDOW_HEIGHT) < 1) {
            ctx->tab_disp_begin_pos = 1;
          }
          ctx->tab_cursor_pos = 0;
          DisplayMatches(ctx);
        }
      }
      break;
    case KEY_HOME:
      if (ctx->tab_disp_begin_pos == 1 && ctx->tab_cursor_pos == 0) {
        UI_Beep(ctx, FALSE);
      } else {
        ctx->tab_disp_begin_pos = 1;
        ctx->tab_cursor_pos = 0;
        DisplayMatches(ctx);
      }
      break;
    case KEY_END:
      ctx->tab_disp_begin_pos =
          MAX(1, ctx->tab_total_matches - MATCHES_WINDOW_HEIGHT);
      ctx->tab_cursor_pos =
          ctx->tab_total_matches - ctx->tab_disp_begin_pos - 1;
      DisplayMatches(ctx);
      break;
    case LF:
    case CR:
      {
        size_t selected_len =
            strlen(ctx->tab_mtchs[ctx->tab_disp_begin_pos + ctx->tab_cursor_pos]);
        TMP = xmalloc(selected_len + 1);
        memcpy(TMP,
               ctx->tab_mtchs[ctx->tab_disp_begin_pos + ctx->tab_cursor_pos],
               selected_len + 1);
      }
      RetVal = TMP;
      break;

    case ESC:
      RetVal = NULL;
      break;

    default:
      UI_Beep(ctx, FALSE);
      break;
    } /* switch */
  } while (ch != CR && ch != ESC && ch != -1);

  touchwin(stdscr);
  return RetVal;
}
