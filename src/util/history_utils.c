/***************************************************************************
 *
 * src/util/history_utils.c
 * Command and path history management
 *
 ***************************************************************************/

#include "ytree_defs.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_HST_FILE_LINES 200

void InsHistory(ViewContext *ctx, const char *NewHst, int type);

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

void ReadHistory(ViewContext *ctx, const char *Filename) {
  FILE *HstFile;
  char buffer[BUFSIZ];
  char *cptr;
  char *content;

  if ((HstFile = fopen(Filename, "r")) != NULL) {
    while (fgets(buffer, sizeof(buffer), HstFile)) {
      if (strlen(buffer) > 0) {
        /* Strip newline */
        if (buffer[strlen(buffer) - 1] == '\n')
          buffer[strlen(buffer) - 1] = '\0';

        if (strlen(buffer) == 0)
          continue;

        /* Try to parse format T:P:Content */
        /* Find first colon */
        cptr = strchr(buffer, ':');
        if (cptr && isdigit((unsigned char)buffer[0])) {
          *cptr = '\0';

          /* Find second colon */
          content = cptr + 1;
          cptr = strchr(content, ':');
          if (cptr && isdigit((unsigned char)content[0])) {
            *cptr = '\0';
            int type = atoi(buffer);
            int pinned = atoi(content);
            content = cptr + 1;

            InsHistory(ctx, content, type);
            /* Apply pinned flag to the newly inserted item (which is at Hist)
             */
            if (ctx->history_head &&
                strcmp(ctx->history_head->hst, content) == 0) {
              ctx->history_head->pinned = pinned;
            }
          } else {
            /* Malformed or legacy line containing colons */
            /* Restore first colon */
            *(--content) = ':';
            InsHistory(ctx, buffer, HST_GENERAL);
          }
        } else {
          /* Legacy format */
          InsHistory(ctx, buffer, HST_GENERAL);
        }
      }
    }
    fclose(HstFile);
  }
  return;
}

void SaveHistory(ViewContext *ctx, const char *Filename) {
  FILE *HstFile;
  int i, count;
  History *hst;
  History **hst_array;

  if (!ctx->history_head)
    return;

  if ((HstFile = fopen(Filename, "w")) == NULL)
    return;

  hst_array = (History **)xmalloc(MAX_HST_FILE_LINES * sizeof(History *));

  /* Collect pointers by traversing forward (Newest -> Oldest) */
  count = 0;
  for (hst = ctx->history_head; hst && count < MAX_HST_FILE_LINES;
       hst = hst->next) {
    hst_array[count++] = hst;
  }

  /* Write backwards (Oldest -> Newest) so ReadHistory restores correct order */
  for (i = count - 1; i >= 0; i--) {
    fprintf(HstFile, "%d:%d:%s\n", hst_array[i]->type, hst_array[i]->pinned,
            hst_array[i]->hst);
  }

  free(hst_array);
  fclose(HstFile);
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
