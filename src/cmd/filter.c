/***************************************************************************
 *
 * src/cmd/filter.c
 * Filter command processing
 *
 ***************************************************************************/

#ifndef YTREE_H
#include "../../include/ytree.h"
#endif

#include <ctype.h>
#include <fnmatch.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern char *GetFileName(FileEntry *fe, char *path);

/* Core filter functions - all headless now */

int SetFilter(char *filter_spec, Statistic *s) {
  if (!filter_spec || !*filter_spec || !s)
    return -1;
  strncpy(s->file_spec, filter_spec, sizeof(s->file_spec) - 1);
  s->file_spec[sizeof(s->file_spec) - 1] = '\0';
  return 0;
}

BOOL Match(FileEntry *fe, Statistic *s) {
  if (!fe || !s)
    return FALSE;

  char *pattern = s->file_spec;
  if (!pattern || !*pattern)
    return TRUE; /* Default to match all */

  if (fnmatch(pattern, fe->name, FNM_PATHNAME | FNM_PERIOD) == 0) {
    return TRUE;
  }
  return FALSE;
}

void ApplyFilter(DirEntry *dir_entry, Statistic *s) {
  FileEntry *fe;
  if (!dir_entry || !s)
    return;

  dir_entry->matching_files = 0;
  dir_entry->matching_bytes = 0;

  for (fe = dir_entry->file; fe; fe = fe->next) {
    fe->matching = Match(fe, s);
    if (fe->matching) {
      dir_entry->matching_files++;
      dir_entry->matching_bytes += fe->stat_struct.st_size;
    }
  }
}