/***************************************************************************
 *
 * src/util/history_utils.c
 * Command and path history management
 *
 ***************************************************************************/

#include "ytree.h"

static void PrintHstEntry(ViewContext *ctx, int entry_no, int y, int color,
                          int start_x, int *hide_left, int *hide_right);
static int DisplayHistory(ViewContext *ctx);

#define MAX_HST_FILE_LINES 200
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static void FreeViewList(ViewContext *ctx) {
  if (ctx->history_view_list) {
    free(ctx->history_view_list);
    ctx->history_view_list = NULL;
  }
  ctx->history_view_count = 0;
  ctx->total_hist = 0;
}

static void BuildViewList(ViewContext *ctx, int type) {
  History *ptr;
  int i;

  FreeViewList(ctx);

  /* First, count matching items */
  for (ptr = ctx->history_head; ptr; ptr = ptr->next) {
    if (ptr->type == type) {
      ctx->history_view_count++;
    }
  }
  ctx->total_hist = ctx->history_view_count;

  if (ctx->history_view_count == 0)
    return;

  ctx->history_view_list =
      (History **)xmalloc(ctx->history_view_count * sizeof(History *));

  /* Populate ViewList: Pinned first (preserving relative order from Hist), then
   * Unpinned */
  i = 0;

  /* Pass 1: Add Pinned items */
  for (ptr = ctx->history_head; ptr; ptr = ptr->next) {
    if (ptr->type == type && ptr->pinned) {
      ctx->history_view_list[i++] = ptr;
    }
  }

  /* Pass 2: Add Unpinned items */
  for (ptr = ctx->history_head; ptr; ptr = ptr->next) {
    if (ptr->type == type && !ptr->pinned) {
      ctx->history_view_list[i++] = ptr;
    }
  }
}

void ReadHistory(ViewContext *ctx, const char *Filename) {
  FILE *HstFile;
  char buffer[BUFSIZ];
  char *cptr;
  char *content;

  if ((HstFile = fopen(Filename, "r")) != NULL) {
    while (fgets(buffer, sizeof(buffer), HstFile)) {
      if (strlen(buffer) > 0) {
        /* Strip newline */
        if (buffer[strlen(buffer) - 1] == '\n')
          buffer[strlen(buffer) - 1] = '\0';

        if (strlen(buffer) == 0)
          continue;

        /* Try to parse format T:P:Content */
        /* Find first colon */
        cptr = strchr(buffer, ':');
        if (cptr && isdigit((unsigned char)buffer[0])) {
          *cptr = '\0';

          /* Find second colon */
          content = cptr + 1;
          cptr = strchr(content, ':');
          if (cptr && isdigit((unsigned char)content[0])) {
            *cptr = '\0';
            int type = atoi(buffer);
            int pinned = atoi(content);
            content = cptr + 1;

            InsHistory(ctx, content, type);
            /* Apply pinned flag to the newly inserted item (which is at Hist)
             */
            if (ctx->history_head &&
                strcmp(ctx->history_head->hst, content) == 0) {
              ctx->history_head->pinned = pinned;
            }
          } else {
            /* Malformed or legacy line containing colons */
            /* Restore first colon */
            *(--content) = ':';
            InsHistory(ctx, buffer, HST_GENERAL);
          }
        } else {
          /* Legacy format */
          InsHistory(ctx, buffer, HST_GENERAL);
        }
      }
    }
    fclose(HstFile);
  }
  return;
}

void SaveHistory(ViewContext *ctx, const char *Filename) {
  FILE *HstFile;
  int i, count;
  History *hst;
  History **hst_array;

  if (!ctx->history_head)
    return;

  if ((HstFile = fopen(Filename, "w")) == NULL)
    return;

  hst_array = (History **)xmalloc(MAX_HST_FILE_LINES * sizeof(History *));

  /* Collect pointers by traversing forward (Newest -> Oldest) */
  count = 0;
  for (hst = ctx->history_head; hst && count < MAX_HST_FILE_LINES;
       hst = hst->next) {
    hst_array[count++] = hst;
  }

  /* Write backwards (Oldest -> Newest) so ReadHistory restores correct order */
  for (i = count - 1; i >= 0; i--) {
    fprintf(HstFile, "%d:%d:%s\n", hst_array[i]->type, hst_array[i]->pinned,
            hst_array[i]->hst);
  }

  free(hst_array);
  fclose(HstFile);
}

void InsHistory(ViewContext *ctx, const char *NewHst, int type) {
  History *TMP, *TMP2 = NULL;
  int flag = 0;

  if (strlen(NewHst) == 0)
    return;

  TMP2 = ctx->history_head;
  for (TMP = ctx->history_head; TMP != NULL; TMP = TMP->next) {
    /* Match string AND type */
    if (strcmp(TMP->hst, NewHst) == 0 && TMP->type == type) {
      if (TMP2 != TMP) {
        TMP2->next = TMP->next;
        if (TMP->next)
          TMP->next->prev = TMP2; /* Fix broken double link */
        TMP->next = ctx->history_head;
        ctx->history_head = TMP;
        if (ctx->history_head->next)
          ctx->history_head->next->prev =
              ctx->history_head; /* Fix prev pointer of old head */
        ctx->history_head->prev = NULL;
      }
      flag = 1;
      break;
    };
    TMP2 = TMP;
  }

  if (flag == 0) {
    TMP = (History *)xmalloc(sizeof(struct _history));
    TMP->next = ctx->history_head;
    TMP->prev = NULL;
    TMP->hst = xstrdup(NewHst);
    TMP->type = type;
    TMP->pinned = 0;

    if (ctx->history_head != NULL)
      ctx->history_head->prev = TMP;
    ctx->history_head = TMP;
  }
  return;
}

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
    wmove(ctx->ctx_history_window, y, 1);

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
    char marker[4];
    if (pp->pinned)
      strcpy(marker, "*<");
    else
      strcpy(marker, " <");

    strcat(line_ptr, (color == CPAIR_HIHST) ? marker : "  ");
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

    /* Draw Pin Marker */
    if (pp->pinned)
      waddch(ctx->ctx_history_window, '*');
    else
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
  BuildViewList(ctx, type);

  /* Reset if list empty or smaller */
  if (ctx->total_hist == 0)
    return NULL;

  ctx->disp_begin_pos = 0;
  ctx->cursor_pos = 0;
  start_x = 0;

  UI_Dialog_Push(ctx->ctx_history_window, UI_TIER_POPOVER);

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
        BuildViewList(ctx, type);

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
      /* Toggle Pin */
      if (ctx->history_view_count > 0) {
        History *target =
            ctx->history_view_list[ctx->disp_begin_pos + ctx->cursor_pos];
        target->pinned = !target->pinned;

        BuildViewList(ctx, type);
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
