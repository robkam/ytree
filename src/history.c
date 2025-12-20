/***************************************************************************
 *
 * history.c
 * F2 function key History
 *
 ***************************************************************************/


#include "ytree.h"


static void PrintHstEntry(int entry_no, int y, int color, int start_x, int *hide_left, int *hide_right);
static int DisplayHistory();



#define MAX_HST_FILE_LINES 50
#define MAX(a,b) (((a) > (b)) ? (a):(b))

typedef struct _history
{
  char             *hst;
  struct _history  *next;
  struct _history  *prev;
} History;


static int total_hist     = 0;
static int cursor_pos     = 0;
static int disp_begin_pos = 0;
static History *Hist = NULL ;



void ReadHistory(char *Filename)
{
  FILE *HstFile;
  char buffer[BUFSIZ];

  if ( (HstFile = fopen( Filename, "r" ) ) != NULL) {
    while( fgets( buffer, sizeof( buffer ), HstFile ) )
    {
        if (strlen(buffer) > 0)
        {
          buffer[strlen(buffer) -1] = '\0';
          InsHistory(buffer);
        }
    }
    fclose( HstFile );
  }
  return;
}



void SaveHistory(char *Filename)
{
  FILE *HstFile;
  int j;
  History *hst, *last_hst;

  hst = last_hst = NULL;

  if((hst = Hist)) {
    if( (HstFile = fopen( Filename, "w" ) ) != NULL) {
      for(j = 0 ; hst && j < MAX_HST_FILE_LINES; j++ ) {
        last_hst = hst;
        hst = hst->next;
      }

      /* write in reverse order */
      for(hst=last_hst; hst; hst=hst->prev) {
         fputs(hst->hst, HstFile);
         fputs("\n", HstFile);
      }
      fclose( HstFile );
    }
  }
  return;
}


void InsHistory( char *NewHst)
{
   History *TMP, *TMP2 = NULL;
   int flag = 0;

   if (strlen(NewHst)== 0)
     return;

   TMP2 = Hist;
   for ( TMP = Hist; TMP != NULL; TMP = TMP -> next)
   {
       if (strcmp(TMP -> hst, NewHst) == 0)
       {
         if (TMP2 != TMP)
         {
            TMP2 -> next = TMP -> next;
            TMP -> next = Hist;
            Hist = TMP;
         }
         flag = 1;
         break;
       };
       TMP2 = TMP;
   }

   if (flag == 0)
   {
      if ((TMP=(History *) malloc(sizeof(struct _history))) != NULL)
      {
         TMP -> next = Hist;
	 TMP -> prev = NULL;
	 TMP -> hst = strdup(NewHst);
	 if(TMP->hst == NULL)
	 {
	    ERROR_MSG("strdup failed*ABORT");
            exit(1);
	 }

         if (Hist != NULL)
	     Hist -> prev = TMP;
         Hist = TMP;
         total_hist++;
      }
   }
   return;
}



static void PrintHstEntry(int entry_no, int y, int color,
                          int start_x, int *hide_left, int *hide_right)
{
  int     n;
  History *pp;
  char    buffer[BUFSIZ];
  char    *line_ptr;
  int     window_width;
  int     window_height;
  int     ef_window_width;


  GetMaxYX( history_window, &window_height, &window_width );
  ef_window_width = window_width - 2; /* Effektive Window-Width */

#ifdef NO_HIGHLIGHT
  ef_window_width = window_width - 3; /* Effektive Window-Width */
#else
  ef_window_width = window_width - 2; /* Effektive Window-Width */
#endif

  *hide_left = *hide_right = 0;

  for(n=0, pp=Hist; pp && (n < entry_no); pp = pp->next)
  {
    n++;
  }

  if(pp)
  {
    (void) strncpy( buffer, pp->hst, BUFSIZ - 3);
    buffer[BUFSIZ - 3] = '\0';
    n = strlen( buffer );
    wmove(history_window,y,1);

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
    strcat(line_ptr, (color == CPAIR_HIHST) ? " <" : "  ");
    WAddStr( history_window, line_ptr );
#else
#ifdef COLOR_SUPPORT
    WbkgdSet(history_window, COLOR_PAIR(color)|A_BOLD);
#else
    if(color == CPAIR_HIHST)
      wattrset( history_window, A_REVERSE );
#endif /* COLOR_SUPPORT */
    WAddStr( history_window, line_ptr );
#ifdef COLOR_SUPPORT
    WbkgdSet(history_window, COLOR_PAIR(CPAIR_WINHST)| A_BOLD);
#else
    if(color == CPAIR_HIHST)
      wattrset( history_window, 0 );
#endif /* COLOR_SUPPORT */
#endif /* NO_HIGHLIGHT */
  }
  return;
}




static int DisplayHistory()
{
  int i, hilight_no, p_y;
  int hide_left, hide_right;

  hilight_no = disp_begin_pos + cursor_pos;
  p_y = -1;
  werase( history_window );
  for(i=0; i < HISTORY_WINDOW_HEIGHT; i++)
  {
    if (disp_begin_pos + i >= total_hist ) break;
    if (disp_begin_pos + i != hilight_no )
        PrintHstEntry(disp_begin_pos + i, i, CPAIR_HST,
	              0, &hide_left, &hide_right);
    else
      p_y = i;
  }
  if(p_y >= 0) {
    PrintHstEntry(disp_begin_pos + p_y, p_y, CPAIR_HIHST,
	          0, &hide_left, &hide_right);
  }
  return 0;
}



char *GetHistory()
{
  int     ch, tmp;
  int     start_x;
  char    *RetVal = NULL;
  History *TMP;
  int     hide_left, hide_right;


  disp_begin_pos = 0;
  cursor_pos     = 0;
  start_x        = 0;
  /* leaveok(stdscr, TRUE); */
  (void) DisplayHistory();

  do
  {
    RefreshWindow( history_window );
    doupdate();
    ch = Getch();

    if(ch != -1 && ch != KEY_RIGHT && ch != KEY_LEFT) {
      if(start_x) {
        start_x = 0;
	PrintHstEntry( disp_begin_pos + cursor_pos,
		       cursor_pos, CPAIR_HIHST,
		       start_x, &hide_left, &hide_right);
      }
    }

    switch( ch )
    {
      case -1:       RetVal = NULL;
                     break;

      case ' ':      break;  /* Quick-Key */

      case KEY_RIGHT: start_x++;
		      PrintHstEntry( disp_begin_pos + cursor_pos,
			             cursor_pos, CPAIR_HIHST,
		                     start_x, &hide_left, &hide_right);
		      if(hide_right < 0)
		        start_x--;
		      break;

      case KEY_LEFT:  if(start_x > 0)
       		        start_x--;
		      PrintHstEntry( disp_begin_pos + cursor_pos,
			             cursor_pos, CPAIR_HIHST,
		                     start_x, &hide_left, &hide_right);
		      break;

      case '\t':
      case KEY_DOWN: if (disp_begin_pos + cursor_pos+1 >= total_hist)
      		     {
		       beep();
		     }
		     else
		     { if( cursor_pos + 1 < HISTORY_WINDOW_HEIGHT )
		       {
			 PrintHstEntry( disp_begin_pos + cursor_pos,
					cursor_pos, CPAIR_HST,
		                        start_x, &hide_left, &hide_right);
			 cursor_pos++;
			 PrintHstEntry( disp_begin_pos + cursor_pos,
					cursor_pos, CPAIR_HIHST,
		                        start_x, &hide_left, &hide_right);
                       }
		       else
		       {
			 PrintHstEntry( disp_begin_pos + cursor_pos,
					cursor_pos, CPAIR_HST,
		                        start_x, &hide_left, &hide_right);
			 scroll( history_window );
			 disp_begin_pos++;
			 PrintHstEntry( disp_begin_pos + cursor_pos,
					cursor_pos, CPAIR_HIHST,
		                        start_x, &hide_left, &hide_right);
                       }
		     }
                     break;
      case KEY_BTAB:
      case KEY_UP  : if( disp_begin_pos + cursor_pos - 1 < 0 )
		     {   beep(); }
		     else
		     {
		       if( cursor_pos - 1 >= 0 )
		       {
			 PrintHstEntry( disp_begin_pos + cursor_pos,
					cursor_pos, CPAIR_HST,
		                        start_x, &hide_left, &hide_right);
			 cursor_pos--;
			 PrintHstEntry( disp_begin_pos + cursor_pos,
					cursor_pos, CPAIR_HIHST,
		                        start_x, &hide_left, &hide_right);
                       }
		       else
		       {
			 PrintHstEntry( disp_begin_pos + cursor_pos,
					cursor_pos, CPAIR_HST,
		                        start_x, &hide_left, &hide_right);
			 wmove( history_window, 0, 0 );
			 winsertln( history_window );
			 disp_begin_pos--;
			 PrintHstEntry( disp_begin_pos + cursor_pos,
					cursor_pos, CPAIR_HIHST,
		                        start_x, &hide_left, &hide_right);
                       }
		     }
                     break;
      case KEY_NPAGE:
      		     if( disp_begin_pos + cursor_pos >= total_hist - 1 )
		     {  beep();  }
		     else
		     {
		       if( cursor_pos < HISTORY_WINDOW_HEIGHT - 1 )
		       {
			 PrintHstEntry( disp_begin_pos + cursor_pos,
					cursor_pos, CPAIR_HST,
		                        start_x, &hide_left, &hide_right);
		         if( disp_begin_pos + HISTORY_WINDOW_HEIGHT > total_hist  - 1 )
			   cursor_pos = total_hist - disp_begin_pos - 1;
			 else
			   cursor_pos = HISTORY_WINDOW_HEIGHT - 1;
			 PrintHstEntry( disp_begin_pos + cursor_pos,
					cursor_pos, CPAIR_HIHST,
		                        start_x, &hide_left, &hide_right);
		       }
		       else
		       {
			 if( disp_begin_pos + cursor_pos + HISTORY_WINDOW_HEIGHT < total_hist )
			 {
			   disp_begin_pos += HISTORY_WINDOW_HEIGHT;
			   cursor_pos = HISTORY_WINDOW_HEIGHT - 1;
			 }
			 else
			 {
			   disp_begin_pos = total_hist - HISTORY_WINDOW_HEIGHT;
			   if( disp_begin_pos < 0 ) disp_begin_pos = 0;
			   cursor_pos = total_hist - disp_begin_pos - 1;
			 }
                         DisplayHistory();
		       }
		     }
                     break;
      case KEY_PPAGE:
		     if( disp_begin_pos + cursor_pos <= 0 )
		     {  beep();  }
		     else
		     {
		       if( cursor_pos > 0 )
		       {
			 PrintHstEntry( disp_begin_pos + cursor_pos,
					cursor_pos, CPAIR_HST,
		                        start_x, &hide_left, &hide_right);
			 cursor_pos = 0;
			 PrintHstEntry( disp_begin_pos + cursor_pos,
					cursor_pos, CPAIR_HIHST,
		                        start_x, &hide_left, &hide_right);
		       }
		       else
		       {
			 if( (disp_begin_pos -= HISTORY_WINDOW_HEIGHT) < 0 )
			 {
			   disp_begin_pos = 0;
			 }
                         cursor_pos = 0;
                         DisplayHistory();
		       }
		     }
                     break;
      case KEY_HOME: if( disp_begin_pos == 0 && cursor_pos == 0 )
		     {   beep();    }
		     else
		     {
		       disp_begin_pos = 0;
		       cursor_pos     = 0;
                       DisplayHistory();
		     }
                     break;
      case KEY_END :
                     disp_begin_pos = MAX(0, total_hist - HISTORY_WINDOW_HEIGHT);
		     cursor_pos     = total_hist - disp_begin_pos - 1;
                     DisplayHistory();
                     break;
      case LF :
      case CR :
                     TMP = Hist;
                     for(tmp = 0; (tmp != disp_begin_pos + cursor_pos); tmp++)
                     {
                        TMP = TMP -> next;
                        if (TMP == NULL)
                          break;
                     }
                     if (TMP != NULL)
                        RetVal = TMP -> hst;
                     else
                        RetVal = NULL;
		     break;

      case ESC:      RetVal = NULL;
                     break;

      default :      beep();
		     break;
    } /* switch */
  } while(ch != CR && ch != ESC && ch != -1);
  /* leaveok(stdscr, FALSE); */
  touchwin(stdscr);
  return RetVal;
}