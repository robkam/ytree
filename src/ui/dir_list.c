/***************************************************************************
 *
 * src/ui/dir_list.c
 * Directory Entry List Management (Build, Free, Query)
 *
 ***************************************************************************/

#include "ytnova_ui.h"
#include "ytnova_appstate_volume.h"

/* Internal recursive helper for BuildDirEntryList */
static void ReadDirList(ViewContext *ctx, DirEntry *dir_entry,
                        struct Volume *vol, int *index_ptr) {
  DirEntry *de_ptr;
  static int level = 0;
  static unsigned long indent = 0L;

  for (de_ptr = dir_entry; de_ptr; de_ptr = de_ptr->next) {
    /* Bounds Checking & Dynamic Reallocation */
    if (*index_ptr >= (int)vol->dir_entry_list_capacity) {
      size_t new_capacity = vol->dir_entry_list_capacity * 2;
      DirEntryList *new_list;
      if (new_capacity == 0)
        new_capacity = 128;

      if (!AppStateCommitVolumeDirEntryList(
              vol, vol->dir_entry_list, vol->dir_entry_list_capacity,
              vol->total_dirs)) {
        UI_Error(ctx, "", 0, "AppState volume cache boundary failed*ABORT");
        exit(1);
      }

      new_list = (DirEntryList *)xrealloc(vol->dir_entry_list,
                                          new_capacity * sizeof(DirEntryList));

      memset(new_list + vol->dir_entry_list_capacity, 0,
             (new_capacity - vol->dir_entry_list_capacity) *
                 sizeof(DirEntryList));

      if (!AppStateCommitVolumeDirEntryList(vol, new_list, new_capacity,
                                            vol->total_dirs)) {
        UI_Error(ctx, "", 0, "AppState volume cache update failed*ABORT");
        exit(1);
      }
    }

    indent &= ~(1L << level);
    if (de_ptr->next)
      indent |= (1L << level);

    vol->dir_entry_list[*index_ptr].dir_entry = de_ptr;
    vol->dir_entry_list[*index_ptr].level = (unsigned short)level;
    vol->dir_entry_list[*index_ptr].indent = indent;

    (*index_ptr)++;

    if (!de_ptr->not_scanned && de_ptr->sub_tree) {
      level++;
      ReadDirList(ctx, de_ptr->sub_tree, vol, index_ptr);
      level--;
    }
  }
}

void BuildDirEntryList(ViewContext *ctx, struct Volume *vol, int *index_ptr) {
  DirEntryList *new_list;
  size_t alloc_count;

  if (vol->dir_entry_list != NULL &&
      !AppStateReleaseVolumeDirEntryList(vol))
    return;

  alloc_count = vol->vol_stats.disk_total_directories;
  if (alloc_count < 16)
    alloc_count = 16;

  new_list = (DirEntryList *)xcalloc(alloc_count, sizeof(DirEntryList));
  if (!AppStateCommitVolumeDirEntryList(vol, new_list, alloc_count, 0)) {
    free(new_list);
    return;
  }

  *index_ptr = 0;

  if (vol->vol_stats.tree) {
    ReadDirList(ctx, vol->vol_stats.tree, vol, index_ptr);
  }

  (void)AppStateCommitVolumeDirEntryList(
      vol, vol->dir_entry_list, vol->dir_entry_list_capacity, *index_ptr);

#ifdef DEBUG
  if (vol->vol_stats.disk_total_directories != vol->total_dirs) {
    /* mismatch detected, but safely handled by realloc in ReadDirList */
  }
#endif
}

BOOL PanelDirIsVisible(const YtreeNovaPanel *panel, const DirEntry *dir_entry) {
  const DirEntry *ancestor;

  if (!panel || !panel->vol || !dir_entry)
    return FALSE;

  if (!panel->hide_dot_files)
    return TRUE;

  if (dir_entry == panel->vol->vol_stats.tree)
    return TRUE;

  if (dir_entry->name[0] == '.')
    return FALSE;

  for (ancestor = dir_entry->up_tree;
       ancestor && ancestor != panel->vol->vol_stats.tree;
       ancestor = ancestor->up_tree) {
    if (ancestor->name[0] == '.')
      return FALSE;
  }

  return TRUE;
}

int PanelFindNextVisibleDirIndex(const YtreeNovaPanel *panel, int start_idx,
                                 int direction) {
  int idx;
  int total_dirs;

  if (!panel || !panel->vol || !panel->vol->dir_entry_list)
    return -1;

  total_dirs = panel->vol->total_dirs;
  if (total_dirs <= 0)
    return -1;

  if (direction == 0)
    direction = 1;
  direction = (direction > 0) ? 1 : -1;

  if (start_idx < 0)
    start_idx = (direction > 0) ? 0 : total_dirs - 1;
  if (start_idx >= total_dirs)
    start_idx = (direction > 0) ? total_dirs - 1 : 0;

  for (idx = start_idx; idx >= 0 && idx < total_dirs; idx += direction) {
    const DirEntry *candidate = panel->vol->dir_entry_list[idx].dir_entry;
    if (PanelDirIsVisible(panel, candidate))
      return idx;
  }

  return -1;
}

int PanelFindFirstVisibleDirIndex(const YtreeNovaPanel *panel) {
  return PanelFindNextVisibleDirIndex(panel, 0, 1);
}

int PanelFindLastVisibleDirIndex(const YtreeNovaPanel *panel) {
  if (!panel || !panel->vol)
    return -1;
  return PanelFindNextVisibleDirIndex(panel, panel->vol->total_dirs - 1, -1);
}

static BOOL PanelVisibleIndexWithinViewport(const YtreeNovaPanel *panel,
                                            int begin_idx, int target_idx,
                                            int height) {
  int idx;
  int visible_rows;

  if (!panel || !panel->vol || !panel->vol->dir_entry_list || height < 1)
    return FALSE;

  if (begin_idx < 0)
    begin_idx = 0;
  if (begin_idx >= panel->vol->total_dirs)
    begin_idx = panel->vol->total_dirs - 1;
  if (target_idx < begin_idx)
    return FALSE;

  visible_rows = 0;
  for (idx = begin_idx; idx < panel->vol->total_dirs; idx++) {
    const DirEntry *candidate = panel->vol->dir_entry_list[idx].dir_entry;

    if (!PanelDirIsVisible(panel, candidate))
      continue;

    if (idx == target_idx)
      return TRUE;

    visible_rows++;
    if (visible_rows >= height)
      break;
  }

  return FALSE;
}

static int PanelFindViewportStartForVisibleIndex(const YtreeNovaPanel *panel,
                                                 int target_idx, int height) {
  int start_idx;
  int i;

  if (!panel || !panel->vol || !panel->vol->dir_entry_list || height < 1)
    return -1;

  if (target_idx < 0)
    target_idx = 0;
  if (target_idx >= panel->vol->total_dirs)
    target_idx = panel->vol->total_dirs - 1;

  start_idx = target_idx;
  for (i = 1; i < height; i++) {
    int prev_idx = PanelFindNextVisibleDirIndex(panel, start_idx - 1, -1);
    if (prev_idx < 0)
      break;
    start_idx = prev_idx;
  }

  return start_idx;
}

/*
 * Preserve the current viewport when the target row is already visible;
 * otherwise advance just enough visible rows to bring the target on screen.
 */
BOOL PanelComputeViewportPosition(const YtreeNovaPanel *panel, int target_idx,
                                  int height, int *begin_io,
                                  int *cursor_io) {
  int begin;
  int visible_idx;

  if (!begin_io || !cursor_io)
    return FALSE;

  begin = *begin_io;

  if (!panel || !panel->vol || !panel->vol->dir_entry_list ||
      panel->vol->total_dirs <= 0) {
    *begin_io = 0;
    *cursor_io = 0;
    return FALSE;
  }

  if (height < 1)
    height = 1;

  if (target_idx < 0)
    target_idx = 0;
  if (target_idx >= panel->vol->total_dirs)
    target_idx = panel->vol->total_dirs - 1;

  visible_idx = PanelFindNextVisibleDirIndex(panel, target_idx, 1);
  if (visible_idx < 0)
    visible_idx = PanelFindNextVisibleDirIndex(panel, target_idx, -1);
  if (visible_idx < 0)
    visible_idx = PanelFindFirstVisibleDirIndex(panel);
  if (visible_idx < 0) {
    *begin_io = 0;
    *cursor_io = 0;
    return FALSE;
  }
  target_idx = visible_idx;

  if (PanelVisibleIndexWithinViewport(panel, begin, target_idx, height)) {
    *begin_io = begin;
    *cursor_io = target_idx - begin;
    return TRUE;
  }

  if (target_idx < begin) {
    *begin_io = target_idx;
    *cursor_io = 0;
    return TRUE;
  }

  begin = PanelFindViewportStartForVisibleIndex(panel, target_idx, height);
  if (begin < 0) {
    *begin_io = 0;
    *cursor_io = 0;
    return FALSE;
  }

  *begin_io = begin;
  *cursor_io = target_idx - begin;
  return TRUE;
}

int GetPanelVisibleSelectionIndex(const YtreeNovaPanel *p) {
  int idx;

  if (!p || !p->vol || !p->vol->dir_entry_list || p->vol->total_dirs <= 0)
    return -1;

  idx = p->disp_begin_pos + p->cursor_pos;
  if (idx < 0)
    idx = 0;
  if (idx >= p->vol->total_dirs)
    idx = p->vol->total_dirs - 1;

  idx = PanelFindNextVisibleDirIndex(p, idx, 1);
  if (idx < 0)
    idx = PanelFindNextVisibleDirIndex(p, p->disp_begin_pos + p->cursor_pos,
                                       -1);
  if (idx < 0)
    idx = PanelFindFirstVisibleDirIndex(p);
  return idx;
}

/*
 * Frees the memory allocated for the dir_entry_list array of a volume.
 */
void FreeVolumeCache(struct Volume *vol) {
  if (vol)
    (void)AppStateReleaseVolumeDirEntryList(vol);
}

/*
 * Frees the memory allocated for the current volume's dir_entry_list.
 * Retained for compatibility.
 */
void FreeDirEntryList(ViewContext *ctx) {
  if (ctx->active->vol) {
    FreeVolumeCache(ctx->active->vol);
  }
}

/*
 * Helper function to return the currently selected directory entry from a
 * specific panel. Uses the panel's ViewContext (cursor_pos, disp_begin_pos)
 * instead of shared Volume stats.
 */
DirEntry *GetPanelDirEntry(YtreeNovaPanel *p) {
  if (p->vol->dir_entry_list != NULL && p->vol->total_dirs > 0) {
    int idx = GetPanelVisibleSelectionIndex(p);
    if (idx >= 0)
      return p->vol->dir_entry_list[idx].dir_entry;
  }
  /* Fallback to root if list is empty/invalid */
  return p->vol->vol_stats.tree;
}

/*
 * Helper function to return the currently selected directory entry.
 * Now takes a Volume context.
 */
DirEntry *GetSelectedDirEntry(const ViewContext *ctx, struct Volume *vol) {
  if (vol->dir_entry_list != NULL && vol->total_dirs > 0) {
    int idx;

    if (ctx && ctx->active)
      idx = GetPanelVisibleSelectionIndex(ctx->active);
    else
      idx = -1;
    if (idx >= 0)
      return vol->dir_entry_list[idx].dir_entry;
  }
  /* Fallback to root if list is empty/invalid */
  return vol->vol_stats.tree;
}
