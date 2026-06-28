/***************************************************************************
 *
 * src/cmd/filter.c
 * Filter command processing
 *
 ***************************************************************************/

#include "ytnova_fs.h"
#include <string.h>


/* Core filter functions - all headless now */

int SetFilter(const char *filter_spec, Statistic *s) {
  if (!filter_spec || !*filter_spec || !s)
    return -1;
  if (s->file_spec != filter_spec) {
    snprintf(s->file_spec, sizeof(s->file_spec), "%s", filter_spec);
  }
  return 0;
}

BOOL Match(FileEntry *fe, const Statistic *s) {
  return FsMatchFilter(fe, s);
}

void ApplyFilter(DirEntry *dir_entry, const Statistic *s) {
  FsApplyFilter(dir_entry, s);
}
