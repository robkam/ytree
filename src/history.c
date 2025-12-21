/***************************************************************************
 *
 * history.c
 * Input History Management
 *
 ***************************************************************************/


#include "ytree.h"


static void PrintHstEntry(int entry_no, int y, int color, int start_x, int *hide_left, int *hide_right);
static int DisplayHistory();



#define MAX_HST_FILE_LINES 200
#define MAX(a,b) (((a) > (b)) ? (a):(b))

typedef struct _history
{
  char             *hst;
  int              type;
  int              pinned;
  struct _history  *next;
  struct _history  *prev;
} History;


static int total_hist     = 0;
static int cursor_pos     = 0;
static int disp_begin_pos = 0;
static History *Hist = NULL ;

/* Helper view structure for sorting/filtering */
static History **ViewList = NULL;
static int view_count = 0;


static void FreeViewList(void) {
    if (ViewList) {
        free(ViewList);
        ViewList = NULL;
    }
    view_count = 0;
    total_hist = 0;
}


static void BuildViewList(int type) {
    History *ptr;
    int i;
    /* int pinned_count = 0; // Unused */
    /* int unpinned_count = 0; // Unused */

    FreeViewList();

    /* First, count matching items */
    for (ptr = Hist; ptr; ptr = ptr->next) {
        if (ptr->type == type) {
            view_count++;
        }
    }
    total_hist = view_count;

    if (view_count == 0) return;

    ViewList = (History **)malloc(view_count * sizeof(History *));
    if (!ViewList) {
        ERROR_MSG("Malloc failed for history view*ABORT");
        exit(1);
    }

    /* Populate ViewList: Pinned first (preserving relative order from Hist), then Unpinned */
    i = 0;

    /* Pass 1: Add Pinned items */
    for (ptr = Hist; ptr; ptr = ptr->next) {
        if (ptr->type == type && ptr->pinned) {
            ViewList[i++] = ptr;
        }
    }

    /* Pass 2: Add Unpinned items */
    for (ptr = Hist; ptr; ptr = ptr->next) {
        if (ptr->type == type && !ptr->pinned) {
            ViewList[i++] = ptr;
        }
    }
}


void ReadHistory(char *Filename)
{
  FILE *HstFile;
  char buffer[BUFSIZ];
  char *cptr;
  int  type, pinned;
  char *content;

  if ( (HstFile = fopen( Filename, "r" ) ) != NULL) {
    while( fgets( buffer, sizeof( buffer ), HstFile ) )
    {
        if (strlen(buffer) > 0)
        {
          /* Strip newline */
          if (buffer[strlen(buffer)-1] == '\n')
             buffer[strlen(buffer)-1] = '\0';

          if (strlen(buffer) == 0) continue;

          /* Try to parse format T:P:Content */
          /* Find first colon */
          cptr = strchr(buffer, ':');
          if (cptr && isdigit((unsigned char)buffer[0])) {
             *cptr = '\0';
             type = atoi(buffer);

             /* Find second colon */
             content = cptr + 1;
             cptr = strchr(content, ':');
             if (cptr && isdigit((unsigned char)content[0])) {
                 *cptr = '\0';
                 pinned = atoi(content);
                 content = cptr + 1;

                 /* Call InsHistory directly with parsed type.
                  * Note: InsHistory puts at head. ReadHistory usually reads older lines later?
                  * No, file is usually written newest-first or oldest-first?
                  * Standard ytree SaveHistory writes newest to oldest in loop 0..N?
                  * No, SaveHistory iterates list (Newest->Oldest) but writes Reversed (Oldest->Newest).
                  * So ReadHistory reads Oldest->Newest.
                  * InsHistory puts at Head. So final list is Newest->Oldest. Correct.
                  */
                  InsHistory(content, type);
                  /* Apply pinned flag to the newly inserted item (which is at Hist) */
                  if (Hist && strcmp(Hist->hst, content) == 0) {
                      Hist->pinned = pinned;
                  }
             } else {
                 /* Malformed or legacy line containing colons */
                 /* Restore first colon */
                 *(--content) = ':';
                 InsHistory(buffer, HST_GENERAL);
             }
          } else {
             /* Legacy format */
             InsHistory(buffer, HST_GENERAL);
          }
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

  /* Note: MAX_HST_FILE_LINES is now larger. We save the first N items of the GLOBAL list.
     This means history for rarely used contexts might drop off if global usage is high.
     For a simple implementation as requested, this is acceptable.
  */

  if((hst = Hist)) {
    if( (HstFile = fopen( Filename, "w" ) ) != NULL) {
      for(j = 0 ; hst && j < MAX_HST_FILE_LINES; j++ ) {
        last_hst = hst;
        hst = hst->next;
      }

      /* write in reverse order (Oldest -> Newest) so ReadHistory restores Newest -> Oldest */
      for(hst=last_hst; hst; hst=hst->prev) {
         fprintf(HstFile, "%d:%d:%s\n", hst->type, hst->pinned, hst->hst);
      }
      fclose( HstFile );
    }
  }
  return;
}


void InsHistory( char *NewHst, int type)
{
   History *TMP, *TMP2 = NULL;
   int flag = 0;

   if (strlen(NewHst)== 0)
     return;

   TMP2 = Hist;
   for ( TMP = Hist; TMP != NULL; TMP = TMP -> next)
   {
       /* Match string AND type */
       if (strcmp(TMP -> hst, NewHst) == 0 && TMP->type == type)
       {
         if (TMP2 != TMP)
         {
            TMP2 -> next = TMP -> next;
            TMP -> next = Hist;
            Hist = TMP;
            if (Hist->next) Hist->next->prev = Hist; /* Fix prev pointer of old head */
            Hist->prev = NULL;
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
     TMP -> type = type;
     TMP -> pinned = 0;

	 if(TMP->hst == NULL)
	 {
	    ERROR_MSG("strdup failed*ABORT");
            exit(1);
	 }

         if (Hist != NULL)
	     Hist -> prev = TMP;
         Hist = TMP;
         /* total_hist updated via BuildViewList during GetHistory, or irrelevant for globals here */
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
  /* Reduce width by 3 to allow for 1 char border + 1 char PIN marker */
  ef_window_width = window_width - 3;

#ifdef NO_HIGHLIGHT
  ef_window_width = window_width - 3;
#endif

  *hide_left = *hide_right = 0;

  /* Use ViewList instead of traversing Hist */
  if (entry_no < 0 || entry_no >= view_count) return;
  pp = ViewList[entry_no];

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
    /* Mark pinned items */
    char marker[4];
    if (pp->pinned) strcpy(marker, "*<");
    else strcpy(marker, " <");

    strcat(line_ptr, (color == CPAIR_HIHST) ? marker : "  ");
    WAddStr( history_window, line_ptr );
#else
#ifdef COLOR_SUPPORT
    /*
     * Align with F2 Visual Style:
     * 1. Use CPAIR_HST as the base color (matches F2).
     * 2. Use A_REVERSE for selection (matches F2).
     * 3. Ignore CPAIR_HIHST to prevent color mismatch.
     */
    int display_attr = COLOR_PAIR(CPAIR_HST);

    /* If passed color indicates selection, use Reverse Video */
    if (color == CPAIR_HIHST) {
        display_attr |= A_REVERSE;
    }

    wattrset(history_window, display_attr);

#else
    if(color == CPAIR_HIHST)
      wattrset( history_window, A_REVERSE );
#endif /* COLOR_SUPPORT */

    /* Draw Pin Marker */
    if (pp->pinned) waddch(history_window, '*');
    else waddch(history_window, ' ');

    /* Draw Text */
    WAddStr( history_window, line_ptr );

#ifdef COLOR_SUPPORT
    /* Reset to standard window background */
    wattrset(history_window, COLOR_PAIR(CPAIR_WINHST));
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



char *GetHistory(int type)
{
  int     ch, tmp;
  int     start_x;
  char    *RetVal = NULL;
  History *TMP;
  int     hide_left, hide_right;

  /* Initialize View */
  BuildViewList(type);

  /* Reset if list empty or smaller */
  if (total_hist == 0) return NULL;

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

      case 'd':
      case 'D':
      case KEY_DC:
                     /* Delete current entry */
                     if (view_count > 0) {
                         History *to_del = ViewList[disp_begin_pos + cursor_pos];

                         /* Unlink from global list */
                         if (to_del->prev) to_del->prev->next = to_del->next;
                         if (to_del->next) to_del->next->prev = to_del->prev;
                         if (Hist == to_del) Hist = to_del->next;

                         if (to_del->hst) free(to_del->hst);
                         free(to_del);

                         /* Rebuild view */
                         BuildViewList(type);

                         /* Adjust cursor */
                         if (disp_begin_pos + cursor_pos >= total_hist) {
                             if (cursor_pos > 0) cursor_pos--;
                             else if (disp_begin_pos > 0) disp_begin_pos--;
                         }
                         if (total_hist == 0) {
                             RetVal = NULL;
                             ch = ESC; /* Exit loop */
                         } else {
                             DisplayHistory();
                         }
                     }
                     break;

      case 'p':
      case 'P':
                     /* Toggle Pin */
                     if (view_count > 0) {
                         History *target = ViewList[disp_begin_pos + cursor_pos];
                         target->pinned = !target->pinned;

                         /* Save current selection string to re-select it after sort?
                            Or just reset cursor to top? Simpler to reset or try to track.
                            Let's keep cursor, but list will re-shuffle. */
                         BuildViewList(type);
                         DisplayHistory();
                     }
                     break;

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
                     if (view_count > 0 && disp_begin_pos + cursor_pos < view_count) {
                        TMP = ViewList[disp_begin_pos + cursor_pos];
                        if (TMP) RetVal = TMP->hst;
                        else RetVal = NULL;
                     } else {
                        RetVal = NULL;
                     }
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