/***************************************************************************
 *
 * filters.c
 * Handling of regular expressions for filenames, attributes, and date filtering
 *
 ***************************************************************************/

#include "ytree.h"
#include <regex.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

typedef struct _MatchRule {
    regex_t re;
    BOOL    is_exclude;
    struct _MatchRule *next;
} MatchRule;

/* Global attribute requirement flags */
static BOOL req_read = FALSE;
static BOOL req_write = FALSE;
static BOOL req_exec = FALSE;
static BOOL req_link = FALSE;
static BOOL req_dir = FALSE;

/* Global date requirement flags */
static BOOL req_date_after = FALSE;
static time_t date_after_val = 0;

static BOOL req_date_before = FALSE;
static time_t date_before_val = 0;

static BOOL req_date_equal = FALSE;
static time_t date_equal_min = 0;
static time_t date_equal_max = 0;

static MatchRule *rules_head = NULL;


static void FreeMatchRules(void) {
    MatchRule *current = rules_head;
    while (current) {
        MatchRule *next = current->next;
        regfree(&current->re);
        free(current);
        current = next;
    }
    rules_head = NULL;
}


static int AddRule(char *pattern, BOOL is_exclude) {
    char *buffer;
    char *b_ptr;
    BOOL meta_flag = FALSE;
    MatchRule *new_rule;
    char *p = pattern;

    /* Allocate buffer for regex string.
       Estimate: 2 chars per pattern char (escaping) + anchors + null. */
    if ((buffer = (char *)malloc(strlen(pattern) * 2 + 4)) == NULL) {
        return -1;
    }

    b_ptr = buffer;
    *b_ptr++ = '^';

    for(; *p; p++) {
        if (meta_flag) {
            *b_ptr++ = *p;
            meta_flag = FALSE;
        } else if (*p == '\\') {
            meta_flag = TRUE;
        } else if (*p == '?') {
            *b_ptr++ = '.';
        } else if (*p == '.') {
            *b_ptr++ = '\\';
            *b_ptr++ = '.';
        } else if (*p == '*') {
            *b_ptr++ = '.';
            *b_ptr++ = '*';
        } else {
            *b_ptr++ = *p;
        }
    }

    *b_ptr++ = '$';
    *b_ptr = '\0';

    if ((new_rule = (MatchRule *)malloc(sizeof(MatchRule))) == NULL) {
        free(buffer);
        return -1;
    }

    if (regcomp(&new_rule->re, buffer, REG_NOSUB | REG_EXTENDED | REG_ICASE)) {
        free(new_rule);
        free(buffer);
        return 1; /* Regex compilation error */
    }

    new_rule->is_exclude = is_exclude;
    new_rule->next = rules_head;
    rules_head = new_rule;

    free(buffer);
    return 0;
}

/*
 * Parse a date string into a range [min, max].
 * Supported formats:
 * YYYY-MM-DD              -> Full day range
 * YYYY-MM-DD HH:MM        -> Minute range (00-59 seconds)
 * YYYY-MM-DD HH:MM:SS     -> Exact second
 *
 * Returns 0 on success, -1 on failure.
 */
static int ParseDateRange(char *str, time_t *min, time_t *max) {
    struct tm tm;
    char *res;

    /* Initialize tm */
    memset(&tm, 0, sizeof(struct tm));
    tm.tm_isdst = -1; /* Let mktime determine DST */

    /* Try Format 1: YYYY-MM-DD HH:MM:SS */
    res = strptime(str, "%Y-%m-%d %H:%M:%S", &tm);
    if (res && *res == '\0') {
        *min = mktime(&tm);
        *max = *min; /* Exact match */
        return (*min == -1) ? -1 : 0;
    }

    /* Try Format 2: YYYY-MM-DD HH:MM */
    memset(&tm, 0, sizeof(struct tm));
    tm.tm_isdst = -1;
    res = strptime(str, "%Y-%m-%d %H:%M", &tm);
    if (res && *res == '\0') {
        tm.tm_sec = 0;
        *min = mktime(&tm);
        tm.tm_sec = 59;
        *max = mktime(&tm);
        return (*min == -1 || *max == -1) ? -1 : 0;
    }

    /* Try Format 3: YYYY-MM-DD */
    memset(&tm, 0, sizeof(struct tm));
    tm.tm_isdst = -1;
    res = strptime(str, "%Y-%m-%d", &tm);
    if (res && *res == '\0') {
        tm.tm_hour = 0; tm.tm_min = 0; tm.tm_sec = 0;
        *min = mktime(&tm);
        tm.tm_hour = 23; tm.tm_min = 59; tm.tm_sec = 59;
        *max = mktime(&tm);
        return (*min == -1 || *max == -1) ? -1 : 0;
    }

    return -1; /* Parse failed */
}

int SetMatchSpec(char *new_spec)
{
  char *spec_copy;
  char *token, *saveptr;

  FreeMatchRules();

  /* Reset global flags */
  req_read = FALSE;
  req_write = FALSE;
  req_exec = FALSE;
  req_link = FALSE;
  req_dir = FALSE;

  req_date_after = FALSE;
  req_date_before = FALSE;
  req_date_equal = FALSE;

  if (!new_spec || !*new_spec) {
      return 0;
  }

  spec_copy = strdup(new_spec);
  if (!spec_copy) return -1;

  token = strtok_r(spec_copy, ",", &saveptr);
  while(token) {
      BOOL is_exclude = FALSE;

      /* Trim whitespace */
      while(isspace((unsigned char)*token)) token++;
      if (*token) {
          char *end = token + strlen(token) - 1;
          while(end > token && isspace((unsigned char)*end)) end--;
          *(end+1) = 0;
      }

      if (*token == ':') {
          /* Attribute spec */
          char *c = token + 1;
          while(*c) {
              switch(*c) {
                  case 'r': req_read = TRUE; break;
                  case 'w': req_write = TRUE; break;
                  case 'x': req_exec = TRUE; break;
                  case 'l': req_link = TRUE; break;
                  case 'd': req_dir = TRUE; break;
              }
              c++;
          }
      } else if (*token == '>') {
          /* Date After */
          time_t t_min, t_max;
          if (ParseDateRange(token + 1, &t_min, &t_max) == 0) {
              req_date_after = TRUE;
              date_after_val = t_max; /* > matches after the END of the specified range/day */
          } else {
              free(spec_copy);
              FreeMatchRules();
              return 1;
          }
      } else if (*token == '<') {
          /* Date Before */
          time_t t_min, t_max;
          if (ParseDateRange(token + 1, &t_min, &t_max) == 0) {
              req_date_before = TRUE;
              date_before_val = t_min; /* < matches before the START of the specified range/day */
          } else {
              free(spec_copy);
              FreeMatchRules();
              return 1;
          }
      } else if (*token == '=') {
          /* Date Equal (Range) */
          time_t t_min, t_max;
          if (ParseDateRange(token + 1, &t_min, &t_max) == 0) {
              req_date_equal = TRUE;
              date_equal_min = t_min;
              date_equal_max = t_max;
          } else {
              free(spec_copy);
              FreeMatchRules();
              return 1;
          }
      } else {
          /* Filename pattern */
          if (*token == '-') {
              is_exclude = TRUE;
              token++;
          }

          if (*token) {
               if (AddRule(token, is_exclude) != 0) {
                   free(spec_copy);
                   FreeMatchRules();
                   return 1;
               }
          }
      }
      token = strtok_r(NULL, ",", &saveptr);
  }
  free(spec_copy);

  return 0;
}



BOOL Match(FileEntry *fe)
{
  MatchRule *curr;
  BOOL has_includes = FALSE;

  if (!fe) return FALSE;

  /* 0. Check Attributes (Global AND logic) */
  if (req_read && !(fe->stat_struct.st_mode & S_IRUSR)) return FALSE;
  if (req_write && !(fe->stat_struct.st_mode & S_IWUSR)) return FALSE;
  if (req_exec && !(fe->stat_struct.st_mode & (S_IXUSR|S_IXGRP|S_IXOTH))) return FALSE;
  if (req_link && !S_ISLNK(fe->stat_struct.st_mode)) return FALSE;
  if (req_dir && !S_ISDIR(fe->stat_struct.st_mode)) return FALSE;

  /* 1. Check Date Constraints */
  if (req_date_after && fe->stat_struct.st_mtime <= date_after_val) return FALSE;
  if (req_date_before && fe->stat_struct.st_mtime >= date_before_val) return FALSE;
  if (req_date_equal && (fe->stat_struct.st_mtime < date_equal_min || fe->stat_struct.st_mtime > date_equal_max)) return FALSE;

  /* If no regex rules are defined, match everything (passed attributes & date) */
  if (!rules_head) return TRUE;

  /* 2. Check Excludes - If any match, reject immediately */
  for (curr = rules_head; curr; curr = curr->next) {
      if (curr->is_exclude) {
          if (regexec(&curr->re, fe->name, 0, NULL, 0) == 0) {
              return FALSE; /* Excluded */
          }
      } else {
          has_includes = TRUE;
      }
  }

  /* 3. Check Includes */
  if (!has_includes) {
      /* No specific includes defined (e.g. only excludes like -*.o),
         so everything not excluded matches. */
      return TRUE;
  }

  /* If there are include patterns, file must match at least one */
  for (curr = rules_head; curr; curr = curr->next) {
      if (!curr->is_exclude) {
          if (regexec(&curr->re, fe->name, 0, NULL, 0) == 0) {
              return TRUE; /* Included */
          }
      }
  }

  return FALSE; /* Has includes, but none matched */
}