/***************************************************************************
 *
 * Funktionen zum Key-Handling
 *
 ***************************************************************************/


#include "ytree.h"


#ifdef READLINE_SUPPORT
#include <readline/readline.h>
#include <readline/tilde.h>
#endif


#define MAX(a,b) (((a) > (b)) ? (a):(b))

static char **Mtchs       = NULL;
static int total_matches  = 0;
static int cursor_pos     = 0;
static int disp_begin_pos = 1;

static void PrintMtchEntry(int entry_no, int y, int color, int start_x, int *hide_left, int *hide_right);
static int DisplayMatches();



static void PrintMtchEntry(int entry_no, int y, int color, 
                           int start_x, int *hide_left, int *hide_right)
{
  int     n;
  char    buffer[BUFSIZ];
  char    *line_ptr;
  int     window_width;
  int     window_height;
  int     ef_window_width;

  
  GetMaxYX( matches_window, &window_height, &window_width );
  ef_window_width = window_width - 2; /* Effektive Window-Width */

#ifdef NO_HIGHLIGHT
  ef_window_width = window_width - 3; /* Effektive Window-Width */
#else
  ef_window_width = window_width - 2; /* Effektive Window-Width */
#endif

  *hide_left = *hide_right = 0;

  if(Mtchs[entry_no])
  {
    (void) strncpy( buffer,(char *) Mtchs[entry_no], BUFSIZ - 3);
    buffer[BUFSIZ - 3] = '\0';
    n = strlen( buffer );
    wmove(matches_window,y,1);

    if(n <= ef_window_width) {

      /* will completely fit into window */
      /*---------------------------------*/

      line_ptr = buffer;
    } else {
      /* does not completely fit into window;
       * ==> use start_x
       */

      if(n > (start_x + ef_window_width))
        line_ptr = &buffer[start_x];
      else
        line_ptr = &buffer[n - ef_window_width];

      *hide_left = start_x;
      *hide_right = n - start_x - ef_window_width;

      line_ptr[ef_window_width] ='\0';
    }

#ifdef NO_HIGHLIGHT
    strcat(line_ptr, (color == HIMTCH_COLOR) ? " <" : "  ");
    WAddStr( matches_window, line_ptr );
#else
#ifdef COLOR_SUPPORT 
    WbkgdSet(matches_window, COLOR_PAIR(color)|A_BOLD);
#else
    if(color == HIMTCH_COLOR)
      wattrset( matches_window, A_REVERSE );
#endif /* COLOR_SUPPORT */
    WAddStr( matches_window, line_ptr );
#ifdef COLOR_SUPPORT
    WbkgdSet(matches_window, COLOR_PAIR(WINMTCH_COLOR)| A_BOLD);
#else
    if(color == HIMTCH_COLOR)
      wattrset( matches_window, 0 );
#endif /* COLOR_SUPPORT */
#endif /* NO_HIGHLIGHT */
  }
  return;
}




static int DisplayMatches()
{
  int i, hilight_no, p_y;
  int hide_left, hide_right;

  hilight_no = disp_begin_pos + cursor_pos;
  p_y = -1;
  werase( matches_window );
  for(i=0; i < MATCHES_WINDOW_HEIGHT; i++)
  {
    if (disp_begin_pos + i >= total_matches ) break;
    if (disp_begin_pos + i != hilight_no )
        PrintMtchEntry(disp_begin_pos + i, i, MTCH_COLOR, 
	              0, &hide_left, &hide_right);
    else
      p_y = i;
  }
  if(p_y >= 0) {
    PrintMtchEntry(disp_begin_pos + p_y, p_y, HIMTCH_COLOR,
	          0, &hide_left, &hide_right);
  }
  return 0;
}


char *GetMatches( char *base)
{
  int     ch;
  int     start_x;
  char    *RetVal = NULL;
  char    *TMP;
  char    *tmpval;
  int     hide_left, hide_right;

/*  tmpval = rl_filename_completion_function(base, 0);
  if (!strcmp(tmpval,base))
    return(tmpval);*/

  Mtchs = NULL;

#ifdef READLINE_SUPPORT
  tmpval=tilde_expand(base);

  if ((Mtchs = rl_completion_matches(tmpval, rl_filename_completion_function)) 
	== NULL)
    return(NULL);
#else
    return(NULL);
#endif


  if (!(strcmp(tmpval,Mtchs[0])==0)){
    TMP=malloc(strlen(Mtchs[0])+1);
    if (TMP != NULL){
      strcpy(TMP, Mtchs[0]);
      RetVal = TMP;
    }else{
      RetVal = NULL;}
    free(Mtchs);
    free(tmpval);
    return RetVal;
  }
  
  for (total_matches=0; Mtchs[total_matches]; total_matches++);
  if (total_matches == 1)
  return(NULL);
  
  disp_begin_pos = 1;
  cursor_pos     = 0;
  start_x        = 0;
  /* leaveok(stdscr, TRUE); */
  (void) DisplayMatches();

  do
  {
    RefreshWindow( matches_window );
    doupdate();
    ch = Getch();

    if(ch != -1 && ch != KEY_RIGHT && ch != KEY_LEFT) {
      if(start_x) {
        start_x = 0;
	PrintMtchEntry( disp_begin_pos + cursor_pos,
		       cursor_pos, HIMTCH_COLOR, 
		       start_x, &hide_left, &hide_right);
      }
    }

    switch( ch )
    {
      case -1:       RetVal = NULL;
                     break;

      case ' ':      break;  /* Quick-Key */

      case KEY_RIGHT: start_x++;
		      PrintMtchEntry( disp_begin_pos + cursor_pos,
			             cursor_pos, HIMTCH_COLOR,
		                     start_x, &hide_left, &hide_right);
		      if(hide_right < 0) 
		        start_x--;
		      break;
      
      case KEY_LEFT:  if(start_x > 0)
       		        start_x--;
		      PrintMtchEntry( disp_begin_pos + cursor_pos,
			             cursor_pos, HIMTCH_COLOR,
		                     start_x, &hide_left, &hide_right);
		      break;
      
      case '\t':
      case KEY_DOWN: if (disp_begin_pos + cursor_pos+1 >= total_matches)
      		     {
		       beep();
		     }
		     else
		     { if( cursor_pos + 1 < MATCHES_WINDOW_HEIGHT )
		       {
			 PrintMtchEntry( disp_begin_pos + cursor_pos,
					cursor_pos, MTCH_COLOR,
		                        start_x, &hide_left, &hide_right);
			 cursor_pos++;
			 PrintMtchEntry( disp_begin_pos + cursor_pos,
					cursor_pos, HIMTCH_COLOR,
		                        start_x, &hide_left, &hide_right);
                       }
		       else
		       {
			 PrintMtchEntry( disp_begin_pos + cursor_pos, 
					cursor_pos, MTCH_COLOR,
		                        start_x, &hide_left, &hide_right);
			 scroll( matches_window );
			 disp_begin_pos++;
			 PrintMtchEntry( disp_begin_pos + cursor_pos, 
					cursor_pos, HIMTCH_COLOR,
		                        start_x, &hide_left, &hide_right);
                       }
		     }
                     break;
      case KEY_BTAB:
      case KEY_UP  : if( disp_begin_pos + cursor_pos - 1 < 1 )
		     {   beep(); }
		     else
		     {
		       if( cursor_pos - 1 >= 0 )
		       {
			 PrintMtchEntry( disp_begin_pos + cursor_pos, 
					cursor_pos, MTCH_COLOR,
		                        start_x, &hide_left, &hide_right);
			 cursor_pos--;
			 PrintMtchEntry( disp_begin_pos + cursor_pos,
					cursor_pos, HIMTCH_COLOR,
		                        start_x, &hide_left, &hide_right);
                       }
		       else
		       {
			 PrintMtchEntry( disp_begin_pos + cursor_pos, 
					cursor_pos, MTCH_COLOR,
		                        start_x, &hide_left, &hide_right);
			 wmove( matches_window, 0, 0 );
			 winsertln( matches_window );
			 disp_begin_pos--;
			 PrintMtchEntry( disp_begin_pos + cursor_pos, 
					cursor_pos, HIMTCH_COLOR,
		                        start_x, &hide_left, &hide_right);
                       }
		     }
                     break;
      case KEY_NPAGE:
      		     if( disp_begin_pos + cursor_pos >= total_matches - 1 )
		     {  beep();  }
		     else
		     {
		       if( cursor_pos < MATCHES_WINDOW_HEIGHT - 1 )
		       {
			 PrintMtchEntry( disp_begin_pos + cursor_pos, 
					cursor_pos, MTCH_COLOR,
		                        start_x, &hide_left, &hide_right);
		         if( disp_begin_pos + MATCHES_WINDOW_HEIGHT > total_matches  - 1 )
			   cursor_pos = total_matches - disp_begin_pos - 1;
			 else
			   cursor_pos = MATCHES_WINDOW_HEIGHT - 1;
			 PrintMtchEntry( disp_begin_pos + cursor_pos, 
					cursor_pos, HIMTCH_COLOR,
		                        start_x, &hide_left, &hide_right);
		       }
		       else
		       {
			 if( disp_begin_pos + cursor_pos + MATCHES_WINDOW_HEIGHT < total_matches )
			 {
			   disp_begin_pos += MATCHES_WINDOW_HEIGHT;
			   cursor_pos = MATCHES_WINDOW_HEIGHT - 1;
			 }
			 else
			 {
			   disp_begin_pos = total_matches - MATCHES_WINDOW_HEIGHT;
			   if( disp_begin_pos < 1 ) disp_begin_pos = 1;
			   cursor_pos = total_matches - disp_begin_pos - 1;
			 }
                         DisplayMatches();
		       } 
		     }
                     break;
      case KEY_PPAGE:
		     if( disp_begin_pos + cursor_pos <= 1 )
		     {  beep();  }
		     else
		     {
		       if( cursor_pos > 0 )
		       {
			 PrintMtchEntry( disp_begin_pos + cursor_pos, 
					cursor_pos, MTCH_COLOR,
		                        start_x, &hide_left, &hide_right);
			 cursor_pos = 0;
			 PrintMtchEntry( disp_begin_pos + cursor_pos, 
					cursor_pos, HIMTCH_COLOR,
		                        start_x, &hide_left, &hide_right);
		       }
		       else
		       {
			 if( (disp_begin_pos -= MATCHES_WINDOW_HEIGHT) < 1 )
			 {
			   disp_begin_pos = 1;
			 }
                         cursor_pos = 0;
                         DisplayMatches();
		       } 
		     }
                     break;
      case KEY_HOME: if( disp_begin_pos == 1 && cursor_pos == 0 )
		     {   beep();    }
		     else
		     {
		       disp_begin_pos = 1;
		       cursor_pos     = 0;
                       DisplayMatches();
		     }
                     break;
      case KEY_END : 
                     disp_begin_pos = MAX(1, total_matches - MATCHES_WINDOW_HEIGHT);
		     cursor_pos     = total_matches - disp_begin_pos - 1;
                     DisplayMatches();
                     break;
      case LF :
      case CR : 
                     TMP=malloc(strlen(Mtchs[ disp_begin_pos + cursor_pos])+1);
		     if (TMP != NULL){
		        strcpy(TMP, Mtchs[disp_begin_pos + cursor_pos]);
                        RetVal = TMP;
		     }else
                        RetVal = NULL;
		     break;

      case ESC:      RetVal = NULL;
                     break;

      default :      beep();
		     break;
    } /* switch */
  } while(ch != CR && ch != ESC && ch != -1);
  /* leaveok(stdscr, FALSE); */
  free(Mtchs);
  free(tmpval);
  touchwin(stdscr);
  return RetVal;
}