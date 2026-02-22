/***************************************************************************
 *
 * src/cmd/sort.c
 * Sort command processing
 *
 ***************************************************************************/

#include "ytree_cmd.h"
#include "ytree_defs.h"
#include "ytree_fs.h"

#include <stdlib.h>
#include <string.h>

extern struct Volume *CurrentVolume;

void SetKindOfSort(int kind_of_sort, Statistic *s) {
  if (s) {
    s->kind_of_sort = kind_of_sort;
  }
}
/* UI logic moved to UI_GetKindOfSort in ctrl_file.c */
