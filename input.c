/***************************************************************************
 * InputStr                                                                *
 * Liest eine Zeichenkette an Position (y,x) mit der max. Laenge length    *
 * Vorschlagswert fuer die Eingabe ist s selbst                            *
 * Zurueckgegeben wird das Zeichen, mit dem die Eingabe beendet wurde      *
 ***************************************************************************/
 
 
#include "ytree.h"
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
  int pos = 0;
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
    
    if(pos + width > visible_count)
      break;  /* exceeds limit */
      
    pos += width;
  }

  left_bytes = s_start - str; 

#else
  left_bytes = visible_count; 
#endif

  result = strndup(str, left_bytes);
  if (result == NULL) {
      ERROR_MSG("strndup failed*ABORT");
      exit(1);
  }
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


int InputString(char *s, int y, int x, int cursor_pos, int length, char *term)
                               /* Ein- und Ausgabestring              */
                               /* Position auf Bildschirm             */
                               /* max. Laenge                         */
                               /* Menge von Terminierungszeichen      */
{
  int p;                       /* Aktuelle Cursor-Position (visual length) */
  int c1;                      /* Gelesenes Zeichen */
  int i, n;                    /* Laufvariable */
  char *pp;
  BOOL len_flag = FALSE;
  char path[PATH_LENGTH + 1];
  char buf[MB_CUR_MAX + 1]; 
  char *ls, *rs;
  static BOOL insert_flag = TRUE;

  if(length < 1)
    return -1;

  /* Setup for input */
  print_time = FALSE;
  curs_set(1);
  leaveok(stdscr, FALSE);
  
  p = cursor_pos;
  
  /* Draw string and fill to max length with '_' */
  MvAddStr( y, x, s );
  
  /* Corrected placeholder loop to ensure no wrap */
  for(i=StrVisualLength(s); i < length; i++)
    addch( '_' );

  MvAddStr( y, x, s );

  /* Disable nodelay mode as it causes issues with multi-key sequences */
  nodelay( stdscr, FALSE );

  do {
    /* Redraw string, fill to max length with '_', and position cursor */
    MvAddStr( y, x, s );
    
    /* Redraw placeholders */
    for(i=StrVisualLength(s); i < length; i++)
      addch( '_' );
    
    move( y, x + p);
    RefreshWindow( stdscr );
    doupdate();

    c1 = Getch();
    
#ifdef VI_KEYS
    c1 = ViKey(c1);
#endif

    /* Check for termination characters */
    if(c1 == ESC || c1 == CR || c1 == LF || strchr(term, c1) != NULL || c1 == -1) {
        if (c1 == LF) c1 = CR; /* Normalize line feed to carriage return */
        break;
    }

    /* Check length constraint */
    if (StrVisualLength(s) >= length) len_flag = TRUE;
    else len_flag = FALSE;

    /* Handle control/function keys */
    switch( c1 )
    {
    case KEY_LEFT:
    case KEY_BTAB:  /* Back-tab (often Shift-Tab), treat as left */
        if( p > 0 ) p--; else beep(); break;
        
    case KEY_RIGHT:
        if( p < StrVisualLength(s) ) p++; else beep(); break;
        
    case KEY_UP:
        pp = GetHistory();
        if (pp == NULL) break;
        if(*pp) {
            ls = StrLeft(pp, length);
            strcpy(s, ls);
            free(ls);
            p = StrVisualLength(s);
        }
        break;

    case KEY_DOWN:
    case KEY_PPAGE:
    case KEY_NPAGE:
        /* These keys mis-map to '#'/'S'/'R' in non-command modes. 
           In the input bar, they should simply be ignored (beep) or mapped 
           to a sensible, simple action like history. For now, treat as unhandled. */
        beep(); break;

    case KEY_HOME: 
        p = 0; break; 
        
    case KEY_END: 
        p = StrVisualLength( s ); break;
        
    case KEY_DC:    /* Delete key, defined in curses.h */
    case 0x7F:      /* ASCII DEL character */
        n = StrVisualLength(s);
        if( p < n ) {
            ls = StrLeft(s, p);
            rs = StrRight(s, n - p - 1);
            strcpy(s, ls);
            strcat(s, rs);
            free(ls);
            free(rs);
        } else {
            beep();
        }
        break;
        
    /* Consolidate all backspace/Ctrl-H aliases */
    case KEY_BACKSPACE: 
    case 0x08: 
        n = StrVisualLength(s);
        if( p > 0 ) {
            ls = StrLeft(s, p - 1);
            rs = StrRight(s, n - p);
            strcpy(s, ls);
            strcat(s, rs);
            free(ls);
            free(rs);
            p--;
        } else {
            beep();
        }
        break;
        
    case KEY_DL: /* Delete to end of line */
        ls = StrLeft(s, p);
        strcpy(s, ls);
        free(ls);
        break;
        
    case KEY_EIC:
    case KEY_IC: insert_flag ^= TRUE; break;
        
    case '\t': 
        pp = GetMatches(s);
        if (pp == NULL) break;
        if(*pp) {
            ls = StrLeft(pp, length);
            strcpy(s, ls);
            free(ls);
            p = StrVisualLength(s);
            free(pp);
        }
        break;
        
    case KEY_RESIZE: resize_request = TRUE; break;
        
#ifdef KEY_F
    case KEY_F(2):
#endif
    case 'F' & 0x1f: 
        if(KeyF2Get( statistic.tree, statistic.disp_begin_pos, statistic.cursor_pos, path)) {
            break;
        }
        if(*path) {
            ls = StrLeft(path, length);
            strcpy(s, ls);
            free(ls);
            p = StrVisualLength(s);
        }
        break;

    case 'C' & 0x1f: c1 = ESC; break; 
    
    default:
        /* Handle printable/multibyte input */
        if (c1 >= ' ' || c1 > 0xFF) {
            /* For multibyte characters read the whole sequence */
#ifdef WITH_UTF8
            if (c1 > 0xFF) {
              /* Attempt to read remaining bytes for the multibyte sequence. */
              /* NOTE: This is an imperfect guess; curses often delivers MB as one keycode */
              /* The logic relies on c1 being the first byte or a multi-byte keycode. */
              /* Since curses typically handles key decoding, we assume c1 is a full sequence. */
              /* If this fails, the original logic is a basic guess for single-byte terminal. */
            }
#endif
            
            /* Copy input character to temporary buffer (max MB_CUR_MAX bytes) */
            buf[0] = (char)c1;
            buf[1] = '\0';
            
            if (len_flag == TRUE) {
                beep();
            } else {
                int visual_insert_len = StrVisualLength(buf);
                if (p + visual_insert_len > length) {
                    beep();
                } else {
                    if ( insert_flag ) {
                        /* Insert logic */
                        n = StrVisualLength(s);
                        if ( p >= n) {
                            strcat(s, buf);
                        } else {
                            if ( p > 0 ) ls = StrLeft(s, p);
                            else ls = strdup("");
                            rs = StrRight(s, n - p);
                            strcpy(s, ls);
                            strcat(s, buf);
                            strcat(s, rs);
                            free(ls);
                            free(rs);
                        }
                    } else {
                        /* Overwrite logic */
                        int byte_pos_to_overwrite = VisualPositionToBytePosition(s, p);
                        int bytes_to_delete = VisualPositionToBytePosition(&s[byte_pos_to_overwrite], visual_insert_len);
                        
                        if ( p > 0 ) ls = StrLeft(s, p);
                        else ls = strdup("");
                        
                        /* Extract remaining part: from after the overwritten content to the end */
                        if (strlen(s) - (byte_pos_to_overwrite + bytes_to_delete) > 0)
                           rs = strdup(&s[byte_pos_to_overwrite + bytes_to_delete]);
                        else 
                           rs = strdup("");

                        strcpy(s, ls);
                        strcat(s, buf);
                        strcat(s, rs);
                        
                        free(ls);
                        free(rs);
                    }
                    p += visual_insert_len;
                }
            }
        } else {
            /* Unhandled non-terminating control sequence. */
            beep();
        }
        break;
    } /* switch */

  } while( 1 );

  /* Final cleanup and history update */
  nodelay( stdscr, FALSE ); /* Ensure curses state is reset */

  p = strlen( s );
  move( y, x + p );

  for(i=0; i < length - p; i++ )
   addch( ' ' );

  move( y, x );
  leaveok( stdscr, TRUE);
  curs_set(0);
  print_time = TRUE;
  InsHistory( s );
#ifdef READLINE_SUPPORT
  pp = tilde_expand(s);
#else
  pp = strdup(s);
  if (pp == NULL) {
      ERROR_MSG("strdup failed*ABORT");
      exit(1);
  }
#endif

  strncpy( s, pp, length - 1);
  s[length]='\0';
  free(pp);
  return( c1 );
}


/* ... rest of input.c ... */

int InputChoise(char *msg, char *term)
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