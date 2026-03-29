#include "ytree.h"

#include <stdio.h>
#include <stdlib.h>

void *xmalloc(size_t size) {
  void *ptr = malloc(size);
  if (!ptr && size > 0) {
    fprintf(stderr, "xmalloc failed\n");
    exit(1);
  }
  return ptr;
}

int main(int argc, char **argv) {
  const char *dest_path;
  const char *const *source_paths;
  size_t source_count;
  int rc;

  if (argc < 2) {
    fprintf(stderr, "usage: %s <dest_path> [source_path ...]\n", argv[0]);
    return 2;
  }

  dest_path = argv[1];
  source_paths = (const char *const *)(argv + 2);
  source_count = (size_t)((argc > 2) ? (argc - 2) : 0);

  rc = Archive_CreateFromPaths(dest_path, source_paths, NULL, source_count);
  printf("%d\n", rc);
  return 0;
}
