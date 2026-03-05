/***************************************************************************
 *
 * src/ui/input_line.c
 * Prompt & Input Manager
 *
 * Implements a managed window for user input, rendering in the footer area
 * (bottom 3 lines) to match XTree style.
 *
 ***************************************************************************/

#include "ytree.h"
#include <ctype.h>  /* For isalnum */
#include <stdlib.h> /* For getenv */

#define PROMPT_WIN_HEIGHT 3

/* Helper to get visible length of string */
/* Using extern from input.c logic (now moved here or duplicated) */

/*
 * UI_ReadString
 * Creates a window at the bottom of the screen, displays a prompt,
 * and reads user input into buffer.
 *
 * prompt: The message to display (Row 0).
 * buffer: The buffer to store input (Row 1).
 * max_len: Maximum length of the string in the buffer.
 * history_type: ID for history management (HST_...).
 *
 * Returns: The terminating key (CR or ESC).
 */
int UI_ReadString(ViewContext *ctx, YtreePanel *panel, const char *prompt,
                  char *buffer, int max_len, int history_type) {
  WINDOW *win;
  int win_y, win_x;
  int input_y = 1; /* Row 1 */
  int input_x = 1; /* Left margin */
  int input_width;
  int ch;
  int p = 0; /* Visual cursor position */
  int scroll_offset = 0;
  static BOOL insert_flag = TRUE;
  const char *hints;

  /* Ensure buffer is valid */
  if (buffer == NULL)
    return ESC;

  /* Initial cursor position at end of string */
  p = StrVisualLength(buffer);

  /* Determine Key Hints */
  if (history_type == HST_LOGIN || history_type == HST_PATH) {
    hints = "[F2] Browse  [Up] History  [Enter] OK  [Esc] Cancel";
  } else {
    hints = "[Up] History  [Enter] OK  [Esc] Cancel";
  }

  /* Clearance: Clear the interaction area on stdscr first */
  mvwhline(stdscr, ctx->layout.prompt_y, 0, ' ', COLS);
  mvwhline(stdscr, ctx->layout.status_y, 0, ' ', COLS);
  wnoutrefresh(stdscr);

  /* win_y becomes absolute from layout */
  win_y = ctx->layout.prompt_y;
  win_x = 0;

  /* Setup Window: 2 lines (Prompt/Input and Hints) */
  win = newwin(2, COLS, win_y, win_x);
  if (win == NULL)
    return ESC;

  UI_Dialog_Push(win, UI_TIER_FOOTER);

  keypad(win, TRUE);
  WbkgdSet(ctx, win, COLOR_PAIR(CPAIR_MENU));
  curs_set(1); /* Show cursor */

  while (1) {
    werase(win);

    /* Row 0: Prompt and Input Field */
    mvwprintw(win, 0, 1, "%s ", prompt);
    int prompt_len = StrVisualLength(prompt) + 2;
    int field_width = COLS - prompt_len - 1;

    /* Row 1: Hints */
    mvwprintw(win, 1, 1, "%s", hints);

    /* Handle Scrolling */
    if (p < scroll_offset) {
      scroll_offset = p;
    } else if (p >= scroll_offset + field_width) {
      scroll_offset = p - field_width + 1;
    }

    /* Extract visible portion */
    int start_byte = VisualPositionToBytePosition(buffer, scroll_offset);
    char *display_str = StrLeft(&buffer[start_byte], field_width);

    mvwaddstr(win, 0, prompt_len, display_str);

    /* Fill remaining space with underscores */
    int len_drawn = StrVisualLength(display_str);
    int i;
    for (i = len_drawn; i < field_width; i++) {
      waddch(win, '_');
    }

    free(display_str);

    /* Position Cursor */
    wmove(win, 0, prompt_len + (p - scroll_offset));

    wrefresh(win);

    curs_set(1); /* Ensure cursor is visible */
    ch = WGetch(ctx, win);

    FILE *df = fopen("/tmp/ytree_input.log", "a");
    if (df) {
      fprintf(df, "UI_ReadString read: %d ('%c')\n", ch,
              (ch >= 32 && ch <= 126) ? ch : '.');
      fclose(df);
    }

    /*
    #ifdef VI_KEYS
        ch = ViKey(ch);
    #endif
    */

    if (ch == ESC) {
      break;
    }
    if (ch == ERR) {
      /* Wait a tiny amount to prevent CPU hang on EOF/NonBlocking */
      napms(10);
      continue;
    }
    if (ch == '\n' || ch == '\r') {
      ch = CR; /* Standardize return */

      /* Handle Tilde Expansion on Enter */
      if (buffer[0] == '~') {
        char *home = getenv("HOME");
        if (home) {
          char expanded[PATH_LENGTH + 1];
          /* Expand only if just "~" or "~/..." */
          if (buffer[1] == '/' || buffer[1] == '\0') {
            snprintf(expanded, sizeof(expanded), "%s%s", home, buffer + 1);
            strncpy(buffer, expanded, max_len - 1);
            buffer[max_len - 1] = '\0';
            /* p will be invalid relative to visual pos, but we are exiting loop
             */
          }
        }
      }

      break;
    }

    if (ctx && ctx->resize_request) {
      ctx->resize_request = FALSE;

      /* Recreate window geometry */
      win_y = ctx->layout.prompt_y;

      wresize(win, 2, COLS);
      mvwin(win, win_y, 0);

      /* Refresh background to clear artifacts */
      RefreshView(ctx, GetSelectedDirEntry(
                                 ctx, (panel ? panel->vol : ctx->active->vol)));
      touchwin(win);
      continue;
    }

    switch (ch) {
    case KEY_LEFT:
    case KEY_BTAB:
      if (p > 0)
        p--;
      break;

    case KEY_RIGHT:
      if (p < StrVisualLength(buffer))
        p++;
      break;

    case 'A' & 0x1F: /* Ctrl-A */
    case KEY_HOME:
      p = 0;
      break;

    case 'E' & 0x1F: /* Ctrl-E */
    case KEY_END:
      p = StrVisualLength(buffer);
      break;

    case 'K' & 0x1F: /* Ctrl-K: Delete to end */
    {
      int byte_pos = VisualPositionToBytePosition(buffer, p);
      buffer[byte_pos] = '\0';
    } break;

    case 'U' & 0x1F: /* Ctrl-U: Delete to start */
      if (p > 0) {
        int byte_pos = VisualPositionToBytePosition(buffer, p);
        memmove(buffer, buffer + byte_pos, strlen(buffer) - byte_pos + 1);
        p = 0;
      }
      break;

    case 'W' & 0x1F: /* Ctrl-W: Delete word left */
      if (p > 0) {
        int byte_p = VisualPositionToBytePosition(buffer, p);
        int end_of_word = byte_p;
        int start_of_word;

        /* Skip backwards over any non-alphanumeric characters */
        while (end_of_word > 0 &&
               !isalnum((unsigned char)buffer[end_of_word - 1])) {
          end_of_word--;
        }
        /* Find the start of the word */
        start_of_word = end_of_word;
        while (start_of_word > 0 &&
               isalnum((unsigned char)buffer[start_of_word - 1])) {
          start_of_word--;
        }

        if (start_of_word < byte_p) {
          memmove(buffer + start_of_word, buffer + byte_p,
                  strlen(buffer + byte_p) + 1);
          /* Recalculate p based on new string prefix */
          char c = buffer[start_of_word];
          buffer[start_of_word] = '\0';
          p = StrVisualLength(buffer);
          buffer[start_of_word] = c;
        }
      }
      break;

    case 'H' & 0x1F: /* Ctrl-H */
    case KEY_BACKSPACE:
    case 127: /* ASCII DEL sometimes maps to Backspace */
      if (p > 0) {
        int curr_byte = VisualPositionToBytePosition(buffer, p);
        int prev_byte = VisualPositionToBytePosition(buffer, p - 1);
        memmove(buffer + prev_byte, buffer + curr_byte,
                strlen(buffer) - curr_byte + 1);
        p--;
      }
      break;

    case 'D' & 0x1F: /* Ctrl-D */
    case KEY_DC:     /* Delete Key */
      if (p < StrVisualLength(buffer)) {
        int curr_byte = VisualPositionToBytePosition(buffer, p);
        int next_byte = VisualPositionToBytePosition(buffer, p + 1);
        memmove(buffer + curr_byte, buffer + next_byte,
                strlen(buffer) - next_byte + 1);
      }
      break;

    case KEY_IC:
      insert_flag = !insert_flag;
      break;

    case KEY_UP: {
      /* GetHistory returns a pointer to internal history data.
         Do NOT free it. */
      char *h = GetHistory(ctx, history_type);
      if (h) {
        strncpy(buffer, h, max_len - 1);
        buffer[max_len - 1] = '\0';
        p = StrVisualLength(buffer);
      }
    } break;

    case '\t': {
      /* GetMatches allocates a new string. MUST free it. */
      char *match = GetMatches(ctx, buffer);
      if (match) {
        strncpy(buffer, match, max_len - 1);
        buffer[max_len - 1] = '\0';
        p = StrVisualLength(buffer);
        free(match);
      }
    } break;

#ifdef KEY_F
    case KEY_F(2):
#endif
    case 'F' & 0x1f:
      if (history_type == HST_LOGIN || history_type == HST_PATH) {
        char path[PATH_LENGTH + 1];
        /* F2 Directory Selection */
        /* Note: KeyF2Get handles saving/restoring the screen context internally
         */
        if (KeyF2Get(ctx, panel, path) == 0) {
          if (*path) {
            strncpy(buffer, path, max_len - 1);
            buffer[max_len - 1] = '\0';
            p = StrVisualLength(buffer);
            /* Force refresh of global view before we redraw our window,
               just in case KeyF2Get left things dirty */
            RefreshView(
                ctx, GetSelectedDirEntry(
                         ctx, (panel ? panel->vol : ctx->active->vol)));
            touchwin(win);
          }
        } else {
          /* Cancelled or error, just ensure background is clean */
          RefreshView(ctx,
                            GetSelectedDirEntry(
                                ctx, (panel ? panel->vol : ctx->active->vol)));
          touchwin(win);
        }
      }
      break;

    default:
      if (ch >= ' ' && ch <= '~') {
        if (strlen(buffer) < (size_t)(max_len - 1)) {
          int byte_pos = VisualPositionToBytePosition(buffer, p);
          if (insert_flag) {
            memmove(buffer + byte_pos + 1, buffer + byte_pos,
                    strlen(buffer) - byte_pos + 1);
          }
          buffer[byte_pos] = ch;
          if (!insert_flag && buffer[byte_pos + 1] == '\0') {
            /* Overwrite mode at end extends string */
            buffer[byte_pos + 1] = '\0';
          }
          p++;
        } else {
          beep();
        }
      }
      break;
    }
  }

  /* Cleanup */
  curs_set(0);
  UI_Dialog_Close(ctx, win);

  /* Update History on success */
  if (ch == CR && buffer[0] != '\0') {
    InsHistory(ctx, buffer, history_type);
  }

  /* Global Refresh to restore what was behind the window */
  RefreshView(
      ctx, GetSelectedDirEntry(ctx, (panel ? panel->vol : ctx->active->vol)));

  return ch;
}