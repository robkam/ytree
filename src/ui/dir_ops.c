/***************************************************************************
 *
 * src/ui/dir_ops.c
 * Directory Tree Operations (Expand, Collapse, Scan, Dotfile Toggle,
 * Mode Switching, Show-All, Refresh)
 *
 ***************************************************************************/

#define NO_YTREE_MACROS
#include "watcher.h"
#include "ytree.h"
#include "ytree_ui.h"

/* TREEDEPTH uses GetProfileValue which is 2-arg in NO_YTREE_MACROS context */
#undef TREEDEPTH
#define TREEDEPTH (GetProfileValue)(ctx, "TREEDEPTH")

/* Progress callback for directory operations */
static void Dir_Progress(ViewContext *ctx, void *data) {
  (void)data; /* Suppress unused parameter warning */
  DrawSpinner(ctx);
}

void HandlePlus(ViewContext *ctx, DirEntry *dir_entry, DirEntry *de_ptr,
                char *new_login_path, BOOL *need_dsp_help, YtreePanel *p) {
  Statistic *s = &p->vol->vol_stats;
  WINDOW *win = p->pan_dir_window;

  /* Renamed usage: s->mode -> s->login_mode */
  if (s->login_mode != DISK_MODE && s->login_mode != USER_MODE) {
    return;
  }
  if (!dir_entry->not_scanned) {
  } else {
    SuspendClock(ctx); /* Suspend clock before scanning */
    for (de_ptr = dir_entry->sub_tree; de_ptr; de_ptr = de_ptr->next) {
      GetPath(de_ptr, new_login_path);
      ReadTree(ctx, de_ptr, new_login_path, 0, s, Dir_Progress, NULL);
      ApplyFilter(de_ptr, s);
    }
    InitClock(ctx); /* Resume clock after scanning */
    dir_entry->not_scanned = FALSE;
    BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);
    BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);
    DisplayTree(ctx, p->vol, p->pan_dir_window, p->disp_begin_pos,
                p->disp_begin_pos + p->cursor_pos, TRUE);
    DisplayDiskStatistic(ctx, s);
    UpdateStatsPanel(ctx, dir_entry, s); /* Show dir stats AND attributes */
    DisplayAvailBytes(ctx, s);
    *need_dsp_help = TRUE;
  }
}

void HandleReadSubTree(ViewContext *ctx, DirEntry *dir_entry,
                       BOOL *need_dsp_help, YtreePanel *p) {
  Statistic *s = &p->vol->vol_stats;
  WINDOW *win = p->pan_dir_window;

  SuspendClock(ctx); /* Suspend clock before scanning */
  if (ScanSubTree(ctx, dir_entry, s) == -1) {
    /* Aborted. Fall through to refresh what we have. */
  }
  InitClock(ctx); /* Resume clock after scanning */
  BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);
  BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);
  DisplayTree(ctx, p->vol, p->pan_dir_window, p->disp_begin_pos,
              p->disp_begin_pos + p->cursor_pos, TRUE);
  RecalculateSysStats(ctx, s); /* Fix for Bug 10: Force full recalculation */
  DisplayDiskStatistic(ctx, s);
  UpdateStatsPanel(ctx, dir_entry, s);
  DisplayAvailBytes(ctx, s);
  *need_dsp_help = TRUE;
}

static BOOL IsDescendant(DirEntry *ancestor, DirEntry *descendant) {
  while (descendant) {
    if (descendant == ancestor)
      return TRUE;
    descendant = descendant->up_tree;
  }
  return FALSE;
}

static int FindDirIndex(struct Volume *vol, DirEntry *target) {
  int i;

  if (!vol || !target || !vol->dir_entry_list || vol->total_dirs <= 0)
    return -1;

  for (i = 0; i < vol->total_dirs; i++) {
    if (vol->dir_entry_list[i].dir_entry == target)
      return i;
  }

  return -1;
}

static void ReanchorPanelToDir(YtreePanel *panel, DirEntry *target) {
  int idx;
  int height;

  if (!panel)
    return;

  if (!panel->vol || panel->vol->total_dirs <= 0) {
    panel->disp_begin_pos = 0;
    panel->cursor_pos = 0;
    return;
  }

  if (!target)
    target = panel->vol->vol_stats.tree;

  idx = FindDirIndex(panel->vol, target);
  if (idx < 0)
    idx = 0;

  height = (panel->pan_dir_window) ? getmaxy(panel->pan_dir_window) : 1;
  if (height < 1)
    height = 1;

  if (idx >= panel->disp_begin_pos && idx < panel->disp_begin_pos + height) {
    panel->cursor_pos = idx - panel->disp_begin_pos;
  } else {
    panel->disp_begin_pos = idx;
    panel->cursor_pos = 0;

    if (panel->disp_begin_pos + height > panel->vol->total_dirs) {
      panel->disp_begin_pos = panel->vol->total_dirs - height;
      if (panel->disp_begin_pos < 0)
        panel->disp_begin_pos = 0;
      panel->cursor_pos = idx - panel->disp_begin_pos;
    }
  }

  if (panel->cursor_pos < 0)
    panel->cursor_pos = 0;
}

void HandleUnreadSubTree(ViewContext *ctx, DirEntry *dir_entry,
                         DirEntry *de_ptr, BOOL *need_dsp_help, YtreePanel *p) {
  Statistic *s = &p->vol->vol_stats;
  YtreePanel *inactive = NULL;
  DirEntry *inactive_de = NULL;
  DirEntry *inactive_fallback = NULL;
  int inactive_idx;

  /* Renamed usage: s->mode -> s->login_mode */
  if (s->login_mode != DISK_MODE && s->login_mode != USER_MODE) {
    return;
  }
  if (dir_entry->not_scanned || (dir_entry->sub_tree == NULL)) {
  } else {
    /* Capture inactive selection before mutating shared tree state. */
    if (ctx->is_split_screen) {
      inactive = (p == ctx->left) ? ctx->right : ctx->left;
      if (inactive && inactive->vol == p->vol) {
        if (inactive->vol->total_dirs > 0) {
          inactive_idx = inactive->disp_begin_pos + inactive->cursor_pos;
          if (inactive_idx < 0)
            inactive_idx = 0;
          if (inactive_idx >= inactive->vol->total_dirs)
            inactive_idx = inactive->vol->total_dirs - 1;
          inactive_de = inactive->vol->dir_entry_list[inactive_idx].dir_entry;
        } else {
          inactive_de = inactive->vol->vol_stats.tree;
        }

        inactive_fallback = inactive_de;
        while (inactive_fallback && inactive_fallback != dir_entry &&
               IsDescendant(dir_entry, inactive_fallback)) {
          inactive_fallback = inactive_fallback->up_tree;
        }
        if (!inactive_fallback)
          inactive_fallback = p->vol->vol_stats.tree;
      }
    }
    for (de_ptr = dir_entry->sub_tree; de_ptr; de_ptr = de_ptr->next) {
      UnReadTree(ctx, de_ptr, s);
    }
    dir_entry->not_scanned = TRUE;
    BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);
    BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);

    if (inactive && inactive->vol == p->vol) {
      ReanchorPanelToDir(inactive, inactive_fallback);
      BuildFileEntryList(ctx, inactive);
    }

    DisplayTree(ctx, p->vol, p->pan_dir_window, p->disp_begin_pos,
                p->disp_begin_pos + p->cursor_pos, TRUE);
    DisplayAvailBytes(ctx, s);
    RecalculateSysStats(ctx, s); /* Fix for Bug 10: Force full recalculation */
    DisplayDiskStatistic(ctx, s);
    UpdateStatsPanel(ctx, dir_entry, s);
    *need_dsp_help = TRUE;
  }
  return;
}

void HandleShowAll(ViewContext *ctx, BOOL tagged_only, DirEntry *dir_entry,
                   BOOL *need_dsp_help, int *ch, YtreePanel *p) {
  Statistic *s = &p->vol->vol_stats;

  if ((tagged_only) ? s->disk_tagged_files : s->disk_matching_files) {
    if (dir_entry->login_flag) {
      dir_entry->login_flag = FALSE;
    } else {
      dir_entry->big_window = TRUE;
      dir_entry->global_flag = TRUE;
      dir_entry->tagged_flag = tagged_only;
      dir_entry->start_file = 0;
      dir_entry->cursor_pos = 0;
    }
    if (HandleFileWindow(ctx, dir_entry) != LOGIN_ESC) {
      /* Restore normal mode and refresh the entire view */
      dir_entry->start_file = 0;
      dir_entry->cursor_pos = -1;
      BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);
      RefreshView(ctx, dir_entry);
    } else {
      BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);
      RefreshView(ctx, dir_entry);
      *ch = 'L';
    }
  } else {
    dir_entry->login_flag = FALSE;
  }
  *need_dsp_help = TRUE;
  return;
}

void HandleSwitchWindow(ViewContext *ctx, DirEntry *dir_entry,
                        BOOL *need_dsp_help, int *ch, YtreePanel *p) {
  /* Critical Safety: Check for volume changes upon return from File Window */
  struct Volume *start_vol = p->vol;
  Statistic *s = &p->vol->vol_stats;

  if (dir_entry->matching_files) {
    if (dir_entry->login_flag) {
      dir_entry->login_flag = FALSE;
    } else {
      dir_entry->global_flag = FALSE;
      dir_entry->tagged_flag = FALSE;
      dir_entry->big_window = ctx->bypass_small_window;
      dir_entry->start_file = 0;
      dir_entry->cursor_pos = 0;
    }
    {
      FILE *fp = fopen("/tmp/ytree_debug_switch.log", "a");
      if (fp) {
        fprintf(fp,
                "DEBUG: HandleSwitchWindow calling HandleFileWindow for %s\n",
                dir_entry->name);
        fclose(fp);
      }
    }
    if (HandleFileWindow(ctx, dir_entry) != LOGIN_ESC) {
      /* Safety Check: If volume was deleted in File Window (via
       * SelectLoadedVolume), abort */
      if (ctx->active->vol != start_vol)
        return;

      /* Check if the panel we were handling is still valid/active */
      if (p != ctx->active) {
        /* Split state changed (Unsplit), and this panel (Right) is dead. */
        /* Force a full menu redraw to restore borders */
        DisplayMenu(ctx);
        ctx->resize_request = TRUE; /* Force full refresh in main loop */
        /* Abort local redraw and return to main loop to handle new
         * ctx->active
         */
        return;
      }

      /* REPLACEMENT: Use Centralized Refresh */
      RefreshView(ctx, dir_entry);

    } else {
      /* ... existing LOGIN_ESC handling ... */
      BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);

      /* Ensure visual consistency here too */
      RefreshView(ctx, dir_entry);

      *ch = 'L';
    }
    DisplayAvailBytes(ctx, s);
    *need_dsp_help = TRUE;
  } else {
    dir_entry->login_flag = FALSE;
  }
  return;
}

void ToggleDotFiles(ViewContext *ctx, YtreePanel *p) {
  DirEntry *target;
  int i, found_idx = -1;
  int win_height;
  Statistic *s;
  WINDOW *win;

  if (!p || !p->vol)
    return;

  s = &p->vol->vol_stats;
  s = &p->vol->vol_stats;
  /* Removed stale 'win' caching */

  /* Suspend clock to prevent signal handler interrupt corrupting UI during
   * rebuild */
  SuspendClock(ctx);

  /* 1. Identify the directory currently under the cursor */
  if (p->vol->total_dirs > 0 &&
      (p->disp_begin_pos + p->cursor_pos < p->vol->total_dirs)) {
    target =
        p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
  } else {
    target = s->tree;
  }

  /* 2. Toggle State and Recalculate Stats */
  ctx->hide_dot_files = !ctx->hide_dot_files;
  RecalculateSysStats(ctx, s);

  /* 3. Rebuild the linear list of visible directories */
  BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);

  /* 4. Search for the 'target' directory in the new list */
  DirEntry *search = target;
  while (search != NULL && found_idx == -1) {
    for (i = 0; i < p->vol->total_dirs; i++) {
      if (p->vol->dir_entry_list[i].dir_entry == search) {
        found_idx = i;
        break;
      }
    }
    /* If the target directory is now hidden, walk up to its parent */
    if (found_idx == -1)
      search = search->up_tree;
  }

  /* 5. Smart Restore of Cursor Position */
  win_height = getmaxy(p->pan_dir_window);

  if (found_idx != -1) {
    /* Check if the found directory is within the current visible page */
    if (found_idx >= p->disp_begin_pos &&
        found_idx < p->disp_begin_pos + ctx->layout.dir_win_height) {
      /* It's still on screen. Just update the cursor, don't scroll/jump. */
      p->cursor_pos = found_idx - p->disp_begin_pos;
    } else {
      /* It moved off page. Re-center or adjust slightly. */
      if (found_idx < ctx->layout.dir_win_height) {
        p->disp_begin_pos = 0;
        p->cursor_pos = found_idx;
      } else {
        /* Center the item */
        p->disp_begin_pos = found_idx - (ctx->layout.dir_win_height / 2);

        /* Bounds check for display position */
        if (p->disp_begin_pos >
            p->vol->total_dirs - ctx->layout.dir_win_height) {
          p->disp_begin_pos = p->vol->total_dirs - ctx->layout.dir_win_height;
        }
        if (p->disp_begin_pos < 0)
          p->disp_begin_pos = 0;

        p->cursor_pos = found_idx - p->disp_begin_pos;
      }
    }
  } else {
    /* Fallback to root if everything went wrong */
    p->disp_begin_pos = 0;
    p->cursor_pos = 0;
  }

  /* Sanity check cursor limits */
  if (p->cursor_pos >= ctx->layout.dir_win_height)
    p->cursor_pos = ctx->layout.dir_win_height - 1;
  if (p->disp_begin_pos + p->cursor_pos >= p->vol->total_dirs) {
    /* Move cursor to last valid item */
    p->cursor_pos = (p->vol->total_dirs > 0)
                        ? (p->vol->total_dirs - 1 - p->disp_begin_pos)
                        : 0;
  }

  /* Refresh Directory Tree */
  DisplayTree(ctx, p->vol, p->pan_dir_window, p->disp_begin_pos,
              p->disp_begin_pos + p->cursor_pos, TRUE);
  DisplayDiskStatistic(ctx, s);

  /* Update current dir pointer using the new accessor function
  because ToggleDotFiles might have changed the list layout */
  if (p->vol->total_dirs > 0) {
    target =
        p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
  } else {
    target = s->tree;
  }

  /* Explicitly update the file window (preview) to match new visibility */
  DisplayFileWindow(ctx, p, target);
  RefreshWindow(ctx->ctx_file_window);
  UpdateStatsPanel(ctx, target, s);
  /* Update header path */
  {
    char path[PATH_LENGTH];
    GetPath(target, path);
    DisplayHeaderPath(ctx, path);
  }

  InitClock(ctx); /* Resume clock and restore signal handling */
}

/*
 * RefreshTreeSafe
 * Performs a non-destructive refresh of the directory tree.
 * Saves expansion state and tags, rescans from disk, restores state, and
 * refreshes the UI. Can be called from both Directory Window and File Window.
 */
DirEntry *RefreshTreeSafe(ViewContext *ctx, YtreePanel *p, DirEntry *entry) {
  Statistic *s;
  WINDOW *win;

  if (!p || !p->vol)
    return entry;

  s = &p->vol->vol_stats;
  s = &p->vol->vol_stats;
  /* Removed stale 'win' caching */

  werase(p->pan_dir_window);
  werase(ctx->ctx_file_window);

  /* Capture flags here to preserve state across destructive rescan */
  BOOL saved_big_window = entry->big_window;
  BOOL saved_login_flag = entry->login_flag;
  BOOL saved_global_flag = entry->global_flag;
  BOOL saved_tagged_flag = entry->tagged_flag;

  if (ctx->view_mode != ARCHIVE_MODE) {
    PathList *expanded = NULL;
    PathList *tagged = NULL;
    char saved_path[PATH_LENGTH + 1];
    int win_height;
    int dummy_width;

    GetMaxYX(p->pan_dir_window, &win_height, &dummy_width);

    /* 1. Save State */
    GetPath(entry, saved_path);
    SaveTreeState(s->tree, &expanded, &tagged);

    /* 2. Destructive Rescan */
    RescanDir(ctx, entry, strtol(TREEDEPTH, NULL, 0), s, Dir_Progress, ctx);

    /* 2a. Restore critical flags destroyed by ReadTree */
    entry->big_window = saved_big_window;
    entry->login_flag = saved_login_flag;
    entry->global_flag = saved_global_flag;
    entry->tagged_flag = saved_tagged_flag;

    /* 3. Restore State */
    RestoreTreeState(ctx, s->tree, &expanded, tagged, s);
    FreePathList(expanded);
    FreePathList(tagged);

    /* 4. Restore Selection */
    BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);

    /* Try to find the directory we were on */
    int found_idx = -1;
    int i;
    char temp_path[PATH_LENGTH + 1];
    for (i = 0; i < p->vol->total_dirs; i++) {
      GetPath(p->vol->dir_entry_list[i].dir_entry, temp_path);
      if (strcmp(temp_path, saved_path) == 0) {
        found_idx = i;
        break;
      }
    }

    if (found_idx != -1) {
      /* Restore cursor */
      if (found_idx >= p->disp_begin_pos &&
          found_idx < p->disp_begin_pos + win_height) {
        p->cursor_pos = found_idx - p->disp_begin_pos;
      } else {
        /* Move to ensure visibility */
        p->disp_begin_pos = found_idx;
        p->cursor_pos = 0;
        if (p->disp_begin_pos + win_height > p->vol->total_dirs) {
          p->disp_begin_pos = MAXIMUM(0, p->vol->total_dirs - win_height);
          p->cursor_pos = found_idx - p->disp_begin_pos;
        }
      }
      entry =
          p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
    } else {
      /* Fallback to start if dir moved/deleted */
      if (p->vol->total_dirs > 0 &&
          (p->disp_begin_pos + p->cursor_pos >= p->vol->total_dirs)) {
        p->disp_begin_pos = 0;
        p->cursor_pos = 0;
        entry = p->vol->dir_entry_list[0].dir_entry;
      }
    }
  } else {
    /* Archive Mode - Standard Rescan */
    RescanDir(ctx, entry, strtol(TREEDEPTH, NULL, 0), s, Dir_Progress, ctx);
    /* Restore flags for Archive mode too, as RescanDir/ReadTree clears them */
    entry->big_window = saved_big_window;
    entry->login_flag = saved_login_flag;
    entry->global_flag = saved_global_flag;
    entry->tagged_flag = saved_tagged_flag;

    BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);
    /* Basic bounds check */
    if (p->vol->total_dirs > 0 &&
        (p->disp_begin_pos + p->cursor_pos >= p->vol->total_dirs)) {
      p->disp_begin_pos = 0;
      p->cursor_pos = 0;
      entry = p->vol->dir_entry_list[0].dir_entry;
    }
  }

  /* Force update of free bytes info during refresh */
  (void)GetAvailBytes(&s->disk_space, s);

  DisplayTree(ctx, p->vol, p->pan_dir_window, p->disp_begin_pos,
              p->disp_begin_pos + p->cursor_pos, TRUE);
  DisplayFileWindow(ctx, p, entry);
  DisplayDiskStatistic(ctx, s);
  UpdateStatsPanel(ctx, entry, s);

  return entry;
}

int ScanSubTree(ViewContext *ctx, DirEntry *dir_entry, Statistic *s) {
  DirEntry *de_ptr;
  char new_login_path[PATH_LENGTH + 1];

  if (dir_entry->not_scanned) {
    for (de_ptr = dir_entry->sub_tree; de_ptr; de_ptr = de_ptr->next) {
      GetPath(de_ptr, new_login_path);
      if (ReadTree(ctx, de_ptr, new_login_path, 999, s, Dir_Progress, ctx) ==
          -1) {
        /* Abort signal received from ReadTree */
        return -1;
      }
      ApplyFilter(de_ptr, s);
    }
    dir_entry->not_scanned = FALSE;
  } else {
    for (de_ptr = dir_entry->sub_tree; de_ptr; de_ptr = de_ptr->next) {
      if (ScanSubTree(ctx, de_ptr, s) == -1) {
        /* Abort signal received from recursive ScanSubTree */
        return -1;
      }
    }
  }
  return (0);
}

int RefreshDirWindow(ViewContext *ctx, YtreePanel *p) {
  DirEntry *de_ptr;
  int i, n;
  int result = -1;
  int window_height;
  Statistic *s;
  WINDOW *win;

  if (!p || !p->vol)
    return -1;

  s = &p->vol->vol_stats;
  win = p->pan_dir_window;

  de_ptr = p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
  BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);

  /* Search old entry */
  for (n = -1, i = 0; i < p->vol->total_dirs; i++) {
    if (p->vol->dir_entry_list[i].dir_entry == de_ptr) {
      n = i;
      break;
    }
  }

  if (n == -1) {
    /* Directory disapeared */
    UI_Error(ctx, "", 0, "Current directory disappeared");
    result = -1;
  } else {

    if (n != (p->disp_begin_pos + p->cursor_pos)) {
      /* Position changed */
      if ((n - p->disp_begin_pos) >= 0) {
        p->cursor_pos = n - p->disp_begin_pos;
      } else {
        p->disp_begin_pos = n;
        p->cursor_pos = 0;
      }
    }

    window_height = getmaxy(win);
    while (p->cursor_pos >= window_height) {
      (p->cursor_pos)--;
      (p->disp_begin_pos)++;
    }
    DisplayTree(ctx, p->vol, win, p->disp_begin_pos,
                p->disp_begin_pos + p->cursor_pos, TRUE);

    DisplayAvailBytes(ctx, s);
    DisplayDiskStatistic(ctx, s);
    de_ptr =
        p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
    UpdateStatsPanel(ctx, de_ptr, s);
    DisplayFileWindow(ctx, p, de_ptr);
    /* Update header path after refresh */
    {
      char path[PATH_LENGTH];
      GetPath(
          p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry,
          path);
      DisplayHeaderPath(ctx, path);
    }
    result = 0;
  }

  return (result);
}
