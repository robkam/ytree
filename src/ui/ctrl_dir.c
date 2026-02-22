/***************************************************************************
 *
 * src/ui/ctrl_dir.c
 * Directory Window Controller (Input & Event Handling)
 *
 ***************************************************************************/

#include "watcher.h"
#include "ytree.h"
#include <stdio.h>

static int current_dir_entry;

static void ReadDirList(DirEntry *dir_entry, struct Volume *vol);
void BuildDirEntryList(struct Volume *vol);
void FreeDirEntryList(void);
void FreeVolumeCache(struct Volume *vol);

/* Refactored prototypes using YtreePanel */
static void HandleReadSubTree(DirEntry *dir_entry, BOOL *need_dsp_help,
                              YtreePanel *p);
static void HandleUnreadSubTree(DirEntry *dir_entry, DirEntry *de_ptr,
                                BOOL *need_dsp_help, YtreePanel *p);
static void MoveEnd(DirEntry **dir_entry, YtreePanel *p);
static void MoveHome(DirEntry **dir_entry, YtreePanel *p);
static void HandlePlus(DirEntry *dir_entry, DirEntry *de_ptr,
                       char *new_login_path, BOOL *need_dsp_help,
                       YtreePanel *p);
static void HandleTagDir(DirEntry *dir_entry, BOOL value, YtreePanel *p);
static void HandleTagAllDirs(struct Volume *vol, DirEntry *dir_entry,
                             BOOL value, YtreePanel *p);
static void HandleShowAll(BOOL tagged_only, DirEntry *dir_entry,
                          BOOL *need_dsp_help, int *ch, YtreePanel *p);
static void HandleSwitchWindow(DirEntry *dir_entry, BOOL *need_dsp_help,
                               int *ch, YtreePanel *p);
static void Movedown(DirEntry **dir_entry, YtreePanel *p);
static void Moveup(DirEntry **dir_entry, YtreePanel *p);
static void Movenpage(DirEntry **dir_entry, YtreePanel *p);
static void Moveppage(DirEntry **dir_entry, YtreePanel *p);

/* Progress callback for directory operations */
static void Dir_Progress(void *data) {
  (void)data; /* Suppress unused parameter warning */
  DrawSpinner();
}

void BuildDirEntryList(struct Volume *vol) {
  if (vol->dir_entry_list) {
    free(vol->dir_entry_list);
    vol->dir_entry_list = NULL;
    vol->dir_entry_list_capacity = 0;
  }

  /* Initialize with the estimated count, but enforce a minimum safety size */
  size_t alloc_count = vol->vol_stats.disk_total_directories;
  if (alloc_count < 16)
    alloc_count = 16;

  vol->dir_entry_list =
      (DirEntryList *)xcalloc(alloc_count, sizeof(DirEntryList));

  vol->dir_entry_list_capacity = alloc_count;
  current_dir_entry = 0;

  /* Only read if we have a valid tree structure */
  if (vol->vol_stats.tree) {
    ReadDirList(vol->vol_stats.tree, vol);
  }

  vol->total_dirs = current_dir_entry;

#ifdef DEBUG
  if (vol->vol_stats.disk_total_directories != vol->total_dirs) {
    /* mismatch detected, but safely handled by realloc in ReadDirList */
  }
#endif
}

/*
 * Frees the memory allocated for the dir_entry_list array of a volume.
 */
void FreeVolumeCache(struct Volume *vol) {
  if (vol && vol->dir_entry_list != NULL) {
    free(vol->dir_entry_list);
    vol->dir_entry_list = NULL;
    vol->dir_entry_list_capacity = 0;
    vol->total_dirs = 0;
  }
}

/*
 * Frees the memory allocated for the current volume's dir_entry_list.
 * Retained for compatibility.
 */
void FreeDirEntryList(void) {
  if (CurrentVolume) {
    FreeVolumeCache(CurrentVolume);
  }
}

static void ReadDirList(DirEntry *dir_entry, struct Volume *vol) {
  DirEntry *de_ptr;
  static int level = 0;
  static unsigned long indent = 0L;

  for (de_ptr = dir_entry; de_ptr; de_ptr = de_ptr->next) {
    /* Check visibility.
    Hide files starting with '.' if option is set.
    EXCEPTION: Never hide the top-level root directory of the current session.
    */
    if (hide_dot_files && de_ptr->name[0] == '.') {
      if (de_ptr != vol->vol_stats.tree)
        continue;
    }

    /* Bounds Checking & Dynamic Reallocation */
    if (current_dir_entry >= (int)vol->dir_entry_list_capacity) {
      size_t new_capacity = vol->dir_entry_list_capacity * 2;
      if (new_capacity == 0)
        new_capacity = 128; /* Fallback if 0 start */

      DirEntryList *new_list = (DirEntryList *)xrealloc(
          vol->dir_entry_list, new_capacity * sizeof(DirEntryList));

      /* Zero out the newly allocated portion to maintain calloc-like safety */
      memset(new_list + vol->dir_entry_list_capacity, 0,
             (new_capacity - vol->dir_entry_list_capacity) *
                 sizeof(DirEntryList));

      vol->dir_entry_list = new_list;
      vol->dir_entry_list_capacity = new_capacity;
    }

    indent &= ~(1L << level);
    if (de_ptr->next)
      indent |= (1L << level);

    vol->dir_entry_list[current_dir_entry].dir_entry = de_ptr;
    vol->dir_entry_list[current_dir_entry].level = (unsigned short)level;
    vol->dir_entry_list[current_dir_entry].indent = indent;

    current_dir_entry++;

    if (!de_ptr->not_scanned && de_ptr->sub_tree) {
      level++;
      ReadDirList(de_ptr->sub_tree, vol);
      level--;
    }
  }
}

/*
 * Helper function to return the currently selected directory entry from a
 * specific panel. Uses the panel's ViewContext (cursor_pos, disp_begin_pos)
 * instead of shared Volume stats.
 */
DirEntry *GetPanelDirEntry(YtreePanel *p) {
  if (p->vol->dir_entry_list != NULL && p->vol->total_dirs > 0) {
    /* Use Panel state directly to avoid leakage via shared Volume stats */
    int idx = p->disp_begin_pos + p->cursor_pos;

    /* Safety bounds check */
    if (idx < 0)
      idx = 0;
    if (idx >= p->vol->total_dirs)
      idx = p->vol->total_dirs - 1;

    return p->vol->dir_entry_list[idx].dir_entry;
  }
  /* Fallback to root if list is empty/invalid */
  return p->vol->vol_stats.tree;
}

/*
 * Helper function to return the currently selected directory entry.
 * Now takes a Volume context.
 */
DirEntry *GetSelectedDirEntry(struct Volume *vol) {
  if (vol->dir_entry_list != NULL && vol->total_dirs > 0) {
    int idx = vol->vol_stats.disp_begin_pos + vol->vol_stats.cursor_pos;

    /* Safety bounds check */
    if (idx < 0)
      idx = 0;
    if (idx >= vol->total_dirs)
      idx = vol->total_dirs - 1;

    return vol->dir_entry_list[idx].dir_entry;
  }
  /* Fallback to root if list is empty/invalid */
  return vol->vol_stats.tree;
}

static void Movedown(DirEntry **dir_entry, YtreePanel *p) {
  Statistic *s = &p->vol->vol_stats;
  WINDOW *win = p->pan_dir_window;

  Nav_MoveDown(&p->cursor_pos, &p->disp_begin_pos, p->vol->total_dirs,
               layout.dir_win_height, 1);

  *dir_entry =
      p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;

  if (GlobalView->refresh_mode & REFRESH_ON_NAV) {
    *dir_entry = RefreshTreeSafe(p, *dir_entry);
    /* Re-sync *dir_entry to global stats which RefreshTreeSafe might have
     * adjusted */
    *dir_entry =
        p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
  }

  (*dir_entry)->start_file = 0;
  (*dir_entry)->cursor_pos = -1;
  DisplayTree(p->vol, p->pan_dir_window, p->disp_begin_pos,
              p->disp_begin_pos + p->cursor_pos, TRUE);
  DisplayFileWindow(p, *dir_entry);
  RefreshWindow(p->pan_file_window);
  DisplayDirStatistic(*dir_entry, NULL, s);
  /* Update header path */
  {
    char path[PATH_LENGTH];
    GetPath(*dir_entry, path);
    DisplayHeaderPath(path);
  }
}

static void Moveup(DirEntry **dir_entry, YtreePanel *p) {
  Statistic *s = &p->vol->vol_stats;
  WINDOW *win = p->pan_dir_window;

  Nav_MoveUp(&p->cursor_pos, &p->disp_begin_pos);

  *dir_entry =
      p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;

  if (GlobalView->refresh_mode & REFRESH_ON_NAV) {
    *dir_entry = RefreshTreeSafe(p, *dir_entry);
    /* Re-sync *dir_entry to global stats which RefreshTreeSafe might have
     * adjusted */
    *dir_entry =
        p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
  }

  (*dir_entry)->start_file = 0;
  (*dir_entry)->cursor_pos = -1;
  DisplayTree(p->vol, p->pan_dir_window, p->disp_begin_pos,
              p->disp_begin_pos + p->cursor_pos, TRUE);
  DisplayFileWindow(p, *dir_entry);
  RefreshWindow(p->pan_file_window);
  DisplayDirStatistic(*dir_entry, NULL, s);
  /* Update header path */
  {
    char path[PATH_LENGTH];
    GetPath(*dir_entry, path);
    DisplayHeaderPath(path);
  }
}

static void Movenpage(DirEntry **dir_entry, YtreePanel *p) {
  Statistic *s = &p->vol->vol_stats;
  WINDOW *win = p->pan_dir_window;

  Nav_PageDown(&p->cursor_pos, &p->disp_begin_pos, p->vol->total_dirs,
               layout.dir_win_height);

  *dir_entry =
      p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;

  if (GlobalView->refresh_mode & REFRESH_ON_NAV) {
    *dir_entry = RefreshTreeSafe(p, *dir_entry);
    /* Re-sync *dir_entry to global stats which RefreshTreeSafe might have
     * adjusted */
    *dir_entry =
        p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
  }

  (*dir_entry)->start_file = 0;
  (*dir_entry)->cursor_pos = -1;
  DisplayTree(p->vol, p->pan_dir_window, p->disp_begin_pos,
              p->disp_begin_pos + p->cursor_pos, TRUE);
  DisplayFileWindow(p, *dir_entry);
  RefreshWindow(p->pan_file_window);
  DisplayDirStatistic(*dir_entry, NULL, s);
  /* Update header path */
  {
    char path[PATH_LENGTH];
    GetPath(*dir_entry, path);
    DisplayHeaderPath(path);
  }
}

static void Moveppage(DirEntry **dir_entry, YtreePanel *p) {
  Statistic *s = &p->vol->vol_stats;
  WINDOW *win = p->pan_dir_window;

  Nav_PageUp(&p->cursor_pos, &p->disp_begin_pos, layout.dir_win_height);

  *dir_entry =
      p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;

  if (GlobalView->refresh_mode & REFRESH_ON_NAV) {
    *dir_entry = RefreshTreeSafe(p, *dir_entry);
    /* Re-sync *dir_entry to global stats which RefreshTreeSafe might have
     * adjusted */
    *dir_entry =
        p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
  }

  (*dir_entry)->start_file = 0;
  (*dir_entry)->cursor_pos = -1;
  DisplayTree(p->vol, p->pan_dir_window, p->disp_begin_pos,
              p->disp_begin_pos + p->cursor_pos, TRUE);
  DisplayFileWindow(p, *dir_entry);
  RefreshWindow(p->pan_file_window);
  DisplayDirStatistic(*dir_entry, NULL, s);
  /* Update header path */
  {
    char path[PATH_LENGTH];
    GetPath(*dir_entry, path);
    DisplayHeaderPath(path);
  }
}

static void MoveEnd(DirEntry **dir_entry, YtreePanel *p) {
  Statistic *s = &p->vol->vol_stats;
  WINDOW *win = p->pan_dir_window;

  Nav_End(&p->cursor_pos, &p->disp_begin_pos, p->vol->total_dirs,
          layout.dir_win_height);

  *dir_entry =
      p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;

  if (GlobalView->refresh_mode & REFRESH_ON_NAV) {
    *dir_entry = RefreshTreeSafe(p, *dir_entry);
    /* Re-sync *dir_entry to global stats which RefreshTreeSafe might have
     * adjusted */
    *dir_entry =
        p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
  }

  (*dir_entry)->start_file = 0;
  (*dir_entry)->cursor_pos = -1;
  DisplayFileWindow(p, *dir_entry);
  RefreshWindow(p->pan_file_window);
  RefreshWindow(p->pan_file_window);
  DisplayTree(p->vol, p->pan_dir_window, p->disp_begin_pos,
              p->disp_begin_pos + p->cursor_pos, TRUE);
  DisplayDirStatistic(*dir_entry, NULL, s);
  /* Update header path */
  {
    char path[PATH_LENGTH];
    GetPath(*dir_entry, path);
    DisplayHeaderPath(path);
  }
  return;
}

static void MoveHome(DirEntry **dir_entry, YtreePanel *p) {
  Statistic *s = &p->vol->vol_stats;
  WINDOW *win = p->pan_dir_window;

  Nav_Home(&p->cursor_pos, &p->disp_begin_pos);

  *dir_entry =
      p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;

  if (GlobalView->refresh_mode & REFRESH_ON_NAV) {
    *dir_entry = RefreshTreeSafe(p, *dir_entry);
    /* Re-sync *dir_entry to global stats which RefreshTreeSafe might have
     * adjusted */
    *dir_entry =
        p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
  }

  (*dir_entry)->start_file = 0;
  (*dir_entry)->cursor_pos = -1;
  DisplayFileWindow(p, *dir_entry);
  RefreshWindow(p->pan_file_window);
  RefreshWindow(p->pan_file_window);
  DisplayTree(p->vol, p->pan_dir_window, p->disp_begin_pos,
              p->disp_begin_pos + p->cursor_pos, TRUE);
  DisplayDirStatistic(*dir_entry, NULL, s);
  /* Update header path */
  {
    char path[PATH_LENGTH];
    GetPath(*dir_entry, path);
    DisplayHeaderPath(path);
  }
  return;
}

static void HandlePlus(DirEntry *dir_entry, DirEntry *de_ptr,
                       char *new_login_path, BOOL *need_dsp_help,
                       YtreePanel *p) {
  Statistic *s = &p->vol->vol_stats;
  WINDOW *win = p->pan_dir_window;

  /* Renamed usage: s->mode -> s->login_mode */
  if (s->login_mode != DISK_MODE && s->login_mode != USER_MODE) {
    return;
  }
  if (!dir_entry->not_scanned) {
  } else {
    SuspendClock(); /* Suspend clock before scanning */
    for (de_ptr = dir_entry->sub_tree; de_ptr; de_ptr = de_ptr->next) {
      GetPath(de_ptr, new_login_path);
      ReadTree(de_ptr, new_login_path, 0, s, Dir_Progress, NULL);
      ApplyFilter(de_ptr, s);
    }
    InitClock(); /* Resume clock after scanning */
    dir_entry->not_scanned = FALSE;
    BuildDirEntryList(p->vol);
    BuildDirEntryList(p->vol);
    DisplayTree(p->vol, p->pan_dir_window, p->disp_begin_pos,
                p->disp_begin_pos + p->cursor_pos, TRUE);
    DisplayDiskStatistic(s);
    DisplayDirStatistic(dir_entry, NULL, s);
    DisplayAvailBytes(s);
    *need_dsp_help = TRUE;
  }
}

static void HandleReadSubTree(DirEntry *dir_entry, BOOL *need_dsp_help,
                              YtreePanel *p) {
  Statistic *s = &p->vol->vol_stats;
  WINDOW *win = p->pan_dir_window;

  SuspendClock(); /* Suspend clock before scanning */
  if (ScanSubTree(dir_entry, s) == -1) {
    /* Aborted. Fall through to refresh what we have. */
  }
  InitClock(); /* Resume clock after scanning */
  BuildDirEntryList(p->vol);
  BuildDirEntryList(p->vol);
  DisplayTree(p->vol, p->pan_dir_window, p->disp_begin_pos,
              p->disp_begin_pos + p->cursor_pos, TRUE);
  RecalculateSysStats(s); /* Fix for Bug 10: Force full recalculation */
  DisplayDiskStatistic(s);
  DisplayDirStatistic(dir_entry, NULL, s);
  DisplayAvailBytes(s);
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

static void HandleUnreadSubTree(DirEntry *dir_entry, DirEntry *de_ptr,
                                BOOL *need_dsp_help, YtreePanel *p) {
  Statistic *s = &p->vol->vol_stats;

  /* Renamed usage: s->mode -> s->login_mode */
  if (s->login_mode != DISK_MODE && s->login_mode != USER_MODE) {
    return;
  }
  if (dir_entry->not_scanned || (dir_entry->sub_tree == NULL)) {
  } else {
    /* If the inactive panel is viewing this directory or its descendants,
     * abort! */
    if (IsSplitScreen) {
      YtreePanel *inactive = (p == LeftPanel) ? RightPanel : LeftPanel;
      if (inactive && inactive->vol == p->vol) {
        DirEntry *inactive_de;
        if (inactive->vol->total_dirs > 0) {
          inactive_de = inactive->vol
                            ->dir_entry_list[inactive->disp_begin_pos +
                                             inactive->cursor_pos]
                            .dir_entry;
        } else {
          inactive_de = inactive->vol->vol_stats.tree;
        }
        if (IsDescendant(dir_entry, inactive_de)) {
          return; /* Abort collapse to prevent use-after-free in inactive panel
                   */
        }
      }
    }
    for (de_ptr = dir_entry->sub_tree; de_ptr; de_ptr = de_ptr->next) {
      UnReadTree(de_ptr, s);
    }
    dir_entry->not_scanned = TRUE;
    BuildDirEntryList(p->vol);
    BuildDirEntryList(p->vol);
    DisplayTree(p->vol, p->pan_dir_window, p->disp_begin_pos,
                p->disp_begin_pos + p->cursor_pos, TRUE);
    DisplayAvailBytes(s);
    RecalculateSysStats(s); /* Fix for Bug 10: Force full recalculation */
    DisplayDiskStatistic(s);
    DisplayDirStatistic(dir_entry, NULL, s);
    *need_dsp_help = TRUE;
  }
  return;
}

static void HandleTagDir(DirEntry *dir_entry, BOOL value, YtreePanel *p) {
  Statistic *s = &p->vol->vol_stats;
  /* WINDOW *win = p->pan_dir_window ? p->pan_dir_window : dir_window; // Not
   * used but available */

  FileEntry *fe_ptr;
  for (fe_ptr = dir_entry->file; fe_ptr; fe_ptr = fe_ptr->next) {
    /* Skip hidden dotfiles if the option is enabled */
    if (hide_dot_files && fe_ptr->name[0] == '.')
      continue;

    if ((fe_ptr->matching) && (fe_ptr->tagged != value)) {
      fe_ptr->tagged = value;
      if (value) {
        dir_entry->tagged_files++;
        dir_entry->tagged_bytes += fe_ptr->stat_struct.st_size;
        s->disk_tagged_files++;
        s->disk_tagged_bytes += fe_ptr->stat_struct.st_size;
      } else {
        dir_entry->tagged_files--;
        dir_entry->tagged_bytes -= fe_ptr->stat_struct.st_size;
        s->disk_tagged_files--;
        s->disk_tagged_bytes -= fe_ptr->stat_struct.st_size;
      }
    }
  }
  dir_entry->start_file = 0;
  dir_entry->cursor_pos = -1;
  DisplayFileWindow(p, dir_entry);
  RefreshWindow(p->pan_file_window);
  DisplayDiskStatistic(s);
  DisplayDirStatistic(dir_entry, NULL, s);
  return;
}

static void HandleTagAllDirs(struct Volume *vol, DirEntry *dir_entry,
                             BOOL value, YtreePanel *p) {
  Statistic *s = &vol->vol_stats;
  FileEntry *fe_ptr;
  long i;
  for (i = 0; i < vol->total_dirs; i++) {
    for (fe_ptr = vol->dir_entry_list[i].dir_entry->file; fe_ptr;
         fe_ptr = fe_ptr->next) {
      /* Skip hidden dotfiles if the option is enabled */
      if (hide_dot_files && fe_ptr->name[0] == '.')
        continue;

      if ((fe_ptr->matching) && (fe_ptr->tagged != value)) {
        if (value) {
          fe_ptr->tagged = value;
          dir_entry->tagged_files++;
          dir_entry->tagged_bytes += fe_ptr->stat_struct.st_size;
          s->disk_tagged_files++;
          s->disk_tagged_bytes += fe_ptr->stat_struct.st_size;
        } else {
          fe_ptr->tagged = value;
          dir_entry->tagged_files--;
          dir_entry->tagged_bytes -= fe_ptr->stat_struct.st_size;
          s->disk_tagged_files--;
          s->disk_tagged_bytes -= fe_ptr->stat_struct.st_size;
        }
      }
    }
  }
  dir_entry->start_file = 0;
  dir_entry->cursor_pos = -1;
  DisplayFileWindow(p, dir_entry);
  RefreshWindow(p->pan_file_window);
  DisplayDiskStatistic(s);
  DisplayDirStatistic(dir_entry, NULL, s);
  return;
}

static void HandleShowAll(BOOL tagged_only, DirEntry *dir_entry,
                          BOOL *need_dsp_help, int *ch, YtreePanel *p) {
  Statistic *s = &p->vol->vol_stats;
  WINDOW *win = p->pan_dir_window;

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
    if (HandleFileWindow(dir_entry) != LOGIN_ESC) {
      DisplayDiskStatistic(s);
      DisplayDirStatistic(dir_entry, NULL, s);
      dir_entry->start_file = 0;
      dir_entry->cursor_pos = -1;
      DisplayFileWindow(p, dir_entry);
      RefreshWindow(small_file_window);
      RefreshWindow(big_file_window);
      BuildDirEntryList(p->vol);

      /* FIX: Update win */
      win = p->pan_dir_window;

      DisplayTree(p->vol, win, p->disp_begin_pos,
                  p->disp_begin_pos + p->cursor_pos, TRUE);
    } else {
      BuildDirEntryList(p->vol);

      /* FIX: Update win */
      win = p->pan_dir_window;

      DisplayTree(p->vol, win, p->disp_begin_pos,
                  p->disp_begin_pos + p->cursor_pos, TRUE);
      *ch = 'L';
    }
  } else {
    dir_entry->login_flag = FALSE;
  }
  *need_dsp_help = TRUE;
  return;
}

static void HandleSwitchWindow(DirEntry *dir_entry, BOOL *need_dsp_help,
                               int *ch, YtreePanel *p) {
  /* Critical Safety: Check for volume changes upon return from File Window */
  struct Volume *start_vol = p->vol;
  Statistic *s = &p->vol->vol_stats;
  /* WINDOW *win = p->pan_dir_window; // Unused */

  if (dir_entry->matching_files) {
    if (dir_entry->login_flag) {
      dir_entry->login_flag = FALSE;
    } else {
      dir_entry->global_flag = FALSE;
      dir_entry->tagged_flag = FALSE;
      dir_entry->big_window = bypass_small_window;
      dir_entry->start_file = 0;
      dir_entry->cursor_pos = 0;
    }
    if (HandleFileWindow(dir_entry) != LOGIN_ESC) {
      /* Safety Check: If volume was deleted in File Window (via
       * SelectLoadedVolume), abort */
      if (CurrentVolume != start_vol)
        return;

      /* Check if the panel we were handling is still valid/active */
      if (p != ActivePanel) {
        /* Split state changed (Unsplit), and this panel (Right) is dead. */
        /* Force a full menu redraw to restore borders */
        DisplayMenu();
        resize_request = TRUE; /* Force full refresh in main loop */
        /* Abort local redraw and return to main loop to handle new ActivePanel
         */
        return;
      }

      /* REPLACEMENT: Use Centralized Refresh */
      RefreshGlobalView(dir_entry);

    } else {
      /* ... existing LOGIN_ESC handling ... */
      BuildDirEntryList(p->vol);

      /* Ensure visual consistency here too */
      RefreshGlobalView(dir_entry);

      *ch = 'L';
    }
    DisplayAvailBytes(s);
    *need_dsp_help = TRUE;
  } else {
    dir_entry->login_flag = FALSE;
  }
  return;
}

void ToggleDotFiles(YtreePanel *p) {
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
  SuspendClock();

  /* 1. Identify the directory currently under the cursor */
  if (p->vol->total_dirs > 0 &&
      (p->disp_begin_pos + p->cursor_pos < p->vol->total_dirs)) {
    target =
        p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
  } else {
    target = s->tree;
  }

  /* 2. Toggle State and Recalculate Stats */
  hide_dot_files = !hide_dot_files;
  RecalculateSysStats(s);

  /* 3. Rebuild the linear list of visible directories */
  BuildDirEntryList(p->vol);

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
        found_idx < p->disp_begin_pos + layout.dir_win_height) {
      /* It's still on screen. Just update the cursor, don't scroll/jump. */
      p->cursor_pos = found_idx - p->disp_begin_pos;
    } else {
      /* It moved off page. Re-center or adjust slightly. */
      if (found_idx < layout.dir_win_height) {
        p->disp_begin_pos = 0;
        p->cursor_pos = found_idx;
      } else {
        /* Center the item */
        p->disp_begin_pos = found_idx - (layout.dir_win_height / 2);

        /* Bounds check for display position */
        if (p->disp_begin_pos > p->vol->total_dirs - layout.dir_win_height) {
          p->disp_begin_pos = p->vol->total_dirs - layout.dir_win_height;
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
  if (p->cursor_pos >= layout.dir_win_height)
    p->cursor_pos = layout.dir_win_height - 1;
  if (p->disp_begin_pos + p->cursor_pos >= p->vol->total_dirs) {
    /* Move cursor to last valid item */
    p->cursor_pos = (p->vol->total_dirs > 0)
                        ? (p->vol->total_dirs - 1 - p->disp_begin_pos)
                        : 0;
  }

  /* Refresh Directory Tree */
  DisplayTree(p->vol, p->pan_dir_window, p->disp_begin_pos,
              p->disp_begin_pos + p->cursor_pos, TRUE);
  DisplayDiskStatistic(s);

  /* Update current dir pointer using the new accessor function
  because ToggleDotFiles might have changed the list layout */
  if (p->vol->total_dirs > 0) {
    target =
        p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
  } else {
    target = s->tree;
  }

  /* Explicitly update the file window (preview) to match new visibility */
  DisplayFileWindow(p, target);
  RefreshWindow(file_window);
  DisplayDirStatistic(target, NULL, s);
  /* Update header path */
  {
    char path[PATH_LENGTH];
    GetPath(target, path);
    DisplayHeaderPath(path);
  }

  InitClock(); /* Resume clock and restore signal handling */
}

/*
 * RefreshTreeSafe
 * Performs a non-destructive refresh of the directory tree.
 * Saves expansion state and tags, rescans from disk, restores state, and
 * refreshes the UI. Can be called from both Directory Window and File Window.
 */
DirEntry *RefreshTreeSafe(YtreePanel *p, DirEntry *entry) {
  Statistic *s;
  WINDOW *win;

  if (!p || !p->vol)
    return entry;

  s = &p->vol->vol_stats;
  s = &p->vol->vol_stats;
  /* Removed stale 'win' caching */

  werase(p->pan_dir_window);
  werase(file_window);
  refresh(); /* Force clear */

  /* Capture flags here to preserve state across destructive rescan */
  BOOL saved_big_window = entry->big_window;
  BOOL saved_login_flag = entry->login_flag;
  BOOL saved_global_flag = entry->global_flag;
  BOOL saved_tagged_flag = entry->tagged_flag;

  if (mode != ARCHIVE_MODE) {
    PathList *expanded = NULL;
    PathList *tagged = NULL;
    char saved_path[PATH_LENGTH + 1];
    int win_height;
    int dummy_width;

    GetMaxYX(p->pan_dir_window, &win_height, &dummy_width);

    /* 1. Save State */
    /* Save path of the *currently selected* directory to restore cursor later
     */
    /* 'entry' passed in is usually the currently selected directory */
    GetPath(entry, saved_path);
    /* Save state relative to the current cursor position or root? */
    /* Ideally save state from root to preserve everything. */
    SaveTreeState(s->tree, &expanded, &tagged);

    /* 2. Destructive Rescan */
    RescanDir(entry, strtol(TREEDEPTH, NULL, 0), s, Dir_Progress, NULL);

    /* 2a. Restore critical flags destroyed by ReadTree */
    entry->big_window = saved_big_window;
    entry->login_flag = saved_login_flag;
    entry->global_flag = saved_global_flag;
    entry->tagged_flag = saved_tagged_flag;

    /* 3. Restore State */
    RestoreTreeState(s->tree, &expanded, tagged, s);
    FreePathList(expanded);
    FreePathList(tagged);

    /* 4. Restore Selection */
    BuildDirEntryList(p->vol);

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
      /* Update entry pointer if it changed address (it shouldn't if found, but
       * good practice) */
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
    RescanDir(entry, strtol(TREEDEPTH, NULL, 0), s, Dir_Progress, NULL);
    /* Restore flags for Archive mode too, as RescanDir/ReadTree clears them */
    entry->big_window = saved_big_window;
    entry->login_flag = saved_login_flag;
    entry->global_flag = saved_global_flag;
    entry->tagged_flag = saved_tagged_flag;

    BuildDirEntryList(p->vol);
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

  DisplayTree(p->vol, p->pan_dir_window, p->disp_begin_pos,
              p->disp_begin_pos + p->cursor_pos, TRUE);
  DisplayFileWindow(p, entry);
  DisplayDiskStatistic(s);
  DisplayDirStatistic(entry, NULL, s);

  return entry;
}

int HandleDirWindow(DirEntry *start_dir_entry) {
  DirEntry *dir_entry, *de_ptr;
  int i, ch, unput_char;
  BOOL need_dsp_help;
  char new_name[PATH_LENGTH + 1];
  char new_login_path[PATH_LENGTH + 1];
  char *home;
  YtreeAction action;                       /* Declare YtreeAction variable */
  struct Volume *start_vol = CurrentVolume; /* Track global volume changes */
  Statistic *s = &CurrentVolume->vol_stats;
  int height;
  char watcher_path[PATH_LENGTH + 1];

  /* ADDED INSTRUCTION: Focus Unification */
  GlobalView->focused_window = FOCUS_TREE;

  ReCreateWindows();
  DisplayMenu();

  unput_char = 0;
  de_ptr = NULL;

  /* Safety fallback if Init() has not set up panels */
  if (ActivePanel == NULL)
    ActivePanel = LeftPanel;

  if (ActivePanel) {
    if (ActivePanel->vol != CurrentVolume) {
      ActivePanel->vol = CurrentVolume;
      ActivePanel->cursor_pos = CurrentVolume->vol_stats.cursor_pos;
      ActivePanel->disp_begin_pos = CurrentVolume->vol_stats.disp_begin_pos;
    }
    /* Ensure pointer is always fresh, but don't overwrite pos if vol is same */
    ActivePanel->vol = CurrentVolume;
    /* Ensure s points to ActivePanel's volume stats */
    s = &ActivePanel->vol->vol_stats;

    /* Update Global View Context to match Active Panel */
    /* This ensures macros like dir_window resolve to the correct ncurses window
     */
    GlobalView->ctx_dir_window = ActivePanel->pan_dir_window;
    GlobalView->ctx_small_file_window = ActivePanel->pan_small_file_window;
    GlobalView->ctx_big_file_window = ActivePanel->pan_big_file_window;
    GlobalView->ctx_file_window = ActivePanel->pan_file_window;
  }

  /* Safety Reset for Preview Mode */
  if (GlobalView->preview_mode) {
    GlobalView->preview_mode = FALSE;
    ReCreateWindows();
    DisplayMenu();
    /* Update context again after ReCreateWindows */
    if (ActivePanel) {
      GlobalView->ctx_dir_window = ActivePanel->pan_dir_window;
      GlobalView->ctx_small_file_window = ActivePanel->pan_small_file_window;
      GlobalView->ctx_big_file_window = ActivePanel->pan_big_file_window;
      GlobalView->ctx_file_window = ActivePanel->pan_file_window;
    }
  }

  height = getmaxy(dir_window);

  /* Clear flags */
  /*-----------------*/

  SetDirMode(MODE_3);

  need_dsp_help = TRUE;

  BuildDirEntryList(ActivePanel->vol);
  if (initial_directory != NULL) {
    if (!strcmp(initial_directory, ".")) /* Entry just a single "." */
    {
      ActivePanel->disp_begin_pos = 0;
      ActivePanel->cursor_pos = 0;
      unput_char = CR;
    } else {
      if (*initial_directory == '.') { /* Entry of form "./alpha/beta" */
        strcpy(new_login_path, start_dir_entry->name);
        strcat(new_login_path, initial_directory + 1);
      } else if (*initial_directory == '~' && (home = getenv("HOME"))) {
        /* Entry of form "~/alpha/beta" */
        strcpy(new_login_path, home);
        strcat(new_login_path, initial_directory + 1);
      } else { /* Entry of form "beta" or "/full/path/alpha/beta" */
        strcpy(new_login_path, initial_directory);
      }
      for (i = 0; i < ActivePanel->vol->total_dirs; i++) {
        if (*new_login_path == FILE_SEPARATOR_CHAR)
          GetPath(ActivePanel->vol->dir_entry_list[i].dir_entry, new_name);
        else
          strcpy(new_name, ActivePanel->vol->dir_entry_list[i].dir_entry->name);
        if (!strcmp(new_login_path, new_name)) {
          ActivePanel->disp_begin_pos = i;
          ActivePanel->cursor_pos = 0;
          unput_char = CR;
          break;
        }
      }
    }
    initial_directory = NULL;
  }

  {
    int safe_idx = ActivePanel->disp_begin_pos + ActivePanel->cursor_pos;
    if (ActivePanel->vol->total_dirs <= 0) {
      ActivePanel->disp_begin_pos = 0;
      ActivePanel->cursor_pos = 0;
      safe_idx = 0;
    } else if (safe_idx < 0 || safe_idx >= ActivePanel->vol->total_dirs) {
      ActivePanel->disp_begin_pos = 0;
      ActivePanel->cursor_pos = 0;
      safe_idx = 0;
    }

    if (ActivePanel->vol->dir_entry_list) {
      dir_entry = ActivePanel->vol->dir_entry_list[safe_idx].dir_entry;
    } else {
      dir_entry = ActivePanel->vol->vol_stats.tree;
    }
  }

  /* REPLACEMENT START: Unified Rendering Call */
  if (!dir_entry->login_flag) {
    dir_entry->start_file = 0;
    dir_entry->cursor_pos = -1;
  }

  RefreshGlobalView(dir_entry);
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
    if (CurrentVolume != start_vol)
      return ESC;

    if (need_dsp_help) {
      need_dsp_help = FALSE;
      DisplayDirHelp();
    }
    DisplayDirParameter(dir_entry);
    RefreshWindow(dir_window);

    if (IsSplitScreen) {
      YtreePanel *inactive =
          (ActivePanel == LeftPanel) ? RightPanel : LeftPanel;
      RenderInactivePanel(inactive);
    }

    if (s->login_mode == DISK_MODE || s->login_mode == USER_MODE) {
      if (GlobalView->refresh_mode & REFRESH_WATCHER) {
        GetPath(dir_entry, watcher_path);
        Watcher_SetDir(watcher_path);
      }
    }

    if (unput_char) {
      ch = unput_char;
      unput_char = '\0';
    } else {
      doupdate();
      ch = (resize_request) ? -1 : GetEventOrKey();
      /* LF to CR normalization is now handled by GetKeyAction */
    }

    if (IsUserActionDefined()) { /* User commands take precedence */
      ch = DirUserMode(dir_entry, ch, &ActivePanel->vol->vol_stats);
    }

    /* ViKey processing is now handled inside GetKeyAction */

    if (resize_request) {
      /* SIMPLIFIED RESIZE: Just call Global Refresh */
      RefreshGlobalView(dir_entry);
      need_dsp_help = TRUE;
      resize_request = FALSE;
    }

    action = GetKeyAction(ch); /* Translate raw input to YtreeAction */

    switch (action) {
    case ACTION_RESIZE:
      resize_request = TRUE;
      break;

    case ACTION_TOGGLE_STATS:
      GlobalView->show_stats = !GlobalView->show_stats;
      resize_request = TRUE;
      break;

    case ACTION_VIEW_PREVIEW: {
      YtreePanel *saved_panel = ActivePanel;

      /* Save Preview State - BEFORE potential panel switching */
      GlobalView->preview_return_panel = ActivePanel;
      GlobalView->preview_return_focus = GlobalView->focused_window;

      GlobalView->preview_mode = TRUE;
      /* ADDED INSTRUCTION: Track Entry Point */
      GlobalView->preview_entry_focus = FOCUS_TREE;

      /* Enter File Window loop via HandleSwitchWindow logic */
      /* We use HandleSwitchWindow to encapsulate the setup for HandleFileWindow
       */
      HandleSwitchWindow(dir_entry, &need_dsp_help, &ch, ActivePanel);

      /* Post-Return Cleanup */
      GlobalView->preview_mode =
          FALSE; /* Restore cleaning of preview mode flag */
      GlobalView->focused_window =
          FOCUS_TREE; /* Restore focus to tree on return */

      if (ActivePanel != saved_panel) {
        /* If panel was switched during preview, we REFRESH local state
         * BEFORE calling RefreshGlobalView to ensure the correct entry is
         * drawn. */
        start_vol = ActivePanel->vol;
        s = &ActivePanel->vol->vol_stats;

        if (ActivePanel->vol && ActivePanel->vol->total_dirs > 0) {
          dir_entry = ActivePanel->vol
                          ->dir_entry_list[ActivePanel->disp_begin_pos +
                                           ActivePanel->cursor_pos]
                          .dir_entry;
        } else {
          dir_entry = s->tree;
        }

        /* Sync Global View Context */
        GlobalView->ctx_dir_window = ActivePanel->pan_dir_window;
        GlobalView->ctx_small_file_window = ActivePanel->pan_small_file_window;
        GlobalView->ctx_big_file_window = ActivePanel->pan_big_file_window;
        GlobalView->ctx_file_window = ActivePanel->pan_file_window;

        /* EXPLICIT REFRESH with the CORRECT dir_entry */
        RefreshGlobalView(dir_entry);

        need_dsp_help = TRUE;
        continue;
      }

      /* Standard refresh if panel didn't change */
      RefreshGlobalView(dir_entry);

      need_dsp_help = TRUE;

      /* Refresh local pointer */
      if (ActivePanel->vol->total_dirs > 0) {
        dir_entry = ActivePanel->vol
                        ->dir_entry_list[ActivePanel->disp_begin_pos +
                                         ActivePanel->cursor_pos]
                        .dir_entry;
      } else {
        dir_entry = s->tree;
      }
    } break;

    case ACTION_SPLIT_SCREEN:
      if (IsSplitScreen && ActivePanel == RightPanel) {
        /* Preserve Right Panel state into Left Panel before unsplitting */
        LeftPanel->vol = RightPanel->vol;
        LeftPanel->cursor_pos = RightPanel->cursor_pos;
        LeftPanel->disp_begin_pos = RightPanel->disp_begin_pos;
        /* Sync Global Volume */
        CurrentVolume = LeftPanel->vol;
      }

      IsSplitScreen = !IsSplitScreen;
      ReCreateWindows(); /* Force layout update immediately */

      if (IsSplitScreen) {
        /* Explicitly copy state here because we won't hit the dirwin logic */
        YtreePanel *source = ActivePanel;
        YtreePanel *target =
            (ActivePanel == LeftPanel) ? RightPanel : LeftPanel;

        target->vol = source->vol;
        target->cursor_pos = source->cursor_pos;
        target->disp_begin_pos = source->disp_begin_pos;
        target->pan_file_window = target->pan_small_file_window;
        target->file_mode = source->file_mode;
        target->reverse_sort = source->reverse_sort;

        /* Bug 1 Fix: Explicitly populate the file entry list for the target
         * panel. This ensures the small file window is not empty on the right
         * side. */
        BuildFileEntryList(target);
      }

      resize_request = TRUE;
      DisplayMenu();
      break;

    case ACTION_SWITCH_PANEL:
      if (!IsSplitScreen)
        break;

      /* Save to Vol for persistence */
      ActivePanel->vol->vol_stats.cursor_pos = ActivePanel->cursor_pos;
      ActivePanel->vol->vol_stats.disp_begin_pos = ActivePanel->disp_begin_pos;

      /* Switch Panel */
      if (ActivePanel == LeftPanel) {
        ActivePanel = RightPanel;
      } else {
        ActivePanel = LeftPanel;
      }

      /* Restore Volume context */
      CurrentVolume = ActivePanel->vol;
      s = &ActivePanel->vol->vol_stats; /* UPDATE S */
      /* Do NOT overwrite ActivePanel->cursor_pos here; preserve its independent
       * state */

      /* Recalculate local dir_entry for the new ActivePanel */
      if (ActivePanel->vol->total_dirs > 0) {
        /* Clamp coordinates just in case */
        if (ActivePanel->disp_begin_pos + ActivePanel->cursor_pos >=
            ActivePanel->vol->total_dirs) {
          ActivePanel->cursor_pos =
              ActivePanel->vol->total_dirs - 1 - ActivePanel->disp_begin_pos;
        }
        dir_entry = ActivePanel->vol
                        ->dir_entry_list[ActivePanel->disp_begin_pos +
                                         ActivePanel->cursor_pos]
                        .dir_entry;
      } else {
        dir_entry = s->tree;
      }

      /* Sync Global View Context */
      GlobalView->ctx_dir_window = ActivePanel->pan_dir_window;
      GlobalView->ctx_small_file_window = ActivePanel->pan_small_file_window;
      GlobalView->ctx_big_file_window = ActivePanel->pan_big_file_window;
      GlobalView->ctx_file_window = ActivePanel->pan_file_window;

      /* REFRESH View for new active panel */
      RefreshGlobalView(dir_entry);
      need_dsp_help = TRUE;
      continue; /* Stay in loop with new context */

    case ACTION_NONE: /* -1 or unhandled keys */
      if (ch == -1)
        break; /* Ignore -1 (resize_request handled above) */
      /* Fall through for other unhandled keys to beep */
      beep();
      break;

    case ACTION_MOVE_DOWN:
      Movedown(&dir_entry, ActivePanel);
      break;
    case ACTION_MOVE_UP:
      Moveup(&dir_entry, ActivePanel);
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

        for (k = 0; k < ActivePanel->vol->total_dirs; k++) {
          if (ActivePanel->vol->dir_entry_list[k].dir_entry == target) {
            found_idx = k;
            break;
          }
        }

        if (found_idx != -1) {
          /* Move cursor to sibling */
          if (found_idx >= ActivePanel->disp_begin_pos &&
              found_idx < ActivePanel->disp_begin_pos + height) {
            ActivePanel->cursor_pos = found_idx - ActivePanel->disp_begin_pos;
          } else {
            /* Off screen, center it or move to top */
            ActivePanel->disp_begin_pos = found_idx;
            ActivePanel->cursor_pos = 0;
            /* Bounds check */
            if (ActivePanel->disp_begin_pos + height >
                ActivePanel->vol->total_dirs) {
              ActivePanel->disp_begin_pos =
                  MAXIMUM(0, ActivePanel->vol->total_dirs - height);
              ActivePanel->cursor_pos = found_idx - ActivePanel->disp_begin_pos;
            }
          }
          /* Sync */
          dir_entry = ActivePanel->vol
                          ->dir_entry_list[ActivePanel->disp_begin_pos +
                                           ActivePanel->cursor_pos]
                          .dir_entry;

          if (GlobalView->refresh_mode & REFRESH_ON_NAV) {
            dir_entry = RefreshTreeSafe(ActivePanel, dir_entry);
            break; /* Skip manual refresh logic below */
          }

          /* Refresh */
          DisplayTree(ActivePanel->vol, dir_window, ActivePanel->disp_begin_pos,
                      ActivePanel->disp_begin_pos + ActivePanel->cursor_pos,
                      TRUE);
          DisplayFileWindow(ActivePanel, dir_entry);
          DisplayDiskStatistic(s);
          DisplayDirStatistic(dir_entry, NULL, s);
          DisplayAvailBytes(s);

          char path[PATH_LENGTH];
          GetPath(dir_entry, path);
          DisplayHeaderPath(path);
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

        for (k = 0; k < ActivePanel->vol->total_dirs; k++) {
          if (ActivePanel->vol->dir_entry_list[k].dir_entry == target) {
            found_idx = k;
            break;
          }
        }

        if (found_idx != -1) {
          /* Move cursor to sibling */
          if (found_idx >= ActivePanel->disp_begin_pos &&
              found_idx < ActivePanel->disp_begin_pos + height) {
            ActivePanel->cursor_pos = found_idx - ActivePanel->disp_begin_pos;
          } else {
            /* Off screen, center it or move to top */
            ActivePanel->disp_begin_pos = found_idx;
            ActivePanel->cursor_pos = 0;
            /* Bounds check */
            if (ActivePanel->disp_begin_pos + height >
                ActivePanel->vol->total_dirs) {
              ActivePanel->disp_begin_pos =
                  MAXIMUM(0, ActivePanel->vol->total_dirs - height);
              ActivePanel->cursor_pos = found_idx - ActivePanel->disp_begin_pos;
            }
          }
          /* Sync */
          dir_entry = ActivePanel->vol
                          ->dir_entry_list[ActivePanel->disp_begin_pos +
                                           ActivePanel->cursor_pos]
                          .dir_entry;

          if (GlobalView->refresh_mode & REFRESH_ON_NAV) {
            dir_entry = RefreshTreeSafe(ActivePanel, dir_entry);
            break; /* Skip manual refresh logic below */
          }

          /* Refresh */
          DisplayTree(ActivePanel->vol, dir_window, ActivePanel->disp_begin_pos,
                      ActivePanel->disp_begin_pos + ActivePanel->cursor_pos,
                      TRUE);
          DisplayFileWindow(ActivePanel, dir_entry);
          DisplayDiskStatistic(s);
          DisplayDirStatistic(dir_entry, NULL, s);
          DisplayAvailBytes(s);

          char path[PATH_LENGTH];
          GetPath(dir_entry, path);
          DisplayHeaderPath(path);
        }
      }
    }
      need_dsp_help = TRUE;
      break;
    case ACTION_PAGE_DOWN:
      Movenpage(&dir_entry, ActivePanel);
      break;
    case ACTION_PAGE_UP:
      Moveppage(&dir_entry, ActivePanel);
      break;
    case ACTION_HOME:
      MoveHome(&dir_entry, ActivePanel);
      break;
    case ACTION_END:
      MoveEnd(&dir_entry, ActivePanel);
      break;
    case ACTION_MOVE_RIGHT:
    case ACTION_TREE_EXPAND_ALL:
      HandlePlus(dir_entry, de_ptr, new_login_path, &need_dsp_help,
                 ActivePanel);
      break;
    case ACTION_ASTERISK:
      HandleReadSubTree(dir_entry, &need_dsp_help, ActivePanel);
      break;
    case ACTION_TREE_EXPAND:
      HandleReadSubTree(dir_entry, &need_dsp_help, ActivePanel);
      break;
    case ACTION_MOVE_LEFT:
      /* 1. Transparent Archive Exit Logic (At Root) */
      if (dir_entry->up_tree == NULL && mode == ARCHIVE_MODE) {
        struct Volume *old_vol = CurrentVolume;
        char archive_path[PATH_LENGTH + 1];
        char parent_dir[PATH_LENGTH + 1];
        char dummy_name[PATH_LENGTH + 1];

        /* Calculate Parent Directory of the Archive File */
        strcpy(archive_path, s->login_path);
        Fnsplit(archive_path, parent_dir, dummy_name);

        /* Force Login/Switch to the Parent Directory */
        /* This handles both "New Volume" and "Switch to Existing" logic
         * automatically */
        if (LogDisk(parent_dir) == 0) {
          /* Successfully switched context. */

          /* Delete the archive wrapper we just left to clean up memory/list */
          /* Fix Archive Double-Free Check */
          BOOL vol_in_use = FALSE;
          if (IsSplitScreen) {
            YtreePanel *other =
                (ActivePanel == LeftPanel) ? RightPanel : LeftPanel;
            if (other->vol == old_vol)
              vol_in_use = TRUE;
          }
          if (!vol_in_use)
            Volume_Delete(old_vol);

          /* Update pointers for the new context */
          s = &CurrentVolume->vol_stats;    /* UPDATE S */
          start_vol = CurrentVolume;        /* Update loop safety variable */
          ActivePanel->vol = CurrentVolume; /* Update Panel volume */

          /* Reset Search Term to prevent bleeding */
          GlobalSearchTerm[0] = '\0';

          /* Attempt to highlight the archive file we just left */
          if (CurrentVolume->total_dirs > 0) {
            /* Usually root, or restored position */
            dir_entry = CurrentVolume
                            ->dir_entry_list[ActivePanel->disp_begin_pos +
                                             ActivePanel->cursor_pos]
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
          GlobalView->view_mode = DISK_MODE;
          mode = DISK_MODE; /* Force legacy macro to update too */

          /* Force layout reset to small window split view */
          SwitchToSmallFileWindow();

          /* Force full screen clear to fix UI artifacts (separator line) */
          werase(file_window); /* Erase file window specifically */
          clearok(stdscr, TRUE);
          refresh();

          /* Refresh Full UI */
          DisplayMenu();
          DisplayTree(CurrentVolume, dir_window, ActivePanel->disp_begin_pos,
                      ActivePanel->disp_begin_pos + ActivePanel->cursor_pos,
                      TRUE);
          DisplayFileWindow(ActivePanel, dir_entry);
          DisplayDiskStatistic(s);
          DisplayDirStatistic(dir_entry, NULL, s);
          DisplayAvailBytes(s);

          {
            char path[PATH_LENGTH];
            GetPath(dir_entry, path);
            DisplayHeaderPath(path);
          }
          need_dsp_help = TRUE;
          break; /* Exit case done */
        }
      }

      /* 2. Standard Tree Navigation Logic */
      if (!dir_entry->not_scanned && dir_entry->sub_tree != NULL) {
        /* It is expanded */
        if (mode == ARCHIVE_MODE) {
          /* In Archive Mode, we cannot collapse (UnRead) without data loss.
          So we skip collapse and fall through to Jump to Parent. */
          goto JUMP_TO_PARENT;
        } else {
          /* In FS Mode, collapse it. */
          HandleUnreadSubTree(dir_entry, de_ptr, &need_dsp_help, ActivePanel);
        }
      } else {
        /* It is collapsed (or leaf) -> Jump to Parent */
      JUMP_TO_PARENT:
        if (dir_entry->up_tree != NULL) {
          /* Find parent in the list */
          int p_idx = -1;
          int k;
          for (k = 0; k < ActivePanel->vol->total_dirs; k++) {
            if (ActivePanel->vol->dir_entry_list[k].dir_entry ==
                dir_entry->up_tree) {
              p_idx = k;
              break;
            }
          }
          if (p_idx != -1) {
            /* Move cursor to parent */
            if (p_idx >= ActivePanel->disp_begin_pos &&
                p_idx < ActivePanel->disp_begin_pos + height) {
              /* Parent is on screen */
              ActivePanel->cursor_pos = p_idx - ActivePanel->disp_begin_pos;
            } else {
              /* Parent is off screen - center it or move to top */
              ActivePanel->disp_begin_pos = p_idx;
              ActivePanel->cursor_pos = 0;
              /* Adjust if near end */
              if (ActivePanel->disp_begin_pos + height >
                  ActivePanel->vol->total_dirs) {
                ActivePanel->disp_begin_pos =
                    MAXIMUM(0, ActivePanel->vol->total_dirs - height);
                ActivePanel->cursor_pos = p_idx - ActivePanel->disp_begin_pos;
              }
            }
            /* Sync pointers */
            dir_entry = ActivePanel->vol
                            ->dir_entry_list[ActivePanel->disp_begin_pos +
                                             ActivePanel->cursor_pos]
                            .dir_entry;

            /* Refresh */
            DisplayTree(
                ActivePanel->vol, dir_window, ActivePanel->disp_begin_pos,
                ActivePanel->disp_begin_pos + ActivePanel->cursor_pos, TRUE);
            DisplayFileWindow(ActivePanel, dir_entry);
            DisplayDiskStatistic(s);
            DisplayDirStatistic(dir_entry, NULL, s);
            DisplayAvailBytes(s);
            /* Update Header Path */
            char path[PATH_LENGTH];
            GetPath(dir_entry, path);
            DisplayHeaderPath(path);
          }
        }
      }
      break;
    case ACTION_TREE_COLLAPSE:
      HandleUnreadSubTree(dir_entry, de_ptr, &need_dsp_help, ActivePanel);
      break;
    case ACTION_TOGGLE_HIDDEN: {
      ToggleDotFiles(ActivePanel);

      /* Update current dir pointer using the new accessor function
      because ToggleDotFiles might have changed the list layout */
      if (ActivePanel->vol->total_dirs > 0) {
        dir_entry = ActivePanel->vol
                        ->dir_entry_list[ActivePanel->disp_begin_pos +
                                         ActivePanel->cursor_pos]
                        .dir_entry;
      } else {
        dir_entry = s->tree;
      }

      need_dsp_help = TRUE;
    } break;
    case ACTION_FILTER:
      if (UI_ReadFilter() == 0) {
        dir_entry->start_file = 0;
        dir_entry->cursor_pos = -1;
        DisplayFileWindow(ActivePanel, dir_entry);
        RefreshWindow(file_window);
        DisplayDiskStatistic(s);
        DisplayDirStatistic(dir_entry, NULL, s);
      }
      need_dsp_help = TRUE;
      break;
    case ACTION_TAG:
      HandleTagDir(dir_entry, TRUE, ActivePanel);
      Movedown(&dir_entry, ActivePanel); /* ADDED */
      break;

    case ACTION_UNTAG:
      HandleTagDir(dir_entry, FALSE, ActivePanel);
      Movedown(&dir_entry, ActivePanel); /* ADDED */
      break;
    case ACTION_TAG_ALL:
      HandleTagAllDirs(CurrentVolume, dir_entry, TRUE, ActivePanel);
      break;
    case ACTION_UNTAG_ALL:
      HandleTagAllDirs(CurrentVolume, dir_entry, FALSE, ActivePanel);
      break;
    case ACTION_TOGGLE_MODE:
      RotateDirMode();
      /*DisplayFileWindow( dir_entry, 0, -1 );*/
      DisplayTree(ActivePanel->vol, dir_window, ActivePanel->disp_begin_pos,
                  ActivePanel->disp_begin_pos + ActivePanel->cursor_pos, TRUE);
      /*RefreshWindow( file_window );*/
      DisplayDiskStatistic(s);
      DisplayDirStatistic(dir_entry, NULL, s);
      need_dsp_help = TRUE;
      break;
    case ACTION_CMD_TAGGED_S:
      HandleShowAll(TRUE, dir_entry, &need_dsp_help, &ch, ActivePanel);
      break;

    case ACTION_CMD_S:
      HandleShowAll(FALSE, dir_entry, &need_dsp_help, &ch, ActivePanel);
      break;
    case ACTION_ENTER:
      if (GlobalView->refresh_mode & REFRESH_ON_ENTER) {
        dir_entry = RefreshTreeSafe(ActivePanel, dir_entry);
        /* Sync pointer from list in case address changed */
        dir_entry = ActivePanel->vol
                        ->dir_entry_list[ActivePanel->disp_begin_pos +
                                         ActivePanel->cursor_pos]
                        .dir_entry;
      }
      /* Fix Context Safety on Return */
      {
        YtreePanel *saved_panel = ActivePanel;

        /* ADDED INSTRUCTION: Track Entry Point */
        GlobalView->preview_entry_focus = FOCUS_FILE;

        HandleSwitchWindow(dir_entry, &need_dsp_help, &ch, ActivePanel);

        /* Restore focus to tree on return */
        GlobalView->focused_window = FOCUS_TREE;

        if (ActivePanel != saved_panel) {
          /* If panel was switched, refresh state locally and continue */
          start_vol = ActivePanel->vol;
          s = &ActivePanel->vol->vol_stats;

          if (ActivePanel->vol && ActivePanel->vol->total_dirs > 0) {
            dir_entry = ActivePanel->vol
                            ->dir_entry_list[ActivePanel->disp_begin_pos +
                                             ActivePanel->cursor_pos]
                            .dir_entry;
          } else {
            dir_entry = s->tree;
          }

          GlobalView->ctx_dir_window = ActivePanel->pan_dir_window;
          GlobalView->ctx_small_file_window =
              ActivePanel->pan_small_file_window;
          GlobalView->ctx_big_file_window = ActivePanel->pan_big_file_window;
          GlobalView->ctx_file_window = ActivePanel->pan_file_window;

          RefreshGlobalView(dir_entry);
          need_dsp_help = TRUE;
          continue;
        }

        /* Important: Check for volume changes after returning from File Window
         */
        if (CurrentVolume != start_vol)
          return ESC;

        /* CENTRALIZED REFRESH: Restore clean layout after return */
        RefreshGlobalView(dir_entry);
      }
      /* Refresh local pointer */
      dir_entry = ActivePanel->vol
                      ->dir_entry_list[ActivePanel->disp_begin_pos +
                                       ActivePanel->cursor_pos]
                      .dir_entry;
      break;
    case ACTION_CMD_X:
      if (mode != DISK_MODE && mode != USER_MODE) {
      } else {
        char command_template[COMMAND_LINE_LENGTH + 1];
        command_template[0] = '\0';
        if (GetCommandLine(command_template) == 0) {
          (void)Execute(dir_entry, NULL, command_template,
                        &ActivePanel->vol->vol_stats, UI_ArchiveCallback);
          dir_entry = RefreshTreeSafe(
              ActivePanel, dir_entry); /* Auto-Refresh after command */
        }
      }
      need_dsp_help = TRUE;
      DisplayAvailBytes(s);
      DisplayDiskStatistic(s);
      DisplayDirStatistic(dir_entry, NULL, s);
      break;
    case ACTION_CMD_MKFILE:
      if (mode != DISK_MODE)
        break;
      {
        char file_name[PATH_LENGTH * 2 + 1];
        int mk_result;

        ClearHelp();
        *file_name = '\0';
        if (UI_ReadString("MAKE FILE:", file_name, PATH_LENGTH, HST_FILE) ==
            CR) {
          mk_result =
              MakeFile(dir_entry, file_name, s, NULL, UI_ChoiceResolver);
          if (mk_result == 0) {
            /* Determine where the new file should be. */
            if (ActivePanel && ActivePanel->pan_file_window) {
              /* Force file window refresh */
              DisplayFileWindow(ActivePanel, dir_entry);
            }
            RefreshGlobalView(dir_entry);
          } else if (mk_result == 1) {
            MESSAGE("File already exists!");
          } else {
            MESSAGE("Can't create File*\"%s\"", file_name);
          }
        }
      }
      need_dsp_help = TRUE;
      break;

    case ACTION_CMD_M: {
      char dir_name[PATH_LENGTH * 2 + 1];
      ClearHelp();
      *dir_name = '\0';
      if (UI_ReadString("MAKE DIRECTORY:", dir_name, PATH_LENGTH, HST_FILE) ==
          CR) {
        if (!MakeDirectory(dir_entry, dir_name, s)) {
          BuildDirEntryList(ActivePanel->vol);
          RefreshGlobalView(dir_entry);
        }
      }
      move(LINES - 2, 1);
      clrtoeol();
    }
      need_dsp_help = TRUE;
      break;
    case ACTION_CMD_D:
      if (!DeleteDirectory(dir_entry, UI_ChoiceResolver)) {
        if (ActivePanel->disp_begin_pos + ActivePanel->cursor_pos > 0) {
          if (ActivePanel->cursor_pos > 0)
            ActivePanel->cursor_pos--;
          else
            ActivePanel->disp_begin_pos--;
        }
      }
      /* Update regardless of success */
      BuildDirEntryList(ActivePanel->vol);
      dir_entry = ActivePanel->vol
                      ->dir_entry_list[ActivePanel->disp_begin_pos +
                                       ActivePanel->cursor_pos]
                      .dir_entry;
      dir_entry->start_file = 0;
      dir_entry->cursor_pos = -1;

      RefreshGlobalView(dir_entry);
      need_dsp_help = TRUE;
      break;

    case ACTION_CMD_R:
      if (!GetRenameParameter(dir_entry->name, new_name)) {
        if (!RenameDirectory(dir_entry, new_name)) {
          /* Rename OK */
          BuildDirEntryList(ActivePanel->vol);
          dir_entry = ActivePanel->vol
                          ->dir_entry_list[ActivePanel->disp_begin_pos +
                                           ActivePanel->cursor_pos]
                          .dir_entry;
          RefreshGlobalView(dir_entry);
        }
      }
      need_dsp_help = TRUE;
      break;
    case ACTION_REFRESH: /* Rescan */
      dir_entry = RefreshTreeSafe(ActivePanel, dir_entry);
      need_dsp_help = TRUE;
      break;
    case ACTION_CMD_G:
      (void)ChangeDirGroup(dir_entry);
      DisplayTree(ActivePanel->vol, dir_window, ActivePanel->disp_begin_pos,
                  ActivePanel->disp_begin_pos + ActivePanel->cursor_pos, TRUE);
      DisplayDiskStatistic(s);
      DisplayDirStatistic(dir_entry, NULL, s);
      need_dsp_help = TRUE;
      break;
    case ACTION_CMD_O:
      (void)ChangeDirOwner(dir_entry);
      DisplayTree(ActivePanel->vol, dir_window, ActivePanel->disp_begin_pos,
                  ActivePanel->disp_begin_pos + ActivePanel->cursor_pos, TRUE);
      DisplayDiskStatistic(s);
      DisplayDirStatistic(dir_entry, NULL, s);
      need_dsp_help = TRUE;
      break;
    case ACTION_CMD_A:
      (void)ChangeDirModus(dir_entry);
      DisplayTree(ActivePanel->vol, dir_window, ActivePanel->disp_begin_pos,
                  ActivePanel->disp_begin_pos + ActivePanel->cursor_pos, TRUE);
      DisplayDiskStatistic(s);
      DisplayDirStatistic(dir_entry, NULL, s);
      need_dsp_help = TRUE;
      break;

    case ACTION_TOGGLE_COMPACT:
      GlobalView->fixed_col_width = (GlobalView->fixed_col_width == 0) ? 32 : 0;
      resize_request = TRUE;
      break;

    case ACTION_CMD_P: /* Pipe Directory */
    {
      char pipe_cmd[PATH_LENGTH + 1];
      pipe_cmd[0] = '\0';
      if (GetPipeCommand(pipe_cmd) == 0) {
        PipeDirectory(dir_entry, pipe_cmd);
      }
    }
      need_dsp_help = TRUE;
      break;

    /* Volume Cycling and Selection */
    case ACTION_VOL_MENU: /* Shift-K: Select Loaded Volume */
    {
      /* Save current panel state to volume before switching away */
      ActivePanel->vol->vol_stats.cursor_pos = ActivePanel->cursor_pos;
      ActivePanel->vol->vol_stats.disp_begin_pos = ActivePanel->disp_begin_pos;

      int res = SelectLoadedVolume(NULL);
      if (res == 0) { /* If volume switch was successful */
        if (CurrentVolume != start_vol)
          return ESC; /* Abort to main loop to handle clean re-entry */

        /* Update loop variables for new volume */
        s = &CurrentVolume->vol_stats; /* UPDATE S */
        ActivePanel->vol = CurrentVolume;
        ActivePanel->cursor_pos = CurrentVolume->vol_stats.cursor_pos;
        ActivePanel->disp_begin_pos = CurrentVolume->vol_stats.disp_begin_pos;

        /* Safety check / Clamping */
        if (ActivePanel->vol->total_dirs > 0) {
          /* If saved position is beyond current end, clamp to last item */
          if (ActivePanel->disp_begin_pos + ActivePanel->cursor_pos >=
              ActivePanel->vol->total_dirs) {
            /* Clamp to last valid index */
            int last_idx = ActivePanel->vol->total_dirs - 1;
            /* Determine new disp_begin_pos and cursor_pos */
            if (last_idx >= layout.dir_win_height) {
              ActivePanel->disp_begin_pos =
                  last_idx - (layout.dir_win_height - 1);
              ActivePanel->cursor_pos = layout.dir_win_height - 1;
            } else {
              ActivePanel->disp_begin_pos = 0;
              ActivePanel->cursor_pos = last_idx;
            }
          }
          /* Now safe to assign dir_entry */
          dir_entry = ActivePanel->vol
                          ->dir_entry_list[ActivePanel->disp_begin_pos +
                                           ActivePanel->cursor_pos]
                          .dir_entry;
        } else {
          dir_entry = s->tree;
        }

        DisplayMenu(); /* Force redraw of frame/separator */
        DisplayTree(ActivePanel->vol, dir_window, ActivePanel->disp_begin_pos,
                    ActivePanel->disp_begin_pos + ActivePanel->cursor_pos,
                    TRUE);
        DisplayFileWindow(
            ActivePanel,
            dir_entry); /* Refresh file window for the new directory */
        RefreshWindow(file_window);
        DisplayDiskStatistic(s);
        DisplayDirStatistic(dir_entry, NULL, s);
        DisplayAvailBytes(s);
        /* Update header path after volume switch */
        {
          char path[PATH_LENGTH];
          GetPath(dir_entry, path);
          DisplayHeaderPath(path);
        }
        need_dsp_help = TRUE;
      }
    } break;

    case ACTION_VOL_PREV: /* Previous Volume */
    {
      /* Save current panel state to volume before switching away */
      ActivePanel->vol->vol_stats.cursor_pos = ActivePanel->cursor_pos;
      ActivePanel->vol->vol_stats.disp_begin_pos = ActivePanel->disp_begin_pos;

      int res = CycleLoadedVolume(-1);
      if (res == 0) { /* If volume switch was successful */
        if (CurrentVolume != start_vol)
          return ESC;

        s = &CurrentVolume->vol_stats; /* UPDATE S */
        ActivePanel->vol = CurrentVolume;
        ActivePanel->cursor_pos = CurrentVolume->vol_stats.cursor_pos;
        ActivePanel->disp_begin_pos = CurrentVolume->vol_stats.disp_begin_pos;

        /* Safety check / Clamping */
        if (ActivePanel->vol->total_dirs > 0) {
          if (ActivePanel->disp_begin_pos + ActivePanel->cursor_pos >=
              ActivePanel->vol->total_dirs) {
            int last_idx = ActivePanel->vol->total_dirs - 1;
            if (last_idx >= layout.dir_win_height) {
              ActivePanel->disp_begin_pos =
                  last_idx - (layout.dir_win_height - 1);
              ActivePanel->cursor_pos = layout.dir_win_height - 1;
            } else {
              ActivePanel->disp_begin_pos = 0;
              ActivePanel->cursor_pos = last_idx;
            }
          }
          dir_entry = ActivePanel->vol
                          ->dir_entry_list[ActivePanel->disp_begin_pos +
                                           ActivePanel->cursor_pos]
                          .dir_entry;
        } else {
          dir_entry = s->tree;
        }

        DisplayMenu();
        DisplayTree(ActivePanel->vol, dir_window, ActivePanel->disp_begin_pos,
                    ActivePanel->disp_begin_pos + ActivePanel->cursor_pos,
                    TRUE);
        DisplayFileWindow(ActivePanel, dir_entry);
        RefreshWindow(file_window);
        DisplayDiskStatistic(s);
        DisplayDirStatistic(dir_entry, NULL, s);
        DisplayAvailBytes(s);
        {
          char path[PATH_LENGTH];
          GetPath(dir_entry, path);
          DisplayHeaderPath(path);
        }
        need_dsp_help = TRUE;
      }
    } break;

    case ACTION_VOL_NEXT: /* Next Volume */
    {
      /* Save current panel state to volume before switching away */
      ActivePanel->vol->vol_stats.cursor_pos = ActivePanel->cursor_pos;
      ActivePanel->vol->vol_stats.disp_begin_pos = ActivePanel->disp_begin_pos;

      int res = CycleLoadedVolume(1);
      if (res == 0) { /* If volume switch was successful */
        if (CurrentVolume != start_vol)
          return ESC;

        s = &CurrentVolume->vol_stats; /* UPDATE S */
        ActivePanel->vol = CurrentVolume;
        ActivePanel->cursor_pos = CurrentVolume->vol_stats.cursor_pos;
        ActivePanel->disp_begin_pos = CurrentVolume->vol_stats.disp_begin_pos;

        if (ActivePanel->vol->total_dirs > 0) {
          if (ActivePanel->disp_begin_pos + ActivePanel->cursor_pos >=
              ActivePanel->vol->total_dirs) {
            int last_idx = ActivePanel->vol->total_dirs - 1;
            if (last_idx >= layout.dir_win_height) {
              ActivePanel->disp_begin_pos =
                  last_idx - (layout.dir_win_height - 1);
              ActivePanel->cursor_pos = layout.dir_win_height - 1;
            } else {
              ActivePanel->disp_begin_pos = 0;
              ActivePanel->cursor_pos = last_idx;
            }
          }
          dir_entry = ActivePanel->vol
                          ->dir_entry_list[ActivePanel->disp_begin_pos +
                                           ActivePanel->cursor_pos]
                          .dir_entry;
        } else {
          dir_entry = s->tree;
        }

        DisplayMenu();
        DisplayTree(ActivePanel->vol, dir_window, ActivePanel->disp_begin_pos,
                    ActivePanel->disp_begin_pos + ActivePanel->cursor_pos,
                    TRUE);
        DisplayFileWindow(ActivePanel, dir_entry);
        RefreshWindow(file_window);
        DisplayDiskStatistic(s);
        DisplayDirStatistic(dir_entry, NULL, s);
        DisplayAvailBytes(s);
        {
          char path[PATH_LENGTH];
          GetPath(dir_entry, path);
          DisplayHeaderPath(path);
        }
        need_dsp_help = TRUE;
      }
    } break;

    case ACTION_QUIT_DIR:
      need_dsp_help = TRUE;
      QuitTo(dir_entry);
      break;

    case ACTION_QUIT:
      need_dsp_help = TRUE;
      Quit();
      action = ACTION_NONE;
      break;

    case ACTION_LOGIN:
      if (mode != DISK_MODE && mode != USER_MODE) {
        if (getcwd(new_login_path, sizeof(new_login_path)) == NULL) {
          strcpy(new_login_path, ".");
        }
      } else {
        (void)GetPath(dir_entry, new_login_path);
      }
      if (!GetNewLoginPath(new_login_path)) {
        int ret; /* DEBUG variable */
        DisplayMenu();
        doupdate();

        ret = LogDisk(new_login_path);

        /* Check return value. Only update state if login succeeded (0). */
        if (ret == 0) {
          /* Safety Check: If volume was changed, return to main loop to handle
           * new context */
          if (CurrentVolume != start_vol)
            return ESC;

          s = &CurrentVolume
                   ->vol_stats; /* Update stats pointer for new volume */
          ActivePanel->vol = CurrentVolume;

          /* Safety check / Clamping */
          if (ActivePanel->vol->total_dirs > 0) {
            if (ActivePanel->disp_begin_pos + ActivePanel->cursor_pos >=
                ActivePanel->vol->total_dirs) {
              int last_idx = ActivePanel->vol->total_dirs - 1;
              if (last_idx >= layout.dir_win_height) {
                ActivePanel->disp_begin_pos =
                    last_idx - (layout.dir_win_height - 1);
                ActivePanel->cursor_pos = layout.dir_win_height - 1;
              } else {
                ActivePanel->disp_begin_pos = 0;
                ActivePanel->cursor_pos = last_idx;
              }
            }
            /* Now safe to assign dir_entry */
            dir_entry = ActivePanel->vol
                            ->dir_entry_list[ActivePanel->disp_begin_pos +
                                             ActivePanel->cursor_pos]
                            .dir_entry;
          } else {
            dir_entry = s->tree;
          }

          DisplayMenu(); /* Redraw menu/frame */

          /* Force Full Display Refresh */
          DisplayTree(ActivePanel->vol, dir_window, ActivePanel->disp_begin_pos,
                      ActivePanel->disp_begin_pos + ActivePanel->cursor_pos,
                      TRUE);
          DisplayFileWindow(ActivePanel, dir_entry);
          RefreshWindow(file_window);
          DisplayDiskStatistic(s);
          DisplayDirStatistic(dir_entry, NULL, s);
          DisplayAvailBytes(s);
          /* Update header path */
          {
            char path[PATH_LENGTH];
            GetPath(dir_entry, path);
            DisplayHeaderPath(path);
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
    if (ActivePanel && ActivePanel->vol && ActivePanel->vol->dir_entry_list) {
      int safe_idx = ActivePanel->disp_begin_pos + ActivePanel->cursor_pos;
      if (safe_idx < 0)
        safe_idx = 0;
      if (safe_idx >= ActivePanel->vol->total_dirs)
        safe_idx = ActivePanel->vol->total_dirs - 1;
      if (safe_idx >= 0) {
        dir_entry = ActivePanel->vol->dir_entry_list[safe_idx].dir_entry;
      } else {
        dir_entry = ActivePanel->vol->vol_stats.tree;
      }
    }

  } while (action != ACTION_QUIT && action != ACTION_ENTER &&
           action != ACTION_ESCAPE &&
           action !=
               ACTION_LOGIN); /* Loop until explicit quit, escape or login */

  /* Sync state back to Volume on exit */
  CurrentVolume->vol_stats.cursor_pos = ActivePanel->cursor_pos;
  CurrentVolume->vol_stats.disp_begin_pos = ActivePanel->disp_begin_pos;

  return (ch); /* Return the last raw character that caused exit */
}

int ScanSubTree(DirEntry *dir_entry, Statistic *s) {
  DirEntry *de_ptr;
  char new_login_path[PATH_LENGTH + 1];

  if (dir_entry->not_scanned) {
    for (de_ptr = dir_entry->sub_tree; de_ptr; de_ptr = de_ptr->next) {
      GetPath(de_ptr, new_login_path);
      if (ReadTree(de_ptr, new_login_path, 999, s, Dir_Progress, NULL) == -1) {
        /* Abort signal received from ReadTree */
        return -1;
      }
      ApplyFilter(de_ptr, s);
    }
    dir_entry->not_scanned = FALSE;
  } else {
    for (de_ptr = dir_entry->sub_tree; de_ptr; de_ptr = de_ptr->next) {
      if (ScanSubTree(de_ptr, s) == -1) {
        /* Abort signal received from recursive ScanSubTree */
        return -1;
      }
    }
  }
  return (0);
}

int KeyF2Get(DirEntry *start_dir_entry, int disp_begin_pos, int cursor_pos,
             char *path) {
  struct Volume *original_vol; /* Declare first */
  int ch;
  int result = -1;
  int win_width, win_height;
  struct Volume *target_vol;
  int local_disp_begin_pos;
  int local_cursor_pos;
  YtreeAction action; /* Declare YtreeAction variable */
  char new_login_path[PATH_LENGTH + 1];

  /* 1. Sync ActivePanel state to CurrentVolume to ensure original_vol has valid
   * data */
  if (ActivePanel && ActivePanel->vol == CurrentVolume) {
    CurrentVolume->vol_stats.disp_begin_pos = ActivePanel->disp_begin_pos;
    CurrentVolume->vol_stats.cursor_pos = ActivePanel->cursor_pos;

    /* Update local variables to match */
    disp_begin_pos = ActivePanel->disp_begin_pos;
    cursor_pos = ActivePanel->cursor_pos;
  }
  original_vol = CurrentVolume;

  if (mode != DISK_MODE && mode != USER_MODE) {
    /* Search for a volume that is in DISK_MODE */
    struct Volume *v, *tmp;
    struct Volume *disk_vol = NULL;

    HASH_ITER(hh, VolumeList, v, tmp) {
      /* Renamed usage: v->vol_stats.mode -> v->vol_stats.login_mode */
      if (v->vol_stats.login_mode == DISK_MODE) {
        disk_vol = v;
        break;
      }
    }

    if (disk_vol) {
      target_vol = disk_vol;
      local_disp_begin_pos = disk_vol->vol_stats.disp_begin_pos;
      local_cursor_pos = disk_vol->vol_stats.cursor_pos;
    } else {
      /* Fallback to CurrentVolume */
      target_vol = CurrentVolume;
      local_disp_begin_pos = disp_begin_pos;
      local_cursor_pos = cursor_pos;
    }
  } else {
    target_vol = CurrentVolume;
    local_disp_begin_pos = disp_begin_pos;
    local_cursor_pos = cursor_pos;
  }

  /* Only rebuild if list is missing. Rebuilding invalidates pointers held by
   * callers! */
  if (target_vol->dir_entry_list == NULL) {
    BuildDirEntryList(target_vol);
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

  GetMaxYX(f2_window, &win_height, &win_width);
  MapF2Window();
  DisplayTree(target_vol, f2_window, local_disp_begin_pos,
              local_disp_begin_pos + local_cursor_pos, TRUE);
  do {
    /* Footer Drawing */
    wattron(f2_window, A_BOLD);
    mvwaddstr(f2_window, win_height - 1, 2, "[ (L)og (< >) Cycle ]");
    wattroff(f2_window, A_BOLD);

    RefreshWindow(f2_window);
    doupdate();
    ch = Getch();
    GetMaxYX(f2_window, &win_height, &win_width); /* Maybe changed... */
    /* LF to CR normalization is now handled by GetKeyAction */

    action = GetKeyAction(ch); /* Translate raw input to YtreeAction */

    switch (action) {
    case ACTION_NONE:
      break; /* -1 or unhandled keys, no beep in F2Get */
    case ACTION_MOVE_DOWN:
      if (local_disp_begin_pos + local_cursor_pos + 1 >=
          target_vol->total_dirs) {
      } else {
        if (local_cursor_pos + 1 < win_height) {
          PrintDirEntry(target_vol, f2_window,
                        local_disp_begin_pos + local_cursor_pos,
                        local_cursor_pos, FALSE, TRUE);
          local_cursor_pos++;

          PrintDirEntry(target_vol, f2_window,
                        local_disp_begin_pos + local_cursor_pos,
                        local_cursor_pos, TRUE, TRUE);
        } else {
          local_disp_begin_pos++;
          DisplayTree(target_vol, f2_window, local_disp_begin_pos,
                      local_disp_begin_pos + local_cursor_pos, TRUE);
        }
      }
      break;

    case ACTION_MOVE_UP:
      if (local_disp_begin_pos + local_cursor_pos - 1 < 0) {
      } else {
        if (local_cursor_pos - 1 >= 0) {
          PrintDirEntry(target_vol, f2_window,
                        local_disp_begin_pos + local_cursor_pos,
                        local_cursor_pos, FALSE, TRUE);
          local_cursor_pos--;
          PrintDirEntry(target_vol, f2_window,
                        local_disp_begin_pos + local_cursor_pos,
                        local_cursor_pos, TRUE, TRUE);
        }

        else {
          local_disp_begin_pos--;
          DisplayTree(target_vol, f2_window, local_disp_begin_pos,
                      local_disp_begin_pos + local_cursor_pos, TRUE);
        }
      }
      break;

    case ACTION_PAGE_DOWN:
      if (local_disp_begin_pos + local_cursor_pos >=
          target_vol->total_dirs - 1) {
      } else {
        if (local_cursor_pos < win_height - 1) {
          PrintDirEntry(target_vol, f2_window,
                        local_disp_begin_pos + local_cursor_pos,
                        local_cursor_pos, FALSE, TRUE);
          if (local_disp_begin_pos + win_height > target_vol->total_dirs - 1)
            local_cursor_pos =
                target_vol->total_dirs - local_disp_begin_pos - 1;
          else
            local_cursor_pos = win_height - 1;
          PrintDirEntry(target_vol, f2_window,
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
          DisplayTree(target_vol, f2_window, local_disp_begin_pos,
                      local_disp_begin_pos + local_cursor_pos, TRUE);
        }
      }
      break;

    case ACTION_PAGE_UP:
      if (local_disp_begin_pos + local_cursor_pos <= 0) {
      } else {
        if (local_cursor_pos > 0) {
          PrintDirEntry(target_vol, f2_window,
                        local_disp_begin_pos + local_cursor_pos,
                        local_cursor_pos, FALSE, TRUE);
          local_cursor_pos = 0;
          PrintDirEntry(target_vol, f2_window,
                        local_disp_begin_pos + local_cursor_pos,
                        local_cursor_pos, TRUE, TRUE);
        } else {
          if ((local_disp_begin_pos -= win_height) < 0) {
            local_disp_begin_pos = 0;
          }
          local_cursor_pos = 0;
          DisplayTree(target_vol, f2_window, local_disp_begin_pos,
                      local_disp_begin_pos + local_cursor_pos, TRUE);
        }
      }
      break;

    case ACTION_HOME:
      if (local_disp_begin_pos == 0 && local_cursor_pos == 0) {
      } else {
        local_disp_begin_pos = 0;
        local_cursor_pos = 0;
        DisplayTree(target_vol, f2_window, local_disp_begin_pos,
                    local_disp_begin_pos + local_cursor_pos, TRUE);
      }
      break;

    case ACTION_END:
      local_disp_begin_pos = MAXIMUM(0, target_vol->total_dirs - win_height);
      local_cursor_pos = target_vol->total_dirs - local_disp_begin_pos - 1;
      DisplayTree(target_vol, f2_window, local_disp_begin_pos,
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
      if (CycleLoadedVolume(-1) == 0) {
        target_vol = CurrentVolume;
        local_disp_begin_pos = target_vol->vol_stats.disp_begin_pos;
        local_cursor_pos = target_vol->vol_stats.cursor_pos;
        BuildDirEntryList(target_vol);
        if (target_vol->total_dirs > 0 &&
            (local_disp_begin_pos + local_cursor_pos >=
             target_vol->total_dirs)) {
          local_disp_begin_pos = 0;
          local_cursor_pos = 0;
        }
        /* Fix blank screen bug: redraw main UI before F2 window */
        DisplayMenu();
        if (ActivePanel) {
          DisplayTree(ActivePanel->vol, dir_window, ActivePanel->disp_begin_pos,
                      ActivePanel->disp_begin_pos + ActivePanel->cursor_pos,
                      TRUE);
          DisplayFileWindow(ActivePanel, GetSelectedDirEntry(ActivePanel->vol));
          RefreshWindow(file_window);
        }
        DisplayDiskStatistic(&CurrentVolume->vol_stats);
        if (CurrentVolume->vol_stats.tree) {
          DisplayDirStatistic(GetSelectedDirEntry(CurrentVolume), NULL,
                              &CurrentVolume->vol_stats);
        }
        DisplayAvailBytes(&CurrentVolume->vol_stats);

        MapF2Window();
        DisplayTree(target_vol, f2_window, local_disp_begin_pos,
                    local_disp_begin_pos + local_cursor_pos, TRUE);
      }
      break;

    case ACTION_VOL_NEXT:
      if (CycleLoadedVolume(1) == 0) {
        target_vol = CurrentVolume;
        local_disp_begin_pos = target_vol->vol_stats.disp_begin_pos;
        local_cursor_pos = target_vol->vol_stats.cursor_pos;
        BuildDirEntryList(target_vol);
        if (target_vol->total_dirs > 0 &&
            (local_disp_begin_pos + local_cursor_pos >=
             target_vol->total_dirs)) {
          local_disp_begin_pos = 0;
          local_cursor_pos = 0;
        }
        /* Fix blank screen bug: redraw main UI before F2 window */
        DisplayMenu();
        if (ActivePanel) {
          DisplayTree(ActivePanel->vol, dir_window, ActivePanel->disp_begin_pos,
                      ActivePanel->disp_begin_pos + ActivePanel->cursor_pos,
                      TRUE);
          DisplayFileWindow(ActivePanel, GetSelectedDirEntry(ActivePanel->vol));
          RefreshWindow(file_window);
        }
        DisplayDiskStatistic(&CurrentVolume->vol_stats);
        if (CurrentVolume->vol_stats.tree) {
          DisplayDirStatistic(GetSelectedDirEntry(CurrentVolume), NULL,
                              &CurrentVolume->vol_stats);
        }
        DisplayAvailBytes(&CurrentVolume->vol_stats);

        MapF2Window();
        DisplayTree(target_vol, f2_window, local_disp_begin_pos,
                    local_disp_begin_pos + local_cursor_pos, TRUE);
      }
      break;

    case ACTION_LOGIN:
      if (target_vol && target_vol->vol_stats.login_mode == DISK_MODE) {
        /* Try to use the path of the currently selected directory in F2 window
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

      if (!GetNewLoginPath(new_login_path)) {
        if (LogDisk(new_login_path) == 0) {
          ClearHelp(); /* ADDED */
          target_vol = CurrentVolume;
          local_disp_begin_pos = target_vol->vol_stats.disp_begin_pos;
          local_cursor_pos = target_vol->vol_stats.cursor_pos;

          BuildDirEntryList(target_vol);

          if (target_vol->total_dirs > 0 &&
              (local_disp_begin_pos + local_cursor_pos >=
               target_vol->total_dirs)) {
            local_disp_begin_pos = 0;
            local_cursor_pos = 0;
          }

          MapF2Window();

          DisplayTree(target_vol, f2_window, local_disp_begin_pos,
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
      beep();
      break;
    } /* switch */
  } while (action != ACTION_QUIT && action != ACTION_ENTER &&
           action != ACTION_ESCAPE && action != ACTION_LOGIN);

  /* Added restoration block */
  if (CurrentVolume != original_vol) {
    /* 1. Restore Global Volume Context */
    CurrentVolume = original_vol;
    mode = CurrentVolume->vol_stats.login_mode;

    /* 2. Restore ActivePanel state from the restored volume */
    if (ActivePanel) {
      ActivePanel->vol = CurrentVolume;
      ActivePanel->cursor_pos = CurrentVolume->vol_stats.cursor_pos;
      ActivePanel->disp_begin_pos = CurrentVolume->vol_stats.disp_begin_pos;
    }

    /* 3. Restore UI Layout */
    DisplayMenu(); /* Restores Frame and Header */

    /* Check which view mode we were in before F2 */
    if (file_window == big_file_window) {
      /* Restore Big Window Mode */
      SwitchToBigFileWindow();
      /* In Big Mode, we don't draw the tree. We draw global stats if global. */
      if (CurrentVolume->vol_stats.tree->global_flag) {
        DisplayDiskStatistic(&CurrentVolume->vol_stats);
      } else {
        /* If regular big window, shows Dir Stats */
        DisplayDirStatistic(GetSelectedDirEntry(CurrentVolume), NULL,
                            &CurrentVolume->vol_stats);
      }
    } else {
      /* Restore Standard Split Mode */
      SwitchToSmallFileWindow();
      DisplayTree(CurrentVolume, dir_window,
                  CurrentVolume->vol_stats.disp_begin_pos,
                  CurrentVolume->vol_stats.disp_begin_pos +
                      CurrentVolume->vol_stats.cursor_pos,
                  TRUE);
      DisplayDiskStatistic(&CurrentVolume->vol_stats);
      DisplayDirStatistic(GetSelectedDirEntry(CurrentVolume), NULL,
                          &CurrentVolume->vol_stats);
    }

    /* 4. Refresh File Content & Footer Background */
    DisplayFileWindow(ActivePanel, GetSelectedDirEntry(CurrentVolume));
    RefreshWindow(file_window);

    DisplayAvailBytes(&CurrentVolume->vol_stats);
  }

  /* Added: If volume didn't change (e.g. F2 cancelled), force panel refresh to
   * sync any state changes */
  if (CurrentVolume == original_vol && ActivePanel) {
    ActivePanel->vol = CurrentVolume;
    ActivePanel->cursor_pos = CurrentVolume->vol_stats.cursor_pos;
    ActivePanel->disp_begin_pos = CurrentVolume->vol_stats.disp_begin_pos;
  }

  UnmapF2Window();

  /* Restore the original directory list for the main window if we switched
   * volume context */
  /* Actually, BuildDirEntryList now writes to vol structure, so other volume
  caches are preserved. We only need to ensure the CurrentVolume is consistent
  for the main window logic, which is untouched here. */
  if (target_vol != CurrentVolume) {
    /* If we messed with another volume's list, that's fine, it's encapsulated.
     */
  } else {
    /* We modified current volume's list. Ensure it's valid. */
    BuildDirEntryList(CurrentVolume);
  }

  if (action == ACTION_ESCAPE || action == ACTION_QUIT)
    return -1;

  return (result);
}

int RefreshDirWindow(YtreePanel *p) {
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
  BuildDirEntryList(p->vol);

  /* Search old entry */
  for (n = -1, i = 0; i < p->vol->total_dirs; i++) {
    if (p->vol->dir_entry_list[i].dir_entry == de_ptr) {
      n = i;
      break;
    }
  }

  if (n == -1) {
    /* Directory disapeared */
    ERROR_MSG("Current directory disappeared");
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
    DisplayTree(p->vol, win, p->disp_begin_pos,
                p->disp_begin_pos + p->cursor_pos, TRUE);

    DisplayAvailBytes(s);
    DisplayDiskStatistic(s);
    de_ptr =
        p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
    DisplayDirStatistic(de_ptr, NULL, s);
    DisplayFileWindow(p, de_ptr);
    /* Update header path after refresh */
    {
      char path[PATH_LENGTH];
      GetPath(
          p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry,
          path);
      DisplayHeaderPath(path);
    }
    result = 0;
  }

  return (result);
}