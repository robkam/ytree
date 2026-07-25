/***************************************************************************
 *
 * src/ui/compare_request.c
 * Compare request prompt/build helpers extracted from interactions.c.
 *
 ***************************************************************************/

#include "interactions_panel_paths.h"
#include "ytnova_fs.h"
#include "ytnova_ui.h"
#include <ctype.h>
#include <string.h>

typedef struct {
  const char *context_id;
  const char *prefix;
  const UICommandStripCommand *commands;
  size_t command_count;
} CompareGeneratedHelpSpec;

static const UICommandStripCommand compare_status_commands[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "context help", "F1", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "cancel", "Esc", NULL}};
static const UICommandStripCommand compare_basis_commands[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "Size", "S", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "Date", "D", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "Size+date", "Z", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "Hash", "H", NULL}};
static const UICommandStripCommand compare_tag_result_commands[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "Different", "F", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "Match", "M", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "Newer", "N", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "Older", "O", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "Unique", "U", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "Type-mismatch", "T", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "Error", "E", NULL}};
static const UICommandStripCommand compare_scope_commands[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "Directory only", "D", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "Logged tree", "T", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "External viewer", "X", NULL}};
static const UICommandStripCommand compare_external_scope_commands[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "Directory", "D", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "Logged tree", "T", NULL}};
static const UICommandStripCommand compare_target_hint_commands[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "browse", "F2", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "history", "Up", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "OK", "Enter", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "cancel", "Esc", NULL}};
static const CompareGeneratedHelpSpec compare_target_help_spec = {
    "prompt.compare-target", "COMMANDS ", compare_target_hint_commands,
    sizeof(compare_target_hint_commands) / sizeof(compare_target_hint_commands[0])};
static const CompareGeneratedHelpSpec compare_scope_help_spec = {
    "prompt.compare-scope", "COMMANDS ", compare_scope_commands,
    sizeof(compare_scope_commands) / sizeof(compare_scope_commands[0])};
static const CompareGeneratedHelpSpec compare_external_scope_help_spec = {
    "prompt.compare-scope", "COMMANDS ", compare_external_scope_commands,
    sizeof(compare_external_scope_commands) /
        sizeof(compare_external_scope_commands[0])};
static const CompareGeneratedHelpSpec compare_basis_help_spec = {
    "prompt.compare-basis", "COMMANDS ", compare_basis_commands,
    sizeof(compare_basis_commands) / sizeof(compare_basis_commands[0])};
static const CompareGeneratedHelpSpec compare_results_help_spec = {
    "prompt.compare-results", "COMMANDS ", compare_tag_result_commands,
    sizeof(compare_tag_result_commands) /
        sizeof(compare_tag_result_commands[0])};

static void ClearComparePromptArea(ViewContext *ctx) {
  if (!ctx || !ctx->ctx_border_window)
    return;

#ifdef COLOR_SUPPORT
  wattrset(ctx->ctx_border_window, COLOR_PAIR(UI_ROLE_STATIC_TEXT));
#else
  wattrset(ctx->ctx_border_window, A_NORMAL);
#endif
  wattroff(ctx->ctx_border_window, A_ALTCHARSET);

  if (ctx->layout.prompt_y > 0) {
    wmove(ctx->ctx_border_window, ctx->layout.prompt_y - 1, 0);
    wclrtoeol(ctx->ctx_border_window);
  }
  wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wmove(ctx->ctx_border_window, ctx->layout.status_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);
  doupdate();
}

static void DrawComparePrompt(ViewContext *ctx, const char *title,
                              const UICommandStripCommand *commands,
                              size_t command_count) {
  int prompt_x;
  int status_x;

  if (!ctx || !ctx->ctx_border_window || !title)
    return;

  ClearComparePromptArea(ctx);
#ifdef COLOR_SUPPORT
  wattrset(ctx->ctx_border_window, COLOR_PAIR(UI_ROLE_STATIC_TEXT));
#else
  wattrset(ctx->ctx_border_window, A_NORMAL);
#endif
  wattroff(ctx->ctx_border_window, A_ALTCHARSET);

  Print(ctx->ctx_border_window, ctx->layout.prompt_y, 1, (char *)title,
        UI_ROLE_STATIC_TEXT);
  prompt_x = 1 + StrVisualLength((char *)title);
  if (commands != NULL && command_count > 0) {
    prompt_x += 2;
    UI_RenderAdaptiveCommandStrip(ctx->ctx_border_window, ctx->layout.prompt_y,
                                  prompt_x, commands, command_count,
                                  UI_ROLE_STATIC_TEXT, UI_ROLE_KEYBIND);
  }

  Print(ctx->ctx_border_window, ctx->layout.status_y, 1, "COMMANDS",
        UI_ROLE_STATIC_TEXT);
  status_x = 1 + StrVisualLength("COMMANDS") + 2;
  UI_RenderAdaptiveCommandStrip(
      ctx->ctx_border_window, ctx->layout.status_y, status_x,
      compare_status_commands,
      sizeof(compare_status_commands) / sizeof(compare_status_commands[0]),
      UI_ROLE_STATIC_TEXT, UI_ROLE_KEYBIND);
  wnoutrefresh(ctx->ctx_border_window);
  doupdate();
}

static int ShowCompareHelpCallback(ViewContext *ctx, void *help_data) {
  const CompareGeneratedHelpSpec *spec =
      (const CompareGeneratedHelpSpec *)help_data;
  UIHelpPopupRow rows[1];

  if (ctx == NULL || spec == NULL)
    return 0;

  rows[0].kind = UI_HELP_POPUP_COMMAND_STRIP;
  rows[0].prefix = spec->prefix;
  rows[0].text = NULL;
  rows[0].commands = spec->commands;
  rows[0].command_count = spec->command_count;
  (void)UI_ShowGeneratedContextHelp(ctx, spec->context_id, rows,
                                    sizeof(rows) / sizeof(rows[0]));
  return 0;
}

static int InputCompareChoice(ViewContext *ctx, const char *title,
                              const UICommandStripCommand *commands,
                              size_t command_count, const char *valid_terms,
                              int default_choice,
                              const CompareGeneratedHelpSpec *help_spec) {
  if (!ctx || !title || !valid_terms)
    return ESC;

  while (1) {
    int ch;

    DrawComparePrompt(ctx, title, commands, command_count);

    ch = WGetch(ctx, ctx->ctx_border_window);
    if (ch < 0)
      continue;

    if (ch == KEY_F(1)) {
      (void)ShowCompareHelpCallback(ctx, (void *)help_spec);
      continue;
    }
    if (ch == ESC) {
      ClearComparePromptArea(ctx);
      return ESC;
    }

    if (ch == CR || ch == LF) {
      if (default_choice > 0) {
        ch = default_choice;
      } else {
        continue;
      }
    }

    if (islower(ch))
      ch = toupper(ch);

    if (strchr(valid_terms, ch) != NULL) {
      ClearComparePromptArea(ctx);
      return ch;
    }
  }
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

static int PromptCompareBasis(ViewContext *ctx, CompareBasis *basis) {
  int ch;

  if (!ctx || !basis)
    return -1;

  ch = InputCompareChoice(
      ctx, "COMPARE BASIS:", compare_basis_commands,
      sizeof(compare_basis_commands) / sizeof(compare_basis_commands[0]), "SDZH",
      0, &compare_basis_help_spec);
  if (ch == ESC || ch < 0)
    return -1;

  switch (ch) {
  case 'S':
    *basis = COMPARE_BASIS_SIZE;
    return 0;
  case 'D':
    *basis = COMPARE_BASIS_DATE;
    return 0;
  case 'Z':
    *basis = COMPARE_BASIS_SIZE_AND_DATE;
    return 0;
  case 'H':
    *basis = COMPARE_BASIS_HASH;
    return 0;
  default:
    return -1;
  }
}

static int PromptCompareTagResult(ViewContext *ctx,
                                  CompareTagResult *tag_result) {
  int ch;

  if (!ctx || !tag_result)
    return -1;

  ch = InputCompareChoice(
      ctx, "TAG FILE LIST:", compare_tag_result_commands,
      sizeof(compare_tag_result_commands) /
          sizeof(compare_tag_result_commands[0]),
      "FMNOUTE", 0, &compare_results_help_spec);
  if (ch == ESC || ch < 0)
    return -1;

  switch (ch) {
  case 'F':
    *tag_result = COMPARE_TAG_DIFFERENT;
    return 0;
  case 'M':
    *tag_result = COMPARE_TAG_MATCH;
    return 0;
  case 'N':
    *tag_result = COMPARE_TAG_NEWER;
    return 0;
  case 'O':
    *tag_result = COMPARE_TAG_OLDER;
    return 0;
  case 'U':
    *tag_result = COMPARE_TAG_UNIQUE;
    return 0;
  case 'T':
    *tag_result = COMPARE_TAG_TYPE_MISMATCH;
    return 0;
  case 'E':
    *tag_result = COMPARE_TAG_ERROR;
    return 0;
  default:
    return -1;
  }
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

int UI_SelectCompareMenuChoice(ViewContext *ctx, CompareMenuChoice *choice) {
  int ch;

  if (!ctx || !choice)
    return -1;

  ch = InputCompareChoice(
      ctx, "COMPARE SCOPE:", compare_scope_commands,
      sizeof(compare_scope_commands) / sizeof(compare_scope_commands[0]), "DTX",
      'D', &compare_scope_help_spec);
  if (ch == ESC || ch < 0)
    return -1;

  if (ch == 'D') {
    *choice = COMPARE_MENU_DIRECTORY_ONLY;
    return 0;
  }
  if (ch == 'T') {
    *choice = COMPARE_MENU_DIRECTORY_PLUS_TREE;
    return 0;
  }
  if (ch == 'X') {
    int scope_ch = InputCompareChoice(
        ctx, "EXTERNAL VIEWER:", compare_external_scope_commands,
        sizeof(compare_external_scope_commands) /
            sizeof(compare_external_scope_commands[0]),
        "DT", 'D', &compare_external_scope_help_spec);
    if (scope_ch == ESC || scope_ch < 0)
      return -1;
    *choice = (scope_ch == 'T') ? COMPARE_MENU_EXTERNAL_TREE
                                : COMPARE_MENU_EXTERNAL_DIRECTORY;
    return 0;
  }

  return -1;
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

static int BuildDirectoryCompareRequestInternal(ViewContext *ctx,
                                                DirEntry *source_dir,
                                                CompareFlowType flow_type,
                                                CompareRequest *request,
                                                BOOL include_compare_prompts) {
  YtreeNovaPanel *inactive = NULL;
  const char *default_target = NULL;

  if (!ctx || !request)
    return -1;
  if (flow_type != COMPARE_FLOW_DIRECTORY &&
      flow_type != COMPARE_FLOW_LOGGED_TREE) {
    return -1;
  }

  memset(request, 0, sizeof(*request));
  request->flow_type = flow_type;

  if (flow_type == COMPARE_FLOW_DIRECTORY) {
    if (!source_dir)
      return -1;
    GetPath(source_dir, request->source_path);
  } else {
    if (!ctx->active || !ctx->active->vol || !ctx->active->vol->vol_stats.tree)
      return -1;
    GetPath(ctx->active->vol->vol_stats.tree, request->source_path);
  }
  request->source_path[PATH_LENGTH] = '\0';

  if (ctx->is_split_screen) {
    inactive = UI_GetInactivePanel(ctx);
    if (inactive) {
      if (flow_type == COMPARE_FLOW_DIRECTORY) {
        if (UI_GetPanelSelectedDirPath(ctx, inactive, request->target_path) == 0) {
          default_target = request->target_path;
          request->used_split_default_target = TRUE;
        }
      } else if (UI_GetPanelLoggedRootPath(inactive, request->target_path) == 0) {
        default_target = request->target_path;
        request->used_split_default_target = TRUE;
      }
    }
  }

  if (PromptCompareTargetPath(ctx, "COMPARE TARGET:",
                              default_target ? default_target
                                             : request->source_path,
                              request->target_path,
                              &compare_target_help_spec) != 0) {
    return -1;
  }
  request->target_path[PATH_LENGTH] = '\0';

  if (include_compare_prompts) {
    if (PromptCompareBasis(ctx, &request->basis) != 0)
      return -1;
    if (PromptCompareTagResult(ctx, &request->tag_result) != 0)
      return -1;
  } else {
    request->basis = COMPARE_BASIS_NONE;
    request->tag_result = COMPARE_TAG_NONE;
  }

  return 0;
}

int UI_BuildDirectoryCompareRequest(ViewContext *ctx, DirEntry *source_dir,
                                    CompareFlowType flow_type,
                                    CompareRequest *request) {
  return BuildDirectoryCompareRequestInternal(ctx, source_dir, flow_type,
                                              request, TRUE);
}

int UI_BuildDirectoryCompareLaunchRequest(ViewContext *ctx,
                                          DirEntry *source_dir,
                                          CompareFlowType flow_type,
                                          CompareRequest *request) {
  return BuildDirectoryCompareRequestInternal(ctx, source_dir, flow_type,
                                              request, FALSE);
}
