#include "ytree.h"
#include "tilde.h"
/* #include "xmalloc.h" <-- REMOVED in Step 1 */


/***************************************************************************
 * InputStr                                                                *
 * Liest eine Zeichenkette an Position (y,x) mit der max. Laenge length    *
 * Vorschlagswert fuer die Eingabe ist s selbst                            *
 * Zurueckgegeben wird das Zeichen, mit dem die Eingabe beendet wurde      *
 ***************************************************************************/


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
    return(Strdup(""));

  len = StrVisualLength(str);
  if (visible_count >= len) 
    return(Strdup(str));
  
#ifdef WITH_UTF8

  s_start = s = str;

  while(*s) {

    wchar_t wc;
    size_t sz;
    int width;

    s_start = s;
    memset(&state, 0, sizeof(state));
    sz = mbrtowc(&wc, s, 4, &state);
    if(sz == (size_t)-1 || sz == (size_t)-2) {
      if( (*s++ & 0xc0) == 0xc0) {  /* skip to next char */
        while( (*s & 0xc0) == 0x80) 
          s++;
      }
      pos += 4; /* assume worst case */
    } else {
      s += sz;
      width = wcwidth(wc);
      if(width >= 0)
        pos += width;
      else
        pos++;
    }

    if(pos > visible_count)
      break;  /* exceeds limit */
  }

  if(*s)
    left_bytes = s_start - str; 
  else
    left_bytes = s - str; 

#else
  left_bytes = visible_count; 
#endif

  result = Strndup(str, left_bytes);
  return(result);
}


char *StrRight(const char *str, size_t visible_count)
{
  char *result;
  size_t visual_len;

#ifdef WITH_UTF8
  int left_bytes, pos = 0;
  mbstate_t state;
  const char *s, *s_start;
#endif

  if (visible_count == 0) 
    return(Strdup(""));

  visual_len = StrVisualLength(str);
  if(visual_len <= visible_count)
    return(Strdup(str));
  
#ifdef WITH_UTF8

  s_start = s = str;

  while(*s) {

    wchar_t wc;
    size_t sz;
    int width;

    s_start = s;
    memset(&state, 0, sizeof(state));
    sz = mbrtowc(&wc, s, 4, &state);
    if(sz == (size_t)-1 || sz == (size_t)-2) {
      if( (*s++ & 0xc0) == 0xc0) {  /* skip to next char */
        while( (*s & 0xc0) == 0x80) 
          s++;
      }
      pos += 4; /* assume worst case */
    } else {
      s += sz;
      width = wcwidth(wc);
      if(width >= 0)
        pos += width;
      else
        pos++;
    }

    if((visual_len - pos) < visible_count)
      break;  /* fits */
  }
  
  if(*s)
    left_bytes = s_start - str; 
  else
    left_bytes = s - str; 

  result = Strdup(&str[left_bytes]);

#else
  result = Strdup( &str[visual_len - visible_count] );
#endif

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
  wchar_t buffer[PATH_LENGTH+1];

  do {
    memset(&state, 0, sizeof(state));
    sz = mbrtowc(&buffer[pos], s, 4, &state);
    if( sz == (size_t) -1 || sz == (size_t)-2 ) {
      if( (*s++ & 0xc0) == 0xc0) {  /* skip to next char */
        while( (*s & 0xc0) == 0x80) 
          s++;
      }
      buffer[pos++]   = L'#';  /* Assume worst case: */
      buffer[pos++]   = L'#';  /* 1 wide char produces 4 visible chars */
      buffer[pos++]   = L'#';
      buffer[pos]     = L'#';
    } else {
      s += sz;
    }
    pos++;
  } while(sz != 0);

  len = wcswidth(buffer, PATH_LENGTH);

  if(len < 0)
    len = pos; /* should not happen */

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
    sz = mbrtowc(&wc, s, 4, &state);
    if(sz == (size_t)-1 || sz == (size_t)-2) {
      if( (*s++ & 0xc0) == 0xc0) {  /* skip to next char */
        while( (*s & 0xc0) == 0x80) 
          s++;
      }
      pos += 4; /* assume worst case */
    } else {
      s += sz;
      width = wcwidth(wc);
      if(width > 0)
	pos += width;
      else
	pos++;
    }

    if(pos > visual_pos)
      return( s_start - str );
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
  int p;                       /* Aktuelle Position                   */
  int c1;                      /* Gelesenes Zeichen                   */
  int i, n;                    /* Laufvariable                        */
  char *pp;
  BOOL len_flag = FALSE;
  char path[PATH_LENGTH + 1];
  char buf[2], sbuf[20], *ls, *rs;
  static BOOL insert_flag = TRUE;

  if(length < 1)
    return -1;

  buf[1] = '\0';
  strcpy(sbuf, "");

  /* Feld gefuellt ausgeben */
  /*------------------------*/
  print_time = FALSE;
  curs_set(1);
  MvAddStr( y, x, s );
  leaveok(stdscr, FALSE);
  
  for(i=strlen(s); i < length; i++)
    addch( '_' );

  p = cursor_pos;
  
  MvAddStr( y, x, s );

  nodelay( stdscr, TRUE );
  do {
    c1 = wgetch(stdscr);
    
    if ( c1 != ERR ) {
    
      if ( (c1 >= ' ' && c1 <= '~') || (c1 > 0xff) ) {
        if ( len_flag == TRUE) beep();
        else {
          buf[0] = (char)c1;
	  strcat(sbuf, buf);
	}
      } else {
        /* Control symbols */
        switch( c1 )
        {
        case 'C' & 0x1f     : c1 = 27; 
			      break;

        case KEY_LEFT       : if( p > 0 )
                                p--;
                              else
                                beep();
                              break;
        case KEY_RIGHT      : if( p < StrVisualLength(s) )
                                p++;
                              else
                                beep();
                              break;
	case KEY_UP         : 
	                      nodelay(stdscr, FALSE);
			      pp = GetHistory();
			      nodelay(stdscr, TRUE);
                             if (pp == NULL) break;
                             if(*pp)
                             {
			       ls = StrLeft(pp, length);
			       strcpy(s, ls);
			       free(ls);
			       p = StrVisualLength(s);
                               MvAddStr( y, x, s );
                               for(i=p; i < length; i++)
                                 addch( '_' );
                               RefreshWindow( stdscr );
                               doupdate();
                             }
                             break;

        case KEY_HOME       : p = 0;
                              break;
        case KEY_END        : p = StrVisualLength( s );
                              break;
        case KEY_DC         : n = StrVisualLength(s);
                              if( p < n )
                              {
			        ls = StrLeft(s, p);
				rs = StrRight(s, n - p - 1);
				strcpy(s, ls);
				strcat(s, rs);
				free(ls);
				free(rs);
				MvAddStr( y, x, s );
                                for(i=StrVisualLength(s); i < n; i++)
                                  addch( '_' );
                              }
                              break;
	case 0x08           :
        case 0x7F           :
        case KEY_BACKSPACE  : if( p > 0 )
                              {
                                n = StrVisualLength(s);
			        ls = StrLeft(s, p - 1);
				rs = StrRight(s, n - p);
				strcpy(s, ls);
				strcat(s, rs);
				free(ls);
				free(rs);
				MvAddStr( y, x, s );
                                for(i=StrVisualLength(s); i < n; i++)
                                  addch( '_' );
                                p--;
                              }
                              else
                                beep();
                              break;
        case KEY_DL         : for(i=0; i < StrVisualLength(s) - p; i++) addch( '_' );
			      ls = StrLeft(s, p);
			      strcpy(s, ls);
			      free(ls);
                              break;
	case KEY_EIC        :
        case KEY_IC         : insert_flag ^= TRUE; 
                              break;
	case '\t'           : if(( pp = GetMatches(s)) == NULL) {
			       break;
			     }
                             if(*pp)
                             {
			       ls = StrLeft(pp, length);
			       strcpy(s, ls);
			       free(ls);
			       p = StrVisualLength(s);
			       free(pp);
                               MvAddStr( y, x, s );
                               for(i=p; i < length; i++) addch( '_' );
                               RefreshWindow( stdscr );
                               doupdate();
                             }
                             break;
#ifdef KEY_RESIZE

        case KEY_RESIZE:     resize_request = TRUE;
                             break;
#endif

#ifdef KEY_F
        case KEY_F(2)       : 
#endif
        case 'F' & 0x1f     : if(KeyF2Get( statistic.tree,
                                           statistic.disp_begin_pos,
                                           statistic.cursor_pos, path)) 
                              {
			        /* beep(); */
				break;
			      }
                              if(*path)
                              {
			        ls = StrLeft(path, length);
				strcpy(s, ls);
				free(ls);
			        p = StrVisualLength(s);
                                MvAddStr( y, x, s );
                                for(i=p; i < length; i++) addch( '_' );
                                RefreshWindow( stdscr );
                                doupdate();
                              }
                              break;

        default             : if( c1 == LF ) c1 = CR;
                              break;
        } /* switch */
      } /* else control symbols */
    } else {
     
      if (strlen(sbuf) > 0) {
       if ( insert_flag ) {
  	    /* append symbol */
	    if ( p >= StrVisualLength(s)) strcat(s, sbuf);
	    else {
	      /* insert symbol at cursor position */
	      if ( p > 0 ) ls = StrLeft(s, p);
	      else ls = Strdup("");
	      rs = StrRight(s, StrVisualLength(s) - p);
	      strcpy(s, ls);
	      strcat(s, sbuf);
	      strcat(s, rs);
	      free(ls);
	      free(rs);
	    }
          } else {
            /* owerwrite symbol at cursor position */
	    if ( p > StrVisualLength(s) - 1) strcat(s, sbuf);
	    else {
  	      if (p > 0) ls = StrLeft(s, p);
	      else ls = Strdup("");
	      rs = StrRight(s, StrVisualLength(s) - p - 1);
	      strcpy(s, ls);
	      strcat(s, sbuf);
	      strcat(s, rs);
	      free(ls);
	      free(rs);
	    }
	  }

	strcpy(sbuf, "");
	p++;
      }

      if (StrVisualLength(s) >= length) len_flag = TRUE;
      else len_flag = FALSE;

      MvAddStr( y, x, s );
      wmove(stdscr, y, x + p);
    }
  } while( c1 !=  27 && c1 != CR );

  nodelay( stdscr, FALSE );

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
  pp = Strdup(s);
#endif

  strncpy( s, pp, length - 1);
  s[length]='\0';
  free(pp);
  return( c1 );
}





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





int GetTapeDeviceName( void )
{
  int  result;
  char path[PATH_LENGTH * 2 +1];

  result = -1;

  ClearHelp();

  (void) strcpy( path, statistic.tape_name );

  MvAddStr( LINES - 2, 1, "Tape-Device:" );
  if( InputString( path, LINES - 2, 14, 0, COLS - 15, "\r\033" ) == CR )
  {
    result = 0;
    (void) strcpy( statistic.tape_name, path );
  }

  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}


void HitReturnToContinue(void)
{
#ifndef XCURSES
  curs_set(1);
  vidattr( A_REVERSE );
  putp( "[Hit return to continue]" );
  (void) fflush( stdout );
  (void) Getch();
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

  
#ifdef _IBMR2
#undef wgetch

int AixWgetch( WINDOW *w )
{
  int c;
  
  if( ( c = wgetch( w ) ) == KEY_ENTER ) c = LF;

  return( c );
}

#endif


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