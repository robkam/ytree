/***************************************************************************
 *
 * src/util/memory_utils.c
 * Centralized safe memory management wrapper functions.
 * Fails fast on allocation errors.
 *
 ***************************************************************************/
#include "ytree_defs.h"

void *xmalloc(size_t size) {
  void *ptr = malloc(size);
  if (!ptr && size > 0) {
    UI_Message(NULL, "Malloc Failed (size: %lu)*ABORT", (unsigned long)size);
    exit(1);
  }
  return ptr;
}

void *xcalloc(size_t nmemb, size_t size) {
  void *ptr = calloc(nmemb, size);
  if (!ptr && nmemb > 0 && size > 0) {
    UI_Message(NULL, "Calloc Failed (count: %lu, size: %lu)*ABORT",
               (unsigned long)nmemb, (unsigned long)size);
    exit(1);
  }
  return ptr;
}

void *xrealloc(void *ptr, size_t size) {
  void *new_ptr = realloc(ptr, size);
  if (!new_ptr && size > 0) {
    UI_Message(NULL, "Realloc Failed (size: %lu)*ABORT", (unsigned long)size);
    exit(1);
  }
  return new_ptr;
}

char *xstrdup(const char *s) {
  char *new_str;
  if (!s)
    return NULL;
  new_str = strdup(s);
  if (!new_str) {
    UI_Message(NULL, "Strdup Failed*ABORT");
    exit(1);
  }
  return new_str;
}
