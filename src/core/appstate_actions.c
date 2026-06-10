/***************************************************************************
 *
 * src/core/appstate_actions.c
 * Runtime lookup table for YtreeAction AppState transition metadata.
 *
 ***************************************************************************/

#include "ytree_appstate_actions.h"
#include <string.h>

enum { APPSTATE_ACTION_TRANSITION_COUNT = ACTION_USER_CMD + 1 };

static const char *const kAppStateTransitionWriteSet0[] = {
  "panel.tree_selection_key",
  "panel.tree_cursor_pos",
  "panel.tree_viewport_origin",
  "panel.focus_shape",
  "panel.panel_generation",
};

static const char *const kAppStateTransitionWriteSet1[] = {
  "ctx.active",
  "panel.volume_key",
  "panel.restore_snapshot",
  "panel.panel_generation",
};

static const char *const kAppStateTransitionWriteSet2[] = {
  "ctx.modal_state",
  "ctx.message_state",
  "panel.focus_shape",
  "panel.panel_generation",
};

static const char *const kAppStateTransitionWriteSet3[] = {
  "volume.dir_tree",
  "volume.logged_state",
  "volume.volume_generation",
  "panel.restore_snapshot",
  "panel.panel_generation",
};

static const char *const kAppStateTransitionWriteSet4[] = {
  "ctx.volumes_head",
  "panel.volume_key",
  "panel.restore_snapshot",
  "panel.panel_generation",
  "volume.volume_generation",
};

static const char *const kAppStateTransitionWriteSet5[] = {
  "ctx.layout",
  "ctx.window_handles",
  "panel.tree_viewport_origin",
  "panel.file_viewport_origin",
  "panel.panel_generation",
};

static const char *const kAppStateTransitionWriteSet6[] = {
  "volume.dir_tree",
  "volume.payload_cache",
  "volume.volume_generation",
  "panel.restore_snapshot",
  "panel.panel_generation",
  "ctx.message_state",
};

static const char *const kAppStateTransitionWriteSet7[] = {
  "ctx.command_state",
  "ctx.message_state",
  "ctx.pending_transition",
  "panel.panel_generation",
};

static const char *const kAppStateTransitionWriteSet8[] = {
  "panel.tree_selection_key",
  "panel.file_selection_key",
  "panel.tree_cursor_pos",
  "panel.tree_viewport_origin",
  "panel.file_viewport_origin",
  "panel.panel_generation",
};

static const char *const kAppStateTransitionWriteSet9[] = {
  "ctx.render_dirty_flags",
  "ctx.window_handles",
};

static const char *const kAppStateCompatibilityShimInvariantChecks0[] = {
  "YtreePanel(active).dotfile_visibility is authoritative",
  "Inactive panel visibility is never overwritten from the mirror",
};

static const char *const kAppStateCompatibilityShimInvariantChecks1[] = {
  "Raw index is never used while a stable identity key resolves",
  "Generation mismatch forces rebind before dereference",
};

static const char *const kAppStateCompatibilityShimInvariantChecks2[] = {
  "Panel focus_shape remains the restore authority",
  "Session flag must not overwrite inactive panel shape",
};

static const char *const kAppStateCompatibilityShimInvariantChecks3[] = {
  "Render projection is not restore authority",
  "Temporary row math is discarded after draw",
};

static const AppStateTransitionMetadata kAppStateTransitions[] = {
  {"transition.keybinding.navigate-tree",
   "keybinding",
   "YtreePanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0])},
  {"transition.menu-action.volume-select",
   "menu_action",
   "ViewContext(session routing) and YtreePanel(active)",
   kAppStateTransitionWriteSet1,
   sizeof(kAppStateTransitionWriteSet1) / sizeof(kAppStateTransitionWriteSet1[0])},
  {"transition.modal-action.dismiss",
   "modal_action",
   "ViewContext.modal_region",
   kAppStateTransitionWriteSet2,
   sizeof(kAppStateTransitionWriteSet2) / sizeof(kAppStateTransitionWriteSet2[0])},
  {"transition.refresh-rebuild.manual-refresh",
   "refresh_rebuild",
   "Volume(shared topology)",
   kAppStateTransitionWriteSet3,
   sizeof(kAppStateTransitionWriteSet3) / sizeof(kAppStateTransitionWriteSet3[0])},
  {"transition.volume-operation.release-cycle",
   "volume_operation",
   "ViewContext.volume_registry and YtreePanel(active)",
   kAppStateTransitionWriteSet4,
   sizeof(kAppStateTransitionWriteSet4) / sizeof(kAppStateTransitionWriteSet4[0])},
  {"transition.terminal-signal-resize",
   "terminal_signal_or_resize",
   "ViewContext.layout_region",
   kAppStateTransitionWriteSet5,
   sizeof(kAppStateTransitionWriteSet5) / sizeof(kAppStateTransitionWriteSet5[0])},
  {"transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreePanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0])},
  {"transition.command-completion.user-command",
   "command_completion",
   "ViewContext.command_region",
   kAppStateTransitionWriteSet7,
   sizeof(kAppStateTransitionWriteSet7) / sizeof(kAppStateTransitionWriteSet7[0])},
  {"transition.rebuild-rebind-callback.panel-anchor",
   "rebuild_rebind_callback",
   "YtreePanel(affected) and Volume(current)",
   kAppStateTransitionWriteSet8,
   sizeof(kAppStateTransitionWriteSet8) / sizeof(kAppStateTransitionWriteSet8[0])},
  {"transition.render-reflow.project-state",
   "render_reflow",
   "ViewContext.render_region",
   kAppStateTransitionWriteSet9,
   sizeof(kAppStateTransitionWriteSet9) / sizeof(kAppStateTransitionWriteSet9[0])},
};

static const AppStateActionTransitionMetadata
    kAppStateActionTransitions[APPSTATE_ACTION_TRANSITION_COUNT] = {
  {ACTION_NONE, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_MOVE_UP, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_MOVE_DOWN, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_MOVE_SIBLING_NEXT, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_MOVE_SIBLING_PREV, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_MOVE_LEFT, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_MOVE_RIGHT, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_PAGE_UP, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_PAGE_DOWN, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_HOME, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_END, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_TREE_EXPAND, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_TREE_COLLAPSE, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_TREE_EXPAND_ALL, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_ENTER, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_ESCAPE, "transition.modal-action.dismiss", "modal_action"},
  {ACTION_LOG, "transition.refresh-rebuild.manual-refresh", "refresh_rebuild"},
  {ACTION_QUIT, "transition.command-completion.user-command", "command_completion"},
  {ACTION_QUIT_DIR, "transition.command-completion.user-command", "command_completion"},
  {ACTION_TAG, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_UNTAG, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_TAG_ALL, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_UNTAG_ALL, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_TAG_REST, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_UNTAG_REST, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_FILTER, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_TOGGLE_MODE, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_REFRESH, "transition.refresh-rebuild.manual-refresh", "refresh_rebuild"},
  {ACTION_RESIZE, "transition.terminal-signal-resize", "terminal_signal_or_resize"},
  {ACTION_VOL_MENU, "transition.menu-action.volume-select", "menu_action"},
  {ACTION_VOL_PREV, "transition.volume-operation.release-cycle", "volume_operation"},
  {ACTION_VOL_NEXT, "transition.volume-operation.release-cycle", "volume_operation"},
  {ACTION_CMD_A,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_B,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_C,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_D,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_E,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_G,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_H,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_I,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_M,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_O,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_P,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_R,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_S,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_V,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_X,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_Y,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_PRINT,
   "transition.command-completion.user-command",
   "command_completion"},
  {ACTION_TOGGLE_HIDDEN, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_TOGGLE_COMPACT, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_CMD_MKFILE,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_TAGGED_A,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_TAGGED_C,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_TAGGED_D,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_TAGGED_G,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_TAGGED_M,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_TAGGED_O,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_TAGGED_P,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_TAGGED_R,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_TAGGED_S,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_TAGGED_V,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_TAGGED_X,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_TAGGED_Y,
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result"},
  {ACTION_CMD_TAGGED_PRINT,
   "transition.command-completion.user-command",
   "command_completion"},
  {ACTION_LIST_JUMP, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_TO_DIR, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_TOGGLE_TAGGED_MODE, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_TOGGLE_STATS, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_ASTERISK, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_INVERT, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_SPLIT_SCREEN, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_SWITCH_PANEL, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_VIEW_PREVIEW, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_PREVIEW_SCROLL_UP, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_PREVIEW_SCROLL_DOWN, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_PREVIEW_HOME, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_PREVIEW_END, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_PREVIEW_PAGE_UP, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_PREVIEW_PAGE_DOWN, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_COMPARE_FILE, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_COMPARE_DIR, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_COMPARE_TREE, "transition.keybinding.navigate-tree", "keybinding"},
  {ACTION_EDIT_CONFIG,
   "transition.command-completion.user-command",
   "command_completion"},
  {ACTION_USER_CMD, "transition.command-completion.user-command", "command_completion"},
};

static const AppStateCompatibilityShimMetadata kAppStateCompatibilityShims[] = {
  {"shim.viewcontext-hide-dot-files",
   "ViewContext derived mirror",
   "ViewContext.hide_dot_files",
   "Allowed only as a derived compatibility mirror for helpers that have not yet accepted YtreePanel dotfile visibility.",
   "Write only when synchronizing from the active panel's authoritative dotfile_visibility during transition commit.",
   kAppStateCompatibilityShimInvariantChecks0,
   sizeof(kAppStateCompatibilityShimInvariantChecks0) /
       sizeof(kAppStateCompatibilityShimInvariantChecks0[0]),
   "All visibility and restore helpers consume panel-local dotfile_visibility directly.",
   "transition.keybinding.navigate-tree",
   "Runtime migration of visibility toggles and restore helpers to AppState panel records.",
   "check_appstate_contract.py requires this shim to declare permissions, invariants, target transition, and removal trigger."},
  {"shim.volume-saved-tree-index",
   "Volume restore breadcrumb",
   "Volume.saved_tree_index",
   "Allowed only as a fallback breadcrumb when a stable path key has already failed to resolve.",
   "Do not write as primary restore authority; future writes must update stable identity keys and generation metadata first.",
   kAppStateCompatibilityShimInvariantChecks1,
   sizeof(kAppStateCompatibilityShimInvariantChecks1) /
       sizeof(kAppStateCompatibilityShimInvariantChecks1[0]),
   "Volume restore breadcrumbs are path-keyed and generation-checked across rebuild/relog paths.",
   "transition.rebuild-rebind-callback.panel-anchor",
   "Replace index breadcrumbs with path-scoped restore snapshots in the canonical panel state record.",
   "check_appstate_contract.py keeps this debt visible until runtime migration removes the shim."},
  {"shim.focused-window-session-flag",
   "ViewContext session routing",
   "ViewContext.focused_window",
   "Allowed for layout routing and footer context selection while AppState focus_shape migration is incomplete.",
   "Write only from transition commit after the active panel focus_shape has been updated.",
   kAppStateCompatibilityShimInvariantChecks2,
   sizeof(kAppStateCompatibilityShimInvariantChecks2) /
       sizeof(kAppStateCompatibilityShimInvariantChecks2[0]),
   "All Enter, Tab, and F8 paths route through the canonical AppState transition entry point.",
   "transition.keybinding.navigate-tree",
   "Move focus-shape authority from session mirrors into panel-local transition records.",
   "check_appstate_contract.py validates shim coverage and links it to an existing transition id."},
  {"shim-render-derived-row-position",
   "Render projection temporary",
   "disp_begin_pos + cursor_pos render-derived lookup",
   "Allowed only inside render projection or bounds-correction code after identity restore has run.",
   "Never write authoritative selection from this calculation.",
   kAppStateCompatibilityShimInvariantChecks3,
   sizeof(kAppStateCompatibilityShimInvariantChecks3) /
       sizeof(kAppStateCompatibilityShimInvariantChecks3[0]),
   "Render paths accept explicit projection inputs and no longer inspect restore authority fields directly.",
   "transition.render-reflow.project-state",
   "Audit render/reflow call sites for projection-only behavior during runtime migration.",
   "check_appstate_contract.py requires render shims to declare no-write authority and target transition linkage."},
};

size_t AppStateActionTransitionCount(void) {
  return sizeof(kAppStateActionTransitions) / sizeof(kAppStateActionTransitions[0]);
}

size_t AppStateTransitionCount(void) {
  return sizeof(kAppStateTransitions) / sizeof(kAppStateTransitions[0]);
}

size_t AppStateCompatibilityShimCount(void) {
  return sizeof(kAppStateCompatibilityShims) /
         sizeof(kAppStateCompatibilityShims[0]);
}

const AppStateTransitionMetadata *AppStateTransitionAt(size_t index) {
  if (index >= AppStateTransitionCount())
    return NULL;

  return &kAppStateTransitions[index];
}

const AppStateCompatibilityShimMetadata *
AppStateCompatibilityShimAt(size_t index) {
  if (index >= AppStateCompatibilityShimCount())
    return NULL;

  return &kAppStateCompatibilityShims[index];
}

const AppStateTransitionMetadata *
AppStateTransitionLookup(const char *transition_id) {
  size_t index;

  if (transition_id == NULL || transition_id[0] == '\0')
    return NULL;

  for (index = 0; index < AppStateTransitionCount(); index++) {
    if (!strcmp(kAppStateTransitions[index].id, transition_id))
      return &kAppStateTransitions[index];
  }

  return NULL;
}

const AppStateCompatibilityShimMetadata *
AppStateCompatibilityShimLookup(const char *shim_id) {
  size_t index;

  if (shim_id == NULL || shim_id[0] == '\0')
    return NULL;

  for (index = 0; index < AppStateCompatibilityShimCount(); index++) {
    if (!strcmp(kAppStateCompatibilityShims[index].id, shim_id))
      return &kAppStateCompatibilityShims[index];
  }

  return NULL;
}

const AppStateActionTransitionMetadata *
AppStateActionTransitionLookup(YtreeAction action) {
  const AppStateActionTransitionMetadata *metadata;

  if ((int)action < 0 || (size_t)action >= AppStateActionTransitionCount())
    return NULL;

  metadata = &kAppStateActionTransitions[(size_t)action];
  if (metadata->action != action)
    return NULL;

  return metadata;
}
