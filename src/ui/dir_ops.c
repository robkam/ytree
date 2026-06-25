/***************************************************************************
 *
 * src/ui/dir_ops.c
 * Directory Tree Operations (Expand, Collapse, Scan, Dotfile Toggle,
 * Mode Switching, Show-All, Refresh)
 *
 ***************************************************************************/

#include "watcher.h"
#include "ytnova_appstate_actions.h"
#include "ytnova_appstate_focus.h"
#include "ytnova_cmd.h"
#include "ytnova_fs.h"
#include "ytnova_panel_anchor.h"
#include "ytnova_split_transition.h"
#include "ytnova_ui.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

/* TREEDEPTH uses GetProfileValue which is 2-arg in NO_YTNOVA_MACROS context */
#undef TREEDEPTH
#define TREEDEPTH (GetProfileValue)(ctx, "TREEDEPTH")

/* Progress callback for directory operations */
static void Dir_Progress(ViewContext *ctx, void *data) {
  (void)data; /* Suppress unused parameter warning */
  DrawSpinner(ctx);
}

static void CaptureInactiveFallback(ViewContext *ctx, YtreeNovaPanel *p,
                                    const DirEntry *dir_entry,
                                    YtreeNovaPanel **inactive_out,
                                    DirEntry **inactive_fallback_out);
static void ReanchorPanelToDir(YtreeNovaPanel *panel, const DirEntry *target);
typedef struct {
  YtreeNovaPanel *panel;
  char selected_path[PATH_LENGTH + 1];
  char next_sibling_path[PATH_LENGTH + 1];
  char prev_sibling_path[PATH_LENGTH + 1];
} InactiveFallbackSnapshot;

static BOOL DirBelongsToVolume(const struct Volume *vol, const DirEntry *target) {
  int i;

  if (!vol || !target || !vol->dir_entry_list || vol->total_dirs <= 0)
    return FALSE;

  for (i = 0; i < vol->total_dirs; i++) {
    if (vol->dir_entry_list[i].dir_entry == target)
      return TRUE;
  }

  return FALSE;
}

static void DebugLogPanelState(const char *label, const YtreeNovaPanel *panel) {
  char tree_path[PATH_LENGTH + 1];
  char file_dir_path[PATH_LENGTH + 1];
  int idx = -1;
  const DirEntry *tree_de = NULL;
  const char *tree_text = "<none>";
  const char *file_dir_text = "<none>";
  const char *selection_dir_text = "<none>";
  const char *selection_name_text = "<none>";

  tree_path[0] = '\0';
  file_dir_path[0] = '\0';

  if (!panel) {
    DEBUG_LOG("PANEL[%s] <null>", label ? label : "?");
    return;
  }

  if (panel->vol && panel->vol->total_dirs > 0 && panel->vol->dir_entry_list) {
    idx = panel->disp_begin_pos + panel->cursor_pos;
    if (idx < 0)
      idx = 0;
    if (idx >= panel->vol->total_dirs)
      idx = panel->vol->total_dirs - 1;
    tree_de = panel->vol->dir_entry_list[idx].dir_entry;
    if (tree_de) {
      GetPath((DirEntry *)tree_de, tree_path);
      tree_path[PATH_LENGTH] = '\0';
      tree_text = tree_path;
    }
  }

  if (panel->file_dir_entry && DirBelongsToVolume(panel->vol, panel->file_dir_entry)) {
    GetPath(panel->file_dir_entry, file_dir_path);
    file_dir_path[PATH_LENGTH] = '\0';
    file_dir_text = file_dir_path;
  } else if (panel->file_dir_entry) {
    file_dir_text = "<stale>";
  }

  if (panel->file_selection_dir_path[0] != '\0')
    selection_dir_text = panel->file_selection_dir_path;
  if (panel->file_selection_name[0] != '\0')
    selection_name_text = panel->file_selection_name;

  DEBUG_LOG(
      "PANEL[%s] saved_focus=%d disp=%d cur=%d idx=%d start=%d fcur=%d "
      "tree='%s' file_dir='%s' sel_dir='%s' sel_name='%s'",
      label ? label : "?", panel->saved_focus, panel->disp_begin_pos,
      panel->cursor_pos, idx, panel->start_file, panel->file_cursor_pos,
      tree_text, file_dir_text, selection_dir_text, selection_name_text);
}

static void DebugLogSplitState(const char *label, const ViewContext *ctx) {
  const char *active_side = "?";

  if (!ctx) {
    DEBUG_LOG("SPLIT[%s] <null>", label ? label : "?");
    return;
  }

  if (ctx->active == ctx->left)
    active_side = "LEFT";
  else if (ctx->active == ctx->right)
    active_side = "RIGHT";

  DEBUG_LOG("SPLIT[%s] is_split=%d active=%s focused=%d", label ? label : "?",
            ctx->is_split_screen, active_side, ctx->focused_window);
  DebugLogPanelState("LEFT", ctx->left);
  DebugLogPanelState("RIGHT", ctx->right);
}

void HandlePlus(ViewContext *ctx, DirEntry *dir_entry, DirEntry *de_ptr,
                char *new_log_path, BOOL *need_dsp_help, YtreeNovaPanel *p) {
  Statistic *s = &p->vol->vol_stats;
  YtreeNovaPanel *inactive = NULL;
  DirEntry *inactive_target = NULL;
  (void)de_ptr;

  /* Renamed usage: s->mode -> s->log_mode */
  if (s->log_mode != DISK_MODE && s->log_mode != USER_MODE &&
      s->log_mode != ARCHIVE_MODE) {
    return;
  }
  if (dir_entry && dir_entry->up_tree == NULL && dir_entry->unlogged_flag &&
      s->log_mode == ARCHIVE_MODE) {
    if (new_log_path) {
      (void)snprintf(new_log_path, PATH_LENGTH + 1, "%s", s->log_path);
      new_log_path[PATH_LENGTH] = '\0';
      if (LogDisk(ctx, p, new_log_path) == 0) {
        DirEntry *refreshed_dir = GetPanelDirEntry(p);
        if (!refreshed_dir && p->vol)
          refreshed_dir = p->vol->vol_stats.tree;
        if (refreshed_dir)
          RefreshView(ctx, refreshed_dir);
        *need_dsp_help = TRUE;
      }
    }
    return;
  }
  if (!dir_entry || !dir_entry->not_scanned)
    return;

  CaptureInactiveFallback(ctx, p, NULL, &inactive, &inactive_target);

  if (!dir_entry->unlogged_flag &&
      (dir_entry->sub_tree != NULL || dir_entry->file != NULL)) {
    dir_entry->not_scanned = FALSE;
    p->vol->volume_generation++;
    BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);
    BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);
    if (inactive && inactive->vol == p->vol) {
      ReanchorPanelToDir(inactive, inactive_target);
      BuildFileEntryList(ctx, inactive);
    }
    DisplayTree(ctx, p->vol, p->pan_dir_window, p->disp_begin_pos,
                p->disp_begin_pos + p->cursor_pos, TRUE);
    DisplayFileWindow(ctx, p, dir_entry);
    DisplayDiskStatistic(ctx, s);
    UpdateStatsPanel(ctx, dir_entry, s);
    DisplayAvailBytes(ctx, s);
    *need_dsp_help = TRUE;
    return;
  }

  {
    int read_depth = 1;

    SuspendClock(ctx); /* Suspend clock before scanning */
    GetPath(dir_entry, new_log_path);
    if (dir_entry->unlogged_flag) {
      read_depth = strtol(TREEDEPTH, NULL, 0);
      if (read_depth < 0)
        read_depth = 0;
    }
    ReadTree(ctx, dir_entry, new_log_path, read_depth, s, Dir_Progress, NULL);
    ApplyFilter(dir_entry, s);
    InitClock(ctx); /* Resume clock after scanning */

    dir_entry->not_scanned = FALSE;
    dir_entry->unlogged_flag = FALSE;
    BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);
    BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);
    if (inactive && inactive->vol == p->vol) {
      ReanchorPanelToDir(inactive, inactive_target);
      BuildFileEntryList(ctx, inactive);
    }
    DisplayTree(ctx, p->vol, p->pan_dir_window, p->disp_begin_pos,
                p->disp_begin_pos + p->cursor_pos, TRUE);
    DisplayFileWindow(ctx, p, dir_entry);
    DisplayDiskStatistic(ctx, s);
    UpdateStatsPanel(ctx, dir_entry, s); /* Show dir stats AND attributes */
    DisplayAvailBytes(ctx, s);
    *need_dsp_help = TRUE;
  }
}

void HandleReadSubTree(ViewContext *ctx, DirEntry *dir_entry,
                       BOOL *need_dsp_help, YtreeNovaPanel *p) {
  const Statistic *s = &p->vol->vol_stats;
  YtreeNovaPanel *inactive = NULL;
  DirEntry *inactive_target = NULL;

  CaptureInactiveFallback(ctx, p, NULL, &inactive, &inactive_target);

  SuspendClock(ctx); /* Suspend clock before scanning */
  if (ScanSubTree(ctx, dir_entry, s) == -1) {
    /* Aborted. Fall through to refresh what we have. */
  }
  InitClock(ctx); /* Resume clock after scanning */
  dir_entry->unlogged_flag = FALSE;
  BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);
  BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);
  if (inactive && inactive->vol == p->vol) {
    ReanchorPanelToDir(inactive, inactive_target);
    BuildFileEntryList(ctx, inactive);
  }
  DisplayTree(ctx, p->vol, p->pan_dir_window, p->disp_begin_pos,
              p->disp_begin_pos + p->cursor_pos, TRUE);
  RecalculateSysStats(ctx, s); /* Fix for Bug 10: Force full recalculation */
  DisplayDiskStatistic(ctx, s);
  UpdateStatsPanel(ctx, dir_entry, s);
  DisplayAvailBytes(ctx, s);
  *need_dsp_help = TRUE;
}

static BOOL IsDescendant(const DirEntry *ancestor, const DirEntry *descendant) {
  while (descendant) {
    if (descendant == ancestor)
      return TRUE;
    descendant = descendant->up_tree;
  }
  return FALSE;
}

static int FindDirIndex(const struct Volume *vol, const DirEntry *target) {
  int i;

  if (!vol || !target || !vol->dir_entry_list || vol->total_dirs <= 0)
    return -1;

  for (i = 0; i < vol->total_dirs; i++) {
    if (vol->dir_entry_list[i].dir_entry == target)
      return i;
  }

  return -1;
}

static DirEntry *FindDirByPathInSubTree(DirEntry *entry, const char *path) {
  char candidate_path[PATH_LENGTH + 1];

  for (; entry; entry = entry->next) {
    GetPath(entry, candidate_path);
    candidate_path[PATH_LENGTH] = '\0';
    if (strcmp(candidate_path, path) == 0)
      return entry;
    if (entry->sub_tree) {
      DirEntry *resolved = FindDirByPathInSubTree(entry->sub_tree, path);
      if (resolved)
        return resolved;
    }
  }

  return NULL;
}

static DirEntry *FindDirByPath(const struct Volume *vol, const char *path) {
  if (!vol || !path || !*path)
    return NULL;

  if (vol->dir_entry_list && vol->total_dirs > 0) {
    char candidate_path[PATH_LENGTH + 1];
    int i;
    for (i = 0; i < vol->total_dirs; i++) {
      DirEntry *candidate = vol->dir_entry_list[i].dir_entry;
      if (!candidate)
        continue;
      GetPath(candidate, candidate_path);
      candidate_path[PATH_LENGTH] = '\0';
      if (strcmp(candidate_path, path) == 0)
        return candidate;
    }
  }

  if (!vol->vol_stats.tree)
    return NULL;

  return FindDirByPathInSubTree(vol->vol_stats.tree, path);
}

static DirEntry *FindVisibleDirByPath(const struct Volume *vol,
                                      const char *path) {
  DirEntry *candidate;

  if (!vol || !path || !*path)
    return NULL;

  candidate = FindDirByPath(vol, path);
  if (!candidate)
    return NULL;

  if (FindDirIndex(vol, candidate) < 0)
    return NULL;

  return candidate;
}

DirEntry *DirOps_FindDirEntryByPath(const ViewContext *ctx,
                                    const char *dir_path) {
  if (!ctx || !ctx->active || !ctx->active->vol || !dir_path ||
      dir_path[0] != FILE_SEPARATOR_CHAR) {
    return NULL;
  }

  return FindDirByPath(ctx->active->vol, dir_path);
}

DirEntry *DirOps_ResolveCopyMoveRefreshAnchor(ViewContext *ctx,
                                              const char *src_path,
                                              const char *dest_dir_path,
                                              DirEntry *fallback) {
  DirEntry *tree;
  DirEntry *anchor;
  char common_path[PATH_LENGTH + 1];
  size_t i;
  size_t last_separator = (size_t)-1;

  if (!ctx || !ctx->active || !ctx->active->vol)
    return fallback;

  tree = ctx->active->vol->vol_stats.tree;
  if (!tree)
    return fallback;

  if (!src_path || !dest_dir_path)
    return tree;
  if (src_path[0] != FILE_SEPARATOR_CHAR ||
      dest_dir_path[0] != FILE_SEPARATOR_CHAR) {
    return tree;
  }

  for (i = 0; src_path[i] != '\0' && dest_dir_path[i] != '\0'; ++i) {
    if (src_path[i] != dest_dir_path[i])
      break;
    if (src_path[i] == FILE_SEPARATOR_CHAR)
      last_separator = i;
  }

  if (last_separator == (size_t)-1)
    return tree;

  if (last_separator == 0) {
    common_path[0] = FILE_SEPARATOR_CHAR;
    common_path[1] = '\0';
  } else {
    if (last_separator >= PATH_LENGTH)
      return tree;
    memcpy(common_path, src_path, last_separator);
    common_path[last_separator] = '\0';
  }

  anchor = DirOps_FindDirEntryByPath(ctx, common_path);
  if (anchor)
    return anchor;
  return tree;
}

static BOOL EnsureDirVisible(ViewContext *ctx, YtreeNovaPanel *panel, DirEntry *target) {
  BOOL changed = FALSE;
  DirEntry *ancestor;

  if (!ctx || !panel || !panel->vol || !target)
    return FALSE;

  for (ancestor = target->up_tree; ancestor; ancestor = ancestor->up_tree) {
    if (ancestor->not_scanned && ancestor->sub_tree) {
      ancestor->not_scanned = FALSE;
      changed = TRUE;
    }
  }

  if (changed)
    BuildDirEntryList(ctx, panel->vol, &panel->current_dir_entry);

  return FindDirIndex(panel->vol, target) >= 0;
}

static DirEntry *FindDirByPathOrAncestor(const struct Volume *vol,
                                         const char *path) {
  char probe[PATH_LENGTH + 1];
  size_t prev_len;

  if (!vol || !path || !*path)
    return NULL;

  (void)snprintf(probe, sizeof(probe), "%s", path);
  probe[PATH_LENGTH] = '\0';
  prev_len = strlen(probe) + 1;

  while (probe[0] != '\0') {
    DirEntry *resolved = FindDirByPath(vol, probe);
    char *slash;

    if (resolved)
      return resolved;

    if (probe[0] == FILE_SEPARATOR_CHAR && probe[1] == '\0')
      break;

    if (strlen(probe) >= prev_len)
      break;
    prev_len = strlen(probe);

    while (prev_len > 1 && probe[prev_len - 1] == FILE_SEPARATOR_CHAR) {
      probe[prev_len - 1] = '\0';
      prev_len--;
    }

    slash = strrchr(probe, FILE_SEPARATOR_CHAR);
    if (!slash)
      break;
    if (slash == probe) {
      probe[1] = '\0';
    } else {
      *slash = '\0';
    }
  }

  return NULL;
}

static void AddPathSnapshot(PathList **list, const char *path) {
  PathList *node;

  if (!list || !path || !*path)
    return;

  node = (PathList *)xcalloc(1, sizeof(PathList));
  node->path = xstrdup(path);
  node->next = *list;
  *list = node;
}

static void CapturePanelTaggedSnapshot(const YtreeNovaPanel *panel,
                                       PathList **tagged) {
  char path[PATH_LENGTH + 1];
  unsigned int i;
  const FileEntry *fe;

  if (!panel || !tagged)
    return;

  for (i = 0; i < panel->file_count; i++) {
    fe = panel->file_entry_list ? panel->file_entry_list[i].file : NULL;
    if (fe && fe->tagged) {
      GetFileNamePath((FileEntry *)fe, path);
      path[PATH_LENGTH] = '\0';
      AddPathSnapshot(tagged, path);
    }
  }

  if (*tagged || !panel->file_dir_entry)
    return;

  for (fe = panel->file_dir_entry->file; fe; fe = fe->next) {
    if (fe->tagged) {
      GetFileNamePath((FileEntry *)fe, path);
      path[PATH_LENGTH] = '\0';
      AddPathSnapshot(tagged, path);
    }
  }
}

static void CaptureCollapsedTreeState(DirEntry *dir, PathList **collapsed) {
  DirEntry *sub;

  if (!dir || !collapsed)
    return;

  if (dir->sub_tree && dir->not_scanned) {
    char path[PATH_LENGTH + 1];
    GetPath(dir, path);
    path[PATH_LENGTH] = '\0';
    AddPathSnapshot(collapsed, path);
    return;
  }

  for (sub = dir->sub_tree; sub; sub = sub->next) {
    CaptureCollapsedTreeState(sub, collapsed);
  }
}

static void RestoreTaggedSnapshot(ViewContext *ctx, struct Volume *vol,
                                  PathList *tagged) {
  PathList *expanded = NULL;

  if (!ctx || !vol || !vol->vol_stats.tree || !tagged)
    return;

  RestoreTreeState(ctx, vol->vol_stats.tree, &expanded, tagged,
                   &vol->vol_stats);
  FreePathList(expanded);
}

static int CountPathSnapshot(const PathList *list) {
  int count = 0;

  for (; list; list = list->next)
    count++;

  return count;
}

static void ReanchorPanelToDir(YtreeNovaPanel *panel, const DirEntry *target) {
  PanelViewportSnapshot snapshot;

  if (!panel)
    return;
  if (!panel->vol || panel->vol->total_dirs <= 0) {
    panel->disp_begin_pos = 0;
    panel->cursor_pos = 0;
    RememberPanelViewportTop(panel);
    return;
  }

  if (!target)
    target = panel->vol->vol_stats.tree;

  CapturePanelViewportSnapshot(panel, panel->vol, &snapshot);
  if (target) {
    char target_path[PATH_LENGTH + 1];

    GetPath((DirEntry *)target, target_path);
    target_path[PATH_LENGTH] = '\0';
    (void)snprintf(snapshot.selected_dir_path,
                   sizeof(snapshot.selected_dir_path), "%s", target_path);
    snapshot.selected_dir_path[PATH_LENGTH] = '\0';
    snapshot.has_selected_dir_path = TRUE;
  }
  if (!RestorePanelViewportSnapshot(panel->vol, panel, &snapshot,
                                    snapshot.top_dir_path)) {
    int idx = FindDirIndex(panel->vol, target);
    if (idx < 0)
      idx = 0;
    PositionPanelAtIndex(panel, idx);
  }
}

BOOL DirOps_SelectVisibleDirAndRefresh(ViewContext *ctx, YtreeNovaPanel *panel,
                                       const DirEntry *target,
                                       DirEntry **dir_entry_ptr) {
  const Statistic *s;
  WINDOW *dir_win;
  int idx;
  DirEntry *selected;
  char path[PATH_LENGTH];

  if (!ctx || !panel || !panel->vol || !target || !dir_entry_ptr)
    return FALSE;
  if (!panel->vol->dir_entry_list || panel->vol->total_dirs <= 0)
    return FALSE;

  while (target && !PanelDirIsVisible(panel, target))
    target = target->up_tree;
  if (!target)
    target = panel->vol->vol_stats.tree;

  idx = FindDirIndex(panel->vol, target);
  if (idx < 0)
    return FALSE;

  PositionPanelAtIndex(panel, idx);
  selected = panel->vol->dir_entry_list[panel->disp_begin_pos + panel->cursor_pos]
                 .dir_entry;
  if (!selected)
    return FALSE;

  *dir_entry_ptr = selected;
  s = &panel->vol->vol_stats;
  dir_win = panel->pan_dir_window ? panel->pan_dir_window : ctx->ctx_dir_window;

  DisplayTree(ctx, panel->vol, dir_win, panel->disp_begin_pos,
              panel->disp_begin_pos + panel->cursor_pos, TRUE);
  DisplayFileWindow(ctx, panel, selected);
  DisplayDiskStatistic(ctx, s);
  UpdateStatsPanel(ctx, selected, s);
  DisplayAvailBytes(ctx, s);
  GetPath(selected, path);
  DisplayHeaderPath(ctx, path);
  return TRUE;
}

static DirEntry *GetPanelSelectedDir(const YtreeNovaPanel *panel) {
  DirEntry *selected = NULL;

  if (!panel || !panel->vol)
    return NULL;

  if (panel->vol->total_dirs > 0 && panel->vol->dir_entry_list) {
    int idx = panel->disp_begin_pos + panel->cursor_pos;
    if (idx < 0)
      idx = 0;
    if (idx >= panel->vol->total_dirs)
      idx = panel->vol->total_dirs - 1;
    selected = panel->vol->dir_entry_list[idx].dir_entry;
  }

  if (!selected && panel->file_selection_dir_path[0] != '\0')
    selected = FindDirByPath(panel->vol, panel->file_selection_dir_path);
  if (!selected)
    selected = panel->file_dir_entry;
  if (!selected)
    selected = panel->vol->vol_stats.tree;

  return selected;
}

static void CaptureInactiveFallbackSnapshot(ViewContext *ctx, YtreeNovaPanel *p,
                                            InactiveFallbackSnapshot *snapshot) {
  YtreeNovaPanel *inactive;
  DirEntry *selected;

  if (!snapshot)
    return;

  snapshot->panel = NULL;
  snapshot->selected_path[0] = '\0';
  snapshot->next_sibling_path[0] = '\0';
  snapshot->prev_sibling_path[0] = '\0';

  if (!ctx || !p || !ctx->is_split_screen)
    return;

  inactive = (p == ctx->left) ? ctx->right : ctx->left;
  if (!inactive || inactive->vol != p->vol)
    return;

  selected = GetPanelSelectedDir(inactive);
  if (!selected)
    return;

  snapshot->panel = inactive;
  GetPath(selected, snapshot->selected_path);
  snapshot->selected_path[PATH_LENGTH] = '\0';
  if (selected->next) {
    GetPath(selected->next, snapshot->next_sibling_path);
    snapshot->next_sibling_path[PATH_LENGTH] = '\0';
  }
  if (selected->prev) {
    GetPath(selected->prev, snapshot->prev_sibling_path);
    snapshot->prev_sibling_path[PATH_LENGTH] = '\0';
  }
}

static const DirEntry *ResolveInactiveFallbackTarget(
    const InactiveFallbackSnapshot *snapshot) {
  const struct Volume *vol;
  DirEntry *target;
  char root_path[PATH_LENGTH + 1];

  if (!snapshot || !snapshot->panel || !snapshot->panel->vol)
    return NULL;
  vol = snapshot->panel->vol;

  target = FindVisibleDirByPath(vol, snapshot->selected_path);
  if (target)
    return target;

  root_path[0] = '\0';
  if (vol->vol_stats.tree) {
    GetPath(vol->vol_stats.tree, root_path);
    root_path[PATH_LENGTH] = '\0';
  }

  target = FindDirByPathOrAncestor(vol, snapshot->selected_path);
  while (target) {
    char target_path[PATH_LENGTH + 1];

    GetPath(target, target_path);
    target_path[PATH_LENGTH] = '\0';
    if (FindDirIndex(vol, target) >= 0 &&
        (root_path[0] == '\0' || strcmp(target_path, root_path) != 0)) {
      return target;
    }
    target = target->up_tree;
  }

  target = FindVisibleDirByPath(vol, snapshot->next_sibling_path);
  if (target)
    return target;

  target = FindVisibleDirByPath(vol, snapshot->prev_sibling_path);
  if (target)
    return target;

  return vol->vol_stats.tree;
}

static void CaptureInactiveFallback(ViewContext *ctx, YtreeNovaPanel *p,
                                    const DirEntry *dir_entry,
                                    YtreeNovaPanel **inactive_out,
                                    DirEntry **inactive_fallback_out) {
  YtreeNovaPanel *inactive = NULL;
  DirEntry *inactive_de = NULL;
  DirEntry *inactive_fallback = NULL;

  if (inactive_out)
    *inactive_out = NULL;
  if (inactive_fallback_out)
    *inactive_fallback_out = NULL;
  if (!ctx || !p || !ctx->is_split_screen)
    return;

  inactive = (p == ctx->left) ? ctx->right : ctx->left;
  if (!inactive || inactive->vol != p->vol)
    return;

  inactive_de = GetPanelSelectedDir(inactive);

  inactive_fallback = inactive_de;
  if (dir_entry != NULL) {
    while (inactive_fallback && inactive_fallback != dir_entry &&
           IsDescendant(dir_entry, inactive_fallback)) {
      inactive_fallback = inactive_fallback->up_tree;
    }
  }
  if (!inactive_fallback)
    inactive_fallback = p->vol->vol_stats.tree;

  if (inactive_out)
    *inactive_out = inactive;
  if (inactive_fallback_out)
    *inactive_fallback_out = inactive_fallback;
}

void HandleCollapseSubTree(ViewContext *ctx, DirEntry *dir_entry,
                           BOOL *need_dsp_help, YtreeNovaPanel *p) {
  Statistic *s = &p->vol->vol_stats;
  YtreeNovaPanel *inactive = NULL;
  DirEntry *inactive_fallback = NULL;
  DirEntry *de_ptr;
  FileEntry *fe_ptr, *next_fe_ptr;

  if (!dir_entry || dir_entry->not_scanned || dir_entry->sub_tree == NULL)
    return;

  CaptureInactiveFallback(ctx, p, dir_entry, &inactive, &inactive_fallback);
  if (ctx->left && ctx->left->vol == p->vol)
    PanelTags_PruneUnderDir(ctx->left, dir_entry);
  if (ctx->right && ctx->right->vol == p->vol)
    PanelTags_PruneUnderDir(ctx->right, dir_entry);

  for (de_ptr = dir_entry->sub_tree; de_ptr; de_ptr = de_ptr->next) {
    UnReadTree(ctx, de_ptr, s);
  }
  for (fe_ptr = dir_entry->file; fe_ptr; fe_ptr = next_fe_ptr) {
    next_fe_ptr = fe_ptr->next;
    RemoveFile(ctx, fe_ptr, s);
  }

  dir_entry->not_scanned = TRUE;
  dir_entry->unlogged_flag = TRUE;
  BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);
  BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);

  if (inactive && inactive->vol == p->vol) {
    ReanchorPanelToDir(inactive, inactive_fallback);
    BuildFileEntryList(ctx, inactive);
  }

  DisplayTree(ctx, p->vol, p->pan_dir_window, p->disp_begin_pos,
              p->disp_begin_pos + p->cursor_pos, TRUE);
  DisplayFileWindow(ctx, p, dir_entry);
  DisplayAvailBytes(ctx, s);
  RecalculateSysStats(ctx, s);
  DisplayDiskStatistic(ctx, s);
  UpdateStatsPanel(ctx, dir_entry, s);
  *need_dsp_help = TRUE;
}

void HandleUnreadSubTree(ViewContext *ctx, DirEntry *dir_entry,
                         DirEntry *de_ptr, BOOL *need_dsp_help, YtreeNovaPanel *p) {
  Statistic *s = &p->vol->vol_stats;
  YtreeNovaPanel *inactive = NULL;
  DirEntry *inactive_fallback = NULL;
  FileEntry *fe_ptr, *next_fe_ptr;

  if (s->log_mode != DISK_MODE && s->log_mode != USER_MODE &&
      s->log_mode != ARCHIVE_MODE) {
    return;
  }
  if (!dir_entry || dir_entry->unlogged_flag) {
    return;
  }

  CaptureInactiveFallback(ctx, p, dir_entry, &inactive, &inactive_fallback);
  if (ctx->left && ctx->left->vol == p->vol)
    PanelTags_PruneUnderDir(ctx->left, dir_entry);
  if (ctx->right && ctx->right->vol == p->vol)
    PanelTags_PruneUnderDir(ctx->right, dir_entry);

  for (de_ptr = dir_entry->sub_tree; de_ptr; de_ptr = de_ptr->next) {
    UnReadTree(ctx, de_ptr, s);
  }
  for (fe_ptr = dir_entry->file; fe_ptr; fe_ptr = next_fe_ptr) {
    next_fe_ptr = fe_ptr->next;
    RemoveFile(ctx, fe_ptr, s);
  }
  dir_entry->not_scanned = TRUE;
  dir_entry->unlogged_flag = TRUE;
  BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);
  BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);

  if (inactive && inactive->vol == p->vol) {
    ReanchorPanelToDir(inactive, inactive_fallback);
    BuildFileEntryList(ctx, inactive);
  }

  DisplayTree(ctx, p->vol, p->pan_dir_window, p->disp_begin_pos,
              p->disp_begin_pos + p->cursor_pos, TRUE);
  DisplayFileWindow(ctx, p, dir_entry);
  DisplayAvailBytes(ctx, s);
  RecalculateSysStats(ctx, s);
  DisplayDiskStatistic(ctx, s);
  UpdateStatsPanel(ctx, dir_entry, s);
  *need_dsp_help = TRUE;
  return;
}

BOOL HandleDirMakeFile(ViewContext *ctx, DirEntry *dir_entry) {
  char file_name[PATH_LENGTH * 2 + 1];

  DEBUG_LOG("ACTION_CMD_MKFILE reached in ctrl_dir.c. mode=%d", ctx->view_mode);
  if (ctx->view_mode != DISK_MODE)
    return FALSE;
  if (!AppStateValidatedDispatchSurface("surface.filesystem-mutation-result"))
    return FALSE;
  if (!AppStateValidatedEvent("event.filesystem-mutation-result"))
    return FALSE;

  ClearHelp(ctx);
  *file_name = '\0';
  if (UI_ReadString(ctx, ctx->active, "MAKE FILE:", file_name, PATH_LENGTH,
                    HST_FILE) == CR) {
    int mk_result =
        MakeFile(ctx, dir_entry, file_name, &ctx->active->vol->vol_stats, NULL,
                 UI_ChoiceResolver);
    if (mk_result == 0) {
      if (ctx->active && ctx->active->pan_file_window) {
        DisplayFileWindow(ctx, ctx->active, dir_entry);
      }
      RefreshView(ctx, dir_entry);
    } else if (mk_result == 1) {
      MESSAGE(ctx, "File already exists!");
    } else {
      MESSAGE(ctx, "Can't create File*\"%s\"", file_name);
    }
  }

  return TRUE;
}

void HandleDirMakeDirectory(ViewContext *ctx, DirEntry *dir_entry,
                            Statistic *s) {
  char dir_name[PATH_LENGTH * 2 + 1];
  char active_anchor_path[PATH_LENGTH + 1];
  YtreeNovaPanel *inactive = NULL;
  DirEntry *inactive_fallback = NULL;
  char inactive_path[PATH_LENGTH + 1];
  BOOL has_inactive_path = FALSE;
  BOOL restore_inactive_file_anchor = FALSE;
  int inactive_start_file = 0;
  int inactive_file_cursor = 0;
  char inactive_file_dir_path[PATH_LENGTH + 1];
  char inactive_file_name[PATH_LENGTH + 1];
  PathList *inactive_tagged_snapshot = NULL;

  if (!ctx || !ctx->active || !ctx->active->vol || !dir_entry)
    return;
  if (!AppStateValidatedDispatchSurface("surface.filesystem-mutation-result"))
    return;
  if (!AppStateValidatedEvent("event.filesystem-mutation-result"))
    return;

  DebugLogSplitState("HandleDirMakeDirectory:entry", ctx);
  GetPath(dir_entry, active_anchor_path);
  active_anchor_path[PATH_LENGTH] = '\0';
  ClearHelp(ctx);
  *dir_name = '\0';
  CaptureInactiveFallback(ctx, ctx->active, NULL, &inactive,
                          &inactive_fallback);
  if (inactive_fallback) {
    char fallback_path[PATH_LENGTH + 1];
    GetPath(inactive_fallback, fallback_path);
    fallback_path[PATH_LENGTH] = '\0';
    DEBUG_LOG("HandleDirMakeDirectory:inactive_fallback='%s'", fallback_path);
  } else {
    DEBUG_LOG("HandleDirMakeDirectory:inactive_fallback=<null>");
  }
  if (inactive && inactive->vol == ctx->active->vol && inactive_fallback) {
    GetPath(inactive_fallback, inactive_path);
    inactive_path[PATH_LENGTH] = '\0';
    has_inactive_path = TRUE;
  }
  if (inactive && inactive->vol == ctx->active->vol &&
      inactive->file_selection_dir_path[0] != '\0' &&
      inactive->file_selection_name[0] != '\0') {
    inactive_start_file = inactive->start_file;
    inactive_file_cursor = inactive->file_cursor_pos;
    (void)snprintf(inactive_file_dir_path, sizeof(inactive_file_dir_path), "%s",
                   inactive->file_selection_dir_path);
    (void)snprintf(inactive_file_name, sizeof(inactive_file_name), "%s",
                   inactive->file_selection_name);
    if (inactive_file_dir_path[0] == '\0' || inactive_file_name[0] == '\0') {
      DirEntry *inactive_dir = inactive->file_dir_entry;
      if (!inactive_dir && inactive->vol->total_dirs > 0) {
        int inactive_idx = inactive->disp_begin_pos + inactive->cursor_pos;
        if (inactive_idx < 0)
          inactive_idx = 0;
        if (inactive_idx >= inactive->vol->total_dirs)
          inactive_idx = inactive->vol->total_dirs - 1;
        inactive_dir = inactive->vol->dir_entry_list[inactive_idx].dir_entry;
      }
      if (inactive_dir) {
        inactive->file_dir_entry = inactive_dir;
        CapturePanelSelectionAnchor(ctx, inactive, inactive_dir);
        (void)snprintf(inactive_file_dir_path, sizeof(inactive_file_dir_path),
                       "%s", inactive->file_selection_dir_path);
        (void)snprintf(inactive_file_name, sizeof(inactive_file_name), "%s",
                       inactive->file_selection_name);
      }
    }
    CapturePanelTaggedSnapshot(inactive, &inactive_tagged_snapshot);
    restore_inactive_file_anchor = TRUE;
  }
  if (UI_ReadString(ctx, ctx->active, "MAKE DIRECTORY:", dir_name, PATH_LENGTH,
                    HST_FILE) == CR) {
    DEBUG_LOG("HandleDirMakeDirectory:requested='%s'", dir_name);
    DebugLogSplitState("HandleDirMakeDirectory:before_make", ctx);
  if (!MakeDirectory(ctx, ctx->active, dir_entry, dir_name, s)) {
      DebugLogSplitState("HandleDirMakeDirectory:after_make_before_rebuild",
                         ctx);
      ctx->active->vol->volume_generation++;
      BuildDirEntryList(ctx, ctx->active->vol, &ctx->active->current_dir_entry);
      if (active_anchor_path[0] != '\0') {
        ReanchorPanelToDir(
            ctx->active,
            ResolvePanelAnchorTarget(ctx->active, ctx->active->vol,
                                     active_anchor_path));
      }
      if (inactive && inactive->vol == ctx->active->vol) {
        const DirEntry *inactive_target = inactive_fallback;
        if (has_inactive_path) {
          inactive_target =
              ResolvePanelAnchorTarget(inactive, ctx->active->vol,
                                       inactive_path);
        }
        ReanchorPanelToDir(inactive, inactive_target);
        if (restore_inactive_file_anchor) {
          DirEntry *resolved_file_dir =
              ResolvePanelAnchorTarget(inactive, inactive->vol,
                                       inactive_file_dir_path);
          if (resolved_file_dir)
            inactive->file_dir_entry = resolved_file_dir;
          inactive->start_file = inactive_start_file;
          inactive->file_cursor_pos = inactive_file_cursor;
          (void)snprintf(inactive->file_selection_dir_path,
                         sizeof(inactive->file_selection_dir_path), "%s",
                         inactive_file_dir_path);
          (void)snprintf(inactive->file_selection_name,
                         sizeof(inactive->file_selection_name), "%s",
                         inactive_file_name);
          if (resolved_file_dir) {
            DirOps_ReloadPanelFileAnchorIfMissing(ctx, inactive,
                                                  resolved_file_dir);
          }
        }
        RestoreTaggedSnapshot(ctx, inactive->vol, inactive_tagged_snapshot);
        PanelTags_Restore(ctx, inactive);
        BuildFileEntryList(ctx, inactive);
        DebugLogSplitState("HandleDirMakeDirectory:after_inactive_restore",
                           ctx);
      }
      RefreshView(ctx, dir_entry);
      DebugLogSplitState("HandleDirMakeDirectory:after_refresh", ctx);
    }
  }
  wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);
  FreePathList(inactive_tagged_snapshot);
}

DirEntry *HandleDirDeleteDirectory(ViewContext *ctx, DirEntry *dir_entry) {
  InactiveFallbackSnapshot inactive_snapshot;
  PanelViewportSnapshot active_viewport;
  const Statistic *s;

  if (!ctx || !ctx->active || !ctx->active->vol || !dir_entry)
    return dir_entry;
  if (!AppStateValidatedDispatchSurface("surface.filesystem-mutation-result"))
    return dir_entry;
  if (!AppStateValidatedEvent("event.filesystem-mutation-result"))
    return dir_entry;

  s = (ctx && ctx->active && ctx->active->vol) ? &ctx->active->vol->vol_stats
                                               : NULL;
  CaptureInactiveFallbackSnapshot(ctx, ctx->active, &inactive_snapshot);
  CapturePanelViewportSnapshot(ctx->active, ctx->active->vol, &active_viewport);

  if (!DeleteDirectory(ctx, dir_entry, UI_ChoiceResolver)) {
    ctx->active->vol->volume_generation++;
  }

  BuildDirEntryList(ctx, ctx->active->vol, &ctx->active->current_dir_entry);
  if (ctx->active->vol && ctx->active->vol->dir_entry_list &&
      ctx->active->vol->total_dirs > 0) {
    if (!RestorePanelViewportSnapshot(ctx->active->vol, ctx->active,
                                      &active_viewport,
                                      active_viewport.top_dir_path)) {
      PositionPanelAtIndex(ctx->active, 0);
    }
  }
  if (inactive_snapshot.panel && inactive_snapshot.panel->vol == ctx->active->vol) {
    const DirEntry *inactive_target =
        ResolveInactiveFallbackTarget(&inactive_snapshot);
    ReanchorPanelToDir(inactive_snapshot.panel, inactive_target);
    BuildFileEntryList(ctx, inactive_snapshot.panel);
  }
  if (!ctx->active || !ctx->active->vol || !ctx->active->vol->dir_entry_list ||
      ctx->active->vol->total_dirs <= 0) {
    dir_entry = ResolveActiveDirEntry(ctx, s);
    if (dir_entry) {
      dir_entry->start_file = 0;
      dir_entry->cursor_pos = -1;
      RefreshView(ctx, dir_entry);
    }
    return dir_entry;
  }
  dir_entry = ResolveActiveDirEntry(ctx, s);
  if (dir_entry) {
    dir_entry->start_file = 0;
    dir_entry->cursor_pos = -1;
    RefreshView(ctx, dir_entry);
  }
  return dir_entry;
}

DirEntry *HandleDirRenameDirectory(ViewContext *ctx, DirEntry *dir_entry) {
  char new_name[PATH_LENGTH + 1];
  const Statistic *s;

  if (!ctx || !ctx->active || !ctx->active->vol || !dir_entry)
    return dir_entry;
  if (!AppStateValidatedDispatchSurface("surface.filesystem-mutation-result"))
    return dir_entry;
  if (!AppStateValidatedEvent("event.filesystem-mutation-result"))
    return dir_entry;

  s = (ctx && ctx->active && ctx->active->vol) ? &ctx->active->vol->vol_stats
                                               : NULL;
  if (!GetRenameParameter(ctx, dir_entry->name, new_name)) {
    int rename_result = RenameDirectory(ctx, dir_entry, new_name);
    if (!rename_result) {
      ctx->active->vol->volume_generation++;
      BuildDirEntryList(ctx, ctx->active->vol, &ctx->active->current_dir_entry);
      dir_entry = ResolveActiveDirEntry(ctx, s);
    }
    if (dir_entry)
      RefreshView(ctx, dir_entry);
  }

  return dir_entry;
}

void HandleShowAll(ViewContext *ctx, BOOL tagged_only, BOOL all_volumes,
                   DirEntry *dir_entry, BOOL *need_dsp_help, int *ch,
                   YtreeNovaPanel *p) {
  Statistic *s = &p->vol->vol_stats;
  long long visible_count = 0;
  struct Volume *vol_iter;

  if (all_volumes) {
    struct Volume *vol_tmp;
    HASH_ITER(hh, ctx->volumes_head, vol_iter, vol_tmp) {
      Statistic *vs = &vol_iter->vol_stats;
      visible_count +=
          tagged_only ? vs->disk_tagged_files : vs->disk_matching_files;
    }
  } else {
    visible_count = tagged_only ? s->disk_tagged_files : s->disk_matching_files;
  }

  if (visible_count > 0) {
    int result;
    if (dir_entry->log_flag) {
      dir_entry->log_flag = FALSE;
    } else {
      dir_entry->big_window = TRUE;
      dir_entry->global_flag = TRUE;
      dir_entry->global_all_volumes = all_volumes;
      dir_entry->tagged_flag = tagged_only;
      dir_entry->start_file = 0;
      dir_entry->cursor_pos = 0;
    }
    /*
     * Tree-to-file handoff must not replay queued tree keystrokes in the
     * file pane. Flush pending input so the new file session starts from a
     * clean event boundary instead of consuming stale keys from the tree
     * controller.
     */
    flushinp();
    result = HandleFileWindow(ctx, dir_entry);
    if (!AppStateCommitPanelFocus(
            ctx, ctx->active, (result == '\\') ? FOCUS_FILE : FOCUS_TREE))
      return;

    if (result != LOG_ESC) {
      /* Restore normal mode and refresh the entire view */
      dir_entry->start_file = 0;
      dir_entry->cursor_pos = -1;
      BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);
      dir_entry = GetPanelDirEntry(p);
      RefreshView(ctx, dir_entry);
      dir_entry->global_all_volumes = FALSE;
    } else {
      BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);
      dir_entry = GetPanelDirEntry(p);
      RefreshView(ctx, dir_entry);
      *ch = 'L';
      dir_entry->global_all_volumes = FALSE;
    }
  } else {
    dir_entry->log_flag = FALSE;
  }
  *need_dsp_help = TRUE;
  return;
}

void HandleSwitchWindow(ViewContext *ctx, DirEntry *dir_entry,
                        BOOL *need_dsp_help, int *ch, YtreeNovaPanel *p) {
  /* Critical Safety: Check for volume changes upon return from File Window */
  const struct Volume *start_vol = p->vol;
  const Statistic *s = &p->vol->vol_stats;
  char current_dir_path[PATH_LENGTH + 1];
  BOOL restore_saved_file_window = FALSE;

  current_dir_path[0] = '\0';
  GetPath(dir_entry, current_dir_path);
  current_dir_path[PATH_LENGTH] = '\0';
  if (p->saved_focus == FOCUS_FILE &&
      p->file_selection_dir_path[0] == '\0') {
    /*
     * Split ownership boundary: a file-focused panel must carry a stable
     * directory identity even if an older transition left the cached path
     * empty. Rehydrate it from the authoritative tree selection before any
     * restore logic consumes the snapshot.
     */
    (void)snprintf(p->file_selection_dir_path,
                   sizeof(p->file_selection_dir_path), "%s",
                   current_dir_path);
    p->file_selection_dir_path[PATH_LENGTH] = '\0';
  }
  assert(p->saved_focus != FOCUS_FILE || p->file_selection_dir_path[0] != '\0');
  /*
   * Split-close handoff can leave the focus mirror stale while the panel-local
   * file-selection identity remains authoritative. Re-enter file mode whenever
   * the current tree row matches that identity, regardless of the last focus
   * mirror value.
   */
  if (p->file_selection_dir_path[0] != '\0' &&
      strcmp(current_dir_path, p->file_selection_dir_path) == 0) {
    restore_saved_file_window = TRUE;
  }

  if (dir_entry->matching_files) {
    if (dir_entry->log_flag) {
      dir_entry->log_flag = FALSE;
    } else {
      dir_entry->global_flag = FALSE;
      dir_entry->global_all_volumes = FALSE;
      dir_entry->tagged_flag = FALSE;
      if (restore_saved_file_window && p->saved_big_file_view)
        dir_entry->big_window = TRUE;
      else
        dir_entry->big_window = ctx->bypass_small_window;
      if (restore_saved_file_window) {
        dir_entry->start_file = p->start_file;
        dir_entry->cursor_pos = p->file_cursor_pos;
      } else {
        /*
         * A fresh file-view entry should start at the top of the file list,
         * not inherit the tree cursor position that happened to select the
         * directory. Preserve the saved file cursor only for the directory
         * that already owns a file-selection snapshot.
         */
        dir_entry->start_file = 0;
        dir_entry->cursor_pos = 0;
      }
      /* Preserve per-directory file selection when returning to file view. */
      if (dir_entry->start_file < 0)
        dir_entry->start_file = 0;
      if (dir_entry->cursor_pos < 0)
        dir_entry->cursor_pos = 0;
    }
    DEBUG_LOG("DEBUG: HandleSwitchWindow calling HandleFileWindow for %s",
              dir_entry->name);
    /*
     * Tree-to-file handoff must not replay queued tree keystrokes in the
     * file pane. Flush any pending input so the newly focused mode starts from
     * a clean event boundary instead of consuming stale keys from the prior
     * tree session.
     */
    flushinp();
    if (HandleFileWindow(ctx, dir_entry) != LOG_ESC) {
      /* Safety Check: If volume was deleted in File Window (via
       * SelectLoadedVolume), abort */
      if (ctx->active->vol != start_vol)
        return;

      dir_entry = GetPanelDirEntry(p);
      if (!dir_entry)
        return;

      p->file_dir_entry = dir_entry;
      p->start_file = dir_entry->start_file;
      p->file_cursor_pos = dir_entry->cursor_pos;
      CapturePanelSelectionAnchor(ctx, p, dir_entry);

      /* Check if the panel we were handling is still valid/active.
       * A normal TAB panel switch from file view changes ctx->active while
       * split mode remains enabled; the caller will handle that hand-off.
       * Only force a full redraw here when the original panel is actually
       * gone (unsplit path). */
      if (p != ctx->active) {
        if (!ctx->is_split_screen) {
          /* Split state changed (unsplit), and this panel no longer exists. */
          DisplayMenu(ctx);
          ctx->resize_request = TRUE;
        }
        return;
      }

      /* REPLACEMENT: Use Centralized Refresh */
      RefreshView(ctx, dir_entry);

    } else {
      /* ... existing LOG_ESC handling ... */
      BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);
      dir_entry = GetPanelDirEntry(p);

      /* Ensure visual consistency here too */
      if (dir_entry)
        RefreshView(ctx, dir_entry);

      *ch = 'L';
    }
    DisplayAvailBytes(ctx, s);
    *need_dsp_help = TRUE;
  } else {
    dir_entry->log_flag = FALSE;
  }
  return;
}

void SyncActivePanelWindows(ViewContext *ctx) {
  if (!ctx || !ctx->active)
    return;

  ctx->ctx_dir_window = ctx->active->pan_dir_window;
  ctx->ctx_small_file_window = ctx->active->pan_small_file_window;
  ctx->ctx_big_file_window = ctx->active->pan_big_file_window;
  ctx->ctx_file_window = ctx->active->pan_file_window;
}

DirEntry *ResolveActiveDirEntry(ViewContext *ctx, const Statistic *s) {
  if (!ctx || !ctx->active || !ctx->active->vol)
    return NULL;

  /* Match the renderer's visible-selection handoff so panel switches keep
   * each panel's own tree viewport anchored to the row that is actually
   * visible. */
  if (ctx->active->vol->total_dirs > 0)
    return GetPanelDirEntry(ctx->active);

  if (!s)
    return NULL;
  return s->tree;
}

void RefreshVolumeSwitchViews(ViewContext *ctx, DirEntry *dir_entry,
                              const Statistic *s) {
  char path[PATH_LENGTH];

  DisplayMenu(ctx);
  DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
              ctx->active->disp_begin_pos,
              ctx->active->disp_begin_pos + ctx->active->cursor_pos, TRUE);
  DisplayFileWindow(ctx, ctx->active, dir_entry);
  RefreshWindow(ctx->ctx_file_window);
  DisplayDiskStatistic(ctx, s);
  UpdateStatsPanel(ctx, dir_entry, s);
  DisplayAvailBytes(ctx, s);
  GetPath(dir_entry, path);
  DisplayHeaderPath(ctx, path);
}



void DirOps_ReloadPanelFileAnchorIfMissing(ViewContext *ctx, YtreeNovaPanel *panel,
                                           DirEntry *dir_entry) {
  char dir_path[PATH_LENGTH + 1];
  Statistic *stats;
  PathList *expanded = NULL;
  PathList *tagged = NULL;
  PathList *panel_tagged = NULL;

  if (!ctx || !panel || !panel->vol || !dir_entry)
    return;
  if (panel->file_selection_name[0] == '\0' ||
      panel->file_selection_dir_path[0] == '\0')
    return;

  GetPath(dir_entry, dir_path);
  dir_path[PATH_LENGTH] = '\0';
  if (strcmp(dir_path, panel->file_selection_dir_path) != 0)
    return;

  if (dir_entry->file != NULL || dir_entry->total_files > 0)
    return;
  if (!dir_entry->not_scanned && !dir_entry->unlogged_flag)
    return;

  stats = &panel->vol->vol_stats;
  DEBUG_LOG(
      "RestorePanelFileSelection:reload anchor path='%s' file='%s' not_scanned=%d",
      panel->file_selection_dir_path, panel->file_selection_name,
      dir_entry->not_scanned);

  CapturePanelTaggedSnapshot(panel, &panel_tagged);
  SaveTreeState(stats->tree, &expanded, &tagged);
  DEBUG_LOG("RestorePanelFileSelection:tag_snapshot panel=%d tree=%d",
            CountPathSnapshot(panel_tagged), CountPathSnapshot(tagged));
  InvalidateVolumePanels(ctx, panel->vol);
  if (RescanDir(ctx, dir_entry, 0, stats, Dir_Progress, ctx) == 0) {
    RestoreTreeState(ctx, stats->tree, &expanded, tagged, stats);
    RestoreTaggedSnapshot(ctx, panel->vol, panel_tagged);
    PanelTags_Restore(ctx, panel);
  }
  FreePathList(expanded);
  FreePathList(tagged);
  FreePathList(panel_tagged);

  BuildDirEntryList(ctx, panel->vol, &panel->current_dir_entry);
  ReanchorPanelToDir(panel,
                     ResolvePanelAnchorTarget(panel, panel->vol, dir_path));
}

DirEntry *RestorePanelFileSelection(ViewContext *ctx, DirEntry *dir_entry,
                                    YtreeNovaPanel *panel) {
  int selected_idx = -1;
  unsigned int file_count = 0;

  if (!ctx || !dir_entry || !panel)
    return dir_entry;
  if (panel->saved_focus != FOCUS_FILE)
    return dir_entry;

  /*
   * Split ownership boundary (docs/ARCHITECTURE.md §4.2.1):
   * This restore path may refresh shared topology, but it must rebind file
   * anchors from the panel-local identity tuple and not re-derive selection
   * from raw tree row indices.
   */
  if (panel->file_selection_dir_path[0] == '\0' ||
      panel->file_selection_name[0] == '\0')
    return dir_entry;

  if (panel->vol) {
    char current_dir_path[PATH_LENGTH + 1];
    DirEntry *resolved_dir = NULL;
    DirEntry *exact_dir = NULL;

    GetPath(dir_entry, current_dir_path);
    current_dir_path[PATH_LENGTH] = '\0';
    if (strcmp(current_dir_path, panel->file_selection_dir_path) != 0) {
      exact_dir = FindDirByPath(panel->vol, panel->file_selection_dir_path);
      if (exact_dir)
        (void)EnsureDirVisible(ctx, panel, exact_dir);
      resolved_dir = ResolvePanelAnchorTarget(panel, panel->vol,
                                              panel->file_selection_dir_path);
      if (resolved_dir)
        dir_entry = resolved_dir;
      else if (panel->vol && panel->vol->vol_stats.tree)
        dir_entry = panel->vol->vol_stats.tree;
    }
  }

  DirOps_ReloadPanelFileAnchorIfMissing(ctx, panel, dir_entry);

  dir_entry->start_file = panel->start_file;
  dir_entry->cursor_pos = panel->file_cursor_pos;

  if (panel->file_selection_name[0] != '\0' &&
      panel->file_selection_dir_path[0] != '\0') {
    char current_dir_path[PATH_LENGTH + 1];

    GetPath(dir_entry, current_dir_path);
    current_dir_path[PATH_LENGTH] = '\0';

    if (strcmp(current_dir_path, panel->file_selection_dir_path) == 0) {
      int i;

      BuildFileEntryList(ctx, panel);
      for (i = 0; i < (int)panel->file_count; i++) {
        const FileEntry *fe = panel->file_entry_list[i].file;
        if (fe && strcmp(fe->name, panel->file_selection_name) == 0) {
          selected_idx = i;
          break;
        }
      }
    }
  }

  if (selected_idx >= 0) {
    int max_disp_files = FileNav_GetMaxDispFiles(ctx);
    int start = dir_entry->start_file;

    if (max_disp_files < 1)
      max_disp_files = 1;

    if (start < 0)
      start = 0;
    if (selected_idx < start)
      start = selected_idx;
    else if (selected_idx >= start + max_disp_files)
      start = selected_idx - max_disp_files + 1;
    if (start < 0)
      start = 0;

    dir_entry->start_file = start;
    dir_entry->cursor_pos = selected_idx - start;
  }

  file_count = panel->file_count;
  if (file_count == 0 && panel->file_selection_dir_path[0] != '\0') {
    BuildFileEntryList(ctx, panel);
    file_count = panel->file_count;
  }
  if (file_count == 0) {
    DEBUG_LOG("RestorePanelFileSelection:empty_file_list path='%s' file='%s'",
              panel->file_selection_dir_path, panel->file_selection_name);
    dir_entry->start_file = 0;
    dir_entry->cursor_pos = 0;
  } else {
    int original_start = dir_entry->start_file;
    int original_cursor = dir_entry->cursor_pos;
    if (dir_entry->start_file < 0)
      dir_entry->start_file = 0;
    if ((unsigned int)dir_entry->start_file >= file_count)
      dir_entry->start_file = (int)file_count - 1;
    if (dir_entry->cursor_pos < 0)
      dir_entry->cursor_pos = 0;
    if ((unsigned int)(dir_entry->start_file + dir_entry->cursor_pos) >=
        file_count) {
      dir_entry->cursor_pos = (int)file_count - 1 - dir_entry->start_file;
      if (dir_entry->cursor_pos < 0)
        dir_entry->cursor_pos = 0;
    }
    if (original_start != dir_entry->start_file ||
        original_cursor != dir_entry->cursor_pos) {
      DEBUG_LOG(
          "RestorePanelFileSelection:clamp start=%d->%d cursor=%d->%d count=%u",
          original_start, dir_entry->start_file, original_cursor,
          dir_entry->cursor_pos, file_count);
    }
  }

  panel->file_dir_entry = dir_entry;
  panel->start_file = dir_entry->start_file;
  panel->file_cursor_pos = dir_entry->cursor_pos;
  if (!AppStateCommitPanelFocus(ctx, panel, FOCUS_FILE))
    return dir_entry;
  if (!dir_entry->global_flag && !dir_entry->tagged_flag) {
    dir_entry->big_window = panel->saved_big_file_view;
  }
  if (dir_entry->start_file < 0)
    dir_entry->start_file = 0;
  if (dir_entry->cursor_pos < 0 && dir_entry->total_files > 0)
    dir_entry->cursor_pos = 0;
  DEBUG_LOG(
      "RestorePanelFileSelection:dir='%s' total=%u matching=%u file_count=%u "
      "start=%d cursor=%d focus=%d spec='%s'",
      dir_entry->name ? dir_entry->name : "<null>",
      (unsigned int)dir_entry->total_files,
      (unsigned int)dir_entry->matching_files, panel->file_count,
      dir_entry->start_file, dir_entry->cursor_pos, panel->saved_focus,
      panel->vol ? panel->vol->vol_stats.file_spec : "");
  return dir_entry;
}

DirWindowDispatchResult
HandleDirWindowPanelAction(ViewContext *ctx, YtreeNovaAction action,
                           DirEntry **dir_entry_ptr, Statistic **s_ptr,
                           const struct Volume **start_vol_ptr,
                           BOOL *need_dsp_help_ptr, int *ch_ptr,
                           const int *unput_char_ptr) {
  if (!ctx || !ctx->active || !dir_entry_ptr || !*dir_entry_ptr || !s_ptr ||
      !*s_ptr || !start_vol_ptr || !*start_vol_ptr || !need_dsp_help_ptr ||
      !ch_ptr || !unput_char_ptr) {
    return DIR_WINDOW_DISPATCH_HANDLED;
  }

  switch (action) {
  case ACTION_VIEW_PREVIEW: {
    const YtreeNovaPanel *saved_panel = ctx->active;

    ctx->preview_return_panel = ctx->active;
    ctx->preview_return_focus = ctx->focused_window;
    ctx->preview_mode = TRUE;
    ctx->preview_entry_focus = FOCUS_TREE;

    HandleSwitchWindow(ctx, *dir_entry_ptr, need_dsp_help_ptr, ch_ptr,
                       ctx->active);

    ctx->preview_mode = FALSE;
    if (!AppStateCommitPanelFocus(ctx, ctx->active, FOCUS_TREE))
      return DIR_WINDOW_DISPATCH_RETURN_ESC;

    if (ctx->active != saved_panel) {
      if (ctx->active->vol == NULL)
        return DIR_WINDOW_DISPATCH_RETURN_ESC;

      *start_vol_ptr = ctx->active->vol;
      *s_ptr = &ctx->active->vol->vol_stats;
      *dir_entry_ptr = ResolveActiveDirEntry(ctx, *s_ptr);
      SyncActivePanelWindows(ctx);
      RefreshView(ctx, *dir_entry_ptr);
      *need_dsp_help_ptr = TRUE;
      return DIR_WINDOW_DISPATCH_CONTINUE;
    }

    *dir_entry_ptr = ResolveActiveDirEntry(ctx, *s_ptr);
    RefreshView(ctx, *dir_entry_ptr);
    *need_dsp_help_ptr = TRUE;
    return DIR_WINDOW_DISPATCH_HANDLED;
  }

  default:
    break;
  }

  return DIR_WINDOW_DISPATCH_UNHANDLED;
}
DirWindowDispatchResult
HandleDirWindowEnterAction(ViewContext *ctx, DirEntry **dir_entry_ptr,
                           Statistic **s_ptr,
                           const struct Volume **start_vol_ptr,
                           BOOL *need_dsp_help_ptr, int *ch_ptr,
                           const int *unput_char_ptr, YtreeNovaAction *action_ptr) {
  const YtreeNovaPanel *saved_panel;

  if (!ctx || !ctx->active || !dir_entry_ptr || !*dir_entry_ptr || !s_ptr ||
      !*s_ptr || !start_vol_ptr || !*start_vol_ptr || !need_dsp_help_ptr ||
      !ch_ptr || !unput_char_ptr || !action_ptr) {
    return DIR_WINDOW_DISPATCH_HANDLED;
  }

  if (ctx->refresh_mode & REFRESH_ON_ENTER) {
    *dir_entry_ptr = RefreshTreeSafe(ctx, ctx->active, *dir_entry_ptr);
    *dir_entry_ptr = ResolveActiveDirEntry(ctx, *s_ptr);
  }

  DEBUG_LOG("ACTION_ENTER: dir_entry=%p name=%s matching=%u",
            (void *)*dir_entry_ptr,
            *dir_entry_ptr ? (*dir_entry_ptr)->name : "NULL",
            *dir_entry_ptr ? (unsigned int)(*dir_entry_ptr)->matching_files
                           : 0U);

  if (*dir_entry_ptr &&
      ((*dir_entry_ptr)->unlogged_flag ||
       ((*dir_entry_ptr)->not_scanned && (*dir_entry_ptr)->total_files == 0))) {
    DirEntry *child;
    char new_log_path[PATH_LENGTH + 1];
    char saved_selected_path[PATH_LENGTH + 1];
    char saved_top_path[PATH_LENGTH + 1];
    int saved_disp_begin = ctx->active->disp_begin_pos;
    int saved_cursor_pos = ctx->active->cursor_pos;
    int win_height;
    int dummy_width;

    new_log_path[0] = '\0';
    saved_selected_path[0] = '\0';
    saved_top_path[0] = '\0';
    GetPath(*dir_entry_ptr, saved_selected_path);
    if (ctx->active->vol->total_dirs > 0 && ctx->active->vol->dir_entry_list &&
        saved_disp_begin >= 0 &&
        saved_disp_begin < ctx->active->vol->total_dirs) {
      GetPath(ctx->active->vol->dir_entry_list[saved_disp_begin].dir_entry,
              saved_top_path);
    }

    HandlePlus(ctx, *dir_entry_ptr, NULL, new_log_path, need_dsp_help_ptr,
               ctx->active);
    *dir_entry_ptr = ResolveActiveDirEntry(ctx, *s_ptr);

    if (*dir_entry_ptr) {
      for (child = (*dir_entry_ptr)->sub_tree; child; child = child->next) {
        child->not_scanned = TRUE;
        child->unlogged_flag = TRUE;
      }
      BuildDirEntryList(ctx, ctx->active->vol, &ctx->active->current_dir_entry);

      GetMaxYX(ctx->active->pan_dir_window, &win_height, &dummy_width);
      if (win_height < 1)
        win_height = 1;
      {
        int i;
        int selected_idx = -1;
        int top_idx = -1;
        int max_begin = MAXIMUM(0, ctx->active->vol->total_dirs - win_height);

        for (i = 0; i < ctx->active->vol->total_dirs; i++) {
          char path[PATH_LENGTH + 1];
          GetPath(ctx->active->vol->dir_entry_list[i].dir_entry, path);
          if (selected_idx < 0 && strcmp(path, saved_selected_path) == 0)
            selected_idx = i;
          if (top_idx < 0 && saved_top_path[0] != '\0' &&
              strcmp(path, saved_top_path) == 0)
            top_idx = i;
          if (selected_idx >= 0 &&
              (top_idx >= 0 || saved_top_path[0] == '\0'))
            break;
        }

        if (selected_idx >= 0) {
          int saved_selected_idx = GetPanelVisibleSelectionIndex(ctx->active);
          if (saved_selected_idx < 0)
            saved_selected_idx = saved_disp_begin + saved_cursor_pos;
          int idx_delta = selected_idx - saved_selected_idx;
          int next_disp_begin =
              (top_idx >= 0) ? top_idx : MAXIMUM(0, saved_disp_begin);

          if (next_disp_begin < 0)
            next_disp_begin = 0;
          if (next_disp_begin > max_begin)
            next_disp_begin = max_begin;

          ctx->active->disp_begin_pos = next_disp_begin;
          ctx->active->cursor_pos = saved_cursor_pos + idx_delta;
        } else {
          int next_disp_begin =
              (top_idx >= 0) ? top_idx : MAXIMUM(0, saved_disp_begin);
          if (next_disp_begin < 0)
            next_disp_begin = 0;
          if (next_disp_begin > max_begin)
            next_disp_begin = max_begin;
          ctx->active->disp_begin_pos = next_disp_begin;
          ctx->active->cursor_pos = saved_cursor_pos;
        }
      }

      if (ctx->active->cursor_pos < 0)
        ctx->active->cursor_pos = 0;
      if (ctx->active->disp_begin_pos + ctx->active->cursor_pos >=
          ctx->active->vol->total_dirs) {
        ctx->active->cursor_pos =
            ctx->active->vol->total_dirs - 1 - ctx->active->disp_begin_pos;
      }
      if (ctx->active->cursor_pos < 0)
        ctx->active->cursor_pos = 0;
      *dir_entry_ptr = ResolveActiveDirEntry(ctx, *s_ptr);
      RefreshView(ctx, *dir_entry_ptr);
    }

    if (!AppStateCommitPanelFocus(ctx, ctx->active, FOCUS_TREE))
      return DIR_WINDOW_DISPATCH_RETURN_ESC;
    *action_ptr = ACTION_NONE;
    return DIR_WINDOW_DISPATCH_HANDLED;
  }

  if (*dir_entry_ptr == NULL || (*dir_entry_ptr)->total_files == 0) {
    UI_Beep(ctx, FALSE);
    *action_ptr = ACTION_NONE;
    return DIR_WINDOW_DISPATCH_HANDLED;
  }

  saved_panel = ctx->active;
  HandleSwitchWindow(ctx, *dir_entry_ptr, need_dsp_help_ptr, ch_ptr, ctx->active);
  *ch_ptr = 0;
  if (!AppStateMirrorActivePanelFocus(ctx))
    return DIR_WINDOW_DISPATCH_RETURN_ESC;

  if (ctx->active != saved_panel) {
    if (ctx->active->vol == NULL)
      return DIR_WINDOW_DISPATCH_RETURN_ESC;

    *start_vol_ptr = ctx->active->vol;
    *s_ptr = &ctx->active->vol->vol_stats;
    *dir_entry_ptr = ResolveActiveDirEntry(ctx, *s_ptr);
    SyncActivePanelWindows(ctx);

    *dir_entry_ptr =
        RestorePanelFileSelection(ctx, *dir_entry_ptr, ctx->active);
    RefreshView(ctx, *dir_entry_ptr);
    *need_dsp_help_ptr = TRUE;
    *action_ptr = ACTION_NONE;
    return DIR_WINDOW_DISPATCH_HANDLED;
  }

  if (ctx->active->vol != *start_vol_ptr)
    return DIR_WINDOW_DISPATCH_RETURN_ESC;

  *dir_entry_ptr = ResolveActiveDirEntry(ctx, *s_ptr);
  if (ctx->active->saved_focus == FOCUS_FILE &&
      ctx->active->file_selection_dir_path[0] != '\0') {
    RestorePanelAnchorPath(ctx->active->vol, ctx->active,
                           ctx->active->file_selection_dir_path);
    *dir_entry_ptr = GetPanelDirEntry(ctx->active);
    *dir_entry_ptr = RestorePanelFileSelection(ctx, *dir_entry_ptr,
                                               ctx->active);
  }
  RefreshView(ctx, *dir_entry_ptr);
  *action_ptr = ACTION_NONE;
  return DIR_WINDOW_DISPATCH_HANDLED;
}

DirWindowDispatchResult
HandleDirWindowVolumeAction(ViewContext *ctx, YtreeNovaAction action,
                            DirEntry **dir_entry_ptr, Statistic **s_ptr,
                            const struct Volume *start_vol,
                            BOOL *need_dsp_help_ptr) {
  int res;

  if (!ctx || !ctx->active || !ctx->active->vol || !dir_entry_ptr ||
      !*dir_entry_ptr || !s_ptr || !*s_ptr || !need_dsp_help_ptr) {
    return DIR_WINDOW_DISPATCH_HANDLED;
  }
  SavePanelTreeViewportSnapshot(ctx->active);

  if (action == ACTION_VOL_MENU) {
    res = SelectLoadedVolume(ctx, NULL);
    if (res != 0) {
      DisplayMenu(ctx);
      *need_dsp_help_ptr = TRUE;
      return DIR_WINDOW_DISPATCH_HANDLED;
    }
  } else if (action == ACTION_VOL_PREV) {
    res = CycleLoadedVolume(ctx, ctx->active, -1);
    if (res != 0)
      return DIR_WINDOW_DISPATCH_HANDLED;
  } else if (action == ACTION_VOL_NEXT) {
    res = CycleLoadedVolume(ctx, ctx->active, 1);
    if (res != 0)
      return DIR_WINDOW_DISPATCH_HANDLED;
  } else {
    return DIR_WINDOW_DISPATCH_UNHANDLED;
  }

  if (ctx->active->vol != start_vol)
    return DIR_WINDOW_DISPATCH_RETURN_ESC;

  *s_ptr = &ctx->active->vol->vol_stats;
  (void)RestorePanelTreeViewportSnapshot(ctx, ctx->active);

  if (ctx->active->vol->total_dirs > 0) {
    if (ctx->active->disp_begin_pos + ctx->active->cursor_pos >=
        ctx->active->vol->total_dirs) {
      int last_idx = ctx->active->vol->total_dirs - 1;
      if (last_idx >= ctx->layout.dir_win_height) {
        ctx->active->disp_begin_pos = last_idx - (ctx->layout.dir_win_height - 1);
        ctx->active->cursor_pos = ctx->layout.dir_win_height - 1;
      } else {
        ctx->active->disp_begin_pos = 0;
        ctx->active->cursor_pos = last_idx;
      }
    }
    *dir_entry_ptr = ResolveActiveDirEntry(ctx, *s_ptr);
  } else {
    *dir_entry_ptr = (*s_ptr)->tree;
  }

  RefreshView(ctx, *dir_entry_ptr);
  *need_dsp_help_ptr = TRUE;
  return DIR_WINDOW_DISPATCH_HANDLED;
}

DirWindowDispatchResult
HandleDirWindowLogAction(ViewContext *ctx, DirEntry **dir_entry_ptr,
                         Statistic **s_ptr, const struct Volume *start_vol,
                         BOOL *need_dsp_help_ptr, char *new_log_path,
                         size_t new_log_path_size) {
  int ret;

  if (!ctx || !ctx->active || !ctx->active->vol || !dir_entry_ptr ||
      !*dir_entry_ptr || !s_ptr || !*s_ptr || !need_dsp_help_ptr ||
      !new_log_path || new_log_path_size == 0) {
    return DIR_WINDOW_DISPATCH_HANDLED;
  }

  if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
    if (getcwd(new_log_path, new_log_path_size) == NULL) {
      (void)snprintf(new_log_path, new_log_path_size, "%s", ".");
    }
  } else {
    (void)GetPath(*dir_entry_ptr, new_log_path);
  }

  if (GetNewLogPath(ctx, ctx->active, new_log_path) != 0) {
    return DIR_WINDOW_DISPATCH_HANDLED;
  }

  DisplayMenu(ctx);
  doupdate();

  ret = LogDisk(ctx, ctx->active, new_log_path);
  if (ret != 0) {
    *need_dsp_help_ptr = TRUE;
    return DIR_WINDOW_DISPATCH_HANDLED;
  }

  if (ctx->active->vol != start_vol)
    return DIR_WINDOW_DISPATCH_RETURN_ESC;

  *s_ptr = &ctx->active->vol->vol_stats;
  if (ctx->active->vol->total_dirs > 0) {
    if (ctx->active->disp_begin_pos + ctx->active->cursor_pos >=
        ctx->active->vol->total_dirs) {
      int last_idx = ctx->active->vol->total_dirs - 1;
      if (last_idx >= ctx->layout.dir_win_height) {
        ctx->active->disp_begin_pos = last_idx - (ctx->layout.dir_win_height - 1);
        ctx->active->cursor_pos = ctx->layout.dir_win_height - 1;
      } else {
        ctx->active->disp_begin_pos = 0;
        ctx->active->cursor_pos = last_idx;
      }
    }
    *dir_entry_ptr = ResolveActiveDirEntry(ctx, *s_ptr);
  } else {
    *dir_entry_ptr = (*s_ptr)->tree;
  }

  DisplayMenu(ctx);
  DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
              ctx->active->disp_begin_pos,
              ctx->active->disp_begin_pos + ctx->active->cursor_pos, TRUE);
  DisplayFileWindow(ctx, ctx->active, *dir_entry_ptr);
  RefreshWindow(ctx->ctx_file_window);
  DisplayDiskStatistic(ctx, *s_ptr);
  DisplayDirStatistic(ctx, *dir_entry_ptr, NULL, *s_ptr);
  DisplayAvailBytes(ctx, *s_ptr);
  {
    char path[PATH_LENGTH];
    GetPath(*dir_entry_ptr, path);
    DisplayHeaderPath(ctx, path);
  }

  *need_dsp_help_ptr = TRUE;
  return DIR_WINDOW_DISPATCH_HANDLED;
}

void ToggleDotFiles(ViewContext *ctx, YtreeNovaPanel *p) {
  DirEntry *target;
  YtreeNovaPanel *inactive = NULL;
  const Statistic *s;
  char inactive_anchor_path[PATH_LENGTH + 1];
  BOOL has_inactive_anchor_path = FALSE;
  PanelViewportSnapshot active_viewport;
  const char *preferred_top_path = NULL;

  if (!ctx || !p || !p->vol)
    return;
  s = &p->vol->vol_stats;
  inactive_anchor_path[0] = '\0';
  CapturePanelViewportSnapshot(p, p->vol, &active_viewport);

  if (ctx->is_split_screen && ctx->left && ctx->right) {
    if (p == ctx->left)
      inactive = ctx->right;
    else if (p == ctx->right)
      inactive = ctx->left;
  }

  if (inactive && inactive->vol == p->vol && inactive->vol->dir_entry_list &&
      inactive->vol->total_dirs > 0) {
    int inactive_idx = inactive->disp_begin_pos + inactive->cursor_pos;
    const DirEntry *inactive_anchor = NULL;

    if (inactive->saved_focus == FOCUS_FILE &&
        inactive->file_selection_dir_path[0] != '\0') {
      (void)snprintf(inactive_anchor_path, sizeof(inactive_anchor_path), "%s",
                     inactive->file_selection_dir_path);
      has_inactive_anchor_path = TRUE;
    } else {
      if (inactive_idx < 0)
        inactive_idx = 0;
      if (inactive_idx >= inactive->vol->total_dirs)
        inactive_idx = inactive->vol->total_dirs - 1;
      inactive_anchor =
          inactive->vol->dir_entry_list[inactive_idx].dir_entry;
      if (inactive_anchor) {
        GetPath((DirEntry *)inactive_anchor, inactive_anchor_path);
        inactive_anchor_path[PATH_LENGTH] = '\0';
        has_inactive_anchor_path = TRUE;
      }
    }
  }

  /* Suspend clock to prevent signal handler interrupt corrupting UI during
   * rebuild */
  SuspendClock(ctx);

  p->hide_dot_files = !p->hide_dot_files;
  p->panel_generation++;
  p->vol->volume_generation++;
  RecalculateSysStats(ctx, s);

  /* Rebuild the linear list of visible directories. */
  BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);

  preferred_top_path = p->tree_viewport_top_dir_path[p->hide_dot_files ? 1 : 0];
  if (preferred_top_path[0] == '\0')
    preferred_top_path = active_viewport.top_dir_path;
  if (!RestorePanelViewportSnapshot(p->vol, p, &active_viewport,
                                    preferred_top_path)) {
    PositionPanelAtIndex(p, 0);
  }

  if (inactive && inactive->vol == p->vol && has_inactive_anchor_path) {
    ReanchorPanelToDir(inactive,
                       ResolvePanelAnchorTarget(inactive, p->vol,
                                                inactive_anchor_path));
  }

  /* Refresh Directory Tree */
  DisplayTree(ctx, p->vol, p->pan_dir_window, p->disp_begin_pos,
              p->disp_begin_pos + p->cursor_pos, TRUE);
  DisplayDiskStatistic(ctx, s);

  /* Update current dir pointer using the new accessor function
  because ToggleDotFiles might have changed the list layout */
  if (p->vol->total_dirs > 0) {
    int selected_idx = GetPanelVisibleSelectionIndex(p);
    if (selected_idx < 0)
      selected_idx = 0;
    target = p->vol->dir_entry_list[selected_idx].dir_entry;
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
DirEntry *RefreshTreeSafe(ViewContext *ctx, YtreeNovaPanel *p, DirEntry *entry) {
  const Statistic *s;
  int saved_disp_begin;
  PanelViewportSnapshot viewport;

  if (!p || !p->vol)
    return entry;
  if (!AppStateValidatedDispatchSurface("surface.refresh-rebuild-rebind"))
    return entry;
  if (!AppStateValidatedEvent("event.refresh-rebuild"))
    return entry;

  s = &p->vol->vol_stats;
  saved_disp_begin = p->disp_begin_pos;
  CapturePanelViewportSnapshot(p, p->vol, &viewport);

  /* RescanDir destroys/recreates FileEntry nodes. Any panel cache on this
   * volume would otherwise keep dangling pointers until that panel becomes
   * active again.
   */
  InvalidateVolumePanels(ctx, p->vol);

  werase(p->pan_dir_window);
  werase(ctx->ctx_file_window);

  /* Capture flags here to preserve state across destructive rescan */
  BOOL saved_big_window = entry->big_window;
  BOOL saved_log_flag = entry->log_flag;
  BOOL saved_global_flag = entry->global_flag;
  BOOL saved_global_all_volumes = entry->global_all_volumes;
  BOOL saved_tagged_flag = entry->tagged_flag;

  if (ctx->view_mode != ARCHIVE_MODE) {
    PathList *expanded = NULL;
    PathList *collapsed = NULL;
    PathList *tagged = NULL;
    PathList *collapsed_curr;
    char saved_path[PATH_LENGTH + 1];
    int win_height;
    int dummy_width;

    GetMaxYX(p->pan_dir_window, &win_height, &dummy_width);
    if (win_height < 1)
      win_height = 1;

    /* 1. Save State */
    GetPath(entry, saved_path);
    SaveTreeState(s->tree, &expanded, &tagged);
    CaptureCollapsedTreeState(s->tree, &collapsed);

    /* 2. Destructive Rescan */
    RescanDir(ctx, entry, strtol(TREEDEPTH, NULL, 0), s, Dir_Progress, ctx);

    /* 2a. Restore critical flags destroyed by ReadTree */
    entry->big_window = saved_big_window;
    entry->log_flag = saved_log_flag;
    entry->global_flag = saved_global_flag;
    entry->global_all_volumes = saved_global_all_volumes;
    entry->tagged_flag = saved_tagged_flag;

    /* 3. Restore State */
    RestoreTreeState(ctx, s->tree, &expanded, tagged, s);
    for (collapsed_curr = collapsed; collapsed_curr;
         collapsed_curr = collapsed_curr->next) {
      DirEntry *collapsed_dir =
          FindDirByPathInSubTree(s->tree, collapsed_curr->path);
      if (!collapsed_dir)
        continue;
      collapsed_dir->not_scanned = TRUE;
      collapsed_dir->unlogged_flag = TRUE;
    }
    PanelTags_Restore(ctx, p);
    FreePathList(expanded);
    FreePathList(collapsed);
    FreePathList(tagged);

    /* 4. Restore Selection */
    BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);

    if (RestorePanelViewportSnapshot(p->vol, p, &viewport, viewport.top_dir_path)) {
      entry = GetPanelDirEntry(p);
    } else {
      /* Try to find the directory we were on */
      char temp_path[PATH_LENGTH + 1];
      int found_idx = -1;
      int i;
      int max_begin;

      for (i = 0; i < p->vol->total_dirs; i++) {
        GetPath(p->vol->dir_entry_list[i].dir_entry, temp_path);
        if (strcmp(temp_path, saved_path) == 0) {
          found_idx = i;
          break;
        }
      }
      max_begin = MAXIMUM(0, p->vol->total_dirs - win_height);

      if (found_idx != -1) {
        int next_disp_begin = saved_disp_begin;

        if (next_disp_begin < 0)
          next_disp_begin = 0;
        if (next_disp_begin > max_begin)
          next_disp_begin = max_begin;

        if (found_idx < next_disp_begin) {
          next_disp_begin = found_idx;
        } else if (found_idx >= next_disp_begin + win_height) {
          next_disp_begin = found_idx - (win_height - 1);
        }

        if (next_disp_begin < 0)
          next_disp_begin = 0;
        if (next_disp_begin > max_begin)
          next_disp_begin = max_begin;

        p->disp_begin_pos = next_disp_begin;
        p->cursor_pos = found_idx - p->disp_begin_pos;
        if (p->cursor_pos < 0)
          p->cursor_pos = 0;
        if (p->cursor_pos >= win_height)
          p->cursor_pos = win_height - 1;
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
    }
  } else {
    /* Archive Mode - Standard Rescan */
    RescanDir(ctx, entry, strtol(TREEDEPTH, NULL, 0), s, Dir_Progress, ctx);
    /* Restore flags for Archive mode too, as RescanDir/ReadTree clears them */
    entry->big_window = saved_big_window;
    entry->log_flag = saved_log_flag;
    entry->global_flag = saved_global_flag;
    entry->global_all_volumes = saved_global_all_volumes;
    entry->tagged_flag = saved_tagged_flag;
    PanelTags_Restore(ctx, p);

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

  if (dir_entry->not_scanned) {
    char new_log_path[PATH_LENGTH + 1];
    for (de_ptr = dir_entry->sub_tree; de_ptr; de_ptr = de_ptr->next) {
      GetPath(de_ptr, new_log_path);
      if (ReadTree(ctx, de_ptr, new_log_path, 999, s, Dir_Progress, ctx) ==
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

int RefreshDirWindow(ViewContext *ctx, YtreeNovaPanel *p) {
  DirEntry *de_ptr;
  int i, n;
  int result = -1;
  const Statistic *s;
  WINDOW *win;
  PanelViewportSnapshot viewport;

  if (!p || !p->vol)
    return -1;

  s = &p->vol->vol_stats;
  win = p->pan_dir_window;

  CapturePanelViewportSnapshot(p, p->vol, &viewport);
  de_ptr = GetPanelDirEntry(p);
  BuildDirEntryList(ctx, p->vol, &p->current_dir_entry);

  if (RestorePanelViewportSnapshot(p->vol, p, &viewport, viewport.top_dir_path)) {
    de_ptr = GetPanelDirEntry(p);
    if (!de_ptr)
      return -1;
    DisplayTree(ctx, p->vol, win, p->disp_begin_pos,
                p->disp_begin_pos + p->cursor_pos, TRUE);
    DisplayAvailBytes(ctx, s);
    DisplayDiskStatistic(ctx, s);
    UpdateStatsPanel(ctx, de_ptr, s);
    DisplayFileWindow(ctx, p, de_ptr);
    {
      char path[PATH_LENGTH];
      GetPath(de_ptr, path);
      DisplayHeaderPath(ctx, path);
    }
    return 0;
  }

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
    int window_height;

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
