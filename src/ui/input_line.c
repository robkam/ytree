/***************************************************************************
 *
 * src/ui/input_line.c
 * Prompt & Input Manager
 *
 * Implements a managed window for user input, rendering in the footer area
 * (bottom 3 lines).
 *
 ***************************************************************************/

#include "ytree.h"
#include <ctype.h>  /* For isalnum */
#include <stdlib.h> /* For getenv */

#define PROMPT_WIN_HEIGHT 3

static BOOL is_octal_mode_string(const char *s) {
  size_t i, len;

  if (!s)
    return FALSE;

  len = strlen(s);
  if (len == 0 || len > 4)
    return FALSE;

  for (i = 0; i < len; i++) {
    if (s[i] < '0' || s[i] > '7')
      return FALSE;
  }
  return TRUE;
}

static BOOL is_mode_literal_char(int ch) {
  switch (ch) {
  case '?':
  case '-':
  case 'd':
  case 'l':
  case 'r':
  case 'w':
  case 'x':
  case 's':
  case 'S':
  case 't':
  case 'T':
    return TRUE;
  default:
    return FALSE;
  }
}

static BOOL is_date_literal_char(int ch) {
  return (isdigit((unsigned char)ch) || ch == '-' || ch == ':' || ch == ' ');
}

static void format_mode_from_octal(const char *octal_digits, char mode_type,
                                   char *out, size_t out_size) {
  mode_t mode_bits;
  mode_t synthetic_mode;
  char attrs[11];

  if (!octal_digits || !out || out_size == 0)
    return;

  mode_bits = (mode_t)strtol(octal_digits, NULL, 8);
  synthetic_mode = S_IFREG | (mode_bits & 0777);

  if (mode_bits & 04000)
    synthetic_mode |= S_ISUID;
  if (mode_bits & 02000)
    synthetic_mode |= S_ISGID;
#ifdef S_ISVTX
  if (mode_bits & 01000)
    synthetic_mode |= S_ISVTX;
#endif

  (void)GetAttributes((unsigned short)synthetic_mode, attrs);
  if (mode_type == '-' || mode_type == 'd' || mode_type == 'l' ||
      mode_type == '?') {
    attrs[0] = mode_type;
  }

  (void)snprintf(out, out_size, "%s", attrs);
}

static int normalize_prompt_escape_key(WINDOW *win, int ch) {
  int c1;

  if (!win || ch != ESC)
    return ch;

  nodelay(win, TRUE);
  c1 = wgetch(win);
  if (c1 == '[' || c1 == 'O') {
    int c2 = wgetch(win);
    switch (c2) {
    case 'A':
      ch = KEY_UP;
      break;
    case 'B':
      ch = KEY_DOWN;
      break;
    case 'C':
      ch = KEY_RIGHT;
      break;
    case 'D':
      ch = KEY_LEFT;
      break;
    default:
      if (c2 != ERR)
        ungetch(c2);
      ungetch(c1);
      break;
    }
  } else if (c1 != ERR) {
    ungetch(c1);
  }
  nodelay(win, FALSE);

  return ch;
}

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
static int UI_ReadStringInternal(ViewContext *ctx, YtreePanel *panel,
                                 const char *prompt, char *buffer, int max_len,
                                 int history_type,
                                 const char *hints_override,
                                 int (*help_callback)(ViewContext *, void *),
                                 void *help_data) {
  WINDOW *win;
  int field_width;
  int hints_row;
  int prompt_row;
  int win_y, win_x;
  int ch;
  int p = 0; /* Visual cursor position */
  int scroll_offset = 0;
  static BOOL insert_flag = TRUE;
  BOOL mode_edit = (history_type == HST_CHANGE_MODUS);
  BOOL date_overwrite_edit = (history_type == HST_GENERAL && prompt != NULL &&
                              strncmp(prompt, "DATE (", 6) == 0);
  BOOL overwrite_edit = (mode_edit || date_overwrite_edit);
  BOOL restore_insert_flag = FALSE;
  BOOL previous_insert_flag = insert_flag;
  BOOL mode_octal_entry = FALSE;
  char mode_octal_input[5] = {0};
  int mode_octal_len = 0;
  char mode_original[32] = {0};
  char mode_type = '-';
  const char *hints;

  /* Ensure buffer is valid */
  if (buffer == NULL)
    return ESC;

  /* Mode editing starts in overwrite-from-start behavior. */
  if (overwrite_edit) {
    strncpy(mode_original, buffer, sizeof(mode_original) - 1);
    mode_original[sizeof(mode_original) - 1] = '\0';
    if (mode_edit && mode_original[0] == '\0') {
      (void)snprintf(mode_original, sizeof(mode_original), "----------");
    }
    mode_type = mode_original[0];
    if (mode_edit && mode_type != '-' && mode_type != 'd' && mode_type != 'l' &&
        mode_type != '?') {
      mode_type = '-';
    }
    p = 0;
    restore_insert_flag = TRUE;
    insert_flag = FALSE;
  } else {
    /* Initial cursor position at end of string */
    p = StrVisualLength(buffer);
  }

  /* Determine Key Hints */
  if (hints_override && *hints_override) {
    hints = hints_override;
  } else if (history_type == HST_LOGIN || history_type == HST_PATH) {
    hints = "(F2) browse  (Up) history  (Enter) OK  (Esc) cancel";
  } else {
    hints = "(Up) history  (Enter) OK  (Esc) cancel";
  }

  /* Clearance: Clear the interaction area on stdscr first */
  /* Clear line above prompt to prevent footer bleed */
  if (ctx->layout.prompt_y > 0) {
    mvwhline(stdscr, ctx->layout.prompt_y - 1, 0, ' ', COLS);
  }
  mvwhline(stdscr, ctx->layout.prompt_y, 0, ' ', COLS);
  mvwhline(stdscr, ctx->layout.status_y, 0, ' ', COLS);
  wnoutrefresh(stdscr);

  /* Use the full three-line prompt area so stale footer rows cannot bleed
   * back in after browse/log interactions. */
  prompt_row = (ctx->layout.prompt_y > 0) ? 1 : 0;
  hints_row = prompt_row + 1;
  win_y = (ctx->layout.prompt_y > 0) ? ctx->layout.prompt_y - 1
                                     : ctx->layout.prompt_y;
  win_x = 0;

  /* Setup Window: blank row + prompt/input + hints */
  win = newwin(PROMPT_WIN_HEIGHT, COLS, win_y, win_x);
  if (win == NULL)
    return ESC;

  werase(win);  /* Clear window immediately to prevent footer bleed */
  UI_Dialog_Push(win, UI_TIER_FOOTER);

  keypad(win, TRUE);
  WbkgdSet(ctx, win, COLOR_PAIR(CPAIR_MENU));
  curs_set(1); /* Show cursor */

  while (1) {
    werase(win);

    /* Prompt and Input Field */
    mvwprintw(win, prompt_row, 1, "%s ", prompt);
    {
      int prompt_len = StrVisualLength(prompt) + 2;
      field_width = COLS - prompt_len - 1;

      /* Hints */
      PrintMenuOptions(win, hints_row, 1, (char *)hints, CPAIR_MENU,
                       CPAIR_HIMENUS);

      /* Handle Scrolling */
      if (p < scroll_offset) {
        scroll_offset = p;
      } else if (p >= scroll_offset + field_width) {
        scroll_offset = p - field_width + 1;
      }

      /* Extract visible portion */
      {
        int start_byte = VisualPositionToBytePosition(buffer, scroll_offset);
        char *display_str = StrLeft(&buffer[start_byte], field_width);
        int i;
        int len_drawn;

        mvwaddstr(win, prompt_row, prompt_len, display_str);

        /* Fill remaining space with underscores */
        len_drawn = StrVisualLength(display_str);
        for (i = len_drawn; i < field_width; i++) {
          waddch(win, '_');
        }

        free(display_str);

        /* Position Cursor */
        wmove(win, prompt_row, prompt_len + (p - scroll_offset));
      }
    }

    wrefresh(win);

    curs_set(1); /* Ensure cursor is visible */
    ch = WGetch(ctx, win);
    ch = normalize_prompt_escape_key(win, ch);

    FILE *df = fopen("/tmp/ytree_input.log", "a");
    if (df) {
      fprintf(df, "UI_ReadString read: %d ('%c')\n", ch,
              (ch >= 32 && ch <= 126) ? ch : '.');
      fclose(df);
    }

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
        const char *home = getenv("HOME");
        if (home) {
          /* Expand only if just "~" or "~/..." */
          if (buffer[1] == '/' || buffer[1] == '\0') {
            char expanded[PATH_LENGTH + 1];
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
      prompt_row = (ctx->layout.prompt_y > 0) ? 1 : 0;
      hints_row = prompt_row + 1;
      win_y = (ctx->layout.prompt_y > 0) ? ctx->layout.prompt_y - 1
                                         : ctx->layout.prompt_y;

      wresize(win, PROMPT_WIN_HEIGHT, COLS);
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
      if (mode_edit && mode_octal_entry) {
        if (mode_octal_len > 0) {
          char converted_mode[16];
          mode_octal_len--;
          mode_octal_input[mode_octal_len] = '\0';
          if (mode_octal_len == 0) {
            mode_octal_entry = FALSE;
            strncpy(buffer, mode_original, max_len - 1);
            buffer[max_len - 1] = '\0';
            p = 0;
          } else if (mode_octal_len < 3) {
            strncpy(buffer, mode_octal_input, max_len - 1);
            buffer[max_len - 1] = '\0';
            p = mode_octal_len;
          } else {
            format_mode_from_octal(mode_octal_input, mode_type, converted_mode,
                                   sizeof(converted_mode));
            strncpy(buffer, converted_mode, max_len - 1);
            buffer[max_len - 1] = '\0';
            p = StrVisualLength(buffer);
          }
        }
        break;
      }
      if (mode_edit) {
        if (p > 0) {
          int prev_byte = VisualPositionToBytePosition(buffer, p - 1);
          buffer[prev_byte] = '-';
          p--;
        }
        break;
      }
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
      if (mode_edit && mode_octal_entry) {
        if (mode_octal_len > 0) {
          char converted_mode[16];
          mode_octal_len--;
          mode_octal_input[mode_octal_len] = '\0';
          if (mode_octal_len == 0) {
            mode_octal_entry = FALSE;
            strncpy(buffer, mode_original, max_len - 1);
            buffer[max_len - 1] = '\0';
            p = 0;
          } else if (mode_octal_len < 3) {
            strncpy(buffer, mode_octal_input, max_len - 1);
            buffer[max_len - 1] = '\0';
            p = mode_octal_len;
          } else {
            format_mode_from_octal(mode_octal_input, mode_type, converted_mode,
                                   sizeof(converted_mode));
            strncpy(buffer, converted_mode, max_len - 1);
            buffer[max_len - 1] = '\0';
            p = StrVisualLength(buffer);
          }
        }
        break;
      }
      if (mode_edit) {
        if (p < StrVisualLength(buffer)) {
          int curr_byte = VisualPositionToBytePosition(buffer, p);
          buffer[curr_byte] = '-';
        }
        break;
      }
      if (p < StrVisualLength(buffer)) {
        int curr_byte = VisualPositionToBytePosition(buffer, p);
        int next_byte = VisualPositionToBytePosition(buffer, p + 1);
        memmove(buffer + curr_byte, buffer + next_byte,
                strlen(buffer) - next_byte + 1);
      }
      break;

    case KEY_IC:
      if (overwrite_edit)
        break;
      insert_flag = !insert_flag;
      break;

    case 'P' & 0x1F: /* Ctrl-P: history up (readline-style) */
    case KEY_UP: {
      /* GetHistory returns a pointer to internal history data.
         Do NOT free it. */
      const char *h = GetHistory(ctx, history_type);
      if (h) {
        strncpy(buffer, h, max_len - 1);
        buffer[max_len - 1] = '\0';
        p = StrVisualLength(buffer);
        if (mode_edit) {
          strncpy(mode_original, buffer, sizeof(mode_original) - 1);
          mode_original[sizeof(mode_original) - 1] = '\0';
          if (mode_original[0] == '\0')
            (void)snprintf(mode_original, sizeof(mode_original), "----------");
          mode_type = mode_original[0];
          if (mode_type != '-' && mode_type != 'd' && mode_type != 'l' &&
              mode_type != '?') {
            mode_type = '-';
          }
          if (is_octal_mode_string(buffer)) {
            mode_octal_entry = TRUE;
            mode_octal_len = (int)strlen(buffer);
            strncpy(mode_octal_input, buffer, sizeof(mode_octal_input) - 1);
            mode_octal_input[sizeof(mode_octal_input) - 1] = '\0';
          } else {
            mode_octal_entry = FALSE;
            mode_octal_len = 0;
            mode_octal_input[0] = '\0';
          }
        }
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
    case KEY_F(1):
      if (help_callback != NULL) {
        curs_set(0);
        (void)help_callback(ctx, help_data);
        touchwin(win);
        curs_set(1);
      }
      break;

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
        if (date_overwrite_edit) {
          if (!is_date_literal_char(ch)) {
            UI_Beep(ctx, FALSE);
            break;
          }
          if (p >= 19) {
            UI_Beep(ctx, FALSE);
            break;
          }
        }

        if (mode_edit) {
          if (isupper(ch))
            ch = tolower(ch);

          if (ch >= '0' && ch <= '7') {
            if (!mode_octal_entry) {
              mode_octal_entry = TRUE;
              mode_octal_len = 0;
              mode_octal_input[0] = '\0';
              scroll_offset = 0;
            }

            if (mode_octal_len >= 4) {
              UI_Beep(ctx, FALSE);
              break;
            }

            mode_octal_input[mode_octal_len++] = (char)ch;
            mode_octal_input[mode_octal_len] = '\0';

            if (mode_octal_len < 3) {
              strncpy(buffer, mode_octal_input, max_len - 1);
              buffer[max_len - 1] = '\0';
              p = mode_octal_len;
            } else {
              char converted_mode[16];
              format_mode_from_octal(mode_octal_input, mode_type,
                                     converted_mode, sizeof(converted_mode));
              strncpy(buffer, converted_mode, max_len - 1);
              buffer[max_len - 1] = '\0';
              p = StrVisualLength(buffer);
            }
            break;
          }

          mode_octal_entry = FALSE;
          mode_octal_len = 0;
          mode_octal_input[0] = '\0';

          if (!is_mode_literal_char(ch)) {
            UI_Beep(ctx, FALSE);
            break;
          }

          if ((int)strlen(buffer) < 10) {
            strncpy(buffer, mode_original, max_len - 1);
            buffer[max_len - 1] = '\0';
            p = 0;
          }

          if (p >= 10) {
            UI_Beep(ctx, FALSE);
            break;
          }
        }

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
          UI_Beep(ctx, FALSE);
        }
      }
      break;
    }
  }

  /* Cleanup */
  if (restore_insert_flag) {
    insert_flag = previous_insert_flag;
  }
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

int UI_ReadString(ViewContext *ctx, YtreePanel *panel, const char *prompt,
                  char *buffer, int max_len, int history_type) {
  return UI_ReadStringInternal(ctx, panel, prompt, buffer, max_len,
                               history_type, NULL, NULL, NULL);
}

int UI_ReadStringWithHelp(ViewContext *ctx, YtreePanel *panel,
                          const char *prompt, char *buffer, int max_len,
                          int history_type, const char *hints_override,
                          int (*help_callback)(ViewContext *, void *),
                          void *help_data) {
  return UI_ReadStringInternal(ctx, panel, prompt, buffer, max_len,
                               history_type, hints_override, help_callback,
                               help_data);
}
