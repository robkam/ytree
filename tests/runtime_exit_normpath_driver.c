#include "ytree_fs.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

char *__real_strdup(const char *s);

static int fail_next_strdup = 0;

char *__wrap_strdup(const char *s) {
  if (fail_next_strdup) {
    fail_next_strdup = 0;
    return NULL;
  }
  return __real_strdup(s);
}

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  va_list args;
  (void)ctx;
  (void)fmt;
  va_start(args, fmt);
  va_end(args);
  return 0;
}

int main(int argc, char **argv) {
  char normalized[PATH_LENGTH + 1];

  if (argc != 2) {
    fprintf(stderr, "usage: %s <path>\n", argv[0]);
    return 2;
  }

  memset(normalized, 0, sizeof(normalized));
  fail_next_strdup = 1;
  NormPath(argv[1], normalized);

  if (normalized[0] == '\0') {
    puts("NORMPATH_EMPTY");
    return 0;
  }

  printf("%s\n", normalized);
  return 3;
}
