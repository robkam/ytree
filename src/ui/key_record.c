/***************************************************************************
 *
 * src/ui/key_record.c
 * Optional human-readable key trace recorder
 * Redact secrets before emitting trace text; traces are for bug reports.
 *
 ***************************************************************************/

#include <ctype.h>

#include "ytree_cmd.h"
#include "ytree_key_record.h"
#include "ytree_ui.h"

static const char *FormatTraceKey(int ch, char *buf, size_t buf_size) {
  static const struct {
    const char *from;
    const char *to;
  } aliases[] = {
      {"KEY_BACKSPACE", "backspace"}, {"KEY_DOWN", "down"},
      {"KEY_UP", "up"},               {"KEY_LEFT", "left"},
      {"KEY_RIGHT", "right"},         {"KEY_HOME", "home"},
      {"KEY_END", "end"},             {"KEY_PPAGE", "page-up"},
      {"KEY_NPAGE", "page-down"},     {"KEY_IC", "insert"},
      {"KEY_DC", "delete"},           {"KEY_BTAB", "shift-tab"},
      {"KEY_ENTER", "enter"},
  };
  const char *name;
  size_t i;

  if (buf == NULL || buf_size == 0)
    return NULL;

  if (ch == '\n' || ch == '\r'
#ifdef KEY_ENTER
      || ch == KEY_ENTER
#endif
  ) {
    snprintf(buf, buf_size, "enter");
    return buf;
  }

  if (ch == '\t') {
    snprintf(buf, buf_size, "tab");
    return buf;
  }

  if (ch == ' ') {
    snprintf(buf, buf_size, "space");
    return buf;
  }

  if (ch == 8 || ch == 127) {
    snprintf(buf, buf_size, "backspace");
    return buf;
  }

  if (ch == 27) {
    snprintf(buf, buf_size, "esc");
    return buf;
  }

  if (ch >= 32 && ch <= 126 && isprint((unsigned char)ch)) {
    buf[0] = (char)ch;
    buf[1] = '\0';
    return buf;
  }

  name = keyname(ch);
  if (name == NULL) {
    snprintf(buf, buf_size, "unknown");
    return buf;
  }

  for (i = 0; i < sizeof(aliases) / sizeof(aliases[0]); i++) {
    if (strcmp(name, aliases[i].from) == 0) {
      snprintf(buf, buf_size, "%s", aliases[i].to);
      return buf;
    }
  }

#ifdef KEY_F
  if (strncmp(name, "KEY_F(", 6) == 0) {
    int fn = -1;

    if (sscanf(name, "KEY_F(%d)", &fn) == 1 && fn >= 0) {
      snprintf(buf, buf_size, "f%d", fn);
      return buf;
    }
  }
#endif

  if (strncmp(name, "KEY_", 4) == 0)
    name += 4;

  for (i = 0; name[i] != '\0' && i + 1 < buf_size; i++) {
    unsigned char ch_u = (unsigned char)name[i];

    if (name[i] == '_')
      buf[i] = '-';
    else
      buf[i] = (char)tolower(ch_u);
  }
  buf[i] = '\0';
  return buf;
}

static int WriteKeyTraceLine(FILE *fp, int ch) {
  char token[64];
  const char *name;

  name = FormatTraceKey(ch, token, sizeof(token));
  if (name == NULL)
    return -1;

  if (fprintf(fp, "key %s\n", name) < 0)
    return -1;

  return 0;
}

static int BuildDefaultRecordPath(char *path, size_t path_size) {
  int index;

  if (!path || path_size == 0)
    return -1;

  for (index = 1; index <= 999; index++) {
    int written;

    written = snprintf(path, path_size, "ytree-keys-%03d.txt", index);
    if (written < 0 || (size_t)written >= path_size)
      return -1;
    if (access(path, F_OK) != 0)
      return 0;
  }

  return -1;
}

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
  fprintf(fp, "# ytree key trace v2\n");
  ctx->key_record_file = fp;
  ctx->key_record_pause = FALSE;
  return 0;
}

void KeyRecord_Stop(ViewContext *ctx) { CloseRecordingFile(ctx); }

BOOL KeyRecord_IsActive(const ViewContext *ctx) {
  return (ctx != NULL && ctx->key_record_file != NULL) ? TRUE : FALSE;
}

void KeyRecord_Log(ViewContext *ctx, int ch) {
  FILE *fp;

  if (!ctx || ch < 0)
    return;

  if (ctx->key_record_pause)
    return;

  fp = ctx->key_record_file;
  if (!fp)
    return;

  if (WriteKeyTraceLine(fp, ch) != 0 || fflush(fp) != 0) {
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

int KeyRecord_Toggle(ViewContext *ctx) {
  if (!ctx)
    return -1;

  if (KeyRecord_IsActive(ctx)) {
    KeyRecord_Stop(ctx);
    return 0;
  }

  return KeyRecord_BeginPrompt(ctx);
}

int KeyRecord_BeginPrompt(ViewContext *ctx) {
  char path[PATH_LENGTH + 1];

  if (!ctx)
    return -1;

  if (KeyRecord_IsActive(ctx)) {
    (void)UI_Notice(ctx, "Key trace recording is already active.");
    return 0;
  }

  if (BuildDefaultRecordPath(path, sizeof(path)) != 0)
    path[0] = '\0';
  if (UI_ReadStringWithHelp(ctx, ctx->active, "Record key trace to:", path,
                            (int)sizeof(path), HST_PATH,
                            "(Enter) OK  (Esc) cancel", NULL, NULL) != CR)
    return -1;

  if (path[0] == '\0')
    return -1;

  if (KeyRecord_Start(ctx, path) != 0) {
    (void)UI_ShowStatusLineError(ctx, "Failed to open key trace file: %s",
                                 path);
    return -1;
  }

  return 0;
}
