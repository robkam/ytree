/***************************************************************************
 *
 * src/ui/dir_nav.c
 * Directory Window Navigation (Move up/down/page/home/end)
 *
 ***************************************************************************/

#include "ytree_fs.h"
#include "ytree_ui.h"

static void PositionPanelAtIndex(YtreePanel *p, int target_idx, int height) {
  if (!p || !p->vol || p->vol->total_dirs <= 0)
    return;

  if (height < 1)
    height = 1;

  if (target_idx < 0)
    target_idx = 0;
  if (target_idx >= p->vol->total_dirs)
    target_idx = p->vol->total_dirs - 1;

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
  int total_dirs;
  int idx;
  int visible_idx;

  if (!ctx || !p || !p->vol || !p->vol->dir_entry_list)
    return FALSE;

  total_dirs = p->vol->total_dirs;
  if (total_dirs <= 0)
    return FALSE;

  idx = p->disp_begin_pos + p->cursor_pos;
  if (idx < 0)
    idx = 0;
  if (idx >= total_dirs)
    idx = total_dirs - 1;

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

void DirNav_Movedown(ViewContext *ctx, DirEntry **dir_entry, YtreePanel *p) {
  const Statistic *s = &p->vol->vol_stats;

  Nav_MoveDown(&p->cursor_pos, &p->disp_begin_pos, p->vol->total_dirs,
               ctx->layout.dir_win_height, 1);
  if (!SyncPanelToVisibleSelection(ctx, p, 1))
    return;

  *dir_entry =
      p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
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
        p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
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

  Nav_MoveUp(&p->cursor_pos, &p->disp_begin_pos);
  if (!SyncPanelToVisibleSelection(ctx, p, -1))
    return;

  *dir_entry =
      p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
  if (*dir_entry == NULL)
    return;

  if (0) {
    *dir_entry = RefreshTreeSafe(ctx, p, *dir_entry);
    /* Re-sync *dir_entry to global stats which RefreshTreeSafe might have
     * adjusted */
    *dir_entry =
        p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
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

  Nav_PageDown(&p->cursor_pos, &p->disp_begin_pos, p->vol->total_dirs,
               ctx->layout.dir_win_height);
  if (!SyncPanelToVisibleSelection(ctx, p, 1))
    return;

  *dir_entry =
      p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
  if (*dir_entry == NULL)
    return;

  if (0) {
    *dir_entry = RefreshTreeSafe(ctx, p, *dir_entry);
    /* Re-sync *dir_entry to global stats which RefreshTreeSafe might have
     * adjusted */
    *dir_entry =
        p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
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

  Nav_PageUp(&p->cursor_pos, &p->disp_begin_pos, ctx->layout.dir_win_height);
  if (!SyncPanelToVisibleSelection(ctx, p, -1))
    return;

  *dir_entry =
      p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
  if (*dir_entry == NULL)
    return;

  if (0) {
    *dir_entry = RefreshTreeSafe(ctx, p, *dir_entry);
    /* Re-sync *dir_entry to global stats which RefreshTreeSafe might have
     * adjusted */
    *dir_entry =
        p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
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

  Nav_End(&p->cursor_pos, &p->disp_begin_pos, p->vol->total_dirs,
          ctx->layout.dir_win_height);
  {
    int idx = PanelFindLastVisibleDirIndex(p);
    if (idx >= 0)
      PositionPanelAtIndex(p, idx, ctx->layout.dir_win_height);
  }
  if (!SyncPanelToVisibleSelection(ctx, p, -1))
    return;

  *dir_entry =
      p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
  if (*dir_entry == NULL)
    return;

  if (0) {
    *dir_entry = RefreshTreeSafe(ctx, p, *dir_entry);
    /* Re-sync *dir_entry to global stats which RefreshTreeSafe might have
     * adjusted */
    *dir_entry =
        p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
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

  *dir_entry =
      p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
  if (*dir_entry == NULL)
    return;

  if (0) {
    *dir_entry = RefreshTreeSafe(ctx, p, *dir_entry);
    /* Re-sync *dir_entry to global stats which RefreshTreeSafe might have
     * adjusted */
    *dir_entry =
        p->vol->dir_entry_list[p->disp_begin_pos + p->cursor_pos].dir_entry;
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
