/***************************************************************************
 *
 * src/ui/dir_nav.c
 * Directory Window Navigation (Move up/down/page/home/end)
 *
 ***************************************************************************/

#include "ytree_fs.h"
#include "ytree_ui.h"

static int FindVisibleBackwardNoWrap(const YtreePanel *p, int start_idx);

static void PositionPanelAtIndex(YtreePanel *p, int target_idx, int height) {
  if (!p || !p->vol || p->vol->total_dirs <= 0)
    return;

  if (height < 1)
    height = 1;

  if (target_idx < 0)
    target_idx = 0;
  if (target_idx >= p->vol->total_dirs)
    target_idx = p->vol->total_dirs - 1;

  if (p->hide_dot_files) {
    int start_idx = target_idx;
    int i;

    for (i = 1; i < height; i++) {
      int prev_idx = FindVisibleBackwardNoWrap(p, start_idx - 1);
      if (prev_idx < 0)
        break;
      start_idx = prev_idx;
    }
    p->disp_begin_pos = start_idx;
    p->cursor_pos = target_idx - p->disp_begin_pos;
    return;
  }

  if (target_idx < p->disp_begin_pos) {
    p->disp_begin_pos = target_idx;
    p->cursor_pos = 0;
    return;
  }

  if (target_idx >= p->disp_begin_pos + height) {
    p->disp_begin_pos = target_idx - height + 1;
    if (p->disp_begin_pos < 0)
      p->disp_begin_pos = 0;
    p->cursor_pos = target_idx - p->disp_begin_pos;
    return;
  }

  p->cursor_pos = target_idx - p->disp_begin_pos;
}

static BOOL SyncPanelToVisibleSelection(const ViewContext *ctx, YtreePanel *p,
                                        int direction_hint) {
  int idx;
  int visible_idx;

  if (!ctx || !p || !p->vol || !p->vol->dir_entry_list)
    return FALSE;

  idx = p->disp_begin_pos + p->cursor_pos;
  if (idx < 0)
    idx = 0;
  if (idx >= p->vol->total_dirs)
    idx = p->vol->total_dirs - 1;


  visible_idx = PanelFindNextVisibleDirIndex(p, idx, direction_hint);
  if (visible_idx < 0)
    visible_idx = PanelFindNextVisibleDirIndex(p, idx, -direction_hint);
  if (visible_idx < 0)
    visible_idx = PanelFindFirstVisibleDirIndex(p);
  if (visible_idx < 0)
    return FALSE;

  PositionPanelAtIndex(p, visible_idx, ctx->layout.dir_win_height);
  return TRUE;
}

static int FindVisibleBackwardNoWrap(const YtreePanel *p, int start_idx) {
  int idx;

  if (!p || !p->vol || !p->vol->dir_entry_list || start_idx < 0)
    return -1;

  for (idx = start_idx; idx >= 0; idx--) {
    const DirEntry *candidate = p->vol->dir_entry_list[idx].dir_entry;
    if (PanelDirIsVisible(p, candidate))
      return idx;
  }
  return -1;
}

static int GetCurrentVisiblePanelIndex(const YtreePanel *p) {
  int total_dirs;
  int idx;
  const DirEntry *current;

  if (!p || !p->vol || !p->vol->dir_entry_list)
    return -1;

  total_dirs = p->vol->total_dirs;
  if (total_dirs <= 0)
    return -1;

  idx = p->disp_begin_pos + p->cursor_pos;
  if (idx < 0)
    idx = 0;
  if (idx >= total_dirs)
    idx = total_dirs - 1;

  current = p->vol->dir_entry_list[idx].dir_entry;
  if (PanelDirIsVisible(p, current))
    return idx;

  idx = PanelFindNextVisibleDirIndex(p, idx, 1);
  if (idx < 0)
    idx = PanelFindNextVisibleDirIndex(p, p->disp_begin_pos + p->cursor_pos, -1);
  if (idx < 0)
    idx = PanelFindFirstVisibleDirIndex(p);
  return idx;
}

static BOOL MoveVisibleSelection(const ViewContext *ctx, YtreePanel *p,
                                 int direction, int steps) {
  int idx;
  int i;
  int total_dirs;

  if (!ctx || !p)
    return FALSE;
  if (steps < 1)
    steps = 1;
  if (direction == 0)
    direction = 1;
  direction = (direction > 0) ? 1 : -1;

  idx = GetCurrentVisiblePanelIndex(p);
  if (idx < 0)
    return FALSE;
  total_dirs = p->vol ? p->vol->total_dirs : 0;
  if (total_dirs <= 0)
    return FALSE;

  for (i = 0; i < steps; i++) {
    int candidate_start = idx + direction;
    if (candidate_start < 0 || candidate_start >= total_dirs)
      break;
    int next_idx = PanelFindNextVisibleDirIndex(p, candidate_start, direction);
    if (next_idx < 0)
      break;
    idx = next_idx;
  }

  PositionPanelAtIndex(p, idx, ctx->layout.dir_win_height);
  return TRUE;
}

void DirNav_Movedown(ViewContext *ctx, DirEntry **dir_entry, YtreePanel *p) {
  const Statistic *s = &p->vol->vol_stats;

  if (!MoveVisibleSelection(ctx, p, 1, 1))
    return;
  if (!SyncPanelToVisibleSelection(ctx, p, 1))
    return;

  *dir_entry = GetPanelDirEntry(p);
  if (*dir_entry == NULL)
    return;

  DEBUG_LOG("Movedown: moved to cursor_pos=%d (disp_begin_pos=%d), "
            "total_dirs=%d, name=%s",
            p->cursor_pos, p->disp_begin_pos, p->vol->total_dirs,
            (*dir_entry) ? (*dir_entry)->name : "NULL");

  if (0) {
    *dir_entry = RefreshTreeSafe(ctx, p, *dir_entry);
    /* Re-sync *dir_entry to global stats which RefreshTreeSafe might have
     * adjusted */
    *dir_entry =
        GetPanelDirEntry(p);
  }

  (*dir_entry)->start_file = 0;
  (*dir_entry)->cursor_pos = -1;
  DisplayTree(ctx, p->vol, p->pan_dir_window, p->disp_begin_pos,
              p->disp_begin_pos + p->cursor_pos, TRUE);
  DisplayFileWindow(ctx, p, *dir_entry);
  RefreshWindow(p->pan_file_window);
  UpdateStatsPanel(ctx, *dir_entry, s);
  /* Update header path */
  {
    char path[PATH_LENGTH];
    GetPath(*dir_entry, path);
    DisplayHeaderPath(ctx, path);
  }
}

void DirNav_Moveup(ViewContext *ctx, DirEntry **dir_entry, YtreePanel *p) {
  const Statistic *s = &p->vol->vol_stats;

  if (!MoveVisibleSelection(ctx, p, -1, 1))
    return;
  if (!SyncPanelToVisibleSelection(ctx, p, -1))
    return;

  *dir_entry = GetPanelDirEntry(p);
  if (*dir_entry == NULL)
    return;

  if (0) {
    *dir_entry = RefreshTreeSafe(ctx, p, *dir_entry);
    /* Re-sync *dir_entry to global stats which RefreshTreeSafe might have
     * adjusted */
    *dir_entry =
        GetPanelDirEntry(p);
  }

  (*dir_entry)->start_file = 0;
  (*dir_entry)->cursor_pos = -1;
  DisplayTree(ctx, p->vol, p->pan_dir_window, p->disp_begin_pos,
              p->disp_begin_pos + p->cursor_pos, TRUE);
  DisplayFileWindow(ctx, p, *dir_entry);
  RefreshWindow(p->pan_file_window);
  UpdateStatsPanel(ctx, *dir_entry, s);
  /* Update header path */
  {
    char path[PATH_LENGTH];
    GetPath(*dir_entry, path);
    DisplayHeaderPath(ctx, path);
  }
}

void DirNav_Movenpage(ViewContext *ctx, DirEntry **dir_entry, YtreePanel *p) {
  const Statistic *s = &p->vol->vol_stats;

  if (!MoveVisibleSelection(ctx, p, 1, ctx->layout.dir_win_height))
    return;
  if (!SyncPanelToVisibleSelection(ctx, p, 1))
    return;

  *dir_entry = GetPanelDirEntry(p);
  if (*dir_entry == NULL)
    return;

  if (0) {
    *dir_entry = RefreshTreeSafe(ctx, p, *dir_entry);
    /* Re-sync *dir_entry to global stats which RefreshTreeSafe might have
     * adjusted */
    *dir_entry =
        GetPanelDirEntry(p);
  }

  (*dir_entry)->start_file = 0;
  (*dir_entry)->cursor_pos = -1;
  DisplayTree(ctx, p->vol, p->pan_dir_window, p->disp_begin_pos,
              p->disp_begin_pos + p->cursor_pos, TRUE);
  DisplayFileWindow(ctx, p, *dir_entry);
  RefreshWindow(p->pan_file_window);
  UpdateStatsPanel(ctx, *dir_entry, s);
  /* Update header path */
  {
    char path[PATH_LENGTH];
    GetPath(*dir_entry, path);
    DisplayHeaderPath(ctx, path);
  }
}

void DirNav_Moveppage(ViewContext *ctx, DirEntry **dir_entry, YtreePanel *p) {
  const Statistic *s = &p->vol->vol_stats;

  if (!MoveVisibleSelection(ctx, p, -1, ctx->layout.dir_win_height))
    return;
  if (!SyncPanelToVisibleSelection(ctx, p, -1))
    return;

  *dir_entry = GetPanelDirEntry(p);
  if (*dir_entry == NULL)
    return;

  if (0) {
    *dir_entry = RefreshTreeSafe(ctx, p, *dir_entry);
    /* Re-sync *dir_entry to global stats which RefreshTreeSafe might have
     * adjusted */
    *dir_entry =
        GetPanelDirEntry(p);
  }

  (*dir_entry)->start_file = 0;
  (*dir_entry)->cursor_pos = -1;
  DisplayTree(ctx, p->vol, p->pan_dir_window, p->disp_begin_pos,
              p->disp_begin_pos + p->cursor_pos, TRUE);
  DisplayFileWindow(ctx, p, *dir_entry);
  RefreshWindow(p->pan_file_window);
  UpdateStatsPanel(ctx, *dir_entry, s);
  /* Update header path */
  {
    char path[PATH_LENGTH];
    GetPath(*dir_entry, path);
    DisplayHeaderPath(ctx, path);
  }
}

void DirNav_MoveEnd(ViewContext *ctx, DirEntry **dir_entry, YtreePanel *p) {
  const Statistic *s = &p->vol->vol_stats;
  int idx;

  if (p->hide_dot_files) {
    idx = PanelFindLastVisibleDirIndex(p);
    if (idx >= 0)
      PositionPanelAtIndex(p, idx, ctx->layout.dir_win_height);
  } else {
    Nav_End(&p->cursor_pos, &p->disp_begin_pos, p->vol->total_dirs,
            ctx->layout.dir_win_height);
    idx = PanelFindLastVisibleDirIndex(p);
    if (idx >= 0)
      PositionPanelAtIndex(p, idx, ctx->layout.dir_win_height);
  }
  if (!SyncPanelToVisibleSelection(ctx, p, -1))
    return;

  *dir_entry = GetPanelDirEntry(p);
  if (*dir_entry == NULL)
    return;

  if (0) {
    *dir_entry = RefreshTreeSafe(ctx, p, *dir_entry);
    /* Re-sync *dir_entry to global stats which RefreshTreeSafe might have
     * adjusted */
    *dir_entry =
        GetPanelDirEntry(p);
  }

  (*dir_entry)->start_file = 0;
  (*dir_entry)->cursor_pos = -1;
  DisplayFileWindow(ctx, p, *dir_entry);
  RefreshWindow(p->pan_file_window);
  RefreshWindow(p->pan_file_window);
  DisplayTree(ctx, p->vol, p->pan_dir_window, p->disp_begin_pos,
              p->disp_begin_pos + p->cursor_pos, TRUE);
  UpdateStatsPanel(ctx, *dir_entry, s);
  /* Update header path */
  {
    char path[PATH_LENGTH];
    GetPath(*dir_entry, path);
    DisplayHeaderPath(ctx, path);
  }
  return;
}

void DirNav_MoveHome(ViewContext *ctx, DirEntry **dir_entry, YtreePanel *p) {
  const Statistic *s = &p->vol->vol_stats;

  Nav_Home(&p->cursor_pos, &p->disp_begin_pos);
  {
    int idx = PanelFindFirstVisibleDirIndex(p);
    if (idx >= 0)
      PositionPanelAtIndex(p, idx, ctx->layout.dir_win_height);
  }
  if (!SyncPanelToVisibleSelection(ctx, p, 1))
    return;

  *dir_entry = GetPanelDirEntry(p);
  if (*dir_entry == NULL)
    return;

  if (0) {
    *dir_entry = RefreshTreeSafe(ctx, p, *dir_entry);
    /* Re-sync *dir_entry to global stats which RefreshTreeSafe might have
     * adjusted */
    *dir_entry =
        GetPanelDirEntry(p);
  }

  (*dir_entry)->start_file = 0;
  (*dir_entry)->cursor_pos = -1;
  DisplayFileWindow(ctx, p, *dir_entry);
  RefreshWindow(p->pan_file_window);
  RefreshWindow(p->pan_file_window);
  DisplayTree(ctx, p->vol, p->pan_dir_window, p->disp_begin_pos,
              p->disp_begin_pos + p->cursor_pos, TRUE);
  UpdateStatsPanel(ctx, *dir_entry, s);
  /* Update header path */
  {
    char path[PATH_LENGTH];
    GetPath(*dir_entry, path);
    DisplayHeaderPath(ctx, path);
  }
  return;
}
