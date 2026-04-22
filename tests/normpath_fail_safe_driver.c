#include "ytree_fs.h"

#include <stdarg.h>
#include <stdio.h>

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

  normalized[0] = '\0';
  NormPath(argv[1], normalized);
  normalized[PATH_LENGTH] = '\0';

  if (normalized[0] == '\0') {
    puts("NORMPATH_FAIL");
    return 1;
  }

  puts(normalized);
  return 0;
}
