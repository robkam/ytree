/***************************************************************************
 *
 * src/util/completion_utils.c
 * Filename completion (TAB key) utilities
 *
 ***************************************************************************/

#include "ytree.h"

#ifdef READLINE_SUPPORT
#include <readline/readline.h>
#include <readline/tilde.h>
#endif

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
  ef_window_width = window_width - 2; /* Effektive Window-Width */

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
    strcat(line_ptr, (color == CPAIR_HIHST) ? " <" : "  ");
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
  char *RetVal = NULL;
  char *TMP;
  char *expanded_base =
      NULL; /* Renamed from tmpval for clarity, holds tilde_expand result */
  int hide_left, hide_right;

  /*  tmpval = rl_filename_completion_function(base, 0);
    if (!strcmp(tmpval,base))
      return(tmpval);*/

  /* --- START OF NEW CLEANUP LOGIC (Bug 3 of 10) --- */
  /* Free previous completion matches if they exist.
   * This ensures that the `char **ctx->tab_mtchs` array and its contained
   * strings from the *previous* call to GetMatches are freed, preventing memory
   * leaks. */
  if (ctx->tab_mtchs != NULL) {
    int i;
    for (i = 0; ctx->tab_mtchs[i] != NULL; i++) {
      free(ctx->tab_mtchs[i]);
    }
    free(ctx->tab_mtchs);
    ctx->tab_mtchs = NULL; /* Reset to NULL after freeing */
  }
  /* --- END OF NEW CLEANUP LOGIC --- */

#ifdef READLINE_SUPPORT
  expanded_base = tilde_expand(base); /* This allocates memory */
  if (expanded_base == NULL) {
    /* tilde_expand returns NULL on allocation failure, or strdup(base) if no
     * expansion. If it returns NULL, there's nothing to free. */
    return (NULL);
  }

  if ((ctx->tab_mtchs = rl_completion_matches(
           expanded_base, rl_filename_completion_function)) == NULL) {
    free(expanded_base); /* Fix: Free the allocated string before returning */
    return (NULL);
  }
#else
  /* If READLINE_SUPPORT is not defined, ctx->tab_mtchs will remain NULL. */
  /* expanded_base is not allocated here, so no need to free. */
  return (NULL);
#endif

  /* Check if the first match is identical to the expanded base.
   * This often means no actual completion happened, or only one trivial match.
   */
  if (ctx->tab_mtchs[0] != NULL &&
      strcmp(expanded_base, ctx->tab_mtchs[0]) == 0) {
    /* If the first match is the same as the input, it's not a useful
     * completion. The original code returned NULL in this case, after freeing
     * ctx->tab_mtchs. With the new static cleanup, ctx->tab_mtchs should
     * persist for the next call. So, we just free expanded_base and return
     * NULL. The ctx->tab_mtchs array will be cleaned up by the *next* call to
     * GetMatches. */
    free(expanded_base);
    return (NULL);
  }

  /* If there's a useful first match that's different from the base,
   * the original code would return a copy of it. */
  if (ctx->tab_mtchs[0] != NULL) {
    TMP = xmalloc(strlen(ctx->tab_mtchs[0]) + 1);
    strcpy(TMP, ctx->tab_mtchs[0]);
    RetVal = TMP;
    /* Original code had `free(ctx->tab_mtchs);` here. This is removed as
     * ctx->tab_mtchs is now managed by the static cleanup at the beginning of
     * the *next* call. */
    free(expanded_base); /* expanded_base is no longer needed */
    return RetVal;
  }

  for (ctx->tab_total_matches = 0; ctx->tab_mtchs[ctx->tab_total_matches];
       ctx->tab_total_matches++)
    ;
  if (ctx->tab_total_matches == 1) {
    /* If only one match, and it wasn't handled by the strcmp check above,
     * it's still not an interactive completion. Return NULL. */
    free(expanded_base); /* expanded_base is no longer needed */
    return (NULL);
  }

  ctx->tab_disp_begin_pos = 1;
  ctx->tab_cursor_pos = 0;
  start_x = 0;
  /* leaveok(stdscr, TRUE); */
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
        beep();
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
        beep();
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
        beep();
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
        beep();
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
        beep();
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
      TMP = xmalloc(
          strlen(
              ctx->tab_mtchs[ctx->tab_disp_begin_pos + ctx->tab_cursor_pos]) +
          1);
      strcpy(TMP,
             ctx->tab_mtchs[ctx->tab_disp_begin_pos + ctx->tab_cursor_pos]);
      RetVal = TMP;
      break;

    case ESC:
      RetVal = NULL;
      break;

    default:
      beep();
      break;
    } /* switch */
  } while (ch != CR && ch != ESC && ch != -1);
  /* leaveok(stdscr, FALSE); */
  /* Original code had `free(ctx->tab_mtchs);` here. This is removed as
   * ctx->tab_mtchs is now managed by the static cleanup at the beginning of the
   * *next* call. */
  free(expanded_base); /* expanded_base is always allocated and must be freed */
  touchwin(stdscr);
  return RetVal;
}