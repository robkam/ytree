/***************************************************************************
 *
 * src/cmd/sort.c
 * Switching the sorting criterion
 *
 ***************************************************************************/


#include "ytree.h"




void GetKindOfSort(void)
{
  int c;
  int s;
  int order;

  order = SORT_ASC; s=0;
  ClearHelp();
  PrintOptions( stdscr, LINES - 2, 1,
            "Sort by (A)ccTime (C)hgTime (E)xtension (G)roup (M)odTime   (O)rder: [ascending]"
	  );
  PrintOptions( stdscr, LINES - 1, 2, "       (N)ame o(W)ner (S)ize" );

  RefreshWindow( stdscr );
  doupdate();
  do
  {
        c = Getch();
	if(c == -1 || c == ESC)
	  return;

        c = toupper(c);

	if (c == 'Q')
	  return;

        switch( c )
        {
                case 'N': s = SORT_BY_NAME;
                        break;
                case 'E': s = SORT_BY_EXTENSION;
                        break;
                case 'M': s = SORT_BY_MOD_TIME;
                        break;
                case 'A': s = SORT_BY_ACC_TIME;
                        break;
                case 'C': s = SORT_BY_CHG_TIME;
                        break;
                case 'G': s = SORT_BY_GROUP;
                        break;
                case 'W': s = SORT_BY_OWNER;
                        break;
                case 'S': s = SORT_BY_SIZE;
                        break;
                case 'O': if (order == SORT_ASC)
                          {
                              PrintOptions( stdscr, LINES - 2, 58, "[descending]" );
                              order = SORT_DSC;
                          }
                          else
                          {
                                PrintOptions( stdscr, LINES - 2, 58, "[ascending] " );
                                order = SORT_ASC;
                          }
                        RefreshWindow( stdscr );
                        doupdate();
                        break;
                default : /* No beep for invalid keys */
                        RefreshWindow( stdscr );
                        doupdate();
                        break;
        }
  } while( ! strchr("ACEGMNWS", c));
  SetKindOfSort(s + order, &CurrentVolume->vol_stats);
}

void SetKindOfSort(int new_kind_of_sort, Statistic *s)
{
  s->kind_of_sort = new_kind_of_sort;
}
