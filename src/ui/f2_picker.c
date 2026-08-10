/***************************************************************************
 *
 * src/ui/f2_picker.c
 * F2 Directory Picker (Independent Event Loop)
 *
 ***************************************************************************/

#include "ytnova_cmd.h"
#include "ytnova_appstate_mode.h"
#include "ytnova_appstate_panel.h"
#include "ytnova_appstate_volume.h"
#include "ytnova_fs.h"
#include "ytnova_panel_anchor.h"
#include "ytnova_ui.h"
#include <stdlib.h>

static const UICommandStripCommand f2_command_strip[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "help", "F1", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Log", "L", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "cycle", "<", ">"},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "dotfiles", "`", NULL}};
static const UICommandStripCommand f2_context_command_strip[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "select", "Enter", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "cancel", "Esc", NULL}};

typedef struct {
  struct Volume *target_vol;
  int win_height;
  int disp_begin_pos;
  int cursor_pos;
  int result;
} F2PickerLoopState;

static void F2NormalizeSelectionForVisibility(
    const YtreeNovaPanel *panel, const struct Volume *target_vol, int win_height,
    int *disp_begin_pos, int *cursor_pos);
static BOOL F2DirIsVisible(const YtreeNovaPanel *panel,
                           const struct Volume *target_vol,
                           const DirEntry *dir_entry);
static int F2FindVisibleIndex(const YtreeNovaPanel *panel,
                              const struct Volume *target_vol, int start_idx,
                              int direction);

static int F2VisibleRows(int win_height) {
  if (win_height <= 1)
    return 1;
  return win_height - 1;
}

static int F2ReadTreeDepth(ViewContext *ctx) {
  int read_depth = 0;

  if (ctx != NULL)
    read_depth = (int)strtol((GetProfileValue)(ctx, "TREEDEPTH"), NULL, 0);
  if (read_depth < 0)
    read_depth = 0;
  return read_depth;
}

static int F2ResolveSelectionIndex(const YtreeNovaPanel *panel,
                                   const struct Volume *target_vol,
                                   int disp_begin_pos, int cursor_pos) {
  int index;
  int step;

  if (!panel || !target_vol || !target_vol->dir_entry_list ||
      target_vol->total_dirs <= 0)
    return -1;

  index = F2FindVisibleIndex(panel, target_vol, disp_begin_pos, 1);
  if (index < 0)
    index = F2FindVisibleIndex(panel, target_vol, 0, 1);
  if (index < 0)
    return -1;

  for (step = 0; step < cursor_pos; ++step) {
    int next_index = F2FindVisibleIndex(panel, target_vol, index + 1, 1);
    if (next_index < 0)
      break;
    index = next_index;
  }

  return index;
}

static int F2VisibleRowOffset(const YtreeNovaPanel *panel,
                              const struct Volume *target_vol, int start_idx,
                              int target_idx, int visible_rows) {
  int index;
  int row;

  if (!panel || !target_vol || !target_vol->dir_entry_list || visible_rows < 1)
    return -1;

  index = F2FindVisibleIndex(panel, target_vol, start_idx, 1);
  if (index < 0)
    return -1;

  for (row = 0; row < visible_rows && index >= 0; ++row) {
    if (index == target_idx)
      return row;
    index = F2FindVisibleIndex(panel, target_vol, index + 1, 1);
  }

  return -1;
}

static int F2BacktrackVisibleIndex(const YtreeNovaPanel *panel,
                                   const struct Volume *target_vol,
                                   int target_idx, int visible_steps) {
  int index;
  int step;

  if (!panel || !target_vol || !target_vol->dir_entry_list || target_idx < 0 ||
      target_idx >= target_vol->total_dirs)
    return -1;

  index = target_idx;
  for (step = 0; step < visible_steps; ++step) {
    int prev_index = F2FindVisibleIndex(panel, target_vol, index - 1, -1);
    if (prev_index < 0)
      break;
    index = prev_index;
  }

  return index;
}

static DirEntry *F2CurrentDir(const YtreeNovaPanel *panel,
                              struct Volume *target_vol, int disp_begin_pos,
                              int cursor_pos) {
  int index;

  if (!panel || !target_vol || !target_vol->dir_entry_list ||
      target_vol->total_dirs <= 0)
    return NULL;

  index = F2ResolveSelectionIndex(panel, target_vol, disp_begin_pos, cursor_pos);
  if (index < 0 || index >= target_vol->total_dirs)
    return NULL;

  return target_vol->dir_entry_list[index].dir_entry;
}

static BOOL F2PositionAtIndex(const YtreeNovaPanel *panel,
                              const struct Volume *target_vol, int target_index,
                              int win_height, int *disp_begin_pos,
                              int *cursor_pos) {
  int visible_rows;
  int start_index;
  int row;

  if (!panel || !target_vol || !disp_begin_pos || !cursor_pos ||
      target_index < 0 || target_index >= target_vol->total_dirs ||
      !F2DirIsVisible(panel, target_vol,
                      target_vol->dir_entry_list[target_index].dir_entry))
    return FALSE;

  visible_rows = F2VisibleRows(win_height);
  if (visible_rows < 1)
    visible_rows = 1;

  start_index = F2FindVisibleIndex(panel, target_vol, *disp_begin_pos, 1);
  if (start_index < 0)
    start_index = F2FindVisibleIndex(panel, target_vol, 0, 1);
  if (start_index < 0)
    return FALSE;

  row = F2VisibleRowOffset(panel, target_vol, start_index, target_index,
                           visible_rows);
  if (row >= 0) {
    *disp_begin_pos = start_index;
    *cursor_pos = row;
    return TRUE;
  }

  if (target_index < start_index) {
    *disp_begin_pos = target_index;
    *cursor_pos = 0;
    return TRUE;
  }

  start_index =
      F2BacktrackVisibleIndex(panel, target_vol, target_index, visible_rows - 1);
  if (start_index < 0)
    return FALSE;

  row = F2VisibleRowOffset(panel, target_vol, start_index, target_index,
                           visible_rows);
  if (row < 0)
    return FALSE;

  *disp_begin_pos = start_index;
  *cursor_pos = row;
  return TRUE;
}

static BOOL F2PositionAtIndexAnchored(const YtreeNovaPanel *panel,
                                      const struct Volume *target_vol,
                                      int target_index, int win_height,
                                      int *disp_begin_pos, int *cursor_pos) {
  int visible_rows;
  int start_index;
  int row;

  if (!panel || !target_vol || !disp_begin_pos || !cursor_pos ||
      target_index < 0 || target_index >= target_vol->total_dirs ||
      !F2DirIsVisible(panel, target_vol,
                      target_vol->dir_entry_list[target_index].dir_entry))
    return FALSE;

  visible_rows = F2VisibleRows(win_height);
  if (visible_rows <= 1)
    return F2PositionAtIndex(panel, target_vol, target_index, win_height,
                             disp_begin_pos, cursor_pos);
  start_index =
      F2BacktrackVisibleIndex(panel, target_vol, target_index, visible_rows / 2);
  if (start_index < 0)
    return FALSE;
  row = F2VisibleRowOffset(panel, target_vol, start_index, target_index,
                           visible_rows);
  if (row < 0)
    return FALSE;
  *disp_begin_pos = start_index;
  *cursor_pos = row;
  return TRUE;
}

static BOOL F2DirIsVisible(const YtreeNovaPanel *panel,
                           const struct Volume *target_vol,
                           const DirEntry *dir_entry) {
  const DirEntry *ancestor;

  if (!panel || !target_vol || !dir_entry)
    return FALSE;

  if (!panel->hide_dot_files)
    return TRUE;

  if (dir_entry == target_vol->vol_stats.tree)
    return TRUE;

  if (dir_entry->name[0] == '.')
    return FALSE;

  for (ancestor = dir_entry->up_tree;
       ancestor && ancestor != target_vol->vol_stats.tree;
       ancestor = ancestor->up_tree) {
    if (ancestor->name[0] == '.')
      return FALSE;
  }

  return TRUE;
}

static int F2FindVisibleIndex(const YtreeNovaPanel *panel,
                              const struct Volume *target_vol, int start_idx,
                              int direction) {
  int idx;
  int total_dirs;

  if (!panel || !target_vol || !target_vol->dir_entry_list)
    return -1;

  total_dirs = target_vol->total_dirs;
  if (total_dirs <= 0)
    return -1;

  if (direction == 0)
    direction = 1;
  direction = (direction > 0) ? 1 : -1;

  if (start_idx < 0)
    return -1;
  if (start_idx >= total_dirs)
    return -1;

  for (idx = start_idx; idx >= 0 && idx < total_dirs; idx += direction) {
    if (F2DirIsVisible(panel, target_vol,
                       target_vol->dir_entry_list[idx].dir_entry))
      return idx;
  }

  return -1;
}

static BOOL F2PositionAtDir(const YtreeNovaPanel *panel,
                            const struct Volume *target_vol,
                            const DirEntry *target_dir, int win_height,
                            int *disp_begin_pos, int *cursor_pos) {
  int i;

  if (!target_vol || !target_dir || !target_vol->dir_entry_list)
    return FALSE;

  for (i = 0; i < target_vol->total_dirs; ++i) {
    if (target_vol->dir_entry_list[i].dir_entry == target_dir)
      return F2PositionAtIndex(panel, target_vol, i, win_height, disp_begin_pos,
                               cursor_pos);
  }

  return FALSE;
}

static int F2ResolveAnchorIndex(const YtreeNovaPanel *panel,
                                const struct Volume *target_vol) {
  int current_index;
  int visible_index;

  if (!panel || !target_vol || !target_vol->dir_entry_list ||
      target_vol->total_dirs <= 0)
    return -1;

  if (panel->vol != target_vol)
    return F2FindVisibleIndex(panel, target_vol, 0, 1);

  current_index = GetPanelVisibleSelectionIndex(panel);
  if (current_index < 0)
    current_index = F2FindVisibleIndex(panel, target_vol, 0, 1);

  if (F2DirIsVisible(panel, target_vol,
                     target_vol->dir_entry_list[current_index].dir_entry)) {
    return current_index;
  }

  visible_index = F2FindVisibleIndex(panel, target_vol, current_index, 1);
  if (visible_index < 0)
    visible_index = F2FindVisibleIndex(panel, target_vol, current_index - 1, -1);
  if (visible_index < 0)
    visible_index = F2FindVisibleIndex(panel, target_vol, 0, 1);
  return visible_index;
}

static void F2AnchorSelectionFromPanel(const YtreeNovaPanel *panel,
                                       const struct Volume *target_vol,
                                       int win_height, int *disp_begin_pos,
                                       int *cursor_pos) {
  int anchor_index;

  if (!panel || !target_vol || !disp_begin_pos || !cursor_pos)
    return;

  anchor_index = F2ResolveAnchorIndex(panel, target_vol);
  if (anchor_index >= 0 &&
      F2PositionAtIndexAnchored(panel, target_vol, anchor_index, win_height,
                                disp_begin_pos, cursor_pos)) {
    return;
  }

  F2NormalizeSelectionForVisibility(panel, target_vol, win_height,
                                    disp_begin_pos, cursor_pos);
}

static BOOL F2ExpandCurrentDir(ViewContext *ctx, const YtreeNovaPanel *panel,
                               struct Volume *target_vol, int win_height,
                               int *disp_begin_pos, int *cursor_pos) {
  DirEntry *selected;
  char selected_path[PATH_LENGTH + 1];
  int dummy_counter = 0;
  int read_depth = 1;

  if (!ctx || !target_vol)
    return FALSE;

  selected = F2CurrentDir(panel, target_vol, *disp_begin_pos, *cursor_pos);
  if (selected == NULL)
    return FALSE;

  if (!selected->not_scanned && selected->sub_tree != NULL)
    return F2PositionAtDir(panel, target_vol, selected->sub_tree, win_height,
                           disp_begin_pos, cursor_pos);

  if (!selected->unlogged_flag &&
      (selected->sub_tree != NULL || selected->file != NULL)) {
    if (!AppStateCommitDirEntryLoggedState(selected, FALSE,
                                           selected->unlogged_flag))
      return FALSE;
    BuildDirEntryList(ctx, target_vol, &dummy_counter);
    BuildDirEntryList(ctx, target_vol, &dummy_counter);
    return F2PositionAtDir(panel, target_vol, selected, win_height, disp_begin_pos,
                           cursor_pos);
  }

  flushinp();
  SuspendClock(ctx);
  GetPath(selected, selected_path);
  if (selected->unlogged_flag)
    read_depth = F2ReadTreeDepth(ctx);
  (void)ReadTree(ctx, selected, selected_path, read_depth,
                 &target_vol->vol_stats, NULL, NULL);
  ApplyFilter(selected, &target_vol->vol_stats);
  InitClock(ctx);

  if (!AppStateCommitDirEntryLoggedState(selected, FALSE, FALSE))
    return FALSE;

  BuildDirEntryList(ctx, target_vol, &dummy_counter);
  BuildDirEntryList(ctx, target_vol, &dummy_counter);
  return F2PositionAtDir(panel, target_vol, selected, win_height,
                         disp_begin_pos, cursor_pos);
}

static BOOL F2CollapseCurrentDir(ViewContext *ctx, const YtreeNovaPanel *panel,
                                 struct Volume *target_vol, int win_height,
                                 int *disp_begin_pos, int *cursor_pos) {
  DirEntry *selected;
  DirEntry *de_ptr;
  FileEntry *fe_ptr;
  FileEntry *next_fe_ptr;
  int dummy_counter = 0;

  if (!ctx || !target_vol)
    return FALSE;

  selected = F2CurrentDir(panel, target_vol, *disp_begin_pos, *cursor_pos);
  if (selected == NULL)
    return FALSE;

  if (selected->not_scanned || selected->sub_tree == NULL) {
    if (selected->up_tree == NULL)
      return FALSE;
    return F2PositionAtDir(panel, target_vol, selected->up_tree, win_height,
                           disp_begin_pos, cursor_pos);
  }

  for (de_ptr = selected->sub_tree; de_ptr; de_ptr = de_ptr->next) {
    UnReadTree(ctx, de_ptr, &target_vol->vol_stats);
  }
  for (fe_ptr = selected->file; fe_ptr; fe_ptr = next_fe_ptr) {
    next_fe_ptr = fe_ptr->next;
    RemoveFile(ctx, fe_ptr, &target_vol->vol_stats);
  }

  if (!AppStateCommitDirEntryLoggedState(selected, TRUE, TRUE))
    return FALSE;

  BuildDirEntryList(ctx, target_vol, &dummy_counter);
  BuildDirEntryList(ctx, target_vol, &dummy_counter);
  return F2PositionAtDir(panel, target_vol, selected, win_height, disp_begin_pos,
                         cursor_pos);
}

static struct Volume *F2ResolveTargetVolume(ViewContext *ctx) {
  struct Volume *v;
  struct Volume *tmp;

  if (ctx == NULL || ctx->active == NULL)
    return NULL;

  if (ctx->view_mode == DISK_MODE || ctx->view_mode == USER_MODE)
    return ctx->active->vol;

  HASH_ITER(hh, ctx->volumes_head, v, tmp) {
    if (v->vol_stats.log_mode == DISK_MODE)
      return v;
  }

  return ctx->active->vol;
}

static void F2EnsureDirEntryList(ViewContext *ctx, struct Volume *target_vol) {
  int dummy_counter;

  if (ctx == NULL || target_vol == NULL || target_vol->dir_entry_list != NULL)
    return;

  BuildDirEntryList(ctx, target_vol, &dummy_counter);
}

static void F2ClampViewport(const struct Volume *target_vol, int *disp_begin_pos,
                            int *cursor_pos) {
  if (disp_begin_pos == NULL || cursor_pos == NULL)
    return;

  if (*disp_begin_pos < 0)
    *disp_begin_pos = 0;
  if (*cursor_pos < 0)
    *cursor_pos = 0;

  if (target_vol != NULL && target_vol->total_dirs > 0 &&
      (*disp_begin_pos + *cursor_pos >= target_vol->total_dirs)) {
    *disp_begin_pos = 0;
    *cursor_pos = 0;
  }
}

static void F2NormalizeSelectionForVisibility(
    const YtreeNovaPanel *panel, const struct Volume *target_vol, int win_height,
    int *disp_begin_pos, int *cursor_pos) {
  int current_index;
  int visible_index;

  if (!panel || !target_vol || !disp_begin_pos || !cursor_pos)
    return;

  F2ClampViewport(target_vol, disp_begin_pos, cursor_pos);
  if (target_vol->total_dirs <= 0)
    return;

  current_index =
      F2ResolveSelectionIndex(panel, target_vol, *disp_begin_pos, *cursor_pos);
  if (current_index < 0)
    current_index = F2FindVisibleIndex(panel, target_vol, 0, 1);
  if (current_index < 0)
    return;

  if (F2DirIsVisible(panel, target_vol,
                     target_vol->dir_entry_list[current_index].dir_entry)) {
    (void)F2PositionAtIndex(panel, target_vol, current_index, win_height,
                            disp_begin_pos, cursor_pos);
    return;
  }

  visible_index = F2FindVisibleIndex(panel, target_vol, current_index, 1);
  if (visible_index < 0)
    visible_index = F2FindVisibleIndex(panel, target_vol, current_index - 1, -1);
  if (visible_index < 0)
    visible_index = F2FindVisibleIndex(panel, target_vol, 0, 1);
  if (visible_index >= 0)
    (void)F2PositionAtIndex(panel, target_vol, visible_index, win_height,
                            disp_begin_pos, cursor_pos);
}

static void F2DisplayTreeAt(ViewContext *ctx, const YtreeNovaPanel *panel,
                            struct Volume *target_vol, int disp_begin_pos,
                            int cursor_pos) {
  int current_index;

  current_index =
      F2ResolveSelectionIndex(panel, target_vol, disp_begin_pos, cursor_pos);
  if (current_index < 0)
    current_index = F2FindVisibleIndex(panel, target_vol, disp_begin_pos, 1);
  if (current_index < 0)
    current_index = 0;

  DisplayTree(ctx, target_vol, ctx->ctx_f2_window, disp_begin_pos,
              current_index, TRUE);
}

static void F2RedrawMainWindows(ViewContext *ctx) {
  if (ctx == NULL || ctx->active == NULL || ctx->active->vol == NULL)
    return;

  DisplayMenu(ctx);
  DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
              ctx->active->disp_begin_pos,
              ctx->active->disp_begin_pos + ctx->active->cursor_pos, TRUE);
  DisplayFileWindow(ctx, ctx->active,
                    GetSelectedDirEntry(ctx, ctx->active->vol));
  RefreshWindow(ctx->ctx_file_window);
  DisplayDiskStatistic(ctx, &ctx->active->vol->vol_stats);
  if (ctx->active->vol->vol_stats.tree) {
    UpdateStatsPanel(ctx, GetSelectedDirEntry(ctx, ctx->active->vol),
                     &ctx->active->vol->vol_stats);
  }
  DisplayAvailBytes(ctx, &ctx->active->vol->vol_stats);
}

static BOOL F2CycleVolume(ViewContext *ctx, YtreeNovaPanel *panel, int direction,
                          F2PickerLoopState *state) {
  int dummy;

  if (ctx == NULL || panel == NULL || state == NULL)
    return FALSE;

  if (CycleLoadedVolume(ctx, panel, direction) != 0)
    return TRUE;

  state->target_vol = ctx->active->vol;
  state->disp_begin_pos = panel->disp_begin_pos;
  state->cursor_pos = panel->cursor_pos;
  BuildDirEntryList(ctx, state->target_vol, &dummy);
  F2AnchorSelectionFromPanel(panel, state->target_vol, state->win_height,
                             &state->disp_begin_pos, &state->cursor_pos);
  F2RedrawMainWindows(ctx);
  MapF2Window(ctx);
  F2DisplayTreeAt(ctx, panel, state->target_vol, state->disp_begin_pos,
                  state->cursor_pos);
  return TRUE;
}

static BOOL F2MoveSelection(ViewContext *ctx, const YtreeNovaPanel *panel,
                            YtreeNovaAction action, F2PickerLoopState *state) {
  int current_idx;
  int target_idx;
  int visible_rows;
  int step;

  if (ctx == NULL || panel == NULL || state == NULL || state->target_vol == NULL)
    return FALSE;

  visible_rows = F2VisibleRows(state->win_height);
  current_idx = F2ResolveSelectionIndex(panel, state->target_vol,
                                        state->disp_begin_pos,
                                        state->cursor_pos);
  if (current_idx < 0)
    current_idx = F2FindVisibleIndex(panel, state->target_vol, 0, 1);
  if (current_idx < 0)
    return TRUE;

  switch (action) {
  case ACTION_MOVE_DOWN:
    target_idx =
        F2FindVisibleIndex(panel, state->target_vol, current_idx + 1, 1);
    if (target_idx < 0)
      return TRUE;
    if (!F2PositionAtIndex(panel, state->target_vol, target_idx,
                           state->win_height,
                           &state->disp_begin_pos, &state->cursor_pos))
      return TRUE;
    F2DisplayTreeAt(ctx, panel, state->target_vol, state->disp_begin_pos,
                    state->cursor_pos);
    return TRUE;

  case ACTION_MOVE_UP:
    target_idx =
        F2FindVisibleIndex(panel, state->target_vol, current_idx - 1, -1);
    if (target_idx < 0)
      return TRUE;
    if (!F2PositionAtIndex(panel, state->target_vol, target_idx,
                           state->win_height,
                           &state->disp_begin_pos, &state->cursor_pos))
      return TRUE;
    F2DisplayTreeAt(ctx, panel, state->target_vol, state->disp_begin_pos,
                    state->cursor_pos);
    return TRUE;

  case ACTION_MOVE_RIGHT:
    if (F2ExpandCurrentDir(ctx, panel, state->target_vol, state->win_height,
                           &state->disp_begin_pos, &state->cursor_pos)) {
      F2DisplayTreeAt(ctx, panel, state->target_vol, state->disp_begin_pos,
                      state->cursor_pos);
    } else {
      UI_Beep(ctx, FALSE);
    }
    return TRUE;

  case ACTION_MOVE_LEFT:
    if (F2CollapseCurrentDir(ctx, panel, state->target_vol, state->win_height,
                             &state->disp_begin_pos, &state->cursor_pos)) {
      F2DisplayTreeAt(ctx, panel, state->target_vol, state->disp_begin_pos,
                      state->cursor_pos);
    } else {
      UI_Beep(ctx, FALSE);
    }
    return TRUE;

  case ACTION_PAGE_DOWN:
    target_idx = current_idx;
    for (step = 0; step < visible_rows - 1; ++step) {
      int next_idx =
          F2FindVisibleIndex(panel, state->target_vol, target_idx + 1, 1);
      if (next_idx < 0)
        break;
      target_idx = next_idx;
    }
    if (target_idx == current_idx)
      return TRUE;
    if (!F2PositionAtIndex(panel, state->target_vol, target_idx,
                           state->win_height,
                           &state->disp_begin_pos, &state->cursor_pos))
      return TRUE;
    F2DisplayTreeAt(ctx, panel, state->target_vol, state->disp_begin_pos,
                    state->cursor_pos);
    return TRUE;

  case ACTION_PAGE_UP:
    target_idx = current_idx;
    for (step = 0; step < visible_rows - 1; ++step) {
      int prev_idx =
          F2FindVisibleIndex(panel, state->target_vol, target_idx - 1, -1);
      if (prev_idx < 0)
        break;
      target_idx = prev_idx;
    }
    if (target_idx == current_idx)
      return TRUE;
    if (!F2PositionAtIndex(panel, state->target_vol, target_idx,
                           state->win_height,
                           &state->disp_begin_pos, &state->cursor_pos))
      return TRUE;
    F2DisplayTreeAt(ctx, panel, state->target_vol, state->disp_begin_pos,
                    state->cursor_pos);
    return TRUE;

  case ACTION_HOME:
    target_idx = F2FindVisibleIndex(panel, state->target_vol, 0, 1);
    if (target_idx < 0 || target_idx == current_idx)
      return TRUE;
    if (!F2PositionAtIndex(panel, state->target_vol, target_idx,
                           state->win_height,
                           &state->disp_begin_pos, &state->cursor_pos))
      return TRUE;
    F2DisplayTreeAt(ctx, panel, state->target_vol, state->disp_begin_pos,
                    state->cursor_pos);
    return TRUE;

  case ACTION_END:
    target_idx = F2FindVisibleIndex(panel, state->target_vol,
                                    state->target_vol->total_dirs - 1, -1);
    if (target_idx < 0 || target_idx == current_idx)
      return TRUE;
    if (!F2PositionAtIndex(panel, state->target_vol, target_idx,
                           state->win_height,
                           &state->disp_begin_pos, &state->cursor_pos))
      return TRUE;
    F2DisplayTreeAt(ctx, panel, state->target_vol, state->disp_begin_pos,
                    state->cursor_pos);
    return TRUE;

  default:
    return FALSE;
  }
}

static BOOL F2HandleLog(ViewContext *ctx, YtreeNovaPanel *panel,
                        F2PickerLoopState *state, YtreeNovaAction *action) {
  char new_log_path[PATH_LENGTH + 1];
  int dummy;

  if (ctx == NULL || panel == NULL || state == NULL || state->target_vol == NULL ||
      action == NULL)
    return FALSE;

  if (state->target_vol->vol_stats.log_mode == DISK_MODE) {
    if (state->target_vol->total_dirs > 0) {
      int current_index = F2ResolveSelectionIndex(panel, state->target_vol,
                                                  state->disp_begin_pos,
                                                  state->cursor_pos);
      if (current_index < 0)
        current_index = F2FindVisibleIndex(panel, state->target_vol, 0, 1);
      if (current_index >= 0) {
        GetPath(state->target_vol->dir_entry_list[current_index].dir_entry,
                new_log_path);
      } else if (getcwd(new_log_path, sizeof(new_log_path)) == NULL) {
        (void)snprintf(new_log_path, sizeof(new_log_path), "%s", ".");
      }
    } else if (getcwd(new_log_path, sizeof(new_log_path)) == NULL) {
      (void)snprintf(new_log_path, sizeof(new_log_path), "%s", ".");
    }
  } else if (getcwd(new_log_path, sizeof(new_log_path)) == NULL) {
    (void)snprintf(new_log_path, sizeof(new_log_path), "%s", ".");
  }

  if (!GetNewLogPath(ctx, panel, new_log_path) && LogDisk(ctx, panel, new_log_path) == 0) {
    ClearHelp(ctx);
    state->target_vol = ctx->active->vol;
    state->disp_begin_pos = panel->disp_begin_pos;
    state->cursor_pos = panel->cursor_pos;
    BuildDirEntryList(ctx, state->target_vol, &dummy);
    F2AnchorSelectionFromPanel(panel, state->target_vol, state->win_height,
                               &state->disp_begin_pos, &state->cursor_pos);
    MapF2Window(ctx);
    F2DisplayTreeAt(ctx, panel, state->target_vol, state->disp_begin_pos,
                    state->cursor_pos);
    *action = ACTION_NONE;
  }

  return TRUE;
}

static BOOL F2HandleAction(ViewContext *ctx, YtreeNovaPanel *panel,
                           F2PickerLoopState *state, YtreeNovaAction *action,
                           char *path) {
  DirEntry *selected;

  if (ctx == NULL || panel == NULL || state == NULL || state->target_vol == NULL ||
      action == NULL || path == NULL)
    return FALSE;

  switch (*action) {
  case ACTION_NONE:
    return TRUE;
  case ACTION_HELP:
    (void)UI_ShowGeneratedContextHelp(ctx, "dialog.f2-picker", NULL, 0);
    MapF2Window(ctx);
    F2DisplayTreeAt(ctx, panel, state->target_vol, state->disp_begin_pos,
                    state->cursor_pos);
    return TRUE;
  case ACTION_ENTER:
    selected = F2CurrentDir(panel, state->target_vol, state->disp_begin_pos,
                            state->cursor_pos);
    if (selected != NULL) {
      GetPath(selected, path);
      state->result = 0;
    }
    return FALSE;
  case ACTION_VOL_PREV:
    return F2CycleVolume(ctx, panel, -1, state);
  case ACTION_VOL_NEXT:
    return F2CycleVolume(ctx, panel, 1, state);
  case ACTION_LOG:
    return F2HandleLog(ctx, panel, state, action);
  case ACTION_TOGGLE_HIDDEN:
    ToggleDotFiles(ctx, panel);
    F2NormalizeSelectionForVisibility(panel, state->target_vol, state->win_height,
                                      &state->disp_begin_pos,
                                      &state->cursor_pos);
    MapF2Window(ctx);
    F2DisplayTreeAt(ctx, panel, state->target_vol, state->disp_begin_pos,
                    state->cursor_pos);
    return TRUE;
  case ACTION_QUIT:
  case ACTION_ESCAPE:
    return FALSE;
  default:
    if (F2MoveSelection(ctx, panel, *action, state))
      return TRUE;
    UI_Beep(ctx, FALSE);
    return TRUE;
  }
}

static void F2RebuildActiveDirEntryList(ViewContext *ctx,
                                        const struct Volume *target_vol) {
  int dummy;

  if (ctx == NULL || ctx->active == NULL || ctx->active->vol == NULL)
    return;

  if (target_vol == ctx->active->vol)
    BuildDirEntryList(ctx, ctx->active->vol, &dummy);
}

int KeyF2Get(ViewContext *ctx, YtreeNovaPanel *panel, char *path) {
  struct Volume *original_vol;
  unsigned int original_panel_generation;
  F2PickerLoopState state;
  int local_disp_begin_pos, local_cursor_pos;
  int selected_index = -1;
  int win_width, win_height;
  YtreeNovaAction action;

  if (ctx == NULL || panel == NULL || path == NULL || ctx->active == NULL ||
      ctx->active->vol == NULL)
    return -1;

  state.result = -1;
  state.disp_begin_pos = panel->disp_begin_pos;
  state.cursor_pos = panel->cursor_pos;
  original_vol = ctx->active->vol;
  original_panel_generation = ctx->active->panel_generation;
  SavePanelTreeViewportSnapshot(ctx->active);
  DEBUG_LOG("ENTER HandleDirWindow: Panel=%s Vol=%s Cursor=%d",
            (panel == ctx->left ? "LEFT" : "RIGHT"),
            (panel->vol ? panel->vol->vol_stats.log_path : "NULL"),
            panel->cursor_pos);
  state.target_vol = F2ResolveTargetVolume(ctx);

  if (state.target_vol == NULL)
    return -1;

  F2EnsureDirEntryList(ctx, state.target_vol);
  GetMaxYX(ctx->ctx_f2_window, &win_height, &win_width);
  state.win_height = win_height;
  F2AnchorSelectionFromPanel(panel, state.target_vol, state.win_height,
                             &state.disp_begin_pos, &state.cursor_pos);
  if (ctx->ctx_dir_window != NULL) {
    werase(ctx->ctx_dir_window);
    RefreshWindow(ctx->ctx_dir_window);
  }
  if (panel->pan_dir_window != NULL && panel->pan_dir_window != ctx->ctx_dir_window) {
    werase(panel->pan_dir_window);
    RefreshWindow(panel->pan_dir_window);
  }
  MapF2Window(ctx);
  F2DisplayTreeAt(ctx, panel, state.target_vol, state.disp_begin_pos,
                  state.cursor_pos);
  do {
    int footer_x = 2;

#ifdef COLOR_SUPPORT
    if (ctx->color_enabled)
      wattrset(ctx->ctx_f2_window, COLOR_PAIR(UI_ROLE_PICKER));
    else
      wattrset(ctx->ctx_f2_window, 0);
#else
    wattrset(ctx->ctx_f2_window, 0);
#endif
    mvwhline(ctx->ctx_f2_window, win_height - 1, 0, ' ', win_width);
    UI_RenderCommandStrip(ctx->ctx_f2_window, win_height - 1, footer_x,
                          f2_command_strip,
                          sizeof(f2_command_strip) / sizeof(f2_command_strip[0]),
                          UI_ROLE_PICKER, UI_ROLE_KEYBIND);
    footer_x += UI_CommandStripVisualLength(
        f2_command_strip, sizeof(f2_command_strip) / sizeof(f2_command_strip[0]));
    footer_x += 2;
    UI_RenderCommandStrip(ctx->ctx_f2_window, win_height - 1, footer_x,
                          f2_context_command_strip,
                          sizeof(f2_context_command_strip) /
                              sizeof(f2_context_command_strip[0]),
                          UI_ROLE_PICKER, UI_ROLE_KEYBIND);

    RefreshWindow(ctx->ctx_f2_window);
    doupdate();
    int ch = Getch(ctx);
    GetMaxYX(ctx->ctx_f2_window, &win_height, &win_width);
    state.win_height = win_height;
    action = GetKeyAction(ctx, ch);
  } while (F2HandleAction(ctx, panel, &state, &action, path));

  local_disp_begin_pos = state.disp_begin_pos;
  local_cursor_pos = state.cursor_pos;
  if (state.target_vol != NULL) {
    selected_index = F2ResolveSelectionIndex(panel, state.target_vol,
                                             local_disp_begin_pos,
                                             local_cursor_pos);
  }

  if (ctx->active->vol != original_vol) {
    if (!AppStateCommitPanelVolume(ctx->active, original_vol))
      return -1;
    if (!AppStateRestorePanelGeneration(ctx->active, original_panel_generation))
      return -1;
    if (!AppStateCommitViewMode(ctx, ctx->active->vol->vol_stats.log_mode))
      return -1;

    if (ctx->active)
      (void)RestorePanelTreeViewportSnapshot(ctx, ctx->active);

    DisplayMenu(ctx);

    if (ctx->ctx_file_window == ctx->ctx_big_file_window) {
      SwitchToBigFileWindow(ctx);
      if (ctx->active->vol->vol_stats.tree->global_flag) {
        DisplayDiskStatistic(ctx, &ctx->active->vol->vol_stats);
      } else {
        UpdateStatsPanel(ctx, GetSelectedDirEntry(ctx, ctx->active->vol),
                         &ctx->active->vol->vol_stats);
      }
    } else {
      SwitchToSmallFileWindow(ctx);
      DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                  panel->disp_begin_pos,
                  panel->disp_begin_pos + panel->cursor_pos, TRUE);
      DisplayDiskStatistic(ctx, &ctx->active->vol->vol_stats);
      UpdateStatsPanel(ctx, GetSelectedDirEntry(ctx, ctx->active->vol),
                       &ctx->active->vol->vol_stats);
    }

    DisplayFileWindow(ctx, ctx->active,
                      GetSelectedDirEntry(ctx, ctx->active->vol));
    RefreshWindow(ctx->ctx_file_window);
    DisplayAvailBytes(ctx, &ctx->active->vol->vol_stats);
  }

  if (panel->vol == state.target_vol && selected_index >= 0) {
    PositionPanelAtIndex(panel, selected_index);
  } else if (panel->vol == state.target_vol) {
    if (!AppStateCommitPanelTreeViewport(panel, local_disp_begin_pos,
                                         local_cursor_pos))
      return -1;
  }

  UnmapF2Window(ctx);
  DEBUG_LOG("EXIT HandleDirWindow: Panel=%s Cursor=%d DispBegin=%d",
            (panel == ctx->left ? "LEFT" : "RIGHT"), panel->cursor_pos,
            panel->disp_begin_pos);

  F2RebuildActiveDirEntryList(ctx, state.target_vol);
  if (ctx->ctx_dir_window != NULL) {
    DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                ctx->active->disp_begin_pos,
                ctx->active->disp_begin_pos + ctx->active->cursor_pos, TRUE);
    RefreshWindow(ctx->ctx_dir_window);
  }
  if (panel->pan_dir_window != NULL && panel->pan_dir_window != ctx->ctx_dir_window) {
    DisplayTree(ctx, ctx->active->vol, panel->pan_dir_window,
                panel->disp_begin_pos,
                panel->disp_begin_pos + panel->cursor_pos, TRUE);
    RefreshWindow(panel->pan_dir_window);
  }

  if (action == ACTION_ESCAPE || action == ACTION_QUIT)
    return -1;

  return state.result;
}
