/***************************************************************************
 *
 * Termcap Term-I/O
 *
 ***************************************************************************/


#include "ytree.h"



#ifdef TERMCAP



#ifdef __NeXT__

char	tcdummy[256], tcbuf[1024];
char	*tcdptr = tcdummy;

#else
# define tcbuf		NULL
# define tcdummy	NULL
#endif /* __NeXT__ */


#undef wgetch
#undef initscr
#undef endwin


typedef struct
{
  int   key_code;
  char  *termcap_name;
  char  *key_sequence;
} SpecialKey;


static SpecialKey special_key[] = {
                                    { KEY_BTAB,      "kB", NULL },
                                    { KEY_DOWN,      "kd", NULL },
                                    { KEY_UP,        "ku", NULL },
                                    { KEY_LEFT,      "kl", NULL },
                                    { KEY_RIGHT,     "kr", NULL },
                                    { KEY_END,       "kH", NULL },
                                    { KEY_HOME,      "kh", NULL },
                                    { KEY_NPAGE,     "kN", NULL },
                                    { KEY_PPAGE,     "kP", NULL },
                                    { KEY_DC,        "kD", NULL },
                                    { KEY_BACKSPACE, "kb", NULL },
                                    { KEY_EIC,       "kM", NULL },
                                    { KEY_IC,        "kI", NULL },
                                    { KEY_DL,        "kL", NULL },
                                    { -1, NULL, NULL }
		                  };



static int TermcapMultiByte( WINDOW *win, int ch );
static void SigAlarm( int signo );
static jmp_buf env_alarm;
static char *reverse;
static char *normal;



void TermcapVidattr( int attr )
{
  if( normal && reverse )
  {
    if( attr == 0 ) putp( normal );
    else
    {
      if( attr & A_REVERSE ) putp( reverse );
    }
  }
}



int TermcapWgetch( WINDOW *win )
{
  int        ch;
  SpecialKey *sk_ptr;
  BOOL       one_matched = FALSE;
  int        i;

  ch = wgetch( stdscr );

  for( i=0, sk_ptr = special_key; 
       sk_ptr->key_code > 0; 
       sk_ptr = &special_key[++i] 
     )
  {
    /* Ein-Byte Codes ausblenden */
    /*---------------------------*/

    if( sk_ptr->key_sequence && *(sk_ptr->key_sequence) == ch )
    {
      /* Erstes Zeichen matched */
      /*------------------------*/

      if( sk_ptr->key_sequence[1] == '\0' )
      { 
        /* 1 Byte-Code */
        /*-------------*/

        ch = sk_ptr->key_code;
        break;
      }
      one_matched = TRUE;
    }
  }

  if( one_matched )
  {
    /* Multi-Byte Code */
    /*-----------------*/

    ch = TermcapMultiByte( win, ch );
  }
  return( ch );
}



static int TermcapMultiByte( WINDOW *win, int ch )
{
  SpecialKey *sk_ptr;
  char buffer[80];
  BOOL found;
  int i, j;

  memset( buffer, 0, sizeof( buffer ) );
  *buffer = ch;

  if( signal( SIGALRM, SigAlarm ) == SIG_ERR )
  {
    ERROR_MSG( "signal SIGALRM failed*ABORT" );
    exit( 1 );
  }

  if( setjmp( env_alarm ) )
  {
    return( ch );
  }

  alarm( 1 );

  for( j=1, found = FALSE; !found && j < sizeof( buffer ); j++ )
  {
    buffer[j] = wgetch( win );

    for( i=0, sk_ptr = special_key; 
	 sk_ptr->key_code > 0; 
	 sk_ptr = &special_key[++i] 
       )
    {
      if( sk_ptr->key_sequence && !strcmp( sk_ptr->key_sequence, buffer ) )
      {
        ch = sk_ptr->key_code;
        found = TRUE;
        break;
      }
    }
  }
  
  alarm( 0 );

  return( ch );
}



static void SigAlarm( int signo )
{
  longjmp( env_alarm, 1 );
}


void TermcapInitscr()
{
  int i;

  initscr();

  tgetent( tcbuf, getenv( "TERM" ) );

  for( i=0; special_key[i].key_code > 0; i++ )
  {
#ifdef __NeXT__
    tcdptr = tcdummy;
    special_key[i].key_sequence = Strdup(tgetstr( special_key[i].termcap_name, &tcdptr ));
#else
    special_key[i].key_sequence = tgetstr( special_key[i].termcap_name, NULL );
#endif
  }
#ifdef __NeXT__
  reverse = Strdup(tgetstr( "so", &tcdptr ));
  normal  = Strdup(tgetstr( "se", &tcdptr ));
#else
  reverse = tgetstr( "so", tcdummy );
  normal  = tgetstr( "se", tcdummy );
#endif
}



void TermcapEndwin()
{
  nl();
  endwin();
  putchar( '\n' );
}


#endif /* TERMCAP */


