/***************************************************************************
 *
 * src/ui/compare_request.c
 * Compare request prompt/build helpers extracted from interactions.c.
 *
 ***************************************************************************/

#include "interactions_panel_paths.h"
#include "ytnova_fs.h"
#include "ytnova_ui.h"
#include <stdio.h>
#include <string.h>

typedef struct {
  const char *context_id;
} CompareGeneratedHelpSpec;

typedef struct {
  ViewContext *ctx;
  DirEntry *source_dir;
  CompareRequest *request;
  BOOL *launch_external;
  char prompt[128];
  char last_auto_target[PATH_LENGTH + 1];
} CompareTargetPromptState;

enum {
  COMPARE_SCOPE_CYCLE_KEY = 3,
  COMPARE_BASIS_CYCLE_KEY = 4,
  COMPARE_TAG_CYCLE_KEY = 5
};

static const UICommandStripCommand compare_target_hint_commands[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "browse", "F2", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "scope", "F3", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "basis", "F4", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "tag", "F5", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "history", "Up", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "OK", "Enter", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "cancel", "Esc", NULL}};
static const CompareGeneratedHelpSpec compare_target_help_spec = {
    "prompt.compare-target"};

static int ShowCompareHelpCallback(ViewContext *ctx, void *help_data) {
  const CompareGeneratedHelpSpec *spec =
      (const CompareGeneratedHelpSpec *)help_data;

  if (ctx == NULL || spec == NULL)
    return 0;

  (void)UI_ShowGeneratedContextHelp(ctx, spec->context_id, NULL, 0);
  return 0;
}

static int PromptCompareTargetPath(ViewContext *ctx, const char *prompt,
                                   const char *default_path, char *target_path,
                                   const CompareGeneratedHelpSpec *help_spec) {
  if (!ctx || !prompt || !target_path)
    return -1;

  if (default_path) {
    if (default_path != target_path) {
      strncpy(target_path, default_path, PATH_LENGTH);
      target_path[PATH_LENGTH] = '\0';
    } else {
      /* Preserve existing in-place target buffer when default aliases it. */
      target_path[PATH_LENGTH] = '\0';
    }
  } else {
    target_path[0] = '\0';
  }

  ClearHelp(ctx);
  if (UI_ReadStringWithHelp(ctx, ctx->active, prompt, target_path, PATH_LENGTH,
                            HST_PATH, compare_target_hint_commands,
                            sizeof(compare_target_hint_commands) /
                                sizeof(compare_target_hint_commands[0]),
                            ShowCompareHelpCallback, (void *)help_spec) != CR) {
    return -1;
  }

  if (target_path[0] == '\0')
    return -1;

  return 0;
}

static int ResolveCompareSourcePath(ViewContext *ctx, DirEntry *source_dir,
                                    CompareFlowType flow_type,
                                    char *source_path) {
  if (!ctx || !source_path)
    return -1;

  if (flow_type == COMPARE_FLOW_DIRECTORY) {
    if (!source_dir)
      return -1;
    GetPath(source_dir, source_path);
  } else if (flow_type == COMPARE_FLOW_LOGGED_TREE) {
    if (!ctx->active || !ctx->active->vol || !ctx->active->vol->vol_stats.tree)
      return -1;
    GetPath(ctx->active->vol->vol_stats.tree, source_path);
  } else {
    return -1;
  }

  source_path[PATH_LENGTH] = '\0';
  return 0;
}

static int ResolveSplitCompareTargetPath(ViewContext *ctx,
                                         CompareFlowType flow_type,
                                         char *target_path) {
  YtreeNovaPanel *inactive = NULL;

  if (!ctx || !ctx->is_split_screen || !target_path)
    return -1;

  inactive = UI_GetInactivePanel(ctx);
  if (!inactive)
    return -1;

  if (flow_type == COMPARE_FLOW_DIRECTORY) {
    return UI_GetPanelSelectedDirPath(ctx, inactive, target_path);
  }
  if (flow_type == COMPARE_FLOW_LOGGED_TREE) {
    return UI_GetPanelLoggedRootPath(inactive, target_path);
  }

  return -1;
}

static CompareBasis NextCompareBasis(CompareBasis basis) {
  switch (basis) {
  case COMPARE_BASIS_SIZE_AND_DATE:
    return COMPARE_BASIS_SIZE;
  case COMPARE_BASIS_SIZE:
    return COMPARE_BASIS_DATE;
  case COMPARE_BASIS_DATE:
    return COMPARE_BASIS_HASH;
  case COMPARE_BASIS_HASH:
  default:
    return COMPARE_BASIS_SIZE_AND_DATE;
  }
}

static CompareTagResult NextCompareTagResult(CompareTagResult tag_result) {
  switch (tag_result) {
  case COMPARE_TAG_DIFFERENT:
    return COMPARE_TAG_MATCH;
  case COMPARE_TAG_MATCH:
    return COMPARE_TAG_NEWER;
  case COMPARE_TAG_NEWER:
    return COMPARE_TAG_OLDER;
  case COMPARE_TAG_OLDER:
    return COMPARE_TAG_UNIQUE;
  case COMPARE_TAG_UNIQUE:
    return COMPARE_TAG_TYPE_MISMATCH;
  case COMPARE_TAG_TYPE_MISMATCH:
    return COMPARE_TAG_ERROR;
  case COMPARE_TAG_ERROR:
  default:
    return COMPARE_TAG_DIFFERENT;
  }
}

static void UpdateCompareTargetPromptLabel(const CompareTargetPromptState *state) {
  const char *scope_name;

  if (!state || !state->request || !state->launch_external)
    return;

  scope_name = (*state->launch_external)
                   ? (state->request->flow_type == COMPARE_FLOW_LOGGED_TREE
                          ? "external tree"
                          : "external directory")
                   : (state->request->flow_type == COMPARE_FLOW_LOGGED_TREE
                          ? "logged tree"
                          : "directory");

  if (*state->launch_external) {
    (void)snprintf(state->prompt, sizeof(state->prompt),
                   "COMPARE TARGET [%s | saved %s | saved %s]:", scope_name,
                   UI_CompareBasisName(state->request->basis),
                   UI_CompareTagResultName(state->request->tag_result));
  } else {
    (void)snprintf(state->prompt, sizeof(state->prompt),
                   "COMPARE TARGET [%s | %s | %s]:", scope_name,
                   UI_CompareBasisName(state->request->basis),
                   UI_CompareTagResultName(state->request->tag_result));
  }
}

static void SyncCompareTargetPromptState(CompareTargetPromptState *state,
                                         char *target_path, int *cursor_pos) {
  char next_auto_target[PATH_LENGTH + 1];
  BOOL used_split_default = FALSE;

  if (!state || !state->request || !target_path)
    return;

  if (ResolveCompareSourcePath(state->ctx, state->source_dir,
                               state->request->flow_type,
                               state->request->source_path) != 0) {
    return;
  }

  if (ResolveSplitCompareTargetPath(state->ctx, state->request->flow_type,
                                    next_auto_target) == 0) {
    used_split_default = TRUE;
  } else {
    strncpy(next_auto_target, state->request->source_path, PATH_LENGTH);
    next_auto_target[PATH_LENGTH] = '\0';
  }

  if ((state->last_auto_target[0] != '\0' &&
       strcmp(target_path, state->last_auto_target) == 0) ||
      target_path[0] == '\0') {
    strncpy(target_path, next_auto_target, PATH_LENGTH);
    target_path[PATH_LENGTH] = '\0';
    if (cursor_pos != NULL)
      *cursor_pos = StrVisualLength(target_path);
  }

  strncpy(state->last_auto_target, next_auto_target, PATH_LENGTH);
  state->last_auto_target[PATH_LENGTH] = '\0';
  state->request->used_split_default_target = used_split_default;
  UpdateCompareTargetPromptLabel(state);
}

static void AdvanceComparePromptScope(CompareTargetPromptState *state,
                                      char *target_path, int *cursor_pos) {
  if (!state || !state->request || !state->launch_external)
    return;

  if (!*state->launch_external &&
      state->request->flow_type == COMPARE_FLOW_DIRECTORY) {
    state->request->flow_type = COMPARE_FLOW_LOGGED_TREE;
  } else if (!*state->launch_external &&
             state->request->flow_type == COMPARE_FLOW_LOGGED_TREE) {
    state->request->flow_type = COMPARE_FLOW_DIRECTORY;
    *state->launch_external = TRUE;
  } else if (*state->launch_external &&
             state->request->flow_type == COMPARE_FLOW_DIRECTORY) {
    state->request->flow_type = COMPARE_FLOW_LOGGED_TREE;
  } else {
    state->request->flow_type = COMPARE_FLOW_DIRECTORY;
    *state->launch_external = FALSE;
  }

  SyncCompareTargetPromptState(state, target_path, cursor_pos);
}

static BOOL HandleCompareTargetAction(ViewContext *ctx, YtreeNovaPanel *panel,
                                      int ch, const char *buffer, int max_len,
                                      int *cursor_pos, void *action_data) {
  const CompareTargetPromptState *state =
      (CompareTargetPromptState *)action_data;

  (void)ctx;
  (void)panel;
  (void)max_len;

  if (!state || !state->request || !buffer)
    return FALSE;

  switch (ch) {
#ifdef KEY_F
  case KEY_F(COMPARE_SCOPE_CYCLE_KEY):
    AdvanceComparePromptScope((CompareTargetPromptState *)state,
                              state->request->target_path, cursor_pos);
    return TRUE;

  case KEY_F(COMPARE_BASIS_CYCLE_KEY):
    state->request->basis = NextCompareBasis(state->request->basis);
    UpdateCompareTargetPromptLabel(state);
    return TRUE;

  case KEY_F(COMPARE_TAG_CYCLE_KEY):
    state->request->tag_result =
        NextCompareTagResult(state->request->tag_result);
    UpdateCompareTargetPromptLabel(state);
    return TRUE;
#endif
  default:
    return FALSE;
  }
}

static int PromptDirectoryCompareTarget(ViewContext *ctx, DirEntry *source_dir,
                                        CompareRequest *request,
                                        BOOL *launch_external) {
  CompareTargetPromptState state;
  UIPromptOptions options;

  if (!ctx || !source_dir || !request || !launch_external)
    return -1;

  memset(&state, 0, sizeof(state));
  state.ctx = ctx;
  state.source_dir = source_dir;
  state.request = request;
  state.launch_external = launch_external;

  request->flow_type = COMPARE_FLOW_DIRECTORY;
  request->basis = COMPARE_BASIS_SIZE_AND_DATE;
  request->tag_result = COMPARE_TAG_DIFFERENT;
  request->used_split_default_target = FALSE;
  request->target_path[0] = '\0';
  *launch_external = FALSE;

  SyncCompareTargetPromptState(&state, request->target_path, NULL);
  memset(&options, 0, sizeof(options));
  options.hints_override = compare_target_hint_commands;
  options.hints_override_count =
      sizeof(compare_target_hint_commands) /
      sizeof(compare_target_hint_commands[0]);
  options.help_callback = ShowCompareHelpCallback;
  options.help_data = (void *)&compare_target_help_spec;
  options.action_handler = HandleCompareTargetAction;
  options.action_data = &state;

  ClearHelp(ctx);
  if (UI_ReadStringWithPromptOptions(ctx, ctx->active, state.prompt,
                                     request->target_path, PATH_LENGTH,
                                     HST_PATH, &options) != CR) {
    return -1;
  }

  if (request->target_path[0] == '\0')
    return -1;

  request->target_path[PATH_LENGTH] = '\0';
  return 0;
}

const char *UI_CompareFlowTypeName(CompareFlowType flow_type) {
  switch (flow_type) {
  case COMPARE_FLOW_FILE:
    return "file";
  case COMPARE_FLOW_DIRECTORY:
    return "directory";
  case COMPARE_FLOW_LOGGED_TREE:
    return "tree";
  default:
    return "unknown";
  }
}

const char *UI_CompareBasisName(CompareBasis basis) {
  switch (basis) {
  case COMPARE_BASIS_SIZE:
    return "size";
  case COMPARE_BASIS_DATE:
    return "date";
  case COMPARE_BASIS_SIZE_AND_DATE:
    return "size+date";
  case COMPARE_BASIS_HASH:
    return "hash";
  default:
    return "none";
  }
}

const char *UI_CompareTagResultName(CompareTagResult tag_result) {
  switch (tag_result) {
  case COMPARE_TAG_DIFFERENT:
    return "different";
  case COMPARE_TAG_MATCH:
    return "match";
  case COMPARE_TAG_NEWER:
    return "newer";
  case COMPARE_TAG_OLDER:
    return "older";
  case COMPARE_TAG_UNIQUE:
    return "unique";
  case COMPARE_TAG_TYPE_MISMATCH:
    return "type-mismatch";
  case COMPARE_TAG_ERROR:
    return "error";
  default:
    return "none";
  }
}

const char *UI_GetCompareHelperCommand(const ViewContext *ctx,
                                       CompareFlowType flow_type) {
  const char *helper;

  if (!ctx)
    return "";

  switch (flow_type) {
  case COMPARE_FLOW_FILE:
    return GetProfileValue(ctx, "FILEDIFF");
  case COMPARE_FLOW_DIRECTORY:
    return GetProfileValue(ctx, "DIRDIFF");
  case COMPARE_FLOW_LOGGED_TREE:
    helper = GetProfileValue(ctx, "TREEDIFF");
    if (!helper || !*helper)
      helper = GetProfileValue(ctx, "DIRDIFF");
    return helper ? helper : "";
  default:
    return "";
  }
}

int UI_BuildFileCompareRequest(ViewContext *ctx, FileEntry *source_file,
                               CompareRequest *request) {
  YtreeNovaPanel *inactive = NULL;
  const char *default_target = NULL;

  if (!ctx || !source_file || !request)
    return -1;

  memset(request, 0, sizeof(*request));
  request->flow_type = COMPARE_FLOW_FILE;
  request->basis = COMPARE_BASIS_NONE;
  request->tag_result = COMPARE_TAG_NONE;

  GetFileNamePath(source_file, request->source_path);
  request->source_path[PATH_LENGTH] = '\0';

  if (ctx->is_split_screen) {
    inactive = UI_GetInactivePanel(ctx);
    if (inactive &&
        UI_GetPanelSelectedFilePath(ctx, inactive, request->target_path) == 0) {
      default_target = request->target_path;
      request->used_split_default_target = TRUE;
    }
  }

  if (PromptCompareTargetPath(
          ctx, "COMPARE TARGET:",
          default_target ? default_target : request->source_path,
          request->target_path, &compare_target_help_spec) != 0) {
    return -1;
  }

  return 0;
}

int UI_BuildDirectoryCompareRequest(ViewContext *ctx, DirEntry *source_dir,
                                    CompareRequest *request,
                                    BOOL *launch_external) {
  if (!ctx || !source_dir || !request || !launch_external)
    return -1;

  memset(request, 0, sizeof(*request));
  return PromptDirectoryCompareTarget(ctx, source_dir, request,
                                      launch_external);
}
