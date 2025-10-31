/***************************************************************************
 *
 * Dynamic Colors-Support
 *
 ***************************************************************************/


#include "ytree.h"


#ifdef COLOR_SUPPORT

static BOOL color_enabled = FALSE;


void StartColors()
{
  start_color();
  if ((COLORS < 8) || (COLOR_PAIRS < 17)) {
    ESCAPE; /* no color support */
  }

  init_pair(DIR_COLOR,     COLOR_WHITE,   COLOR_BLUE);
  init_pair(HIDIR_COLOR,   COLOR_BLACK,   COLOR_WHITE);
  init_pair(WINDIR_COLOR,  COLOR_CYAN,    COLOR_BLUE);
  init_pair(FILE_COLOR,    COLOR_WHITE,   COLOR_BLUE);
  init_pair(HIFILE_COLOR,  COLOR_BLACK,   COLOR_WHITE);
  init_pair(WINFILE_COLOR, COLOR_CYAN,    COLOR_BLUE);
  init_pair(STATS_COLOR,   COLOR_BLUE,    COLOR_CYAN);
  init_pair(WINSTATS_COLOR,COLOR_BLUE,    COLOR_CYAN);
  init_pair(BORDERS_COLOR, COLOR_BLUE,    COLOR_CYAN);
  init_pair(HIMENUS_COLOR, COLOR_WHITE,   COLOR_BLUE);
  init_pair(MENU_COLOR,    COLOR_CYAN,    COLOR_BLUE);
  init_pair(WINERR_COLOR,  COLOR_BLUE,    COLOR_WHITE);
  init_pair(HST_COLOR,     COLOR_YELLOW,  COLOR_CYAN);
  init_pair(HIHST_COLOR,   COLOR_WHITE,   COLOR_WHITE);
  init_pair(WINHST_COLOR,  COLOR_YELLOW,  COLOR_CYAN);
  init_pair(HIGLOBAL_COLOR,COLOR_BLUE,    COLOR_WHITE);
  init_pair(GLOBAL_COLOR,  COLOR_YELLOW,  COLOR_CYAN);

  color_enabled = TRUE;
FNC_XIT: ;

}



void WbkgdSet(WINDOW *w, chtype c)
{
  if(color_enabled) {
    wbkgdset(w, c);
  } else {
    c &= ~A_BOLD;
    if(c == COLOR_PAIR(HIDIR_COLOR)   ||
       c == COLOR_PAIR(HIFILE_COLOR)  ||
       c == COLOR_PAIR(HISTATS_COLOR) ||
       c == COLOR_PAIR(HIMENUS_COLOR) ||
       c == COLOR_PAIR(HIHST_COLOR)) {
    
      wattrset(w, A_REVERSE);
    } else {
      wattrset(w, 0);
    }
  }
}


#endif /* COLOR_SUPPORT */ 
