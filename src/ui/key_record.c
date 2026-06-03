/***************************************************************************
 *
 * src/ui/key_record.c
 * Optional raw key stream recorder
 *
 ***************************************************************************/

#include "ytree_cmd.h"
#include "ytree_key_record.h"
#include "ytree_ui.h"

static void CloseRecordingFile(ViewContext *ctx) {
  FILE *fp;

  if (!ctx)
    return;

  fp = ctx->key_record_file;
  ctx->key_record_file = NULL;
  if (!fp)
    return;

  if (fclose(fp) != 0) {
    fprintf(stderr, "WARNING: failed to close key recording: %s\n",
            strerror(errno));
  }
}

int KeyRecord_Start(ViewContext *ctx, const char *path) {
  int fd;
  FILE *fp;

  if (!ctx || !path || !*path)
    return -1;

  CloseRecordingFile(ctx);

  fd = open(path, O_WRONLY | O_CREAT | O_TRUNC
#ifdef O_CLOEXEC
            | O_CLOEXEC
#endif
#ifdef O_NOFOLLOW
            | O_NOFOLLOW
#endif
            ,
            S_IRUSR | S_IWUSR);
  if (fd == -1)
    return -1;

  fp = fdopen(fd, "w");
  if (!fp) {
    close(fd);
    return -1;
  }

  setvbuf(fp, NULL, _IOLBF, 0);
  fprintf(fp, "# ytree key trace v1\n");
  ctx->key_record_file = fp;
  ctx->key_record_pause = FALSE;
  return 0;
}

void KeyRecord_Stop(ViewContext *ctx) { CloseRecordingFile(ctx); }

void KeyRecord_Log(ViewContext *ctx, int ch) {
  FILE *fp;
  const char *name;

  if (!ctx || ch < 0)
    return;

  fp = ctx->key_record_file;
  if (!fp)
    return;

  name = keyname(ch);
  if (name == NULL)
    name = "<unknown>";

  if (fprintf(fp, "%d %s\n", ch, name) < 0 || fflush(fp) != 0) {
    int saved_errno = errno;

    CloseRecordingFile(ctx);
    fprintf(stderr, "WARNING: failed to write key recording: %s\n",
            strerror(saved_errno));
  }
}

void KeyRecord_Pause(ViewContext *ctx, BOOL pause) {
  if (!ctx)
    return;
  ctx->key_record_pause = pause;
}

int KeyRecord_BeginPrompt(ViewContext *ctx) {
  char path[PATH_LENGTH + 1];

  if (!ctx)
    return -1;

  if (ctx->key_record_file != NULL) {
    (void)UI_Notice(ctx, "Key trace recording is already active.");
    return 0;
  }

  path[0] = '\0';
  if (UI_ReadString(ctx, ctx->active, "Record key trace to:", path,
                    (int)sizeof(path), HST_PATH) != CR)
    return -1;

  if (path[0] == '\0')
    return -1;

  if (KeyRecord_Start(ctx, path) != 0) {
    (void)UI_Notice(ctx, "Failed to open key trace file: %s", path);
    return -1;
  }

  (void)UI_Notice(ctx, "Recording key trace to %s", path);
  return 0;
}
