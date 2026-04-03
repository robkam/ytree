/***************************************************************************
 *
 * src/ui/history_dialog.c
 * Command/path history dialog rendering and input loop
 *
 ***************************************************************************/

#include "ytree_ui.h"
#include "ytree_cmd.h"
#include <stdlib.h>
#include <string.h>

#define HISTORY_WINDOW_HEIGHT (LINES - 6)
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static void PrintHstEntry(ViewContext *ctx, int entry_no, int y, int color,
                          int start_x, int *hide_left, int *hide_right);
static int DisplayHistory(ViewContext *ctx);

static void PrintHstEntry(ViewContext *ctx, int entry_no, int y, int color,
                          int start_x, int *hide_left, int *hide_right) {
  int n;
  const History *pp;
  char buffer[BUFSIZ];
  char *line_ptr;
  int window_width;
  int window_height;
  int ef_window_width;

  GetMaxYX(ctx->ctx_history_window, &window_height, &window_width);
  /* Reduce width by 3 to allow for 1 char border + 1 char PIN marker */
  ef_window_width = window_width - 3;

  *hide_left = *hide_right = 0;

  /* Use ViewList instead of traversing Hist */
  if (entry_no < 0 || entry_no >= ctx->history_view_count)
    return;
  pp = ctx->history_view_list[entry_no];

  if (pp) {
    (void)strncpy(buffer, pp->hst, BUFSIZ - 3);
    buffer[BUFSIZ - 3] = '\0';
    n = strlen(buffer);
    wmove(ctx->ctx_history_window, y, 0);

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
    /* Mark pinned items */
    const char *marker = pp->pinned ? "+<" : " <";
    const char *suffix = (color == CPAIR_HIHST) ? marker : "  ";
    size_t line_len = strlen(line_ptr);
    size_t suffix_len = strlen(suffix);
    if (line_len + suffix_len + 1 <= BUFSIZ) {
      memcpy(line_ptr + line_len, suffix, suffix_len + 1);
    }
    WAddStr(ctx->ctx_history_window, line_ptr);
#else
#ifdef COLOR_SUPPORT
    /*
     * Align with F2 Visual Style:
     * 1. Use CPAIR_HST as the base color (matches F2).
     * 2. Use A_REVERSE for selection (matches F2).
     * 3. Ignore CPAIR_HIHST to prevent color mismatch.
     */
    int display_attr = COLOR_PAIR(CPAIR_HST);

    /* If passed color indicates selection, use Reverse Video */
    if (color == CPAIR_HIHST) {
      display_attr |= A_REVERSE;
    }

    wattrset(ctx->ctx_history_window, display_attr);

#else
    if (color == CPAIR_HIHST)
      wattrset(ctx->ctx_history_window, A_REVERSE);
#endif /* COLOR_SUPPORT */

    /* Draw Pin Marker and Space */
    if (pp->pinned)
      waddch(ctx->ctx_history_window, ACS_CKBOARD);
    else
      waddch(ctx->ctx_history_window, ' ');
    waddch(ctx->ctx_history_window, ' ');

    /* Draw Text */
    WAddStr(ctx->ctx_history_window, line_ptr);

#ifdef COLOR_SUPPORT
    /* Reset to standard window background */
    wattrset(ctx->ctx_history_window, COLOR_PAIR(CPAIR_WINHST));
#else
    if (color == CPAIR_HIHST)
      wattrset(ctx->ctx_history_window, 0);
#endif /* COLOR_SUPPORT */
#endif /* NO_HIGHLIGHT */
  }
  return;
}

static int DisplayHistory(ViewContext *ctx) {
  int i, hilight_no, p_y;
  int hide_left, hide_right;

  hilight_no = ctx->disp_begin_pos + ctx->cursor_pos;
  p_y = -1;
  werase(ctx->ctx_history_window);
  for (i = 0; i < HISTORY_WINDOW_HEIGHT; i++) {
    if (ctx->disp_begin_pos + i >= ctx->total_hist)
      break;
    if (ctx->disp_begin_pos + i != hilight_no)
      PrintHstEntry(ctx, ctx->disp_begin_pos + i, i, CPAIR_HST, 0, &hide_left,
                    &hide_right);
    else
      p_y = i;
  }
  if (p_y >= 0) {
    PrintHstEntry(ctx, ctx->disp_begin_pos + p_y, p_y, CPAIR_HIHST, 0,
                  &hide_left, &hide_right);
  }
  return 0;
}

char *GetHistory(ViewContext *ctx, int type) {
  int ch;
  int start_x;
  char *RetVal = NULL;
  History *TMP;
  int hide_left, hide_right;

  /* Initialize View */
  BuildHistoryViewList(ctx, type);

  /* Reset if list empty or smaller */
  if (ctx->total_hist == 0)
    return NULL;

  ctx->disp_begin_pos = 0;
  ctx->cursor_pos = 0;
  start_x = 0;

  UI_Dialog_Push(ctx->ctx_history_window, UI_TIER_POPOVER);
  DisplayHistoryHelp(ctx);

  (void)DisplayHistory(ctx);

  do {
    RefreshWindow(ctx->ctx_history_window);
    doupdate();
    ch = Getch(ctx);

    if (ch != -1 && ch != KEY_RIGHT && ch != KEY_LEFT) {
      if (start_x) {
        start_x = 0;
        PrintHstEntry(ctx, ctx->disp_begin_pos + ctx->cursor_pos,
                      ctx->cursor_pos, CPAIR_HIHST, start_x, &hide_left,
                      &hide_right);
      }
    }

    switch (ch) {
    case -1:
      RetVal = NULL;
      break;

    case ' ':
      break; /* Quick-Key */

    case 'd':
    case 'D':
    case KEY_DC:
      /* Delete current entry */
      if (ctx->history_view_count > 0) {
        History *to_del =
            ctx->history_view_list[ctx->disp_begin_pos + ctx->cursor_pos];

        /* Unlink from global list */
        if (to_del->prev)
          to_del->prev->next = to_del->next;
        if (to_del->next)
          to_del->next->prev = to_del->prev;
        if (ctx->history_head == to_del)
          ctx->history_head = to_del->next;

        if (to_del->hst)
          free(to_del->hst);
        free(to_del);

        /* Rebuild view */
        BuildHistoryViewList(ctx, type);

        /* Adjust cursor */
        if (ctx->disp_begin_pos + ctx->cursor_pos >= ctx->total_hist) {
          if (ctx->cursor_pos > 0)
            ctx->cursor_pos--;
          else if (ctx->disp_begin_pos > 0)
            ctx->disp_begin_pos--;
        }
        if (ctx->total_hist == 0) {
          RetVal = NULL;
          ch = ESC; /* Exit loop */
        } else {
          DisplayHistory(ctx);
        }
      }
      break;

    case 'p':
    case 'P':
    case 'k':
    case 'K':
    case KEY_IC:
      /* Toggle Pin */
      if (ctx->history_view_count > 0) {
        History *target =
            ctx->history_view_list[ctx->disp_begin_pos + ctx->cursor_pos];
        target->pinned = !target->pinned;

        BuildHistoryViewList(ctx, type);
        DisplayHistory(ctx);
      }
      break;

    case KEY_RIGHT:
      start_x++;
      PrintHstEntry(ctx, ctx->disp_begin_pos + ctx->cursor_pos, ctx->cursor_pos,
                    CPAIR_HIHST, start_x, &hide_left, &hide_right);
      if (hide_right < 0)
        start_x--;
      break;

    case KEY_LEFT:
      if (start_x > 0)
        start_x--;
      PrintHstEntry(ctx, ctx->disp_begin_pos + ctx->cursor_pos, ctx->cursor_pos,
                    CPAIR_HIHST, start_x, &hide_left, &hide_right);
      break;

    case '\t':
    case KEY_DOWN:
      if (ctx->disp_begin_pos + ctx->cursor_pos + 1 >= ctx->total_hist) {
        UI_Beep(ctx, FALSE);
      } else {
        if (ctx->cursor_pos + 1 < HISTORY_WINDOW_HEIGHT) {
          PrintHstEntry(ctx, ctx->disp_begin_pos + ctx->cursor_pos,
                        ctx->cursor_pos, CPAIR_HST, start_x, &hide_left,
                        &hide_right);
          ctx->cursor_pos++;
          PrintHstEntry(ctx, ctx->disp_begin_pos + ctx->cursor_pos,
                        ctx->cursor_pos, CPAIR_HIHST, start_x, &hide_left,
                        &hide_right);
        } else {
          PrintHstEntry(ctx, ctx->disp_begin_pos + ctx->cursor_pos,
                        ctx->cursor_pos, CPAIR_HST, start_x, &hide_left,
                        &hide_right);
          scroll(ctx->ctx_history_window);
          ctx->disp_begin_pos++;
          PrintHstEntry(ctx, ctx->disp_begin_pos + ctx->cursor_pos,
                        ctx->cursor_pos, CPAIR_HIHST, start_x, &hide_left,
                        &hide_right);
        }
      }
      break;
    case KEY_BTAB:
    case KEY_UP:
      if (ctx->disp_begin_pos + ctx->cursor_pos - 1 < 0) {
        UI_Beep(ctx, FALSE);
      } else {
        if (ctx->cursor_pos - 1 >= 0) {
          PrintHstEntry(ctx, ctx->disp_begin_pos + ctx->cursor_pos,
                        ctx->cursor_pos, CPAIR_HST, start_x, &hide_left,
                        &hide_right);
          ctx->cursor_pos--;
          PrintHstEntry(ctx, ctx->disp_begin_pos + ctx->cursor_pos,
                        ctx->cursor_pos, CPAIR_HIHST, start_x, &hide_left,
                        &hide_right);
        } else {
          PrintHstEntry(ctx, ctx->disp_begin_pos + ctx->cursor_pos,
                        ctx->cursor_pos, CPAIR_HST, start_x, &hide_left,
                        &hide_right);
          wmove(ctx->ctx_history_window, 0, 0);
          winsertln(ctx->ctx_history_window);
          ctx->disp_begin_pos--;
          PrintHstEntry(ctx, ctx->disp_begin_pos + ctx->cursor_pos,
                        ctx->cursor_pos, CPAIR_HIHST, start_x, &hide_left,
                        &hide_right);
        }
      }
      break;
    case KEY_NPAGE:
      if (ctx->disp_begin_pos + ctx->cursor_pos >= ctx->total_hist - 1) {
        UI_Beep(ctx, FALSE);
      } else {
        if (ctx->cursor_pos < HISTORY_WINDOW_HEIGHT - 1) {
          PrintHstEntry(ctx, ctx->disp_begin_pos + ctx->cursor_pos,
                        ctx->cursor_pos, CPAIR_HST, start_x, &hide_left,
                        &hide_right);
          if (ctx->disp_begin_pos + HISTORY_WINDOW_HEIGHT > ctx->total_hist - 1)
            ctx->cursor_pos = ctx->total_hist - ctx->disp_begin_pos - 1;
          else
            ctx->cursor_pos = HISTORY_WINDOW_HEIGHT - 1;
          PrintHstEntry(ctx, ctx->disp_begin_pos + ctx->cursor_pos,
                        ctx->cursor_pos, CPAIR_HIHST, start_x, &hide_left,
                        &hide_right);
        } else {
          if (ctx->disp_begin_pos + ctx->cursor_pos + HISTORY_WINDOW_HEIGHT <
              ctx->total_hist) {
            ctx->disp_begin_pos += HISTORY_WINDOW_HEIGHT;
            ctx->cursor_pos = HISTORY_WINDOW_HEIGHT - 1;
          } else {
            ctx->disp_begin_pos = ctx->total_hist - HISTORY_WINDOW_HEIGHT;
            if (ctx->disp_begin_pos < 0)
              ctx->disp_begin_pos = 0;
            ctx->cursor_pos = ctx->total_hist - ctx->disp_begin_pos - 1;
          }
          DisplayHistory(ctx);
        }
      }
      break;
    case KEY_PPAGE:
      if (ctx->disp_begin_pos + ctx->cursor_pos <= 0) {
        UI_Beep(ctx, FALSE);
      } else {
        if (ctx->cursor_pos > 0) {
          PrintHstEntry(ctx, ctx->disp_begin_pos + ctx->cursor_pos,
                        ctx->cursor_pos, CPAIR_HST, start_x, &hide_left,
                        &hide_right);
          ctx->cursor_pos = 0;
          PrintHstEntry(ctx, ctx->disp_begin_pos + ctx->cursor_pos,
                        ctx->cursor_pos, CPAIR_HIHST, start_x, &hide_left,
                        &hide_right);
        } else {
          if ((ctx->disp_begin_pos -= HISTORY_WINDOW_HEIGHT) < 0) {
            ctx->disp_begin_pos = 0;
          }
          ctx->cursor_pos = 0;
          DisplayHistory(ctx);
        }
      }
      break;
    case KEY_HOME:
      if (ctx->disp_begin_pos == 0 && ctx->cursor_pos == 0) {
        UI_Beep(ctx, FALSE);
      } else {
        ctx->disp_begin_pos = 0;
        ctx->cursor_pos = 0;
        DisplayHistory(ctx);
      }
      break;
    case KEY_END:
      ctx->disp_begin_pos = MAX(0, ctx->total_hist - HISTORY_WINDOW_HEIGHT);
      ctx->cursor_pos = ctx->total_hist - ctx->disp_begin_pos - 1;
      DisplayHistory(ctx);
      break;
    case LF:
    case CR:
      if (ctx->history_view_count > 0 &&
          ctx->disp_begin_pos + ctx->cursor_pos < ctx->history_view_count) {
        TMP = ctx->history_view_list[ctx->disp_begin_pos + ctx->cursor_pos];
        if (TMP)
          RetVal = TMP->hst;
        else
          RetVal = NULL;
      } else {
        RetVal = NULL;
      }
      break;

    case ESC:
      RetVal = NULL;
      break;

    default:
      UI_Beep(ctx, FALSE);
      break;
    } /* switch */
  } while (ch != CR && ch != ESC && ch != -1);

  UI_Dialog_Pop(ctx->ctx_history_window);
  UI_Dialog_RefreshAll(ctx);

  return RetVal;
}
