#include "ytree_ui.h"

#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  ArchivePayload payload;
  const char *const *sources;
  size_t source_count;
  int rc;
  PathList *orig;
  ArchiveExpandedEntry *expanded;

  payload.original_source_list = NULL;
  payload.expanded_file_list = NULL;

  if (argc < 2) {
    fprintf(stderr, "usage: %s <source_path> [source_path ...]\n", argv[0]);
    return 2;
  }

  sources = (const char *const *)(argv + 1);
  source_count = (size_t)(argc - 1);

  rc = UI_BuildArchivePayloadFromPaths(sources, source_count, TRUE, &payload);
  printf("rc=%d\n", rc);

  if (rc == 0) {
    for (orig = payload.original_source_list; orig; orig = orig->next)
      printf("orig:%s\n", orig->path);

    for (expanded = payload.expanded_file_list; expanded;
         expanded = expanded->next) {
      printf("exp:%s|%s\n", expanded->source_path, expanded->archive_path);
    }
  }

  UI_FreeArchivePayload(&payload);
  return 0;
}
