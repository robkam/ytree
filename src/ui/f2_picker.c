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

int KeyF2Get(ViewContext *ctx, YtreeNovaPanel *panel, char *path) {
  struct Volume *original_vol; /* Declare first */
  unsigned int original_panel_generation;
  int result = -1;
  int win_width, win_height;
  struct Volume *target_vol;

  if (ctx == NULL || panel == NULL || path == NULL || ctx->active == NULL ||
      ctx->active->vol == NULL) {
    return -1;
  }

  int local_disp_begin_pos = panel->disp_begin_pos;
  int local_cursor_pos = panel->cursor_pos;
  YtreeNovaAction action; /* Declare YtreeNovaAction variable */
  char new_log_path[PATH_LENGTH + 1];

  original_vol = ctx->active->vol;
  original_panel_generation = ctx->active->panel_generation;
  SavePanelTreeViewportSnapshot(ctx->active);
  DEBUG_LOG("ENTER HandleDirWindow: Panel=%s Vol=%s Cursor=%d",
            (panel == ctx->left ? "LEFT" : "RIGHT"),
            (panel->vol ? panel->vol->vol_stats.log_path : "NULL"),
            panel->cursor_pos);

  if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
    /* Search for a volume that is in DISK_MODE */
    struct Volume *v, *tmp;
    struct Volume *disk_vol = NULL;

    HASH_ITER(hh, ctx->volumes_head, v, tmp) {
      /* Renamed usage: v->vol_stats.mode -> v->vol_stats.log_mode */
      if (v->vol_stats.log_mode == DISK_MODE) {
        disk_vol = v;
        break;
      }
    }

    if (disk_vol) {
      target_vol = disk_vol;
    } else {
      target_vol = ctx->active->vol;
    }
  } else {
    target_vol = ctx->active->vol;
  }

  if (target_vol == NULL)
    return -1;

  /* Only rebuild if list is missing. Rebuilding invalidates pointers held by
   * callers! */
  if (target_vol->dir_entry_list == NULL) {
    int dummy_counter;
    BuildDirEntryList(ctx, target_vol, &dummy_counter);
  }

  /* Safety bounds check */
  if (local_disp_begin_pos < 0)
    local_disp_begin_pos = 0;
  if (local_cursor_pos < 0)
    local_cursor_pos = 0;
  if (target_vol->total_dirs > 0 &&
      (local_disp_begin_pos + local_cursor_pos >= target_vol->total_dirs)) {
    local_disp_begin_pos = 0;
    local_cursor_pos = 0;
  }

  GetMaxYX(ctx->ctx_f2_window, &win_height, &win_width);
  MapF2Window(ctx);
  DisplayTree(ctx, target_vol, ctx->ctx_f2_window, local_disp_begin_pos,
              local_disp_begin_pos + local_cursor_pos, TRUE);
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
    GetMaxYX(ctx->ctx_f2_window, &win_height,
             &win_width); /* Maybe changed... */
    /* LF to CR normalization is now handled by GetKeyAction */

    action = GetKeyAction(ctx, ch); /* Translate raw input to YtreeNovaAction */

    switch (action) {
    case ACTION_NONE:
      break; /* -1 or unhandled keys, no beep in F2Get */
    case ACTION_MOVE_DOWN:
      if (local_disp_begin_pos + local_cursor_pos + 1 >=
          target_vol->total_dirs) {
      } else {
        if (local_cursor_pos + 1 < win_height) {
          PrintDirEntry(ctx, target_vol, ctx->ctx_f2_window,
                        local_disp_begin_pos + local_cursor_pos,
                        local_cursor_pos, FALSE, TRUE);
          local_cursor_pos++;

          PrintDirEntry(ctx, target_vol, ctx->ctx_f2_window,
                        local_disp_begin_pos + local_cursor_pos,
                        local_cursor_pos, TRUE, TRUE);
        } else {
          local_disp_begin_pos++;
          DisplayTree(ctx, target_vol, ctx->ctx_f2_window, local_disp_begin_pos,
                      local_disp_begin_pos + local_cursor_pos, TRUE);
        }
      }
      break;

    case ACTION_MOVE_UP:
      if (local_disp_begin_pos + local_cursor_pos - 1 < 0) {
      } else {
        if (local_cursor_pos - 1 >= 0) {
          PrintDirEntry(ctx, target_vol, ctx->ctx_f2_window,
                        local_disp_begin_pos + local_cursor_pos,
                        local_cursor_pos, FALSE, TRUE);
          local_cursor_pos--;
          PrintDirEntry(ctx, target_vol, ctx->ctx_f2_window,
                        local_disp_begin_pos + local_cursor_pos,
                        local_cursor_pos, TRUE, TRUE);
        }

        else {
          local_disp_begin_pos--;
          DisplayTree(ctx, target_vol, ctx->ctx_f2_window, local_disp_begin_pos,
                      local_disp_begin_pos + local_cursor_pos, TRUE);
        }
      }
      break;

    case ACTION_MOVE_RIGHT:
      if (F2ExpandCurrentDir(ctx, target_vol, win_height, &local_disp_begin_pos,
                             &local_cursor_pos)) {
        DisplayTree(ctx, target_vol, ctx->ctx_f2_window, local_disp_begin_pos,
                    local_disp_begin_pos + local_cursor_pos, TRUE);
      } else {
        UI_Beep(ctx, FALSE);
      }
      break;

    case ACTION_MOVE_LEFT:
      if (F2CollapseCurrentDir(ctx, target_vol, win_height,
                               &local_disp_begin_pos, &local_cursor_pos)) {
        DisplayTree(ctx, target_vol, ctx->ctx_f2_window, local_disp_begin_pos,
                    local_disp_begin_pos + local_cursor_pos, TRUE);
      } else {
        UI_Beep(ctx, FALSE);
      }
      break;

    case ACTION_PAGE_DOWN:
      if (local_disp_begin_pos + local_cursor_pos >=
          target_vol->total_dirs - 1) {
      } else {
        if (local_cursor_pos < win_height - 1) {
          PrintDirEntry(ctx, target_vol, ctx->ctx_f2_window,
                        local_disp_begin_pos + local_cursor_pos,
                        local_cursor_pos, FALSE, TRUE);
          if (local_disp_begin_pos + win_height > target_vol->total_dirs - 1)
            local_cursor_pos =
                target_vol->total_dirs - local_disp_begin_pos - 1;
          else
            local_cursor_pos = win_height - 1;
          PrintDirEntry(ctx, target_vol, ctx->ctx_f2_window,
                        local_disp_begin_pos + local_cursor_pos,
                        local_cursor_pos, TRUE, TRUE);
        } else {
          if (local_disp_begin_pos + local_cursor_pos + win_height <
              target_vol->total_dirs) {
            local_disp_begin_pos += win_height;
            local_cursor_pos = win_height - 1;
          } else {
            local_disp_begin_pos = target_vol->total_dirs - win_height;
            if (local_disp_begin_pos < 0)
              local_disp_begin_pos = 0;
            local_cursor_pos =
                target_vol->total_dirs - local_disp_begin_pos - 1;
          }
          DisplayTree(ctx, target_vol, ctx->ctx_f2_window, local_disp_begin_pos,
                      local_disp_begin_pos + local_cursor_pos, TRUE);
        }
      }
      break;

    case ACTION_PAGE_UP:
      if (local_disp_begin_pos + local_cursor_pos <= 0) {
      } else {
        if (local_cursor_pos > 0) {
          PrintDirEntry(ctx, target_vol, ctx->ctx_f2_window,
                        local_disp_begin_pos + local_cursor_pos,
                        local_cursor_pos, FALSE, TRUE);
          local_cursor_pos = 0;
          PrintDirEntry(ctx, target_vol, ctx->ctx_f2_window,
                        local_disp_begin_pos + local_cursor_pos,
                        local_cursor_pos, TRUE, TRUE);
        } else {
          if ((local_disp_begin_pos -= win_height) < 0) {
            local_disp_begin_pos = 0;
          }
          local_cursor_pos = 0;
          DisplayTree(ctx, target_vol, ctx->ctx_f2_window, local_disp_begin_pos,
                      local_disp_begin_pos + local_cursor_pos, TRUE);
        }
      }
      break;

    case ACTION_HOME:
      if (local_disp_begin_pos == 0 && local_cursor_pos == 0) {
      } else {
        local_disp_begin_pos = 0;
        local_cursor_pos = 0;
        DisplayTree(ctx, target_vol, ctx->ctx_f2_window, local_disp_begin_pos,
                    local_disp_begin_pos + local_cursor_pos, TRUE);
      }
      break;

    case ACTION_END:
      local_disp_begin_pos = MAXIMUM(0, target_vol->total_dirs - win_height);
      local_cursor_pos = target_vol->total_dirs - local_disp_begin_pos - 1;
      DisplayTree(ctx, target_vol, ctx->ctx_f2_window, local_disp_begin_pos,
                  local_disp_begin_pos + local_cursor_pos, TRUE);
      break;

    case ACTION_ENTER:
      GetPath(
          target_vol->dir_entry_list[local_cursor_pos + local_disp_begin_pos]
              .dir_entry,
          path);
      result = 0;
      break;

    case ACTION_VOL_PREV:
      if (CycleLoadedVolume(ctx, panel, -1) == 0) {
        target_vol = ctx->active->vol;
        local_disp_begin_pos = panel->disp_begin_pos;
        local_cursor_pos = panel->cursor_pos;
        {
          int dummy;
          BuildDirEntryList(ctx, target_vol, &dummy);
        }
        if (target_vol->total_dirs > 0 &&
            (local_disp_begin_pos + local_cursor_pos >=
             target_vol->total_dirs)) {
          local_disp_begin_pos = 0;
          local_cursor_pos = 0;
        }
        /* Fix blank screen bug: redraw main UI before F2 window */
        DisplayMenu(ctx);
        if (ctx->active && ctx->active->vol) {
          DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                      ctx->active->disp_begin_pos,
                      ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                      TRUE);
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

        MapF2Window(ctx);
        DisplayTree(ctx, target_vol, ctx->ctx_f2_window, local_disp_begin_pos,
                    local_disp_begin_pos + local_cursor_pos, TRUE);
      }
      break;

    case ACTION_VOL_NEXT:
      if (CycleLoadedVolume(ctx, panel, 1) == 0) {
        target_vol = ctx->active->vol;
        local_disp_begin_pos = panel->disp_begin_pos;
        local_cursor_pos = panel->cursor_pos;
        {
          int dummy;
          BuildDirEntryList(ctx, target_vol, &dummy);
        }
        if (target_vol->total_dirs > 0 &&
            (local_disp_begin_pos + local_cursor_pos >=
             target_vol->total_dirs)) {
          local_disp_begin_pos = 0;
          local_cursor_pos = 0;
        }
        /* Fix blank screen bug: redraw main UI before F2 window */
        DisplayMenu(ctx);
        if (ctx->active && ctx->active->vol) {
          DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                      ctx->active->disp_begin_pos,
                      ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                      TRUE);
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

        MapF2Window(ctx);
        DisplayTree(ctx, target_vol, ctx->ctx_f2_window, local_disp_begin_pos,
                    local_disp_begin_pos + local_cursor_pos, TRUE);
      }
      break;

    case ACTION_LOG:
      if (target_vol && target_vol->vol_stats.log_mode == DISK_MODE) {
        /* Try to use the path of the currently selected directory in F2
         * window
         */
        if (target_vol->total_dirs > 0) {
          GetPath(target_vol
                      ->dir_entry_list[local_disp_begin_pos + local_cursor_pos]
                      .dir_entry,
                  new_log_path);
        } else {
          if (getcwd(new_log_path, sizeof(new_log_path)) == NULL)
            (void)snprintf(new_log_path, sizeof(new_log_path), "%s", ".");
        }
      } else {
        if (getcwd(new_log_path, sizeof(new_log_path)) == NULL)
          (void)snprintf(new_log_path, sizeof(new_log_path), "%s", ".");
      }

      if (!GetNewLogPath(ctx, panel, new_log_path)) {
        if (LogDisk(ctx, panel, new_log_path) == 0) {
          ClearHelp(ctx); /* ADDED */
          target_vol = ctx->active->vol;
          local_disp_begin_pos = panel->disp_begin_pos;
          local_cursor_pos = panel->cursor_pos;

          {
            int dummy;
            BuildDirEntryList(ctx, target_vol, &dummy);
          }

          if (target_vol->total_dirs > 0 &&
              (local_disp_begin_pos + local_cursor_pos >=
               target_vol->total_dirs)) {
            local_disp_begin_pos = 0;
            local_cursor_pos = 0;
          }

          MapF2Window(ctx);

          DisplayTree(ctx, target_vol, ctx->ctx_f2_window, local_disp_begin_pos,
                      local_disp_begin_pos + local_cursor_pos, TRUE);

          action = ACTION_NONE;
        }
      }
      break;

    case ACTION_QUIT:
      break;
    case ACTION_ESCAPE:
      break;

    default:
      UI_Beep(ctx, FALSE);
      break;
    } /* switch */
  } while (action != ACTION_QUIT && action != ACTION_ENTER &&
           action != ACTION_ESCAPE &&
           action != ACTION_LOG);

  if (ctx->active->vol != original_vol) {
    if (!AppStateCommitPanelVolume(ctx->active, original_vol))
      return -1;
    if (!AppStateRestorePanelGeneration(ctx->active, original_panel_generation))
      return -1;
    if (!AppStateCommitViewMode(ctx, ctx->active->vol->vol_stats.log_mode))
      return -1;

    if (ctx->active)
      (void)RestorePanelTreeViewportSnapshot(ctx, ctx->active);

    DisplayMenu(ctx); /* Restores Frame and Header */

    /* Check which view mode we were in before F2 */
    if (ctx->ctx_file_window == ctx->ctx_big_file_window) {
      /* Restore Big Window Mode */
      SwitchToBigFileWindow(ctx);
      /* In Big Mode, we don't draw the tree. We draw global stats if global.
       */
      if (ctx->active->vol->vol_stats.tree->global_flag) {
        DisplayDiskStatistic(ctx, &ctx->active->vol->vol_stats);
      } else {
        /* If regular big window, shows Dir Stats */
        UpdateStatsPanel(ctx, GetSelectedDirEntry(ctx, ctx->active->vol),
                         &ctx->active->vol->vol_stats);
      }
    } else {
      /* Restore Standard Split Mode */
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

  /* Restore the original directory list for the main window if we switched
   * volume context */
  /* Actually, BuildDirEntryList now writes to vol structure, so other volume
  caches are preserved. We only need to ensure the ctx->active->vol is
  consistent for the main window logic, which is untouched here. */
  if (target_vol != ctx->active->vol) {
    /* If we messed with another volume's list, that's fine, it's
     * encapsulated.
     */
  } else {
    /* We modified current volume's list. Ensure it's valid. */
    {
      int dummy;
      BuildDirEntryList(ctx, ctx->active->vol, &dummy);
    }
  }

  if (action == ACTION_ESCAPE || action == ACTION_QUIT)
    return -1;

  return (result);
}
