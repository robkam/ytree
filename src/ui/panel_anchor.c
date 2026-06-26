/***************************************************************************
 *
 * src/ui/panel_anchor.c
 * Panel anchor helpers for split-panel directory/file state restoration.
 *
 ***************************************************************************/

#include "ytnova_appstate_actions.h"
#include "ytnova_appstate_focus.h"
#include "ytnova_fs.h"
#include "ytnova_panel_anchor.h"
#include "ytnova_ui.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static int PanelViewportSlot(const YtreeNovaPanel *panel) {
  return (panel && panel->hide_dot_files) ? 1 : 0;
}

static int FindTopVisibleDirIndex(const YtreeNovaPanel *panel) {
  int idx;

  if (!panel || !panel->vol || !panel->vol->dir_entry_list ||
      panel->vol->total_dirs <= 0)
    return -1;

  idx = panel->disp_begin_pos;
  if (idx < 0)
    idx = 0;
  if (idx >= panel->vol->total_dirs)
    idx = panel->vol->total_dirs - 1;

  idx = PanelFindNextVisibleDirIndex(panel, idx, 1);
  if (idx < 0)
    idx = PanelFindFirstVisibleDirIndex(panel);

  return idx;
}

void RememberPanelViewportTop(YtreeNovaPanel *panel) {
  int idx;
  int slot;

  if (!panel)
    return;

  slot = PanelViewportSlot(panel);
  panel->tree_viewport_top_dir_path[slot][0] = '\0';

  idx = FindTopVisibleDirIndex(panel);
  if (idx < 0)
    return;

  GetPath(panel->vol->dir_entry_list[idx].dir_entry,
          panel->tree_viewport_top_dir_path[slot]);
  panel->tree_viewport_top_dir_path[slot][PATH_LENGTH] = '\0';
}

BOOL CapturePanelAnchorPath(const YtreeNovaPanel *panel, const struct Volume *vol,
                            char *out_path, size_t out_path_size) {
  int idx;
  DirEntry *entry;

  if (!out_path || out_path_size == 0)
    return FALSE;
  out_path[0] = '\0';

  if (!panel || !vol)
    return FALSE;
  assert(!panel->vol || panel->vol == vol);
  if (panel->vol != vol)
    return FALSE;
  assert(panel->saved_focus != FOCUS_FILE ||
         panel->file_selection_dir_path[0] != '\0');

  if (panel->saved_focus == FOCUS_FILE) {
    if (panel->file_selection_dir_path[0]) {
      (void)snprintf(out_path, out_path_size, "%s",
                     panel->file_selection_dir_path);
      return TRUE;
    }
  }

  if (!vol->dir_entry_list || vol->total_dirs <= 0)
    return FALSE;

  idx = GetPanelVisibleSelectionIndex(panel);
  if (idx < 0)
    return FALSE;
  entry = vol->dir_entry_list[idx].dir_entry;
  if (!entry)
    return FALSE;

  GetPath(entry, out_path);
  out_path[out_path_size - 1] = '\0';
  return TRUE;
}

void CapturePanelViewportSnapshot(YtreeNovaPanel *panel, const struct Volume *vol,
                                  PanelViewportSnapshot *snapshot) {
  int idx;

  if (!snapshot)
    return;

  snapshot->selected_dir_path[0] = '\0';
  snapshot->top_dir_path[0] = '\0';
  snapshot->has_selected_dir_path = FALSE;
  snapshot->has_top_dir_path = FALSE;

  if (!panel || !vol || panel->vol != vol)
    return;

  RememberPanelViewportTop(panel);

  if (CapturePanelAnchorPath(panel, vol, snapshot->selected_dir_path,
                             sizeof(snapshot->selected_dir_path))) {
    snapshot->has_selected_dir_path = TRUE;
  }

  idx = FindTopVisibleDirIndex(panel);
  if (idx >= 0) {
    GetPath(panel->vol->dir_entry_list[idx].dir_entry, snapshot->top_dir_path);
    snapshot->top_dir_path[PATH_LENGTH] = '\0';
    snapshot->has_top_dir_path = TRUE;
  }
}

PanelVolumeFileState *FindPanelVolumeFileState(YtreeNovaPanel *panel,
                                               int volume_id) {
  PanelVolumeFileState *state;

  if (!panel)
    return NULL;

  for (state = panel->volume_file_state; state; state = state->next) {
    if (state->volume_id == volume_id)
      return state;
  }
  return NULL;
}

PanelVolumeFileState *GetPanelVolumeFileState(YtreeNovaPanel *panel,
                                              int volume_id) {
  PanelVolumeFileState *state;

  state = FindPanelVolumeFileState(panel, volume_id);
  if (state)
    return state;

  state = (PanelVolumeFileState *)xcalloc(1, sizeof(PanelVolumeFileState));
  state->volume_id = volume_id;
  state->next = panel->volume_file_state;
  panel->volume_file_state = state;
  return state;
}

void SavePanelTreeViewportSnapshot(YtreeNovaPanel *panel) {
  PanelVolumeFileState *state;
  PanelViewportSnapshot snapshot;
  if (!panel || !panel->vol)
    return;
  state = GetPanelVolumeFileState(panel, panel->vol->id);
  CapturePanelViewportSnapshot(panel, panel->vol, &snapshot);
  state->saved_tree_panel_generation = panel->panel_generation;
  state->saved_tree_volume_generation = panel->vol->volume_generation;
  state->has_saved_tree_selection = snapshot.has_selected_dir_path;
  state->has_saved_tree_top = snapshot.has_top_dir_path;
  state->saved_tree_selected_dir_path[0] = '\0';
  state->saved_tree_top_dir_path[0] = '\0';
  if (snapshot.has_selected_dir_path) {
    (void)snprintf(state->saved_tree_selected_dir_path,
                   sizeof(state->saved_tree_selected_dir_path), "%s",
                   snapshot.selected_dir_path);
    state->saved_tree_selected_dir_path[PATH_LENGTH] = '\0';
  }
  if (snapshot.has_top_dir_path) {
    (void)snprintf(state->saved_tree_top_dir_path,
                   sizeof(state->saved_tree_top_dir_path), "%s",
                   snapshot.top_dir_path);
    state->saved_tree_top_dir_path[PATH_LENGTH] = '\0';
  }

}

void ResetPanelTreeViewportSnapshot(YtreeNovaPanel *panel) {
  PanelVolumeFileState *state;

  if (!panel || !panel->vol)
    return;

  state = GetPanelVolumeFileState(panel, panel->vol->id);
  state->saved_tree_panel_generation = panel->panel_generation;
  state->saved_tree_volume_generation = panel->vol->volume_generation;
  state->has_saved_tree_selection = FALSE;
  state->has_saved_tree_top = FALSE;
  state->saved_tree_selected_dir_path[0] = '\0';
  state->saved_tree_top_dir_path[0] = '\0';
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

DirEntry *FindDirByPathOrAncestor(const struct Volume *vol, const char *path) {
  int idx;

  idx = FindDirIndexByPathOrAncestor(vol, path);
  if (idx < 0 || !vol || !vol->dir_entry_list || idx >= vol->total_dirs)
    return NULL;

  return vol->dir_entry_list[idx].dir_entry;
}

static BOOL PanelAnchorTargetIsVisible(const YtreeNovaPanel *panel,
                                       const struct Volume *vol,
                                       const DirEntry *entry) {
  char candidate_path[PATH_LENGTH + 1];

  if (!panel || !vol || !entry)
    return FALSE;

  GetPath((DirEntry *)entry, candidate_path);
  candidate_path[PATH_LENGTH] = '\0';
  if (FindDirIndexByPath(vol, candidate_path) < 0)
    return FALSE;

  return PanelDirIsVisible(panel, entry);
}

static DirEntry *PanelAnchorFindVisibleAncestor(const YtreeNovaPanel *panel,
                                                const struct Volume *vol,
                                                DirEntry *entry) {
  while (entry) {
    if (entry->up_tree && PanelAnchorTargetIsVisible(panel, vol, entry))
      return entry;
    entry = entry->up_tree;
  }

  return NULL;
}

static DirEntry *PanelAnchorFindVisibleSibling(const YtreeNovaPanel *panel,
                                               const struct Volume *vol,
                                               DirEntry *entry) {
  DirEntry *sibling;

  if (!entry)
    return NULL;

  for (sibling = entry->next; sibling; sibling = sibling->next) {
    if (PanelAnchorTargetIsVisible(panel, vol, sibling))
      return sibling;
  }
  for (sibling = entry->prev; sibling; sibling = sibling->prev) {
    if (PanelAnchorTargetIsVisible(panel, vol, sibling))
      return sibling;
  }

  return NULL;
}

/*
 * Canonical restore authority stays path-based; the helper only rebinds to
 * the current visible list using the fixed fallback order from the spec.
 */
DirEntry *ResolvePanelAnchorTarget(const YtreeNovaPanel *panel,
                                   const struct Volume *vol,
                                   const char *anchor_path) {
  DirEntry *exact;
  DirEntry *ancestor;
  DirEntry *sibling_base;

  if (!panel || !vol || !anchor_path || !*anchor_path ||
      !vol->vol_stats.tree)
    return NULL;

  exact = FindDirByPathInTree(vol->vol_stats.tree, anchor_path);
  if (PanelAnchorTargetIsVisible(panel, vol, exact))
    return exact;

  ancestor = exact ? exact->up_tree : FindDirByPathOrAncestor(vol, anchor_path);
  ancestor = PanelAnchorFindVisibleAncestor(panel, vol, ancestor);
  if (ancestor)
    return ancestor;

  sibling_base = exact ? exact : FindDirByPathOrAncestor(vol, anchor_path);
  for (; sibling_base && sibling_base->up_tree; sibling_base = sibling_base->up_tree) {
    DirEntry *sibling = PanelAnchorFindVisibleSibling(panel, vol, sibling_base);

    if (sibling)
      return sibling;
  }

  return vol->vol_stats.tree;
}

void PositionPanelAtIndex(YtreeNovaPanel *panel, int idx) {
  int height;
  int begin;
  int cursor;

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

  begin = panel->disp_begin_pos;
  cursor = panel->cursor_pos;
  if (!PanelComputeViewportPosition(panel, idx, height, &begin, &cursor)) {
    begin = 0;
    cursor = 0;
  }

  panel->disp_begin_pos = begin;
  panel->cursor_pos = cursor;
  RememberPanelViewportTop(panel);
  panel->panel_generation++;
}

static BOOL VisibleIndexWithinTopPath(const struct Volume *vol,
                                      const YtreeNovaPanel *panel,
                                      const char *top_path, int selected_idx,
                                      int height, int *top_idx_out) {
  int top_idx;
  int idx;
  int visible_rows;

  if (top_idx_out)
    *top_idx_out = -1;

  if (!vol || !panel || !top_path || !*top_path || selected_idx < 0 ||
      height < 1)
    return FALSE;

  top_idx = FindDirIndexByPath(vol, top_path);
  if (top_idx < 0)
    return FALSE;
  if (!PanelDirIsVisible(panel, vol->dir_entry_list[top_idx].dir_entry))
    return FALSE;

  visible_rows = 0;
  for (idx = top_idx; idx < vol->total_dirs; idx++) {
    const DirEntry *candidate = vol->dir_entry_list[idx].dir_entry;

    if (!PanelDirIsVisible(panel, candidate))
      continue;
    if (idx == selected_idx) {
      if (top_idx_out)
        *top_idx_out = top_idx;
      return visible_rows < height;
    }
    visible_rows++;
    if (visible_rows >= height)
      break;
  }

  return FALSE;
}

BOOL RestorePanelViewportSnapshot(const struct Volume *vol, YtreeNovaPanel *panel,
                                  const PanelViewportSnapshot *snapshot,
                                  const char *preferred_top_path) {
  DirEntry *target;
  char target_path[PATH_LENGTH + 1];
  const char *top_path = NULL;
  int target_idx;
  int top_idx = -1;
  int height;
  int begin;
  int cursor;

  if (!vol || !panel || !snapshot)
    return FALSE;
  assert(!panel->vol || panel->vol == vol);
  if (panel->vol && panel->vol != vol)
    return FALSE;
  if (!snapshot->has_selected_dir_path || !snapshot->selected_dir_path[0])
    return FALSE;

  target = ResolvePanelAnchorTarget(panel, vol, snapshot->selected_dir_path);
  if (!target)
    return FALSE;

  GetPath(target, target_path);
  target_path[PATH_LENGTH] = '\0';
  target_idx = FindDirIndexByPath(vol, target_path);
  if (target_idx < 0)
    return FALSE;

  height = panel->pan_dir_window ? getmaxy(panel->pan_dir_window) : 1;
  if (height < 1)
    height = 1;

  if (preferred_top_path && preferred_top_path[0])
    top_path = preferred_top_path;
  else if (snapshot->has_top_dir_path && snapshot->top_dir_path[0])
    top_path = snapshot->top_dir_path;

  if (top_path &&
      VisibleIndexWithinTopPath(vol, panel, top_path, target_idx, height,
                                &top_idx)) {
    panel->disp_begin_pos = top_idx;
    panel->cursor_pos = target_idx - top_idx;
  } else {
    begin = panel->disp_begin_pos;
    cursor = panel->cursor_pos;
    if (!PanelComputeViewportPosition(panel, target_idx, height, &begin,
                                      &cursor)) {
      begin = target_idx;
      cursor = 0;
    }
    panel->disp_begin_pos = begin;
    panel->cursor_pos = cursor;
  }

  RememberPanelViewportTop(panel);
  panel->panel_generation++;
  return TRUE;
}

BOOL RestorePanelTreeViewportSnapshot(ViewContext *ctx, YtreeNovaPanel *panel) {
  const PanelVolumeFileState *state;
  PanelViewportSnapshot snapshot;
  int selected_index;
  int total_dirs;
  int win_height;
  BOOL generation_valid;

  if (!ctx || !panel || !panel->vol)
    return FALSE;

  total_dirs = panel->vol->total_dirs;
  if (total_dirs <= 0) {
    panel->disp_begin_pos = 0;
    panel->cursor_pos = 0;
    return FALSE;
  }

  state = FindPanelVolumeFileState(panel, panel->vol->id);
  generation_valid =
      state != NULL &&
      state->saved_tree_panel_generation == panel->panel_generation &&
      state->saved_tree_volume_generation == panel->vol->volume_generation;
  if (generation_valid && state->has_saved_tree_selection &&
      state->saved_tree_selected_dir_path[0]) {
    snapshot.has_selected_dir_path = state->has_saved_tree_selection;
    snapshot.has_top_dir_path = state->has_saved_tree_top;
    (void)snprintf(snapshot.selected_dir_path,
                   sizeof(snapshot.selected_dir_path), "%s",
                   state->saved_tree_selected_dir_path);
    (void)snprintf(snapshot.top_dir_path, sizeof(snapshot.top_dir_path), "%s",
                   state->saved_tree_top_dir_path);
    snapshot.selected_dir_path[PATH_LENGTH] = '\0';
    snapshot.top_dir_path[PATH_LENGTH] = '\0';
    if (RestorePanelViewportSnapshot(panel->vol, panel, &snapshot,
                                     state->saved_tree_top_dir_path))
      return TRUE;
  }
  selected_index = 0;

  win_height = ctx->layout.dir_win_height;
  if (win_height <= 0 && ctx->ctx_dir_window)
    win_height = getmaxy(ctx->ctx_dir_window);
  if (win_height <= 0)
    win_height = 1;

  if (panel->disp_begin_pos < 0)
    panel->disp_begin_pos = 0;
  if (selected_index >= panel->disp_begin_pos &&
      selected_index < panel->disp_begin_pos + win_height) {
    panel->cursor_pos = selected_index - panel->disp_begin_pos;
    return FALSE;
  }

  if (selected_index >= win_height) {
    panel->disp_begin_pos = selected_index - (win_height - 1);
    panel->cursor_pos = win_height - 1;
  } else {
    panel->disp_begin_pos = 0;
    panel->cursor_pos = selected_index;
  }
  return FALSE;
}

void RestorePanelAnchorPath(const struct Volume *vol, YtreeNovaPanel *panel,
                            const char *anchor_path) {
  PanelViewportSnapshot snapshot;
  DirEntry *target;
  char target_path[PATH_LENGTH + 1];
  int idx;

  if (!AppStateValidatedDispatchSurface("surface.panel-anchor-rebind"))
    return;
  if (!AppStateValidatedEvent("event.rebuild-rebind-callback"))
    return;
  if (!vol || !panel || !anchor_path || !*anchor_path)
    return;
  assert(!panel->vol || panel->vol == vol);
  if (panel->vol && panel->vol != vol)
    return;

  CapturePanelViewportSnapshot(panel, vol, &snapshot);
  (void)snprintf(snapshot.selected_dir_path, sizeof(snapshot.selected_dir_path),
                 "%s", anchor_path);
  snapshot.selected_dir_path[PATH_LENGTH] = '\0';
  snapshot.has_selected_dir_path = TRUE;
  if (RestorePanelViewportSnapshot(vol, panel, &snapshot, snapshot.top_dir_path)) {
    if (panel->saved_focus == FOCUS_FILE) {
      target = ResolvePanelAnchorTarget(panel, vol, anchor_path);
      if (target)
        panel->file_dir_entry = target;
    }
    return;
  }

  target = ResolvePanelAnchorTarget(panel, vol, anchor_path);
  if (!target)
    return;

  GetPath(target, target_path);
  target_path[PATH_LENGTH] = '\0';
  idx = FindDirIndexByPath(vol, target_path);
  if (idx < 0)
    return;

  PositionPanelAtIndex(panel, idx);
  if (panel->saved_focus == FOCUS_FILE)
    panel->file_dir_entry = target;
}

static void FreePanelVolumeFileState(PanelVolumeFileState *state) {
  PanelVolumeFileState *next;

  while (state) {
    next = state->next;
    free(state);
    state = next;
  }
}

static PanelVolumeFileState *
CopyPanelVolumeFileState(const PanelVolumeFileState *src) {
  PanelVolumeFileState *head = NULL;
  PanelVolumeFileState **tail = &head;

  for (; src; src = src->next) {
    PanelVolumeFileState *node;

    node = (PanelVolumeFileState *)xcalloc(1, sizeof(PanelVolumeFileState));
    *node = *src;
    node->next = NULL;
    *tail = node;
    tail = &node->next;
  }

  return head;
}

BOOL DonatePanelState(ViewContext *ctx, YtreeNovaPanel *dst,
                      const YtreeNovaPanel *src) {
  char file_dir_path[PATH_LENGTH + 1];
  BOOL dst_saved_big_file_view;
  int dst_cursor_pos;
  int dst_disp_begin_pos;
  int dst_start_file;
  int dst_file_cursor_pos;
  const DirEntry *dst_file_dir_entry;
  int dst_current_dir_entry;
  unsigned int dst_panel_generation;
  char dst_file_selection_name[PATH_LENGTH + 1];
  char dst_file_selection_dir_path[PATH_LENGTH + 1];
  const PanelVolumeFileState *dst_volume_state;
  const PanelVolumeFileState *dst_current_volume_state;
  PanelVolumeFileState *volume_file_state;
  BOOL source_is_file;

  if (!ctx || !dst || !src || dst == src)
    return FALSE;
  assert(!dst->vol || !src->vol || dst->vol == src->vol);
  assert(src->saved_focus != FOCUS_FILE ||
         src->file_selection_dir_path[0] != '\0');

  source_is_file = (src->saved_focus == FOCUS_FILE);
  dst_saved_big_file_view = dst->saved_big_file_view;
  dst_cursor_pos = dst->cursor_pos;
  dst_disp_begin_pos = dst->disp_begin_pos;
  dst_start_file = dst->start_file;
  dst_file_cursor_pos = dst->file_cursor_pos;
  dst_file_dir_entry = dst->file_dir_entry;
  dst_current_dir_entry = dst->current_dir_entry;
  dst_panel_generation = dst->panel_generation;
  (void)snprintf(dst_file_selection_name, sizeof(dst_file_selection_name), "%s",
                 dst->file_selection_name);
  (void)snprintf(dst_file_selection_dir_path,
                 sizeof(dst_file_selection_dir_path), "%s",
                 dst->file_selection_dir_path);
  dst_volume_state = dst->volume_file_state;
  dst_current_volume_state = NULL;
  if (dst->vol) {
    for (; dst_volume_state; dst_volume_state = dst_volume_state->next) {
      if (dst_volume_state->volume_id == dst->vol->id) {
        dst_current_volume_state = dst_volume_state;
        break;
      }
    }
  }
  volume_file_state =
      source_is_file ? CopyPanelVolumeFileState(src->volume_file_state) : NULL;

  file_dir_path[0] = '\0';
  if (dst_file_selection_dir_path[0] != '\0') {
    (void)snprintf(file_dir_path, sizeof(file_dir_path), "%s",
                   dst_file_selection_dir_path);
  } else if (source_is_file && src->file_selection_dir_path[0] != '\0') {
    (void)snprintf(file_dir_path, sizeof(file_dir_path), "%s",
                   src->file_selection_dir_path);
  }

  FreeFileEntryList(dst);
  dst->vol = src->vol;
  dst->cursor_pos = src->cursor_pos;
  dst->disp_begin_pos = src->disp_begin_pos;
  memcpy(dst->tree_viewport_top_dir_path, src->tree_viewport_top_dir_path,
         sizeof(dst->tree_viewport_top_dir_path));
  dst->start_file = src->start_file;
  dst->file_cursor_pos = src->file_cursor_pos;
  dst->file_dir_entry = src->file_dir_entry;
  dst->file_mode = src->file_mode;
  dst->max_column = src->max_column;
  dst->current_dir_entry = src->current_dir_entry;
  dst->panel_generation = src->panel_generation;
  if (!AppStateCommitPanelFocus(ctx, dst, src->saved_focus))
    return FALSE;
  if (!AppStateCommitPanelFileShape(dst, src->saved_big_file_view))
    return FALSE;
  dst->max_visual_filename_len = src->max_visual_filename_len;
  dst->max_visual_linkname_len = src->max_visual_linkname_len;
  dst->max_visual_userview_len = src->max_visual_userview_len;
  dst->reverse_sort = src->reverse_sort;
  dst->hide_dot_files = src->hide_dot_files;
  (void)snprintf(dst->file_selection_name, sizeof(dst->file_selection_name),
                 "%s", src->file_selection_name);
  (void)snprintf(dst->file_selection_dir_path,
                 sizeof(dst->file_selection_dir_path), "%s",
                 src->file_selection_dir_path);
  dst->file_dir_entry = NULL;
  PanelTags_Copy(dst, src);
  if (!source_is_file) {
    dst->cursor_pos = dst_cursor_pos;
    dst->disp_begin_pos = dst_disp_begin_pos;
    dst->current_dir_entry = dst_current_dir_entry;
    dst->panel_generation = dst_panel_generation;
    if (dst_current_volume_state &&
        dst_current_volume_state->saved_file_selection_dir_path[0] != '\0') {
      if (!AppStateCommitPanelFileShape(
              dst, dst_current_volume_state->saved_big_file_view))
        return FALSE;
      dst->start_file = dst_current_volume_state->saved_file_start;
      dst->file_cursor_pos = dst_current_volume_state->saved_file_cursor;
      (void)snprintf(dst->file_selection_name,
                     sizeof(dst->file_selection_name), "%s",
                     dst_current_volume_state->saved_file_selection_name);
      (void)snprintf(dst->file_selection_dir_path,
                     sizeof(dst->file_selection_dir_path), "%s",
                     dst_current_volume_state->saved_file_selection_dir_path);
    } else {
      if (!AppStateCommitPanelFocus(ctx, dst, FOCUS_FILE))
        return FALSE;
      if (!AppStateCommitPanelFileShape(dst, dst_saved_big_file_view))
        return FALSE;
      dst->start_file = dst_start_file;
      dst->file_cursor_pos = dst_file_cursor_pos;
      dst->file_dir_entry = (DirEntry *)dst_file_dir_entry;
      (void)snprintf(dst->file_selection_name,
                     sizeof(dst->file_selection_name), "%s",
                     dst_file_selection_name);
      (void)snprintf(dst->file_selection_dir_path,
                     sizeof(dst->file_selection_dir_path), "%s",
                     dst_file_selection_dir_path);
    }
    if (!AppStateCommitPanelFocus(ctx, dst, FOCUS_TREE))
      return FALSE;
  } else {
    FreePanelVolumeFileState(dst->volume_file_state);
    dst->volume_file_state = volume_file_state;
  }
  RestorePanelAnchorPath(dst->vol, dst, file_dir_path);
  return TRUE;
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
                              YtreeNovaPanel *panel, const char *label) {
  DirEntry *target;
  DirEntry *ancestor;
  BOOL changed = FALSE;

  if (!AppStateValidatedDispatchSurface("surface.panel-anchor-rebind"))
    return;
  if (!AppStateValidatedEvent("event.rebuild-rebind-callback"))
    return;
  if (!ctx || !vol || !panel)
    return;
  assert(!panel->vol || panel->vol == vol);
  if (panel->vol && panel->vol != vol)
    return;
  if (panel->saved_focus != FOCUS_FILE ||
      panel->file_selection_dir_path[0] == '\0')
    return;

  target = FindDirByPathInTree(vol->vol_stats.tree, panel->file_selection_dir_path);
  if (!target)
    target = FindDirByPathOrAncestor(vol, panel->file_selection_dir_path);
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

  target = ResolvePanelAnchorTarget(panel, vol, panel->file_selection_dir_path);
  if (target) {
    char target_path[PATH_LENGTH + 1];
    int idx;

    GetPath(target, target_path);
    target_path[PATH_LENGTH] = '\0';
    idx = FindDirIndexByPath(vol, target_path);
    if (idx >= 0) {
      PositionPanelAtIndex(panel, idx);
      panel->file_dir_entry = target;
    }
  }
}

void DebugLogDirLoopState(const char *label, const ViewContext *ctx,
                          const DirEntry *dir_entry, int ch, YtreeNovaAction action,
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
