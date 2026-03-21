/***************************************************************************
 *
 * src/ui/f2_picker.c
 * F2 Directory Picker (Independent Event Loop)
 *
 ***************************************************************************/

#define NO_YTREE_MACROS
#include "ytree.h"
#include "ytree_ui.h"

int KeyF2Get(ViewContext *ctx, YtreePanel *panel, char *path) {
  struct Volume *original_vol; /* Declare first */
  int result = -1;
  int win_width, win_height;
  struct Volume *target_vol;

  if (ctx == NULL || panel == NULL || path == NULL || ctx->active == NULL ||
      ctx->active->vol == NULL) {
    return -1;
  }

  int local_disp_begin_pos = panel->disp_begin_pos;
  int local_cursor_pos = panel->cursor_pos;
  YtreeAction action; /* Declare YtreeAction variable */
  char new_login_path[PATH_LENGTH + 1];

  original_vol = ctx->active->vol;
  DEBUG_LOG("ENTER HandleDirWindow: Panel=%s Vol=%s Cursor=%d",
            (panel == ctx->left ? "LEFT" : "RIGHT"),
            (panel->vol ? panel->vol->vol_stats.login_path : "NULL"),
            panel->cursor_pos);

  if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
    /* Search for a volume that is in DISK_MODE */
    struct Volume *v, *tmp;
    struct Volume *disk_vol = NULL;

    HASH_ITER(hh, ctx->volumes_head, v, tmp) {
      /* Renamed usage: v->vol_stats.mode -> v->vol_stats.login_mode */
      if (v->vol_stats.login_mode == DISK_MODE) {
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
    /* Footer Drawing */
    wattron(ctx->ctx_f2_window, A_BOLD);
    mvwaddstr(ctx->ctx_f2_window, win_height - 1, 2, "[ (L)og (< >) Cycle ]");
    wattroff(ctx->ctx_f2_window, A_BOLD);

    RefreshWindow(ctx->ctx_f2_window);
    doupdate();
    int ch = Getch(ctx);
    GetMaxYX(ctx->ctx_f2_window, &win_height,
             &win_width); /* Maybe changed... */
    /* LF to CR normalization is now handled by GetKeyAction */

    action = GetKeyAction(ctx, ch); /* Translate raw input to YtreeAction */

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
    case ACTION_LOGIN:
      if (target_vol && target_vol->vol_stats.login_mode == DISK_MODE) {
        /* Try to use the path of the currently selected directory in F2
         * window
         */
        if (target_vol->total_dirs > 0) {
          GetPath(target_vol
                      ->dir_entry_list[local_disp_begin_pos + local_cursor_pos]
                      .dir_entry,
                  new_login_path);
        } else {
          if (getcwd(new_login_path, sizeof(new_login_path)) == NULL)
            strcpy(new_login_path, ".");
        }
      } else {
        if (getcwd(new_login_path, sizeof(new_login_path)) == NULL)
          strcpy(new_login_path, ".");
      }

      if (!GetNewLoginPath(ctx, panel, new_login_path)) {
        if (LogDisk(ctx, panel, new_login_path) == 0) {
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
           action != ACTION_ESCAPE && action != ACTION_LOGIN &&
           action != ACTION_LOG);

  /* Added restoration block */
  if (ctx->active->vol != original_vol) {
    /* 1. Restore Global Volume Context */
    ctx->active->vol = original_vol;
    ctx->view_mode = ctx->active->vol->vol_stats.login_mode;

    /* 2. Restore ctx->active state from the restored volume */
    if (ctx->active) {
      /* Panel isolation: No vol_stats sync */
      ctx->active->disp_begin_pos = ctx->active->vol->id /* legacy removed */;
    }

    /* 3. Restore UI Layout */
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

    /* 4. Refresh File Content & Footer Background */
    DisplayFileWindow(ctx, ctx->active,
                      GetSelectedDirEntry(ctx, ctx->active->vol));
    RefreshWindow(ctx->ctx_file_window);

    DisplayAvailBytes(ctx, &ctx->active->vol->vol_stats);
  }

  panel->cursor_pos = local_cursor_pos;
  panel->disp_begin_pos = local_disp_begin_pos;

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
