/***************************************************************************
 *
 * src/ui/volume_menu.c
 * Volume Selection Menu UI
 *
 ***************************************************************************/

#include "ytnova_cmd.h"
#include "ytnova_appstate_actions.h"
#include "ytnova_appstate_panel.h"
#include "ytnova_appstate_render.h"
#include "ytnova_fs.h"
#include "ytnova_ui.h"

static void NormalizePanelCursorForVolume(YtreeNovaPanel *panel) {
  int disp_begin_pos;
  int cursor_pos;

  if (!panel || !panel->vol) {
    return;
  }

  if (panel->vol->total_dirs <= 0) {
    if (!AppStateCommitPanelTreeViewport(panel, 0, 0))
      return;
    if (!AppStateCommitPanelFileAnchor(panel, NULL))
      return;
    return;
  }

  disp_begin_pos = panel->disp_begin_pos;
  cursor_pos = panel->cursor_pos;
  if (disp_begin_pos < 0)
    disp_begin_pos = 0;
  if (cursor_pos < 0)
    cursor_pos = 0;
  if (disp_begin_pos >= panel->vol->total_dirs)
    disp_begin_pos = panel->vol->total_dirs - 1;
  if (disp_begin_pos + cursor_pos >= panel->vol->total_dirs)
    cursor_pos = panel->vol->total_dirs - 1 - disp_begin_pos;
  if (cursor_pos < 0)
    cursor_pos = 0;
  (void)AppStateCommitPanelTreeViewport(panel, disp_begin_pos, cursor_pos);
}

static void EnsurePanelsReferenceActiveVolume(ViewContext *ctx) {
  int idx;

  if (!ctx || !ctx->active || !ctx->active->vol)
    return;

  if (ctx->left && ctx->left->vol == NULL &&
      !AppStateCommitPanelVolume(ctx->left, ctx->active->vol))
    return;
  if (ctx->right && ctx->right->vol == NULL &&
      !AppStateCommitPanelVolume(ctx->right, ctx->active->vol))
    return;

  if (!ctx->is_split_screen)
    return;

  if (ctx->left && ctx->left->vol == ctx->active->vol) {
    BuildDirEntryList(ctx, ctx->left->vol, &ctx->left->current_dir_entry);
    NormalizePanelCursorForVolume(ctx->left);
    idx = ctx->left->disp_begin_pos + ctx->left->cursor_pos;
    if (ctx->left->vol->total_dirs > 0) {
      if (!AppStateCommitPanelFileAnchor(
              ctx->left, ctx->left->vol->dir_entry_list[idx].dir_entry))
        return;
      BuildFileEntryList(ctx, ctx->left);
    }
  }

  if (ctx->right && ctx->right->vol == ctx->active->vol) {
    BuildDirEntryList(ctx, ctx->right->vol, &ctx->right->current_dir_entry);
    NormalizePanelCursorForVolume(ctx->right);
    idx = ctx->right->disp_begin_pos + ctx->right->cursor_pos;
    if (ctx->right->vol->total_dirs > 0) {
      if (!AppStateCommitPanelFileAnchor(
              ctx->right, ctx->right->vol->dir_entry_list[idx].dir_entry))
        return;
      BuildFileEntryList(ctx, ctx->right);
    }
  }

  SyncActivePanelWindows(ctx);
}

/*
 * SelectLoadedVolume
 * Displays a list of currently loaded volumes and allows the user to switch
 * between them. Returns 0 on successful switch, -1 on cancel.
 */
int SelectLoadedVolume(ViewContext *ctx, int *return_key) {
  struct Volume *s, *tmp;
  struct Volume **vol_array = NULL;
  int num_volumes;
  int i, j, ch; /* Added j for visible line iteration */
  int selected_index = 0;
  WINDOW *win = NULL;
  int win_height, win_width, win_x, win_y;
  int result = -1; /* Assume cancel by default */
  char title[] = "Select Volume";
  char prompt[] =
      "Select (Up)/(Down)  Switch (Enter)  (Esc)/(Q)uit  (D)elete";
  BOOL changes_made = FALSE;
  BOOL restart_menu = FALSE;

  /* Scrolling variables */
  int visible_lines;

  if (!AppStateValidatedDispatchSurface("surface.volume-menu-selection"))
    return -1;
  if (!AppStateValidatedDispatchSurface("surface.volume-operation"))
    return -1;
  if (!AppStateValidatedEvent("event.volume-lifecycle"))
    return -1;

  ClearHelp(ctx);

  do {
    int max_path_len;
    int current_volume_index;
    BOOL menu_active;
    int scroll_offset;

    restart_menu = FALSE;
    menu_active = TRUE;

    /* Reset num_volumes and max_path_len for fresh snapshot */
    max_path_len = 0;
    current_volume_index = -1;
    scroll_offset = 0; /* Reset scroll offset for new menu display */

    num_volumes = HASH_COUNT(ctx->volumes_head);

    if (num_volumes == 0) {
      UI_Message(ctx, "No volumes currently loaded.");
      // If we deleted the last volume, GlobalView->active->vol should have been
      // reset to a blank one. In this case, we should return 0 to force a
      // refresh of the main screen. If we started with 0 volumes, this is an
      // error.
      return changes_made ? 0 : -1;
    }

    vol_array =
        (struct Volume **)xmalloc(num_volumes * sizeof(struct Volume *));

    i = 0;
    int new_selected_index = 0; // Default to first item
    HASH_ITER(hh, ctx->volumes_head, s, tmp) {
      if (s == NULL)
        continue;
      vol_array[i] = s;
      int len = StrVisualLength(s->vol_stats.log_path);
      if (len > max_path_len) {
        max_path_len = len;
      }
      if (s == ctx->active->vol) {
        current_volume_index = i;
        new_selected_index = i; // Set selected to current volume
      }
      i++;
    }
    selected_index =
        new_selected_index; // Update selected_index after array is built
    // Ensure selected_index is within bounds (should be if ctx->active->vol is
    // valid)
    if (selected_index >= num_volumes)
      selected_index = num_volumes - 1;
    if (selected_index < 0)
      selected_index = 0;

    /* 2. Window Setup */
    /* Base width calculation */
    /* FIX: Cast strlen result to int for MAXIMUM macro to avoid signed/unsigned
     * warning */
    win_width = MAXIMUM((int)(strlen(title) + 4), max_path_len + 12);

    /* Ensure it covers the prompt */
    win_width = MAXIMUM(win_width, StrVisualLength(prompt) + 4);

    /* Constraint: Fit strictly within the main directory area (left of stats
     * panel) */
    /* ctx->layout.stats_width is 24, STATS_MARGIN is 2, so the main area
     * ends at COLS - 26 */
    win_width = MINIMUM(win_width, COLS - ctx->layout.stats_width - 2);

    win_height =
        MINIMUM(ctx->layout.bottom_border_y,
                num_volumes +
                    5); /* 5 for top/bottom border, title, prompt, empty line */
    win_height = MAXIMUM(win_height, 10); /* Minimum 10 lines */

    /* Center the window relative to the directory area */
    win_x = ((COLS - ctx->layout.stats_width) - win_width) / 2;
    /* Ensure safe X coordinate */
    if (win_x < 1)
      win_x = 1;

    win_y = (LINES - win_height) / 2;

    // Calculate visible lines for items
    visible_lines =
        win_height -
        5; /* Height minus TopBorder, Title, Spacer, Prompt, BottomBorder */
    visible_lines =
        MAXIMUM(1, visible_lines); /* Ensure at least one line is visible */

    // Adjust initial scroll_offset to make current_volume_index visible
    if (selected_index >= visible_lines) {
      scroll_offset = selected_index - visible_lines + 1;
      // Ensure scroll_offset doesn't go past the end of the list
      scroll_offset = MINIMUM(scroll_offset, num_volumes - visible_lines);
    }
    // Ensure scroll_offset is not negative
    scroll_offset = MAXIMUM(0, scroll_offset);

    // Create new window
    win = newwin(win_height, win_width, win_y, win_x);
    if (win == NULL) {
      UI_Error(ctx, __FILE__, __LINE__,
               "Failed to create window for volume selection.");
      free(vol_array);
      return -1;
    }

    UI_Dialog_Push(win, UI_TIER_MODAL);

    keypad(win, TRUE);
    WbkgdSet(ctx, win, COLOR_PAIR(CPAIR_WINHST));
    curs_set(0); /* Hide cursor */

    /* 3. Input Loop */
    while (menu_active) {
      werase(win);
      box(win, 0, 0);
      mvwprintw(win, 1, (win_width - strlen(title)) / 2, "%s", title);
      mvwprintw(win, win_height - 2, (win_width - StrVisualLength(prompt)) / 2,
                "%s", prompt);

      /* Drawing loop using scroll_offset and visible_lines */
      for (j = 0; j < visible_lines; j++) { /* Iterate for visible lines */
        int actual_idx = scroll_offset + j; /* Calculate actual volume index */
        if (actual_idx >= num_volumes) {
          break; /* No more volumes to display */
        }

        int y_pos =
            3 + j; /* Start listing from y=3, relative to visible line j */

        if (actual_idx == selected_index) {
#ifdef COLOR_SUPPORT
          wattron(win, COLOR_PAIR(CPAIR_HIHST));
#else
          wattron(win, A_REVERSE);
#endif
        } else if (actual_idx == current_volume_index) {
#ifdef COLOR_SUPPORT
          wattron(win, COLOR_PAIR(CPAIR_HST) | A_BOLD);
#else
          wattron(win, A_BOLD);
#endif
        }

        const char *path_to_display =
            vol_array[actual_idx]->vol_stats.log_path;
        char display_buf[PATH_LENGTH + 1];

        if (strlen(path_to_display) == 0) {
          path_to_display = "<No Path>";
        }

        /* Cut pathname if too long for the window */
        /* FIX: Subtract 8 for padding to avoid overwriting right border (e.g.
         * "[ ] " + path + border) */
        int max_w = win_width - 8;
        CutPathname(display_buf, path_to_display, max_w);

        mvwprintw(win, y_pos, 2, "[%c] %s",
                  (actual_idx == selected_index ? '*' : ' '), display_buf);

        if (actual_idx == current_volume_index &&
            actual_idx != selected_index) {
#ifdef COLOR_SUPPORT
          wattroff(win, COLOR_PAIR(CPAIR_HST) | A_BOLD);
#else
          wattroff(win, A_BOLD);
#endif
        } else if (actual_idx == selected_index) {
#ifdef COLOR_SUPPORT
          wattroff(win, COLOR_PAIR(CPAIR_HIHST));
#else
          wattroff(win, A_REVERSE);
#endif
        }
      }
      wrefresh(win);

      ch = WGetch(ctx, win);

      if (ctx && ctx->resize_request) {
        (void)AppStateClearResizeRequest(ctx);
        ReCreateWindows(ctx);
        DisplayMenu(ctx);
        DisplayDiskStatistic(ctx, &ctx->active->vol->vol_stats);
        restart_menu = TRUE;
        break;
      }

      switch (ch) {
      case KEY_UP:
        selected_index--;
        if (selected_index < 0) {
          selected_index = num_volumes - 1;
          scroll_offset = MAXIMUM(0, num_volumes - visible_lines);
        } else if (selected_index < scroll_offset) {
          scroll_offset--;
        }
        break;
      case KEY_DOWN:
        selected_index++;
        if (selected_index >= num_volumes) {
          selected_index = 0;
          scroll_offset = 0;
        } else if (selected_index >= scroll_offset + visible_lines) {
          scroll_offset++;
        }
        break;
      case LF:
      case CR:
        result = 0; /* Success */
        restart_menu = FALSE;
        menu_active = FALSE;
        break;
      case ESC:
      case 'q':
        result = -1; /* Cancel */
        restart_menu = FALSE;
        menu_active = FALSE;
        break;
      case 'D':
      case 'd':
      case KEY_DC: // Delete key
        if (return_key) {
          *return_key = ch;
          if (vol_array)
            free(vol_array);
          if (win)
            UI_Dialog_Close(ctx, win);
          curs_set(1);
          return selected_index;
        }

        if (num_volumes <= 1) {
          UI_Message(ctx, "Cannot release the last volume.");
          // No need to redraw, loop will do it.
          break; // break from switch, loop continues to redraw
        }

        if (InputChoice(ctx, "Release this volume? (Y/N)", "YN\033") == 'Y') {
          struct Volume *target_vol = vol_array[selected_index];

          if (target_vol == ctx->active->vol) {
            /* Scenario A: Deleting Current Volume */
            /* Find a neighbor to switch to */
            // If selected is 0, try 1. Otherwise, try 0.
            int neighbor_idx = (selected_index == 0) ? 1 : 0;
            struct Volume *neighbor = vol_array[neighbor_idx];

            /* Verify neighbor accessibility before switching */
            // This logic is similar to  LogDisk, but for a neighbor.
            BOOL neighbor_access_ok = FALSE;
            struct stat neighbor_st_check;

            /* Renamed usage: neighbor->vol_stats.mode ->
             * neighbor->vol_stats.log_mode */
            if (neighbor->vol_stats.log_mode == ARCHIVE_MODE) {
              if (stat(neighbor->vol_stats.log_path, &neighbor_st_check) ==
                      0 &&
                  !S_ISDIR(neighbor_st_check.st_mode)) {
                neighbor_access_ok = TRUE;
                char neighbor_parent_dir[PATH_LENGTH + 1];
                strncpy(neighbor_parent_dir, neighbor->vol_stats.log_path,
                        PATH_LENGTH);
                neighbor_parent_dir[PATH_LENGTH] = '\0';
                char *slash = strrchr(neighbor_parent_dir, FILE_SEPARATOR_CHAR);
                if (slash) {
                  *slash = '\0';
                  if (chdir(neighbor_parent_dir) != 0) {
                    /* Suppress */
                  }
                }
              }
            } else {
              if (chdir(neighbor->vol_stats.log_path) == 0) {
                neighbor_access_ok = TRUE;
              }
            }

            if (!neighbor_access_ok) {
              char error_message_buffer[MESSAGE_LENGTH + 1];
              snprintf(error_message_buffer, sizeof(error_message_buffer),
                       "Neighbor volume \"%s\" not accessible (Error: %s). "
                       "Removed.",
                       neighbor->vol_stats.log_path, strerror(errno));
              UI_Message(ctx, error_message_buffer);
              Volume_Delete(ctx, neighbor); // Delete the inaccessible neighbor
              changes_made = TRUE;

              restart_menu = TRUE;
              menu_active = FALSE;
              break;
            }

            {
              char neighbor_path[PATH_LENGTH + 1];

              (void)snprintf(neighbor_path, sizeof(neighbor_path), "%s",
                             neighbor->vol_stats.log_path);
              if (LogDisk(ctx, ctx->active, neighbor_path) != 0) {
                restart_menu = TRUE;
                menu_active = FALSE;
                break;
              }
            }
          }
          /* Scenario B: Deleting Background Volume (or target_vol is now
           * ctx->active->vol's old self) */
          Volume_Delete(ctx, target_vol);
          changes_made = TRUE;

          /* Cleanup and restart menu */
          restart_menu = TRUE;
          menu_active = FALSE;

          /* Rebuild global list if we just switched or modified
           * ctx->active->vol indirectly */
          {
            int dummy;
            BuildDirEntryList(ctx, ctx->active->vol, &dummy);
          }
        }
        break; // break from switch, loop continues to redraw (if not
               // restart_menu)
      default:
        /* Ignore other keys */
        break;
      }
    }

    /* 5. Cleanup inside loop before potentially restarting */
    if (vol_array) {
      free(vol_array);
      vol_array = NULL;
    }
    if (win) {
      UI_Dialog_Close(ctx, win);
      win = NULL;
    }
    /* touchwin(stdscr);
    refresh(); */

  } while (restart_menu);

  /* 6. Execution (if not cancelled) */
  curs_set(1); /* Restore cursor */

  if (result == 0) {
    /* We need to rebuild vol_array one last time to get the target volume
       pointer because we freed it inside the loop. However, since we just
       exited the loop, we don't have the array anymore. BUT: selected_index
       refers to the index in the *last displayed* list. We need to reconstruct
       the list to map index to pointer again safely.
    */

    num_volumes = HASH_COUNT(ctx->volumes_head);
    if (num_volumes > 0) {
      vol_array =
          (struct Volume **)xmalloc(num_volumes * sizeof(struct Volume *));
      i = 0;
      HASH_ITER(hh, ctx->volumes_head, s, tmp) { vol_array[i++] = s; }

      /* Ensure index is still valid */
      if (selected_index >= num_volumes)
        selected_index = num_volumes - 1;
      if (selected_index < 0)
        selected_index = 0;

      struct Volume *target_vol = vol_array[selected_index];
      if (target_vol != ctx->active->vol) {
        int login_result =
            LogDisk(ctx, ctx->active, target_vol->vol_stats.log_path);
        EnsurePanelsReferenceActiveVolume(ctx);
        free(vol_array);
        return login_result;
      }

      free(vol_array);
      /* Already on selected volume: preserve existing in-memory state. */
      {
        int dummy;
        BuildDirEntryList(ctx, ctx->active->vol, &dummy);
      }
      EnsurePanelsReferenceActiveVolume(ctx);
      return 0;
    }
  }

  // If changes were made (volumes deleted), return 0 to force main loop to
  // refresh. Otherwise, return original result (0 for switch, -1 for cancel).
  if (changes_made) {
    /* Ensure main loop has a valid list if we deleted something but didn't
     * switch via  LogDisk */
    {
      int dummy;
      BuildDirEntryList(ctx, ctx->active->vol, &dummy);
    }
    EnsurePanelsReferenceActiveVolume(ctx);
    return 0;
  }

  return result;
}
