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

static const UICommandStripCommand volume_command_strip[] = {
    {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("volume-menu.commands", "help"), "F1",
     NULL, "volume-menu.commands"},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("volume-menu.commands", "release"), "D",
     NULL, "volume-menu.commands"},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("volume-menu.commands", "switch"),
     "Enter", NULL, "volume-menu.commands"},
    {UI_COMMAND_LAYOUT_KEY_PREFIX, NP_("volume-menu.commands", "cancel"),
     "Esc", NULL, "volume-menu.commands"}};

enum {
  VOLUME_MENU_TITLE_PADDING = 4,
  VOLUME_MENU_PATH_PADDING = 12,
  VOLUME_MENU_NON_ITEM_ROWS = 5,
  VOLUME_MENU_ITEM_START_ROW = 3,
  VOLUME_MENU_COMMAND_STRIP_X = 2,
  VOLUME_MENU_ITEM_PATH_PADDING = 8
};

typedef struct {
  struct Volume **vol_array;
  int num_volumes;
  int selected_index;
  int max_path_len;
} VolumeMenuSnapshot;

typedef struct {
  int win_width;
  int win_height;
  int win_x;
  int win_y;
  int visible_lines;
  int scroll_offset;
} VolumeMenuLayout;

static int ShowVolumeHelpPopup(ViewContext *ctx) {
  if (ctx == NULL)
    return -1;

  return UI_ShowGeneratedContextHelp(ctx, "dialog.volume-menu", NULL, 0);
}

static void PaintVolumeRow(const ViewContext *ctx, WINDOW *win, int y_pos,
                           int win_width, char *item_text, BOOL selected,
                           BOOL active) {
  chtype base_attr;
  chtype item_attr;

#ifdef COLOR_SUPPORT
  if (ctx->color_enabled) {
    base_attr = COLOR_PAIR(UI_ROLE_PICKER);
    item_attr =
        selected ? UISelectionAttrForBase(ctx, UI_ROLE_PICKER) : base_attr;
  } else {
    base_attr = 0;
    item_attr = selected ? A_REVERSE : (active ? A_BOLD : 0);
  }
#else
  (void)ctx;
  base_attr = 0;
  item_attr = selected ? A_REVERSE : (active ? A_BOLD : 0);
#endif

  wattrset(win, base_attr);
  mvwhline(win, y_pos, 1, ' ', win_width - 2);
  wmove(win, y_pos, 2);
  wattrset(win, item_attr);
  WAddStr(win, item_text);
  wattrset(win, base_attr);
}

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

static int VolumeMenuPromptWidth(void) {
  return UI_CommandStripVisualLength(volume_command_strip,
                                     sizeof(volume_command_strip) /
                                         sizeof(volume_command_strip[0]));
}

static int BuildVolumeMenuSnapshot(ViewContext *ctx,
                                   VolumeMenuSnapshot *snapshot) {
  struct Volume *s;
  struct Volume *tmp;
  struct Volume **vol_array;
  int num_volumes;
  int i;
  int selected_index = 0;
  int max_path_len = 0;

  if (!ctx || !snapshot)
    return -1;

  num_volumes = HASH_COUNT(ctx->volumes_head);
  snapshot->num_volumes = num_volumes;
  if (num_volumes <= 0) {
    snapshot->vol_array = NULL;
    snapshot->selected_index = 0;
    snapshot->max_path_len = 0;
    return 0;
  }

  vol_array = (struct Volume **)xmalloc(num_volumes * sizeof(struct Volume *));
  i = 0;
  HASH_ITER(hh, ctx->volumes_head, s, tmp) {
    int len;

    if (s == NULL)
      continue;
    vol_array[i] = s;
    len = StrVisualLength(s->vol_stats.log_path);
    if (len > max_path_len)
      max_path_len = len;
    if (s == ctx->active->vol)
      selected_index = i;
    i++;
  }

  if (selected_index >= num_volumes)
    selected_index = num_volumes - 1;
  if (selected_index < 0)
    selected_index = 0;

  snapshot->vol_array = vol_array;
  snapshot->selected_index = selected_index;
  snapshot->max_path_len = max_path_len;
  return 0;
}

static void ComputeVolumeMenuLayout(const ViewContext *ctx, const char *title,
                                    int prompt_width,
                                    const VolumeMenuSnapshot *snapshot,
                                    VolumeMenuLayout *layout) {
  int win_width;
  int win_height;
  int win_x;
  int win_y;
  int visible_lines;
  int scroll_offset = 0;

  if (!ctx || !title || !snapshot || !layout)
    return;

  win_width = MAXIMUM((int)(strlen(title) + VOLUME_MENU_TITLE_PADDING),
                      snapshot->max_path_len + VOLUME_MENU_PATH_PADDING);
  win_width = MAXIMUM(win_width, prompt_width + 4);
  win_width = MINIMUM(win_width, COLS - ctx->layout.stats_width - 2);

  win_height = MINIMUM(ctx->layout.bottom_border_y,
                       snapshot->num_volumes + VOLUME_MENU_NON_ITEM_ROWS);
  win_height = MAXIMUM(win_height, 10);

  win_x = ((COLS - ctx->layout.stats_width) - win_width) / 2;
  if (win_x < 1)
    win_x = 1;
  win_y = (LINES - win_height) / 2;

  visible_lines = win_height - VOLUME_MENU_NON_ITEM_ROWS;
  visible_lines = MAXIMUM(1, visible_lines);
  if (snapshot->selected_index >= visible_lines) {
    scroll_offset = snapshot->selected_index - visible_lines + 1;
    scroll_offset = MINIMUM(scroll_offset, snapshot->num_volumes - visible_lines);
  }
  scroll_offset = MAXIMUM(0, scroll_offset);

  layout->win_width = win_width;
  layout->win_height = win_height;
  layout->win_x = win_x;
  layout->win_y = win_y;
  layout->visible_lines = visible_lines;
  layout->scroll_offset = scroll_offset;
}

static void RefreshActiveVolumeSelection(ViewContext *ctx) {
  int dummy;

  if (!ctx || !ctx->active || !ctx->active->vol)
    return;

  BuildDirEntryList(ctx, ctx->active->vol, &dummy);
  EnsurePanelsReferenceActiveVolume(ctx);
}

/*
 * SelectLoadedVolume
 * Displays a list of currently loaded volumes and allows the user to switch
 * between them. Returns 0 on successful switch, -1 on cancel.
 */
int SelectLoadedVolume(ViewContext *ctx, int *return_key) {
  VolumeMenuSnapshot snapshot;
  VolumeMenuLayout layout;
  int ch;
  int prompt_width;
  WINDOW *win = NULL;
  int result = -1; /* Assume cancel by default */
  const char title[] = "Select Volume";
  BOOL changes_made = FALSE;
  BOOL restart_menu = FALSE;

  if (ctx == NULL)
    return -1;

  if (!AppStateValidatedDispatchSurface("surface.volume-menu-selection"))
    return -1;
  if (!AppStateValidatedDispatchSurface("surface.volume-operation"))
    return -1;
  if (!AppStateValidatedEvent("event.volume-lifecycle"))
    return -1;

  ClearHelp(ctx);
  memset(&snapshot, 0, sizeof(snapshot));
  memset(&layout, 0, sizeof(layout));

  do {
    BOOL menu_active;

    restart_menu = FALSE;
    menu_active = TRUE;
    prompt_width = VolumeMenuPromptWidth();

    if (BuildVolumeMenuSnapshot(ctx, &snapshot) != 0)
      return -1;

    if (snapshot.num_volumes == 0) {
      UI_Message(ctx, "No volumes currently loaded.");
      // If we deleted the last volume, GlobalView->active->vol should have been
      // reset to a blank one. In this case, we should return 0 to force a
      // refresh of the main screen. If we started with 0 volumes, this is an
      // error.
      return changes_made ? 0 : -1;
    }

    ComputeVolumeMenuLayout(ctx, title, prompt_width, &snapshot, &layout);
    win = newwin(layout.win_height, layout.win_width, layout.win_y, layout.win_x);
    if (win == NULL) {
      UI_Error(ctx, __FILE__, __LINE__,
               "Failed to create window for volume selection.");
      free(snapshot.vol_array);
      return -1;
    }

    UI_Dialog_Push(win, UI_TIER_MODAL);

    keypad(win, TRUE);
    WbkgdSet(ctx, win, COLOR_PAIR(UI_ROLE_PICKER));
    curs_set(0); /* Hide cursor */

    while (menu_active) {
      int j;

      werase(win);
#ifdef COLOR_SUPPORT
      wattron(win, COLOR_PAIR(UI_ROLE_PICKER));
#endif
      box(win, 0, 0);
#ifdef COLOR_SUPPORT
      wattroff(win, COLOR_PAIR(UI_ROLE_PICKER));
#endif
      mvwprintw(win, 1, (layout.win_width - (int)strlen(title)) / 2, "%s",
                title);
      UI_RenderCommandStrip(
          win, layout.win_height - 2, VOLUME_MENU_COMMAND_STRIP_X,
          volume_command_strip,
          sizeof(volume_command_strip) / sizeof(volume_command_strip[0]),
          UI_ROLE_PICKER, UI_ROLE_KEYBIND);

      for (j = 0; j < layout.visible_lines; j++) {
        int actual_idx = layout.scroll_offset + j;
        int y_pos = VOLUME_MENU_ITEM_START_ROW + j;
        const char *path_to_display;
        char display_buf[PATH_LENGTH + 1];
        char item_buf[PATH_LENGTH + 8];
        int max_w;

        if (actual_idx >= snapshot.num_volumes)
          break;

        path_to_display = snapshot.vol_array[actual_idx]->vol_stats.log_path;
        if (strlen(path_to_display) == 0)
          path_to_display = "<No Path>";

        max_w = layout.win_width - VOLUME_MENU_ITEM_PATH_PADDING;
        CutPathname(display_buf, path_to_display, max_w);
        (void)snprintf(item_buf, sizeof(item_buf), "[%c] %s",
                       (actual_idx == snapshot.selected_index ? '*' : ' '),
                       display_buf);
        PaintVolumeRow(ctx, win, y_pos, layout.win_width, item_buf,
                       actual_idx == snapshot.selected_index,
                       snapshot.vol_array[actual_idx] == ctx->active->vol);
      }
      wrefresh(win);

      ch = WGetch(ctx, win);

      if (ctx->resize_request) {
        (void)AppStateClearResizeRequest(ctx);
        ReCreateWindows(ctx);
        DisplayMenu(ctx);
        DisplayDiskStatistic(ctx, &ctx->active->vol->vol_stats);
        restart_menu = TRUE;
        break;
      }

      switch (ch) {
      case KEY_F(1):
        (void)ShowVolumeHelpPopup(ctx);
        break;
      case KEY_UP:
        snapshot.selected_index--;
        if (snapshot.selected_index < 0) {
          snapshot.selected_index = snapshot.num_volumes - 1;
          layout.scroll_offset =
              MAXIMUM(0, snapshot.num_volumes - layout.visible_lines);
        } else if (snapshot.selected_index < layout.scroll_offset) {
          layout.scroll_offset--;
        }
        break;
      case KEY_DOWN:
        snapshot.selected_index++;
        if (snapshot.selected_index >= snapshot.num_volumes) {
          snapshot.selected_index = 0;
          layout.scroll_offset = 0;
        } else if (snapshot.selected_index >=
                   layout.scroll_offset + layout.visible_lines) {
          layout.scroll_offset++;
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
          if (snapshot.vol_array)
            free(snapshot.vol_array);
          if (win)
            UI_Dialog_Close(ctx, win);
          curs_set(1);
          return snapshot.selected_index;
        }

        if (snapshot.num_volumes <= 1) {
          UI_Message(ctx, "Cannot release the last volume.");
          // No need to redraw, loop will do it.
          break; // break from switch, loop continues to redraw
        }

        if (InputChoice(ctx, "Release this volume? (Y/N)", "YN\033") == 'Y') {
          struct Volume *target_vol = snapshot.vol_array[snapshot.selected_index];

          if (target_vol == ctx->active->vol) {
            /* Scenario A: Deleting Current Volume */
            /* Find a neighbor to switch to */
            // If selected is 0, try 1. Otherwise, try 0.
            int neighbor_idx = (snapshot.selected_index == 0) ? 1 : 0;
            struct Volume *neighbor = snapshot.vol_array[neighbor_idx];

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
          RefreshActiveVolumeSelection(ctx);
        }
        break; // break from switch, loop continues to redraw (if not
               // restart_menu)
      default:
        /* Ignore other keys */
        break;
      }
    }

    /* 5. Cleanup inside loop before potentially restarting */
    if (snapshot.vol_array) {
      free(snapshot.vol_array);
      snapshot.vol_array = NULL;
    }
    if (win) {
      UI_Dialog_Close(ctx, win);
      win = NULL;
    }

  } while (restart_menu);

  /* 6. Execution (if not cancelled) */
  curs_set(1); /* Restore cursor */

  if (result == 0) {
    struct Volume *s;
    struct Volume *tmp;
    struct Volume **vol_array = NULL;
    struct Volume *target_vol;
    int dummy = 0;
    int num_volumes;
    int i;

    if (!ctx->active || !ctx->active->vol)
      return -1;

    num_volumes = HASH_COUNT(ctx->volumes_head);
    if (num_volumes <= 0)
      return -1;

    vol_array = (struct Volume **)xmalloc(num_volumes * sizeof(struct Volume *));
    i = 0;
    HASH_ITER(hh, ctx->volumes_head, s, tmp) { vol_array[i++] = s; }

    if (snapshot.selected_index >= num_volumes)
      snapshot.selected_index = num_volumes - 1;
    if (snapshot.selected_index < 0)
      snapshot.selected_index = 0;

    target_vol = vol_array[snapshot.selected_index];
    if (target_vol != ctx->active->vol) {
      result = LogDisk(ctx, ctx->active, target_vol->vol_stats.log_path);
      EnsurePanelsReferenceActiveVolume(ctx);
      free(vol_array);
      return result;
    }

    free(vol_array);
    BuildDirEntryList(ctx, ctx->active->vol, &dummy);
    EnsurePanelsReferenceActiveVolume(ctx);
    return 0;
  }

  // If changes were made (volumes deleted), return 0 to force main loop to
  // refresh. Otherwise, return original result (0 for switch, -1 for cancel).
  if (changes_made) {
    RefreshActiveVolumeSelection(ctx);
    return 0;
  }

  return result;
}
