/***************************************************************************
 * src/ui/key_engine.c
 * Input Handling Utilities
 *
 * Contains low-level input helpers. The main string input logic has moved
 * to input_line.c (UI_ReadString).
 ***************************************************************************/

#include "watcher.h"
#include "ytree.h"
#include "ytree_cmd.h"
#include <ctype.h>
#include <curses.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

#ifdef READLINE_SUPPORT
#include <readline/tilde.h>
#endif

/* External declarations for Signal Safety enforcement */

/* Wrapper function to satisfy tputs(..., int (*putc_func)(int)) signature */
/* It writes the character 'c' to standard output. */
static int term_putc(int c) { return fputc(c, stdout); }

char *StrLeft(const char *str, size_t visible_count) {
  char *result;
  size_t len;
  int left_bytes;

#ifdef WITH_UTF8
  mbstate_t state;
  const char *s, *s_start;
  size_t pos = 0;
#endif

  if (visible_count == 0)
    return (xstrdup(""));

  len = StrVisualLength(str);
  if (visible_count >= len)
    return (xstrdup(str));

#ifdef WITH_UTF8

  s_start = s = str;

  while (*s) {

    wchar_t wc;
    size_t sz;
    int width;

    s_start = s;
    memset(&state, 0, sizeof(state));
    sz = mbrtowc(&wc, s, MB_CUR_MAX, &state);

    if (sz == (size_t)-1 || sz == (size_t)-2 || sz == 0) {
      s++;
      width = 1;
    } else {
      s += sz;
      width = wcwidth(wc);
      if (width < 0)
        width = 1;
    }

    if (pos + (size_t)width > visible_count)
      break;

    pos += width;
  }

  left_bytes = s_start - str;

#else
  left_bytes = visible_count;
#endif

  result = xmalloc(left_bytes + 1);
  memcpy(result, str, left_bytes);
  result[left_bytes] = '\0';
  return (result);
}

char *StrRight(const char *str, size_t visible_count) {
  char *result;
  size_t visual_len;

#ifdef WITH_UTF8
  int left_bytes;
  mbstate_t state;
  const char *s, *s_start;
  int pos_start = 0;
#endif

  if (visible_count == 0)
    return (xstrdup(""));

  visual_len = StrVisualLength(str);
  if (visual_len <= visible_count)
    return (xstrdup(str));

#ifdef WITH_UTF8

  s_start = s = str;

  while (*s) {

    wchar_t wc;
    size_t sz;
    int width;

    s_start = s;
    memset(&state, 0, sizeof(state));
    sz = mbrtowc(&wc, s, MB_CUR_MAX, &state);

    if (sz == (size_t)-1 || sz == (size_t)-2 || sz == 0) {
      s++;
      width = 1;
    } else {
      s += sz;
      width = wcwidth(wc);
      if (width < 0)
        width = 1;
    }

    if ((visual_len - (pos_start + width)) < visible_count)
      break;

    pos_start += width;
  }

  left_bytes = s_start - str;

  result = xstrdup(&str[left_bytes]);

#else
  result = xstrdup(&str[visual_len - visible_count]);
#endif

  return (result);
}

int StrVisualLength(const char *str) {
  int len;

#ifdef WITH_UTF8

  int pos = 0;
  mbstate_t state;
  const char *s = str;

  while (*s) {
    wchar_t wc;
    int width;

    memset(&state, 0, sizeof(state));
    size_t sz = mbrtowc(&wc, s, MB_CUR_MAX, &state);

    if (sz == (size_t)-1 || sz == (size_t)-2 || sz == 0) {
      s++;
      width = 1;
    } else {
      s += sz;
      width = wcwidth(wc);
      if (width < 0)
        width = 1;
    }
    pos += width;
  }
  len = pos;

#else
  len = strlen(str);
#endif

  return len;
}

/* returns byte position for visual position */
int VisualPositionToBytePosition(const char *str, int visual_pos) {

#ifdef WITH_UTF8

  mbstate_t state;
  const char *s;
  int pos = 0;

  s = str;

  while (*s) {

    wchar_t wc;
    size_t sz;
    int width;

    const char *s_start = s;
    memset(&state, 0, sizeof(state));
    sz = mbrtowc(&wc, s, MB_CUR_MAX, &state);

    if (sz == (size_t)-1 || sz == (size_t)-2 || sz == 0) {
      s++;
      width = 1;
    } else {
      s += sz;
      width = wcwidth(wc);
      if (width < 0)
        width = 1;
    }

    if (pos + width > visual_pos)
      return (s_start - str);

    pos += width;
  }

  return (s - str);

#else
  return visual_pos;
#endif
}

int InputChoice(ViewContext *ctx, const char *msg, const char *term) {
  int c;

  ClearHelp(ctx);

  curs_set(1);
  leaveok(ctx->ctx_border_window, FALSE);
  mvwhline(ctx->ctx_border_window, ctx->layout.prompt_y, 1, ' ', COLS - 2);
  PrintMenuOptions(ctx->ctx_border_window, ctx->layout.prompt_y, 1, (char *)msg,
                   CPAIR_MENU, CPAIR_HIMENUS);
  wnoutrefresh(ctx->ctx_border_window);
  doupdate();
  do {
    c = WGetch(ctx, ctx->ctx_border_window);
    if (c == ESC)
      break;
    if (c >= 0)
      if (islower(c))
        c = toupper(c);
  } while (c != -1 && !strchr(term, c));

  mvwaddstr(ctx->ctx_border_window, ctx->layout.prompt_y, 1, " ");
  mvwhline(ctx->ctx_border_window, ctx->layout.prompt_y, 1, ' ', COLS - 2);
  wnoutrefresh(ctx->ctx_border_window);
  leaveok(ctx->ctx_border_window, TRUE);
  curs_set(0);
  doupdate();

  return (c);
}

void HitReturnToContinue(void) {
#if !defined(XCURSES)
  char *te;

  char *tgetstr(const char *, char **);
  int tputs(const char *, int, int (*)(int));

  curs_set(1);

  vidattr(A_REVERSE);

  putp("[Hit return to continue]");
  (void)fflush(stdout);

  (void)getchar();

  te = tgetstr("me", NULL);
  if (te != NULL) {
    tputs(te, 1, term_putc);
  } else {
    putp("\033[0m");
  }
  (void)fflush(stdout);

#endif

  curs_set(0);
  doupdate();
}

BOOL KeyPressed() {
  BOOL pressed = FALSE;

  nodelay(stdscr, TRUE);
  int c = wgetch(stdscr);
  if (c != ERR) {
    pressed = TRUE;
    ungetch(c);

    FILE *l = fopen("/tmp/ytree_wgetch.log", "a");
    if (l) {
      fprintf(l, "KeyPressed() saw: %3d ('%c') - UNGETTING\n", c,
              (c >= 32 && c <= 126) ? c : '.');
      fclose(l);
    }
  }
  nodelay(stdscr, FALSE);

  return (pressed);
}

BOOL EscapeKeyPressed(void) {
  int c;
  BOOL pressed = FALSE;

  nodelay(stdscr, TRUE);
  if ((c = wgetch(stdscr)) != ERR) {
    FILE *l = fopen("/tmp/ytree_wgetch.log", "a");
    if (l) {
      fprintf(l, "EscapeKeyPressed() saw: %3d ('%c')\n", c,
              (c >= 32 && c <= 126) ? c : '.');
      fclose(l);
    }

    if (c == ESC) {
      pressed = TRUE;
    } else {
      ungetch(c);
    }
  }
  nodelay(stdscr, FALSE);

  return (pressed);
}

int ViKey(int ch) {
  switch (ch) {
  case VI_KEY_UP:
    ch = KEY_UP;
    break;
  case VI_KEY_DOWN:
    ch = KEY_DOWN;
    break;
  case VI_KEY_RIGHT:
    ch = KEY_RIGHT;
    break;
  case VI_KEY_LEFT:
    ch = KEY_LEFT;
    break;
  case VI_KEY_PPAGE:
    ch = KEY_PPAGE;
    break;
  case VI_KEY_NPAGE:
    ch = KEY_NPAGE;
    break;
  }
  return (ch);
}

BOOL IsViKeysEnabled(const ViewContext *ctx) {
  if (!ctx || !ctx->profile_data)
    return FALSE;
  return (strtol(GetProfileValue(ctx, "VI_KEYS"), NULL, 0)) ? TRUE : FALSE;
}

int NormalizeViKey(const ViewContext *ctx, int ch) {
  if (IsViKeysEnabled(ctx))
    return ViKey(ch);
  return ch;
}

YtreeAction GetKeyAction(const ViewContext *ctx, int ch) {
  BOOL vi_keys_enabled = IsViKeysEnabled(ctx);
  if (vi_keys_enabled)
    ch = ViKey(ch);

  switch (ch) {
  case KEY_UP:
    return ACTION_MOVE_UP;
  case KEY_DOWN:
    return ACTION_MOVE_DOWN;
  case KEY_LEFT:
    return ACTION_MOVE_LEFT;
  case KEY_RIGHT:
    return ACTION_MOVE_RIGHT;
  case KEY_PPAGE:
    return ACTION_PAGE_UP;
  case KEY_NPAGE:
    return ACTION_PAGE_DOWN;
  case KEY_HOME:
    return ACTION_HOME;
  case KEY_END:
    return ACTION_END;

  case '\t':
    return (ctx && ctx->is_split_screen) ? ACTION_SWITCH_PANEL
                                         : ACTION_MOVE_SIBLING_NEXT;
  case '*':
    return ACTION_ASTERISK;
  case KEY_BTAB:
    return ACTION_MOVE_SIBLING_PREV;
  case '-':
    return ACTION_TREE_COLLAPSE;
  case '+':
    return ACTION_TREE_EXPAND_ALL;
  case '/':
    return ACTION_LIST_JUMP;
  case '\\':
    return ACTION_TO_DIR;

  case CR:
  case LF:
    return ACTION_ENTER;
  case ESC:
    return ACTION_ESCAPE;
  case 'l':
  case 'L':
    return ACTION_LOG;
  case 'q':
  case 'Q':
    return ACTION_QUIT;
  case 0x11:
    return ACTION_QUIT_DIR;
  case 't':
  case 'T':
    return ACTION_TAG;
  case 'u':
    return ACTION_UNTAG;
  case 'U':
    /* In vi-key mode, Ctrl-U is reserved for page-up navigation.
     * Use uppercase U as the file-window "untag all" command key.
     */
    if (vi_keys_enabled && ctx && ctx->focused_window == FOCUS_FILE)
      return ACTION_UNTAG_ALL;
    return ACTION_UNTAG;
  case 0x14: /* Ctrl+T */
    return ACTION_TAG_ALL;
  case 0x15: /* Ctrl+U */
    return ACTION_UNTAG_ALL;
  case ';':
    return ACTION_TAG_REST;
  case ':':
    return ACTION_UNTAG_REST;
  case 'f':
  case 'F':
    return ACTION_FILTER;
  case 0x06:
    return ACTION_TOGGLE_MODE;
  case 0x0C:
    return ACTION_REFRESH;
  case KEY_RESIZE:
    return ACTION_RESIZE;

  case 'k': /* Note: lowercase 'k' is KEY_UP when VI_KEYS profile is enabled */
  case 'K':
    return ACTION_VOL_MENU;
  case ',':
  case '<':
    return ACTION_VOL_PREV;
  case '.':
  case '>':
    return ACTION_VOL_NEXT;

  case 'a':
  case 'A':
    return ACTION_CMD_A;
  case 'b':
  case 'B':
    return ACTION_TOGGLE_COMPACT;
  case 'c':
    if (ctx && ctx->focused_window == FOCUS_TREE)
      return ACTION_COMPARE_DIR;
    return ACTION_CMD_C;
  case 'C':
    if (ctx && ctx->focused_window == FOCUS_TREE)
      return ACTION_COMPARE_DIR;
    return ACTION_CMD_C;
  case 'd':
    return ACTION_CMD_D;
  case 'D':
    /* In vi-key mode, Ctrl-D is reserved for page-down navigation.
     * Use uppercase D as the file-window "delete tagged" command key.
     */
    if (vi_keys_enabled && ctx && ctx->focused_window == FOCUS_FILE)
      return ACTION_CMD_TAGGED_D;
    return ACTION_CMD_D;
  case KEY_DC:
    return ACTION_CMD_D;
  case 'e':
  case 'E':
    return ACTION_CMD_E;
  case 'g':
  case 'G':
    return ACTION_CMD_G;
  case 'h':
  case 'H':
    return ACTION_CMD_H;
  case 'm':
  case 'M':
    return ACTION_CMD_M;
  case 'n':
  case 'N':
    return ACTION_CMD_MKFILE;
  case 'p':
  case 'P':
    return ACTION_CMD_P;
  case 'r':
  case 'R':
    return ACTION_CMD_R;
  case 's':
  case 'S':
    return ACTION_CMD_S;
  case 'v':
  case 'V':
    return ACTION_CMD_V;
  case 'x':
  case 'X':
    return ACTION_CMD_X;
  case 'y':
  case 'Y':
    return ACTION_CMD_Y;
  case '`':
    return ACTION_TOGGLE_HIDDEN;

  case 0x01:
    return ACTION_CMD_TAGGED_A;
  case 0x03:
    return ACTION_CMD_TAGGED_C;
  case 0x0B:
    return ACTION_CMD_TAGGED_C;
  case 0x04:
    return ACTION_CMD_TAGGED_D;
  case 0x07:
    return ACTION_NONE;
  case 0x0E:
    if (ctx && ctx->preview_mode)
      return ACTION_PREVIEW_SCROLL_DOWN;
    return ACTION_CMD_TAGGED_M;
  case 0x0F:
    return ACTION_NONE;
  case 0x10:
    if (ctx && ctx->preview_mode)
      return ACTION_PREVIEW_SCROLL_UP;
    return ACTION_CMD_TAGGED_P;
  case 0x12:
    return ACTION_CMD_TAGGED_R;
  case 0x13:
    return ACTION_CMD_TAGGED_S;
  case 0x16:
    return ACTION_CMD_TAGGED_V;
  case 0x18:
    return ACTION_CMD_TAGGED_X;
  case 0x19:
    return ACTION_CMD_TAGGED_Y;

#ifdef KEY_F
  case KEY_F(12):
    return ACTION_LIST_JUMP;
  case KEY_F(8):
    return ACTION_SPLIT_SCREEN;
  case KEY_F(7):
    return ACTION_VIEW_PREVIEW;
  case KEY_F(6):
    return ACTION_TOGGLE_STATS;
  case KEY_F(5):
    return ACTION_REFRESH;
  case '8':
  case KEY_F(28):
    return ACTION_TOGGLE_TAGGED_MODE;
  case KEY_F(16):
    return ACTION_TOGGLE_TAGGED_MODE;
#endif

  case 'j':
    if (!vi_keys_enabled && ctx && ctx->focused_window == FOCUS_FILE)
      return ACTION_COMPARE_FILE;
    return ACTION_NONE;
  case 'J':
    if (ctx && ctx->focused_window == FOCUS_FILE)
      return ACTION_COMPARE_FILE;
    return ACTION_NONE;

#ifdef KEY_SF
  case KEY_SF:
    return ACTION_PREVIEW_SCROLL_DOWN;
#endif
#ifdef KEY_SR
  case KEY_SR:
    return ACTION_PREVIEW_SCROLL_UP;
#endif
#ifdef KEY_SHOME
  case KEY_SHOME:
    return ACTION_PREVIEW_HOME;
#endif
#ifdef KEY_SEND
  case KEY_SEND:
    return ACTION_PREVIEW_END;
#endif
#ifdef KEY_SPREVIOUS
  case KEY_SPREVIOUS:
    return ACTION_PREVIEW_PAGE_UP;
#endif
#ifdef KEY_SNEXT
  case KEY_SNEXT:
    return ACTION_PREVIEW_PAGE_DOWN;
#endif

  default:
    return ACTION_NONE;
  }
}

int WGetch(ViewContext *ctx, WINDOW *win) {
  int c;

  c = wgetch(win);

#ifdef KEY_RESIZE
  if (c == KEY_RESIZE) {
    if (ctx)
      ctx->resize_request = TRUE;
    c = -1;
  }
#endif

  return (c);
}

int Getch(ViewContext *ctx) { return WGetch(ctx, stdscr); }

int GetEventOrKey(ViewContext *ctx) {
  int ch;
  int w_fd = Watcher_GetFD(ctx);
  fd_set fds;
  struct timeval tv;

  if (ctx && ctx->resize_request)
    return KEY_RESIZE;

  /* Before the select loop, check the shutdown flag */
  if (ytree_shutdown_flag)
    return 'q';

  /* Check if input is already available to avoid select delay */
  nodelay(stdscr, TRUE);
  ch = WGetch(ctx, stdscr);
  nodelay(stdscr, FALSE);

  if (ch != ERR) {
    return ch;
  }

  if (ctx && ctx->resize_request)
    return KEY_RESIZE;

  while (1) {
    int max_fd;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    max_fd = STDIN_FILENO;

    if (w_fd >= 0) {
      FD_SET(w_fd, &fds);
      if (w_fd > max_fd)
        max_fd = w_fd;
    }

    /* Setup timeout for 500ms for clock update */
    tv.tv_sec = 0;
    tv.tv_usec = 500000;

    int result = select(max_fd + 1, &fds, NULL, NULL, &tv);

    if (result == 0) {
      /* Timeout: Update Clock */
      if (ctx) {
        ClockHandler(ctx, 0);
        doupdate();
      }
      continue;
    }

    if (result == -1) {
      if (errno == EINTR) {
        if (ytree_shutdown_flag)
          return 'q';

        nodelay(stdscr, TRUE);
        ch = WGetch(ctx, stdscr);
        nodelay(stdscr, FALSE);
        if (ch != ERR)
          return ch;
        if (ctx && ctx->resize_request)
          return KEY_RESIZE;

        continue;
      }
      return -1;
    }

    if (ctx && (ctx->refresh_mode & REFRESH_WATCHER) && w_fd >= 0 &&
        FD_ISSET(w_fd, &fds)) {
      if (Watcher_ProcessEvents(ctx)) {
        return KEY_F(5);
      }
    }

    if (FD_ISSET(STDIN_FILENO, &fds)) {
      /* Input available, perform WGetch */
      return WGetch(ctx, stdscr);
    }
  }
}

int UI_AskConflict(ViewContext *ctx, const char *src_path, const char *dst_path,
                   int *mode_flags) {
  char msg[1024];
  int c;

  (void)src_path;

  snprintf(msg, sizeof(msg), "Overwrite %.300s? (Y)es/(N)o/(A)ll/(Q)uit",
           dst_path);

  /* Allow Y, N, A, Q, and ESC (27) */
  c = InputChoice(ctx, msg, "YNAQ\033");

  if (c == 'Y')
    return CONFLICT_OVERWRITE;
  if (c == 'N')
    return CONFLICT_SKIP;
  if (c == 'A') {
    if (mode_flags)
      *mode_flags = 2; /* Set to 2 for ALL, distinguishing from 1 (Yes/Force) */
    return CONFLICT_ALL;
  }
  if (c == 'Q' || c == 27)
    return CONFLICT_ABORT;

  return CONFLICT_ABORT;
}
