/***************************************************************************
 *
 * src/ui/panel_anchor.c
 * Panel anchor helpers for split-panel directory/file state restoration.
 *
 ***************************************************************************/

#include "ytree_fs.h"
#include "ytree_panel_anchor.h"
#include "ytree_ui.h"
#include <stdio.h>
#include <string.h>

BOOL CapturePanelAnchorPath(const YtreePanel *panel, const struct Volume *vol,
                            char *out_path, size_t out_path_size) {
  int idx;
  DirEntry *entry;

  if (!out_path || out_path_size == 0)
    return FALSE;
  out_path[0] = '\0';

  if (!panel || !vol || panel->vol != vol)
    return FALSE;

  if (panel->saved_focus == FOCUS_FILE && panel->file_selection_dir_path[0]) {
    (void)snprintf(out_path, out_path_size, "%s", panel->file_selection_dir_path);
    return TRUE;
  }

  if (!vol->dir_entry_list || vol->total_dirs <= 0)
    return FALSE;

  idx = panel->disp_begin_pos + panel->cursor_pos;
  if (idx < 0)
    idx = 0;
  if (idx >= vol->total_dirs)
    idx = vol->total_dirs - 1;
  entry = vol->dir_entry_list[idx].dir_entry;
  if (!entry)
    return FALSE;

  GetPath(entry, out_path);
  out_path[out_path_size - 1] = '\0';
  return TRUE;
}

int FindDirIndexByPath(const struct Volume *vol, const char *path) {
  int i;
  char candidate_path[PATH_LENGTH + 1];

  if (!vol || !path || !*path || !vol->dir_entry_list || vol->total_dirs <= 0)
    return -1;

  for (i = 0; i < vol->total_dirs; i++) {
    DirEntry *candidate = vol->dir_entry_list[i].dir_entry;
    if (!candidate)
      continue;
    GetPath(candidate, candidate_path);
    candidate_path[PATH_LENGTH] = '\0';
    if (strcmp(candidate_path, path) == 0)
      return i;
  }

  return -1;
}

int FindDirIndexByPathOrAncestor(const struct Volume *vol, const char *path) {
  char probe[PATH_LENGTH + 1];

  if (!vol || !path || !*path)
    return -1;

  (void)snprintf(probe, sizeof(probe), "%s", path);
  probe[PATH_LENGTH] = '\0';

  while (probe[0] != '\0') {
    int idx = FindDirIndexByPath(vol, probe);
    char *slash;
    size_t len;

    if (idx >= 0)
      return idx;

    len = strlen(probe);
    while (len > 1 && probe[len - 1] == FILE_SEPARATOR_CHAR) {
      probe[len - 1] = '\0';
      len--;
    }
    if (probe[0] == FILE_SEPARATOR_CHAR && probe[1] == '\0')
      break;

    slash = strrchr(probe, FILE_SEPARATOR_CHAR);
    if (!slash)
      break;
    if (slash == probe)
      probe[1] = '\0';
    else
      *slash = '\0';
  }

  return -1;
}

void PositionPanelAtIndex(YtreePanel *panel, int idx) {
  int height;

  if (!panel || !panel->vol || !panel->vol->dir_entry_list ||
      panel->vol->total_dirs <= 0)
    return;

  if (idx < 0)
    idx = 0;
  if (idx >= panel->vol->total_dirs)
    idx = panel->vol->total_dirs - 1;

  height = panel->pan_dir_window ? getmaxy(panel->pan_dir_window) : 1;
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
      if (panel->cursor_pos < 0)
        panel->cursor_pos = 0;
    }
  }
}

void RestorePanelAnchorPath(const struct Volume *vol, YtreePanel *panel,
                            const char *anchor_path) {
  int idx;

  if (!vol || !panel || panel->vol != vol || !anchor_path || !*anchor_path)
    return;

  idx = FindDirIndexByPathOrAncestor(vol, anchor_path);
  if (idx < 0)
    return;

  PositionPanelAtIndex(panel, idx);
  if (panel->saved_focus == FOCUS_FILE)
    panel->file_dir_entry = vol->dir_entry_list[idx].dir_entry;
}

DirEntry *FindDirByPathInTree(DirEntry *entry, const char *path) {
  char candidate_path[PATH_LENGTH + 1];

  for (; entry; entry = entry->next) {
    GetPath(entry, candidate_path);
    candidate_path[PATH_LENGTH] = '\0';
    if (strcmp(candidate_path, path) == 0)
      return entry;
    if (entry->sub_tree) {
      DirEntry *resolved = FindDirByPathInTree(entry->sub_tree, path);
      if (resolved)
        return resolved;
    }
  }

  return NULL;
}

void EnsurePanelAnchorVisible(ViewContext *ctx, const struct Volume *vol,
                              YtreePanel *panel, const char *label) {
  int idx;
  DirEntry *target;
  DirEntry *ancestor;
  BOOL changed = FALSE;

  if (!ctx || !vol || !panel || panel->vol != vol)
    return;
  if (panel->saved_focus != FOCUS_FILE ||
      panel->file_selection_dir_path[0] == '\0')
    return;

  idx = FindDirIndexByPath(vol, panel->file_selection_dir_path);
  if (idx >= 0) {
    target = vol->dir_entry_list[idx].dir_entry;
    PositionPanelAtIndex(panel, idx);
    panel->file_dir_entry = target;
    return;
  }

  target = FindDirByPathInTree(vol->vol_stats.tree, panel->file_selection_dir_path);
  if (!target)
    return;

  for (ancestor = target->up_tree; ancestor; ancestor = ancestor->up_tree) {
    if (ancestor->not_scanned && ancestor->sub_tree) {
      ancestor->not_scanned = FALSE;
      changed = TRUE;
    }
  }

  if (changed) {
    DEBUG_LOG("HandleDirWindow:expand anchor label=%s path='%s'",
              label ? label : "?", panel->file_selection_dir_path);
    BuildDirEntryList(ctx, panel->vol, &panel->current_dir_entry);
  }

  idx = FindDirIndexByPathOrAncestor(vol, panel->file_selection_dir_path);
  if (idx >= 0) {
    PositionPanelAtIndex(panel, idx);
    panel->file_dir_entry = vol->dir_entry_list[idx].dir_entry;
  }
}

void DebugLogDirLoopState(const char *label, const ViewContext *ctx,
                          const DirEntry *dir_entry, int ch, YtreeAction action,
                          int unput_char) {
  char dir_path[PATH_LENGTH + 1];
  const char *active_side = "?";

  dir_path[0] = '\0';
  if (dir_entry) {
    GetPath((DirEntry *)dir_entry, dir_path);
    dir_path[PATH_LENGTH] = '\0';
  }
  if (ctx && ctx->active) {
    if (ctx->active == ctx->left)
      active_side = "LEFT";
    else if (ctx->active == ctx->right)
      active_side = "RIGHT";
  }

  DEBUG_LOG("DirLoop[%s] ch=%d action=%d unput=%d active=%s focus=%d "
            "left(d=%d c=%d sf=%d fc=%d) right(d=%d c=%d sf=%d fc=%d) dir='%s'",
            label ? label : "?", ch, (int)action, unput_char, active_side,
            ctx ? (int)ctx->focused_window : -1,
            (ctx && ctx->left) ? ctx->left->disp_begin_pos : -1,
            (ctx && ctx->left) ? ctx->left->cursor_pos : -1,
            (ctx && ctx->left) ? ctx->left->start_file : -1,
            (ctx && ctx->left) ? ctx->left->file_cursor_pos : -1,
            (ctx && ctx->right) ? ctx->right->disp_begin_pos : -1,
            (ctx && ctx->right) ? ctx->right->cursor_pos : -1,
            (ctx && ctx->right) ? ctx->right->start_file : -1,
            (ctx && ctx->right) ? ctx->right->file_cursor_pos : -1,
            dir_entry ? dir_path : "<null>");
}
