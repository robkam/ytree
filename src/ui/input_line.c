/***************************************************************************
 *
 * src/ui/input_line.c
 * Prompt & Input Manager
 *
 * Implements a managed window for user input, rendering in the footer area
 * (bottom 3 lines).
 *
 ***************************************************************************/

#include "ytnova_appstate_render.h"
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
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
static const UICommandStripCommand read_string_path_hint_commands[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "browse", "F2", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "history", "Up", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "OK", "Enter", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "cancel", "Esc", NULL}};
static const UICommandStripCommand read_string_help_hint_commands[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "help", "F1", NULL}};
static const UICommandStripCommand read_string_history_hint_commands[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "history", "Up", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "OK", "Enter", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "cancel", "Esc", NULL}};

typedef struct {
  ViewContext *ctx;
  YtreeNovaPanel *panel;
  WINDOW *win;
  const char *prompt;
  char *buffer;
  int max_len;
  int history_type;
  int ch;
  int p;
  int scroll_offset;
  int field_width;
  int prompt_row;
  int hints_row;
  int win_y;
  BOOL mode_edit;
  BOOL date_overwrite_edit;
  BOOL overwrite_edit;
  BOOL restore_insert_flag;
  BOOL saved_insert_flag;
  BOOL insert_flag;
  BOOL accept_special_term;
  BOOL mode_octal_entry;
  char mode_octal_input[5];
  int mode_octal_len;
  char mode_original[32];
  char mode_type;
  const UICommandStripCommand *hints;
  size_t hint_count;
  int (*help_callback)(ViewContext *, void *);
  void *help_data;
  UIPromptActionHandler action_handler;
  void *action_data;
} PromptSession;

static DirEntry *PromptSessionSelectedEntry(PromptSession *session) {
  if (!session || !session->ctx)
    return NULL;
  return GetSelectedDirEntry(
      session->ctx,
      (session->panel ? session->panel->vol : session->ctx->active->vol));
}

static void PromptSessionRefreshBackground(PromptSession *session) {
  if (!session || !session->ctx)
    return;
  RefreshView(session->ctx, PromptSessionSelectedEntry(session));
}

static void PromptSessionComputeWindowRows(PromptSession *session) {
  session->prompt_row = (session->ctx->layout.prompt_y > 0) ? 1 : 0;
  session->hints_row = session->prompt_row + 1;
  session->win_y = (session->ctx->layout.prompt_y > 0)
                       ? session->ctx->layout.prompt_y - 1
                       : session->ctx->layout.prompt_y;
}

static void PromptSessionResolveHints(
    PromptSession *session, const UICommandStripCommand *hints_override,
    size_t hints_override_count) {
  if (hints_override != NULL && hints_override_count > 0) {
    session->hints = hints_override;
    session->hint_count = hints_override_count;
  } else if (session->history_type == HST_LOG ||
             session->history_type == HST_PATH) {
    session->hints = read_string_path_hint_commands;
    session->hint_count = sizeof(read_string_path_hint_commands) /
                          sizeof(read_string_path_hint_commands[0]);
  } else {
    session->hints = read_string_history_hint_commands;
    session->hint_count = sizeof(read_string_history_hint_commands) /
                          sizeof(read_string_history_hint_commands[0]);
  }
}

static void PromptSessionLoadModeOriginal(PromptSession *session) {
  strncpy(session->mode_original, session->buffer,
          sizeof(session->mode_original) - 1);
  session->mode_original[sizeof(session->mode_original) - 1] = '\0';
  if (session->mode_edit && session->mode_original[0] == '\0') {
    (void)snprintf(session->mode_original, sizeof(session->mode_original),
                   "----------");
  }

  session->mode_type = session->mode_original[0];
  if (session->mode_edit && session->mode_type != '-' &&
      session->mode_type != 'd' && session->mode_type != 'l' &&
      session->mode_type != '?') {
    session->mode_type = '-';
  }

  if (session->mode_edit && is_octal_mode_string(session->buffer)) {
    session->mode_octal_entry = TRUE;
    session->mode_octal_len = (int)strlen(session->buffer);
    strncpy(session->mode_octal_input, session->buffer,
            sizeof(session->mode_octal_input) - 1);
    session->mode_octal_input[sizeof(session->mode_octal_input) - 1] = '\0';
  } else {
    session->mode_octal_entry = FALSE;
    session->mode_octal_len = 0;
    session->mode_octal_input[0] = '\0';
  }
}

static void PromptSessionPrepareInitialState(PromptSession *session) {
  if (session->overwrite_edit) {
    PromptSessionLoadModeOriginal(session);
    session->p = 0;
    session->restore_insert_flag = TRUE;
    session->insert_flag = FALSE;
  } else {
    session->p = StrVisualLength(session->buffer);
  }
}

static void PromptSessionClearPromptArea(PromptSession *session) {
  if (session->ctx->layout.prompt_y > 0) {
    mvwhline(stdscr, session->ctx->layout.prompt_y - 1, 0, ' ', COLS);
  }
  mvwhline(stdscr, session->ctx->layout.prompt_y, 0, ' ', COLS);
  mvwhline(stdscr, session->ctx->layout.status_y, 0, ' ', COLS);
  wnoutrefresh(stdscr);
}

static void PromptSessionExpandTildeOnAccept(PromptSession *session) {
  if (session->buffer[0] == '~') {
    const char *home = getenv("HOME");

    if (home && (session->buffer[1] == '/' || session->buffer[1] == '\0')) {
      char expanded[PATH_LENGTH + 1];

      snprintf(expanded, sizeof(expanded), "%s%s", home, session->buffer + 1);
      strncpy(session->buffer, expanded, session->max_len - 1);
      session->buffer[session->max_len - 1] = '\0';
    }
  }
}

static void PromptSessionHandleResize(PromptSession *session) {
  PromptSessionComputeWindowRows(session);
  wresize(session->win, PROMPT_WIN_HEIGHT, COLS);
  mvwin(session->win, session->win_y, 0);
  PromptSessionRefreshBackground(session);
  touchwin(session->win);
}

static void PromptSessionUpdateModeBufferFromOctal(PromptSession *session) {
  if (session->mode_octal_len == 0) {
    session->mode_octal_entry = FALSE;
    strncpy(session->buffer, session->mode_original, session->max_len - 1);
    session->buffer[session->max_len - 1] = '\0';
    session->p = 0;
  } else if (session->mode_octal_len < 3) {
    strncpy(session->buffer, session->mode_octal_input, session->max_len - 1);
    session->buffer[session->max_len - 1] = '\0';
    session->p = session->mode_octal_len;
  } else {
    char converted_mode[16];

    format_mode_from_octal(session->mode_octal_input, session->mode_type,
                           converted_mode, sizeof(converted_mode));
    strncpy(session->buffer, converted_mode, session->max_len - 1);
    session->buffer[session->max_len - 1] = '\0';
    session->p = StrVisualLength(session->buffer);
  }
}

static BOOL PromptSessionHandleModeOctalDelete(PromptSession *session) {
  if (!session->mode_edit || !session->mode_octal_entry)
    return FALSE;
  if (session->mode_octal_len > 0) {
    session->mode_octal_len--;
    session->mode_octal_input[session->mode_octal_len] = '\0';
    PromptSessionUpdateModeBufferFromOctal(session);
  }
  return TRUE;
}

static BOOL PromptSessionHandleNavigationKey(PromptSession *session) {
  switch (session->ch) {
  case KEY_LEFT:
  case KEY_BTAB:
    if (session->p > 0)
      session->p--;
    return TRUE;

  case KEY_RIGHT:
    if (session->p < StrVisualLength(session->buffer))
      session->p++;
    return TRUE;

  case 'A' & 0x1F:
  case KEY_HOME:
    session->p = 0;
    return TRUE;

  case 'E' & 0x1F:
  case KEY_END:
    session->p = StrVisualLength(session->buffer);
    return TRUE;

  case 'K' & 0x1F: {
    int byte_pos = VisualPositionToBytePosition(session->buffer, session->p);

    session->buffer[byte_pos] = '\0';
    return TRUE;
  }

  case 'U' & 0x1F:
    if (session->p > 0) {
      int byte_pos = VisualPositionToBytePosition(session->buffer, session->p);

      memmove(session->buffer, session->buffer + byte_pos,
              strlen(session->buffer) - byte_pos + 1);
      session->p = 0;
    }
    return TRUE;

  case 'W' & 0x1F:
    if (session->p > 0) {
      int byte_p = VisualPositionToBytePosition(session->buffer, session->p);
      int end_of_word = byte_p;
      int start_of_word;

      while (end_of_word > 0 &&
             !isalnum((unsigned char)session->buffer[end_of_word - 1])) {
        end_of_word--;
      }
      start_of_word = end_of_word;
      while (start_of_word > 0 &&
             isalnum((unsigned char)session->buffer[start_of_word - 1])) {
        start_of_word--;
      }

      if (start_of_word < byte_p) {
        char saved_char;

        memmove(session->buffer + start_of_word, session->buffer + byte_p,
                strlen(session->buffer + byte_p) + 1);
        saved_char = session->buffer[start_of_word];
        session->buffer[start_of_word] = '\0';
        session->p = StrVisualLength(session->buffer);
        session->buffer[start_of_word] = saved_char;
      }
    }
    return TRUE;

  default:
    return FALSE;
  }
}

static BOOL PromptSessionHandleDeletionKey(PromptSession *session) {
  switch (session->ch) {
  case 'H' & 0x1F:
  case KEY_BACKSPACE:
  case 127:
    if (PromptSessionHandleModeOctalDelete(session))
      return TRUE;
    if (session->mode_edit) {
      if (session->p > 0) {
        int prev_byte =
            VisualPositionToBytePosition(session->buffer, session->p - 1);

        session->buffer[prev_byte] = '-';
        session->p--;
      }
      return TRUE;
    }
    if (session->p > 0) {
      int curr_byte = VisualPositionToBytePosition(session->buffer, session->p);
      int prev_byte =
          VisualPositionToBytePosition(session->buffer, session->p - 1);

      memmove(session->buffer + prev_byte, session->buffer + curr_byte,
              strlen(session->buffer) - curr_byte + 1);
      session->p--;
    }
    return TRUE;

  case 'D' & 0x1F:
  case KEY_DC:
    if (PromptSessionHandleModeOctalDelete(session))
      return TRUE;
    if (session->mode_edit) {
      if (session->p < StrVisualLength(session->buffer)) {
        int curr_byte =
            VisualPositionToBytePosition(session->buffer, session->p);

        session->buffer[curr_byte] = '-';
      }
      return TRUE;
    }
    if (session->p < StrVisualLength(session->buffer)) {
      int curr_byte = VisualPositionToBytePosition(session->buffer, session->p);
      int next_byte =
          VisualPositionToBytePosition(session->buffer, session->p + 1);

      memmove(session->buffer + curr_byte, session->buffer + next_byte,
              strlen(session->buffer) - next_byte + 1);
    }
    return TRUE;

  case KEY_IC:
    if (!session->overwrite_edit)
      session->insert_flag = !session->insert_flag;
    return TRUE;

  default:
    return FALSE;
  }
}

static BOOL PromptSessionHandleHistoryKey(PromptSession *session) {
  switch (session->ch) {
  case 'P' & 0x1F:
  case KEY_UP: {
    const char *history = GetHistory(session->ctx, session->history_type);

    if (history) {
      strncpy(session->buffer, history, session->max_len - 1);
      session->buffer[session->max_len - 1] = '\0';
      session->p = StrVisualLength(session->buffer);
      if (session->mode_edit)
        PromptSessionLoadModeOriginal(session);
    }
    return TRUE;
  }

  case '\t':
    if (session->history_type == HST_FILTER) {
      session->accept_special_term = TRUE;
      return TRUE;
    }
    {
      char *match = GetMatches(session->ctx, session->buffer);

      if (match) {
        strncpy(session->buffer, match, session->max_len - 1);
        session->buffer[session->max_len - 1] = '\0';
        session->p = StrVisualLength(session->buffer);
        free(match);
      }
    }
    return TRUE;

  default:
    return FALSE;
  }
}

static BOOL PromptSessionHandleActionKey(PromptSession *session) {
  switch (session->ch) {
#ifdef KEY_F
  case KEY_F(1):
    if (session->help_callback != NULL) {
      curs_set(0);
      (void)session->help_callback(session->ctx, session->help_data);
      touchwin(session->win);
      curs_set(1);
    }
    return TRUE;

  case KEY_F(2):
#endif
  case 'F' & 0x1f:
    if (session->history_type == HST_LOG || session->history_type == HST_PATH) {
      char path[PATH_LENGTH + 1];

      if (KeyF2Get(session->ctx, session->panel, path) == 0) {
        if (*path) {
          strncpy(session->buffer, path, session->max_len - 1);
          session->buffer[session->max_len - 1] = '\0';
          session->p = StrVisualLength(session->buffer);
        }
      }
      PromptSessionRefreshBackground(session);
      touchwin(session->win);
    }
    return TRUE;

  default:
    return FALSE;
  }
}

static BOOL PromptSessionHandlePromptAction(PromptSession *session) {
  if (!session || session->action_handler == NULL)
    return FALSE;

  if (!session->action_handler(session->ctx, session->panel, session->ch,
                               session->buffer, session->max_len, &session->p,
                               session->action_data)) {
    return FALSE;
  }

  if (session->p < 0)
    session->p = 0;
  if (session->p > StrVisualLength(session->buffer))
    session->p = StrVisualLength(session->buffer);
  return TRUE;
}

static BOOL PromptSessionHandlePrintableKey(PromptSession *session) {
  int ch = session->ch;

  if (ch < ' ' || ch > '~')
    return FALSE;

  if (session->date_overwrite_edit) {
    if (!is_date_literal_char(ch)) {
      UI_Beep(session->ctx, FALSE);
      return TRUE;
    }
    if (session->p >= 19) {
      UI_Beep(session->ctx, FALSE);
      return TRUE;
    }
  }

  if (session->mode_edit) {
    if (isupper(ch))
      ch = tolower(ch);

    if (ch >= '0' && ch <= '7') {
      if (!session->mode_octal_entry) {
        session->mode_octal_entry = TRUE;
        session->mode_octal_len = 0;
        session->mode_octal_input[0] = '\0';
        session->scroll_offset = 0;
      }

      if (session->mode_octal_len >= 4) {
        UI_Beep(session->ctx, FALSE);
        return TRUE;
      }

      session->mode_octal_input[session->mode_octal_len++] = (char)ch;
      session->mode_octal_input[session->mode_octal_len] = '\0';
      PromptSessionUpdateModeBufferFromOctal(session);
      return TRUE;
    }

    session->mode_octal_entry = FALSE;
    session->mode_octal_len = 0;
    session->mode_octal_input[0] = '\0';

    if (!is_mode_literal_char(ch)) {
      UI_Beep(session->ctx, FALSE);
      return TRUE;
    }

    if ((int)strlen(session->buffer) < 10) {
      strncpy(session->buffer, session->mode_original, session->max_len - 1);
      session->buffer[session->max_len - 1] = '\0';
      session->p = 0;
    }

    if (session->p >= 10) {
      UI_Beep(session->ctx, FALSE);
      return TRUE;
    }
  }

  if (strlen(session->buffer) < (size_t)(session->max_len - 1)) {
    int byte_pos = VisualPositionToBytePosition(session->buffer, session->p);

    if (session->insert_flag) {
      memmove(session->buffer + byte_pos + 1, session->buffer + byte_pos,
              strlen(session->buffer) - byte_pos + 1);
    }
    session->buffer[byte_pos] = ch;
    if (!session->insert_flag && session->buffer[byte_pos + 1] == '\0') {
      session->buffer[byte_pos + 1] = '\0';
    }
    session->p++;
  } else {
    UI_Beep(session->ctx, FALSE);
  }

  return TRUE;
}

static int UI_ReadStringInternal(ViewContext *ctx, YtreeNovaPanel *panel,
                                 const char *prompt, char *buffer, int max_len,
                                 int history_type,
                                 const UIPromptOptions *options) {
  static BOOL insert_flag = TRUE;
  PromptSession session;
  WINDOW *win;
  int hints_row;
  int prompt_row;
  const UICommandStripCommand *hints;
  size_t hint_count;

  if (buffer == NULL)
    return ESC;

  memset(&session, 0, sizeof(session));
  session.ctx = ctx;
  session.panel = panel;
  session.prompt = prompt;
  session.buffer = buffer;
  session.max_len = max_len;
  session.history_type = history_type;
  session.mode_edit = (history_type == HST_CHANGE_MODUS);
  session.date_overwrite_edit =
      (history_type == HST_GENERAL && prompt != NULL &&
       strncmp(prompt, "DATE ", sizeof("DATE ") - 1) == 0);
  session.overwrite_edit = (session.mode_edit || session.date_overwrite_edit);
  session.saved_insert_flag = insert_flag;
  session.insert_flag = insert_flag;
  session.mode_type = '-';
  if (options != NULL) {
    session.help_callback = options->help_callback;
    session.help_data = options->help_data;
    session.action_handler = options->action_handler;
    session.action_data = options->action_data;
  }

  PromptSessionPrepareInitialState(&session);
  PromptSessionResolveHints(&session, options ? options->hints_override : NULL,
                            options ? options->hints_override_count : 0);
  hints = session.hints;
  hint_count = session.hint_count;
  PromptSessionClearPromptArea(&session);
  PromptSessionComputeWindowRows(&session);
  prompt_row = session.prompt_row;
  hints_row = session.hints_row;

  win = newwin(PROMPT_WIN_HEIGHT, COLS, session.win_y, 0);
  if (win == NULL)
    return ESC;
  session.win = win;

  werase(win);
  UI_Dialog_Push(win, UI_TIER_FOOTER);
  keypad(win, TRUE);
  WbkgdSet(ctx, win, COLOR_PAIR(UI_ROLE_DIALOG));
  curs_set(1);

  while (1) {
    int field_width;
    int prompt_len;

    werase(win);
    mvwprintw(win, prompt_row, 1, "%s ", prompt);
    prompt_len = StrVisualLength(prompt) + 2;
    field_width = COLS - prompt_len - 1;
    session.field_width = field_width;

    if (session.help_callback != NULL) {
      int hint_x = 1;

      hint_x += UI_RenderAdaptiveCommandStrip(
          win, hints_row, hint_x, read_string_help_hint_commands,
          sizeof(read_string_help_hint_commands) /
              sizeof(read_string_help_hint_commands[0]),
          UI_ROLE_DIALOG, UI_ROLE_KEYBIND);
      if (hints != NULL && hint_count > 0)
        hint_x += 2;
      UI_RenderAdaptiveCommandStrip(win, hints_row, hint_x, hints, hint_count,
                                    UI_ROLE_DIALOG, UI_ROLE_KEYBIND);
    } else {
      UI_RenderAdaptiveCommandStrip(win, hints_row, 1, hints, hint_count,
                                    UI_ROLE_DIALOG, UI_ROLE_KEYBIND);
    }

    if (session.p < session.scroll_offset) {
      session.scroll_offset = session.p;
    } else if (session.p >= session.scroll_offset + field_width) {
      session.scroll_offset = session.p - field_width + 1;
    }

    {
      int start_byte =
          VisualPositionToBytePosition(buffer, session.scroll_offset);
      char *display_str = StrLeft(&buffer[start_byte], field_width);
      int i;
      int len_drawn;

      mvwaddstr(win, prompt_row, prompt_len, display_str);
      len_drawn = StrVisualLength(display_str);
      for (i = len_drawn; i < field_width; i++) {
        waddch(win, '_');
      }
      free(display_str);
      wmove(win, prompt_row, prompt_len + (session.p - session.scroll_offset));
    }

    wrefresh(win);
    curs_set(1);
    session.ch = WGetch(ctx, win);
    session.ch = normalize_prompt_escape_key(win, session.ch);

    if (session.ch == ESC) {
      break;
    }
    if (session.ch == ERR) {
      napms(10);
      continue;
    }
    if (session.ch == '\n' || session.ch == '\r') {
      session.ch = CR;
      PromptSessionExpandTildeOnAccept(&session);
      break;
    }

    if (ctx && ctx->resize_request) {
      (void)AppStateClearResizeRequest(ctx);
      PromptSessionHandleResize(&session);
      prompt_row = session.prompt_row;
      hints_row = session.hints_row;
      continue;
    }

    if (PromptSessionHandleNavigationKey(&session) ||
        PromptSessionHandleDeletionKey(&session) ||
        PromptSessionHandleHistoryKey(&session) ||
        PromptSessionHandleActionKey(&session) ||
        PromptSessionHandlePromptAction(&session) ||
        PromptSessionHandlePrintableKey(&session)) {
      if (session.accept_special_term)
        break;
      continue;
    }

    if (session.accept_special_term)
      break;
  }

  insert_flag =
      session.restore_insert_flag ? session.saved_insert_flag : session.insert_flag;
  curs_set(0);
  UI_Dialog_Close(ctx, win);

  if (session.ch == CR && buffer[0] != '\0') {
    InsHistory(ctx, buffer, history_type);
  }

  PromptSessionRefreshBackground(&session);
  return session.ch;
}

int UI_ReadString(ViewContext *ctx, YtreeNovaPanel *panel, const char *prompt,
                  char *buffer, int max_len, int history_type) {
  return UI_ReadStringInternal(ctx, panel, prompt, buffer, max_len,
                               history_type, NULL);
}

int UI_ReadStringWithPromptOptions(
    ViewContext *ctx, YtreeNovaPanel *panel, const char *prompt, char *buffer,
    int max_len, int history_type, const UIPromptOptions *options) {
  return UI_ReadStringInternal(ctx, panel, prompt, buffer, max_len,
                               history_type, options);
}

int UI_ReadStringWithHelp(ViewContext *ctx, YtreeNovaPanel *panel,
                          const char *prompt, char *buffer, int max_len,
                          int history_type,
                          const UICommandStripCommand *hints_override,
                          size_t hints_override_count,
                          int (*help_callback)(ViewContext *, void *),
                          void *help_data) {
  UIPromptOptions options;

  memset(&options, 0, sizeof(options));
  options.hints_override = hints_override;
  options.hints_override_count = hints_override_count;
  options.help_callback = help_callback;
  options.help_data = help_data;
  return UI_ReadStringInternal(ctx, panel, prompt, buffer, max_len,
                               history_type, &options);
}
