#include "ytree_cmd.h"
#include "ytree_fs.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void *__real_malloc(size_t size);

static int fail_next_malloc = 0;

void *__wrap_malloc(size_t size) {
  if (fail_next_malloc) {
    fail_next_malloc = 0;
    return NULL;
  }
  return __real_malloc(size);
}

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  va_list args;
  (void)ctx;
  (void)fmt;
  va_start(args, fmt);
  va_end(args);
  return 0;
}

char *GetProfileValue(const ViewContext *ctx, const char *name) {
  (void)ctx;
  (void)name;
  return "cat";
}

int SystemCall(ViewContext *ctx, const char *command_line, Statistic *s) {
  (void)ctx;
  (void)command_line;
  (void)s;
  return 0;
}

int main(int argc, char **argv) {
  ViewContext ctx;
  YtreePanel panel;
  struct Volume vol;
  char dir_entry_storage[sizeof(DirEntry) + 2];
  DirEntry *dir_entry = (DirEntry *)dir_entry_storage;
  int rc;

  if (argc != 2) {
    fprintf(stderr, "usage: %s <path>\n", argv[0]);
    return 2;
  }

  memset(&ctx, 0, sizeof(ctx));
  memset(&panel, 0, sizeof(panel));
  memset(&vol, 0, sizeof(vol));
  memset(dir_entry_storage, 0, sizeof(dir_entry_storage));
  dir_entry->name[0] = '.';
  dir_entry->name[1] = '\0';

  ctx.view_mode = DISK_MODE;
  ctx.active = &panel;
  panel.vol = &vol;

  fail_next_malloc = 1;
  rc = Edit(&ctx, dir_entry, argv[1]);
  printf("EDIT_RETURN=%d\n", rc);

  return (rc == -1) ? 0 : 3;
}
