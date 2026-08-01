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
    {UI_COMMAND_LAYOUT_MNEMONIC, "Log", "L", NULL},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, "cycle", "<", ">"}};

typedef struct {
  struct Volume *target_vol;
  int win_height;
  int disp_begin_pos;
  int cursor_pos;
  int result;
} F2PickerLoopState;

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

static DirEntry *F2CurrentDir(struct Volume *target_vol, int disp_begin_pos,
                              int cursor_pos) {
  int index;

  if (!target_vol || !target_vol->dir_entry_list || target_vol->total_dirs <= 0)
    return NULL;

  index = disp_begin_pos + cursor_pos;
  if (index < 0 || index >= target_vol->total_dirs)
    return NULL;

  return target_vol->dir_entry_list[index].dir_entry;
}

static BOOL F2PositionAtIndex(const struct Volume *target_vol, int target_index,
                              int win_height, int *disp_begin_pos,
                              int *cursor_pos) {
  int visible_rows;

  if (!target_vol || !disp_begin_pos || !cursor_pos || target_index < 0 ||
      target_index >= target_vol->total_dirs)
    return FALSE;

  visible_rows = F2VisibleRows(win_height);
  if (*disp_begin_pos < 0)
    *disp_begin_pos = 0;
  if (target_index < *disp_begin_pos) {
    *disp_begin_pos = target_index;
    *cursor_pos = 0;
  } else if (target_index >= *disp_begin_pos + visible_rows) {
    *disp_begin_pos = target_index - visible_rows + 1;
    *cursor_pos = visible_rows - 1;
  } else {
    *cursor_pos = target_index - *disp_begin_pos;
  }

  if (*disp_begin_pos < 0)
    *disp_begin_pos = 0;
  if (*cursor_pos < 0)
    *cursor_pos = 0;
  return TRUE;
}

static BOOL F2PositionAtDir(const struct Volume *target_vol,
                            const DirEntry *target_dir, int win_height,
                            int *disp_begin_pos, int *cursor_pos) {
  int i;

  if (!target_vol || !target_dir || !target_vol->dir_entry_list)
    return FALSE;

  for (i = 0; i < target_vol->total_dirs; ++i) {
    if (target_vol->dir_entry_list[i].dir_entry == target_dir)
      return F2PositionAtIndex(target_vol, i, win_height, disp_begin_pos,
                               cursor_pos);
  }

  return FALSE;
}

static BOOL F2ExpandCurrentDir(ViewContext *ctx, struct Volume *target_vol,
                               int win_height, int *disp_begin_pos,
                               int *cursor_pos) {
  DirEntry *selected;
  char selected_path[PATH_LENGTH + 1];
  int dummy_counter = 0;
  int read_depth = 1;

  if (!ctx || !target_vol)
    return FALSE;

  selected = F2CurrentDir(target_vol, *disp_begin_pos, *cursor_pos);
  if (selected == NULL)
    return FALSE;

  if (!selected->not_scanned && selected->sub_tree != NULL)
    return F2PositionAtDir(target_vol, selected->sub_tree, win_height,
                           disp_begin_pos, cursor_pos);

  if (!selected->unlogged_flag &&
      (selected->sub_tree != NULL || selected->file != NULL)) {
    if (!AppStateCommitDirEntryLoggedState(selected, FALSE,
                                           selected->unlogged_flag))
      return FALSE;
    BuildDirEntryList(ctx, target_vol, &dummy_counter);
    BuildDirEntryList(ctx, target_vol, &dummy_counter);
    return F2PositionAtDir(target_vol, selected, win_height, disp_begin_pos,
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
  return F2PositionAtDir(target_vol, selected, win_height,
                         disp_begin_pos, cursor_pos);
}

static BOOL F2CollapseCurrentDir(ViewContext *ctx, struct Volume *target_vol,
                                 int win_height, int *disp_begin_pos,
                                 int *cursor_pos) {
  DirEntry *selected;
  DirEntry *de_ptr;
  FileEntry *fe_ptr;
  FileEntry *next_fe_ptr;
  int dummy_counter = 0;

  if (!ctx || !target_vol)
    return FALSE;

  selected = F2CurrentDir(target_vol, *disp_begin_pos, *cursor_pos);
  if (selected == NULL)
    return FALSE;

  if (selected->not_scanned || selected->sub_tree == NULL) {
    if (selected->up_tree == NULL)
      return FALSE;
    return F2PositionAtDir(target_vol, selected->up_tree, win_height,
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
  return F2PositionAtDir(target_vol, selected, win_height, disp_begin_pos,
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

static void F2DisplayTreeAt(ViewContext *ctx, struct Volume *target_vol,
                            int disp_begin_pos, int cursor_pos) {
  DisplayTree(ctx, target_vol, ctx->ctx_f2_window, disp_begin_pos,
              disp_begin_pos + cursor_pos, TRUE);
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
  F2ClampViewport(state->target_vol, &state->disp_begin_pos, &state->cursor_pos);
  F2RedrawMainWindows(ctx);
  MapF2Window(ctx);
  F2DisplayTreeAt(ctx, state->target_vol, state->disp_begin_pos,
                  state->cursor_pos);
  return TRUE;
}

static BOOL F2MoveSelection(ViewContext *ctx, YtreeNovaAction action,
                            F2PickerLoopState *state) {
  if (ctx == NULL || state == NULL || state->target_vol == NULL)
    return FALSE;

  switch (action) {
  case ACTION_MOVE_DOWN:
    if (state->disp_begin_pos + state->cursor_pos + 1 >=
        state->target_vol->total_dirs)
      return TRUE;
    if (state->cursor_pos + 1 < state->win_height) {
      PrintDirEntry(ctx, state->target_vol, ctx->ctx_f2_window,
                    state->disp_begin_pos + state->cursor_pos,
                    state->cursor_pos, FALSE, TRUE);
      state->cursor_pos++;
      PrintDirEntry(ctx, state->target_vol, ctx->ctx_f2_window,
                    state->disp_begin_pos + state->cursor_pos,
                    state->cursor_pos, TRUE, TRUE);
      return TRUE;
    }
    state->disp_begin_pos++;
    F2DisplayTreeAt(ctx, state->target_vol, state->disp_begin_pos,
                    state->cursor_pos);
    return TRUE;

  case ACTION_MOVE_UP:
    if (state->disp_begin_pos + state->cursor_pos - 1 < 0)
      return TRUE;
    if (state->cursor_pos - 1 >= 0) {
      PrintDirEntry(ctx, state->target_vol, ctx->ctx_f2_window,
                    state->disp_begin_pos + state->cursor_pos,
                    state->cursor_pos, FALSE, TRUE);
      state->cursor_pos--;
      PrintDirEntry(ctx, state->target_vol, ctx->ctx_f2_window,
                    state->disp_begin_pos + state->cursor_pos,
                    state->cursor_pos, TRUE, TRUE);
      return TRUE;
    }
    state->disp_begin_pos--;
    F2DisplayTreeAt(ctx, state->target_vol, state->disp_begin_pos,
                    state->cursor_pos);
    return TRUE;

  case ACTION_MOVE_RIGHT:
    if (F2ExpandCurrentDir(ctx, state->target_vol, state->win_height,
                           &state->disp_begin_pos, &state->cursor_pos)) {
      F2DisplayTreeAt(ctx, state->target_vol, state->disp_begin_pos,
                      state->cursor_pos);
    } else {
      UI_Beep(ctx, FALSE);
    }
    return TRUE;

  case ACTION_MOVE_LEFT:
    if (F2CollapseCurrentDir(ctx, state->target_vol, state->win_height,
                             &state->disp_begin_pos, &state->cursor_pos)) {
      F2DisplayTreeAt(ctx, state->target_vol, state->disp_begin_pos,
                      state->cursor_pos);
    } else {
      UI_Beep(ctx, FALSE);
    }
    return TRUE;

  case ACTION_PAGE_DOWN:
    if (state->disp_begin_pos + state->cursor_pos >=
        state->target_vol->total_dirs - 1)
      return TRUE;
    if (state->cursor_pos < state->win_height - 1) {
      PrintDirEntry(ctx, state->target_vol, ctx->ctx_f2_window,
                    state->disp_begin_pos + state->cursor_pos,
                    state->cursor_pos, FALSE, TRUE);
      if (state->disp_begin_pos + state->win_height >
          state->target_vol->total_dirs - 1)
        state->cursor_pos =
            state->target_vol->total_dirs - state->disp_begin_pos - 1;
      else
        state->cursor_pos = state->win_height - 1;
      PrintDirEntry(ctx, state->target_vol, ctx->ctx_f2_window,
                    state->disp_begin_pos + state->cursor_pos,
                    state->cursor_pos, TRUE, TRUE);
      return TRUE;
    }
    if (state->disp_begin_pos + state->cursor_pos + state->win_height <
        state->target_vol->total_dirs) {
      state->disp_begin_pos += state->win_height;
      state->cursor_pos = state->win_height - 1;
    } else {
      state->disp_begin_pos = state->target_vol->total_dirs - state->win_height;
      if (state->disp_begin_pos < 0)
        state->disp_begin_pos = 0;
      state->cursor_pos =
          state->target_vol->total_dirs - state->disp_begin_pos - 1;
    }
    F2DisplayTreeAt(ctx, state->target_vol, state->disp_begin_pos,
                    state->cursor_pos);
    return TRUE;

  case ACTION_PAGE_UP:
    if (state->disp_begin_pos + state->cursor_pos <= 0)
      return TRUE;
    if (state->cursor_pos > 0) {
      PrintDirEntry(ctx, state->target_vol, ctx->ctx_f2_window,
                    state->disp_begin_pos + state->cursor_pos,
                    state->cursor_pos, FALSE, TRUE);
      state->cursor_pos = 0;
      PrintDirEntry(ctx, state->target_vol, ctx->ctx_f2_window,
                    state->disp_begin_pos + state->cursor_pos,
                    state->cursor_pos, TRUE, TRUE);
      return TRUE;
    }
    state->disp_begin_pos -= state->win_height;
    if (state->disp_begin_pos < 0)
      state->disp_begin_pos = 0;
    state->cursor_pos = 0;
    F2DisplayTreeAt(ctx, state->target_vol, state->disp_begin_pos,
                    state->cursor_pos);
    return TRUE;

  case ACTION_HOME:
    if (state->disp_begin_pos == 0 && state->cursor_pos == 0)
      return TRUE;
    state->disp_begin_pos = 0;
    state->cursor_pos = 0;
    F2DisplayTreeAt(ctx, state->target_vol, state->disp_begin_pos,
                    state->cursor_pos);
    return TRUE;

  case ACTION_END:
    state->disp_begin_pos =
        MAXIMUM(0, state->target_vol->total_dirs - state->win_height);
    state->cursor_pos =
        state->target_vol->total_dirs - state->disp_begin_pos - 1;
    F2DisplayTreeAt(ctx, state->target_vol, state->disp_begin_pos,
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
      GetPath(state->target_vol
                  ->dir_entry_list[state->disp_begin_pos + state->cursor_pos]
                  .dir_entry,
              new_log_path);
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
    F2ClampViewport(state->target_vol, &state->disp_begin_pos,
                    &state->cursor_pos);
    MapF2Window(ctx);
    F2DisplayTreeAt(ctx, state->target_vol, state->disp_begin_pos,
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
    F2DisplayTreeAt(ctx, state->target_vol, state->disp_begin_pos,
                    state->cursor_pos);
    return TRUE;
  case ACTION_ENTER:
    selected = F2CurrentDir(state->target_vol, state->disp_begin_pos,
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
  case ACTION_QUIT:
  case ACTION_ESCAPE:
    return FALSE;
  default:
    if (F2MoveSelection(ctx, *action, state))
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
  F2ClampViewport(state.target_vol, &state.disp_begin_pos, &state.cursor_pos);
  GetMaxYX(ctx->ctx_f2_window, &win_height, &win_width);
  state.win_height = win_height;
  MapF2Window(ctx);
  F2DisplayTreeAt(ctx, state.target_vol, state.disp_begin_pos,
                  state.cursor_pos);
  do {
#ifdef COLOR_SUPPORT
    if (ctx->color_enabled)
      wattrset(ctx->ctx_f2_window, COLOR_PAIR(UI_ROLE_PICKER));
    else
      wattrset(ctx->ctx_f2_window, 0);
#else
    wattrset(ctx->ctx_f2_window, 0);
#endif
    mvwhline(ctx->ctx_f2_window, win_height - 1, 0, ' ', win_width);
    UI_RenderCommandStrip(
        ctx->ctx_f2_window, win_height - 1, 2, f2_command_strip,
        sizeof(f2_command_strip) / sizeof(f2_command_strip[0]), UI_ROLE_PICKER,
        UI_ROLE_KEYBIND);

    RefreshWindow(ctx->ctx_f2_window);
    doupdate();
    int ch = Getch(ctx);
    GetMaxYX(ctx->ctx_f2_window, &win_height, &win_width);
    state.win_height = win_height;
    action = GetKeyAction(ctx, ch);
  } while (F2HandleAction(ctx, panel, &state, &action, path));

  local_disp_begin_pos = state.disp_begin_pos;
  local_cursor_pos = state.cursor_pos;

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

  if (!AppStateCommitPanelTreeViewport(panel, local_disp_begin_pos,
                                       local_cursor_pos))
    return -1;

  UnmapF2Window(ctx);
  DEBUG_LOG("EXIT HandleDirWindow: Panel=%s Cursor=%d DispBegin=%d",
            (panel == ctx->left ? "LEFT" : "RIGHT"), panel->cursor_pos,
            panel->disp_begin_pos);

  F2RebuildActiveDirEntryList(ctx, state.target_vol);

  if (action == ACTION_ESCAPE || action == ACTION_QUIT)
    return -1;

  return state.result;
}
