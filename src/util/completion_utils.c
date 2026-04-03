/***************************************************************************
 *
 * src/util/completion_utils.c
 * Filename completion data preparation utilities
 *
 ***************************************************************************/

#include "ytree_defs.h"
#include <stdlib.h>
#include <string.h>

#ifdef READLINE_SUPPORT
#include <readline/readline.h>
#include <readline/tilde.h>
#endif

static void FreeTabMatches(ViewContext *ctx) {
  if (ctx->tab_mtchs != NULL) {
    int i;
    for (i = 0; ctx->tab_mtchs[i] != NULL; i++) {
      free(ctx->tab_mtchs[i]);
    }
    free(ctx->tab_mtchs);
    ctx->tab_mtchs = NULL;
  }
}

char *PrepareCompletionMatches(ViewContext *ctx, char *base, int *show_dialog) {
#ifdef READLINE_SUPPORT
  char *RetVal = NULL;
  char *TMP;
  char *expanded_base = NULL;
#endif

  if (show_dialog != NULL)
    *show_dialog = 0;

  FreeTabMatches(ctx);

#ifdef READLINE_SUPPORT
  expanded_base = tilde_expand(base);
  if (expanded_base == NULL) {
    return NULL;
  }

  if ((ctx->tab_mtchs =
           rl_completion_matches(expanded_base, rl_filename_completion_function)) ==
      NULL) {
    free(expanded_base);
    return NULL;
  }

  if (ctx->tab_mtchs[0] != NULL && strcmp(expanded_base, ctx->tab_mtchs[0]) == 0) {
    free(expanded_base);
    return NULL;
  }

  if (ctx->tab_mtchs[0] != NULL) {
    size_t match_len = strlen(ctx->tab_mtchs[0]);
    TMP = xmalloc(match_len + 1);
    memcpy(TMP, ctx->tab_mtchs[0], match_len + 1);
    RetVal = TMP;
    free(expanded_base);
    return RetVal;
  }

  for (ctx->tab_total_matches = 0; ctx->tab_mtchs[ctx->tab_total_matches];
       ctx->tab_total_matches++)
    ;

  if (ctx->tab_total_matches == 1) {
    free(expanded_base);
    return NULL;
  }

  ctx->tab_disp_begin_pos = 1;
  ctx->tab_cursor_pos = 0;
  if (show_dialog != NULL)
    *show_dialog = 1;

  free(expanded_base);
  return NULL;
#else
  (void)ctx;
  (void)base;
  return NULL;
#endif
}
