/***************************************************************************
 *
 * src/ui/compare_request.c
 * Compare request prompt/build helpers extracted from interactions.c.
 *
 ***************************************************************************/

#include "interactions_panel_paths.h"
#include "ytree_fs.h"
#include "ytree_ui.h"
#include <ctype.h>
#include <string.h>

typedef enum {
  COMPARE_HELP_FILE_TARGET = 0,
  COMPARE_HELP_DIRECTORY_TARGET,
  COMPARE_HELP_TREE_TARGET,
  COMPARE_HELP_SCOPE,
  COMPARE_HELP_EXTERNAL_SCOPE,
  COMPARE_HELP_BASIS,
  COMPARE_HELP_RESULTS
} CompareHelpTopic;

static void ClearComparePromptArea(ViewContext *ctx) {
  if (!ctx || !ctx->ctx_border_window)
    return;

#ifdef COLOR_SUPPORT
  wattrset(ctx->ctx_border_window, COLOR_PAIR(CPAIR_MENU));
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

static void DrawComparePrompt(ViewContext *ctx, const char *prompt) {
  if (!ctx || !ctx->ctx_border_window || !prompt)
    return;

  ClearComparePromptArea(ctx);
#ifdef COLOR_SUPPORT
  wattrset(ctx->ctx_border_window, COLOR_PAIR(CPAIR_MENU));
#else
  wattrset(ctx->ctx_border_window, A_NORMAL);
#endif
  wattroff(ctx->ctx_border_window, A_ALTCHARSET);
  PrintMenuOptions(ctx->ctx_border_window, ctx->layout.prompt_y, 1,
                   (char *)prompt, CPAIR_MENU, CPAIR_HIMENUS);
  PrintMenuOptions(ctx->ctx_border_window, ctx->layout.status_y, 1,
                   "COMMANDS  (F1) context help  (Esc) cancel", CPAIR_MENU,
                   CPAIR_HIMENUS);
  wnoutrefresh(ctx->ctx_border_window);
  doupdate();
}

static void GetCompareHelpLines(CompareHelpTopic topic, const char **title,
                                const char **line_0, const char **line_1,
                                const char **line_2) {
  if (!title || !line_0 || !line_1 || !line_2)
    return;

  *title = "Compare Help";

  switch (topic) {
  case COMPARE_HELP_FILE_TARGET:
    *line_0 = "Current file is the compare source.";
    *line_1 = "Enter the target path, or use (F2) browse and (Up) history.";
    *line_2 = "In split view, the inactive panel provides the default target.";
    break;
  case COMPARE_HELP_DIRECTORY_TARGET:
    *line_0 = "Current directory is the compare source.";
    *line_1 = "Enter the target path, or use (F2) browse and (Up) history.";
    *line_2 = "In split view, the inactive panel provides the default target.";
    break;
  case COMPARE_HELP_TREE_TARGET:
    *line_0 = "Current logged tree is the compare source.";
    *line_1 = "Enter the target path, or use (F2) browse and (Up) history.";
    *line_2 = "In split view, the inactive panel provides the default target.";
    break;
  case COMPARE_HELP_SCOPE:
    *line_0 = "Directory only compares the current directory.";
    *line_1 = "Logged tree compares the current logged tree recursively.";
    *line_2 = "Tree compare never auto-logs unopened '+' subdirectories.";
    break;
  case COMPARE_HELP_EXTERNAL_SCOPE:
    *line_0 = "External compare launches DIRDIFF/TREEDIFF viewer commands.";
    *line_1 = "It does not tag files and does not replace internal compare.";
    *line_2 = "Choose directory or logged tree source scope for launch.";
    break;
  case COMPARE_HELP_BASIS:
    *line_0 = "Size checks file length. Date checks the last-modified time.";
    *line_1 =
        "siZe+date marks a difference if size or modification time differs.";
    *line_2 = "Hash opens both files and compares their content exactly, so it "
              "is slower.";
    break;
  case COMPARE_HELP_RESULTS:
  default:
    *line_0 = "Choose which compare result to tag in the active file list.";
    *line_1 =
        "diFferent tags basis mismatches. Unique tags source-only entries.";
    *line_2 =
        "Match/Newer/Older/Type-mismatch/Error each tag only that outcome.";
    break;
  }
}

static void ShowCompareHelpPopup(ViewContext *ctx, CompareHelpTopic topic) {
  WINDOW *win;
  const char *title = NULL;
  const char *line_0 = NULL;
  const char *line_1 = NULL;
  const char *line_2 = NULL;
  const char *close_prompt = "(F1)/(Esc) close help";
  const char *help_lines[3];
  int height;
  int i;
  int width;
  int win_x;
  int win_y;

  if (!ctx)
    return;

  GetCompareHelpLines(topic, &title, &line_0, &line_1, &line_2);
  help_lines[0] = line_0;
  help_lines[1] = line_1;
  help_lines[2] = line_2;

  width = StrVisualLength(title) + 8;
  for (i = 0; i < 3; i++) {
    int line_len = StrVisualLength(help_lines[i]) + 4;
    if (line_len > width)
      width = line_len;
  }
  if (StrVisualLength(close_prompt) + 4 > width)
    width = StrVisualLength(close_prompt) + 4;

  width = MINIMUM(width, COLS - 4);
  width = MAXIMUM(width, 42);
  height = 7;
  win_x = MAXIMUM(1, (COLS - width) / 2);
  win_y = MAXIMUM(1, (LINES - height) / 2);

  win = newwin(height, width, win_y, win_x);
  if (!win)
    return;

  UI_Dialog_Push(win, UI_TIER_MODAL);
  keypad(win, TRUE);
  WbkgdSet(ctx, win, COLOR_PAIR(CPAIR_MENU));
  curs_set(0);

  while (1) {
    int ch;

    werase(win);
    box(win, 0, 0);
    mvwprintw(win, 1, MAXIMUM(2, (width - StrVisualLength(title)) / 2), "%s",
              title);
    for (i = 0; i < 3; i++) {
      mvwprintw(win, 2 + i, 2, "%.*s", width - 4, help_lines[i]);
    }
    PrintMenuOptions(win, height - 2, 2, (char *)close_prompt, CPAIR_MENU,
                     CPAIR_HIMENUS);
    wrefresh(win);

    ch = WGetch(ctx, win);
    if (ch != ERR)
      break;
  }

  UI_Dialog_Close(ctx, win);
}

static int ShowCompareHelpCallback(ViewContext *ctx, void *help_data) {
  CompareHelpTopic topic = COMPARE_HELP_FILE_TARGET;

  if (help_data != NULL)
    topic = *(CompareHelpTopic *)help_data;

  ShowCompareHelpPopup(ctx, topic);
  return 0;
}

static int InputCompareChoice(ViewContext *ctx, const char *prompt,
                              const char *valid_terms, int default_choice,
                              CompareHelpTopic help_topic) {
  if (!ctx || !prompt || !valid_terms)
    return ESC;

  while (1) {
    int ch;

    DrawComparePrompt(ctx, prompt);

    ch = WGetch(ctx, ctx->ctx_border_window);
    if (ch < 0)
      continue;

    if (ch == KEY_F(1)) {
      ShowCompareHelpPopup(ctx, help_topic);
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
                                   CompareHelpTopic help_topic) {
  static const char compare_target_hints[] =
      "(F1) help  (F2) browse  (Up) history  (Enter) OK  (Esc) cancel";

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
                            HST_PATH, compare_target_hints,
                            ShowCompareHelpCallback, &help_topic) != CR) {
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

  ch =
      InputCompareChoice(ctx, "COMPARE BASIS: (S)ize (D)ate si(Z)e+date (H)ash",
                         "SDZH", 0, COMPARE_HELP_BASIS);
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
      ctx,
      "TAG FILE LIST: di(F)ferent (M)atch (N)ewer (O)lder (U)nique "
      "(T)ype-mismatch (E)rror",
      "FMNOUTE", 0, COMPARE_HELP_RESULTS);
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

  ch = InputCompareChoice(ctx,
                          "COMPARE SCOPE: (D)irectory only  logged (T)ree  "
                          "e(X)ternal viewer",
                          "DTX", 'D', COMPARE_HELP_SCOPE);
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
    int scope_ch =
        InputCompareChoice(ctx, "EXTERNAL VIEWER: (D)irectory  logged (T)ree",
                           "DT", 'D', COMPARE_HELP_EXTERNAL_SCOPE);
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
  YtreePanel *inactive = NULL;
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
          request->target_path, COMPARE_HELP_FILE_TARGET) != 0) {
    return -1;
  }

  return 0;
}

static int BuildDirectoryCompareRequestInternal(ViewContext *ctx,
                                                DirEntry *source_dir,
                                                CompareFlowType flow_type,
                                                CompareRequest *request,
                                                BOOL include_compare_prompts) {
  YtreePanel *inactive = NULL;
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
                              flow_type == COMPARE_FLOW_DIRECTORY
                                  ? COMPARE_HELP_DIRECTORY_TARGET
                                  : COMPARE_HELP_TREE_TARGET) != 0) {
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
