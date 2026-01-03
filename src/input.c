/***************************************************************************
 * input.c
 * InputStr                                                                *
 * Reads a string at position (y,x) with max length                        *
 * Default value for input is s itself                                     *
 * Returns the character that terminated the input                         *
 ***************************************************************************/


#include "ytree.h"
#include "watcher.h"
#include <sys/select.h>
#include <unistd.h>
#include <errno.h>

#ifdef READLINE_SUPPORT
#include <readline/tilde.h>
#endif


/* Wrapper function to satisfy tputs(..., int (*putc_func)(int)) signature */
/* It writes the character 'c' to standard output. */
static int term_putc(int c)
{
    return fputc(c, stdout);
}


char *StrLeft(const char *str, size_t visible_count)
{
  char *result;
  size_t len;
  int  left_bytes;

#ifdef WITH_UTF8
  mbstate_t state;
  const char *s, *s_start;;
  size_t pos = 0; /* Changed from int to size_t to match visible_count */
#endif

  if (visible_count == 0)
    return(strdup(""));

  len = StrVisualLength(str);
  if (visible_count >= len)
    return(strdup(str));

#ifdef WITH_UTF8

  s_start = s = str;

  while(*s) {

    wchar_t wc;
    size_t sz;
    int width;

    s_start = s;
    memset(&state, 0, sizeof(state));
    sz = mbrtowc(&wc, s, MB_CUR_MAX, &state);

    if(sz == (size_t)-1 || sz == (size_t)-2 || sz == 0) {
      /* Invalid or incomplete sequence. Treat as a single block for safety. */
      sz = 1;
      s++;
      width = 1;
    } else {
      s += sz;
      width = wcwidth(wc);
      if(width < 0)
        width = 1;
    }

    if(pos + (size_t)width > visible_count)
      break;  /* exceeds limit */

    pos += width;
  }

  left_bytes = s_start - str;

#else
  left_bytes = visible_count;
#endif

  result = malloc(left_bytes + 1);
  if (result == NULL) {
      ERROR_MSG("malloc failed*ABORT");
      exit(1);
  }
  memcpy(result, str, left_bytes);
  result[left_bytes] = '\0';
  return(result);
}


char *StrRight(const char *str, size_t visible_count)
{
  char *result;
  size_t visual_len;

#ifdef WITH_UTF8
  int left_bytes;
  mbstate_t state;
  const char *s, *s_start;
  int pos_start = 0;
#endif

  if (visible_count == 0)
    return(strdup(""));

  visual_len = StrVisualLength(str);
  if(visual_len <= visible_count)
    return(strdup(str));

#ifdef WITH_UTF8

  s_start = s = str;

  while(*s) {

    wchar_t wc;
    size_t sz;
    int width;

    s_start = s;
    memset(&state, 0, sizeof(state));
    sz = mbrtowc(&wc, s, MB_CUR_MAX, &state);

    if(sz == (size_t)-1 || sz == (size_t)-2 || sz == 0) {
      sz = 1;
      s++;
      width = 1;
    } else {
      s += sz;
      width = wcwidth(wc);
      if(width < 0)
        width = 1;
    }

    if((visual_len - (pos_start + width)) < visible_count)
      break;  /* The next character is the first to display */

    pos_start += width;
  }

  left_bytes = s_start - str;

  result = strdup(&str[left_bytes]);

#else
  result = strdup( &str[visual_len - visible_count] );
#endif

  if (result == NULL) {
      ERROR_MSG("strdup failed*ABORT");
      exit(1);
  }
  return(result);
}



int StrVisualLength(const char *str)
{
  int len;

#ifdef WITH_UTF8

  int pos = 0;
  size_t sz;
  mbstate_t state;
  const char *s = str;

  while(*s) {
    wchar_t wc;
    int width;

    memset(&state, 0, sizeof(state));
    sz = mbrtowc(&wc, s, MB_CUR_MAX, &state);

    if( sz == (size_t) -1 || sz == (size_t)-2 || sz == 0 ) {
      /* Invalid/incomplete sequence. Treat as 1 byte, 1 visual char, and skip 1 byte. */
      s++;
      width = 1;
    } else {
      s += sz;
      width = wcwidth(wc);
      if(width < 0)
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
int VisualPositionToBytePosition(const char *str, int visual_pos)
{

#ifdef WITH_UTF8

  mbstate_t state;
  const char *s, *s_start;
  int pos = 0;

  s_start = s = str;

  while(*s) {

    wchar_t wc;
    size_t sz;
    int width;

    s_start = s;
    memset(&state, 0, sizeof(state));
    sz = mbrtowc(&wc, s, MB_CUR_MAX, &state);

    if(sz == (size_t)-1 || sz == (size_t)-2 || sz == 0) {
      sz = 1;
      s++;
      width = 1;
    } else {
      s += sz;
      width = wcwidth(wc);
      if(width < 0)
        width = 1;
    }

    if(pos + width > visual_pos)
      return( s_start - str );

    pos += width;
  }

  return( s - str );

#else
  return visual_pos;
#endif

}


int InputStringEx(char *s, int y, int x, int cursor_pos, int display_width, int max_len, char *term, int history_type)
{
  int p;                       /* Current Cursor-Position (visual) */
  int c1;                      /* Read Char */
  int i, n;                    /* Loop vars */
  char *pp;
  BOOL len_flag = FALSE;
  char path[PATH_LENGTH + 1];
  char buf[MB_CUR_MAX + 1];
  char *ls;
  static BOOL insert_flag = TRUE;
  int scroll_offset = 0;       /* Visual offset */

  if(max_len < 1) return -1;

  /* Setup for input */
  print_time = FALSE;
  curs_set(1);
  leaveok(stdscr, FALSE);

  p = cursor_pos;

  nodelay( stdscr, FALSE );

  do {
    /* Handle Scrolling logic */
    if (p < scroll_offset) {
        scroll_offset = p;
    } else if (p >= scroll_offset + display_width) {
        scroll_offset = p - display_width + 1;
    }

    /* Redraw string window */
    move(y, x);

    char *to_free = NULL;
    int start_byte = VisualPositionToBytePosition(s, scroll_offset);
    char *temp_alloc = strdup(&s[start_byte]);

    if (temp_alloc) {
        /* Truncate to display width */
        to_free = StrLeft(temp_alloc, display_width);
        free(temp_alloc);
    }

    int drawn_len = 0;
    if (to_free) {
        addstr(to_free);
        drawn_len = StrVisualLength(to_free);
    }

    /* Fill remaining with spaces (or underscores as per original logic) */
    for(i = drawn_len; i < display_width; i++)
      addch( '_' );

    if (to_free) free(to_free); /* Always free allocated display string */

    move( y, x + (p - scroll_offset));
    RefreshWindow( stdscr );
    doupdate();

    c1 = Getch();

#ifdef VI_KEYS
    c1 = ViKey(c1); /* ViKey processing is still needed here for input string editing */
#endif

    /* Check for termination characters */
    if(c1 == ESC || c1 == CR || c1 == LF || strchr(term, c1) != NULL || c1 == -1) {
        if (c1 == LF) c1 = CR; /* Normalize line feed to carriage return */
        break;
    }

    /* Check visual length constraint for UI feedback */
    if (StrVisualLength(s) >= max_len) len_flag = TRUE;
    else len_flag = FALSE;

    /* Handle control/function keys */
    switch( c1 )
    {
    case KEY_LEFT:
    case KEY_BTAB:  /* Back-tab (often Shift-Tab), treat as left */
        if( p > 0 ) p--;
        break;

    case KEY_RIGHT:
        if( p < StrVisualLength(s) ) p++;
        break;

    case KEY_UP:
        pp = GetHistory(history_type);
        if (pp == NULL) break;
        if(*pp) {
            ls = StrLeft(pp, max_len);
            /* Safety check before copy */
            if (strlen(ls) < (size_t)max_len) {
                strcpy(s, ls);
                p = StrVisualLength(s);
            }
            free(ls);
        }
        break;

    case KEY_DOWN:
    case KEY_PPAGE:
    case KEY_NPAGE:
        break;

    case 'A' & 0x1F: /* Ctrl-A, Beginning of Line */
    case KEY_HOME:
        p = 0; break;

    case 'E' & 0x1F: /* Ctrl-E, End of Line */
    case KEY_END:
        p = StrVisualLength( s ); break;

    case 'K' & 0x1F: /* Ctrl-K, Kill to End of Line */
    case KEY_DL:     /* Delete to end of line */
        {
            int byte_pos = VisualPositionToBytePosition(s, p);
            s[byte_pos] = '\0';
        }
        break;

    case 'U' & 0x1F: /* Ctrl-U, Kill to Beginning of Line */
        if (p > 0) {
            int byte_pos = VisualPositionToBytePosition(s, p);
            memmove(s, s + byte_pos, strlen(s) - byte_pos + 1);
            p = 0;
        }
        break;

    case 'W' & 0x1F: /* Ctrl-W, Kill Word */
        if (p > 0) {
            int byte_p = VisualPositionToBytePosition(s, p);
            int end_of_word = byte_p;
            int start_of_word;

            /* Skip backwards over any non-alphanumeric characters from cursor */
            while (end_of_word > 0 && !isalnum((unsigned char)s[end_of_word - 1])) {
                end_of_word--;
            }

            /* Find the start of the word (beginning of alphanumeric sequence) */
            start_of_word = end_of_word;
            while (start_of_word > 0 && isalnum((unsigned char)s[start_of_word - 1])) {
                start_of_word--;
            }

            if (start_of_word < byte_p) {
                memmove(&s[start_of_word], &s[byte_p], strlen(&s[byte_p]) + 1);

                /* Recalculate p based on new string content up to start_of_word */
                char c = s[start_of_word];
                s[start_of_word] = '\0';
                p = StrVisualLength(s);
                s[start_of_word] = c;
            }
        }
        break;

    case 'D' & 0x1F: /* Ctrl-D, Delete character */
    case KEY_DC:     /* Delete key, defined in curses.h */
    case 0x7F:       /* ASCII DEL character */
        n = StrVisualLength(s);
        if( p < n ) {
            int curr_byte = VisualPositionToBytePosition(s, p);
            int next_byte = VisualPositionToBytePosition(s, p + 1);
            memmove(s + curr_byte, s + next_byte, strlen(s) - next_byte + 1);
        }
        break;

    /* Consolidate all backspace/Ctrl-H aliases */
    case KEY_BACKSPACE:
    case 0x08:       /* ASCII BS, often sent for Ctrl-H */
        if( p > 0 ) {
            int curr_byte = VisualPositionToBytePosition(s, p);
            int prev_byte = VisualPositionToBytePosition(s, p - 1);
            memmove(s + prev_byte, s + curr_byte, strlen(s) - curr_byte + 1);
            p--;
        }
        break;

    case KEY_EIC:
    case KEY_IC: insert_flag ^= TRUE; break;

    case '\t':
        pp = GetMatches(s);
        if (pp == NULL) break;
        if(*pp) {
            ls = StrLeft(pp, max_len);
            if (strlen(ls) < (size_t)max_len) {
                strcpy(s, ls);
                p = StrVisualLength(s);
            }
            free(ls);
            free(pp);
        }
        break;

    case KEY_RESIZE: resize_request = TRUE; break;

#ifdef KEY_F
    case KEY_F(2):
#endif
    case 'F' & 0x1f:
        if (history_type != HST_LOGIN && history_type != HST_PATH) {
            break;
        }

        /* Updated with global CurrentVolume usage */
        if(KeyF2Get( CurrentVolume->vol_stats.tree, CurrentVolume->vol_stats.disp_begin_pos, CurrentVolume->vol_stats.cursor_pos, path) == 0) {
            if(*path) {
                ls = StrLeft(path, max_len);
                if (strlen(ls) < (size_t)max_len) {
                    strcpy(s, ls);
                    p = StrVisualLength(s);
                }
                free(ls);
            }
        }
        break;

    case 'C' & 0x1f: c1 = ESC; break;

    default:
        /* Handle printable/multibyte input */
        if (c1 >= ' ' || c1 > 0xFF) {
            int ins_len = 0;

            if (c1 < 0x100) {
                buf[0] = (char)c1;
                buf[1] = '\0';
                ins_len = 1;
            } else {
                /* Ignore unknown codes */
                break;
            }

            /* Calculate Buffer Constraints - max_len is treated as byte buffer limit */
            if (strlen(s) + ins_len >= (size_t)max_len) {
                beep();
                break;
            }

            int byte_pos = VisualPositionToBytePosition(s, p);

            if ( insert_flag ) {
                memmove(s + byte_pos + ins_len, s + byte_pos, strlen(s) - byte_pos + 1);
                memcpy(s + byte_pos, buf, ins_len);
            } else {
                /* Overwrite logic */
                int next_byte_pos = VisualPositionToBytePosition(s, p + 1);
                int char_len = next_byte_pos - byte_pos;

                if (char_len != ins_len) {
                     if (ins_len < char_len) {
                         /* New char is shorter than old one - shift left */
                         memcpy(s + byte_pos, buf, ins_len);
                         memmove(s + byte_pos + ins_len, s + next_byte_pos, strlen(s) - next_byte_pos + 1);
                     } else {
                         /* New char is longer than old one - shift right */
                         memmove(s + byte_pos + ins_len, s + next_byte_pos, strlen(s) - next_byte_pos + 1);
                         memcpy(s + byte_pos, buf, ins_len);
                     }
                } else {
                    /* Same length - direct overwrite */
                    memcpy(s + byte_pos, buf, ins_len);
                }
            }
            p += StrVisualLength(buf);
        }
        break;
    } /* switch */

  } while( 1 );

  /* Final cleanup and history update */
  nodelay( stdscr, FALSE ); /* Ensure curses state is reset */

  /* Clear any remaining visual artifacts from scrolling on exit */
  move( y, x );
  for(i=0; i < display_width; i++ ) addch( ' ' );

  /* Redraw final string trimmed to display_width */
  char *final_show = StrLeft(s, display_width);
  MvAddStr( y, x, final_show);
  free(final_show);

  leaveok( stdscr, TRUE);
  curs_set(0);
  print_time = TRUE;
  InsHistory( s, history_type );
#ifdef READLINE_SUPPORT
  pp = tilde_expand(s);
#else
  pp = strdup(s);
  if (pp == NULL) {
      ERROR_MSG("strdup failed*ABORT");
      exit(1);
  }
#endif

  strncpy( s, pp, max_len - 1);
  s[max_len-1] = '\0'; /* Ensure null termination within bounds */

  free(pp);
  return( c1 );
}


int InputString(char *s, int y, int x, int cursor_pos, int length, char *term, int history_type)
{
    return InputStringEx(s, y, x, cursor_pos, length, length, term, history_type);
}


int InputChoice(char *msg, char *term)
{
  int  c;

  ClearHelp();

  curs_set(1);
  leaveok(stdscr, FALSE);
  mvprintw( LINES - 2, 1, "%s", msg );
  RefreshWindow( stdscr );
  doupdate();
  do
  {
    c = Getch();
    if(c >= 0)
      if( islower( c ) ) c = toupper( c );
  } while( c != -1 && !strchr( term, c ) );

  if(c >= 0)
    echochar( c );

  move( LINES - 2, 1 ); clrtoeol();
  leaveok(stdscr, TRUE);
  curs_set(0);
  refresh();

  return( c );
}


void HitReturnToContinue(void)
{
  /* Fix: Use term_putc callback to satisfy tputs signature and reset terminal attributes after endwin() */
  char *te; /* termcap variable for exit_attribute_mode ("me") */

#if !defined(XCURSES) /* XCURSES handles this differently or it's not needed */

  /* tgetstr() is typically available after including <term.h> in ytree.h */
  char *tgetstr(const char *, char **);
  int tputs(const char *, int, int (*)(int));

  curs_set(1);

  /* Use putp to set reverse video (for the prompt text) */
  vidattr( A_REVERSE );

  putp( "[Hit return to continue]" );
  (void) fflush( stdout );

  /* Wait for key press */
  (void) getchar();

  /* Reset all attributes in the terminal using tgetstr/tputs */
  /* This is necessary because the prompt was printed outside of curses mode */
  te = tgetstr("me", NULL);
  if (te != NULL) {
      tputs(te, 1, term_putc); /* Use new wrapper function */
  } else {
      /* Fallback: use putp which may not be a perfect fix on all terminals */
      putp("\033[0m");
  }
  (void) fflush(stdout);

#endif /* XCURSES */

  curs_set(0);
  doupdate();
}


BOOL KeyPressed()
{
  BOOL pressed = FALSE;

  nodelay( stdscr, TRUE );
  if( wgetch( stdscr ) != ERR ) pressed = TRUE;
  nodelay( stdscr, FALSE );

  return( pressed );
}


BOOL EscapeKeyPressed()
{
  BOOL pressed = FALSE;
  int  c;

  nodelay( stdscr, TRUE );
  if( ( c = wgetch( stdscr ) ) != ERR ) pressed = TRUE;
  nodelay( stdscr, FALSE );

  return( ( pressed && c == ESC ) ? TRUE : FALSE );
}


#ifdef VI_KEYS

int ViKey( int ch )
{
  switch( ch )
  {
    case VI_KEY_UP:    ch = KEY_UP;    break;
    case VI_KEY_DOWN:  ch = KEY_DOWN;  break;
    case VI_KEY_RIGHT: ch = KEY_RIGHT; break;
    case VI_KEY_LEFT:  ch = KEY_LEFT;  break;
    case VI_KEY_PPAGE: ch = KEY_PPAGE; break;
    case VI_KEY_NPAGE: ch = KEY_NPAGE; break;
  }
  return(ch);
}

#endif /* VI_KEYS */


/*
 * GetKeyAction
 * Translates a raw input character (potentially pre-processed by ViKey)
 * into a logical YtreeAction.
 */
YtreeAction GetKeyAction(int ch)
{
#ifdef VI_KEYS
    ch = ViKey(ch);
#endif

    switch (ch) {
        /* Navigation */
        case KEY_UP:    return ACTION_MOVE_UP;
        case KEY_DOWN:  return ACTION_MOVE_DOWN;
        case KEY_LEFT:  return ACTION_MOVE_LEFT;
        case KEY_RIGHT: return ACTION_MOVE_RIGHT;
        case KEY_PPAGE: return ACTION_PAGE_UP;
        case KEY_NPAGE: return ACTION_PAGE_DOWN;
        case KEY_HOME:  return ACTION_HOME;
        case KEY_END:   return ACTION_END;

        /* Tree Ops */
        case '\t':      return ACTION_MOVE_SIBLING_NEXT; /* TAB: Jump to next sibling */
        case '*':       return ACTION_ASTERISK;          /* Asterisk: Expand All / Invert Tags */
        case KEY_BTAB:  return ACTION_MOVE_SIBLING_PREV; /* Back-tab: Jump to prev sibling */
        case '-':       return ACTION_TREE_COLLAPSE;
        case '+':       return ACTION_TREE_EXPAND_ALL;

        /* Standard Commands */
        case CR:
        case LF:        return ACTION_ENTER;
        case ESC:       return ACTION_ESCAPE; /* Explicitly map ESC to ACTION_ESCAPE */
        case 'l':
        case 'L':       return ACTION_LOGIN;
        case 'q':
        case 'Q':       return ACTION_QUIT;
        case 0x11:      return ACTION_QUIT_DIR; /* Ctrl-Q */
        case 't':       return ACTION_TAG;
        case 'u':       return ACTION_UNTAG;
        case 'T':       return ACTION_TAG_ALL;
        case 0x14:      return ACTION_TAG_ALL; /* Ctrl-T */
        case 'U':       return ACTION_UNTAG_ALL;
        case 0x15:      return ACTION_UNTAG_ALL; /* Ctrl-U */
        case ';':       return ACTION_TAG_REST;
        case ':':       return ACTION_UNTAG_REST;
        case 'f':
        case 'F':       return ACTION_FILTER;
        case 0x06:      return ACTION_TOGGLE_MODE; /* Ctrl-F */
        case 0x0C:      return ACTION_REFRESH; /* Ctrl-L */
        case KEY_RESIZE: return ACTION_RESIZE;

        /* Volume Ops */
        case 'K':       return ACTION_VOL_MENU;
        case ',':
        case '<':       return ACTION_VOL_PREV;
        case '.':
        case '>':       return ACTION_VOL_NEXT;

        /* Context Commands (Letters) */
        case 'a':
        case 'A':       return ACTION_CMD_A;
        case 'b':
        case 'B':       return ACTION_TOGGLE_COMPACT; /* Brief Mode */
        case 'c':
        case 'C':       return ACTION_CMD_C;
        case 'd':
        case 'D':
        case KEY_DC:    return ACTION_CMD_D;
        case 'e':
        case 'E':       return ACTION_CMD_E;
        case 'g':
        case 'G':       return ACTION_CMD_G;
        case 'h':
        case 'H':       return ACTION_CMD_H;
        case 'm':
        case 'M':       return ACTION_CMD_M;
        case 'o':
        case 'O':       return ACTION_CMD_O;
        case 'p':
        case 'P':       return ACTION_CMD_P;
        case 'r':
        case 'R':       return ACTION_CMD_R;
        case 's':
        case 'S':       return ACTION_CMD_S;
        case 'v':
        case 'V':       return ACTION_CMD_V;
        case 'x':
        case 'X':       return ACTION_CMD_X;
        case 'y':
        case 'Y':       return ACTION_CMD_Y;
        case '`':       return ACTION_TOGGLE_HIDDEN;

        /* Tagged/Ctrl Variants */
        case 0x01:      return ACTION_CMD_TAGGED_A; /* Ctrl-A */
        case 0x03:      return ACTION_CMD_TAGGED_C; /* Ctrl-C */
        case 0x0B:      return ACTION_CMD_TAGGED_C; /* Ctrl-K (legacy copy) */
        case 0x04:      return ACTION_CMD_TAGGED_D; /* Ctrl-D */
        case 0x07:      return ACTION_CMD_TAGGED_G; /* Ctrl-G */
        case 0x0E:      return ACTION_CMD_TAGGED_M; /* Ctrl-N (Move Tagged legacy) */
        case 0x0F:      return ACTION_CMD_TAGGED_O; /* Ctrl-O */
        case 0x10:      return ACTION_CMD_TAGGED_P; /* Ctrl-P */
        case 0x12:      return ACTION_CMD_TAGGED_R; /* Ctrl-R */
        case 0x13:      return ACTION_CMD_TAGGED_S; /* Ctrl-S */
        case 0x18:      return ACTION_CMD_TAGGED_X; /* Ctrl-X */
        case 0x19:      return ACTION_CMD_TAGGED_Y; /* Ctrl-Y */

        /* Function Keys */
#ifdef KEY_F
        case KEY_F(12): return ACTION_LIST_JUMP;
        case KEY_F(6):  return ACTION_TOGGLE_STATS;
        case KEY_F(5):  return ACTION_REFRESH; /* ADDED */
        case '8':
        case KEY_F(28): return ACTION_TOGGLE_TAGGED_MODE; /* Shift-F4 */
        case KEY_F(16): return ACTION_TOGGLE_TAGGED_MODE; /* F4 (often Shift-F4 on some terminals) */
#endif

        default:        return ACTION_NONE;
    }
}


/* Removed AixWgetch/Aix specific logic */


int WGetch(WINDOW *win)
{
  int c;

  c = wgetch(win);

#ifdef KEY_RESIZE
  if(c == KEY_RESIZE) {
    resize_request = TRUE;
    c = -1;
  }
#endif

  return(c);
}


int Getch()
{
  return(WGetch(stdscr));
}

int GetEventOrKey(void)
{
    int ch;
    int w_fd = Watcher_GetFD();
    fd_set fds;
    int max_fd;
    int result;

    /* 1. Check for pending resize or buffered input first */
    if (resize_request) return KEY_RESIZE;

    /* 2. Check ncurses internal buffer (non-blocking) */
    nodelay(stdscr, TRUE);
    ch = WGetch(stdscr);
    nodelay(stdscr, FALSE);
    if (ch != ERR) return ch;
    /* Check again in case WGetch set the flag but returned ERR */
    if (resize_request) return KEY_RESIZE;

    while (1) {
        FD_ZERO(&fds);
        FD_SET(STDIN_FILENO, &fds);
        max_fd = STDIN_FILENO;

        if (w_fd >= 0) {
            FD_SET(w_fd, &fds);
            if (w_fd > max_fd) max_fd = w_fd;
        }

        /* 3. Wait for input or event */
        result = select(max_fd + 1, &fds, NULL, NULL, NULL);

        if (result == -1) {
            if (errno == EINTR) {
                /* Signal received (likely SIGWINCH).
                   Check for KEY_RESIZE in ncurses buffer. */
                nodelay(stdscr, TRUE);
                ch = WGetch(stdscr);
                nodelay(stdscr, FALSE);
                if (ch != ERR) return ch;
                if (resize_request) return KEY_RESIZE; /* FIX: Check flag */

                /* If just a signal and no key, retry select */
                continue;
            }
            return -1; /* Error */
        }

        /* 4. Handle File System Events */
        if (w_fd >= 0 && FD_ISSET(w_fd, &fds)) {
            if (Watcher_ProcessEvents()) {
                return KEY_F(5); /* Trigger Refresh */
            }
            /* If event was filtered (e.g. ignored), continue waiting */
        }

        /* 5. Handle Keyboard Input */
        if (FD_ISSET(STDIN_FILENO, &fds)) {
            /* STDIN ready, perform blocking read */
            return WGetch(stdscr);
        }
    }
}