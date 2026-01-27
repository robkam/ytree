/***************************************************************************
 *
 * src/ui/error.c
 * Output of error messages
 *
 ***************************************************************************/


#include "ytree.h"
#include "patchlev.h"



static void MapErrorWindow(char *header);
static void MapNoticeWindow(char *header);
static void UnmapErrorWindow(void);
static void PrintErrorLine(int y, char *str);
static void DisplayMessage(char *msg);
static int  PrintMessage(char *msg, BOOL do_beep);



void UI_Message(const char *fmt, ...)
{
  char buffer[MESSAGE_LENGTH + 1];
  va_list ap;

  va_start(ap, fmt);
  (void) vsnprintf(buffer, sizeof(buffer), fmt, ap);
  va_end(ap);

  MapErrorWindow( "E R R O R" );
  (void) PrintMessage( buffer, TRUE );
}


void UI_Notice(const char *fmt, ...)
{
  char buffer[MESSAGE_LENGTH + 1];
  va_list ap;

  va_start(ap, fmt);
  (void) vsnprintf(buffer, sizeof(buffer), fmt, ap);
  va_end(ap);

  MapNoticeWindow( "N O T I C E" );
  DisplayMessage( buffer );
  RefreshWindow( error_window );
  refresh();
}


void UI_Warning(const char *fmt, ...)
{
  char buffer[MESSAGE_LENGTH + 1];
  va_list ap;

  va_start(ap, fmt);
  (void) vsnprintf(buffer, sizeof(buffer), fmt, ap);
  va_end(ap);

  MapErrorWindow( "W A R N I N G" );
  (void) PrintMessage( buffer, FALSE );
}


void AboutBox()
{
  static char version[80];

  (void) snprintf( version, sizeof(version),
#ifdef WITH_UTF8
                  "ytree (UTF8) Version %s %s*",
#else
                  "ytree Version %s %s*",
#endif
                  VERSION,
                  VERSIONDATE
                );

  MapErrorWindow( "ABOUT" );
  (void) PrintMessage( version, FALSE );
}


void UI_Error(const char *module, int line, const char *fmt, ...)
{
  char msg_buffer[MESSAGE_LENGTH + 1];
  char final_buffer[MESSAGE_LENGTH + 1];
  va_list ap;

  va_start(ap, fmt);
  (void) vsnprintf(msg_buffer, sizeof(msg_buffer), fmt, ap);
  va_end(ap);

  MapErrorWindow( "INTERNAL ERROR" );
  (void) snprintf( final_buffer, sizeof(final_buffer), "%s*In Module \"%s\"*Line %d",
		  msg_buffer, module, line
		);
  (void) PrintMessage( final_buffer, TRUE );
}





static void MapErrorWindow(char *header)
{
   werase( error_window );
   box( error_window, 0, 0 );

   PrintSpecialString( error_window,
		       ERROR_WINDOW_HEIGHT - 3,
		       0,
		       "6--------------------------------------7",
		       CPAIR_WINERR
		     );
   wattrset( error_window, A_REVERSE | A_BLINK );
   MvWAddStr( error_window,
	      ERROR_WINDOW_HEIGHT - 2,
	      1,
	      "             PRESS ENTER              "
	    );
   wattrset( error_window, 0 );
   PrintErrorLine( 1, header );
}


static void MapNoticeWindow(char *header)
{
   werase( error_window );
   box( error_window, 0, 0 );

   PrintSpecialString( error_window,
		       ERROR_WINDOW_HEIGHT - 3,
		       0,
		       "6--------------------------------------7",
		       CPAIR_WINERR
		     );
   wattrset( error_window, A_REVERSE | A_BLINK );
   MvWAddStr( error_window,
	      ERROR_WINDOW_HEIGHT - 2,
	      1,
	      "             PLEASE WAIT              "
	    );
   wattrset( error_window, 0 );
   PrintErrorLine( 1, header );
}


static void UnmapErrorWindow(void)
{
   werase( error_window );
   touchwin( stdscr );
   doupdate();
}


void UnmapNoticeWindow(void)
{
   werase( error_window );
   touchwin( stdscr );
   doupdate();
}



static void PrintErrorLine(int y, char *str)
{
  int l;

  l = strlen( str );

  MvWAddStr( error_window, y, (ERROR_WINDOW_WIDTH - l) >> 1, str );
}




static void DisplayMessage(char *msg)
{
  int  y, i, j, count;
  char buffer[ERROR_WINDOW_WIDTH - 2 + 1];

  for(i=0, count=0; msg[i]; i++)
    if( msg[i] == '*' ) count++;

  if( count > 3 )      y = 2;
  else if( count > 1 ) y = 3;
  else                 y = 4;


  for( i=0,j=0; msg[i]; i++ )
  {
    if( msg[i] == '*' )
    {
      buffer[j] = '\0';
      PrintErrorLine( y++, buffer );
      j=0;
    }
    else
    {
      if( j < (int)((sizeof( buffer) - 1)) ) buffer[j++] = msg[i];
    }
  }
  buffer[j] = '\0';
  PrintErrorLine( y, buffer );
}



static int PrintMessage(char *msg, BOOL do_beep)
{
  int c;

  DisplayMessage( msg );
  if(do_beep && (strtol(AUDIBLEERROR, NULL, 0) != 0))
    beep();
  RefreshWindow( error_window );
  doupdate();
  c = wgetch(error_window);
  UnmapErrorWindow();
  touchwin( dir_window );
  return( c );
}