/***************************************************************************
 *
 * match.c
 * Behandlung reg. Ausdruecke fuer Dateinamen
 *
 ***************************************************************************/


#include "ytree.h"
#include <regex.h>
#include <string.h>
#include <stdlib.h>

typedef struct _MatchRule {
    regex_t re;
    BOOL    is_exclude;
    struct _MatchRule *next;
} MatchRule;

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
       Estimate: 2 chars per pattern char (escaping) + anchors + null.
       Safe upper bound. */
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


int SetMatchSpec(char *new_spec)
{
  char *spec_copy;
  char *token, *saveptr;

  FreeMatchRules();

  if (!new_spec || !*new_spec) {
      /* If spec is empty, we match everything (default behavior of Match) */
      return 0;
  }

  spec_copy = strdup(new_spec);
  if (!spec_copy) return -1;

  token = strtok_r(spec_copy, ",", &saveptr);
  while(token) {
      BOOL is_exclude = FALSE;

      /* Trim leading whitespace */
      while(isspace((unsigned char)*token)) token++;

      /* Trim trailing whitespace */
      if (*token) {
          char *end = token + strlen(token) - 1;
          while(end > token && isspace((unsigned char)*end)) end--;
          *(end+1) = 0;
      }

      if (*token == '-') {
          is_exclude = TRUE;
          token++;
      }

      if (*token) { /* Skip empty tokens or standalone '-' */
           if (AddRule(token, is_exclude) != 0) {
               /* Error compiling regex */
               free(spec_copy);
               FreeMatchRules();
               return 1;
           }
      }
      token = strtok_r(NULL, ",", &saveptr);
  }
  free(spec_copy);

  return 0;
}



BOOL Match(char *file_name)
{
  MatchRule *curr;
  BOOL has_includes = FALSE;

  /* If no rules are defined, match everything */
  if (!rules_head) return TRUE;

  /* 1. Check Excludes - If any match, reject immediately */
  for (curr = rules_head; curr; curr = curr->next) {
      if (curr->is_exclude) {
          if (regexec(&curr->re, file_name, 0, NULL, 0) == 0) {
              return FALSE; /* Excluded */
          }
      } else {
          has_includes = TRUE;
      }
  }

  /* 2. Check Includes */
  if (!has_includes) {
      /* No specific includes defined (e.g. only excludes like -*.o),
         so everything not excluded matches. */
      return TRUE;
  }

  /* If there are include patterns, file must match at least one */
  for (curr = rules_head; curr; curr = curr->next) {
      if (!curr->is_exclude) {
          if (regexec(&curr->re, file_name, 0, NULL, 0) == 0) {
              return TRUE; /* Included */
          }
      }
  }

  return FALSE; /* Has includes, but none matched */
}