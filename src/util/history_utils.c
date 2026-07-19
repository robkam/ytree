/***************************************************************************
 *
 * src/util/history_utils.c
 * Command and path history management
 *
 ***************************************************************************/

#include "ytnova_defs.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_HST_FILE_LINES 200
#define HISTORY_ENTRY_INITIAL_CAPACITY 16

void InsHistory(ViewContext *ctx, const char *NewHst, int type);
static int EnsureParentDirectoryExists(const char *filename);
static int HistoryValidationError(ViewContext *ctx, const char *filename,
                                  int line_number, const char *detail);
static void FreeHistoryDiskEntries(char **entries, int count);
static BOOL ParseHistoryLongValue(const char *text, long *parsed_value);
static int ParseHistoryLine(ViewContext *ctx, const char *filename,
                            int line_number, char *line, int *type,
                            int *pinned, char **content);
static int WriteHistoryFile(FILE *fp, void *user_data);

int ResolvePreferredHistoryPath(char *path, size_t path_size) {
  const char *xdg_state_home;
  int written;

  if (path == NULL || path_size == 0)
    return -1;

  path[0] = '\0';
  xdg_state_home = getenv(HISTORY_STATE_HOME_ENV);
  if (xdg_state_home != NULL && *xdg_state_home != '\0') {
    written = snprintf(path, path_size, "%s/%s", xdg_state_home,
                       HISTORY_STATE_HOME_PATH);
    if (written >= 0 && (size_t)written < path_size)
      return 0;
    path[0] = '\0';
    return -1;
  }

  xdg_state_home = getenv("HOME");
  if (xdg_state_home == NULL || *xdg_state_home == '\0')
    return -1;

  written = snprintf(path, path_size, "%s/%s", xdg_state_home,
                     HISTORY_STATE_HOME_FALLBACK);
  if (written < 0 || (size_t)written >= path_size) {
    path[0] = '\0';
    return -1;
  }
  return 0;
}

int ResolveLegacyHistoryPath(char *path, size_t path_size) {
  const char *home;
  int written;

  if (path == NULL || path_size == 0)
    return -1;

  path[0] = '\0';
  home = getenv("HOME");
  if (home == NULL || *home == '\0')
    return -1;

  written = snprintf(path, path_size, "%s/%s", home, HISTORY_LEGACY_FILENAME);
  if (written < 0 || (size_t)written >= path_size) {
    path[0] = '\0';
    return -1;
  }
  return 0;
}

static int EnsureParentDirectoryExists(const char *filename) {
  char path[PATH_LENGTH + 1];
  char *cursor;

  if (filename == NULL || *filename == '\0')
    return -1;
  if (snprintf(path, sizeof(path), "%s", filename) < 0 ||
      strlen(filename) >= sizeof(path))
    return -1;

  cursor = strrchr(path, FILE_SEPARATOR_CHAR);
  if (cursor == NULL)
    return 0;
  *cursor = '\0';

  for (cursor = path + 1; *cursor != '\0'; ++cursor) {
    struct stat st;

    if (*cursor != FILE_SEPARATOR_CHAR)
      continue;
    *cursor = '\0';
    if (mkdir(path, S_IRWXU) != 0 &&
        (errno != EEXIST || stat(path, &st) != 0 || !S_ISDIR(st.st_mode))) {
      *cursor = FILE_SEPARATOR_CHAR;
      return -1;
    }
    *cursor = FILE_SEPARATOR_CHAR;
  }

  {
    struct stat st;

    if (mkdir(path, S_IRWXU) != 0 &&
        (errno != EEXIST || stat(path, &st) != 0 || !S_ISDIR(st.st_mode)))
      return -1;
  }

  return 0;
}

static void FreeViewList(ViewContext *ctx) {
  if (ctx->history_view_list) {
    free(ctx->history_view_list);
    ctx->history_view_list = NULL;
  }
  ctx->history_view_count = 0;
  ctx->total_hist = 0;
}

void BuildHistoryViewList(ViewContext *ctx, int type) {
  History *ptr;
  int i;

  FreeViewList(ctx);

  /* First, count matching items */
  for (ptr = ctx->history_head; ptr; ptr = ptr->next) {
    if (ptr->type == type) {
      ctx->history_view_count++;
    }
  }
  ctx->total_hist = ctx->history_view_count;

  if (ctx->history_view_count == 0)
    return;

  ctx->history_view_list =
      (History **)xmalloc(ctx->history_view_count * sizeof(History *));

  /* Populate ViewList: Pinned first (preserving relative order from Hist), then
   * Unpinned */
  i = 0;

  /* Pass 1: Add Pinned items */
  for (ptr = ctx->history_head; ptr; ptr = ptr->next) {
    if (ptr->type == type && ptr->pinned) {
      ctx->history_view_list[i++] = ptr;
    }
  }

  /* Pass 2: Add Unpinned items */
  for (ptr = ctx->history_head; ptr; ptr = ptr->next) {
    if (ptr->type == type && !ptr->pinned) {
      ctx->history_view_list[i++] = ptr;
    }
  }
}

static int HistoryValidationError(ViewContext *ctx, const char *filename,
                                  int line_number, const char *detail) {
  char message[256];

  if (detail == NULL)
    detail = "invalid history entry";
  if (filename == NULL)
    filename = "(unknown)";

  (void)snprintf(message, sizeof(message), "Invalid history \"%s\": line %d: %s",
                 filename, line_number, detail);
  UI_Message(ctx, "%s", message);
  return 1;
}

static void FreeHistoryDiskEntries(char **entries, int count) {
  int i;

  if (entries == NULL)
    return;
  for (i = 0; i < count; ++i)
    free(entries[i]);
  free(entries);
}

static BOOL ParseHistoryLongValue(const char *text, long *parsed_value) {
  char *end_ptr;
  long parsed;

  if (text == NULL || *text == '\0' || parsed_value == NULL)
    return FALSE;

  errno = 0;
  parsed = strtol(text, &end_ptr, 10);
  if (errno != 0 || end_ptr == text || *end_ptr != '\0')
    return FALSE;

  *parsed_value = parsed;
  return TRUE;
}

static int ParseHistoryLine(ViewContext *ctx, const char *filename,
                            int line_number, char *line, int *type,
                            int *pinned, char **content) {
  char *first_colon;
  char *second_colon;
  long parsed_type;
  long parsed_pinned;

  if (line == NULL || type == NULL || pinned == NULL || content == NULL)
    return -1;

  first_colon = strchr(line, ':');
  if (first_colon == NULL || !isdigit((unsigned char)line[0])) {
    *type = HST_GENERAL;
    *pinned = 0;
    *content = line;
    return 0;
  }

  second_colon = strchr(first_colon + 1, ':');
  if (second_colon == NULL) {
    *type = HST_GENERAL;
    *pinned = 0;
    *content = line;
    return 0;
  }

  *first_colon = '\0';
  *second_colon = '\0';
  if (!ParseHistoryLongValue(line, &parsed_type) || parsed_type < HST_GENERAL ||
      parsed_type > HST_PRINT_FRAME) {
    return HistoryValidationError(ctx, filename, line_number,
                                  "history type is out of range");
  }
  if (!ParseHistoryLongValue(first_colon + 1, &parsed_pinned) ||
      (parsed_pinned != 0 && parsed_pinned != 1)) {
    return HistoryValidationError(ctx, filename, line_number,
                                  "pinned flag must be 0 or 1");
  }
  if (second_colon[1] == '\0') {
    return HistoryValidationError(ctx, filename, line_number,
                                  "history entry content must not be empty");
  }

  *type = (int)parsed_type;
  *pinned = (int)parsed_pinned;
  *content = second_colon + 1;
  return 0;
}

int ReadHistory(ViewContext *ctx, const char *Filename) {
  FILE *HstFile;
  char buffer[BUFSIZ];
  char **entries = NULL;
  int *types = NULL;
  int *pinned_values = NULL;
  int count = 0;
  int capacity = 0;
  int line_number = 0;
  int result = -1;

  if (ctx == NULL || Filename == NULL)
    return -1;

  HstFile = fopen(Filename, "r");
  if (HstFile == NULL)
    return -1;

  while (fgets(buffer, sizeof(buffer), HstFile) != NULL) {
    char *content;
    int type;
    int pinned;
    size_t length;

    ++line_number;
    if (strchr(buffer, '\n') == NULL && !feof(HstFile)) {
      result = HistoryValidationError(ctx, Filename, line_number,
                                      "line is too long");
      goto cleanup;
    }

    length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n')
      buffer[length - 1] = '\0';
    if (buffer[0] == '\0')
      continue;

    result = ParseHistoryLine(ctx, Filename, line_number, buffer, &type,
                              &pinned, &content);
    if (result != 0)
      goto cleanup;

    if (count == capacity) {
      int new_capacity = capacity == 0 ? HISTORY_ENTRY_INITIAL_CAPACITY
                                       : capacity * 2;

      entries = (char **)xrealloc(entries, (size_t)new_capacity * sizeof(*entries));
      types = (int *)xrealloc(types, (size_t)new_capacity * sizeof(*types));
      pinned_values =
          (int *)xrealloc(pinned_values, (size_t)new_capacity * sizeof(*pinned_values));
      capacity = new_capacity;
    }

    entries[count] = xstrdup(content);
    types[count] = type;
    pinned_values[count] = pinned;
    ++count;
  }

  if (ferror(HstFile)) {
    result = -1;
    goto cleanup;
  }

  {
    int i;

    for (i = 0; i < count; ++i) {
      InsHistory(ctx, entries[i], types[i]);
      if (ctx->history_head != NULL &&
          strcmp(ctx->history_head->hst, entries[i]) == 0) {
        ctx->history_head->pinned = pinned_values[i];
      }
    }
  }

  result = 0;

cleanup:
  if (HstFile != NULL)
    fclose(HstFile);
  FreeHistoryDiskEntries(entries, count);
  free(types);
  free(pinned_values);
  return result;
}

static int WriteHistoryFile(FILE *fp, void *user_data) {
  ViewContext *ctx = (ViewContext *)user_data;
  int i, count;
  History *hst;
  History **hst_array;

  if (fp == NULL || ctx == NULL)
    return -1;

  if (!ctx->history_head)
    return 0;

  hst_array = (History **)xmalloc(MAX_HST_FILE_LINES * sizeof(History *));

  /* Collect pointers by traversing forward (Newest -> Oldest) */
  count = 0;
  for (hst = ctx->history_head; hst && count < MAX_HST_FILE_LINES;
       hst = hst->next) {
    hst_array[count++] = hst;
  }

  /* Write backwards (Oldest -> Newest) so ReadHistory restores correct order */
  for (i = count - 1; i >= 0; i--) {
    if (fprintf(fp, "%d:%d:%s\n", hst_array[i]->type, hst_array[i]->pinned,
                hst_array[i]->hst) < 0) {
      free(hst_array);
      return -1;
    }
  }

  free(hst_array);
  return 0;
}

int SaveHistory(ViewContext *ctx, const char *Filename) {
  if (ctx == NULL || Filename == NULL || *Filename == '\0')
    return -1;
  if (EnsureParentDirectoryExists(Filename) != 0) {
    UI_Message(ctx, "Can't save history \"%s\"*%s", Filename, strerror(errno));
    return -1;
  }
  if (AtomicFileWrite(Filename, WriteHistoryFile, ctx) != 0) {
    UI_Message(ctx, "Can't save history \"%s\"*%s", Filename, strerror(errno));
    return -1;
  }
  return 0;
}

void InsHistory(ViewContext *ctx, const char *NewHst, int type) {
  History *TMP, *TMP2 = NULL;
  int flag = 0;

  if (strlen(NewHst) == 0)
    return;

  TMP2 = ctx->history_head;
  for (TMP = ctx->history_head; TMP != NULL; TMP = TMP->next) {
    /* Match string AND type */
    if (strcmp(TMP->hst, NewHst) == 0 && TMP->type == type) {
      if (TMP2 != TMP) {
        TMP2->next = TMP->next;
        if (TMP->next)
          TMP->next->prev = TMP2; /* Fix broken double link */
        TMP->next = ctx->history_head;
        ctx->history_head = TMP;
        if (ctx->history_head->next)
          ctx->history_head->next->prev =
              ctx->history_head; /* Fix prev pointer of old head */
        ctx->history_head->prev = NULL;
      }
      flag = 1;
      break;
    };
    TMP2 = TMP;
  }

  if (flag == 0) {
    TMP = (History *)xmalloc(sizeof(struct _history));
    TMP->next = ctx->history_head;
    TMP->prev = NULL;
    TMP->hst = xstrdup(NewHst);
    TMP->type = type;
    TMP->pinned = 0;

    if (ctx->history_head != NULL)
      ctx->history_head->prev = TMP;
    ctx->history_head = TMP;
  }
  return;
}
