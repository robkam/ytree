/***************************************************************************
 *
 * src/ui/dir_nav.c
 * Directory Window Navigation (Move up/down/page/home/end)
 *
 ***************************************************************************/

#define NO_YTREE_MACROS
#include "ytree.h"
#include "ytree_ui.h"

void DirNav_Movedown(ViewContext *ctx, DirEntry **dir_entry, YtreePanel *p) {
  const Statistic *s = &p->vol->vol_stats;

  Nav_MoveDown(&p->cursor_pos, &p->disp_begin_pos, p->vol->total_dirs,
               ctx->layout.dir_win_height, 1);

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
