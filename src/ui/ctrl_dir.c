/***************************************************************************
 *
 * src/ui/ctrl_dir.c
 * Directory Window Controller (Input & Event Handling)
 *
 ***************************************************************************/

#define NO_YTREE_MACROS
#include "watcher.h"
#include "ytree.h"
#include <stdio.h>

/* TREEDEPTH uses GetProfileValue which is 2-arg in NO_YTREE_MACROS context */
#undef TREEDEPTH
#define TREEDEPTH (GetProfileValue)(ctx, "TREEDEPTH")

/* dir_list.c: BuildDirEntryList, FreeDirEntryList, FreeVolumeCache,
 *             GetPanelDirEntry, GetSelectedDirEntry */

/* dir_ops.c */
void HandlePlus(ViewContext *ctx, DirEntry *dir_entry, DirEntry *de_ptr,
                char *new_login_path, BOOL *need_dsp_help, YtreePanel *p);
void HandleReadSubTree(ViewContext *ctx, DirEntry *dir_entry,
                       BOOL *need_dsp_help, YtreePanel *p);
void HandleUnreadSubTree(ViewContext *ctx, DirEntry *dir_entry,
                         DirEntry *de_ptr, BOOL *need_dsp_help, YtreePanel *p);
void HandleShowAll(ViewContext *ctx, BOOL tagged_only, DirEntry *dir_entry,
                   BOOL *need_dsp_help, int *ch, YtreePanel *p);
void HandleSwitchWindow(ViewContext *ctx, DirEntry *dir_entry,
                        BOOL *need_dsp_help, int *ch, YtreePanel *p);

/* dir_nav.c */
void DirNav_Movedown(ViewContext *ctx, DirEntry **dir_entry, YtreePanel *p);
void DirNav_Moveup(ViewContext *ctx, DirEntry **dir_entry, YtreePanel *p);
void DirNav_Movenpage(ViewContext *ctx, DirEntry **dir_entry, YtreePanel *p);
void DirNav_Moveppage(ViewContext *ctx, DirEntry **dir_entry, YtreePanel *p);
void DirNav_MoveEnd(ViewContext *ctx, DirEntry **dir_entry, YtreePanel *p);
void DirNav_MoveHome(ViewContext *ctx, DirEntry **dir_entry, YtreePanel *p);

static BOOL HandleDirTagActions(ViewContext *ctx, int action,
                                DirEntry **dir_entry_ptr, BOOL *need_dsp_help,
                                int *ch) {
  DirEntry *dir_entry = *dir_entry_ptr;
  switch (action) {
  case ACTION_TAG:
    HandleTagDir(ctx, dir_entry, TRUE, ctx->active);
    DirNav_Movedown(ctx, dir_entry_ptr, ctx->active);
    return TRUE;
  case ACTION_UNTAG:
    HandleTagDir(ctx, dir_entry, FALSE, ctx->active);
    DirNav_Movedown(ctx, dir_entry_ptr, ctx->active);
    return TRUE;
  case ACTION_TAG_ALL:
    HandleTagAllDirs(ctx, ctx->active->vol, dir_entry, TRUE, ctx->active);
    return TRUE;
  case ACTION_UNTAG_ALL:
    HandleTagAllDirs(ctx, ctx->active->vol, dir_entry, FALSE, ctx->active);
    return TRUE;
  case ACTION_CMD_TAGGED_S:
    HandleShowAll(ctx, TRUE, dir_entry, need_dsp_help, ch, ctx->active);
    return TRUE;
  }
  return FALSE;
}

int HandleDirWindow(ViewContext *ctx, DirEntry *start_dir_entry) {
  DirEntry *dir_entry, *de_ptr;
  int i, ch, unput_char;
  BOOL need_dsp_help;
  char new_name[PATH_LENGTH + 1];
  char new_login_path[PATH_LENGTH + 1];
  char *home;
  YtreeAction action; /* Declare YtreeAction variable */
  struct Volume *start_vol;
  Statistic *s;
  int height;
  char watcher_path[PATH_LENGTH + 1];
  int local_cursor_pos;
  int local_disp_begin_pos;

  /* ADDED INSTRUCTION: Focus Unification */
  ctx->focused_window = FOCUS_TREE;

  DEBUG_LOG("HandleDirWindow: Calling ReCreateWindows");
  ReCreateWindows(ctx);
  DEBUG_LOG("HandleDirWindow: Calling DisplayMenu");
  DisplayMenu(ctx);

  unput_char = 0;
  de_ptr = NULL;

  /* Safety fallback if Init() has not set up panels */
  if (ctx->active == NULL)
    ctx->active = ctx->left;

  if (ctx->active) {
    DEBUG_LOG("HandleDirWindow: Syncing panel state");
    start_vol = ctx->active->vol;
    if (ctx->active->vol)
      s = &ctx->active->vol->vol_stats;
    else
      s = NULL;

    if (ctx->active->vol != ctx->active->vol) {
      ctx->active->vol = ctx->active->vol;
      /* Panel isolation: No vol_stats sync */
      ctx->active->disp_begin_pos = ctx->active->vol->id /* legacy removed */;
    }
    /* Ensure pointer is always fresh, but don't overwrite pos if vol is same */
    ctx->active->vol = ctx->active->vol;
    local_cursor_pos = ctx->active->cursor_pos;
    local_disp_begin_pos = ctx->active->disp_begin_pos;
    /* Ensure s points to ctx->active's volume stats */
    s = &ctx->active->vol->vol_stats;

    /* Update Global View Context to match Active Panel */
    /* This ensures macros like ctx->ctx_dir_window resolve to the
     * correct ncurses window
     */
    ctx->ctx_dir_window = ctx->active->pan_dir_window;
    ctx->ctx_small_file_window = ctx->active->pan_small_file_window;
    ctx->ctx_big_file_window = ctx->active->pan_big_file_window;
    ctx->ctx_file_window = ctx->active->pan_file_window;
  }

  /* Safety Reset for Preview Mode */
  if (ctx->preview_mode) {
    DEBUG_LOG("HandleDirWindow: Resetting preview mode");
    ctx->preview_mode = FALSE;
    ReCreateWindows(ctx);
    DisplayMenu(ctx);
    /* Update context again after ReCreateWindows */
    if (ctx->active) {
      ctx->ctx_dir_window = ctx->active->pan_dir_window;
      ctx->ctx_small_file_window = ctx->active->pan_small_file_window;
      ctx->ctx_big_file_window = ctx->active->pan_big_file_window;
      ctx->ctx_file_window = ctx->active->pan_file_window;
    }
  }

  height = getmaxy(ctx->ctx_dir_window);
  DEBUG_LOG("HandleDirWindow: Window height=%d", height);

  /* Clear flags */
  /*-----------------*/

  SetDirMode(ctx, MODE_3);

  need_dsp_help = TRUE;

  DEBUG_LOG("HandleDirWindow: Building DirEntryList for vol=%p",
            (void *)ctx->active->vol);
  BuildDirEntryList(ctx, ctx->active->vol, &ctx->active->current_dir_entry);
  if (ctx->initial_directory != NULL) {
    if (!strcmp(ctx->initial_directory, ".")) /* Entry just a single "." */
    {
      ctx->active->disp_begin_pos = 0;
      ctx->active->cursor_pos = 0;
      unput_char = CR;
    } else {
      if (*ctx->initial_directory == '.') { /* Entry of form "./alpha/beta" */
        strcpy(new_login_path, start_dir_entry->name);
        strcat(new_login_path, ctx->initial_directory + 1);
      } else if (*ctx->initial_directory == '~' && (home = getenv("HOME"))) {
        /* Entry of form "~/alpha/beta" */
        strcpy(new_login_path, home);
        strcat(new_login_path, ctx->initial_directory + 1);
      } else { /* Entry of form "beta" or "/full/path/alpha/beta" */
        strcpy(new_login_path, ctx->initial_directory);
      }
      for (i = 0; i < ctx->active->vol->total_dirs; i++) {
        if (*new_login_path == FILE_SEPARATOR_CHAR)
          GetPath(ctx->active->vol->dir_entry_list[i].dir_entry, new_name);
        else
          strcpy(new_name, ctx->active->vol->dir_entry_list[i].dir_entry->name);
        if (!strcmp(new_login_path, new_name)) {
          ctx->active->disp_begin_pos = i;
          ctx->active->cursor_pos = 0;
          unput_char = CR;
          break;
        }
      }
    }
    ctx->initial_directory = NULL;
  }

  {
    int safe_idx = ctx->active->disp_begin_pos + ctx->active->cursor_pos;
    if (ctx->active->vol->total_dirs <= 0) {
      ctx->active->disp_begin_pos = 0;
      ctx->active->cursor_pos = 0;
      safe_idx = 0;
    } else if (safe_idx < 0 || safe_idx >= ctx->active->vol->total_dirs) {
      ctx->active->disp_begin_pos = 0;
      ctx->active->cursor_pos = 0;
      safe_idx = 0;
    }

    if (ctx->active->vol->dir_entry_list) {
      dir_entry = ctx->active->vol->dir_entry_list[safe_idx].dir_entry;
    } else {
      dir_entry = ctx->active->vol->vol_stats.tree;
    }
  }

  /* REPLACEMENT START: Unified Rendering Call */
  if (!dir_entry->login_flag) {
    dir_entry->start_file = 0;
    dir_entry->cursor_pos = -1;
  }

  RefreshView(ctx, dir_entry);
  /* REPLACEMENT END */

  if (dir_entry->login_flag) {
    if ((dir_entry->global_flag) || (dir_entry->tagged_flag)) {
      unput_char = 'S';
    } else {
      unput_char = CR;
    }
  }
  do {
    /* Detect Global Volume Change (Split Brain Fix) */
    if (ctx->active->vol != start_vol)
      return ESC;

    if (need_dsp_help) {
      need_dsp_help = FALSE;
      DisplayDirHelp(ctx);
    }
    DisplayDirParameter(ctx, dir_entry);
    RefreshWindow(ctx->ctx_dir_window);

    if (ctx->is_split_screen) {
      YtreePanel *inactive =
          (ctx->active == ctx->left) ? ctx->right : ctx->left;
      RenderInactivePanel(ctx, inactive);
    }

    if (s->login_mode == DISK_MODE || s->login_mode == USER_MODE) {
      if (ctx->refresh_mode & REFRESH_WATCHER) {
        GetPath(dir_entry, watcher_path);
        Watcher_SetDir(ctx, watcher_path);
      }
    }

    if (unput_char) {
      ch = unput_char;
      unput_char = '\0';
    } else {
      doupdate();
      ch = (ctx->resize_request) ? -1 : GetEventOrKey(ctx);
      /* LF to CR normalization is now handled by GetKeyAction */
    }

    if (IsUserActionDefined(ctx)) { /* User commands take precedence */
      ch = DirUserMode(ctx, dir_entry, ch, &ctx->active->vol->vol_stats);
    }

    /* ViKey processing is now handled inside GetKeyAction */

    if (ctx->resize_request) {
      /* SIMPLIFIED RESIZE: Just call Global Refresh */
      RefreshView(ctx, dir_entry);
      need_dsp_help = TRUE;
      ctx->resize_request = FALSE;
    }

    action = GetKeyAction(ctx, ch); /* Translate raw input to YtreeAction */

    switch (action) {
    case ACTION_RESIZE:
      ctx->resize_request = TRUE;
      break;

    case ACTION_TOGGLE_STATS:
      ctx->show_stats = !ctx->show_stats;
      ctx->resize_request = TRUE;
      break;

    case ACTION_VIEW_PREVIEW: {
      YtreePanel *saved_panel = ctx->active;

      /* Save Preview State - BEFORE potential panel switching */
      ctx->preview_return_panel = ctx->active;
      ctx->preview_return_focus = ctx->focused_window;

      ctx->preview_mode = TRUE;
      /* ADDED INSTRUCTION: Track Entry Point */
      ctx->preview_entry_focus = FOCUS_TREE;

      /* Enter File Window loop via HandleSwitchWindow logic */
      /* We use HandleSwitchWindow to encapsulate the setup for HandleFileWindow
       */
      HandleSwitchWindow(ctx, dir_entry, &need_dsp_help, &ch, ctx->active);

      /* Post-Return Cleanup */
      ctx->preview_mode = FALSE; /* Restore cleaning of preview mode flag */
      ctx->focused_window = FOCUS_TREE; /* Restore focus to tree on return */

      if (ctx->active != saved_panel) {
        /* If panel was switched during preview, we REFRESH local state
         * BEFORE calling RefreshView to ensure the correct entry is
         * drawn. */
        start_vol = ctx->active->vol;
        s = &ctx->active->vol->vol_stats;

        if (ctx->active->vol && ctx->active->vol->total_dirs > 0) {
          dir_entry = ctx->active->vol
                          ->dir_entry_list[ctx->active->disp_begin_pos +
                                           ctx->active->cursor_pos]
                          .dir_entry;
        } else {
          dir_entry = s->tree;
        }

        /* Sync Global View Context */
        ctx->ctx_dir_window = ctx->active->pan_dir_window;
        ctx->ctx_small_file_window = ctx->active->pan_small_file_window;
        ctx->ctx_big_file_window = ctx->active->pan_big_file_window;
        ctx->ctx_file_window = ctx->active->pan_file_window;

        /* EXPLICIT REFRESH with the CORRECT dir_entry */
        RefreshView(ctx, dir_entry);

        need_dsp_help = TRUE;
        continue;
      }

      /* Standard refresh if panel didn't change */
      RefreshView(ctx, dir_entry);

      need_dsp_help = TRUE;

      /* Refresh local pointer */
      if (ctx->active->vol->total_dirs > 0) {
        dir_entry = ctx->active->vol
                        ->dir_entry_list[ctx->active->disp_begin_pos +
                                         ctx->active->cursor_pos]
                        .dir_entry;
      } else {
        dir_entry = s->tree;
      }
    } break;

    case ACTION_SPLIT_SCREEN:
      if (ctx->is_split_screen && ctx->active == ctx->right) {
        /* Preserve Right Panel state into Left Panel before unsplitting */
        ctx->left->vol = ctx->right->vol;
        ctx->left->cursor_pos = ctx->right->cursor_pos;
        ctx->left->disp_begin_pos = ctx->right->disp_begin_pos;
        /* Sync Global Volume */
        ctx->active->vol = ctx->left->vol;
      }

      ctx->is_split_screen = !ctx->is_split_screen;
      ReCreateWindows(ctx); /* Force layout update immediately */

      if (ctx->is_split_screen) {
        if (ctx->right && ctx->left) {
          ctx->right->vol = ctx->left->vol;
          ctx->right->cursor_pos = ctx->left->cursor_pos;
          ctx->right->disp_begin_pos = ctx->left->disp_begin_pos;
        }
      } else {
        ctx->active = ctx->left;
      }

      ctx->resize_request = TRUE;
      DisplayMenu(ctx);
      break;

    case ACTION_SWITCH_PANEL:
      if (!ctx->is_split_screen)
        break;

      /* Save local state to Active Panel for persistence before switching */
      /* Note: Navigation actions already update ctx->active->cursor_pos
       * directly. Do NOT overwrite with stale local_cursor_pos! */

      /* Switch Panel */
      if (ctx->active == ctx->left) {
        ctx->active = ctx->right;
      } else {
        ctx->active = ctx->left;
      }

      /* Restore Volume context */
      ctx->active->vol = ctx->active->vol;
      s = &ctx->active->vol->vol_stats; /* UPDATE S */
      /* Do NOT overwrite ctx->active->cursor_pos here; preserve its
       * independent state */

      /* Recalculate local dir_entry for the new ctx->active */
      if (ctx->active->vol->total_dirs > 0) {
        /* Clamp coordinates just in case */
        if (ctx->active->disp_begin_pos + ctx->active->cursor_pos >=
            ctx->active->vol->total_dirs) {
          ctx->active->cursor_pos =
              ctx->active->vol->total_dirs - 1 - ctx->active->disp_begin_pos;
        }
        dir_entry = ctx->active->vol
                        ->dir_entry_list[ctx->active->disp_begin_pos +
                                         ctx->active->cursor_pos]
                        .dir_entry;
      } else {
        dir_entry = s->tree;
      }
      DEBUG_LOG("ACTION_SWITCH_PANEL: active panel is now %s with "
                "cursor_pos=%d, dir_entry=%s",
                ctx->active == ctx->left ? "LEFT" : "RIGHT",
                ctx->active->cursor_pos, dir_entry ? dir_entry->name : "NULL");

      /* Sync Global View Context */
      ctx->ctx_dir_window = ctx->active->pan_dir_window;
      ctx->ctx_small_file_window = ctx->active->pan_small_file_window;
      ctx->ctx_big_file_window = ctx->active->pan_big_file_window;
      ctx->ctx_file_window = ctx->active->pan_file_window;

      /* REFRESH View for new active panel */
      RefreshView(ctx, dir_entry);
      need_dsp_help = TRUE;
      continue; /* Stay in loop with new context */

    case ACTION_NONE: /* -1 or unhandled keys */
      if (ch == -1)
        break; /* Ignore -1 (ctx->resize_request handled above) */
      /* Fall through for other unhandled keys to beep */
      beep();
      break;

    case ACTION_MOVE_DOWN:
      DirNav_Movedown(ctx, &dir_entry, ctx->active);
      break;
    case ACTION_MOVE_UP:
      DirNav_Moveup(ctx, &dir_entry, ctx->active);
      break;
    case ACTION_MOVE_SIBLING_NEXT: {
      DirEntry *target = dir_entry->next;
      if (target == NULL) {
        /* Wrap to first sibling */
        if (dir_entry->up_tree != NULL) {
          target = dir_entry->up_tree->sub_tree;
        }
      }

      if (target != NULL && target != dir_entry) {
        /* Find the sibling in the linear list to update cursor index */
        int k;
        int found_idx = -1;

        for (k = 0; k < ctx->active->vol->total_dirs; k++) {
          if (ctx->active->vol->dir_entry_list[k].dir_entry == target) {
            found_idx = k;
            break;
          }
        }

        if (found_idx != -1) {
          /* Move cursor to sibling */
          if (found_idx >= ctx->active->disp_begin_pos &&
              found_idx < ctx->active->disp_begin_pos + height) {
            ctx->active->cursor_pos = found_idx - ctx->active->disp_begin_pos;
          } else {
            /* Off screen, center it or move to top */
            ctx->active->disp_begin_pos = found_idx;
            ctx->active->cursor_pos = 0;
            /* Bounds check */
            if (ctx->active->disp_begin_pos + height >
                ctx->active->vol->total_dirs) {
              ctx->active->disp_begin_pos =
                  MAXIMUM(0, ctx->active->vol->total_dirs - height);
              ctx->active->cursor_pos = found_idx - ctx->active->disp_begin_pos;
            }
          }
          /* Sync */
          dir_entry = ctx->active->vol
                          ->dir_entry_list[ctx->active->disp_begin_pos +
                                           ctx->active->cursor_pos]
                          .dir_entry;

          if (0) {
            dir_entry = RefreshTreeSafe(ctx, ctx->active, dir_entry);
            break; /* Skip manual refresh logic below */
          }

          /* Refresh */
          DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                      ctx->active->disp_begin_pos,
                      ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                      TRUE);
          DisplayFileWindow(ctx, ctx->active, dir_entry);
          DisplayDiskStatistic(ctx, s);
          UpdateStatsPanel(ctx, dir_entry, s);
          DisplayAvailBytes(ctx, s);

          char path[PATH_LENGTH];
          GetPath(dir_entry, path);
          DisplayHeaderPath(ctx, path);
        }
      }
    }
      need_dsp_help = TRUE;
      break;
    case ACTION_MOVE_SIBLING_PREV: {
      DirEntry *target = dir_entry->prev;
      if (target == NULL) {
        /* Wrap to last sibling */
        if (dir_entry->up_tree != NULL) {
          target = dir_entry->up_tree->sub_tree;
          while (target && target->next != NULL) {
            target = target->next;
          }
        }
      }

      if (target != NULL && target != dir_entry) {
        /* Find the sibling in the linear list to update cursor index */
        int k;
        int found_idx = -1;

        for (k = 0; k < ctx->active->vol->total_dirs; k++) {
          if (ctx->active->vol->dir_entry_list[k].dir_entry == target) {
            found_idx = k;
            break;
          }
        }

        if (found_idx != -1) {
          /* Move cursor to sibling */
          if (found_idx >= ctx->active->disp_begin_pos &&
              found_idx < ctx->active->disp_begin_pos + height) {
            ctx->active->cursor_pos = found_idx - ctx->active->disp_begin_pos;
          } else {
            /* Off screen, center it or move to top */
            ctx->active->disp_begin_pos = found_idx;
            ctx->active->cursor_pos = 0;
            /* Bounds check */
            if (ctx->active->disp_begin_pos + height >
                ctx->active->vol->total_dirs) {
              ctx->active->disp_begin_pos =
                  MAXIMUM(0, ctx->active->vol->total_dirs - height);
              ctx->active->cursor_pos = found_idx - ctx->active->disp_begin_pos;
            }
          }
          /* Sync */
          dir_entry = ctx->active->vol
                          ->dir_entry_list[ctx->active->disp_begin_pos +
                                           ctx->active->cursor_pos]
                          .dir_entry;

          if (0) {
            dir_entry = RefreshTreeSafe(ctx, ctx->active, dir_entry);
            break; /* Skip manual refresh logic below */
          }

          /* Refresh */
          DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                      ctx->active->disp_begin_pos,
                      ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                      TRUE);
          DisplayFileWindow(ctx, ctx->active, dir_entry);
          DisplayDiskStatistic(ctx, s);
          UpdateStatsPanel(ctx, dir_entry, s);
          DisplayAvailBytes(ctx, s);

          char path[PATH_LENGTH];
          GetPath(dir_entry, path);
          DisplayHeaderPath(ctx, path);
        }
      }
    }
      need_dsp_help = TRUE;
      break;
    case ACTION_PAGE_DOWN:
      DirNav_Movenpage(ctx, &dir_entry, ctx->active);
      break;
    case ACTION_PAGE_UP:
      DirNav_Moveppage(ctx, &dir_entry, ctx->active);
      break;
    case ACTION_HOME:
      DirNav_MoveHome(ctx, &dir_entry, ctx->active);
      break;
    case ACTION_END:
      DirNav_MoveEnd(ctx, &dir_entry, ctx->active);
      break;
    case ACTION_MOVE_RIGHT:
    case ACTION_TREE_EXPAND_ALL:
      HandlePlus(ctx, dir_entry, de_ptr, new_login_path, &need_dsp_help,
                 ctx->active);
      break;
    case ACTION_ASTERISK:
      HandleReadSubTree(ctx, dir_entry, &need_dsp_help, ctx->active);
      break;
    case ACTION_TREE_EXPAND:
      HandleReadSubTree(ctx, dir_entry, &need_dsp_help, ctx->active);
      break;
    case ACTION_MOVE_LEFT:
      /* 1. Transparent Archive Exit Logic (At Root) */
      if (dir_entry->up_tree == NULL && ctx->view_mode == ARCHIVE_MODE) {
        struct Volume *old_vol = ctx->active->vol;
        char archive_path[PATH_LENGTH + 1];
        char parent_dir[PATH_LENGTH + 1];
        char dummy_name[PATH_LENGTH + 1];

        /* Calculate Parent Directory of the Archive File */
        strcpy(archive_path, s->login_path);
        Fnsplit(archive_path, parent_dir, dummy_name);

        /* Force Login/Switch to the Parent Directory */
        /* This handles both "New Volume" and "Switch to Existing" logic
         * automatically */
        if (LogDisk(ctx, ctx->active, parent_dir) == 0) {
          /* Successfully switched context. */

          /* Delete the archive wrapper we just left to clean up memory/list */
          /* Fix Archive Double-Free Check */
          BOOL vol_in_use = FALSE;
          if (ctx->is_split_screen) {
            YtreePanel *other =
                (ctx->active == ctx->left) ? ctx->right : ctx->left;
            if (other->vol == old_vol)
              vol_in_use = TRUE;
          }
          if (!vol_in_use)
            Volume_Delete(ctx, old_vol);

          /* Update pointers for the new context */
          s = &ctx->active->vol->vol_stats;    /* UPDATE S */
          start_vol = ctx->active->vol;        /* Update loop safety variable */
          ctx->active->vol = ctx->active->vol; /* Update Panel volume */

          /* Reset Search Term to prevent bleeding */
          ctx->global_search_term[0] = '\0';

          /* Attempt to highlight the archive file we just left */
          if (ctx->active->vol->total_dirs > 0) {
            /* Usually root, or restored position */
            dir_entry = ctx->active->vol
                            ->dir_entry_list[ctx->active->disp_begin_pos +
                                             ctx->active->cursor_pos]
                            .dir_entry;

            /* Find the archive file in the file list */
            FileEntry *fe;
            int f_idx = 0;
            for (fe = dir_entry->file; fe; fe = fe->next) {
              if (strcmp(fe->name, dummy_name) == 0) {
                dir_entry->start_file = f_idx;
                dir_entry->cursor_pos = 0;
                break;
              }
              f_idx++;
            }
          } else {
            dir_entry = s->tree;
          }

          /* Reset global view mode explicitly */
          ctx->view_mode = DISK_MODE;
          ctx->view_mode = DISK_MODE; /* Force legacy macro to update too */

          /* Force layout reset to small window split view */
          SwitchToSmallFileWindow(ctx);

          /* Force full screen clear to fix UI artifacts (separator line) */
          werase(ctx->ctx_file_window); /* Erase file window specifically */
          clearok(stdscr, TRUE);
          refresh();

          /* Refresh Full UI */
          DisplayMenu(ctx);
          DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                      ctx->active->disp_begin_pos,
                      ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                      TRUE);
          DisplayFileWindow(ctx, ctx->active, dir_entry);
          DisplayDiskStatistic(ctx, s);
          UpdateStatsPanel(ctx, dir_entry, s);
          DisplayAvailBytes(ctx, s);

          {
            char path[PATH_LENGTH];
            GetPath(dir_entry, path);
            DisplayHeaderPath(ctx, path);
          }
          need_dsp_help = TRUE;
          break; /* Exit case done */
        }
      }

      /* 2. Standard Tree Navigation Logic */
      if (!dir_entry->not_scanned && dir_entry->sub_tree != NULL) {
        /* It is expanded */
        if (ctx->view_mode == ARCHIVE_MODE) {
          /* In Archive Mode, we cannot collapse (UnRead) without data loss.
          So we skip collapse and fall through to Jump to Parent. */
          goto JUMP_TO_PARENT;
        } else {
          /* In FS Mode, collapse it. */
          HandleUnreadSubTree(ctx, dir_entry, de_ptr, &need_dsp_help,
                              ctx->active);
        }
      } else {
        /* It is collapsed (or leaf) -> Jump to Parent */
      JUMP_TO_PARENT:
        if (dir_entry->up_tree != NULL) {
          /* Find parent in the list */
          int p_idx = -1;
          int k;
          for (k = 0; k < ctx->active->vol->total_dirs; k++) {
            if (ctx->active->vol->dir_entry_list[k].dir_entry ==
                dir_entry->up_tree) {
              p_idx = k;
              break;
            }
          }
          if (p_idx != -1) {
            /* Move cursor to parent */
            if (p_idx >= ctx->active->disp_begin_pos &&
                p_idx < ctx->active->disp_begin_pos + height) {
              /* Parent is on screen */
              ctx->active->cursor_pos = p_idx - ctx->active->disp_begin_pos;
            } else {
              /* Parent is off screen - center it or move to top */
              ctx->active->disp_begin_pos = p_idx;
              ctx->active->cursor_pos = 0;
              /* Adjust if near end */
              if (ctx->active->disp_begin_pos + height >
                  ctx->active->vol->total_dirs) {
                ctx->active->disp_begin_pos =
                    MAXIMUM(0, ctx->active->vol->total_dirs - height);
                ctx->active->cursor_pos = p_idx - ctx->active->disp_begin_pos;
              }
            }
            /* Sync pointers */
            dir_entry = ctx->active->vol
                            ->dir_entry_list[ctx->active->disp_begin_pos +
                                             ctx->active->cursor_pos]
                            .dir_entry;

            /* Refresh */
            DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                        ctx->active->disp_begin_pos,
                        ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                        TRUE);
            DisplayFileWindow(ctx, ctx->active, dir_entry);
            DisplayDiskStatistic(ctx, s);
            UpdateStatsPanel(ctx, dir_entry, s);
            DisplayAvailBytes(ctx, s);
            /* Update Header Path */
            char path[PATH_LENGTH];
            GetPath(dir_entry, path);
            DisplayHeaderPath(ctx, path);
          }
        }
      }
      break;
    case ACTION_TREE_COLLAPSE:
      HandleUnreadSubTree(ctx, dir_entry, de_ptr, &need_dsp_help, ctx->active);
      break;
    case ACTION_TOGGLE_HIDDEN: {
      ToggleDotFiles(ctx, ctx->active);

      /* Update current dir pointer using the new accessor function
      because ToggleDotFiles might have changed the list layout */
      if (ctx->active->vol->total_dirs > 0) {
        dir_entry = ctx->active->vol
                        ->dir_entry_list[ctx->active->disp_begin_pos +
                                         ctx->active->cursor_pos]
                        .dir_entry;
      } else {
        dir_entry = s->tree;
      }

      need_dsp_help = TRUE;
    } break;
    case ACTION_FILTER:
      if (UI_ReadFilter(ctx) == 0) {
        RecalculateSysStats(ctx, s);
        dir_entry->start_file = 0;
        dir_entry->cursor_pos = -1;
        RefreshView(ctx, dir_entry);
      }
      need_dsp_help = TRUE;
      break;
    case ACTION_TAG:
    case ACTION_UNTAG:
    case ACTION_TAG_ALL:
    case ACTION_UNTAG_ALL:
    case ACTION_CMD_TAGGED_S:
      if (HandleDirTagActions(ctx, action, &dir_entry, &need_dsp_help, &ch)) {
        break;
      }
      break;

    case ACTION_TOGGLE_MODE:
      RotateDirMode(ctx);
      /*DisplayFileWindow(ctx,  dir_entry, 0, -1 );*/
      DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                  ctx->active->disp_begin_pos,
                  ctx->active->disp_begin_pos + ctx->active->cursor_pos, TRUE);
      /*RefreshWindow( ctx->ctx_file_window );*/
      DisplayDiskStatistic(ctx, s);
      UpdateStatsPanel(ctx, dir_entry, s);
      need_dsp_help = TRUE;
      break;

    case ACTION_CMD_S:
      HandleShowAll(ctx, FALSE, dir_entry, &need_dsp_help, &ch, ctx->active);
      break;
    case ACTION_ENTER:
      if (dir_entry == NULL) {
        beep();
        break;
      }

      if (ctx->refresh_mode & REFRESH_ON_ENTER) {
        dir_entry = RefreshTreeSafe(ctx, ctx->active, dir_entry);
        /* Sync pointer from list in case address changed */
        dir_entry = ctx->active->vol
                        ->dir_entry_list[ctx->active->disp_begin_pos +
                                         ctx->active->cursor_pos]
                        .dir_entry;
      }

      DEBUG_LOG("ACTION_ENTER: dir_entry=%p name=%s matching=%d",
                (void *)dir_entry, dir_entry ? dir_entry->name : "NULL",
                dir_entry ? dir_entry->matching_files : -1);

      if (dir_entry == NULL || dir_entry->total_files == 0) {
        beep();
        break;
      }

      /* Fix Context Safety on Return */
      {
        YtreePanel *saved_panel = ctx->active;

        /* Note: Navigation actions already update active->cursor_pos. */

        HandleSwitchWindow(ctx, dir_entry, &need_dsp_help, &ch, ctx->active);

        /* Restore focus to tree on return */
        ctx->focused_window = FOCUS_TREE;

        if (ctx->active != saved_panel) {
          /* If panel was switched, refresh state locally and continue */
          start_vol = ctx->active->vol;
          s = &ctx->active->vol->vol_stats;

          if (ctx->active->vol && ctx->active->vol->total_dirs > 0) {
            dir_entry = ctx->active->vol
                            ->dir_entry_list[ctx->active->disp_begin_pos +
                                             ctx->active->cursor_pos]
                            .dir_entry;
          } else {
            dir_entry = s->tree;
          }

          ctx->ctx_dir_window = ctx->active->pan_dir_window;
          ctx->ctx_small_file_window = ctx->active->pan_small_file_window;
          ctx->ctx_big_file_window = ctx->active->pan_big_file_window;
          ctx->ctx_file_window = ctx->active->pan_file_window;

          RefreshView(ctx, dir_entry);
          need_dsp_help = TRUE;
          action = ACTION_NONE; /* Prevent while(...) condition from exiting
                                   HandleDirWindow! */
          continue;
        }

        /* Important: Check for volume changes after returning from File Window
         */
        if (ctx->active->vol != start_vol)
          return ESC;

        /* CENTRALIZED REFRESH: Restore clean layout after return */
        RefreshView(ctx, dir_entry);
      }
      /* Refresh local pointer */
      dir_entry = ctx->active->vol
                      ->dir_entry_list[ctx->active->disp_begin_pos +
                                       ctx->active->cursor_pos]
                      .dir_entry;
      break;
    case ACTION_CMD_X:
      if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
      } else {
        char command_template[COMMAND_LINE_LENGTH + 1];
        command_template[0] = '\0';
        if (GetCommandLine(ctx, command_template) == 0) {
          (void)Execute(ctx, dir_entry, NULL, command_template,
                        &ctx->active->vol->vol_stats, UI_ArchiveCallback);
          dir_entry = RefreshTreeSafe(
              ctx, ctx->active, dir_entry); /* Auto-Refresh after command */
        }
      }
      need_dsp_help = TRUE;
      DisplayAvailBytes(ctx, s);
      DisplayDiskStatistic(ctx, s);
      UpdateStatsPanel(ctx, dir_entry, s);
      break;
    case ACTION_CMD_MKFILE:
      DEBUG_LOG("ACTION_CMD_MKFILE reached in ctrl_dir.c. mode=%d",
                ctx->view_mode);
      if (ctx->view_mode != DISK_MODE)
        break;
      {
        char file_name[PATH_LENGTH * 2 + 1];
        int mk_result;

        ClearHelp(ctx);
        *file_name = '\0';
        if (UI_ReadString(ctx, ctx->active, "MAKE FILE:", file_name,
                          PATH_LENGTH, HST_FILE) == CR) {
          mk_result =
              MakeFile(ctx, dir_entry, file_name, s, NULL, UI_ChoiceResolver);
          if (mk_result == 0) {
            /* Determine where the new file should be. */
            if (ctx->active && ctx->active->pan_file_window) {
              /* Force file window refresh */
              DisplayFileWindow(ctx, ctx->active, dir_entry);
            }
            RefreshView(ctx, dir_entry);
          } else if (mk_result == 1) {
            MESSAGE(ctx, "File already exists!");
          } else {
            MESSAGE(ctx, "Can't create File*\"%s\"", file_name);
          }
        }
      }
      need_dsp_help = TRUE;
      break;

    case ACTION_CMD_M: {
      char dir_name[PATH_LENGTH * 2 + 1];
      ClearHelp(ctx);
      *dir_name = '\0';
      if (UI_ReadString(ctx, ctx->active, "MAKE DIRECTORY:", dir_name,
                        PATH_LENGTH, HST_FILE) == CR) {
        if (!MakeDirectory(ctx, ctx->active, dir_entry, dir_name, s)) {
          BuildDirEntryList(ctx, ctx->active->vol,
                            &ctx->active->current_dir_entry);
          RefreshView(ctx, dir_entry);
        }
      }
      move(LINES - 2, 1);
      clrtoeol();
    }
      need_dsp_help = TRUE;
      break;
    case ACTION_CMD_D:
      if (!DeleteDirectory(ctx, dir_entry, UI_ChoiceResolver)) {
        if (ctx->active->disp_begin_pos + ctx->active->cursor_pos > 0) {
          if (ctx->active->cursor_pos > 0)
            ctx->active->cursor_pos--;
          else
            ctx->active->disp_begin_pos--;
        }
      }
      /* Update regardless of success */
      BuildDirEntryList(ctx, ctx->active->vol, &ctx->active->current_dir_entry);
      dir_entry = ctx->active->vol
                      ->dir_entry_list[ctx->active->disp_begin_pos +
                                       ctx->active->cursor_pos]
                      .dir_entry;
      dir_entry->start_file = 0;
      dir_entry->cursor_pos = -1;

      RefreshView(ctx, dir_entry);
      need_dsp_help = TRUE;
      break;

    case ACTION_CMD_R:
      if (!GetRenameParameter(ctx, dir_entry->name, new_name)) {
        if (!RenameDirectory(ctx, dir_entry, new_name)) {
          /* Rename OK */
          BuildDirEntryList(ctx, ctx->active->vol,
                            &ctx->active->current_dir_entry);
          dir_entry = ctx->active->vol
                          ->dir_entry_list[ctx->active->disp_begin_pos +
                                           ctx->active->cursor_pos]
                          .dir_entry;
          RefreshView(ctx, dir_entry);
        }
      }
      need_dsp_help = TRUE;
      break;
    case ACTION_REFRESH: /* Rescan */
      dir_entry = RefreshTreeSafe(ctx, ctx->active, dir_entry);
      need_dsp_help = TRUE;
      break;
    case ACTION_CMD_G:
      (void)ChangeDirGroup(dir_entry);
      DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                  ctx->active->disp_begin_pos,
                  ctx->active->disp_begin_pos + ctx->active->cursor_pos, TRUE);
      DisplayDiskStatistic(ctx, s);
      UpdateStatsPanel(ctx, dir_entry, s);
      need_dsp_help = TRUE;
      break;
    case ACTION_CMD_O:
      (void)ChangeDirOwner(dir_entry);
      DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                  ctx->active->disp_begin_pos,
                  ctx->active->disp_begin_pos + ctx->active->cursor_pos, TRUE);
      DisplayDiskStatistic(ctx, s);
      UpdateStatsPanel(ctx, dir_entry, s);
      need_dsp_help = TRUE;
      break;
    case ACTION_CMD_A:
      (void)ChangeDirModus(ctx, dir_entry);
      DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                  ctx->active->disp_begin_pos,
                  ctx->active->disp_begin_pos + ctx->active->cursor_pos, TRUE);
      DisplayDiskStatistic(ctx, s);
      UpdateStatsPanel(ctx, dir_entry, s);
      need_dsp_help = TRUE;
      break;

    case ACTION_TOGGLE_COMPACT:
      ctx->fixed_col_width = (ctx->fixed_col_width == 0) ? 32 : 0;
      ctx->resize_request = TRUE;
      break;

    case ACTION_CMD_P: /* Pipe Directory */
    {
      char pipe_cmd[PATH_LENGTH + 1];
      pipe_cmd[0] = '\0';
      if (GetPipeCommand(ctx, pipe_cmd) == 0) {
        PipeDirectory(ctx, dir_entry, pipe_cmd);
      }
    }
      need_dsp_help = TRUE;
      break;

    /* Volume Cycling and Selection */
    case ACTION_VOL_MENU: /* Shift-K: Select Loaded Volume */
    {
      /* Save current panel state to volume before switching away */
      /* Panel state isolation: No vol_stats sync */

      int res = SelectLoadedVolume(ctx, NULL);
      if (res == 0) { /* If volume switch was successful */
        if (ctx->active->vol != start_vol)
          return ESC; /* Abort to main loop to handle clean re-entry */

        /* Update loop variables for new volume */
        s = &ctx->active->vol->vol_stats; /* UPDATE S */
        ctx->active->vol = ctx->active->vol;
        /* Panel isolation: No vol_stats sync */
        ctx->active->disp_begin_pos = ctx->active->vol->id /* legacy removed */;

        /* Safety check / Clamping */
        if (ctx->active->vol->total_dirs > 0) {
          /* If saved position is beyond current end, clamp to last item */
          if (ctx->active->disp_begin_pos + ctx->active->cursor_pos >=
              ctx->active->vol->total_dirs) {
            /* Clamp to last valid index */
            int last_idx = ctx->active->vol->total_dirs - 1;
            /* Determine new disp_begin_pos and cursor_pos */
            if (last_idx >= ctx->layout.dir_win_height) {
              ctx->active->disp_begin_pos =
                  last_idx - (ctx->layout.dir_win_height - 1);
              ctx->active->cursor_pos = ctx->layout.dir_win_height - 1;
            } else {
              ctx->active->disp_begin_pos = 0;
              ctx->active->cursor_pos = last_idx;
            }
          }
          /* Now safe to assign dir_entry */
          dir_entry = ctx->active->vol
                          ->dir_entry_list[ctx->active->disp_begin_pos +
                                           ctx->active->cursor_pos]
                          .dir_entry;
        } else {
          dir_entry = s->tree;
        }

        DisplayMenu(ctx); /* Force redraw of frame/separator */
        DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                    ctx->active->disp_begin_pos,
                    ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                    TRUE);
        DisplayFileWindow(
            ctx, ctx->active,
            dir_entry); /* Refresh file window for the new directory */
        RefreshWindow(ctx->ctx_file_window);
        DisplayDiskStatistic(ctx, s);
        UpdateStatsPanel(ctx, dir_entry, s);
        DisplayAvailBytes(ctx, s);
        /* Update header path after volume switch */
        {
          char path[PATH_LENGTH];
          GetPath(dir_entry, path);
          DisplayHeaderPath(ctx, path);
        }
        need_dsp_help = TRUE;
      }
    } break;

    case ACTION_VOL_PREV: /* Previous Volume */
    {
      /* Save current panel state to volume before switching away */
      /* Panel isolation: No vol_stats sync */
      ctx->active->vol->id /* legacy removed */ = ctx->active->disp_begin_pos;

      int res = CycleLoadedVolume(ctx, ctx->active, -1);
      if (res == 0) { /* If volume switch was successful */
        if (ctx->active->vol != start_vol)
          return ESC;

        s = &ctx->active->vol->vol_stats; /* UPDATE S */
        ctx->active->vol = ctx->active->vol;
        /* Panel isolation: No vol_stats sync */
        ctx->active->disp_begin_pos = ctx->active->vol->id /* legacy removed */;

        /* Safety check / Clamping */
        if (ctx->active->vol->total_dirs > 0) {
          if (ctx->active->disp_begin_pos + ctx->active->cursor_pos >=
              ctx->active->vol->total_dirs) {
            int last_idx = ctx->active->vol->total_dirs - 1;
            if (last_idx >= ctx->layout.dir_win_height) {
              ctx->active->disp_begin_pos =
                  last_idx - (ctx->layout.dir_win_height - 1);
              ctx->active->cursor_pos = ctx->layout.dir_win_height - 1;
            } else {
              ctx->active->disp_begin_pos = 0;
              ctx->active->cursor_pos = last_idx;
            }
          }
          dir_entry = ctx->active->vol
                          ->dir_entry_list[ctx->active->disp_begin_pos +
                                           ctx->active->cursor_pos]
                          .dir_entry;
        } else {
          dir_entry = s->tree;
        }

        DisplayMenu(ctx);
        DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                    ctx->active->disp_begin_pos,
                    ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                    TRUE);
        DisplayFileWindow(ctx, ctx->active, dir_entry);
        RefreshWindow(ctx->ctx_file_window);
        DisplayDiskStatistic(ctx, s);
        UpdateStatsPanel(ctx, dir_entry, s);
        DisplayAvailBytes(ctx, s);
        {
          char path[PATH_LENGTH];
          GetPath(dir_entry, path);
          DisplayHeaderPath(ctx, path);
        }
        need_dsp_help = TRUE;
      }
    } break;

    case ACTION_VOL_NEXT: /* Next Volume */
    {
      /* Save current panel state to volume before switching away */
      /* Panel isolation: No vol_stats sync */
      ctx->active->vol->id /* legacy removed */ = ctx->active->disp_begin_pos;

      int res = CycleLoadedVolume(ctx, ctx->active, 1);
      if (res == 0) { /* If volume switch was successful */
        if (ctx->active->vol != start_vol)
          return ESC;

        s = &ctx->active->vol->vol_stats; /* UPDATE S */
        ctx->active->vol = ctx->active->vol;
        /* Panel isolation: No vol_stats sync */
        ctx->active->disp_begin_pos = ctx->active->vol->id /* legacy removed */;

        if (ctx->active->vol->total_dirs > 0) {
          if (ctx->active->disp_begin_pos + ctx->active->cursor_pos >=
              ctx->active->vol->total_dirs) {
            int last_idx = ctx->active->vol->total_dirs - 1;
            if (last_idx >= ctx->layout.dir_win_height) {
              ctx->active->disp_begin_pos =
                  last_idx - (ctx->layout.dir_win_height - 1);
              ctx->active->cursor_pos = ctx->layout.dir_win_height - 1;
            } else {
              ctx->active->disp_begin_pos = 0;
              ctx->active->cursor_pos = last_idx;
            }
          }
          dir_entry = ctx->active->vol
                          ->dir_entry_list[ctx->active->disp_begin_pos +
                                           ctx->active->cursor_pos]
                          .dir_entry;
        } else {
          dir_entry = s->tree;
        }

        DisplayMenu(ctx);
        DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                    ctx->active->disp_begin_pos,
                    ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                    TRUE);
        DisplayFileWindow(ctx, ctx->active, dir_entry);
        RefreshWindow(ctx->ctx_file_window);
        DisplayDiskStatistic(ctx, s);
        UpdateStatsPanel(ctx, dir_entry, s);
        DisplayAvailBytes(ctx, s);
        {
          char path[PATH_LENGTH];
          GetPath(dir_entry, path);
          DisplayHeaderPath(ctx, path);
        }
        need_dsp_help = TRUE;
      }
    } break;

    case ACTION_QUIT_DIR:
      need_dsp_help = TRUE;
      QuitTo(ctx, dir_entry);
      break;

    case ACTION_QUIT:
      need_dsp_help = TRUE;
      Quit(ctx);
      action = ACTION_NONE;
      break;

    case ACTION_LOG:
    case ACTION_LOGIN:
      if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
        if (getcwd(new_login_path, sizeof(new_login_path)) == NULL) {
          strcpy(new_login_path, ".");
        }
      } else {
        (void)GetPath(dir_entry, new_login_path);
      }
      if (!GetNewLoginPath(ctx, ctx->active, new_login_path)) {
        int ret; /* DEBUG variable */
        DisplayMenu(ctx);
        doupdate();

        ret = LogDisk(ctx, ctx->active, new_login_path);

        /* Check return value. Only update state if login succeeded (0). */
        if (ret == 0) {
          /* Safety Check: If volume was changed, return to main loop to handle
           * new context */
          if (ctx->active->vol != start_vol)
            return ESC;

          s = &ctx->active->vol
                   ->vol_stats; /* Update stats pointer for new volume */
          ctx->active->vol = ctx->active->vol;

          /* Safety check / Clamping */
          if (ctx->active->vol->total_dirs > 0) {
            if (ctx->active->disp_begin_pos + ctx->active->cursor_pos >=
                ctx->active->vol->total_dirs) {
              int last_idx = ctx->active->vol->total_dirs - 1;
              if (last_idx >= ctx->layout.dir_win_height) {
                ctx->active->disp_begin_pos =
                    last_idx - (ctx->layout.dir_win_height - 1);
                ctx->active->cursor_pos = ctx->layout.dir_win_height - 1;
              } else {
                ctx->active->disp_begin_pos = 0;
                ctx->active->cursor_pos = last_idx;
              }
            }
            /* Now safe to assign dir_entry */
            dir_entry = ctx->active->vol
                            ->dir_entry_list[ctx->active->disp_begin_pos +
                                             ctx->active->cursor_pos]
                            .dir_entry;
          } else {
            dir_entry = s->tree;
          }

          DisplayMenu(ctx); /* Redraw menu/frame */

          /* Force Full Display Refresh */
          DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                      ctx->active->disp_begin_pos,
                      ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                      TRUE);
          DisplayFileWindow(ctx, ctx->active, dir_entry);
          RefreshWindow(ctx->ctx_file_window);
          DisplayDiskStatistic(ctx, s);
          DisplayDirStatistic(ctx, dir_entry, NULL, s);
          DisplayAvailBytes(ctx, s);
          /* Update header path */
          {
            char path[PATH_LENGTH];
            GetPath(dir_entry, path);
            DisplayHeaderPath(ctx, path);
          }
        }
        need_dsp_help = TRUE;
      }
      break;
    /* Ctrl-L is now ACTION_REFRESH, handled above */
    default: /* Unhandled action, beep */
      beep();
      break;
    } /* switch */

    /* Refresh dir_entry pointer in case tree was rescanned/reallocated during
     * action */
    if (ctx->active && ctx->active->vol && ctx->active->vol->dir_entry_list) {
      int safe_idx = ctx->active->disp_begin_pos + ctx->active->cursor_pos;
      if (safe_idx < 0)
        safe_idx = 0;
      if (safe_idx >= ctx->active->vol->total_dirs)
        safe_idx = ctx->active->vol->total_dirs - 1;
      if (safe_idx >= 0) {
        dir_entry = ctx->active->vol->dir_entry_list[safe_idx].dir_entry;
      } else {
        dir_entry = ctx->active->vol->vol_stats.tree;
      }
    }

  } while (action != ACTION_QUIT && action != ACTION_ENTER &&
           action != ACTION_ESCAPE &&
           action !=
               ACTION_LOGIN); /* Loop until explicit quit, escape or login */

  /* Sync state back to Volume on exit */
  /* Removed shared state sync on exit */

  return (ch); /* Return the last raw character that caused exit */
}
