/***************************************************************************
 *
 * src/cmd/filter.c
 * Filter command processing
 *
 ***************************************************************************/

#ifndef YTREE_FS_H
#include "ytree.h"
#include "../../include/ytree_fs.h"
#endif
#include <ctype.h>
#include <fnmatch.h>
#include <string.h>


/* Core filter functions - all headless now */

int SetFilter(char *filter_spec, Statistic *s) {
  if (!filter_spec || !*filter_spec || !s)
    return -1;
  if (s->file_spec != filter_spec) {
    strncpy(s->file_spec, filter_spec, sizeof(s->file_spec) - 1);
    s->file_spec[sizeof(s->file_spec) - 1] = '\0';
  }
  return 0;
}

BOOL Match(FileEntry *fe, Statistic *s) {
  char temp_spec[sizeof(s->file_spec)];
  char *pattern;
  char *saveptr;
  BOOL has_positive = FALSE;
  BOOL positive_match = FALSE;

  if (!fe || !s)
    return FALSE;

  if (!s->file_spec[0])
    return TRUE; /* Default to match all */

  /* Work on a copy because strtok modifies the string */
  strncpy(temp_spec, s->file_spec, sizeof(temp_spec) - 1);
  temp_spec[sizeof(temp_spec) - 1] = '\0';

  /* First pass: check for negative patterns (exclusions) */
  pattern = strtok_r(temp_spec, ",", &saveptr);
  while (pattern) {
    /* Trim leading whitespace */
    while (*pattern && isspace((unsigned char)*pattern))
      pattern++;

    if (*pattern && *pattern == '-') {
      /* Negative pattern: if it matches, exclude the file */
      char *actual_pattern = pattern + 1;
      if (fnmatch(actual_pattern, fe->name, 0) == 0) {
        return FALSE; /* File matches negative pattern, exclude it */
      }
    } else if (*pattern) {
      has_positive = TRUE;
    }
    pattern = strtok_r(NULL, ",", &saveptr);
  }

  /* Second pass: check for positive patterns (inclusions) */
  if (has_positive) {
    strncpy(temp_spec, s->file_spec, sizeof(temp_spec) - 1);
    temp_spec[sizeof(temp_spec) - 1] = '\0';
    pattern = strtok_r(temp_spec, ",", &saveptr);
    while (pattern) {
      /* Trim leading whitespace */
      while (*pattern && isspace((unsigned char)*pattern))
        pattern++;

      if (*pattern && *pattern != '-') {
        /* Trim trailing whitespace */
        char *end = pattern + strlen(pattern) - 1;
        while (end > pattern && isspace((unsigned char)*end)) {
          *end = '\0';
          end--;
        }

        if (fnmatch(pattern, fe->name, 0) == 0) {
          return TRUE; /* File matches positive pattern, include it */
        }
      }
      pattern = strtok_r(NULL, ",", &saveptr);
    }
    return FALSE; /* No positive pattern matched */
  } else {
    /* No positive patterns, only negative - include by default if not excluded */
    return TRUE;
  }
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