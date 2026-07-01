/***************************************************************************
 *
 * src/core/appstate_actions.c
 * Runtime lookup table for YtreeNovaAction AppState transition metadata.
 *
 ***************************************************************************/

#include "ytnova_appstate_actions.h"
#include <string.h>

enum {
  APPSTATE_ACTION_TRANSITION_COUNT = ACTION_USER_CMD + 1,
  APPSTATE_ACTION_COVERAGE_COUNT = ACTION_USER_CMD + 1,
  APPSTATE_EVENT_COVERAGE_COUNT = 9
};

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

static const char *const kAppStateOwnerFieldInvariantChecks0[] = {
  "invariant.inactive-panel-frozen",
  "invariant.shared-state-panel-local-isolation",
};
static const char *const kAppStateOwnerFieldInvariantChecks1[] = {
  "invariant.blocked-transition-determinism",
  "invariant.shared-state-panel-local-isolation",
};
static const char *const kAppStateOwnerFieldInvariantChecks2[] = {
  "invariant.blocked-transition-determinism",
  "invariant.render-projection-read-only",
};
static const char *const kAppStateOwnerFieldInvariantChecks3[] = {
  "invariant.panel-local-focus-restore",
  "invariant.blocked-transition-determinism",
};
static const char *const kAppStateOwnerFieldInvariantChecks4[] = {
  "invariant.blocked-transition-determinism",
  "invariant.stale-snapshot-fail-closed",
};
static const char *const kAppStateOwnerFieldInvariantChecks5[] = {
  "invariant.shared-state-panel-local-isolation",
  "invariant.viewport-identity-rebind",
};
static const char *const kAppStateOwnerFieldInvariantChecks6[] = {
  "invariant.render-projection-read-only",
  "invariant.viewport-identity-rebind",
};
static const char *const kAppStateOwnerFieldInvariantChecks7[] = {
  "invariant.render-projection-read-only",
  "invariant.blocked-transition-determinism",
};
static const char *const kAppStateOwnerFieldInvariantChecks8[] = {
  "invariant.render-projection-read-only",
  "invariant.panel-local-focus-restore",
};
static const char *const kAppStateOwnerFieldInvariantChecks9[] = {
  "invariant.viewport-identity-rebind",
  "invariant.inactive-panel-frozen",
};
static const char *const kAppStateOwnerFieldInvariantChecks10[] = {
  "invariant.viewport-identity-rebind",
  "invariant.render-projection-read-only",
};
static const char *const kAppStateOwnerFieldInvariantChecks11[] = {
  "invariant.panel-local-focus-restore",
  "invariant.shared-state-panel-local-isolation",
};
static const char *const kAppStateOwnerFieldInvariantChecks12[] = {
  "invariant.render-projection-read-only",
  "invariant.stale-snapshot-fail-closed",
};
static const char *const kAppStateOwnerFieldInvariantChecks13[] = {
  "invariant.viewport-identity-rebind",
  "invariant.stale-snapshot-fail-closed",
};
static const char *const kAppStateOwnerFieldInvariantChecks14[] = {
  "invariant.hidden-entry-visible-navigation",
  "invariant.inactive-panel-frozen",
};
static const char *const kAppStateOwnerFieldInvariantChecks15[] = {
  "invariant.viewport-identity-rebind",
  "invariant.stale-snapshot-fail-closed",
};
static const char *const kAppStateOwnerFieldInvariantChecks16[] = {
  "invariant.hidden-entry-visible-navigation",
  "invariant.render-projection-read-only",
};
static const char *const kAppStateOwnerFieldInvariantChecks17[] = {
  "invariant.stale-snapshot-fail-closed",
  "invariant.shared-state-panel-local-isolation",
};
static const char *const kAppStateOwnerFieldInvariantChecks18[] = {
  "invariant.shared-state-panel-local-isolation",
  "invariant.stale-snapshot-fail-closed",
};
static const char *const kAppStateOwnerFieldInvariantChecks19[] = {
  "invariant.shared-state-panel-local-isolation",
  "invariant.viewport-identity-rebind",
};
static const char *const kAppStateOwnerFieldInvariantChecks20[] = {
  "invariant.shared-state-panel-local-isolation",
  "invariant.blocked-transition-determinism",
};
static const char *const kAppStateOwnerFieldInvariantChecks21[] = {
  "invariant.render-projection-read-only",
  "invariant.stale-snapshot-fail-closed",
};
static const char *const kAppStateOwnerFieldInvariantChecks22[] = {
  "invariant.inactive-panel-frozen",
  "invariant.shared-state-panel-local-isolation",
};

static const AppStateOwnerFieldMetadata kAppStateOwnerFields[] = {
  {"ctx.active",
   "ctx/session state",
   "ViewContext.session routing",
   "ViewContext.active",
   "May change only during an allowed panel-routing or volume/menu transition commit.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks0,
   sizeof(kAppStateOwnerFieldInvariantChecks0) / sizeof(kAppStateOwnerFieldInvariantChecks0[0])},
  {"ctx.command_state",
   "ctx/session state",
   "ViewContext.command_region",
   "ViewContext command state fields",
   "May change only through command start/completion/cancel transitions.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks1,
   sizeof(kAppStateOwnerFieldInvariantChecks1) / sizeof(kAppStateOwnerFieldInvariantChecks1[0])},
  {"ctx.refresh_mode",
   "ctx/session state",
   "ViewContext.refresh_policy",
   "ViewContext.refresh_mode",
   "May change only through initialization or profile refresh policy commits before refresh dispatch observes it.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks1,
   sizeof(kAppStateOwnerFieldInvariantChecks1) / sizeof(kAppStateOwnerFieldInvariantChecks1[0])},
  {"ctx.view_mode",
   "ctx/session state",
   "ViewContext.view_mode",
   "ViewContext.view_mode",
   "May change only through allowed volume restore, log, or mode transition commits.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks1,
   sizeof(kAppStateOwnerFieldInvariantChecks1) / sizeof(kAppStateOwnerFieldInvariantChecks1[0])},
  {"ctx.dir_mode",
   "ctx/session state",
   "ViewContext.directory_display_mode",
   "ViewContext.dir_mode",
   "May change only through directory display mode commits that validate supported presentation modes.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks1,
   sizeof(kAppStateOwnerFieldInvariantChecks1) / sizeof(kAppStateOwnerFieldInvariantChecks1[0])},
  {"ctx.message_state",
   "ctx/session state",
   "ViewContext.message_region",
   "ViewContext message/footer state fields",
   "May change only through transitions that declare user-visible outcome or constraint messaging.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks2,
   sizeof(kAppStateOwnerFieldInvariantChecks2) / sizeof(kAppStateOwnerFieldInvariantChecks2[0])},
  {"ctx.modal_state",
   "ctx/session state",
   "ViewContext.modal_region",
   "ViewContext modal/dialog state fields",
   "May change only when entering, completing, or dismissing a registered modal transition.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks3,
   sizeof(kAppStateOwnerFieldInvariantChecks3) / sizeof(kAppStateOwnerFieldInvariantChecks3[0])},
  {"ctx.pending_transition",
   "ctx/session state",
   "ViewContext.transition_queue",
   "ViewContext pending transition marker",
   "May be queued only by transitions that declare a deterministic follow-up boundary.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks4,
   sizeof(kAppStateOwnerFieldInvariantChecks4) / sizeof(kAppStateOwnerFieldInvariantChecks4[0])},
  {"ctx.volumes_head",
   "volume/shared topology and payload state",
   "ViewContext.volume_registry",
   "ViewContext.volumes_head",
   "May change only through volume lifecycle transitions that validate panel bindings.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks5,
   sizeof(kAppStateOwnerFieldInvariantChecks5) / sizeof(kAppStateOwnerFieldInvariantChecks5[0])},
  {"ctx.layout",
   "render/projection/invalidation state",
   "ViewContext.layout_region",
   "ViewContext layout geometry fields",
   "May change only during resize/reflow transitions executed in the main loop.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks6,
   sizeof(kAppStateOwnerFieldInvariantChecks6) / sizeof(kAppStateOwnerFieldInvariantChecks6[0])},
  {"ctx.render_dirty_flags",
   "render/projection/invalidation state",
   "ViewContext.render_region",
   "ViewContext render invalidation fields",
   "May change only when a transition marks or clears declared projection surfaces.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks7,
   sizeof(kAppStateOwnerFieldInvariantChecks7) / sizeof(kAppStateOwnerFieldInvariantChecks7[0])},
  {"ctx.window_handles",
   "render/projection/invalidation state",
   "ViewContext ncurses window/layout handles",
   "ViewContext window handle fields",
   "May change only during main-loop layout/reflow or render projection setup.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks8,
   sizeof(kAppStateOwnerFieldInvariantChecks8) / sizeof(kAppStateOwnerFieldInvariantChecks8[0])},
  {"panel.file_selection_key",
   "panel-local state",
   "YtreeNovaPanel.file identity owner",
   "YtreeNovaPanel file selection identity fields",
   "May change only for the targeted panel through navigation, restore, or rebind transitions.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks9,
   sizeof(kAppStateOwnerFieldInvariantChecks9) / sizeof(kAppStateOwnerFieldInvariantChecks9[0])},
  {"panel.file_display_state",
   "panel-local state",
   "YtreeNovaPanel.file display projection state",
   "YtreeNovaPanel.file_mode and max_column",
   "May change only through file display mode commits that validate supported presentation modes and derived column bounds.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks22,
   sizeof(kAppStateOwnerFieldInvariantChecks22) /
       sizeof(kAppStateOwnerFieldInvariantChecks22[0])},
  {"panel.file_viewport_origin",
   "panel-local state",
   "YtreeNovaPanel.file viewport owner",
   "YtreeNovaPanel file viewport fields",
   "May change only for the targeted panel when navigation, bounds correction, or reflow declares it.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks10,
   sizeof(kAppStateOwnerFieldInvariantChecks10) / sizeof(kAppStateOwnerFieldInvariantChecks10[0])},
  {"panel.focus_shape",
   "panel-local state",
   "YtreeNovaPanel.focus owner",
   "YtreeNovaPanel focus/window-shape fields",
   "May change only during allowed focus, modal restore, split, or file/tree transition commits.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks11,
   sizeof(kAppStateOwnerFieldInvariantChecks11) / sizeof(kAppStateOwnerFieldInvariantChecks11[0])},
  {"panel.panel_generation",
   "panel-local state",
   "YtreeNovaPanel.generation owner",
   "YtreeNovaPanel panel generation field",
   "May increment only when panel-local restore authority changes.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks12,
   sizeof(kAppStateOwnerFieldInvariantChecks12) / sizeof(kAppStateOwnerFieldInvariantChecks12[0])},
  {"panel.restore_snapshot",
   "panel-local state",
   "YtreeNovaPanel.restore snapshot owner",
   "YtreeNovaPanel restore snapshot fields",
   "May change only through snapshot capture, rebind, fallback, or volume-binding transitions.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks13,
   sizeof(kAppStateOwnerFieldInvariantChecks13) / sizeof(kAppStateOwnerFieldInvariantChecks13[0])},
  {"panel.tree_cursor_pos",
   "panel-local state",
   "YtreeNovaPanel.tree cursor owner",
   "YtreeNovaPanel tree cursor field",
   "May change only for the targeted panel during tree navigation or rebind/fallback transitions.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks14,
   sizeof(kAppStateOwnerFieldInvariantChecks14) / sizeof(kAppStateOwnerFieldInvariantChecks14[0])},
  {"panel.tree_selection_key",
   "panel-local state",
   "YtreeNovaPanel.tree selection owner",
   "YtreeNovaPanel tree selection identity fields",
   "May change only through tree navigation, restore, or topology rebind transitions for the targeted panel.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks15,
   sizeof(kAppStateOwnerFieldInvariantChecks15) / sizeof(kAppStateOwnerFieldInvariantChecks15[0])},
  {"panel.tree_viewport_origin",
   "panel-local state",
   "YtreeNovaPanel.tree viewport owner",
   "YtreeNovaPanel tree viewport fields",
   "May change only when navigation, bounds correction, resize, or rebind declares viewport mutation.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks16,
   sizeof(kAppStateOwnerFieldInvariantChecks16) / sizeof(kAppStateOwnerFieldInvariantChecks16[0])},
  {"panel.volume_key",
   "panel-local state",
   "YtreeNovaPanel.volume binding owner",
   "YtreeNovaPanel volume binding fields",
   "May change only through volume selection, cycle, release, or restore transitions.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks17,
   sizeof(kAppStateOwnerFieldInvariantChecks17) / sizeof(kAppStateOwnerFieldInvariantChecks17[0])},
  {"volume.dir_tree",
   "volume/shared topology and payload state",
   "Volume.topology owner",
   "Volume directory tree fields",
   "May change only through logging, rebuild, refresh, release, or completed filesystem mutation transitions.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks18,
   sizeof(kAppStateOwnerFieldInvariantChecks18) / sizeof(kAppStateOwnerFieldInvariantChecks18[0])},
  {"volume.logged_state",
   "volume/shared topology and payload state",
   "Volume.logged topology owner",
   "Volume logged/unlogged directory state",
   "May change only through explicit log, relog, release, collapse, refresh, or rebuild transitions.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks19,
   sizeof(kAppStateOwnerFieldInvariantChecks19) / sizeof(kAppStateOwnerFieldInvariantChecks19[0])},
  {"volume.payload_cache",
   "volume/shared topology and payload state",
   "Volume.payload cache owner",
   "Volume file payload/statistics cache fields",
   "May change only through payload load, refresh, archive, or completed filesystem mutation transitions.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks20,
   sizeof(kAppStateOwnerFieldInvariantChecks20) / sizeof(kAppStateOwnerFieldInvariantChecks20[0])},
  {"volume.volume_generation",
   "volume/shared topology and payload state",
   "Volume.generation owner",
   "Volume topology/payload generation field",
   "May increment only when shared topology, payload identity, logged state, or namespace mapping changes.",
   "runtime_backed",
   kAppStateOwnerFieldInvariantChecks21,
   sizeof(kAppStateOwnerFieldInvariantChecks21) / sizeof(kAppStateOwnerFieldInvariantChecks21[0])},
};

static const char *const kAppStateGenerationDomainIdentityFields0[] = {
  "panel.tree_selection_key",
  "panel.file_selection_key",
  "panel.tree_viewport_origin",
  "panel.file_viewport_origin",
  "panel.focus_shape",
  "panel.restore_snapshot",
};
static const char *const kAppStateGenerationDomainIdentityFields1[] = {
  "ctx.volumes_head",
  "volume.dir_tree",
  "volume.logged_state",
  "volume.payload_cache",
  "panel.volume_key",
};
static const char *const kAppStateGenerationDomainIdentityFields2[] = {
  "panel.tree_selection_key",
  "panel.tree_cursor_pos",
  "panel.tree_viewport_origin",
  "volume.dir_tree",
  "volume.logged_state",
};
static const char *const kAppStateGenerationDomainIdentityFields3[] = {
  "panel.file_selection_key",
  "panel.file_viewport_origin",
  "volume.payload_cache",
  "volume.dir_tree",
};
static const char *const kAppStateGenerationDomainIdentityFields4[] = {
  "panel.focus_shape",
  "panel.restore_snapshot",
};
static const char *const kAppStateGenerationDomainIdentityFields5[] = {
  "ctx.modal_state",
  "ctx.command_state",
  "ctx.pending_transition",
  "ctx.message_state",
};
static const char *const kAppStateGenerationDomainIdentityFields6[] = {
  "panel.tree_selection_key",
  "panel.tree_viewport_origin",
  "panel.file_selection_key",
  "panel.file_viewport_origin",
  "volume.logged_state",
};
static const char *const kAppStateGenerationDomainIdentityFields7[] = {
  "volume.dir_tree",
  "volume.logged_state",
  "ctx.volumes_head",
};
static const char *const kAppStateGenerationDomainIdentityFields8[] = {
  "volume.payload_cache",
  "panel.file_selection_key",
  "panel.file_viewport_origin",
};
static const char *const kAppStateGenerationDomainIdentityFields9[] = {
  "ctx.volumes_head",
  "panel.volume_key",
  "panel.restore_snapshot",
  "volume.logged_state",
};
static const char *const kAppStateGenerationDomainIdentityFields10[] = {
  "ctx.layout",
  "ctx.window_handles",
  "ctx.render_dirty_flags",
  "panel.tree_viewport_origin",
  "panel.file_viewport_origin",
};
static const char *const kAppStateGenerationDomainCoverageTransitionIds0[] = {
  "transition.keybinding.navigate-tree",
  "transition.menu-action.volume-select",
  "transition.modal-action.dismiss",
  "transition.modal-action.completion",
  "transition.refresh-rebuild.manual-refresh",
  "transition.refresh-rebuild.watcher-live-refresh",
  "transition.volume-operation.release-cycle",
  "transition.terminal-signal-resize",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
  "transition.rebuild-rebind-callback.panel-anchor",
};
static const char *const kAppStateGenerationDomainCoverageTransitionIds1[] = {
  "transition.refresh-rebuild.manual-refresh",
  "transition.refresh-rebuild.watcher-live-refresh",
  "transition.volume-operation.release-cycle",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
};
static const char *const kAppStateGenerationDomainCoverageTransitionIds2[] = {
  "transition.refresh-rebuild.manual-refresh",
  "transition.refresh-rebuild.watcher-live-refresh",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
  "transition.volume-operation.release-cycle",
  "transition.rebuild-rebind-callback.panel-anchor",
};
static const char *const kAppStateGenerationDomainCoverageTransitionIds3[] = {
  "transition.refresh-rebuild.manual-refresh",
  "transition.refresh-rebuild.watcher-live-refresh",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
  "transition.rebuild-rebind-callback.panel-anchor",
};
static const char *const kAppStateGenerationDomainCoverageTransitionIds4[] = {
  "transition.keybinding.navigate-tree",
  "transition.modal-action.dismiss",
  "transition.modal-action.completion",
  "transition.menu-action.volume-select",
  "transition.rebuild-rebind-callback.panel-anchor",
};
static const char *const kAppStateGenerationDomainCoverageTransitionIds5[] = {
  "transition.modal-action.dismiss",
  "transition.modal-action.completion",
  "transition.command-completion.user-command",
};
static const char *const kAppStateGenerationDomainCoverageTransitionIds6[] = {
  "transition.refresh-rebuild.manual-refresh",
  "transition.refresh-rebuild.watcher-live-refresh",
  "transition.keybinding.navigate-tree",
  "transition.rebuild-rebind-callback.panel-anchor",
};
static const char *const kAppStateGenerationDomainCoverageTransitionIds7[] = {
  "transition.refresh-rebuild.manual-refresh",
  "transition.refresh-rebuild.watcher-live-refresh",
  "transition.volume-operation.release-cycle",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
};
static const char *const kAppStateGenerationDomainCoverageTransitionIds8[] = {
  "transition.refresh-rebuild.manual-refresh",
  "transition.refresh-rebuild.watcher-live-refresh",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
  "transition.rebuild-rebind-callback.panel-anchor",
};
static const char *const kAppStateGenerationDomainCoverageTransitionIds9[] = {
  "transition.volume-operation.release-cycle",
  "transition.menu-action.volume-select",
  "transition.refresh-rebuild.manual-refresh",
  "transition.refresh-rebuild.watcher-live-refresh",
};
static const char *const kAppStateGenerationDomainCoverageTransitionIds10[] = {
  "transition.terminal-signal-resize",
  "transition.render-reflow.project-state",
};
static const char *const kAppStateGenerationDomainAdvancesOnTransitionIds0[] = {
  "transition.keybinding.navigate-tree",
  "transition.menu-action.volume-select",
  "transition.modal-action.dismiss",
  "transition.modal-action.completion",
  "transition.refresh-rebuild.manual-refresh",
  "transition.refresh-rebuild.watcher-live-refresh",
  "transition.volume-operation.release-cycle",
  "transition.terminal-signal-resize",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
  "transition.rebuild-rebind-callback.panel-anchor",
};
static const char *const kAppStateGenerationDomainAdvancesOnTransitionIds1[] = {
  "transition.refresh-rebuild.manual-refresh",
  "transition.refresh-rebuild.watcher-live-refresh",
  "transition.volume-operation.release-cycle",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
};
static const char *const kAppStateGenerationDomainAdvancesOnTransitionIds2[] = {
  "transition.refresh-rebuild.manual-refresh",
  "transition.refresh-rebuild.watcher-live-refresh",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
  "transition.volume-operation.release-cycle",
  "transition.rebuild-rebind-callback.panel-anchor",
};
static const char *const kAppStateGenerationDomainAdvancesOnTransitionIds3[] = {
  "transition.refresh-rebuild.manual-refresh",
  "transition.refresh-rebuild.watcher-live-refresh",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
  "transition.rebuild-rebind-callback.panel-anchor",
};
static const char *const kAppStateGenerationDomainAdvancesOnTransitionIds4[] = {
  "transition.keybinding.navigate-tree",
  "transition.modal-action.dismiss",
  "transition.modal-action.completion",
  "transition.menu-action.volume-select",
  "transition.rebuild-rebind-callback.panel-anchor",
};
static const char *const kAppStateGenerationDomainAdvancesOnTransitionIds5[] = {
  "transition.modal-action.dismiss",
  "transition.modal-action.completion",
  "transition.command-completion.user-command",
};
static const char *const kAppStateGenerationDomainAdvancesOnTransitionIds6[] = {
  "transition.refresh-rebuild.manual-refresh",
  "transition.refresh-rebuild.watcher-live-refresh",
  "transition.keybinding.navigate-tree",
  "transition.rebuild-rebind-callback.panel-anchor",
};
static const char *const kAppStateGenerationDomainAdvancesOnTransitionIds7[] = {
  "transition.refresh-rebuild.manual-refresh",
  "transition.refresh-rebuild.watcher-live-refresh",
  "transition.volume-operation.release-cycle",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
};
static const char *const kAppStateGenerationDomainAdvancesOnTransitionIds8[] = {
  "transition.refresh-rebuild.manual-refresh",
  "transition.refresh-rebuild.watcher-live-refresh",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
  "transition.rebuild-rebind-callback.panel-anchor",
};
static const char *const kAppStateGenerationDomainAdvancesOnTransitionIds9[] = {
  "transition.volume-operation.release-cycle",
  "transition.menu-action.volume-select",
  "transition.refresh-rebuild.manual-refresh",
  "transition.refresh-rebuild.watcher-live-refresh",
};
static const char *const kAppStateGenerationDomainAdvancesOnTransitionIds10[] = {
  "transition.terminal-signal-resize",
};
static const char *const kAppStateGenerationDomainMigrationNotes0[] = {
  "Runtime panel_generation domain metadata is registered and validated against the canonical panel UI state record.",
};
static const char *const kAppStateGenerationDomainMigrationNotes1[] = {
  "Runtime volume_generation domain metadata is registered and validated against Volume topology and payload commits.",
};
static const char *const kAppStateGenerationDomainMigrationNotes2[] = {
  "Directory identity runtime metadata is registered with volume.volume_generation as the authoritative invalidation owner for topology changes.",
};
static const char *const kAppStateGenerationDomainMigrationNotes3[] = {
  "File identity runtime metadata is registered with volume.volume_generation as the authoritative owner for payload and namespace invalidation.",
};
static const char *const kAppStateGenerationDomainMigrationNotes4[] = {
  "Focus-shape runtime metadata is registered with panel.panel_generation as the current invalidation owner until a narrower focus-shape generation exists.",
};
static const char *const kAppStateGenerationDomainMigrationNotes5[] = {
  "Modal command target runtime metadata is registered with panel.panel_generation guarding command paths that restore focus or queue panel-local follow-up work.",
};
static const char *const kAppStateGenerationDomainMigrationNotes6[] = {
  "Visibility-filter runtime metadata is registered with panel.panel_generation as the panel-local invalidation owner for filter and dotfile visibility state.",
};
static const char *const kAppStateGenerationDomainMigrationNotes7[] = {
  "Topology runtime metadata is registered with volume.volume_generation representing Volume topology commit invalidation.",
};
static const char *const kAppStateGenerationDomainMigrationNotes8[] = {
  "File payload runtime metadata is registered with volume.volume_generation until a narrower payload generation field is registered.",
};
static const char *const kAppStateGenerationDomainMigrationNotes9[] = {
  "Volume lifecycle runtime metadata is registered with volume.volume_generation as the shared invalidation owner while ctx.volumes_head remains the registry identity field.",
};
static const char *const kAppStateGenerationDomainMigrationNotes10[] = {
  "Render projection itself is read-only/projection-only and does not advance generations; terminal resize uses panel.panel_generation only when saved viewport origins are corrected.",
  "Render projection-only coverage is registered explicitly even when no generation counter advances.",
};

static const AppStateGenerationDomainMetadata kAppStateGenerationDomains[] = {
  {"generation.panel.local-authority",
   "panel_generation",
   "panel-local state",
   "panel.panel_generation",
   kAppStateGenerationDomainIdentityFields0,
   sizeof(kAppStateGenerationDomainIdentityFields0) / sizeof(kAppStateGenerationDomainIdentityFields0[0]),
   kAppStateGenerationDomainCoverageTransitionIds0,
   sizeof(kAppStateGenerationDomainCoverageTransitionIds0) / sizeof(kAppStateGenerationDomainCoverageTransitionIds0[0]),
   kAppStateGenerationDomainAdvancesOnTransitionIds0,
   sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds0) / sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds0[0]),
   "Reject snapshots whose saved panel_generation does not match the panel-local generation marker before restore authority is applied.",
   "Re-resolve by stable identity, then nearest visible ancestor, next visible sibling, previous visible sibling, and finally root visible node.",
   "Canonical panel-anchor restore helpers commit panel-local selection, viewport, focus shape, and snapshot generation together.",
   "covered_by_runtime_registry",
   kAppStateGenerationDomainMigrationNotes0,
   sizeof(kAppStateGenerationDomainMigrationNotes0) / sizeof(kAppStateGenerationDomainMigrationNotes0[0])},
  {"generation.volume.shared-authority",
   "volume_generation",
   "volume/shared topology and payload state",
   "volume.volume_generation",
   kAppStateGenerationDomainIdentityFields1,
   sizeof(kAppStateGenerationDomainIdentityFields1) / sizeof(kAppStateGenerationDomainIdentityFields1[0]),
   kAppStateGenerationDomainCoverageTransitionIds1,
   sizeof(kAppStateGenerationDomainCoverageTransitionIds1) / sizeof(kAppStateGenerationDomainCoverageTransitionIds1[0]),
   kAppStateGenerationDomainAdvancesOnTransitionIds1,
   sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds1) / sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds1[0]),
   "Treat any saved volume_generation mismatch as stale and require topology/payload identity re-resolution before panel snapshots are reused.",
   "Keep the previous settled topology on blocked transitions; after invalidation, rebind panels through stable identities or fall back to root visible node.",
   "Refresh/rebuild and mutation-result commits advance volume generation before panel restore consumers can observe the changed volume.",
   "covered_by_runtime_registry",
   kAppStateGenerationDomainMigrationNotes1,
   sizeof(kAppStateGenerationDomainMigrationNotes1) / sizeof(kAppStateGenerationDomainMigrationNotes1[0])},
  {"identity.directory.stable-key",
   "directory_identity",
   "panel-local state plus volume/shared topology and payload state",
   "volume.volume_generation",
   kAppStateGenerationDomainIdentityFields2,
   sizeof(kAppStateGenerationDomainIdentityFields2) / sizeof(kAppStateGenerationDomainIdentityFields2[0]),
   kAppStateGenerationDomainCoverageTransitionIds2,
   sizeof(kAppStateGenerationDomainCoverageTransitionIds2) / sizeof(kAppStateGenerationDomainCoverageTransitionIds2[0]),
   kAppStateGenerationDomainAdvancesOnTransitionIds2,
   sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds2) / sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds2[0]),
   "Directory snapshots must compare stable path identity against the current volume generation before row or cursor state is trusted.",
   "Exact directory identity, nearest visible ancestor, next visible sibling, previous visible sibling, then root visible node.",
   "Directory rebind runs through panel-anchor helpers after the shared topology generation has settled.",
   "covered_by_runtime_registry",
   kAppStateGenerationDomainMigrationNotes2,
   sizeof(kAppStateGenerationDomainMigrationNotes2) / sizeof(kAppStateGenerationDomainMigrationNotes2[0])},
  {"identity.file.stable-key",
   "file_identity",
   "panel-local state plus volume/shared topology and payload state",
   "volume.volume_generation",
   kAppStateGenerationDomainIdentityFields3,
   sizeof(kAppStateGenerationDomainIdentityFields3) / sizeof(kAppStateGenerationDomainIdentityFields3[0]),
   kAppStateGenerationDomainCoverageTransitionIds3,
   sizeof(kAppStateGenerationDomainCoverageTransitionIds3) / sizeof(kAppStateGenerationDomainCoverageTransitionIds3[0]),
   kAppStateGenerationDomainAdvancesOnTransitionIds3,
   sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds3) / sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds3[0]),
   "File snapshots must revalidate path/name identity and payload membership when volume generation changes.",
   "Preserve the directory anchor when possible and choose a valid visible file only after exact file identity is unavailable.",
   "File identity restore is committed with the panel snapshot after topology or payload mutation has settled.",
   "covered_by_runtime_registry",
   kAppStateGenerationDomainMigrationNotes3,
   sizeof(kAppStateGenerationDomainMigrationNotes3) / sizeof(kAppStateGenerationDomainMigrationNotes3[0])},
  {"shape.panel.focus",
   "focus_shape",
   "panel-local state",
   "panel.panel_generation",
   kAppStateGenerationDomainIdentityFields4,
   sizeof(kAppStateGenerationDomainIdentityFields4) / sizeof(kAppStateGenerationDomainIdentityFields4[0]),
   kAppStateGenerationDomainCoverageTransitionIds4,
   sizeof(kAppStateGenerationDomainCoverageTransitionIds4) / sizeof(kAppStateGenerationDomainCoverageTransitionIds4[0]),
   kAppStateGenerationDomainAdvancesOnTransitionIds4,
   sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds4) / sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds4[0]),
   "Saved focus shape is stale when panel_generation differs and must not be restored by transient render shape.",
   "Restore the recorded panel shape directly or keep the current settled shape without rendering an intermediate guess.",
   "Modal dismissal, panel reactivation, and rebind callbacks commit focus shape only through panel-local state.",
   "covered_by_runtime_registry",
   kAppStateGenerationDomainMigrationNotes4,
   sizeof(kAppStateGenerationDomainMigrationNotes4) / sizeof(kAppStateGenerationDomainMigrationNotes4[0])},
  {"target.modal-command.session",
   "modal_command_target",
   "ctx/session state",
   "panel.panel_generation",
   kAppStateGenerationDomainIdentityFields5,
   sizeof(kAppStateGenerationDomainIdentityFields5) / sizeof(kAppStateGenerationDomainIdentityFields5[0]),
   kAppStateGenerationDomainCoverageTransitionIds5,
   sizeof(kAppStateGenerationDomainCoverageTransitionIds5) / sizeof(kAppStateGenerationDomainCoverageTransitionIds5[0]),
   kAppStateGenerationDomainAdvancesOnTransitionIds5,
   sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds5) / sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds5[0]),
   "Modal and command targets may write panel or volume state only through a declared follow-up transition boundary.",
   "On blocked or failed command completion, preserve authoritative panel and volume state and write only declared message/modal fields.",
   "Modal dismissal and command completion settle session state before any panel restore or refresh follow-up observes the result.",
   "covered_by_runtime_registry",
   kAppStateGenerationDomainMigrationNotes5,
   sizeof(kAppStateGenerationDomainMigrationNotes5) / sizeof(kAppStateGenerationDomainMigrationNotes5[0])},
  {"state.visibility-filter.panel-volume",
   "visibility_filter_state",
   "panel-local state plus volume/shared topology and payload state",
   "panel.panel_generation",
   kAppStateGenerationDomainIdentityFields6,
   sizeof(kAppStateGenerationDomainIdentityFields6) / sizeof(kAppStateGenerationDomainIdentityFields6[0]),
   kAppStateGenerationDomainCoverageTransitionIds6,
   sizeof(kAppStateGenerationDomainCoverageTransitionIds6) / sizeof(kAppStateGenerationDomainCoverageTransitionIds6[0]),
   kAppStateGenerationDomainAdvancesOnTransitionIds6,
   sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds6) / sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds6[0]),
   "Visibility/filter changes that alter rendered rows invalidate saved panel anchors before any snapshot can be reused.",
   "Rebind visible identity through the deterministic fallback order rather than deriving from hidden or filtered row indexes.",
   "Visibility-filter commits update panel generation before rebind and render projection.",
   "covered_by_runtime_registry",
   kAppStateGenerationDomainMigrationNotes6,
   sizeof(kAppStateGenerationDomainMigrationNotes6) / sizeof(kAppStateGenerationDomainMigrationNotes6[0])},
  {"state.topology.volume",
   "topology_state",
   "volume/shared topology and payload state",
   "volume.volume_generation",
   kAppStateGenerationDomainIdentityFields7,
   sizeof(kAppStateGenerationDomainIdentityFields7) / sizeof(kAppStateGenerationDomainIdentityFields7[0]),
   kAppStateGenerationDomainCoverageTransitionIds7,
   sizeof(kAppStateGenerationDomainCoverageTransitionIds7) / sizeof(kAppStateGenerationDomainCoverageTransitionIds7[0]),
   kAppStateGenerationDomainAdvancesOnTransitionIds7,
   sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds7) / sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds7[0]),
   "Topology snapshots are invalid after any volume_generation advance and must be rebuilt or rebound by identity.",
   "Blocked topology changes retain the previous settled tree; completed invalidation falls back panel anchors only after exact identity fails.",
   "Topology rebuild commits settle volume.dir_tree and volume.logged_state before panel rebind callbacks run.",
   "covered_by_runtime_registry",
   kAppStateGenerationDomainMigrationNotes7,
   sizeof(kAppStateGenerationDomainMigrationNotes7) / sizeof(kAppStateGenerationDomainMigrationNotes7[0])},
  {"state.file-payload.volume",
   "file_payload_state",
   "volume/shared topology and payload state",
   "volume.volume_generation",
   kAppStateGenerationDomainIdentityFields8,
   sizeof(kAppStateGenerationDomainIdentityFields8) / sizeof(kAppStateGenerationDomainIdentityFields8[0]),
   kAppStateGenerationDomainCoverageTransitionIds8,
   sizeof(kAppStateGenerationDomainCoverageTransitionIds8) / sizeof(kAppStateGenerationDomainCoverageTransitionIds8[0]),
   kAppStateGenerationDomainAdvancesOnTransitionIds8,
   sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds8) / sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds8[0]),
   "Payload cache identity must be revalidated on generation mismatch before saved file selection is restored.",
   "If payload cannot be loaded safely, preserve directory selection and leave file authority unchanged or empty according to current AppState.",
   "Payload changes settle in Volume before panel file anchors are rebound.",
   "covered_by_runtime_registry",
   kAppStateGenerationDomainMigrationNotes8,
   sizeof(kAppStateGenerationDomainMigrationNotes8) / sizeof(kAppStateGenerationDomainMigrationNotes8[0])},
  {"lifecycle.volume.registry",
   "volume_lifecycle",
   "ctx/session state plus volume/shared topology and payload state",
   "volume.volume_generation",
   kAppStateGenerationDomainIdentityFields9,
   sizeof(kAppStateGenerationDomainIdentityFields9) / sizeof(kAppStateGenerationDomainIdentityFields9[0]),
   kAppStateGenerationDomainCoverageTransitionIds9,
   sizeof(kAppStateGenerationDomainCoverageTransitionIds9) / sizeof(kAppStateGenerationDomainCoverageTransitionIds9[0]),
   kAppStateGenerationDomainAdvancesOnTransitionIds9,
   sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds9) / sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds9[0]),
   "Panel volume bindings must resolve to an existing volume identity before per-volume snapshots can be restored.",
   "Do not orphan panel bindings; use deterministic volume fallback and then panel identity fallback after release or cycle invalidation.",
   "Volume lifecycle transitions update registry/binding state before panel-local restore snapshots are applied.",
   "covered_by_runtime_registry",
   kAppStateGenerationDomainMigrationNotes9,
   sizeof(kAppStateGenerationDomainMigrationNotes9) / sizeof(kAppStateGenerationDomainMigrationNotes9[0])},
  {"reflow.layout.projection",
   "layout_reflow",
   "render/projection/invalidation state",
   "panel.panel_generation",
   kAppStateGenerationDomainIdentityFields10,
   sizeof(kAppStateGenerationDomainIdentityFields10) / sizeof(kAppStateGenerationDomainIdentityFields10[0]),
   kAppStateGenerationDomainCoverageTransitionIds10,
   sizeof(kAppStateGenerationDomainCoverageTransitionIds10) / sizeof(kAppStateGenerationDomainCoverageTransitionIds10[0]),
   kAppStateGenerationDomainAdvancesOnTransitionIds10,
   sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds10) / sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds10[0]),
   "Layout reflow must project from settled AppState and may advance panel generation only for viewport bounds correction.",
   "If safe projection cannot be computed, degrade or skip render without choosing new authoritative identities.",
   "Resize handling runs in the main loop and commits any viewport correction before render projection.",
   "covered_by_runtime_registry",
   kAppStateGenerationDomainMigrationNotes10,
   sizeof(kAppStateGenerationDomainMigrationNotes10) / sizeof(kAppStateGenerationDomainMigrationNotes10[0])},
};

static const char *const kAppStateDiffHarnessSnapshotPhases0[] = {
  "before_guard",
  "after_allowed_or_blocked_result",
};

static const char *const kAppStateDiffHarnessSnapshotRegions0[] = {
  "ctx/session state",
  "panel-local state",
  "volume/shared topology and payload state",
  "render/projection/invalidation state",
};

static const char *const kAppStateDiffHarnessTransitionIds0[] = {
  "transition.keybinding.navigate-tree",
  "transition.refresh-rebuild.manual-refresh",
  "transition.refresh-rebuild.watcher-live-refresh",
  "transition.volume-operation.release-cycle",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
  "transition.rebuild-rebind-callback.panel-anchor",
};

static const char *const kAppStateDiffHarnessOwnerFieldRefs0[] = {
  "ctx.active",
  "ctx.volumes_head",
  "panel.tree_selection_key",
  "panel.tree_cursor_pos",
  "panel.tree_viewport_origin",
  "panel.file_selection_key",
  "panel.file_display_state",
  "panel.file_viewport_origin",
  "panel.restore_snapshot",
  "panel.panel_generation",
  "volume.dir_tree",
  "volume.logged_state",
  "volume.volume_generation",
};

static const char *const kAppStateDiffHarnessInvariantIds0[] = {
  "invariant.inactive-panel-frozen",
  "invariant.shared-state-panel-local-isolation",
  "invariant.viewport-identity-rebind",
};

static const char *const kAppStateDiffHarnessGenerationDomainIds0[] = {
  "generation.panel.local-authority",
  "generation.volume.shared-authority",
  "identity.directory.stable-key",
  "identity.file.stable-key",
  "shape.panel.focus",
  "state.visibility-filter.panel-volume",
  "state.topology.volume",
  "lifecycle.volume.registry",
};

static const char *const kAppStateDiffHarnessMigrationNotes0[] = {
  "This registry entry defines the machine-readable coverage target; no runtime C runner exists in this unit.",
};

static const char *const kAppStateDiffHarnessSnapshotPhases1[] = {
  "before_transition",
  "after_transition_commit",
};

static const char *const kAppStateDiffHarnessSnapshotRegions1[] = {
  "ctx/session state",
  "panel-local state",
  "volume/shared topology and payload state",
};

static const char *const kAppStateDiffHarnessTransitionIds1[] = {
  "transition.keybinding.navigate-tree",
  "transition.menu-action.volume-select",
  "transition.modal-action.dismiss",
  "transition.modal-action.completion",
  "transition.command-completion.user-command",
};

static const char *const kAppStateDiffHarnessOwnerFieldRefs1[] = {
  "ctx.active",
  "ctx.command_state",
  "ctx.refresh_mode",
  "ctx.view_mode",
  "ctx.dir_mode",
  "ctx.message_state",
  "ctx.modal_state",
  "ctx.pending_transition",
  "panel.volume_key",
  "panel.tree_selection_key",
  "panel.tree_cursor_pos",
  "panel.tree_viewport_origin",
  "panel.file_display_state",
  "panel.focus_shape",
  "panel.restore_snapshot",
  "panel.panel_generation",
};

static const char *const kAppStateDiffHarnessInvariantIds1[] = {
  "invariant.panel-local-focus-restore",
  "invariant.shared-state-panel-local-isolation",
  "invariant.blocked-transition-determinism",
  "invariant.inactive-panel-frozen",
};

static const char *const kAppStateDiffHarnessGenerationDomainIds1[] = {
  "generation.panel.local-authority",
  "shape.panel.focus",
  "target.modal-command.session",
  "lifecycle.volume.registry",
};

static const char *const kAppStateDiffHarnessMigrationNotes1[] = {
  "Runtime migration should derive concrete field comparisons from docs/appstate_transition_matrix.json declared_write_set records.",
};

static const char *const kAppStateDiffHarnessSnapshotPhases2[] = {
  "before_render_projection",
  "after_render_projection",
};

static const char *const kAppStateDiffHarnessSnapshotRegions2[] = {
  "panel-local state",
  "volume/shared topology and payload state",
  "render/projection/invalidation state",
};

static const char *const kAppStateDiffHarnessTransitionIds2[] = {
  "transition.render-reflow.project-state",
};

static const char *const kAppStateDiffHarnessOwnerFieldRefs2[] = {
  "ctx.render_dirty_flags",
  "ctx.window_handles",
  "panel.tree_selection_key",
  "panel.file_selection_key",
  "panel.tree_viewport_origin",
  "panel.file_viewport_origin",
  "panel.focus_shape",
  "panel.panel_generation",
  "volume.volume_generation",
};

static const char *const kAppStateDiffHarnessInvariantIds2[] = {
  "invariant.render-projection-read-only",
  "invariant.inactive-panel-frozen",
};

static const char *const kAppStateDiffHarnessGenerationDomainIds2[] = {
  "reflow.layout.projection",
  "generation.panel.local-authority",
  "generation.volume.shared-authority",
};

static const char *const kAppStateDiffHarnessMigrationNotes2[] = {
  "Dirty-flag clearing and window-handle staging remain named so the future harness can distinguish projection bookkeeping from selection authority.",
};

static const char *const kAppStateDiffHarnessSnapshotPhases3[] = {
  "saved_snapshot",
  "current_authority",
  "restore_attempt_result",
};

static const char *const kAppStateDiffHarnessSnapshotRegions3[] = {
  "panel-local state",
  "volume/shared topology and payload state",
};

static const char *const kAppStateDiffHarnessTransitionIds3[] = {
  "transition.refresh-rebuild.manual-refresh",
  "transition.refresh-rebuild.watcher-live-refresh",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
  "transition.rebuild-rebind-callback.panel-anchor",
  "transition.terminal-signal-resize",
};

static const char *const kAppStateDiffHarnessOwnerFieldRefs3[] = {
  "ctx.layout",
  "ctx.window_handles",
  "panel.restore_snapshot",
  "panel.tree_selection_key",
  "panel.tree_cursor_pos",
  "panel.tree_viewport_origin",
  "panel.file_selection_key",
  "panel.file_viewport_origin",
  "panel.panel_generation",
  "volume.dir_tree",
  "volume.payload_cache",
  "volume.volume_generation",
};

static const char *const kAppStateDiffHarnessInvariantIds3[] = {
  "invariant.stale-snapshot-fail-closed",
  "invariant.viewport-identity-rebind",
  "invariant.hidden-entry-visible-navigation",
  "invariant.render-projection-read-only",
  "invariant.shared-state-panel-local-isolation",
};

static const char *const kAppStateDiffHarnessGenerationDomainIds3[] = {
  "generation.panel.local-authority",
  "generation.volume.shared-authority",
  "identity.directory.stable-key",
  "identity.file.stable-key",
  "state.topology.volume",
  "state.file-payload.volume",
  "reflow.layout.projection",
};

static const char *const kAppStateDiffHarnessMigrationNotes3[] = {
  "The future runner should seed mismatched saved/current generations to verify deterministic restore fallback.",
};

static const char *const kAppStateDiffHarnessSnapshotPhases4[] = {
  "before_guard",
  "after_blocked_result",
};

static const char *const kAppStateDiffHarnessSnapshotRegions4[] = {
  "ctx/session state",
  "panel-local state",
  "volume/shared topology and payload state",
};

static const char *const kAppStateDiffHarnessTransitionIds4[] = {
  "transition.keybinding.navigate-tree",
  "transition.modal-action.dismiss",
  "transition.modal-action.completion",
  "transition.volume-operation.release-cycle",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
  "transition.command-completion.user-command",
};

static const char *const kAppStateDiffHarnessOwnerFieldRefs4[] = {
  "ctx.message_state",
  "ctx.modal_state",
  "panel.volume_key",
  "panel.tree_selection_key",
  "panel.file_selection_key",
  "panel.panel_generation",
  "volume.dir_tree",
  "volume.volume_generation",
};

static const char *const kAppStateDiffHarnessInvariantIds4[] = {
  "invariant.blocked-transition-determinism",
  "invariant.inactive-panel-frozen",
  "invariant.shared-state-panel-local-isolation",
};

static const char *const kAppStateDiffHarnessGenerationDomainIds4[] = {
  "generation.panel.local-authority",
  "generation.volume.shared-authority",
  "lifecycle.volume.registry",
  "target.modal-command.session",
};

static const char *const kAppStateDiffHarnessMigrationNotes4[] = {
  "Concrete blocked-result allowances should be derived from the target transition's blocked_result and declared_write_set records.",
};

static const AppStateDiffHarnessMetadata kAppStateDiffHarnesses[] = {
  {"harness.transition-before-after-snapshot",
   "transition_before_after_snapshot",
   kAppStateDiffHarnessSnapshotPhases0,
   sizeof(kAppStateDiffHarnessSnapshotPhases0) / sizeof(kAppStateDiffHarnessSnapshotPhases0[0]),
   kAppStateDiffHarnessSnapshotRegions0,
   sizeof(kAppStateDiffHarnessSnapshotRegions0) / sizeof(kAppStateDiffHarnessSnapshotRegions0[0]),
   kAppStateDiffHarnessTransitionIds0,
   sizeof(kAppStateDiffHarnessTransitionIds0) / sizeof(kAppStateDiffHarnessTransitionIds0[0]),
   kAppStateDiffHarnessOwnerFieldRefs0,
   sizeof(kAppStateDiffHarnessOwnerFieldRefs0) / sizeof(kAppStateDiffHarnessOwnerFieldRefs0[0]),
   kAppStateDiffHarnessInvariantIds0,
   sizeof(kAppStateDiffHarnessInvariantIds0) / sizeof(kAppStateDiffHarnessInvariantIds0[0]),
   kAppStateDiffHarnessGenerationDomainIds0,
   sizeof(kAppStateDiffHarnessGenerationDomainIds0) / sizeof(kAppStateDiffHarnessGenerationDomainIds0[0]),
   "A later dynamic harness snapshots every referenced AppState region before guard evaluation and after the transition result, then compares only authoritative state owned by the registered transition boundary.",
   "Any unclassified region change, stale snapshot reuse, or missing before/after phase is reported as a transition contract violation.",
   "covered_by_runtime_registry",
   kAppStateDiffHarnessMigrationNotes0,
   sizeof(kAppStateDiffHarnessMigrationNotes0) / sizeof(kAppStateDiffHarnessMigrationNotes0[0])},
  {"harness.declared-write-set-diff",
   "declared_write_set_diff",
   kAppStateDiffHarnessSnapshotPhases1,
   sizeof(kAppStateDiffHarnessSnapshotPhases1) / sizeof(kAppStateDiffHarnessSnapshotPhases1[0]),
   kAppStateDiffHarnessSnapshotRegions1,
   sizeof(kAppStateDiffHarnessSnapshotRegions1) / sizeof(kAppStateDiffHarnessSnapshotRegions1[0]),
   kAppStateDiffHarnessTransitionIds1,
   sizeof(kAppStateDiffHarnessTransitionIds1) / sizeof(kAppStateDiffHarnessTransitionIds1[0]),
   kAppStateDiffHarnessOwnerFieldRefs1,
   sizeof(kAppStateDiffHarnessOwnerFieldRefs1) / sizeof(kAppStateDiffHarnessOwnerFieldRefs1[0]),
   kAppStateDiffHarnessInvariantIds1,
   sizeof(kAppStateDiffHarnessInvariantIds1) / sizeof(kAppStateDiffHarnessInvariantIds1[0]),
   kAppStateDiffHarnessGenerationDomainIds1,
   sizeof(kAppStateDiffHarnessGenerationDomainIds1) / sizeof(kAppStateDiffHarnessGenerationDomainIds1[0]),
   "The after snapshot may differ from the before snapshot only in owner fields named by the transition's declared_write_set or by an explicitly registered follow-up transition.",
   "A changed owner field outside the registered write set is rejected as an undeclared AppState mutation.",
   "covered_by_runtime_registry",
   kAppStateDiffHarnessMigrationNotes1,
   sizeof(kAppStateDiffHarnessMigrationNotes1) / sizeof(kAppStateDiffHarnessMigrationNotes1[0])},
  {"harness.render-projection-read-only-diff",
   "render_projection_read_only_diff",
   kAppStateDiffHarnessSnapshotPhases2,
   sizeof(kAppStateDiffHarnessSnapshotPhases2) / sizeof(kAppStateDiffHarnessSnapshotPhases2[0]),
   kAppStateDiffHarnessSnapshotRegions2,
   sizeof(kAppStateDiffHarnessSnapshotRegions2) / sizeof(kAppStateDiffHarnessSnapshotRegions2[0]),
   kAppStateDiffHarnessTransitionIds2,
   sizeof(kAppStateDiffHarnessTransitionIds2) / sizeof(kAppStateDiffHarnessTransitionIds2[0]),
   kAppStateDiffHarnessOwnerFieldRefs2,
   sizeof(kAppStateDiffHarnessOwnerFieldRefs2) / sizeof(kAppStateDiffHarnessOwnerFieldRefs2[0]),
   kAppStateDiffHarnessInvariantIds2,
   sizeof(kAppStateDiffHarnessInvariantIds2) / sizeof(kAppStateDiffHarnessInvariantIds2[0]),
   kAppStateDiffHarnessGenerationDomainIds2,
   sizeof(kAppStateDiffHarnessGenerationDomainIds2) / sizeof(kAppStateDiffHarnessGenerationDomainIds2[0]),
   "Render/reflow projection may consume settled AppState and stage terminal output, but it must not mutate authoritative selection, identity, focus, or generation fields.",
   "Any authoritative owner or generation delta produced by render projection is rejected as render-state leakage into AppState authority.",
   "covered_by_runtime_registry",
   kAppStateDiffHarnessMigrationNotes2,
   sizeof(kAppStateDiffHarnessMigrationNotes2) / sizeof(kAppStateDiffHarnessMigrationNotes2[0])},
  {"harness.generation-mismatch-check",
   "generation_mismatch_check",
   kAppStateDiffHarnessSnapshotPhases3,
   sizeof(kAppStateDiffHarnessSnapshotPhases3) / sizeof(kAppStateDiffHarnessSnapshotPhases3[0]),
   kAppStateDiffHarnessSnapshotRegions3,
   sizeof(kAppStateDiffHarnessSnapshotRegions3) / sizeof(kAppStateDiffHarnessSnapshotRegions3[0]),
   kAppStateDiffHarnessTransitionIds3,
   sizeof(kAppStateDiffHarnessTransitionIds3) / sizeof(kAppStateDiffHarnessTransitionIds3[0]),
   kAppStateDiffHarnessOwnerFieldRefs3,
   sizeof(kAppStateDiffHarnessOwnerFieldRefs3) / sizeof(kAppStateDiffHarnessOwnerFieldRefs3[0]),
   kAppStateDiffHarnessInvariantIds3,
   sizeof(kAppStateDiffHarnessInvariantIds3) / sizeof(kAppStateDiffHarnessInvariantIds3[0]),
   kAppStateDiffHarnessGenerationDomainIds3,
   sizeof(kAppStateDiffHarnessGenerationDomainIds3) / sizeof(kAppStateDiffHarnessGenerationDomainIds3[0]),
   "Saved panel or volume snapshots whose generation markers mismatch current authority must be rejected or rebound through stable identity fallback before any restore commit.",
   "Reusing stale row, pointer, or payload identity after a generation mismatch is rejected as a fail-closed violation.",
   "covered_by_runtime_registry",
   kAppStateDiffHarnessMigrationNotes3,
   sizeof(kAppStateDiffHarnessMigrationNotes3) / sizeof(kAppStateDiffHarnessMigrationNotes3[0])},
  {"harness.blocked-transition-no-unrelated-mutation",
   "blocked_transition_no_unrelated_mutation",
   kAppStateDiffHarnessSnapshotPhases4,
   sizeof(kAppStateDiffHarnessSnapshotPhases4) / sizeof(kAppStateDiffHarnessSnapshotPhases4[0]),
   kAppStateDiffHarnessSnapshotRegions4,
   sizeof(kAppStateDiffHarnessSnapshotRegions4) / sizeof(kAppStateDiffHarnessSnapshotRegions4[0]),
   kAppStateDiffHarnessTransitionIds4,
   sizeof(kAppStateDiffHarnessTransitionIds4) / sizeof(kAppStateDiffHarnessTransitionIds4[0]),
   kAppStateDiffHarnessOwnerFieldRefs4,
   sizeof(kAppStateDiffHarnessOwnerFieldRefs4) / sizeof(kAppStateDiffHarnessOwnerFieldRefs4[0]),
   kAppStateDiffHarnessInvariantIds4,
   sizeof(kAppStateDiffHarnessInvariantIds4) / sizeof(kAppStateDiffHarnessInvariantIds4[0]),
   kAppStateDiffHarnessGenerationDomainIds4,
   sizeof(kAppStateDiffHarnessGenerationDomainIds4) / sizeof(kAppStateDiffHarnessGenerationDomainIds4[0]),
   "A blocked transition may report only the registered user-visible failure state and must leave unrelated owner fields and all unaffected generations byte-for-byte unchanged.",
   "Any unrelated owner-field or generation delta after a blocked result is rejected as nondeterministic blocked-transition mutation.",
   "covered_by_runtime_registry",
   kAppStateDiffHarnessMigrationNotes4,
   sizeof(kAppStateDiffHarnessMigrationNotes4) / sizeof(kAppStateDiffHarnessMigrationNotes4[0])},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds0_0[] = {
  "invariant.inactive-panel-frozen",
  "invariant.blocked-transition-determinism",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds0_0[] = {
  "harness.transition-before-after-snapshot",
  "harness.declared-write-set-diff",
};

static const char *const kAppStateTransitionSequenceStepActionCoverageRefs0_0[] = {
  "ACTION_SPLIT_SCREEN",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations0_0[] = {
  {"generation.panel.local-authority", "Validate panel generation before and after the step; advance only when the transition declares panel-local writes."},
  {"shape.panel.focus", "Preserve or rebind focus shape only through the transition result."},
  {"reflow.layout.projection", "Layout reflow generation is projection-only unless resize transition declares it."},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds0_1[] = {
  "invariant.blocked-transition-determinism",
  "invariant.inactive-panel-frozen",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds0_1[] = {
  "harness.blocked-transition-no-unrelated-mutation",
};

static const char *const kAppStateTransitionSequenceStepActionCoverageRefs0_1[] = {
  "ACTION_SPLIT_SCREEN",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations0_1[] = {
  {"generation.panel.local-authority", "Validate panel generation before and after the step; advance only when the transition declares panel-local writes."},
  {"reflow.layout.projection", "Layout reflow generation is projection-only unless resize transition declares it."},
};

static const AppStateTransitionSequenceNoUnrelatedMutationMetadata kAppStateTransitionSequenceStepNoUnrelatedMutation0_1 = {"harness.blocked-transition-no-unrelated-mutation", "The blocked/invalid result may update only the declared diagnostic or dirty state and must not mutate unrelated owner fields."};

static const AppStateTransitionSequenceStepMetadata kAppStateTransitionSequenceSteps0[] = {
  {1,
   "split-open",
   "transition.keybinding.navigate-tree",
   "ACTION_SPLIT_SCREEN",
   NULL,
   kAppStateTransitionSequenceStepActionCoverageRefs0_0,
   sizeof(kAppStateTransitionSequenceStepActionCoverageRefs0_0) / sizeof(kAppStateTransitionSequenceStepActionCoverageRefs0_0[0]),
   NULL,
   0,
   "allowed",
   kAppStateTransitionSequenceStepInvariantIds0_0,
   sizeof(kAppStateTransitionSequenceStepInvariantIds0_0) / sizeof(kAppStateTransitionSequenceStepInvariantIds0_0[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds0_0,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds0_0) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds0_0[0]),
   kAppStateTransitionSequenceStepGenerationExpectations0_0,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations0_0) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations0_0[0]),
   NULL,
   NULL,
   NULL},
  {2,
   "split-toggle-blocked-by-modal",
   "transition.keybinding.navigate-tree",
   "ACTION_SPLIT_SCREEN",
   NULL,
   kAppStateTransitionSequenceStepActionCoverageRefs0_1,
   sizeof(kAppStateTransitionSequenceStepActionCoverageRefs0_1) / sizeof(kAppStateTransitionSequenceStepActionCoverageRefs0_1[0]),
   NULL,
   0,
   "blocked",
   kAppStateTransitionSequenceStepInvariantIds0_1,
   sizeof(kAppStateTransitionSequenceStepInvariantIds0_1) / sizeof(kAppStateTransitionSequenceStepInvariantIds0_1[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds0_1,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds0_1) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds0_1[0]),
   kAppStateTransitionSequenceStepGenerationExpectations0_1,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations0_1) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations0_1[0]),
   &kAppStateTransitionSequenceStepNoUnrelatedMutation0_1,
   NULL,
   NULL},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds1_0[] = {
  "invariant.inactive-panel-frozen",
  "invariant.panel-local-focus-restore",
  "invariant.shared-state-panel-local-isolation",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds1_0[] = {
  "harness.transition-before-after-snapshot",
  "harness.declared-write-set-diff",
};

static const char *const kAppStateTransitionSequenceStepActionCoverageRefs1_0[] = {
  "ACTION_SWITCH_PANEL",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations1_0[] = {
  {"generation.panel.local-authority", "Validate panel generation before and after the step; advance only when the transition declares panel-local writes."},
  {"shape.panel.focus", "Preserve or rebind focus shape only through the transition result."},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds1_1[] = {
  "invariant.inactive-panel-frozen",
  "invariant.panel-local-focus-restore",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds1_1[] = {
  "harness.transition-before-after-snapshot",
  "harness.declared-write-set-diff",
};

static const char *const kAppStateTransitionSequenceStepActionCoverageRefs1_1[] = {
  "ACTION_SWITCH_PANEL",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations1_1[] = {
  {"generation.panel.local-authority", "Validate panel generation before and after the step; advance only when the transition declares panel-local writes."},
  {"shape.panel.focus", "Preserve or rebind focus shape only through the transition result."},
};

static const AppStateTransitionSequenceStepMetadata kAppStateTransitionSequenceSteps1[] = {
  {1,
   "tab-to-inactive-panel",
   "transition.keybinding.navigate-tree",
   "ACTION_SWITCH_PANEL",
   NULL,
   kAppStateTransitionSequenceStepActionCoverageRefs1_0,
   sizeof(kAppStateTransitionSequenceStepActionCoverageRefs1_0) / sizeof(kAppStateTransitionSequenceStepActionCoverageRefs1_0[0]),
   NULL,
   0,
   "allowed",
   kAppStateTransitionSequenceStepInvariantIds1_0,
   sizeof(kAppStateTransitionSequenceStepInvariantIds1_0) / sizeof(kAppStateTransitionSequenceStepInvariantIds1_0[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds1_0,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds1_0) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds1_0[0]),
   kAppStateTransitionSequenceStepGenerationExpectations1_0,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations1_0) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations1_0[0]),
   NULL,
   NULL,
   NULL},
  {2,
   "tab-back-to-original-panel",
   "transition.keybinding.navigate-tree",
   "ACTION_SWITCH_PANEL",
   NULL,
   kAppStateTransitionSequenceStepActionCoverageRefs1_1,
   sizeof(kAppStateTransitionSequenceStepActionCoverageRefs1_1) / sizeof(kAppStateTransitionSequenceStepActionCoverageRefs1_1[0]),
   NULL,
   0,
   "allowed",
   kAppStateTransitionSequenceStepInvariantIds1_1,
   sizeof(kAppStateTransitionSequenceStepInvariantIds1_1) / sizeof(kAppStateTransitionSequenceStepInvariantIds1_1[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds1_1,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds1_1) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds1_1[0]),
   kAppStateTransitionSequenceStepGenerationExpectations1_1,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations1_1) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations1_1[0]),
   NULL,
   NULL,
   NULL},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds2_0[] = {
  "invariant.inactive-panel-frozen",
  "invariant.hidden-entry-visible-navigation",
  "invariant.viewport-identity-rebind",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds2_0[] = {
  "harness.transition-before-after-snapshot",
  "harness.declared-write-set-diff",
};

static const char *const kAppStateTransitionSequenceStepActionCoverageRefs2_0[] = {
  "ACTION_ENTER",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations2_0[] = {
  {"generation.panel.local-authority", "Validate panel generation before and after the step; advance only when the transition declares panel-local writes."},
  {"identity.directory.stable-key", "Directory identity remains durable across rebuild/rebind."},
  {"identity.file.stable-key", "File identity remains durable across payload or view-shape changes."},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds2_1[] = {
  "invariant.inactive-panel-frozen",
  "invariant.panel-local-focus-restore",
  "invariant.viewport-identity-rebind",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds2_1[] = {
  "harness.transition-before-after-snapshot",
  "harness.declared-write-set-diff",
};

static const char *const kAppStateTransitionSequenceStepActionCoverageRefs2_1[] = {
  "ACTION_TO_DIR",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations2_1[] = {
  {"generation.panel.local-authority", "Validate panel generation before and after the step; advance only when the transition declares panel-local writes."},
  {"identity.directory.stable-key", "Directory identity remains durable across rebuild/rebind."},
  {"identity.file.stable-key", "File identity remains durable across payload or view-shape changes."},
};

static const AppStateTransitionSequenceStepMetadata kAppStateTransitionSequenceSteps2[] = {
  {1,
   "enter-directory",
   "transition.keybinding.navigate-tree",
   "ACTION_ENTER",
   NULL,
   kAppStateTransitionSequenceStepActionCoverageRefs2_0,
   sizeof(kAppStateTransitionSequenceStepActionCoverageRefs2_0) / sizeof(kAppStateTransitionSequenceStepActionCoverageRefs2_0[0]),
   NULL,
   0,
   "allowed",
   kAppStateTransitionSequenceStepInvariantIds2_0,
   sizeof(kAppStateTransitionSequenceStepInvariantIds2_0) / sizeof(kAppStateTransitionSequenceStepInvariantIds2_0[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds2_0,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds2_0) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds2_0[0]),
   kAppStateTransitionSequenceStepGenerationExpectations2_0,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations2_0) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations2_0[0]),
   NULL,
   NULL,
   NULL},
  {2,
   "enter-file-target",
   "transition.keybinding.navigate-tree",
   "ACTION_TO_DIR",
   NULL,
   kAppStateTransitionSequenceStepActionCoverageRefs2_1,
   sizeof(kAppStateTransitionSequenceStepActionCoverageRefs2_1) / sizeof(kAppStateTransitionSequenceStepActionCoverageRefs2_1[0]),
   NULL,
   0,
   "allowed",
   kAppStateTransitionSequenceStepInvariantIds2_1,
   sizeof(kAppStateTransitionSequenceStepInvariantIds2_1) / sizeof(kAppStateTransitionSequenceStepInvariantIds2_1[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds2_1,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds2_1) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds2_1[0]),
   kAppStateTransitionSequenceStepGenerationExpectations2_1,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations2_1) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations2_1[0]),
   NULL,
   NULL,
   NULL},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds3_0[] = {
  "invariant.panel-local-focus-restore",
  "invariant.blocked-transition-determinism",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds3_0[] = {
  "harness.transition-before-after-snapshot",
  "harness.declared-write-set-diff",
};

static const char *const kAppStateTransitionSequenceStepActionCoverageRefs3_0[] = {
  "ACTION_ESCAPE",
};

static const char *const kAppStateTransitionSequenceStepEventCoverageRefs3_0[] = {
  "event.modal-completion",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations3_0[] = {
  {"target.modal-command.session", "Modal target generation validates before applying completion or dismissal."},
  {"generation.panel.local-authority", "Validate panel generation before and after the step; advance only when the transition declares panel-local writes."},
};

static const AppStateTransitionSequenceStepMetadata kAppStateTransitionSequenceSteps3[] = {
  {1,
   "active-modal-escape-dismiss",
   "transition.modal-action.dismiss",
   "ACTION_ESCAPE",
   NULL,
   kAppStateTransitionSequenceStepActionCoverageRefs3_0,
   sizeof(kAppStateTransitionSequenceStepActionCoverageRefs3_0) / sizeof(kAppStateTransitionSequenceStepActionCoverageRefs3_0[0]),
   NULL,
   0,
   "allowed",
   kAppStateTransitionSequenceStepInvariantIds3_0,
   sizeof(kAppStateTransitionSequenceStepInvariantIds3_0) / sizeof(kAppStateTransitionSequenceStepInvariantIds3_0[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds3_0,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds3_0) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds3_0[0]),
   kAppStateTransitionSequenceStepGenerationExpectations3_0,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations3_0) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations3_0[0]),
   NULL,
   NULL,
   NULL},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds4_0[] = {
  "invariant.inactive-panel-frozen",
  "invariant.hidden-entry-visible-navigation",
  "invariant.panel-local-focus-restore",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds4_0[] = {
  "harness.transition-before-after-snapshot",
  "harness.declared-write-set-diff",
};

static const char *const kAppStateTransitionSequenceStepActionCoverageRefs4_0[] = {
  "ACTION_TOGGLE_HIDDEN",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations4_0[] = {
  {"generation.panel.local-authority", "Validate panel generation before and after the step; advance only when the transition declares panel-local writes."},
  {"state.visibility-filter.panel-volume", "Visibility/filter generation matches the selected panel after the transition."},
  {"identity.directory.stable-key", "Directory identity remains durable across rebuild/rebind."},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds4_1[] = {
  "invariant.inactive-panel-frozen",
  "invariant.hidden-entry-visible-navigation",
  "invariant.viewport-identity-rebind",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds4_1[] = {
  "harness.transition-before-after-snapshot",
  "harness.declared-write-set-diff",
};

static const char *const kAppStateTransitionSequenceStepActionCoverageRefs4_1[] = {
  "ACTION_TOGGLE_HIDDEN",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations4_1[] = {
  {"generation.panel.local-authority", "Validate panel generation before and after the step; advance only when the transition declares panel-local writes."},
  {"state.visibility-filter.panel-volume", "Visibility/filter generation matches the selected panel after the transition."},
  {"identity.directory.stable-key", "Directory identity remains durable across rebuild/rebind."},
};

static const AppStateTransitionSequenceStepMetadata kAppStateTransitionSequenceSteps4[] = {
  {1,
   "reveal-dotfiles",
   "transition.keybinding.navigate-tree",
   "ACTION_TOGGLE_HIDDEN",
   NULL,
   kAppStateTransitionSequenceStepActionCoverageRefs4_0,
   sizeof(kAppStateTransitionSequenceStepActionCoverageRefs4_0) / sizeof(kAppStateTransitionSequenceStepActionCoverageRefs4_0[0]),
   NULL,
   0,
   "allowed",
   kAppStateTransitionSequenceStepInvariantIds4_0,
   sizeof(kAppStateTransitionSequenceStepInvariantIds4_0) / sizeof(kAppStateTransitionSequenceStepInvariantIds4_0[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds4_0,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds4_0) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds4_0[0]),
   kAppStateTransitionSequenceStepGenerationExpectations4_0,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations4_0) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations4_0[0]),
   NULL,
   NULL,
   NULL},
  {2,
   "conceal-dotfiles",
   "transition.keybinding.navigate-tree",
   "ACTION_TOGGLE_HIDDEN",
   NULL,
   kAppStateTransitionSequenceStepActionCoverageRefs4_1,
   sizeof(kAppStateTransitionSequenceStepActionCoverageRefs4_1) / sizeof(kAppStateTransitionSequenceStepActionCoverageRefs4_1[0]),
   NULL,
   0,
   "allowed",
   kAppStateTransitionSequenceStepInvariantIds4_1,
   sizeof(kAppStateTransitionSequenceStepInvariantIds4_1) / sizeof(kAppStateTransitionSequenceStepInvariantIds4_1[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds4_1,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds4_1) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds4_1[0]),
   kAppStateTransitionSequenceStepGenerationExpectations4_1,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations4_1) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations4_1[0]),
   NULL,
   NULL,
   NULL},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds5_0[] = {
  "invariant.hidden-entry-visible-navigation",
  "invariant.viewport-identity-rebind",
  "invariant.shared-state-panel-local-isolation",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds5_0[] = {
  "harness.transition-before-after-snapshot",
  "harness.generation-mismatch-check",
};

static const char *const kAppStateTransitionSequenceStepActionCoverageRefs5_0[] = {
  "ACTION_REFRESH",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations5_0[] = {
  {"generation.panel.local-authority", "Validate panel generation before and after the step; advance only when the transition declares panel-local writes."},
  {"identity.directory.stable-key", "Directory identity remains durable across rebuild/rebind."},
  {"identity.file.stable-key", "File identity remains durable across payload or view-shape changes."},
  {"generation.volume.shared-authority", "Volume generation advances only for declared shared-volume mutations."},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds5_1[] = {
  "invariant.stale-snapshot-fail-closed",
  "invariant.hidden-entry-visible-navigation",
  "invariant.viewport-identity-rebind",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds5_1[] = {
  "harness.generation-mismatch-check",
  "harness.blocked-transition-no-unrelated-mutation",
};

static const char *const kAppStateTransitionSequenceStepEventCoverageRefs5_1[] = {
  "event.rebuild-rebind-callback",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations5_1[] = {
  {"generation.panel.local-authority", "Validate panel generation before and after the step; advance only when the transition declares panel-local writes."},
  {"identity.directory.stable-key", "Directory identity remains durable across rebuild/rebind."},
  {"identity.file.stable-key", "File identity remains durable across payload or view-shape changes."},
};

static const AppStateTransitionSequenceNoUnrelatedMutationMetadata kAppStateTransitionSequenceStepNoUnrelatedMutation5_1 = {"harness.generation-mismatch-check", "Fallback/stale-snapshot/generation-mismatch handling may mutate only the declared transition fields and must leave unrelated owner fields unchanged."};

static const AppStateTransitionSequenceDeterministicFallbackMetadata kAppStateTransitionSequenceStepDeterministicFallback5_1 = {"Fail closed to the nearest valid durable identity or preserve the prior valid selection without using stale rows.", "Only the registered fallback/no-op result may run; unrelated owner fields remain unchanged."};

static const char *const kAppStateTransitionSequenceStepEventCoverageRefs16_0[] = {
  "event.watcher-live-refresh",
};

static const AppStateTransitionSequenceStepMetadata kAppStateTransitionSequenceSteps5[] = {
  {1,
   "manual-refresh",
   "transition.refresh-rebuild.manual-refresh",
   "ACTION_REFRESH",
   NULL,
   kAppStateTransitionSequenceStepActionCoverageRefs5_0,
   sizeof(kAppStateTransitionSequenceStepActionCoverageRefs5_0) / sizeof(kAppStateTransitionSequenceStepActionCoverageRefs5_0[0]),
   NULL,
   0,
   "allowed",
   kAppStateTransitionSequenceStepInvariantIds5_0,
   sizeof(kAppStateTransitionSequenceStepInvariantIds5_0) / sizeof(kAppStateTransitionSequenceStepInvariantIds5_0[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds5_0,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds5_0) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds5_0[0]),
   kAppStateTransitionSequenceStepGenerationExpectations5_0,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations5_0) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations5_0[0]),
   NULL,
   NULL,
   NULL},
  {2,
   "stale-snapshot-rebind",
   "transition.rebuild-rebind-callback.panel-anchor",
   NULL,
   "event.rebuild-rebind-callback",
   NULL,
   0,
   kAppStateTransitionSequenceStepEventCoverageRefs5_1,
   sizeof(kAppStateTransitionSequenceStepEventCoverageRefs5_1) / sizeof(kAppStateTransitionSequenceStepEventCoverageRefs5_1[0]),
   "fallback",
   kAppStateTransitionSequenceStepInvariantIds5_1,
   sizeof(kAppStateTransitionSequenceStepInvariantIds5_1) / sizeof(kAppStateTransitionSequenceStepInvariantIds5_1[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds5_1,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds5_1) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds5_1[0]),
   kAppStateTransitionSequenceStepGenerationExpectations5_1,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations5_1) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations5_1[0]),
   &kAppStateTransitionSequenceStepNoUnrelatedMutation5_1,
   "stale_snapshot",
   &kAppStateTransitionSequenceStepDeterministicFallback5_1},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds6_0[] = {
  "invariant.blocked-transition-determinism",
  "invariant.viewport-identity-rebind",
  "invariant.shared-state-panel-local-isolation",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds6_0[] = {
  "harness.transition-before-after-snapshot",
  "harness.generation-mismatch-check",
};

static const char *const kAppStateTransitionSequenceStepEventCoverageRefs6_0[] = {
  "event.filesystem-mutation-result",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations6_0[] = {
  {"generation.volume.shared-authority", "Volume generation advances only for declared shared-volume mutations."},
  {"state.topology.volume", "Topology generation advances only for declared topology changes."},
  {"state.file-payload.volume", "File payload generation advances only for declared payload rebuilds."},
  {"identity.directory.stable-key", "Directory identity remains durable across rebuild/rebind."},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds6_1[] = {
  "invariant.stale-snapshot-fail-closed",
  "invariant.blocked-transition-determinism",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds6_1[] = {
  "harness.generation-mismatch-check",
  "harness.blocked-transition-no-unrelated-mutation",
};

static const char *const kAppStateTransitionSequenceStepActionCoverageRefs6_1[] = {
  "ACTION_CMD_D",
};

static const char *const kAppStateTransitionSequenceStepEventCoverageRefs6_1[] = {
  "event.filesystem-mutation-result",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations6_1[] = {
  {"generation.volume.shared-authority", "Volume generation advances only for declared shared-volume mutations."},
  {"state.topology.volume", "Topology generation advances only for declared topology changes."},
  {"state.file-payload.volume", "File payload generation advances only for declared payload rebuilds."},
};

static const AppStateTransitionSequenceNoUnrelatedMutationMetadata kAppStateTransitionSequenceStepNoUnrelatedMutation6_1 = {"harness.generation-mismatch-check", "Fallback/stale-snapshot/generation-mismatch handling may mutate only the declared transition fields and must leave unrelated owner fields unchanged."};

static const AppStateTransitionSequenceDeterministicFallbackMetadata kAppStateTransitionSequenceStepDeterministicFallback6_1 = {"Reject stale mutation result, request a registered refresh/rebind, and keep unrelated panel state unchanged.", "Only the registered fallback/no-op result may run; unrelated owner fields remain unchanged."};

static const AppStateTransitionSequenceStepMetadata kAppStateTransitionSequenceSteps6[] = {
  {1,
   "delete-or-mkdir-result",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   NULL,
   "event.filesystem-mutation-result",
   NULL,
   0,
   kAppStateTransitionSequenceStepEventCoverageRefs6_0,
   sizeof(kAppStateTransitionSequenceStepEventCoverageRefs6_0) / sizeof(kAppStateTransitionSequenceStepEventCoverageRefs6_0[0]),
   "allowed",
   kAppStateTransitionSequenceStepInvariantIds6_0,
   sizeof(kAppStateTransitionSequenceStepInvariantIds6_0) / sizeof(kAppStateTransitionSequenceStepInvariantIds6_0[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds6_0,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds6_0) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds6_0[0]),
   kAppStateTransitionSequenceStepGenerationExpectations6_0,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations6_0) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations6_0[0]),
   NULL,
   NULL,
   NULL},
  {2,
   "mutation-generation-mismatch",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "ACTION_CMD_D",
   "event.filesystem-mutation-result",
   kAppStateTransitionSequenceStepActionCoverageRefs6_1,
   sizeof(kAppStateTransitionSequenceStepActionCoverageRefs6_1) / sizeof(kAppStateTransitionSequenceStepActionCoverageRefs6_1[0]),
   kAppStateTransitionSequenceStepEventCoverageRefs6_1,
   sizeof(kAppStateTransitionSequenceStepEventCoverageRefs6_1) / sizeof(kAppStateTransitionSequenceStepEventCoverageRefs6_1[0]),
   "fallback",
   kAppStateTransitionSequenceStepInvariantIds6_1,
   sizeof(kAppStateTransitionSequenceStepInvariantIds6_1) / sizeof(kAppStateTransitionSequenceStepInvariantIds6_1[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds6_1,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds6_1) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds6_1[0]),
   kAppStateTransitionSequenceStepGenerationExpectations6_1,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations6_1) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations6_1[0]),
   &kAppStateTransitionSequenceStepNoUnrelatedMutation6_1,
   "generation_mismatch",
   &kAppStateTransitionSequenceStepDeterministicFallback6_1},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds7_0[] = {
  "invariant.inactive-panel-frozen",
  "invariant.hidden-entry-visible-navigation",
  "invariant.panel-local-focus-restore",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds7_0[] = {
  "harness.transition-before-after-snapshot",
  "harness.declared-write-set-diff",
};

static const char *const kAppStateTransitionSequenceStepActionCoverageRefs7_0[] = {
  "ACTION_LIST_JUMP",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations7_0[] = {
  {"generation.panel.local-authority", "Validate panel generation before and after the step; advance only when the transition declares panel-local writes."},
  {"identity.directory.stable-key", "Directory identity remains durable across rebuild/rebind."},
  {"identity.file.stable-key", "File identity remains durable across payload or view-shape changes."},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds7_1[] = {
  "invariant.panel-local-focus-restore",
  "invariant.blocked-transition-determinism",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds7_1[] = {
  "harness.transition-before-after-snapshot",
  "harness.declared-write-set-diff",
};

static const char *const kAppStateTransitionSequenceStepEventCoverageRefs7_1[] = {
  "event.command-completion",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations7_1[] = {
  {"target.modal-command.session", "Modal target generation validates before applying completion or dismissal."},
  {"generation.panel.local-authority", "Validate panel generation before and after the step; advance only when the transition declares panel-local writes."},
};

static const AppStateTransitionSequenceStepMetadata kAppStateTransitionSequenceSteps7[] = {
  {1,
   "list-jump-visible-target",
   "transition.keybinding.navigate-tree",
   "ACTION_LIST_JUMP",
   NULL,
   kAppStateTransitionSequenceStepActionCoverageRefs7_0,
   sizeof(kAppStateTransitionSequenceStepActionCoverageRefs7_0) / sizeof(kAppStateTransitionSequenceStepActionCoverageRefs7_0[0]),
   NULL,
   0,
   "allowed",
   kAppStateTransitionSequenceStepInvariantIds7_0,
   sizeof(kAppStateTransitionSequenceStepInvariantIds7_0) / sizeof(kAppStateTransitionSequenceStepInvariantIds7_0[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds7_0,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds7_0) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds7_0[0]),
   kAppStateTransitionSequenceStepGenerationExpectations7_0,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations7_0) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations7_0[0]),
   NULL,
   NULL,
   NULL},
  {2,
   "search-command-completion",
   "transition.command-completion.user-command",
   NULL,
   "event.command-completion",
   NULL,
   0,
   kAppStateTransitionSequenceStepEventCoverageRefs7_1,
   sizeof(kAppStateTransitionSequenceStepEventCoverageRefs7_1) / sizeof(kAppStateTransitionSequenceStepEventCoverageRefs7_1[0]),
   "allowed",
   kAppStateTransitionSequenceStepInvariantIds7_1,
   sizeof(kAppStateTransitionSequenceStepInvariantIds7_1) / sizeof(kAppStateTransitionSequenceStepInvariantIds7_1[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds7_1,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds7_1) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds7_1[0]),
   kAppStateTransitionSequenceStepGenerationExpectations7_1,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations7_1) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations7_1[0]),
   NULL,
   NULL,
   NULL},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds8_0[] = {
  "invariant.inactive-panel-frozen",
  "invariant.shared-state-panel-local-isolation",
  "invariant.hidden-entry-visible-navigation",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds8_0[] = {
  "harness.transition-before-after-snapshot",
  "harness.declared-write-set-diff",
};

static const char *const kAppStateTransitionSequenceStepActionCoverageRefs8_0[] = {
  "ACTION_TOGGLE_TAGGED_MODE",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations8_0[] = {
  {"generation.panel.local-authority", "Validate panel generation before and after the step; advance only when the transition declares panel-local writes."},
  {"state.visibility-filter.panel-volume", "Visibility/filter generation matches the selected panel after the transition."},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds8_1[] = {
  "invariant.inactive-panel-frozen",
  "invariant.shared-state-panel-local-isolation",
  "invariant.hidden-entry-visible-navigation",
  "invariant.render-projection-read-only",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds8_1[] = {
  "harness.declared-write-set-diff",
  "harness.render-projection-read-only-diff",
};

static const char *const kAppStateTransitionSequenceStepActionCoverageRefs8_1[] = {
  "ACTION_ASTERISK",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations8_1[] = {
  {"generation.panel.local-authority", "Validate panel generation before and after the step; advance only when the transition declares panel-local writes."},
  {"state.visibility-filter.panel-volume", "Visibility/filter generation matches the selected panel after the transition."},
  {"generation.volume.shared-authority", "Volume generation advances only for declared shared-volume mutations."},
};

static const AppStateTransitionSequenceStepMetadata kAppStateTransitionSequenceSteps8[] = {
  {1,
   "toggle-tagged-only",
   "transition.keybinding.navigate-tree",
   "ACTION_TOGGLE_TAGGED_MODE",
   NULL,
   kAppStateTransitionSequenceStepActionCoverageRefs8_0,
   sizeof(kAppStateTransitionSequenceStepActionCoverageRefs8_0) / sizeof(kAppStateTransitionSequenceStepActionCoverageRefs8_0[0]),
   NULL,
   0,
   "allowed",
   kAppStateTransitionSequenceStepInvariantIds8_0,
   sizeof(kAppStateTransitionSequenceStepInvariantIds8_0) / sizeof(kAppStateTransitionSequenceStepInvariantIds8_0[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds8_0,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds8_0) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds8_0[0]),
   kAppStateTransitionSequenceStepGenerationExpectations8_0,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations8_0) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations8_0[0]),
   NULL,
   NULL,
   NULL},
  {2,
   "toggle-showall-global-projection",
   "transition.keybinding.navigate-tree",
   "ACTION_ASTERISK",
   NULL,
   kAppStateTransitionSequenceStepActionCoverageRefs8_1,
   sizeof(kAppStateTransitionSequenceStepActionCoverageRefs8_1) / sizeof(kAppStateTransitionSequenceStepActionCoverageRefs8_1[0]),
   NULL,
   0,
   "allowed",
   kAppStateTransitionSequenceStepInvariantIds8_1,
   sizeof(kAppStateTransitionSequenceStepInvariantIds8_1) / sizeof(kAppStateTransitionSequenceStepInvariantIds8_1[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds8_1,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds8_1) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds8_1[0]),
   kAppStateTransitionSequenceStepGenerationExpectations8_1,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations8_1) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations8_1[0]),
   NULL,
   NULL,
   NULL},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds9_0[] = {
  "invariant.inactive-panel-frozen",
  "invariant.panel-local-focus-restore",
  "invariant.render-projection-read-only",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds9_0[] = {
  "harness.declared-write-set-diff",
  "harness.render-projection-read-only-diff",
};

static const char *const kAppStateTransitionSequenceStepActionCoverageRefs9_0[] = {
  "ACTION_TOGGLE_MODE",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations9_0[] = {
  {"generation.panel.local-authority", "Validate panel generation before and after the step; advance only when the transition declares panel-local writes."},
  {"identity.file.stable-key", "File identity remains durable across payload or view-shape changes."},
  {"shape.panel.focus", "Preserve or rebind focus shape only through the transition result."},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds9_1[] = {
  "invariant.inactive-panel-frozen",
  "invariant.panel-local-focus-restore",
  "invariant.render-projection-read-only",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds9_1[] = {
  "harness.declared-write-set-diff",
  "harness.render-projection-read-only-diff",
};

static const char *const kAppStateTransitionSequenceStepActionCoverageRefs9_1[] = {
  "ACTION_VIEW_PREVIEW",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations9_1[] = {
  {"generation.panel.local-authority", "Validate panel generation before and after the step; advance only when the transition declares panel-local writes."},
  {"identity.file.stable-key", "File identity remains durable across payload or view-shape changes."},
  {"reflow.layout.projection", "Layout reflow generation is projection-only unless resize transition declares it."},
};

static const AppStateTransitionSequenceStepMetadata kAppStateTransitionSequenceSteps9[] = {
  {1,
   "toggle-file-mode",
   "transition.keybinding.navigate-tree",
   "ACTION_TOGGLE_MODE",
   NULL,
   kAppStateTransitionSequenceStepActionCoverageRefs9_0,
   sizeof(kAppStateTransitionSequenceStepActionCoverageRefs9_0) / sizeof(kAppStateTransitionSequenceStepActionCoverageRefs9_0[0]),
   NULL,
   0,
   "allowed",
   kAppStateTransitionSequenceStepInvariantIds9_0,
   sizeof(kAppStateTransitionSequenceStepInvariantIds9_0) / sizeof(kAppStateTransitionSequenceStepInvariantIds9_0[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds9_0,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds9_0) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds9_0[0]),
   kAppStateTransitionSequenceStepGenerationExpectations9_0,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations9_0) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations9_0[0]),
   NULL,
   NULL,
   NULL},
  {2,
   "view-preview-shape",
   "transition.keybinding.navigate-tree",
   "ACTION_VIEW_PREVIEW",
   NULL,
   kAppStateTransitionSequenceStepActionCoverageRefs9_1,
   sizeof(kAppStateTransitionSequenceStepActionCoverageRefs9_1) / sizeof(kAppStateTransitionSequenceStepActionCoverageRefs9_1[0]),
   NULL,
   0,
   "allowed",
   kAppStateTransitionSequenceStepInvariantIds9_1,
   sizeof(kAppStateTransitionSequenceStepInvariantIds9_1) / sizeof(kAppStateTransitionSequenceStepInvariantIds9_1[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds9_1,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds9_1) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds9_1[0]),
   kAppStateTransitionSequenceStepGenerationExpectations9_1,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations9_1) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations9_1[0]),
   NULL,
   NULL,
   NULL},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds10_0[] = {
  "invariant.shared-state-panel-local-isolation",
  "invariant.viewport-identity-rebind",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds10_0[] = {
  "harness.transition-before-after-snapshot",
  "harness.declared-write-set-diff",
};

static const char *const kAppStateTransitionSequenceStepActionCoverageRefs10_0[] = {
  "ACTION_VOL_NEXT",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations10_0[] = {
  {"generation.panel.local-authority", "Validate panel generation before and after the step; advance only when the transition declares panel-local writes."},
  {"generation.volume.shared-authority", "Volume generation advances only for declared shared-volume mutations."},
  {"lifecycle.volume.registry", "Volume lifecycle generation validates before rebinding panels."},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds10_1[] = {
  "invariant.stale-snapshot-fail-closed",
  "invariant.shared-state-panel-local-isolation",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds10_1[] = {
  "harness.transition-before-after-snapshot",
  "harness.generation-mismatch-check",
};

static const char *const kAppStateTransitionSequenceStepEventCoverageRefs10_1[] = {
  "event.volume-lifecycle",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations10_1[] = {
  {"generation.volume.shared-authority", "Volume generation advances only for declared shared-volume mutations."},
  {"lifecycle.volume.registry", "Volume lifecycle generation validates before rebinding panels."},
  {"state.topology.volume", "Topology generation advances only for declared topology changes."},
};

static const AppStateTransitionSequenceStepMetadata kAppStateTransitionSequenceSteps10[] = {
  {1,
   "volume-next",
   "transition.volume-operation.release-cycle",
   "ACTION_VOL_NEXT",
   NULL,
   kAppStateTransitionSequenceStepActionCoverageRefs10_0,
   sizeof(kAppStateTransitionSequenceStepActionCoverageRefs10_0) / sizeof(kAppStateTransitionSequenceStepActionCoverageRefs10_0[0]),
   NULL,
   0,
   "allowed",
   kAppStateTransitionSequenceStepInvariantIds10_0,
   sizeof(kAppStateTransitionSequenceStepInvariantIds10_0) / sizeof(kAppStateTransitionSequenceStepInvariantIds10_0[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds10_0,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds10_0) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds10_0[0]),
   kAppStateTransitionSequenceStepGenerationExpectations10_0,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations10_0) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations10_0[0]),
   NULL,
   NULL,
   NULL},
  {2,
   "volume-release-event",
   "transition.volume-operation.release-cycle",
   NULL,
   "event.volume-lifecycle",
   NULL,
   0,
   kAppStateTransitionSequenceStepEventCoverageRefs10_1,
   sizeof(kAppStateTransitionSequenceStepEventCoverageRefs10_1) / sizeof(kAppStateTransitionSequenceStepEventCoverageRefs10_1[0]),
   "allowed",
   kAppStateTransitionSequenceStepInvariantIds10_1,
   sizeof(kAppStateTransitionSequenceStepInvariantIds10_1) / sizeof(kAppStateTransitionSequenceStepInvariantIds10_1[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds10_1,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds10_1) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds10_1[0]),
   kAppStateTransitionSequenceStepGenerationExpectations10_1,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations10_1) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations10_1[0]),
   NULL,
   NULL,
   NULL},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds11_0[] = {
  "invariant.inactive-panel-frozen",
  "invariant.panel-local-focus-restore",
  "invariant.render-projection-read-only",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds11_0[] = {
  "harness.declared-write-set-diff",
  "harness.render-projection-read-only-diff",
};

static const char *const kAppStateTransitionSequenceStepActionCoverageRefs11_0[] = {
  "ACTION_SPLIT_SCREEN",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations11_0[] = {
  {"generation.panel.local-authority", "Validate panel generation before and after the step; advance only when the transition declares panel-local writes."},
  {"shape.panel.focus", "Preserve or rebind focus shape only through the transition result."},
  {"reflow.layout.projection", "Layout reflow generation is projection-only unless resize transition declares it."},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds11_1[] = {
  "invariant.inactive-panel-frozen",
  "invariant.panel-local-focus-restore",
  "invariant.viewport-identity-rebind",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds11_1[] = {
  "harness.transition-before-after-snapshot",
  "harness.declared-write-set-diff",
};

static const char *const kAppStateTransitionSequenceStepActionCoverageRefs11_1[] = {
  "ACTION_SPLIT_SCREEN",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations11_1[] = {
  {"generation.panel.local-authority", "Validate panel generation before and after the step; advance only when the transition declares panel-local writes."},
  {"shape.panel.focus", "Preserve or rebind focus shape only through the transition result."},
  {"reflow.layout.projection", "Layout reflow generation is projection-only unless resize transition declares it."},
};

static const AppStateTransitionSequenceStepMetadata kAppStateTransitionSequenceSteps11[] = {
  {1,
   "split-close",
   "transition.keybinding.navigate-tree",
   "ACTION_SPLIT_SCREEN",
   NULL,
   kAppStateTransitionSequenceStepActionCoverageRefs11_0,
   sizeof(kAppStateTransitionSequenceStepActionCoverageRefs11_0) / sizeof(kAppStateTransitionSequenceStepActionCoverageRefs11_0[0]),
   NULL,
   0,
   "allowed",
   kAppStateTransitionSequenceStepInvariantIds11_0,
   sizeof(kAppStateTransitionSequenceStepInvariantIds11_0) / sizeof(kAppStateTransitionSequenceStepInvariantIds11_0[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds11_0,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds11_0) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds11_0[0]),
   kAppStateTransitionSequenceStepGenerationExpectations11_0,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations11_0) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations11_0[0]),
   NULL,
   NULL,
   NULL},
  {2,
   "split-reopen",
   "transition.keybinding.navigate-tree",
   "ACTION_SPLIT_SCREEN",
   NULL,
   kAppStateTransitionSequenceStepActionCoverageRefs11_1,
   sizeof(kAppStateTransitionSequenceStepActionCoverageRefs11_1) / sizeof(kAppStateTransitionSequenceStepActionCoverageRefs11_1[0]),
   NULL,
   0,
   "allowed",
   kAppStateTransitionSequenceStepInvariantIds11_1,
   sizeof(kAppStateTransitionSequenceStepInvariantIds11_1) / sizeof(kAppStateTransitionSequenceStepInvariantIds11_1[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds11_1,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds11_1) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds11_1[0]),
   kAppStateTransitionSequenceStepGenerationExpectations11_1,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations11_1) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations11_1[0]),
   NULL,
   NULL,
   NULL},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds12_0[] = {
  "invariant.render-projection-read-only",
  "invariant.viewport-identity-rebind",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds12_0[] = {
  "harness.generation-mismatch-check",
};

static const char *const kAppStateTransitionSequenceStepEventCoverageRefs12_0[] = {
  "event.terminal-resize-signal",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations12_0[] = {
  {"reflow.layout.projection", "Layout reflow generation changes only through the resize transition."},
  {"generation.panel.local-authority", "Panel generation validates before viewport rebind after resize."},
  {"identity.directory.stable-key", "Directory identity remains durable while viewport geometry is rebound."},
  {"identity.file.stable-key", "File identity remains durable while viewport geometry is rebound."},
};

static const AppStateTransitionSequenceStepMetadata kAppStateTransitionSequenceSteps12[] = {
  {1,
   "terminal-resize-event",
   "transition.terminal-signal-resize",
   NULL,
   "event.terminal-resize-signal",
   NULL,
   0,
   kAppStateTransitionSequenceStepEventCoverageRefs12_0,
   sizeof(kAppStateTransitionSequenceStepEventCoverageRefs12_0) / sizeof(kAppStateTransitionSequenceStepEventCoverageRefs12_0[0]),
   "allowed",
   kAppStateTransitionSequenceStepInvariantIds12_0,
   sizeof(kAppStateTransitionSequenceStepInvariantIds12_0) / sizeof(kAppStateTransitionSequenceStepInvariantIds12_0[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds12_0,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds12_0) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds12_0[0]),
   kAppStateTransitionSequenceStepGenerationExpectations12_0,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations12_0) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations12_0[0]),
   NULL,
   NULL,
   NULL},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds13_0[] = {
  "invariant.render-projection-read-only",
  "invariant.blocked-transition-determinism",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds13_0[] = {
  "harness.render-projection-read-only-diff",
};

static const char *const kAppStateTransitionSequenceStepEventCoverageRefs13_0[] = {
  "event.render-reflow",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations13_0[] = {
  {"reflow.layout.projection", "Render projection consumes layout reflow state without claiming owner authority."},
  {"generation.panel.local-authority", "Panel generation remains an input to projection, not a render-owned mutation."},
  {"generation.volume.shared-authority", "Volume generation remains an input to projection, not a render-owned mutation."},
};

static const AppStateTransitionSequenceStepMetadata kAppStateTransitionSequenceSteps13[] = {
  {1,
   "render-reflow-event",
   "transition.render-reflow.project-state",
   NULL,
   "event.render-reflow",
   NULL,
   0,
   kAppStateTransitionSequenceStepEventCoverageRefs13_0,
   sizeof(kAppStateTransitionSequenceStepEventCoverageRefs13_0) / sizeof(kAppStateTransitionSequenceStepEventCoverageRefs13_0[0]),
   "allowed",
   kAppStateTransitionSequenceStepInvariantIds13_0,
   sizeof(kAppStateTransitionSequenceStepInvariantIds13_0) / sizeof(kAppStateTransitionSequenceStepInvariantIds13_0[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds13_0,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds13_0) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds13_0[0]),
   kAppStateTransitionSequenceStepGenerationExpectations13_0,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations13_0) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations13_0[0]),
   NULL,
   NULL,
   NULL},
};

static const char *const kAppStateTransitionSequenceStepInvariantIds14_0[] = {
  "invariant.inactive-panel-frozen",
  "invariant.panel-local-focus-restore",
  "invariant.shared-state-panel-local-isolation",
};

static const char *const kAppStateTransitionSequenceStepDiffHarnessIds14_0[] = {
  "harness.declared-write-set-diff",
};

static const char *const kAppStateTransitionSequenceStepActionCoverageRefs14_0[] = {
  "ACTION_VOL_MENU",
};

static const AppStateTransitionSequenceGenerationExpectationMetadata kAppStateTransitionSequenceStepGenerationExpectations14_0[] = {
  {"generation.panel.local-authority", "Advance panel generation only when the selected loaded volume changes the active panel binding."},
  {"shape.panel.focus", "Restore focus shape from the selected volume's panel-local snapshot instead of cross-panel state."},
  {"lifecycle.volume.registry", "Read the loaded-volume registry without advancing volume generation during selection."},
};

static const AppStateTransitionSequenceStepMetadata kAppStateTransitionSequenceSteps14[] = {
  {1,
   "select-loaded-volume",
   "transition.menu-action.volume-select",
   "ACTION_VOL_MENU",
   NULL,
   kAppStateTransitionSequenceStepActionCoverageRefs14_0,
   sizeof(kAppStateTransitionSequenceStepActionCoverageRefs14_0) / sizeof(kAppStateTransitionSequenceStepActionCoverageRefs14_0[0]),
   NULL,
   0,
   "allowed",
   kAppStateTransitionSequenceStepInvariantIds14_0,
   sizeof(kAppStateTransitionSequenceStepInvariantIds14_0) / sizeof(kAppStateTransitionSequenceStepInvariantIds14_0[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds14_0,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds14_0) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds14_0[0]),
   kAppStateTransitionSequenceStepGenerationExpectations14_0,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations14_0) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations14_0[0]),
   NULL,
   NULL,
   NULL},
};

static const AppStateTransitionSequenceStepMetadata kAppStateTransitionSequenceSteps15[] = {
  {1,
   "modal-completion-settle",
   "transition.modal-action.completion",
   NULL,
   "event.modal-completion",
   NULL,
   0,
   kAppStateTransitionSequenceStepEventCoverageRefs3_0,
   sizeof(kAppStateTransitionSequenceStepEventCoverageRefs3_0) / sizeof(kAppStateTransitionSequenceStepEventCoverageRefs3_0[0]),
   "allowed",
   kAppStateTransitionSequenceStepInvariantIds3_0,
   sizeof(kAppStateTransitionSequenceStepInvariantIds3_0) / sizeof(kAppStateTransitionSequenceStepInvariantIds3_0[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds3_0,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds3_0) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds3_0[0]),
   kAppStateTransitionSequenceStepGenerationExpectations3_0,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations3_0) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations3_0[0]),
   NULL,
   NULL,
   NULL},
};

static const AppStateTransitionSequenceStepMetadata kAppStateTransitionSequenceSteps16[] = {
  {1,
   "watcher-live-refresh",
   "transition.refresh-rebuild.watcher-live-refresh",
   NULL,
   "event.watcher-live-refresh",
   NULL,
   0,
   kAppStateTransitionSequenceStepEventCoverageRefs16_0,
   sizeof(kAppStateTransitionSequenceStepEventCoverageRefs16_0) / sizeof(kAppStateTransitionSequenceStepEventCoverageRefs16_0[0]),
   "allowed",
   kAppStateTransitionSequenceStepInvariantIds5_0,
   sizeof(kAppStateTransitionSequenceStepInvariantIds5_0) / sizeof(kAppStateTransitionSequenceStepInvariantIds5_0[0]),
   kAppStateTransitionSequenceStepDiffHarnessIds5_0,
   sizeof(kAppStateTransitionSequenceStepDiffHarnessIds5_0) / sizeof(kAppStateTransitionSequenceStepDiffHarnessIds5_0[0]),
   kAppStateTransitionSequenceStepGenerationExpectations5_0,
   sizeof(kAppStateTransitionSequenceStepGenerationExpectations5_0) / sizeof(kAppStateTransitionSequenceStepGenerationExpectations5_0[0]),
   NULL,
   NULL,
   NULL},
};

static const AppStateTransitionSequenceMetadata kAppStateTransitionSequences[] = {
  {"sequence.split-toggle-f8",
   "layout_split",
   "split_toggle_f8",
   "Toggle split layout with F8 and prove inactive-panel and layout ownership after each transition.",
   "runtime_backed",
   kAppStateTransitionSequenceSteps0,
   sizeof(kAppStateTransitionSequenceSteps0) / sizeof(kAppStateTransitionSequenceSteps0[0])},
  {"sequence.tab-panel-switch",
   "panel_navigation",
   "tab_panel_switch",
   "Switch active panel with Tab while preserving inactive-panel cursor, viewport, and focus snapshots.",
   "runtime_backed",
   kAppStateTransitionSequenceSteps1,
   sizeof(kAppStateTransitionSequenceSteps1) / sizeof(kAppStateTransitionSequenceSteps1[0])},
  {"sequence.enter-directory-file-transition",
   "directory_file_transition",
   "enter_directory_file_transition",
   "Enter from tree/file targets and validate durable directory/file identity after each step.",
   "runtime_backed",
   kAppStateTransitionSequenceSteps2,
   sizeof(kAppStateTransitionSequenceSteps2) / sizeof(kAppStateTransitionSequenceSteps2[0])},
  {"sequence.esc-modal-dismissal",
   "modal_command",
   "esc_modal_dismissal",
   "Dismiss command/modal ownership through the registered modal transition and preserve panel-local state.",
   "runtime_backed",
   kAppStateTransitionSequenceSteps3,
   sizeof(kAppStateTransitionSequenceSteps3) / sizeof(kAppStateTransitionSequenceSteps3[0])},
  {"sequence.modal-completion",
   "modal_command",
   "modal_completion",
   "Settle prompt, menu, and dialog completion through the explicit modal completion transition while preserving panel-local state.",
   "runtime_backed",
   kAppStateTransitionSequenceSteps15,
   sizeof(kAppStateTransitionSequenceSteps15) / sizeof(kAppStateTransitionSequenceSteps15[0])},
  {"sequence.dotfile-reveal-conceal",
   "visibility_filter",
   "dotfile_reveal_conceal",
   "Reveal and conceal dotfiles without letting hidden entries become visible-navigation selections.",
   "runtime_backed",
   kAppStateTransitionSequenceSteps4,
   sizeof(kAppStateTransitionSequenceSteps4) / sizeof(kAppStateTransitionSequenceSteps4[0])},
  {"sequence.refresh-rebuild",
   "refresh_rebuild",
   "refresh_rebuild",
   "Refresh and rebuild with generation validation and deterministic stale-snapshot fail-closed behavior.",
   "runtime_backed",
   kAppStateTransitionSequenceSteps5,
   sizeof(kAppStateTransitionSequenceSteps5) / sizeof(kAppStateTransitionSequenceSteps5[0])},
  {"sequence.watcher-live-refresh",
   "refresh_rebuild",
   "watcher_live_refresh",
   "Settle watcher-triggered refresh notifications through their explicit transition boundary while preserving rebuild and rebind ordering.",
   "runtime_backed",
   kAppStateTransitionSequenceSteps16,
   sizeof(kAppStateTransitionSequenceSteps16) / sizeof(kAppStateTransitionSequenceSteps16[0])},
  {"sequence.filesystem-mutation-result",
   "filesystem_mutation",
   "filesystem_mutation_result",
   "Apply filesystem mutation results through the registered result transition and validate generation mismatch fallback.",
   "runtime_backed",
   kAppStateTransitionSequenceSteps6,
   sizeof(kAppStateTransitionSequenceSteps6) / sizeof(kAppStateTransitionSequenceSteps6[0])},
  {"sequence.search-jump",
   "search_jump",
   "search_jump",
   "Search/list jump updates selection only through visible-navigation and focus ownership rules.",
   "runtime_backed",
   kAppStateTransitionSequenceSteps7,
   sizeof(kAppStateTransitionSequenceSteps7) / sizeof(kAppStateTransitionSequenceSteps7[0])},
  {"sequence.showall-global-tagged-only",
   "display_mode",
   "showall_global_tagged_only",
   "Toggle showall/global/tagged-only style filters without moving panel-local ownership into shared volume state.",
   "runtime_backed",
   kAppStateTransitionSequenceSteps8,
   sizeof(kAppStateTransitionSequenceSteps8) / sizeof(kAppStateTransitionSequenceSteps8[0])},
  {"sequence.file-small-big-transitions",
   "directory_file_transition",
   "file_small_big_transitions",
   "Move between small-file, big-file, and preview-shaped views without render-side ownership repair.",
   "runtime_backed",
   kAppStateTransitionSequenceSteps9,
   sizeof(kAppStateTransitionSequenceSteps9) / sizeof(kAppStateTransitionSequenceSteps9[0])},
  {"sequence.volume-cycling-release",
   "volume_lifecycle",
   "volume_cycling_release",
   "Cycle and release volumes through shared lifecycle generation while each panel rebinds by identity.",
   "runtime_backed",
   kAppStateTransitionSequenceSteps10,
   sizeof(kAppStateTransitionSequenceSteps10) / sizeof(kAppStateTransitionSequenceSteps10[0])},
  {"sequence.split-close-reopen",
   "layout_split",
   "split_close_reopen",
   "Close and reopen split layout and prove panel snapshots survive layout projection changes.",
   "runtime_backed",
   kAppStateTransitionSequenceSteps11,
   sizeof(kAppStateTransitionSequenceSteps11) / sizeof(kAppStateTransitionSequenceSteps11[0])},
  {"sequence.terminal-resize-reflow",
   "terminal_resize",
   "terminal_resize_reflow",
   "Handle terminal resize events with layout, window-handle, viewport, and generation coverage.",
   "runtime_backed",
   kAppStateTransitionSequenceSteps12,
   sizeof(kAppStateTransitionSequenceSteps12) / sizeof(kAppStateTransitionSequenceSteps12[0])},
  {"sequence.render-reflow-projection",
   "render_reflow",
   "render_reflow_projection",
   "Project settled AppState to render output without mutating authoritative owner fields.",
   "runtime_backed",
   kAppStateTransitionSequenceSteps13,
   sizeof(kAppStateTransitionSequenceSteps13) / sizeof(kAppStateTransitionSequenceSteps13[0])},
  {"sequence.volume-menu-select",
   "menu_action",
   "volume_menu_select",
   "Select a loaded volume from the volume menu while preserving panel-local restore authority.",
   "runtime_backed",
   kAppStateTransitionSequenceSteps14,
   sizeof(kAppStateTransitionSequenceSteps14) / sizeof(kAppStateTransitionSequenceSteps14[0])},
};
static const char *const kAppStateDispatchSurfaceMigrationNotes0[] = {
  "Current input polling and key normalization feed controller dispatch; AppState mutation remains in downstream handlers until runtime transition objects are introduced.",
};

static const char *const kAppStateDispatchSurfaceTransitionSequenceRefs0[] = {
  "sequence.split-toggle-f8",
};

static const char *const kAppStateDispatchSurfaceAllowedDirectWrites1[] = {
  "panel.tree_selection_key",
  "panel.tree_cursor_pos",
  "panel.tree_viewport_origin",
  "panel.focus_shape",
  "panel.panel_generation",
};

static const char *const kAppStateDispatchSurfaceTransitionSequenceRefs1[] = {
  "sequence.split-toggle-f8",
};

static const char *const kAppStateDispatchSurfaceMigrationNotes1[] = {
  "Current directory-window switch dispatch owns tree navigation and focus updates; later migration should route each action through canonical transition boundaries.",
};

static const char *const kAppStateDispatchSurfaceAllowedDirectWrites2[] = {
  "panel.focus_shape",
  "panel.panel_generation",
};

static const char *const kAppStateDispatchSurfaceTransitionSequenceRefs2[] = {
  "sequence.file-small-big-transitions",
};

static const char *const kAppStateDispatchSurfaceMigrationNotes2[] = {
  "Current file-window dispatch remains under the broad keybinding foundation; only writes shared with the navigate-tree contract stay authorized until file-specific transitions are split out.",
};

static const char *const kAppStateDispatchSurfaceTransitionSequenceRefs3[] = {
  "sequence.esc-modal-dismissal",
};

static const char *const kAppStateDispatchSurfaceMigrationNotes3[] = {
  "Current menu and modal choice completion returns a selected key to callers; AppState modal writes remain with the caller until modal transition boundaries are introduced.",
};

static const char *const kAppStateDispatchSurfaceAllowedDirectWrites4[] = {
  "ctx.layout",
  "ctx.window_handles",
  "panel.tree_viewport_origin",
  "panel.file_viewport_origin",
  "panel.panel_generation",
};

static const char *const kAppStateDispatchSurfaceTransitionSequenceRefs4[] = {
  "sequence.terminal-resize-reflow",
};

static const char *const kAppStateDispatchSurfaceMigrationNotes4[] = {
  "Current resize dispatch converts KEY_RESIZE and resize_request state into controller refresh handling; signal-safe work must remain outside asynchronous handlers.",
};

static const char *const kAppStateDispatchSurfaceAllowedDirectWrites5[] = {
  "volume.dir_tree",
  "volume.logged_state",
  "volume.volume_generation",
  "panel.restore_snapshot",
  "panel.panel_generation",
};

static const char *const kAppStateDispatchSurfaceTransitionSequenceRefs5[] = {
  "sequence.refresh-rebuild",
};

static const char *const kAppStateDispatchSurfaceMigrationNotes5[] = {
  "Current safe refresh saves state, rescans, restores, and rebinds panel anchors; migration must preserve that ordering at the transition boundary.",
};

static const char *const kAppStateDispatchSurfaceAllowedDirectWrites6[] = {
  "volume.dir_tree",
  "volume.payload_cache",
  "volume.volume_generation",
  "panel.restore_snapshot",
  "panel.panel_generation",
  "ctx.message_state",
};

static const char *const kAppStateDispatchSurfaceTransitionSequenceRefs6[] = {
  "sequence.filesystem-mutation-result",
};

static const char *const kAppStateDispatchSurfaceMigrationNotes6[] = {
  "Current filesystem command handlers refresh and rebind after successful mutations; only completed mutation results should commit AppState metadata.",
};

static const char *const kAppStateDispatchSurfaceAllowedDirectWrites7[] = {
  "ctx.volumes_head",
  "panel.volume_key",
  "panel.restore_snapshot",
  "panel.panel_generation",
  "volume.volume_generation",
};

static const char *const kAppStateDispatchSurfaceTransitionSequenceRefs7[] = {
  "sequence.volume-cycling-release",
};

static const char *const kAppStateDispatchSurfaceMigrationNotes7[] = {
  "Current loaded-volume selection and cycling update panel bindings directly; migration must preserve inactive panel restore records.",
};

static const char *const kAppStateDispatchSurfaceTransitionSequenceRefs8[] = {
  "sequence.watcher-live-refresh",
};

static const char *const kAppStateDispatchSurfaceMigrationNotes8[] = {
  "Current watcher processing reports settled filesystem activity to input dispatch; the registry now tracks that watcher completion through its own explicit refresh transition boundary.",
};

static const char *const kAppStateDispatchSurfaceTransitionSequenceRefs9[] = {
  "sequence.render-reflow-projection",
};

static const char *const kAppStateDispatchSurfaceMigrationNotes9[] = {
  "Current render refresh projects settled state to ncurses windows; projection must not become selection, viewport, or topology authority.",
};

static const char *const kAppStateDispatchSurfaceTransitionSequenceRefs10[] = {
  "sequence.search-jump",
};
static const char *const kAppStateDispatchSurfaceMigrationNotes10[] = {
  "Current command handlers settle completion state after external or user-command execution returns; runtime migration should lift that completion boundary without broadening write authority here.",
};
static const char *const kAppStateDispatchSurfaceTransitionSequenceRefs15[] = {
  "sequence.modal-completion",
};
static const char *const kAppStateDispatchSurfaceMigrationNotes15[] = {
  "Current menu and modal choice completion returns a selected key to callers; the registry now tracks that completion through its own explicit transition boundary while runtime modal writes remain with the caller.",
};
static const char *const kAppStateDispatchSurfaceAllowedDirectWrites11[] = {
  "ctx.active",
  "panel.volume_key",
  "panel.restore_snapshot",
  "panel.panel_generation",
};
static const char *const kAppStateDispatchSurfaceTransitionSequenceRefs11[] = {
  "sequence.volume-menu-select",
};
static const char *const kAppStateDispatchSurfaceMigrationNotes11[] = {
  "Current loaded-volume menu selection remains the existing selection boundary for active-panel volume binding until dedicated runtime transition objects replace menu-coupled control flow.",
};
static const char *const kAppStateDispatchSurfaceAllowedDirectWrites12[] = {
  "panel.tree_selection_key",
  "panel.file_selection_key",
  "panel.tree_cursor_pos",
  "panel.tree_viewport_origin",
  "panel.file_viewport_origin",
  "panel.panel_generation",
};
static const char *const kAppStateDispatchSurfaceTransitionSequenceRefs12[] = {
  "sequence.refresh-rebuild",
};
static const char *const kAppStateDispatchSurfaceMigrationNotes12[] = {
  "Current panel-anchor restore helpers remain the canonical callback surface for post-rebuild re-resolution; migration should keep fallback ordering anchored here.",
};

static const AppStateDispatchSurfaceMetadata kAppStateDispatchSurfaces[] = {
  {"surface.key-decode-input-dispatch",
   "key_decode_input_dispatch",
   "src/ui/key_engine.c",
   "GetEventOrKey",
   "transition.keybinding.navigate-tree",
   "covered_by_transition_record",
   NULL,
   0,
   kAppStateDispatchSurfaceTransitionSequenceRefs0,
   sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs0) /
       sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs0[0]),
   kAppStateDispatchSurfaceMigrationNotes0,
   sizeof(kAppStateDispatchSurfaceMigrationNotes0) / sizeof(kAppStateDispatchSurfaceMigrationNotes0[0])},
  {"surface.directory-window-action-dispatch",
   "directory_window_action_dispatch",
   "src/ui/ctrl_dir.c",
   "HandleDirWindow",
   "transition.keybinding.navigate-tree",
   "covered_by_transition_record",
   kAppStateDispatchSurfaceAllowedDirectWrites1,
   sizeof(kAppStateDispatchSurfaceAllowedDirectWrites1) /
       sizeof(kAppStateDispatchSurfaceAllowedDirectWrites1[0]),
   kAppStateDispatchSurfaceTransitionSequenceRefs1,
   sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs1) /
       sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs1[0]),
   kAppStateDispatchSurfaceMigrationNotes1,
   sizeof(kAppStateDispatchSurfaceMigrationNotes1) / sizeof(kAppStateDispatchSurfaceMigrationNotes1[0])},
  {"surface.file-window-action-dispatch",
   "file_window_action_dispatch",
   "src/ui/ctrl_file.c",
   "HandleFileWindow",
   "transition.keybinding.navigate-tree",
   "covered_by_transition_record",
   kAppStateDispatchSurfaceAllowedDirectWrites2,
   sizeof(kAppStateDispatchSurfaceAllowedDirectWrites2) /
       sizeof(kAppStateDispatchSurfaceAllowedDirectWrites2[0]),
   kAppStateDispatchSurfaceTransitionSequenceRefs2,
   sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs2) /
       sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs2[0]),
   kAppStateDispatchSurfaceMigrationNotes2,
   sizeof(kAppStateDispatchSurfaceMigrationNotes2) / sizeof(kAppStateDispatchSurfaceMigrationNotes2[0])},
  {"surface.menu-modal-completion",
   "menu_modal_completion",
   "src/ui/key_engine.c",
   "InputChoice",
   "transition.modal-action.dismiss",
   "covered_by_transition_record",
   NULL,
   0,
   kAppStateDispatchSurfaceTransitionSequenceRefs3,
   sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs3) /
       sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs3[0]),
   kAppStateDispatchSurfaceMigrationNotes3,
   sizeof(kAppStateDispatchSurfaceMigrationNotes3) / sizeof(kAppStateDispatchSurfaceMigrationNotes3[0])},
  {"surface.modal-completion-event",
   "menu_modal_completion",
   "src/ui/key_engine.c",
   "InputChoice",
   "transition.modal-action.completion",
   "covered_by_transition_record",
   NULL,
   0,
   kAppStateDispatchSurfaceTransitionSequenceRefs15,
   sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs15) /
       sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs15[0]),
   kAppStateDispatchSurfaceMigrationNotes15,
   sizeof(kAppStateDispatchSurfaceMigrationNotes15) /
       sizeof(kAppStateDispatchSurfaceMigrationNotes15[0])},
  {"surface.resize-signal-handling",
   "resize_signal_handling",
   "src/ui/key_engine.c",
   "GetEventOrKey",
   "transition.terminal-signal-resize",
   "covered_by_transition_record",
   kAppStateDispatchSurfaceAllowedDirectWrites4,
   sizeof(kAppStateDispatchSurfaceAllowedDirectWrites4) /
       sizeof(kAppStateDispatchSurfaceAllowedDirectWrites4[0]),
   kAppStateDispatchSurfaceTransitionSequenceRefs4,
   sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs4) /
       sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs4[0]),
   kAppStateDispatchSurfaceMigrationNotes4,
   sizeof(kAppStateDispatchSurfaceMigrationNotes4) / sizeof(kAppStateDispatchSurfaceMigrationNotes4[0])},
  {"surface.refresh-rebuild-rebind",
   "refresh_rebuild_rebind",
   "src/ui/dir_ops.c",
   "RefreshTreeSafe",
   "transition.refresh-rebuild.manual-refresh",
   "covered_by_transition_record",
   kAppStateDispatchSurfaceAllowedDirectWrites5,
   sizeof(kAppStateDispatchSurfaceAllowedDirectWrites5) /
       sizeof(kAppStateDispatchSurfaceAllowedDirectWrites5[0]),
   kAppStateDispatchSurfaceTransitionSequenceRefs5,
   sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs5) /
       sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs5[0]),
   kAppStateDispatchSurfaceMigrationNotes5,
   sizeof(kAppStateDispatchSurfaceMigrationNotes5) / sizeof(kAppStateDispatchSurfaceMigrationNotes5[0])},
  {"surface.filesystem-mutation-result",
   "filesystem_mutation_result",
   "src/ui/dir_ops.c",
   "HandleDirMakeDirectory",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "covered_by_transition_record",
   kAppStateDispatchSurfaceAllowedDirectWrites6,
   sizeof(kAppStateDispatchSurfaceAllowedDirectWrites6) /
       sizeof(kAppStateDispatchSurfaceAllowedDirectWrites6[0]),
   kAppStateDispatchSurfaceTransitionSequenceRefs6,
   sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs6) /
       sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs6[0]),
   kAppStateDispatchSurfaceMigrationNotes6,
   sizeof(kAppStateDispatchSurfaceMigrationNotes6) / sizeof(kAppStateDispatchSurfaceMigrationNotes6[0])},
  {"surface.volume-operation",
   "volume_operation",
   "src/ui/volume_menu.c",
   "SelectLoadedVolume",
   "transition.volume-operation.release-cycle",
   "covered_by_transition_record",
   kAppStateDispatchSurfaceAllowedDirectWrites7,
   sizeof(kAppStateDispatchSurfaceAllowedDirectWrites7) /
       sizeof(kAppStateDispatchSurfaceAllowedDirectWrites7[0]),
   kAppStateDispatchSurfaceTransitionSequenceRefs7,
   sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs7) /
       sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs7[0]),
   kAppStateDispatchSurfaceMigrationNotes7,
   sizeof(kAppStateDispatchSurfaceMigrationNotes7) / sizeof(kAppStateDispatchSurfaceMigrationNotes7[0])},
  {"surface.watcher-live-refresh",
   "watcher_live_refresh",
   "src/ui/key_engine.c",
   "GetEventOrKey",
   "transition.refresh-rebuild.watcher-live-refresh",
   "covered_by_transition_record",
   NULL,
   0,
   kAppStateDispatchSurfaceTransitionSequenceRefs8,
   sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs8) /
       sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs8[0]),
   kAppStateDispatchSurfaceMigrationNotes8,
   sizeof(kAppStateDispatchSurfaceMigrationNotes8) / sizeof(kAppStateDispatchSurfaceMigrationNotes8[0])},
  {"surface.render-reflow-projection",
   "render_reflow_projection",
   "src/ui/display.c",
   "RefreshView",
   "transition.render-reflow.project-state",
   "covered_by_transition_record",
   NULL,
   0,
   kAppStateDispatchSurfaceTransitionSequenceRefs9,
   sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs9) /
       sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs9[0]),
   kAppStateDispatchSurfaceMigrationNotes9,
   sizeof(kAppStateDispatchSurfaceMigrationNotes9) / sizeof(kAppStateDispatchSurfaceMigrationNotes9[0])},
  {"surface.command-completion-dispatch",
   "command_completion_dispatch",
   "src/ui/ctrl_file_ops.c",
   "handle_file_window_command_action",
   "transition.command-completion.user-command",
   "covered_by_transition_record",
   NULL,
   0,
   kAppStateDispatchSurfaceTransitionSequenceRefs10,
   sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs10) /
       sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs10[0]),
   kAppStateDispatchSurfaceMigrationNotes10,
   sizeof(kAppStateDispatchSurfaceMigrationNotes10) / sizeof(kAppStateDispatchSurfaceMigrationNotes10[0])},
  {"surface.volume-menu-selection",
   "volume_menu_selection",
   "src/ui/volume_menu.c",
   "SelectLoadedVolume",
   "transition.menu-action.volume-select",
   "covered_by_transition_record",
   kAppStateDispatchSurfaceAllowedDirectWrites11,
   sizeof(kAppStateDispatchSurfaceAllowedDirectWrites11) /
       sizeof(kAppStateDispatchSurfaceAllowedDirectWrites11[0]),
   kAppStateDispatchSurfaceTransitionSequenceRefs11,
   sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs11) /
       sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs11[0]),
   kAppStateDispatchSurfaceMigrationNotes11,
   sizeof(kAppStateDispatchSurfaceMigrationNotes11) / sizeof(kAppStateDispatchSurfaceMigrationNotes11[0])},
  {"surface.panel-anchor-rebind",
   "rebuild_rebind_callback",
   "src/ui/panel_anchor.c",
   "RestorePanelViewportSnapshot",
   "transition.rebuild-rebind-callback.panel-anchor",
   "covered_by_transition_record",
   kAppStateDispatchSurfaceAllowedDirectWrites12,
   sizeof(kAppStateDispatchSurfaceAllowedDirectWrites12) /
       sizeof(kAppStateDispatchSurfaceAllowedDirectWrites12[0]),
   kAppStateDispatchSurfaceTransitionSequenceRefs12,
   sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs12) /
       sizeof(kAppStateDispatchSurfaceTransitionSequenceRefs12[0]),
   kAppStateDispatchSurfaceMigrationNotes12,
   sizeof(kAppStateDispatchSurfaceMigrationNotes12) / sizeof(kAppStateDispatchSurfaceMigrationNotes12[0])},
};
static const char *const kAppStateCompatibilityShimInvariantChecks1[] = {
  "invariant.panel-local-focus-restore",
  "invariant.inactive-panel-frozen",
};

static const char *const kAppStateCompatibilityShimInvariantChecks2[] = {
  "invariant.render-projection-read-only",
  "invariant.blocked-transition-determinism",
};

static const char *const kAppStateCompatibilityShimOwnerFieldRefs1[] = {
  "panel.focus_shape",
  "panel.panel_generation",
};

static const char *const kAppStateCompatibilityShimOwnerFieldRefs2[] = {
  "panel.tree_cursor_pos",
  "panel.tree_viewport_origin",
  "panel.file_viewport_origin",
};

static const char *const kAppStateCompatibilityShimGenerationDomainRefs1[] = {
  "generation.panel.local-authority",
  "shape.panel.focus",
};

static const char *const kAppStateCompatibilityShimGenerationDomainRefs2[] = {
  "generation.panel.local-authority",
  "identity.directory.stable-key",
  "identity.file.stable-key",
  "reflow.layout.projection",
};

static const char *const kAppStateCompatibilityShimDiffHarnessRefs1[] = {
  "harness.declared-write-set-diff",
};

static const char *const kAppStateCompatibilityShimDiffHarnessRefs2[] = {
  "harness.render-projection-read-only-diff",
  "harness.generation-mismatch-check",
  "harness.blocked-transition-no-unrelated-mutation",
};

static const char *const kAppStateCompatibilityShimSourceBoundaryRefs1[] = {
  "src/ui/appstate_focus.c",
  "src/ui/ctrl_file_ops.c",
  "src/ui/ctrl_file.c",
  "src/ui/ctrl_dir.c",
  "src/ui/dir_ops.c",
  "src/ui/split_transition.c",
};

static const char *const kAppStateCompatibilityShimSourceBoundaryRefs2[] = {
  "src/ui/display.c",
};

static const char *const kAppStateInvariantProtectedFields0[] = {
  "ctx.active",
  "panel.volume_key",
  "panel.tree_selection_key",
  "panel.tree_cursor_pos",
  "panel.tree_viewport_origin",
  "panel.file_selection_key",
  "panel.file_display_state",
  "panel.file_viewport_origin",
  "panel.focus_shape",
  "panel.restore_snapshot",
  "panel.panel_generation",
};

static const char *const kAppStateInvariantTransitionIds0[] = {
  "transition.keybinding.navigate-tree",
  "transition.menu-action.volume-select",
  "transition.modal-action.dismiss",
  "transition.modal-action.completion",
  "transition.refresh-rebuild.manual-refresh",
  "transition.refresh-rebuild.watcher-live-refresh",
  "transition.volume-operation.release-cycle",
  "transition.terminal-signal-resize",
};

static const char *const kAppStateInvariantDispatchSurfaceIds0[] = {
  "surface.directory-window-action-dispatch",
  "surface.file-window-action-dispatch",
  "surface.volume-menu-selection",
  "surface.menu-modal-completion",
  "surface.modal-completion-event",
  "surface.refresh-rebuild-rebind",
  "surface.volume-operation",
  "surface.resize-signal-handling",
};

static const char *const kAppStateInvariantMigrationNotes0[] = {
  "Future dynamic tests should distinguish shared topology mirroring from inactive panel-local mutation.",
};

static const char *const kAppStateInvariantProtectedFields1[] = {
  "ctx.layout",
  "ctx.render_dirty_flags",
  "ctx.window_handles",
  "panel.tree_selection_key",
  "panel.file_selection_key",
  "panel.tree_viewport_origin",
  "panel.file_viewport_origin",
  "panel.focus_shape",
  "panel.panel_generation",
  "volume.volume_generation",
};

static const char *const kAppStateInvariantTransitionIds1[] = {
  "transition.render-reflow.project-state",
  "transition.terminal-signal-resize",
};

static const char *const kAppStateInvariantDispatchSurfaceIds1[] = {
  "surface.render-reflow-projection",
  "surface.resize-signal-handling",
};

static const char *const kAppStateInvariantMigrationNotes1[] = {
  "Runtime migration must keep ncurses drawing and temporary row calculations from becoming restore authority.",
};

static const char *const kAppStateInvariantProtectedFields2[] = {
  "panel.tree_selection_key",
  "panel.tree_cursor_pos",
  "panel.tree_viewport_origin",
  "panel.restore_snapshot",
  "panel.panel_generation",
  "volume.dir_tree",
  "volume.logged_state",
  "volume.volume_generation",
};

static const char *const kAppStateInvariantTransitionIds2[] = {
  "transition.keybinding.navigate-tree",
  "transition.refresh-rebuild.manual-refresh",
  "transition.refresh-rebuild.watcher-live-refresh",
  "transition.rebuild-rebind-callback.panel-anchor",
};

static const char *const kAppStateInvariantDispatchSurfaceIds2[] = {
  "surface.directory-window-action-dispatch",
  "surface.refresh-rebuild-rebind",
  "surface.panel-anchor-rebind",
  "surface.watcher-live-refresh",
};

static const char *const kAppStateInvariantMigrationNotes2[] = {
  "Hidden-entry checks must use stable identity and visibility metadata rather than stale row positions.",
};

static const char *const kAppStateInvariantProtectedFields3[] = {
  "ctx.modal_state",
  "panel.focus_shape",
  "panel.tree_selection_key",
  "panel.file_selection_key",
  "panel.tree_viewport_origin",
  "panel.file_viewport_origin",
  "panel.restore_snapshot",
  "panel.panel_generation",
};

static const char *const kAppStateInvariantTransitionIds3[] = {
  "transition.keybinding.navigate-tree",
  "transition.menu-action.volume-select",
  "transition.modal-action.dismiss",
  "transition.modal-action.completion",
  "transition.rebuild-rebind-callback.panel-anchor",
};

static const char *const kAppStateInvariantDispatchSurfaceIds3[] = {
  "surface.directory-window-action-dispatch",
  "surface.file-window-action-dispatch",
  "surface.menu-modal-completion",
  "surface.modal-completion-event",
  "surface.volume-operation",
};

static const char *const kAppStateInvariantMigrationNotes3[] = {
  "Compatibility session mirrors may shadow focus only after the panel-local owner has committed the transition.",
};

static const char *const kAppStateInvariantProtectedFields4[] = {
  "panel.tree_selection_key",
  "panel.file_selection_key",
  "panel.tree_viewport_origin",
  "panel.file_viewport_origin",
  "panel.restore_snapshot",
  "panel.panel_generation",
  "volume.dir_tree",
  "volume.volume_generation",
};

static const char *const kAppStateInvariantTransitionIds4[] = {
  "transition.refresh-rebuild.manual-refresh",
  "transition.refresh-rebuild.watcher-live-refresh",
  "transition.terminal-signal-resize",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
  "transition.rebuild-rebind-callback.panel-anchor",
};

static const char *const kAppStateInvariantDispatchSurfaceIds4[] = {
  "surface.refresh-rebuild-rebind",
  "surface.resize-signal-handling",
  "surface.filesystem-mutation-result",
  "surface.panel-anchor-rebind",
  "surface.watcher-live-refresh",
};

static const char *const kAppStateInvariantMigrationNotes4[] = {
  "Canonical restore helpers must remain the only authority for rebind after topology or layout invalidation.",
};

static const char *const kAppStateInvariantProtectedFields5[] = {
  "ctx.active",
  "ctx.volumes_head",
  "panel.volume_key",
  "panel.tree_selection_key",
  "panel.file_selection_key",
  "panel.file_display_state",
  "panel.focus_shape",
  "panel.restore_snapshot",
  "panel.panel_generation",
  "volume.dir_tree",
  "volume.logged_state",
  "volume.payload_cache",
  "volume.volume_generation",
};

static const char *const kAppStateInvariantTransitionIds5[] = {
  "transition.menu-action.volume-select",
  "transition.refresh-rebuild.manual-refresh",
  "transition.refresh-rebuild.watcher-live-refresh",
  "transition.volume-operation.release-cycle",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
};

static const char *const kAppStateInvariantDispatchSurfaceIds5[] = {
  "surface.volume-menu-selection",
  "surface.volume-operation",
  "surface.refresh-rebuild-rebind",
  "surface.filesystem-mutation-result",
  "surface.watcher-live-refresh",
};

static const char *const kAppStateInvariantMigrationNotes5[] = {
  "Shared topology mirroring must be followed by panel-local rebind rather than copying active panel state into inactive panels.",
};

static const char *const kAppStateInvariantProtectedFields6[] = {
  "panel.tree_selection_key",
  "panel.file_selection_key",
  "panel.tree_viewport_origin",
  "panel.file_viewport_origin",
  "panel.restore_snapshot",
  "panel.panel_generation",
  "volume.dir_tree",
  "volume.volume_generation",
};

static const char *const kAppStateInvariantTransitionIds6[] = {
  "transition.refresh-rebuild.manual-refresh",
  "transition.refresh-rebuild.watcher-live-refresh",
  "transition.volume-operation.release-cycle",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
  "transition.rebuild-rebind-callback.panel-anchor",
};

static const char *const kAppStateInvariantDispatchSurfaceIds6[] = {
  "surface.refresh-rebuild-rebind",
  "surface.volume-operation",
  "surface.filesystem-mutation-result",
  "surface.panel-anchor-rebind",
  "surface.watcher-live-refresh",
};

static const char *const kAppStateInvariantMigrationNotes6[] = {
  "Generation validation must happen before any restore snapshot is applied to a panel record.",
};

static const char *const kAppStateInvariantProtectedFields7[] = {
  "ctx.command_state",
  "ctx.refresh_mode",
  "ctx.view_mode",
  "ctx.dir_mode",
  "ctx.message_state",
  "ctx.modal_state",
  "ctx.pending_transition",
  "panel.tree_selection_key",
  "panel.file_selection_key",
  "panel.restore_snapshot",
  "panel.panel_generation",
  "volume.dir_tree",
  "volume.payload_cache",
  "volume.volume_generation",
};

static const char *const kAppStateInvariantTransitionIds7[] = {
  "transition.keybinding.navigate-tree",
  "transition.menu-action.volume-select",
  "transition.modal-action.dismiss",
  "transition.modal-action.completion",
  "transition.refresh-rebuild.manual-refresh",
  "transition.refresh-rebuild.watcher-live-refresh",
  "transition.volume-operation.release-cycle",
  "transition.terminal-signal-resize",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
  "transition.command-completion.user-command",
  "transition.rebuild-rebind-callback.panel-anchor",
  "transition.render-reflow.project-state",
};

static const char *const kAppStateInvariantDispatchSurfaceIds7[] = {
  "surface.key-decode-input-dispatch",
  "surface.directory-window-action-dispatch",
  "surface.file-window-action-dispatch",
  "surface.menu-modal-completion",
  "surface.modal-completion-event",
  "surface.resize-signal-handling",
  "surface.refresh-rebuild-rebind",
  "surface.filesystem-mutation-result",
  "surface.volume-operation",
  "surface.watcher-live-refresh",
  "surface.render-reflow-projection",
};

static const char *const kAppStateInvariantMigrationNotes7[] = {
  "Blocked outcomes are cross-cutting across all registered transition and dispatch surfaces but are listed explicitly here for guard traceability.",
};

static const AppStateInvariantMetadata kAppStateInvariants[] = {
  {"invariant.inactive-panel-frozen",
   "inactive_panel_frozen",
   "panel-local state",
   kAppStateInvariantProtectedFields0,
   sizeof(kAppStateInvariantProtectedFields0) /
       sizeof(kAppStateInvariantProtectedFields0[0]),
   kAppStateInvariantTransitionIds0,
   sizeof(kAppStateInvariantTransitionIds0) /
       sizeof(kAppStateInvariantTransitionIds0[0]),
   kAppStateInvariantDispatchSurfaceIds0,
   sizeof(kAppStateInvariantDispatchSurfaceIds0) /
       sizeof(kAppStateInvariantDispatchSurfaceIds0[0]),
   "Fail if an active-only transition mutates inactive panel-local identity, viewport, focus, restore, or generation fields outside an explicitly targeted transition.",
   "covered_by_runtime_registry",
   "State-sequence harness snapshots both panels before each active-panel transition and compares inactive panel fields after allowed and blocked outcomes.",
   kAppStateInvariantMigrationNotes0,
   sizeof(kAppStateInvariantMigrationNotes0) /
       sizeof(kAppStateInvariantMigrationNotes0[0])},
  {"invariant.render-projection-read-only",
   "render_projection_read_only",
   "render/projection/invalidation state",
   kAppStateInvariantProtectedFields1,
   sizeof(kAppStateInvariantProtectedFields1) /
       sizeof(kAppStateInvariantProtectedFields1[0]),
   kAppStateInvariantTransitionIds1,
   sizeof(kAppStateInvariantTransitionIds1) /
       sizeof(kAppStateInvariantTransitionIds1[0]),
   kAppStateInvariantDispatchSurfaceIds1,
   sizeof(kAppStateInvariantDispatchSurfaceIds1) /
       sizeof(kAppStateInvariantDispatchSurfaceIds1[0]),
   "Fail if render or reflow chooses new authoritative selection, focus, viewport, panel generation, or volume generation from projected rows or window geometry.",
   "covered_by_runtime_registry",
   "Transition-diff harness runs render/reflow passes after settled transitions and verifies only declared render projection fields change.",
   kAppStateInvariantMigrationNotes1,
   sizeof(kAppStateInvariantMigrationNotes1) /
       sizeof(kAppStateInvariantMigrationNotes1[0])},
  {"invariant.hidden-entry-visible-navigation",
   "hidden_entry_visible_navigation",
   "panel-local state",
   kAppStateInvariantProtectedFields2,
   sizeof(kAppStateInvariantProtectedFields2) /
       sizeof(kAppStateInvariantProtectedFields2[0]),
   kAppStateInvariantTransitionIds2,
   sizeof(kAppStateInvariantTransitionIds2) /
       sizeof(kAppStateInvariantTransitionIds2[0]),
   kAppStateInvariantDispatchSurfaceIds2,
   sizeof(kAppStateInvariantDispatchSurfaceIds2) /
       sizeof(kAppStateInvariantDispatchSurfaceIds2[0]),
   "Fail if visible navigation lands on a hidden entry or resurrects a concealed identity after visibility/topology changes.",
   "covered_by_runtime_registry",
   "Generated navigation sequences toggle visibility, rebuild, and move through visible rows while asserting the selected identity remains visible or falls back deterministically.",
   kAppStateInvariantMigrationNotes2,
   sizeof(kAppStateInvariantMigrationNotes2) /
       sizeof(kAppStateInvariantMigrationNotes2[0])},
  {"invariant.panel-local-focus-restore",
   "panel_local_focus_restore",
   "panel-local state",
   kAppStateInvariantProtectedFields3,
   sizeof(kAppStateInvariantProtectedFields3) /
       sizeof(kAppStateInvariantProtectedFields3[0]),
   kAppStateInvariantTransitionIds3,
   sizeof(kAppStateInvariantTransitionIds3) /
       sizeof(kAppStateInvariantTransitionIds3[0]),
   kAppStateInvariantDispatchSurfaceIds3,
   sizeof(kAppStateInvariantDispatchSurfaceIds3) /
       sizeof(kAppStateInvariantDispatchSurfaceIds3[0]),
   "Fail if focus restoration imports another panel's shape, briefly renders a different shape, or restores focus from session mirrors instead of panel-local records.",
   "covered_by_runtime_registry",
   "Sequence tests alternate modal dismissal, Tab/F8-style routing, and file/tree transitions while asserting each panel restores its own recorded focus shape.",
   kAppStateInvariantMigrationNotes3,
   sizeof(kAppStateInvariantMigrationNotes3) /
       sizeof(kAppStateInvariantMigrationNotes3[0])},
  {"invariant.viewport-identity-rebind",
   "viewport_identity_rebind",
   "panel-local state",
   kAppStateInvariantProtectedFields4,
   sizeof(kAppStateInvariantProtectedFields4) /
       sizeof(kAppStateInvariantProtectedFields4[0]),
   kAppStateInvariantTransitionIds4,
   sizeof(kAppStateInvariantTransitionIds4) /
       sizeof(kAppStateInvariantTransitionIds4[0]),
   kAppStateInvariantDispatchSurfaceIds4,
   sizeof(kAppStateInvariantDispatchSurfaceIds4) /
       sizeof(kAppStateInvariantDispatchSurfaceIds4[0]),
   "Fail if rebuild, mutation, or resize restores viewport from raw rows when the stable identity is still visible or before deterministic fallback order is exhausted.",
   "covered_by_runtime_registry",
   "Snapshot/diff tests rebuild or resize around stable directory and file identities, then assert exact rebind when visible and deterministic fallback otherwise.",
   kAppStateInvariantMigrationNotes4,
   sizeof(kAppStateInvariantMigrationNotes4) /
       sizeof(kAppStateInvariantMigrationNotes4[0])},
  {"invariant.shared-state-panel-local-isolation",
   "shared_state_panel_local_isolation",
   "volume/shared topology and payload state",
   kAppStateInvariantProtectedFields5,
   sizeof(kAppStateInvariantProtectedFields5) /
       sizeof(kAppStateInvariantProtectedFields5[0]),
   kAppStateInvariantTransitionIds5,
   sizeof(kAppStateInvariantTransitionIds5) /
       sizeof(kAppStateInvariantTransitionIds5[0]),
   kAppStateInvariantDispatchSurfaceIds5,
   sizeof(kAppStateInvariantDispatchSurfaceIds5) /
       sizeof(kAppStateInvariantDispatchSurfaceIds5[0]),
   "Fail if shared volume topology or registry changes overwrite panel-local selection, focus, tags, viewport, or restore snapshots by shared index or pointer aliasing.",
   "covered_by_runtime_registry",
   "Split-panel sequences share a volume, mutate shared topology, and verify each panel rebinds through its own identity without cross-panel field drift.",
   kAppStateInvariantMigrationNotes5,
   sizeof(kAppStateInvariantMigrationNotes5) /
       sizeof(kAppStateInvariantMigrationNotes5[0])},
  {"invariant.stale-snapshot-fail-closed",
   "stale_snapshot_fail_closed",
   "panel-local state",
   kAppStateInvariantProtectedFields6,
   sizeof(kAppStateInvariantProtectedFields6) /
       sizeof(kAppStateInvariantProtectedFields6[0]),
   kAppStateInvariantTransitionIds6,
   sizeof(kAppStateInvariantTransitionIds6) /
       sizeof(kAppStateInvariantTransitionIds6[0]),
   kAppStateInvariantDispatchSurfaceIds6,
   sizeof(kAppStateInvariantDispatchSurfaceIds6) /
       sizeof(kAppStateInvariantDispatchSurfaceIds6[0]),
   "Fail if a generation mismatch reuses stale DirEntry/FileEntry pointers, stale flat-list rows, or stale snapshot payloads instead of exact rebind or deterministic fallback.",
   "covered_by_runtime_registry",
   "Harness corrupts or invalidates saved generations around rebuild/mutation transitions and asserts stale snapshots fail closed without unrelated mutation.",
   kAppStateInvariantMigrationNotes6,
   sizeof(kAppStateInvariantMigrationNotes6) /
       sizeof(kAppStateInvariantMigrationNotes6[0])},
  {"invariant.blocked-transition-determinism",
   "blocked_transition_determinism",
   "ctx/session state",
   kAppStateInvariantProtectedFields7,
   sizeof(kAppStateInvariantProtectedFields7) /
       sizeof(kAppStateInvariantProtectedFields7[0]),
   kAppStateInvariantTransitionIds7,
   sizeof(kAppStateInvariantTransitionIds7) /
       sizeof(kAppStateInvariantTransitionIds7[0]),
   kAppStateInvariantDispatchSurfaceIds7,
   sizeof(kAppStateInvariantDispatchSurfaceIds7) /
       sizeof(kAppStateInvariantDispatchSurfaceIds7[0]),
   "Fail if a blocked or invalid transition partially mutates authoritative panel/volume state, advances generations, performs hidden side effects, or chooses a non-deterministic fallback.",
   "covered_by_runtime_registry",
   "Negative state-sequence tests force guard failures and unavailable targets, then assert no unrelated owner fields differ and any message/modal output is declared.",
   kAppStateInvariantMigrationNotes7,
   sizeof(kAppStateInvariantMigrationNotes7) /
       sizeof(kAppStateInvariantMigrationNotes7[0])},
};

static const char *const kAppStateTransitionSideEffects0[] = {
  "May request directory payload load when Enter reveals an unlogged directory.",
};
static const char *const kAppStateTransitionSideEffects1[] = {
  "May read the volume registry.",
};
static const char *const kAppStateTransitionSideEffects2[] = {
  "No filesystem side effects.",
};
static const char *const kAppStateTransitionSideEffects3[] = {
  "Filesystem scan/read operations.",
};

static const char *const kAppStateTransitionSideEffects15[] = {
  "Filesystem scan/read operations after watcher debounce/settle.",
};
static const char *const kAppStateTransitionSideEffects4[] = {
  "May close archive/filesystem resources.",
};
static const char *const kAppStateTransitionSideEffects5[] = {
  "Ncurses window recreation in main loop only.",
};
static const char *const kAppStateTransitionSideEffects6[] = {
  "Filesystem create/copy/move/delete/chmod-like operations already performed by command layer.",
};
static const char *const kAppStateTransitionSideEffects7[] = {
  "External command execution is outside the AppState commit boundary.",
};
static const char *const kAppStateTransitionSideEffects8[] = {
  "May request payload reload for a visible but unloaded file-view anchor.",
};
static const char *const kAppStateTransitionSideEffects9[] = {
  "Ncurses drawing and doupdate.",
};

static const AppStateTransitionMetadata kAppStateTransitions[] = {
  {"transition.keybinding.navigate-tree",
   "keybinding",
   "AppState.panel[active].focus_shape=tree",
   "canonical_key:up_down_left_right_enter",
   "Input is valid for the active focus shape and target identity resolves in the current volume namespace.",
   "Mutate only the active panel selection, viewport, and focus-shape fields required by the key semantics.",
   "Leave AppState unchanged except for an explicit no-op or constraint message when user-visible clarity is required.",
   "AppState.panel[active] records the new tree selection, viewport, and focus shape before render projection.",
   "YtreeNovaPanel(active)",
   "Increment panel_generation when selection, viewport, or focus shape changes; volume_generation is unchanged.",
   kAppStateTransitionSideEffects0,
   sizeof(kAppStateTransitionSideEffects0) /
       sizeof(kAppStateTransitionSideEffects0[0]),
   "tree_view,file_view,footer",
   "covered_by_transition_record",
   "Route HandleDirWindow key dispatch through the canonical transition boundary in a later runtime migration.",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0])},
  {"transition.menu-action.volume-select",
   "menu_action",
   "AppState.modal=volume_menu",
   "menu_selection:loaded_volume",
   "Selected volume identity exists; selecting the already-active volume must preserve its logged topology.",
   "Bind the active panel to the selected volume and restore that panel's own snapshot for the volume.",
   "Dismiss or keep the menu according to existing UI semantics without changing panel or volume ownership.",
   "AppState.panel[active].volume_key matches the selected volume and uses the panel-local restore record.",
   "ViewContext(session routing) and YtreeNovaPanel(active)",
   "Increment panel_generation when the active panel binds to a different volume; volume_generation changes only for explicit relog/rebuild.",
   kAppStateTransitionSideEffects1,
   sizeof(kAppStateTransitionSideEffects1) /
       sizeof(kAppStateTransitionSideEffects1[0]),
   "layout,tree_view,file_view,stats,footer",
   "covered_by_transition_record",
   "Keep the loaded-volume preservation rule from the specification during runtime migration.",
   kAppStateTransitionWriteSet1,
   sizeof(kAppStateTransitionWriteSet1) / sizeof(kAppStateTransitionWriteSet1[0])},
  {"transition.modal-action.dismiss",
   "modal_action",
   "AppState.modal=severity_or_neutral_dialog",
   "modal_key:esc_or_enter",
   "The key is accepted by the active modal class and no destructive confirmation is being accepted implicitly.",
   "Clear the modal region and restore the previously recorded panel/session focus shape.",
   "Keep the modal active and leave underlying panel/volume state unchanged.",
   "AppState.modal=none with the suspended panel state restored from its record.",
   "ViewContext.modal_region",
   "Increment panel_generation only if restoring the suspended focus shape changes panel-local state.",
   kAppStateTransitionSideEffects2,
   sizeof(kAppStateTransitionSideEffects2) /
       sizeof(kAppStateTransitionSideEffects2[0]),
   "modal_overlay,footer,underlying_view_projection",
   "covered_by_transition_record",
   "Modal migration must preserve severity versus neutral dialog routing.",
   kAppStateTransitionWriteSet2,
   sizeof(kAppStateTransitionWriteSet2) / sizeof(kAppStateTransitionWriteSet2[0])},
  {"transition.modal-action.completion",
   "modal_action",
   "AppState.modal=prompt_menu_or_dialog_awaiting_completion",
   "modal_completion:selected_key_or_cancel",
   "The completion result belongs to the active modal source and destructive confirmations remain routed through their explicit command transitions.",
   "Record the modal completion outcome, clear the modal region, and restore the previously recorded panel/session focus shape.",
   "Ignore stale or mismatched modal completion and leave the modal plus underlying panel/volume state unchanged.",
   "AppState.modal=none with the completion outcome returned to the owning command path and suspended panel state restored from its record.",
   "ViewContext.modal_region",
   "Increment panel_generation only if restoring the suspended focus shape changes panel-local state.",
   kAppStateTransitionSideEffects2,
   sizeof(kAppStateTransitionSideEffects2) /
       sizeof(kAppStateTransitionSideEffects2[0]),
   "modal_overlay,footer,underlying_view_projection",
   "covered_by_transition_record",
   "Runtime migration must preserve prompt/menu/dialog completion routing while destructive confirmations stay on their dedicated command transitions.",
   kAppStateTransitionWriteSet2,
   sizeof(kAppStateTransitionWriteSet2) / sizeof(kAppStateTransitionWriteSet2[0])},
  {"transition.refresh-rebuild.manual-refresh",
   "refresh_rebuild",
   "AppState.volume[current].topology=current_generation",
   "manual_refresh_command_or_explicit_relog",
   "The active volume namespace is available and rebuild can complete or fail closed.",
   "Complete rebuild, advance volume_generation, then re-resolve panel snapshots by stable identity keys.",
   "Keep the previous topology and surface the refresh failure; do not apply partial row-index guesses.",
   "AppState.volume[current] has settled topology and panels are rebound or deterministically fallen back.",
   "Volume(shared topology)",
   "Increment volume_generation for any topology or visibility-set change; increment affected panel_generation after rebind/fallback.",
   kAppStateTransitionSideEffects3,
   sizeof(kAppStateTransitionSideEffects3) /
       sizeof(kAppStateTransitionSideEffects3[0]),
   "tree_view,file_view,stats,footer",
   "covered_by_transition_record",
   "Runtime migration must keep restore ordering: rebuild, generation advance, rebind/fallback, render.",
   kAppStateTransitionWriteSet3,
   sizeof(kAppStateTransitionWriteSet3) / sizeof(kAppStateTransitionWriteSet3[0])},
  {"transition.refresh-rebuild.watcher-live-refresh",
   "refresh_rebuild",
   "AppState.volume[current].topology=settled_generation awaiting watcher refresh",
   "watcher_live_refresh:settled_notification",
   "Watcher debounce/settle has completed for the active volume namespace and rebuild can complete or fail closed.",
   "Complete rebuild for the settled watcher notification, advance volume_generation, then re-resolve panel snapshots by stable identity keys.",
   "Keep the previous topology and surface the watcher refresh failure; do not apply partial row-index guesses.",
   "AppState.volume[current] reflects the settled watcher refresh and panels are rebound or deterministically fallen back.",
   "Volume(shared topology)",
   "Increment volume_generation for any watcher-settled topology or visibility-set change; increment affected panel_generation after rebind/fallback.",
   kAppStateTransitionSideEffects15,
   sizeof(kAppStateTransitionSideEffects15) /
       sizeof(kAppStateTransitionSideEffects15[0]),
   "tree_view,file_view,stats,footer",
   "covered_by_transition_record",
   "Runtime migration must keep watcher debounce/settle routing distinct from explicit manual refresh commands while preserving rebuild ordering.",
   kAppStateTransitionWriteSet3,
   sizeof(kAppStateTransitionWriteSet3) / sizeof(kAppStateTransitionWriteSet3[0])},
  {"transition.volume-operation.release-cycle",
   "volume_operation",
   "AppState.panel[active].volume_key=current",
   "volume_command:cycle_or_release",
   "Operation has a valid target volume and will not orphan required panel restore state.",
   "Update the active panel's volume binding or release the selected volume after safe ownership checks.",
   "Preserve the existing active volume binding and report the constraint when the operation cannot proceed.",
   "AppState session registry and active panel volume binding are consistent.",
   "ViewContext.volume_registry and YtreeNovaPanel(active)",
   "Increment panel_generation on binding changes; increment volume_generation on release/relog topology invalidation.",
   kAppStateTransitionSideEffects4,
   sizeof(kAppStateTransitionSideEffects4) /
       sizeof(kAppStateTransitionSideEffects4[0]),
   "layout,tree_view,file_view,stats,footer",
   "covered_by_transition_record",
   "Ensure inactive panels sharing a released volume use deterministic fallback instead of stale pointers.",
   kAppStateTransitionWriteSet4,
   sizeof(kAppStateTransitionWriteSet4) / sizeof(kAppStateTransitionWriteSet4[0])},
  {"transition.terminal-signal-resize",
   "terminal_signal_or_resize",
   "AppState.layout=current_geometry",
   "signal:SIGWINCH_or_resize_poll",
   "Signal handling has only set flags; resize work is executing in the main loop after curses can be called safely.",
   "Recompute layout geometry and project existing panel state into the new viewport bounds.",
   "Keep existing authoritative AppState records; if geometry is unusable, render only the safe degraded message surface.",
   "AppState.layout records current geometry while panel selection identities remain unchanged.",
   "ViewContext.layout_region",
   "Increment panel_generation only when viewport bounds correction changes saved origins; volume_generation is unchanged.",
   kAppStateTransitionSideEffects5,
   sizeof(kAppStateTransitionSideEffects5) /
       sizeof(kAppStateTransitionSideEffects5[0]),
   "full_screen_projection",
   "covered_by_transition_record",
   "Render reflow must remain projection-only and must not choose a new selection by row math.",
   kAppStateTransitionWriteSet5,
   sizeof(kAppStateTransitionWriteSet5) / sizeof(kAppStateTransitionWriteSet5[0])},
  {"transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "AppState.command=in_progress_filesystem_mutation",
   "mutation_result:success_or_failure",
   "Mutation result is complete and source/target paths are normalized within the intended namespace.",
   "On success, update affected topology/payload records and rebind panels by identity; on failure, preserve prior authority and report the failure.",
   "Do not apply partial topology changes; keep panel snapshots and volume_generation unchanged unless a verified rebuild follows.",
   "AppState.volume and affected panels reflect the completed mutation or the unchanged pre-mutation state.",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   "Increment volume_generation for mutations that change topology, identity, or visible payload; increment affected panel_generation after rebind/fallback.",
   kAppStateTransitionSideEffects6,
   sizeof(kAppStateTransitionSideEffects6) /
       sizeof(kAppStateTransitionSideEffects6[0]),
   "tree_view,file_view,stats,footer",
   "covered_by_transition_record",
   "Runtime migration must separate command side effects from AppState commit/rollback metadata.",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0])},
  {"transition.command-completion.user-command",
   "command_completion",
   "AppState.command=external_or_user_menu_command",
   "command_result:exit_status",
   "Command completion has a final status and any requested refresh/reload boundary is explicit.",
   "Record outcome message and schedule refresh/rebind only when the command contract declares filesystem impact.",
   "Preserve panel and volume state while reporting command failure or cancellation.",
   "AppState.command=idle with message and optional refresh transition queued.",
   "ViewContext.command_region",
   "No generation change unless a declared follow-up refresh/rebuild or panel focus change occurs.",
   kAppStateTransitionSideEffects7,
   sizeof(kAppStateTransitionSideEffects7) /
       sizeof(kAppStateTransitionSideEffects7[0]),
   "footer,stats_optional",
   "covered_by_transition_record",
   "Later runtime boundary should declare whether each command completion queues refresh_rebuild.",
   kAppStateTransitionWriteSet7,
   sizeof(kAppStateTransitionWriteSet7) / sizeof(kAppStateTransitionWriteSet7[0])},
  {"transition.rebuild-rebind-callback.panel-anchor",
   "rebuild_rebind_callback",
   "AppState.restore_snapshot=saved_generation_or_stale",
   "callback:post_rebuild_rebind",
   "Topology rebuild is settled and stable identity keys are available for re-resolution.",
   "Re-resolve exact identity or apply the deterministic fallback order, then commit the new panel anchor.",
   "Fail closed to root visible node only after exact/ancestor/sibling fallbacks are exhausted; never dereference stale pointers.",
   "AppState.panel snapshot matches current panel_generation and volume_generation.",
   "YtreeNovaPanel(affected) and Volume(current)",
   "Refresh snapshot generation markers after successful rebind/fallback; volume_generation is read, not advanced, by the callback.",
   kAppStateTransitionSideEffects8,
   sizeof(kAppStateTransitionSideEffects8) /
       sizeof(kAppStateTransitionSideEffects8[0]),
   "tree_view,file_view",
   "covered_by_transition_record",
   "Use panel_anchor helpers as the canonical restore boundary during runtime migration.",
   kAppStateTransitionWriteSet8,
   sizeof(kAppStateTransitionWriteSet8) / sizeof(kAppStateTransitionWriteSet8[0])},
  {"transition.render-reflow.project-state",
   "render_reflow",
   "AppState.render_invalidated=true",
   "render_tick:doupdate_ready",
   "Authoritative AppState records are settled before rendering begins.",
   "Compute temporary view projections and flush staged ncurses updates without mutating selection authority.",
   "Skip or degrade rendering; do not synthesize a new authoritative selection, viewport, or focus shape.",
   "Rendered screen reflects AppState; AppState selection and ownership fields are unchanged by projection.",
   "ViewContext.render_region",
   "No panel_generation or volume_generation change from render projection alone.",
   kAppStateTransitionSideEffects9,
   sizeof(kAppStateTransitionSideEffects9) /
       sizeof(kAppStateTransitionSideEffects9[0]),
   "cleared_after_successful_projection",
   "covered_by_transition_record",
   "Audit render paths so temporary projections cannot become restore authority.",
   kAppStateTransitionWriteSet9,
   sizeof(kAppStateTransitionWriteSet9) / sizeof(kAppStateTransitionWriteSet9[0])}
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
static const char *const kAppStateEventCoverageTriggerPaths0[] = {
  "Signal flag set outside curses work",
  "Main-loop resize/reflow handling",
};
static const char *const kAppStateEventCoverageTriggerPaths1[] = {
  "Manual refresh command",
  "Explicit relog of the current path",
};
static const char *const kAppStateEventCoverageTriggerPaths2[] = {
  "Post-refresh restore",
  "Post-mutation restore",
  "Visibility or topology generation mismatch rebind",
};
static const char *const kAppStateEventCoverageTriggerPaths3[] = {
  "Create directory result",
  "Copy or move result",
  "Delete or chmod-like result",
};
static const char *const kAppStateEventCoverageTriggerPaths4[] = {
  "Watcher notification",
  "Live refresh scheduling",
  "Settled topology refresh",
};
static const char *const kAppStateEventCoverageTriggerPaths5[] = {
  "External command completion",
  "User command menu completion",
  "Command failure or cancellation outcome",
};
static const char *const kAppStateEventCoverageTriggerPaths6[] = {
  "Modal Enter completion",
  "Modal Esc cancellation",
  "Neutral dialog dismissal",
};
static const char *const kAppStateEventCoverageTriggerPaths7[] = {
  "Cycle loaded volume",
  "Release volume",
  "Bind active panel to selected volume",
};
static const char *const kAppStateEventCoverageTriggerPaths8[] = {
  "Render dirty flag projection",
  "Layout reflow projection",
  "doupdate-ready render tick",
};
static const char *const kAppStateEventCoverageOwnerFieldRefs0[] = {
  "ctx.layout",
  "ctx.window_handles",
  "panel.tree_viewport_origin",
  "panel.file_viewport_origin",
  "panel.panel_generation",
};
static const char *const kAppStateEventCoverageOwnerFieldRefs1[] = {
  "volume.dir_tree",
  "volume.logged_state",
  "volume.volume_generation",
  "panel.restore_snapshot",
  "panel.panel_generation",
};
static const char *const kAppStateEventCoverageOwnerFieldRefs2[] = {
  "panel.tree_selection_key",
  "panel.file_selection_key",
  "panel.tree_cursor_pos",
  "panel.tree_viewport_origin",
  "panel.file_viewport_origin",
  "panel.panel_generation",
};
static const char *const kAppStateEventCoverageOwnerFieldRefs3[] = {
  "volume.dir_tree",
  "volume.payload_cache",
  "volume.volume_generation",
  "panel.restore_snapshot",
  "panel.panel_generation",
  "ctx.message_state",
};
static const char *const kAppStateEventCoverageOwnerFieldRefs4[] = {
  "ctx.command_state",
  "ctx.message_state",
  "ctx.pending_transition",
  "panel.panel_generation",
};
static const char *const kAppStateEventCoverageOwnerFieldRefs5[] = {
  "ctx.modal_state",
  "ctx.message_state",
  "panel.focus_shape",
  "panel.panel_generation",
};
static const char *const kAppStateEventCoverageOwnerFieldRefs6[] = {
  "ctx.volumes_head",
  "panel.volume_key",
  "panel.restore_snapshot",
  "panel.panel_generation",
  "volume.volume_generation",
};
static const char *const kAppStateEventCoverageOwnerFieldRefs7[] = {
  "ctx.render_dirty_flags",
  "ctx.window_handles",
};
static const char *const kAppStateEventCoverageTransitionSequenceRefs0[] = {
  "sequence.terminal-resize-reflow",
};
static const char *const kAppStateEventCoverageTransitionSequenceRefs1[] = {
  "sequence.refresh-rebuild",
};
static const char *const kAppStateEventCoverageTransitionSequenceRefs2[] = {
  "sequence.filesystem-mutation-result",
};
static const char *const kAppStateEventCoverageTransitionSequenceRefs3[] = {
  "sequence.search-jump",
};
static const char *const kAppStateEventCoverageTransitionSequenceRefs5[] = {
  "sequence.volume-cycling-release",
};
static const char *const kAppStateEventCoverageTransitionSequenceRefs6[] = {
  "sequence.render-reflow-projection",
};
static const char *const kAppStateEventCoverageTransitionSequenceRefs7[] = {
  "sequence.watcher-live-refresh",
};
static const char *const kAppStateEventCoverageTransitionSequenceRefs8[] = {
  "sequence.modal-completion",
};
static const char *const kAppStateEventCoverageDispatchSurfaceRefs0[] = {
  "surface.resize-signal-handling",
};
static const char *const kAppStateEventCoverageDispatchSurfaceRefs1[] = {
  "surface.refresh-rebuild-rebind",
};
static const char *const kAppStateEventCoverageDispatchSurfaceRefs2[] = {
  "surface.panel-anchor-rebind",
};
static const char *const kAppStateEventCoverageDispatchSurfaceRefs3[] = {
  "surface.filesystem-mutation-result",
};
static const char *const kAppStateEventCoverageDispatchSurfaceRefs4[] = {
  "surface.watcher-live-refresh",
};
static const char *const kAppStateEventCoverageDispatchSurfaceRefs5[] = {
  "surface.command-completion-dispatch",
};
static const char *const kAppStateEventCoverageDispatchSurfaceRefs6[] = {
  "surface.modal-completion-event",
};
static const char *const kAppStateEventCoverageDispatchSurfaceRefs7[] = {
  "surface.volume-operation",
};
static const char *const kAppStateEventCoverageDispatchSurfaceRefs8[] = {
  "surface.render-reflow-projection",
};
static const char *const kAppStateEventCoverageInvariantRefs0[] = {
  "invariant.render-projection-read-only",
};
static const char *const kAppStateEventCoverageInvariantRefs1[] = {
  "invariant.hidden-entry-visible-navigation",
};
static const char *const kAppStateEventCoverageInvariantRefs2[] = {
  "invariant.hidden-entry-visible-navigation",
  "invariant.viewport-identity-rebind",
};
static const char *const kAppStateEventCoverageInvariantRefs3[] = {
  "invariant.blocked-transition-determinism",
};
static const char *const kAppStateEventCoverageInvariantRefs4[] = {
  "invariant.panel-local-focus-restore",
  "invariant.blocked-transition-determinism",
};
static const char *const kAppStateEventCoverageInvariantRefs5[] = {
  "invariant.shared-state-panel-local-isolation",
};
static const char *const kAppStateEventCoverageGenerationDomainRefs0[] = {
  "generation.panel.local-authority",
  "reflow.layout.projection",
};
static const char *const kAppStateEventCoverageGenerationDomainRefs1[] = {
  "generation.panel.local-authority",
  "generation.volume.shared-authority",
  "identity.directory.stable-key",
  "identity.file.stable-key",
  "state.visibility-filter.panel-volume",
  "state.topology.volume",
  "state.file-payload.volume",
  "lifecycle.volume.registry",
};
static const char *const kAppStateEventCoverageGenerationDomainRefs2[] = {
  "generation.panel.local-authority",
  "identity.directory.stable-key",
  "identity.file.stable-key",
  "shape.panel.focus",
  "state.visibility-filter.panel-volume",
  "state.file-payload.volume",
};
static const char *const kAppStateEventCoverageGenerationDomainRefs3[] = {
  "generation.panel.local-authority",
  "generation.volume.shared-authority",
  "identity.directory.stable-key",
  "identity.file.stable-key",
  "state.topology.volume",
  "state.file-payload.volume",
};
static const char *const kAppStateEventCoverageGenerationDomainRefs4[] = {
  "target.modal-command.session",
};
static const char *const kAppStateEventCoverageGenerationDomainRefs5[] = {
  "generation.panel.local-authority",
  "shape.panel.focus",
  "target.modal-command.session",
};
static const char *const kAppStateEventCoverageGenerationDomainRefs6[] = {
  "generation.panel.local-authority",
  "generation.volume.shared-authority",
  "identity.directory.stable-key",
  "state.topology.volume",
  "lifecycle.volume.registry",
};
static const char *const kAppStateEventCoverageGenerationDomainRefs7[] = {
  "reflow.layout.projection",
};
static const char *const kAppStateEventCoverageDiffHarnessRefs0[] = {
  "harness.generation-mismatch-check",
};
static const char *const kAppStateEventCoverageDiffHarnessRefs1[] = {
  "harness.transition-before-after-snapshot",
  "harness.generation-mismatch-check",
};
static const char *const kAppStateEventCoverageDiffHarnessRefs2[] = {
  "harness.transition-before-after-snapshot",
  "harness.generation-mismatch-check",
};
static const char *const kAppStateEventCoverageDiffHarnessRefs3[] = {
  "harness.transition-before-after-snapshot",
  "harness.generation-mismatch-check",
  "harness.blocked-transition-no-unrelated-mutation",
};
static const char *const kAppStateEventCoverageDiffHarnessRefs4[] = {
  "harness.declared-write-set-diff",
  "harness.blocked-transition-no-unrelated-mutation",
};
static const char *const kAppStateEventCoverageDiffHarnessRefs5[] = {
  "harness.declared-write-set-diff",
  "harness.blocked-transition-no-unrelated-mutation",
};
static const char *const kAppStateEventCoverageDiffHarnessRefs6[] = {
  "harness.transition-before-after-snapshot",
  "harness.blocked-transition-no-unrelated-mutation",
};
static const char *const kAppStateEventCoverageDiffHarnessRefs7[] = {
  "harness.render-projection-read-only-diff",
};

static const char *const kAppStateEventCoverageMigrationNotes0[] = {
  "Signal handlers may only set flags; resize commits through the main-loop transition boundary.",
};
static const char *const kAppStateEventCoverageMigrationNotes1[] = {
  "Rebuild must settle topology, advance generation, then rebind panels by stable identity.",
};
static const char *const kAppStateEventCoverageMigrationNotes2[] = {
  "Callback coverage points to the existing rebuild/rebind transition rather than inventing a separate runtime event.",
};
static const char *const kAppStateEventCoverageMigrationNotes3[] = {
  "Command side effects remain outside AppState commit; only completed results may update AppState metadata.",
};
static const char *const kAppStateEventCoverageMigrationNotes4[] = {
  "Watcher/live-refresh now uses its own explicit covered transition record rather than borrowing the manual refresh boundary.",
};
static const char *const kAppStateEventCoverageMigrationNotes5[] = {
  "Command completion may schedule refresh only when the command contract declares filesystem impact.",
};
static const char *const kAppStateEventCoverageMigrationNotes6[] = {
  "Modal completion now uses its own explicit covered transition record while destructive confirmations remain governed by their dedicated command transitions.",
};
static const char *const kAppStateEventCoverageMigrationNotes7[] = {
  "Lifecycle coverage keeps inactive panels from inheriting stale volume pointers during migration.",
};
static const char *const kAppStateEventCoverageMigrationNotes8[] = {
  "Render/reflow is projection-only and must not become restore authority.",
};

static const AppStateEventCoverageMetadata
    kAppStateEventCoverages[APPSTATE_EVENT_COVERAGE_COUNT] = {
  {"event.terminal-resize-signal",
   "terminal_resize_signal",
   "transition.terminal-signal-resize",
   "terminal_signal_or_resize",
   "SIGWINCH flag handling and resize polling in the main loop",
   "ViewContext.layout_region",
   kAppStateTransitionWriteSet5,
   sizeof(kAppStateTransitionWriteSet5) / sizeof(kAppStateTransitionWriteSet5[0]),
   kAppStateEventCoverageOwnerFieldRefs0,
   sizeof(kAppStateEventCoverageOwnerFieldRefs0) / sizeof(kAppStateEventCoverageOwnerFieldRefs0[0]),
   "covered_by_transition_record",
   kAppStateEventCoverageTriggerPaths0,
   sizeof(kAppStateEventCoverageTriggerPaths0) / sizeof(kAppStateEventCoverageTriggerPaths0[0]),
   kAppStateEventCoverageTransitionSequenceRefs0,
   sizeof(kAppStateEventCoverageTransitionSequenceRefs0) / sizeof(kAppStateEventCoverageTransitionSequenceRefs0[0]),
   kAppStateEventCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateEventCoverageDispatchSurfaceRefs0) / sizeof(kAppStateEventCoverageDispatchSurfaceRefs0[0]),
   kAppStateEventCoverageInvariantRefs0,
   sizeof(kAppStateEventCoverageInvariantRefs0) / sizeof(kAppStateEventCoverageInvariantRefs0[0]),
   kAppStateEventCoverageGenerationDomainRefs0,
   sizeof(kAppStateEventCoverageGenerationDomainRefs0) / sizeof(kAppStateEventCoverageGenerationDomainRefs0[0]),
   kAppStateEventCoverageDiffHarnessRefs0,
   sizeof(kAppStateEventCoverageDiffHarnessRefs0) / sizeof(kAppStateEventCoverageDiffHarnessRefs0[0]),
   kAppStateEventCoverageMigrationNotes0,
   sizeof(kAppStateEventCoverageMigrationNotes0) / sizeof(kAppStateEventCoverageMigrationNotes0[0])},
  {"event.refresh-rebuild",
   "refresh_rebuild",
   "transition.refresh-rebuild.manual-refresh",
   "refresh_rebuild",
   "Manual refresh, explicit relog, or declared refresh boundary",
   "Volume(shared topology)",
   kAppStateTransitionWriteSet3,
   sizeof(kAppStateTransitionWriteSet3) / sizeof(kAppStateTransitionWriteSet3[0]),
   kAppStateEventCoverageOwnerFieldRefs1,
   sizeof(kAppStateEventCoverageOwnerFieldRefs1) / sizeof(kAppStateEventCoverageOwnerFieldRefs1[0]),
   "covered_by_transition_record",
   kAppStateEventCoverageTriggerPaths1,
   sizeof(kAppStateEventCoverageTriggerPaths1) / sizeof(kAppStateEventCoverageTriggerPaths1[0]),
   kAppStateEventCoverageTransitionSequenceRefs1,
   sizeof(kAppStateEventCoverageTransitionSequenceRefs1) / sizeof(kAppStateEventCoverageTransitionSequenceRefs1[0]),
   kAppStateEventCoverageDispatchSurfaceRefs1,
   sizeof(kAppStateEventCoverageDispatchSurfaceRefs1) / sizeof(kAppStateEventCoverageDispatchSurfaceRefs1[0]),
   kAppStateEventCoverageInvariantRefs1,
   sizeof(kAppStateEventCoverageInvariantRefs1) / sizeof(kAppStateEventCoverageInvariantRefs1[0]),
   kAppStateEventCoverageGenerationDomainRefs1,
   sizeof(kAppStateEventCoverageGenerationDomainRefs1) / sizeof(kAppStateEventCoverageGenerationDomainRefs1[0]),
   kAppStateEventCoverageDiffHarnessRefs1,
   sizeof(kAppStateEventCoverageDiffHarnessRefs1) / sizeof(kAppStateEventCoverageDiffHarnessRefs1[0]),
   kAppStateEventCoverageMigrationNotes1,
   sizeof(kAppStateEventCoverageMigrationNotes1) / sizeof(kAppStateEventCoverageMigrationNotes1[0])},
  {"event.rebuild-rebind-callback",
   "rebuild_rebind_callback",
   "transition.rebuild-rebind-callback.panel-anchor",
   "rebuild_rebind_callback",
   "Post-rebuild panel anchor re-resolution callback",
   "YtreeNovaPanel(affected) and Volume(current)",
   kAppStateTransitionWriteSet8,
   sizeof(kAppStateTransitionWriteSet8) / sizeof(kAppStateTransitionWriteSet8[0]),
   kAppStateEventCoverageOwnerFieldRefs2,
   sizeof(kAppStateEventCoverageOwnerFieldRefs2) / sizeof(kAppStateEventCoverageOwnerFieldRefs2[0]),
   "covered_by_transition_record",
   kAppStateEventCoverageTriggerPaths2,
   sizeof(kAppStateEventCoverageTriggerPaths2) / sizeof(kAppStateEventCoverageTriggerPaths2[0]),
   kAppStateEventCoverageTransitionSequenceRefs1,
   sizeof(kAppStateEventCoverageTransitionSequenceRefs1) / sizeof(kAppStateEventCoverageTransitionSequenceRefs1[0]),
   kAppStateEventCoverageDispatchSurfaceRefs2,
   sizeof(kAppStateEventCoverageDispatchSurfaceRefs2) / sizeof(kAppStateEventCoverageDispatchSurfaceRefs2[0]),
   kAppStateEventCoverageInvariantRefs2,
   sizeof(kAppStateEventCoverageInvariantRefs2) / sizeof(kAppStateEventCoverageInvariantRefs2[0]),
   kAppStateEventCoverageGenerationDomainRefs2,
   sizeof(kAppStateEventCoverageGenerationDomainRefs2) / sizeof(kAppStateEventCoverageGenerationDomainRefs2[0]),
   kAppStateEventCoverageDiffHarnessRefs2,
   sizeof(kAppStateEventCoverageDiffHarnessRefs2) / sizeof(kAppStateEventCoverageDiffHarnessRefs2[0]),
   kAppStateEventCoverageMigrationNotes2,
   sizeof(kAppStateEventCoverageMigrationNotes2) / sizeof(kAppStateEventCoverageMigrationNotes2[0])},
  {"event.filesystem-mutation-result",
   "filesystem_mutation_result",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Completed filesystem mutation command result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateEventCoverageOwnerFieldRefs3,
   sizeof(kAppStateEventCoverageOwnerFieldRefs3) / sizeof(kAppStateEventCoverageOwnerFieldRefs3[0]),
   "covered_by_transition_record",
   kAppStateEventCoverageTriggerPaths3,
   sizeof(kAppStateEventCoverageTriggerPaths3) / sizeof(kAppStateEventCoverageTriggerPaths3[0]),
   kAppStateEventCoverageTransitionSequenceRefs2,
   sizeof(kAppStateEventCoverageTransitionSequenceRefs2) / sizeof(kAppStateEventCoverageTransitionSequenceRefs2[0]),
   kAppStateEventCoverageDispatchSurfaceRefs3,
   sizeof(kAppStateEventCoverageDispatchSurfaceRefs3) / sizeof(kAppStateEventCoverageDispatchSurfaceRefs3[0]),
   kAppStateEventCoverageInvariantRefs3,
   sizeof(kAppStateEventCoverageInvariantRefs3) / sizeof(kAppStateEventCoverageInvariantRefs3[0]),
   kAppStateEventCoverageGenerationDomainRefs3,
   sizeof(kAppStateEventCoverageGenerationDomainRefs3) / sizeof(kAppStateEventCoverageGenerationDomainRefs3[0]),
   kAppStateEventCoverageDiffHarnessRefs3,
   sizeof(kAppStateEventCoverageDiffHarnessRefs3) / sizeof(kAppStateEventCoverageDiffHarnessRefs3[0]),
   kAppStateEventCoverageMigrationNotes3,
   sizeof(kAppStateEventCoverageMigrationNotes3) / sizeof(kAppStateEventCoverageMigrationNotes3[0])},
  {"event.watcher-live-refresh",
   "watcher_live_refresh",
   "transition.refresh-rebuild.watcher-live-refresh",
   "refresh_rebuild",
   "Filesystem watcher or live-refresh notification after debounce/settle",
   "Volume(shared topology)",
   kAppStateTransitionWriteSet3,
   sizeof(kAppStateTransitionWriteSet3) / sizeof(kAppStateTransitionWriteSet3[0]),
   kAppStateEventCoverageOwnerFieldRefs1,
   sizeof(kAppStateEventCoverageOwnerFieldRefs1) / sizeof(kAppStateEventCoverageOwnerFieldRefs1[0]),
   "covered_by_transition_record",
   kAppStateEventCoverageTriggerPaths4,
   sizeof(kAppStateEventCoverageTriggerPaths4) / sizeof(kAppStateEventCoverageTriggerPaths4[0]),
   kAppStateEventCoverageTransitionSequenceRefs7,
   sizeof(kAppStateEventCoverageTransitionSequenceRefs7) / sizeof(kAppStateEventCoverageTransitionSequenceRefs7[0]),
   kAppStateEventCoverageDispatchSurfaceRefs4,
   sizeof(kAppStateEventCoverageDispatchSurfaceRefs4) / sizeof(kAppStateEventCoverageDispatchSurfaceRefs4[0]),
   kAppStateEventCoverageInvariantRefs1,
   sizeof(kAppStateEventCoverageInvariantRefs1) / sizeof(kAppStateEventCoverageInvariantRefs1[0]),
   kAppStateEventCoverageGenerationDomainRefs1,
   sizeof(kAppStateEventCoverageGenerationDomainRefs1) / sizeof(kAppStateEventCoverageGenerationDomainRefs1[0]),
   kAppStateEventCoverageDiffHarnessRefs1,
   sizeof(kAppStateEventCoverageDiffHarnessRefs1) / sizeof(kAppStateEventCoverageDiffHarnessRefs1[0]),
   kAppStateEventCoverageMigrationNotes4,
   sizeof(kAppStateEventCoverageMigrationNotes4) / sizeof(kAppStateEventCoverageMigrationNotes4[0])},
  {"event.command-completion",
   "command_completion",
   "transition.command-completion.user-command",
   "command_completion",
   "External or user command exit-status completion",
   "ViewContext.command_region",
   kAppStateTransitionWriteSet7,
   sizeof(kAppStateTransitionWriteSet7) / sizeof(kAppStateTransitionWriteSet7[0]),
   kAppStateEventCoverageOwnerFieldRefs4,
   sizeof(kAppStateEventCoverageOwnerFieldRefs4) / sizeof(kAppStateEventCoverageOwnerFieldRefs4[0]),
   "covered_by_transition_record",
   kAppStateEventCoverageTriggerPaths5,
   sizeof(kAppStateEventCoverageTriggerPaths5) / sizeof(kAppStateEventCoverageTriggerPaths5[0]),
   kAppStateEventCoverageTransitionSequenceRefs3,
   sizeof(kAppStateEventCoverageTransitionSequenceRefs3) / sizeof(kAppStateEventCoverageTransitionSequenceRefs3[0]),
   kAppStateEventCoverageDispatchSurfaceRefs5,
   sizeof(kAppStateEventCoverageDispatchSurfaceRefs5) / sizeof(kAppStateEventCoverageDispatchSurfaceRefs5[0]),
   kAppStateEventCoverageInvariantRefs3,
   sizeof(kAppStateEventCoverageInvariantRefs3) / sizeof(kAppStateEventCoverageInvariantRefs3[0]),
   kAppStateEventCoverageGenerationDomainRefs4,
   sizeof(kAppStateEventCoverageGenerationDomainRefs4) / sizeof(kAppStateEventCoverageGenerationDomainRefs4[0]),
   kAppStateEventCoverageDiffHarnessRefs4,
   sizeof(kAppStateEventCoverageDiffHarnessRefs4) / sizeof(kAppStateEventCoverageDiffHarnessRefs4[0]),
   kAppStateEventCoverageMigrationNotes5,
   sizeof(kAppStateEventCoverageMigrationNotes5) / sizeof(kAppStateEventCoverageMigrationNotes5[0])},
  {"event.modal-completion",
   "modal_completion",
   "transition.modal-action.completion",
   "modal_action",
   "Modal prompt, menu, or dialog completion",
   "ViewContext.modal_region",
   kAppStateTransitionWriteSet2,
   sizeof(kAppStateTransitionWriteSet2) / sizeof(kAppStateTransitionWriteSet2[0]),
   kAppStateEventCoverageOwnerFieldRefs5,
   sizeof(kAppStateEventCoverageOwnerFieldRefs5) / sizeof(kAppStateEventCoverageOwnerFieldRefs5[0]),
   "covered_by_transition_record",
   kAppStateEventCoverageTriggerPaths6,
   sizeof(kAppStateEventCoverageTriggerPaths6) / sizeof(kAppStateEventCoverageTriggerPaths6[0]),
   kAppStateEventCoverageTransitionSequenceRefs8,
   sizeof(kAppStateEventCoverageTransitionSequenceRefs8) / sizeof(kAppStateEventCoverageTransitionSequenceRefs8[0]),
   kAppStateEventCoverageDispatchSurfaceRefs6,
   sizeof(kAppStateEventCoverageDispatchSurfaceRefs6) / sizeof(kAppStateEventCoverageDispatchSurfaceRefs6[0]),
   kAppStateEventCoverageInvariantRefs4,
   sizeof(kAppStateEventCoverageInvariantRefs4) / sizeof(kAppStateEventCoverageInvariantRefs4[0]),
   kAppStateEventCoverageGenerationDomainRefs5,
   sizeof(kAppStateEventCoverageGenerationDomainRefs5) / sizeof(kAppStateEventCoverageGenerationDomainRefs5[0]),
   kAppStateEventCoverageDiffHarnessRefs5,
   sizeof(kAppStateEventCoverageDiffHarnessRefs5) / sizeof(kAppStateEventCoverageDiffHarnessRefs5[0]),
   kAppStateEventCoverageMigrationNotes6,
   sizeof(kAppStateEventCoverageMigrationNotes6) / sizeof(kAppStateEventCoverageMigrationNotes6[0])},
  {"event.volume-lifecycle",
   "volume_lifecycle",
   "transition.volume-operation.release-cycle",
   "volume_operation",
   "Volume cycle, release, or loaded-volume selection lifecycle event",
   "ViewContext.volume_registry and YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet4,
   sizeof(kAppStateTransitionWriteSet4) / sizeof(kAppStateTransitionWriteSet4[0]),
   kAppStateEventCoverageOwnerFieldRefs6,
   sizeof(kAppStateEventCoverageOwnerFieldRefs6) / sizeof(kAppStateEventCoverageOwnerFieldRefs6[0]),
   "covered_by_transition_record",
   kAppStateEventCoverageTriggerPaths7,
   sizeof(kAppStateEventCoverageTriggerPaths7) / sizeof(kAppStateEventCoverageTriggerPaths7[0]),
   kAppStateEventCoverageTransitionSequenceRefs5,
   sizeof(kAppStateEventCoverageTransitionSequenceRefs5) / sizeof(kAppStateEventCoverageTransitionSequenceRefs5[0]),
   kAppStateEventCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateEventCoverageDispatchSurfaceRefs7) / sizeof(kAppStateEventCoverageDispatchSurfaceRefs7[0]),
   kAppStateEventCoverageInvariantRefs5,
   sizeof(kAppStateEventCoverageInvariantRefs5) / sizeof(kAppStateEventCoverageInvariantRefs5[0]),
   kAppStateEventCoverageGenerationDomainRefs6,
   sizeof(kAppStateEventCoverageGenerationDomainRefs6) / sizeof(kAppStateEventCoverageGenerationDomainRefs6[0]),
   kAppStateEventCoverageDiffHarnessRefs6,
   sizeof(kAppStateEventCoverageDiffHarnessRefs6) / sizeof(kAppStateEventCoverageDiffHarnessRefs6[0]),
   kAppStateEventCoverageMigrationNotes7,
   sizeof(kAppStateEventCoverageMigrationNotes7) / sizeof(kAppStateEventCoverageMigrationNotes7[0])},
  {"event.render-reflow",
   "render_reflow",
   "transition.render-reflow.project-state",
   "render_reflow",
   "Render invalidation projection and doupdate-ready reflow",
   "ViewContext.render_region",
   kAppStateTransitionWriteSet9,
   sizeof(kAppStateTransitionWriteSet9) / sizeof(kAppStateTransitionWriteSet9[0]),
   kAppStateEventCoverageOwnerFieldRefs7,
   sizeof(kAppStateEventCoverageOwnerFieldRefs7) / sizeof(kAppStateEventCoverageOwnerFieldRefs7[0]),
   "covered_by_transition_record",
   kAppStateEventCoverageTriggerPaths8,
   sizeof(kAppStateEventCoverageTriggerPaths8) / sizeof(kAppStateEventCoverageTriggerPaths8[0]),
   kAppStateEventCoverageTransitionSequenceRefs6,
   sizeof(kAppStateEventCoverageTransitionSequenceRefs6) / sizeof(kAppStateEventCoverageTransitionSequenceRefs6[0]),
   kAppStateEventCoverageDispatchSurfaceRefs8,
   sizeof(kAppStateEventCoverageDispatchSurfaceRefs8) / sizeof(kAppStateEventCoverageDispatchSurfaceRefs8[0]),
   kAppStateEventCoverageInvariantRefs0,
   sizeof(kAppStateEventCoverageInvariantRefs0) / sizeof(kAppStateEventCoverageInvariantRefs0[0]),
   kAppStateEventCoverageGenerationDomainRefs7,
   sizeof(kAppStateEventCoverageGenerationDomainRefs7) / sizeof(kAppStateEventCoverageGenerationDomainRefs7[0]),
   kAppStateEventCoverageDiffHarnessRefs7,
   sizeof(kAppStateEventCoverageDiffHarnessRefs7) / sizeof(kAppStateEventCoverageDiffHarnessRefs7[0]),
   kAppStateEventCoverageMigrationNotes8,
   sizeof(kAppStateEventCoverageMigrationNotes8) / sizeof(kAppStateEventCoverageMigrationNotes8[0])},
};

static const char *const kAppStateActionCoverageMigrationNotes0[] = {
  "Runtime action coverage is validated through the shared keybinding transition record and decoded-action dispatch metadata.",
};
static const char *const kAppStateActionCoverageMigrationNotes1[] = {
  "Esc dismissal is validated through the modal_action dismiss transition record; non-modal Esc no-op behavior remains a blocked keybinding outcome.",
};
static const char *const kAppStateActionCoverageMigrationNotes2[] = {
  "Runtime action coverage is validated through the shared refresh/rebuild transition record and refresh dispatch metadata.",
};
static const char *const kAppStateActionCoverageMigrationNotes3[] = {
  "Runtime action coverage is validated through the shared command-completion transition record and command dispatch metadata.",
};
static const char *const kAppStateActionCoverageMigrationNotes4[] = {
  "Runtime action coverage is validated through the terminal resize transition record; signal handlers must only set flags before this transition commits.",
};
static const char *const kAppStateActionCoverageMigrationNotes5[] = {
  "Runtime action coverage is validated through the volume menu selection transition record and menu dispatch metadata.",
};
static const char *const kAppStateActionCoverageMigrationNotes6[] = {
  "Runtime action coverage is validated through the shared volume-operation transition record and volume dispatch metadata.",
};
static const char *const kAppStateActionCoverageMigrationNotes7[] = {
  "Runtime action coverage is validated through the shared filesystem-mutation-result transition record; per-operation command metadata remains a future refinement.",
};
static const char *const kAppStateActionCoverageOwnerFieldRefs0[] = {
  "panel.tree_selection_key",
  "panel.tree_cursor_pos",
  "panel.tree_viewport_origin",
  "panel.focus_shape",
  "panel.panel_generation",
};
static const char *const kAppStateActionCoverageOwnerFieldRefs1[] = {
  "ctx.modal_state",
  "ctx.message_state",
  "panel.focus_shape",
  "panel.panel_generation",
};
static const char *const kAppStateActionCoverageOwnerFieldRefs2[] = {
  "volume.dir_tree",
  "volume.logged_state",
  "volume.volume_generation",
  "panel.restore_snapshot",
  "panel.panel_generation",
};
static const char *const kAppStateActionCoverageOwnerFieldRefs3[] = {
  "ctx.command_state",
  "ctx.message_state",
  "ctx.pending_transition",
  "panel.panel_generation",
};
static const char *const kAppStateActionCoverageOwnerFieldRefs4[] = {
  "ctx.layout",
  "ctx.window_handles",
  "panel.tree_viewport_origin",
  "panel.file_viewport_origin",
  "panel.panel_generation",
};
static const char *const kAppStateActionCoverageOwnerFieldRefs5[] = {
  "ctx.active",
  "panel.volume_key",
  "panel.restore_snapshot",
  "panel.panel_generation",
};
static const char *const kAppStateActionCoverageOwnerFieldRefs6[] = {
  "ctx.volumes_head",
  "panel.volume_key",
  "panel.restore_snapshot",
  "panel.panel_generation",
  "volume.volume_generation",
};
static const char *const kAppStateActionCoverageOwnerFieldRefs7[] = {
  "volume.dir_tree",
  "volume.payload_cache",
  "volume.volume_generation",
  "panel.restore_snapshot",
  "panel.panel_generation",
  "ctx.message_state",
};
static const char *const kAppStateActionCoverageTransitionSequenceRefs0[] = {
  "sequence.dotfile-reveal-conceal",
  "sequence.enter-directory-file-transition",
  "sequence.file-small-big-transitions",
  "sequence.search-jump",
  "sequence.showall-global-tagged-only",
  "sequence.split-close-reopen",
  "sequence.split-toggle-f8",
  "sequence.tab-panel-switch",
};
static const char *const kAppStateActionCoverageTransitionSequenceRefs1[] = {
  "sequence.esc-modal-dismissal",
};
static const char *const kAppStateActionCoverageTransitionSequenceRefs2[] = {
  "sequence.refresh-rebuild",
};
static const char *const kAppStateActionCoverageTransitionSequenceRefs3[] = {
  "sequence.search-jump",
};
static const char *const kAppStateActionCoverageTransitionSequenceRefs4[] = {
  "sequence.terminal-resize-reflow",
};
static const char *const kAppStateActionCoverageTransitionSequenceRefs5[] = {
  "sequence.volume-menu-select",
};
static const char *const kAppStateActionCoverageTransitionSequenceRefs6[] = {
  "sequence.volume-cycling-release",
};
static const char *const kAppStateActionCoverageTransitionSequenceRefs7[] = {
  "sequence.filesystem-mutation-result",
};
static const char *const kAppStateActionCoverageDispatchSurfaceRefs0[] = {
  "surface.key-decode-input-dispatch",
  "surface.directory-window-action-dispatch",
  "surface.file-window-action-dispatch",
};
static const char *const kAppStateActionCoverageDispatchSurfaceRefs1[] = {
  "surface.menu-modal-completion",
};
static const char *const kAppStateActionCoverageDispatchSurfaceRefs2[] = {
  "surface.refresh-rebuild-rebind",
};
static const char *const kAppStateActionCoverageDispatchSurfaceRefs3[] = {
  "surface.command-completion-dispatch",
};
static const char *const kAppStateActionCoverageDispatchSurfaceRefs4[] = {
  "surface.resize-signal-handling",
};
static const char *const kAppStateActionCoverageDispatchSurfaceRefs5[] = {
  "surface.volume-menu-selection",
};
static const char *const kAppStateActionCoverageDispatchSurfaceRefs6[] = {
  "surface.volume-operation",
};
static const char *const kAppStateActionCoverageDispatchSurfaceRefs7[] = {
  "surface.filesystem-mutation-result",
};
static const char *const kAppStateActionCoverageInvariantRefs0[] = {
  "invariant.inactive-panel-frozen",
};
static const char *const kAppStateActionCoverageInvariantRefs1[] = {
  "invariant.panel-local-focus-restore",
  "invariant.blocked-transition-determinism",
};
static const char *const kAppStateActionCoverageInvariantRefs2[] = {
  "invariant.hidden-entry-visible-navigation",
};
static const char *const kAppStateActionCoverageInvariantRefs3[] = {
  "invariant.blocked-transition-determinism",
};
static const char *const kAppStateActionCoverageInvariantRefs4[] = {
  "invariant.render-projection-read-only",
};
static const char *const kAppStateActionCoverageInvariantRefs5[] = {
  "invariant.shared-state-panel-local-isolation",
};
static const char *const kAppStateActionCoverageGenerationDomainRefs0[] = {
  "generation.panel.local-authority",
  "shape.panel.focus",
  "state.visibility-filter.panel-volume",
};
static const char *const kAppStateActionCoverageGenerationDomainRefs1[] = {
  "generation.panel.local-authority",
  "shape.panel.focus",
  "target.modal-command.session",
};
static const char *const kAppStateActionCoverageGenerationDomainRefs2[] = {
  "generation.panel.local-authority",
  "generation.volume.shared-authority",
  "identity.directory.stable-key",
  "identity.file.stable-key",
  "state.visibility-filter.panel-volume",
  "state.topology.volume",
  "state.file-payload.volume",
  "lifecycle.volume.registry",
};
static const char *const kAppStateActionCoverageGenerationDomainRefs3[] = {
  "target.modal-command.session",
};
static const char *const kAppStateActionCoverageGenerationDomainRefs4[] = {
  "generation.panel.local-authority",
  "reflow.layout.projection",
};
static const char *const kAppStateActionCoverageGenerationDomainRefs5[] = {
  "generation.panel.local-authority",
  "shape.panel.focus",
  "lifecycle.volume.registry",
};
static const char *const kAppStateActionCoverageGenerationDomainRefs6[] = {
  "generation.panel.local-authority",
  "generation.volume.shared-authority",
  "identity.directory.stable-key",
  "state.topology.volume",
  "lifecycle.volume.registry",
};
static const char *const kAppStateActionCoverageGenerationDomainRefs7[] = {
  "generation.panel.local-authority",
  "generation.volume.shared-authority",
  "identity.directory.stable-key",
  "identity.file.stable-key",
  "state.topology.volume",
  "state.file-payload.volume",
};

static const char *const kAppStateActionCoverageDiffHarnessRefs0[] = {
  "harness.transition-before-after-snapshot",
  "harness.declared-write-set-diff",
  "harness.blocked-transition-no-unrelated-mutation",
};
static const char *const kAppStateActionCoverageDiffHarnessRefs1[] = {
  "harness.declared-write-set-diff",
  "harness.blocked-transition-no-unrelated-mutation",
};
static const char *const kAppStateActionCoverageDiffHarnessRefs2[] = {
  "harness.transition-before-after-snapshot",
  "harness.generation-mismatch-check",
};
static const char *const kAppStateActionCoverageDiffHarnessRefs3[] = {
  "harness.declared-write-set-diff",
  "harness.blocked-transition-no-unrelated-mutation",
};
static const char *const kAppStateActionCoverageDiffHarnessRefs4[] = {
  "harness.generation-mismatch-check",
};
static const char *const kAppStateActionCoverageDiffHarnessRefs5[] = {
  "harness.declared-write-set-diff",
};
static const char *const kAppStateActionCoverageDiffHarnessRefs6[] = {
  "harness.transition-before-after-snapshot",
  "harness.blocked-transition-no-unrelated-mutation",
};
static const char *const kAppStateActionCoverageDiffHarnessRefs7[] = {
  "harness.transition-before-after-snapshot",
  "harness.generation-mismatch-check",
  "harness.blocked-transition-no-unrelated-mutation",
};

static const AppStateActionCoverageMetadata
    kAppStateActionCoverages[APPSTATE_ACTION_COVERAGE_COUNT] = {
  {ACTION_NONE,
   "ACTION_NONE",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_MOVE_UP,
   "ACTION_MOVE_UP",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_MOVE_DOWN,
   "ACTION_MOVE_DOWN",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_MOVE_SIBLING_NEXT,
   "ACTION_MOVE_SIBLING_NEXT",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_MOVE_SIBLING_PREV,
   "ACTION_MOVE_SIBLING_PREV",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_MOVE_LEFT,
   "ACTION_MOVE_LEFT",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_MOVE_RIGHT,
   "ACTION_MOVE_RIGHT",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_PAGE_UP,
   "ACTION_PAGE_UP",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_PAGE_DOWN,
   "ACTION_PAGE_DOWN",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_HOME,
   "ACTION_HOME",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_END,
   "ACTION_END",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_TREE_EXPAND,
   "ACTION_TREE_EXPAND",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_TREE_COLLAPSE,
   "ACTION_TREE_COLLAPSE",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_TREE_EXPAND_ALL,
   "ACTION_TREE_EXPAND_ALL",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_ENTER,
   "ACTION_ENTER",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_ESCAPE,
   "ACTION_ESCAPE",
   "transition.modal-action.dismiss",
   "modal_action",
   "ViewContext.modal_region",
   kAppStateTransitionWriteSet2,
   sizeof(kAppStateTransitionWriteSet2) / sizeof(kAppStateTransitionWriteSet2[0]),
   kAppStateActionCoverageOwnerFieldRefs1,
   sizeof(kAppStateActionCoverageOwnerFieldRefs1) / sizeof(kAppStateActionCoverageOwnerFieldRefs1[0]),
   kAppStateActionCoverageTransitionSequenceRefs1,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs1) / sizeof(kAppStateActionCoverageTransitionSequenceRefs1[0]),
   kAppStateActionCoverageDispatchSurfaceRefs1,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs1) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs1[0]),
   kAppStateActionCoverageInvariantRefs1,
   sizeof(kAppStateActionCoverageInvariantRefs1) / sizeof(kAppStateActionCoverageInvariantRefs1[0]),
   kAppStateActionCoverageGenerationDomainRefs1,
   sizeof(kAppStateActionCoverageGenerationDomainRefs1) / sizeof(kAppStateActionCoverageGenerationDomainRefs1[0]),
   kAppStateActionCoverageDiffHarnessRefs1,
   sizeof(kAppStateActionCoverageDiffHarnessRefs1) / sizeof(kAppStateActionCoverageDiffHarnessRefs1[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes1,
   sizeof(kAppStateActionCoverageMigrationNotes1) / sizeof(kAppStateActionCoverageMigrationNotes1[0])},
  {ACTION_LOG,
   "ACTION_LOG",
   "transition.refresh-rebuild.manual-refresh",
   "refresh_rebuild",
   "Volume(shared topology)",
   kAppStateTransitionWriteSet3,
   sizeof(kAppStateTransitionWriteSet3) / sizeof(kAppStateTransitionWriteSet3[0]),
   kAppStateActionCoverageOwnerFieldRefs2,
   sizeof(kAppStateActionCoverageOwnerFieldRefs2) / sizeof(kAppStateActionCoverageOwnerFieldRefs2[0]),
   kAppStateActionCoverageTransitionSequenceRefs2,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs2) / sizeof(kAppStateActionCoverageTransitionSequenceRefs2[0]),
   kAppStateActionCoverageDispatchSurfaceRefs2,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs2) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs2[0]),
   kAppStateActionCoverageInvariantRefs2,
   sizeof(kAppStateActionCoverageInvariantRefs2) / sizeof(kAppStateActionCoverageInvariantRefs2[0]),
   kAppStateActionCoverageGenerationDomainRefs2,
   sizeof(kAppStateActionCoverageGenerationDomainRefs2) / sizeof(kAppStateActionCoverageGenerationDomainRefs2[0]),
   kAppStateActionCoverageDiffHarnessRefs2,
   sizeof(kAppStateActionCoverageDiffHarnessRefs2) / sizeof(kAppStateActionCoverageDiffHarnessRefs2[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes2,
   sizeof(kAppStateActionCoverageMigrationNotes2) / sizeof(kAppStateActionCoverageMigrationNotes2[0])},
  {ACTION_QUIT,
   "ACTION_QUIT",
   "transition.command-completion.user-command",
   "command_completion",
   "ViewContext.command_region",
   kAppStateTransitionWriteSet7,
   sizeof(kAppStateTransitionWriteSet7) / sizeof(kAppStateTransitionWriteSet7[0]),
   kAppStateActionCoverageOwnerFieldRefs3,
   sizeof(kAppStateActionCoverageOwnerFieldRefs3) / sizeof(kAppStateActionCoverageOwnerFieldRefs3[0]),
   kAppStateActionCoverageTransitionSequenceRefs3,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs3) / sizeof(kAppStateActionCoverageTransitionSequenceRefs3[0]),
   kAppStateActionCoverageDispatchSurfaceRefs3,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs3) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs3[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs3,
   sizeof(kAppStateActionCoverageGenerationDomainRefs3) / sizeof(kAppStateActionCoverageGenerationDomainRefs3[0]),
   kAppStateActionCoverageDiffHarnessRefs3,
   sizeof(kAppStateActionCoverageDiffHarnessRefs3) / sizeof(kAppStateActionCoverageDiffHarnessRefs3[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes3,
   sizeof(kAppStateActionCoverageMigrationNotes3) / sizeof(kAppStateActionCoverageMigrationNotes3[0])},
  {ACTION_QUIT_DIR,
   "ACTION_QUIT_DIR",
   "transition.command-completion.user-command",
   "command_completion",
   "ViewContext.command_region",
   kAppStateTransitionWriteSet7,
   sizeof(kAppStateTransitionWriteSet7) / sizeof(kAppStateTransitionWriteSet7[0]),
   kAppStateActionCoverageOwnerFieldRefs3,
   sizeof(kAppStateActionCoverageOwnerFieldRefs3) / sizeof(kAppStateActionCoverageOwnerFieldRefs3[0]),
   kAppStateActionCoverageTransitionSequenceRefs3,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs3) / sizeof(kAppStateActionCoverageTransitionSequenceRefs3[0]),
   kAppStateActionCoverageDispatchSurfaceRefs3,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs3) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs3[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs3,
   sizeof(kAppStateActionCoverageGenerationDomainRefs3) / sizeof(kAppStateActionCoverageGenerationDomainRefs3[0]),
   kAppStateActionCoverageDiffHarnessRefs3,
   sizeof(kAppStateActionCoverageDiffHarnessRefs3) / sizeof(kAppStateActionCoverageDiffHarnessRefs3[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes3,
   sizeof(kAppStateActionCoverageMigrationNotes3) / sizeof(kAppStateActionCoverageMigrationNotes3[0])},
  {ACTION_TAG,
   "ACTION_TAG",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_UNTAG,
   "ACTION_UNTAG",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_TAG_ALL,
   "ACTION_TAG_ALL",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_UNTAG_ALL,
   "ACTION_UNTAG_ALL",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_TAG_REST,
   "ACTION_TAG_REST",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_UNTAG_REST,
   "ACTION_UNTAG_REST",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_FILTER,
   "ACTION_FILTER",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_TOGGLE_MODE,
   "ACTION_TOGGLE_MODE",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_REFRESH,
   "ACTION_REFRESH",
   "transition.refresh-rebuild.manual-refresh",
   "refresh_rebuild",
   "Volume(shared topology)",
   kAppStateTransitionWriteSet3,
   sizeof(kAppStateTransitionWriteSet3) / sizeof(kAppStateTransitionWriteSet3[0]),
   kAppStateActionCoverageOwnerFieldRefs2,
   sizeof(kAppStateActionCoverageOwnerFieldRefs2) / sizeof(kAppStateActionCoverageOwnerFieldRefs2[0]),
   kAppStateActionCoverageTransitionSequenceRefs2,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs2) / sizeof(kAppStateActionCoverageTransitionSequenceRefs2[0]),
   kAppStateActionCoverageDispatchSurfaceRefs2,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs2) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs2[0]),
   kAppStateActionCoverageInvariantRefs2,
   sizeof(kAppStateActionCoverageInvariantRefs2) / sizeof(kAppStateActionCoverageInvariantRefs2[0]),
   kAppStateActionCoverageGenerationDomainRefs2,
   sizeof(kAppStateActionCoverageGenerationDomainRefs2) / sizeof(kAppStateActionCoverageGenerationDomainRefs2[0]),
   kAppStateActionCoverageDiffHarnessRefs2,
   sizeof(kAppStateActionCoverageDiffHarnessRefs2) / sizeof(kAppStateActionCoverageDiffHarnessRefs2[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes2,
   sizeof(kAppStateActionCoverageMigrationNotes2) / sizeof(kAppStateActionCoverageMigrationNotes2[0])},
  {ACTION_RESIZE,
   "ACTION_RESIZE",
   "transition.terminal-signal-resize",
   "terminal_signal_or_resize",
   "ViewContext.layout_region",
   kAppStateTransitionWriteSet5,
   sizeof(kAppStateTransitionWriteSet5) / sizeof(kAppStateTransitionWriteSet5[0]),
   kAppStateActionCoverageOwnerFieldRefs4,
   sizeof(kAppStateActionCoverageOwnerFieldRefs4) / sizeof(kAppStateActionCoverageOwnerFieldRefs4[0]),
   kAppStateActionCoverageTransitionSequenceRefs4,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs4) / sizeof(kAppStateActionCoverageTransitionSequenceRefs4[0]),
   kAppStateActionCoverageDispatchSurfaceRefs4,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs4) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs4[0]),
   kAppStateActionCoverageInvariantRefs4,
   sizeof(kAppStateActionCoverageInvariantRefs4) / sizeof(kAppStateActionCoverageInvariantRefs4[0]),
   kAppStateActionCoverageGenerationDomainRefs4,
   sizeof(kAppStateActionCoverageGenerationDomainRefs4) / sizeof(kAppStateActionCoverageGenerationDomainRefs4[0]),
   kAppStateActionCoverageDiffHarnessRefs4,
   sizeof(kAppStateActionCoverageDiffHarnessRefs4) / sizeof(kAppStateActionCoverageDiffHarnessRefs4[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes4,
   sizeof(kAppStateActionCoverageMigrationNotes4) / sizeof(kAppStateActionCoverageMigrationNotes4[0])},
  {ACTION_VOL_MENU,
   "ACTION_VOL_MENU",
   "transition.menu-action.volume-select",
   "menu_action",
   "ViewContext(session routing) and YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet1,
   sizeof(kAppStateTransitionWriteSet1) / sizeof(kAppStateTransitionWriteSet1[0]),
   kAppStateActionCoverageOwnerFieldRefs5,
   sizeof(kAppStateActionCoverageOwnerFieldRefs5) / sizeof(kAppStateActionCoverageOwnerFieldRefs5[0]),
   kAppStateActionCoverageTransitionSequenceRefs5,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs5) / sizeof(kAppStateActionCoverageTransitionSequenceRefs5[0]),
   kAppStateActionCoverageDispatchSurfaceRefs5,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs5) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs5[0]),
   kAppStateActionCoverageInvariantRefs5,
   sizeof(kAppStateActionCoverageInvariantRefs5) / sizeof(kAppStateActionCoverageInvariantRefs5[0]),
   kAppStateActionCoverageGenerationDomainRefs5,
   sizeof(kAppStateActionCoverageGenerationDomainRefs5) / sizeof(kAppStateActionCoverageGenerationDomainRefs5[0]),
   kAppStateActionCoverageDiffHarnessRefs5,
   sizeof(kAppStateActionCoverageDiffHarnessRefs5) / sizeof(kAppStateActionCoverageDiffHarnessRefs5[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes5,
   sizeof(kAppStateActionCoverageMigrationNotes5) / sizeof(kAppStateActionCoverageMigrationNotes5[0])},
  {ACTION_VOL_PREV,
   "ACTION_VOL_PREV",
   "transition.volume-operation.release-cycle",
   "volume_operation",
   "ViewContext.volume_registry and YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet4,
   sizeof(kAppStateTransitionWriteSet4) / sizeof(kAppStateTransitionWriteSet4[0]),
   kAppStateActionCoverageOwnerFieldRefs6,
   sizeof(kAppStateActionCoverageOwnerFieldRefs6) / sizeof(kAppStateActionCoverageOwnerFieldRefs6[0]),
   kAppStateActionCoverageTransitionSequenceRefs6,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs6) / sizeof(kAppStateActionCoverageTransitionSequenceRefs6[0]),
   kAppStateActionCoverageDispatchSurfaceRefs6,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs6) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs6[0]),
   kAppStateActionCoverageInvariantRefs5,
   sizeof(kAppStateActionCoverageInvariantRefs5) / sizeof(kAppStateActionCoverageInvariantRefs5[0]),
   kAppStateActionCoverageGenerationDomainRefs6,
   sizeof(kAppStateActionCoverageGenerationDomainRefs6) / sizeof(kAppStateActionCoverageGenerationDomainRefs6[0]),
   kAppStateActionCoverageDiffHarnessRefs6,
   sizeof(kAppStateActionCoverageDiffHarnessRefs6) / sizeof(kAppStateActionCoverageDiffHarnessRefs6[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes6,
   sizeof(kAppStateActionCoverageMigrationNotes6) / sizeof(kAppStateActionCoverageMigrationNotes6[0])},
  {ACTION_VOL_NEXT,
   "ACTION_VOL_NEXT",
   "transition.volume-operation.release-cycle",
   "volume_operation",
   "ViewContext.volume_registry and YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet4,
   sizeof(kAppStateTransitionWriteSet4) / sizeof(kAppStateTransitionWriteSet4[0]),
   kAppStateActionCoverageOwnerFieldRefs6,
   sizeof(kAppStateActionCoverageOwnerFieldRefs6) / sizeof(kAppStateActionCoverageOwnerFieldRefs6[0]),
   kAppStateActionCoverageTransitionSequenceRefs6,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs6) / sizeof(kAppStateActionCoverageTransitionSequenceRefs6[0]),
   kAppStateActionCoverageDispatchSurfaceRefs6,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs6) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs6[0]),
   kAppStateActionCoverageInvariantRefs5,
   sizeof(kAppStateActionCoverageInvariantRefs5) / sizeof(kAppStateActionCoverageInvariantRefs5[0]),
   kAppStateActionCoverageGenerationDomainRefs6,
   sizeof(kAppStateActionCoverageGenerationDomainRefs6) / sizeof(kAppStateActionCoverageGenerationDomainRefs6[0]),
   kAppStateActionCoverageDiffHarnessRefs6,
   sizeof(kAppStateActionCoverageDiffHarnessRefs6) / sizeof(kAppStateActionCoverageDiffHarnessRefs6[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes6,
   sizeof(kAppStateActionCoverageMigrationNotes6) / sizeof(kAppStateActionCoverageMigrationNotes6[0])},
  {ACTION_CMD_A,
   "ACTION_CMD_A",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_B,
   "ACTION_CMD_B",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_C,
   "ACTION_CMD_C",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_D,
   "ACTION_CMD_D",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_E,
   "ACTION_CMD_E",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_G,
   "ACTION_CMD_G",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_H,
   "ACTION_CMD_H",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_I,
   "ACTION_CMD_I",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_M,
   "ACTION_CMD_M",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_O,
   "ACTION_CMD_O",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_P,
   "ACTION_CMD_P",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_R,
   "ACTION_CMD_R",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_S,
   "ACTION_CMD_S",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_V,
   "ACTION_CMD_V",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_X,
   "ACTION_CMD_X",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_Y,
   "ACTION_CMD_Y",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_PRINT,
   "ACTION_CMD_PRINT",
   "transition.command-completion.user-command",
   "command_completion",
   "ViewContext.command_region",
   kAppStateTransitionWriteSet7,
   sizeof(kAppStateTransitionWriteSet7) / sizeof(kAppStateTransitionWriteSet7[0]),
   kAppStateActionCoverageOwnerFieldRefs3,
   sizeof(kAppStateActionCoverageOwnerFieldRefs3) / sizeof(kAppStateActionCoverageOwnerFieldRefs3[0]),
   kAppStateActionCoverageTransitionSequenceRefs3,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs3) / sizeof(kAppStateActionCoverageTransitionSequenceRefs3[0]),
   kAppStateActionCoverageDispatchSurfaceRefs3,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs3) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs3[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs3,
   sizeof(kAppStateActionCoverageGenerationDomainRefs3) / sizeof(kAppStateActionCoverageGenerationDomainRefs3[0]),
   kAppStateActionCoverageDiffHarnessRefs3,
   sizeof(kAppStateActionCoverageDiffHarnessRefs3) / sizeof(kAppStateActionCoverageDiffHarnessRefs3[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes3,
   sizeof(kAppStateActionCoverageMigrationNotes3) / sizeof(kAppStateActionCoverageMigrationNotes3[0])},
  {ACTION_TOGGLE_HIDDEN,
   "ACTION_TOGGLE_HIDDEN",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_TOGGLE_COMPACT,
   "ACTION_TOGGLE_COMPACT",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_CMD_MKFILE,
   "ACTION_CMD_MKFILE",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_TAGGED_A,
   "ACTION_CMD_TAGGED_A",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_TAGGED_C,
   "ACTION_CMD_TAGGED_C",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_TAGGED_D,
   "ACTION_CMD_TAGGED_D",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_TAGGED_G,
   "ACTION_CMD_TAGGED_G",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_TAGGED_M,
   "ACTION_CMD_TAGGED_M",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_TAGGED_O,
   "ACTION_CMD_TAGGED_O",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_TAGGED_P,
   "ACTION_CMD_TAGGED_P",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_TAGGED_R,
   "ACTION_CMD_TAGGED_R",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_TAGGED_S,
   "ACTION_CMD_TAGGED_S",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_TAGGED_V,
   "ACTION_CMD_TAGGED_V",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_TAGGED_X,
   "ACTION_CMD_TAGGED_X",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_TAGGED_Y,
   "ACTION_CMD_TAGGED_Y",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0]),
   kAppStateActionCoverageOwnerFieldRefs7,
   sizeof(kAppStateActionCoverageOwnerFieldRefs7) / sizeof(kAppStateActionCoverageOwnerFieldRefs7[0]),
   kAppStateActionCoverageTransitionSequenceRefs7,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs7) / sizeof(kAppStateActionCoverageTransitionSequenceRefs7[0]),
   kAppStateActionCoverageDispatchSurfaceRefs7,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs7) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs7[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs7,
   sizeof(kAppStateActionCoverageGenerationDomainRefs7) / sizeof(kAppStateActionCoverageGenerationDomainRefs7[0]),
   kAppStateActionCoverageDiffHarnessRefs7,
   sizeof(kAppStateActionCoverageDiffHarnessRefs7) / sizeof(kAppStateActionCoverageDiffHarnessRefs7[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes7,
   sizeof(kAppStateActionCoverageMigrationNotes7) / sizeof(kAppStateActionCoverageMigrationNotes7[0])},
  {ACTION_CMD_TAGGED_PRINT,
   "ACTION_CMD_TAGGED_PRINT",
   "transition.command-completion.user-command",
   "command_completion",
   "ViewContext.command_region",
   kAppStateTransitionWriteSet7,
   sizeof(kAppStateTransitionWriteSet7) / sizeof(kAppStateTransitionWriteSet7[0]),
   kAppStateActionCoverageOwnerFieldRefs3,
   sizeof(kAppStateActionCoverageOwnerFieldRefs3) / sizeof(kAppStateActionCoverageOwnerFieldRefs3[0]),
   kAppStateActionCoverageTransitionSequenceRefs3,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs3) / sizeof(kAppStateActionCoverageTransitionSequenceRefs3[0]),
   kAppStateActionCoverageDispatchSurfaceRefs3,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs3) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs3[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs3,
   sizeof(kAppStateActionCoverageGenerationDomainRefs3) / sizeof(kAppStateActionCoverageGenerationDomainRefs3[0]),
   kAppStateActionCoverageDiffHarnessRefs3,
   sizeof(kAppStateActionCoverageDiffHarnessRefs3) / sizeof(kAppStateActionCoverageDiffHarnessRefs3[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes3,
   sizeof(kAppStateActionCoverageMigrationNotes3) / sizeof(kAppStateActionCoverageMigrationNotes3[0])},
  {ACTION_LIST_JUMP,
   "ACTION_LIST_JUMP",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_TO_DIR,
   "ACTION_TO_DIR",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_TOGGLE_TAGGED_MODE,
   "ACTION_TOGGLE_TAGGED_MODE",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_TOGGLE_STATS,
   "ACTION_TOGGLE_STATS",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_ASTERISK,
   "ACTION_ASTERISK",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_INVERT,
   "ACTION_INVERT",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_SPLIT_SCREEN,
   "ACTION_SPLIT_SCREEN",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_SWITCH_PANEL,
   "ACTION_SWITCH_PANEL",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_VIEW_PREVIEW,
   "ACTION_VIEW_PREVIEW",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_PREVIEW_SCROLL_UP,
   "ACTION_PREVIEW_SCROLL_UP",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_PREVIEW_SCROLL_DOWN,
   "ACTION_PREVIEW_SCROLL_DOWN",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_PREVIEW_HOME,
   "ACTION_PREVIEW_HOME",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_PREVIEW_END,
   "ACTION_PREVIEW_END",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_PREVIEW_PAGE_UP,
   "ACTION_PREVIEW_PAGE_UP",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_PREVIEW_PAGE_DOWN,
   "ACTION_PREVIEW_PAGE_DOWN",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_COMPARE_FILE,
   "ACTION_COMPARE_FILE",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_COMPARE_DIR,
   "ACTION_COMPARE_DIR",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_COMPARE_TREE,
   "ACTION_COMPARE_TREE",
   "transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0]),
   kAppStateActionCoverageOwnerFieldRefs0,
   sizeof(kAppStateActionCoverageOwnerFieldRefs0) / sizeof(kAppStateActionCoverageOwnerFieldRefs0[0]),
   kAppStateActionCoverageTransitionSequenceRefs0,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs0) / sizeof(kAppStateActionCoverageTransitionSequenceRefs0[0]),
   kAppStateActionCoverageDispatchSurfaceRefs0,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs0) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs0[0]),
   kAppStateActionCoverageInvariantRefs0,
   sizeof(kAppStateActionCoverageInvariantRefs0) / sizeof(kAppStateActionCoverageInvariantRefs0[0]),
   kAppStateActionCoverageGenerationDomainRefs0,
   sizeof(kAppStateActionCoverageGenerationDomainRefs0) / sizeof(kAppStateActionCoverageGenerationDomainRefs0[0]),
   kAppStateActionCoverageDiffHarnessRefs0,
   sizeof(kAppStateActionCoverageDiffHarnessRefs0) / sizeof(kAppStateActionCoverageDiffHarnessRefs0[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes0,
   sizeof(kAppStateActionCoverageMigrationNotes0) / sizeof(kAppStateActionCoverageMigrationNotes0[0])},
  {ACTION_EDIT_CONFIG,
   "ACTION_EDIT_CONFIG",
   "transition.command-completion.user-command",
   "command_completion",
   "ViewContext.command_region",
   kAppStateTransitionWriteSet7,
   sizeof(kAppStateTransitionWriteSet7) / sizeof(kAppStateTransitionWriteSet7[0]),
   kAppStateActionCoverageOwnerFieldRefs3,
   sizeof(kAppStateActionCoverageOwnerFieldRefs3) / sizeof(kAppStateActionCoverageOwnerFieldRefs3[0]),
   kAppStateActionCoverageTransitionSequenceRefs3,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs3) / sizeof(kAppStateActionCoverageTransitionSequenceRefs3[0]),
   kAppStateActionCoverageDispatchSurfaceRefs3,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs3) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs3[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs3,
   sizeof(kAppStateActionCoverageGenerationDomainRefs3) / sizeof(kAppStateActionCoverageGenerationDomainRefs3[0]),
   kAppStateActionCoverageDiffHarnessRefs3,
   sizeof(kAppStateActionCoverageDiffHarnessRefs3) / sizeof(kAppStateActionCoverageDiffHarnessRefs3[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes3,
   sizeof(kAppStateActionCoverageMigrationNotes3) / sizeof(kAppStateActionCoverageMigrationNotes3[0])},
  {ACTION_USER_CMD,
   "ACTION_USER_CMD",
   "transition.command-completion.user-command",
   "command_completion",
   "ViewContext.command_region",
   kAppStateTransitionWriteSet7,
   sizeof(kAppStateTransitionWriteSet7) / sizeof(kAppStateTransitionWriteSet7[0]),
   kAppStateActionCoverageOwnerFieldRefs3,
   sizeof(kAppStateActionCoverageOwnerFieldRefs3) / sizeof(kAppStateActionCoverageOwnerFieldRefs3[0]),
   kAppStateActionCoverageTransitionSequenceRefs3,
   sizeof(kAppStateActionCoverageTransitionSequenceRefs3) / sizeof(kAppStateActionCoverageTransitionSequenceRefs3[0]),
   kAppStateActionCoverageDispatchSurfaceRefs3,
   sizeof(kAppStateActionCoverageDispatchSurfaceRefs3) / sizeof(kAppStateActionCoverageDispatchSurfaceRefs3[0]),
   kAppStateActionCoverageInvariantRefs3,
   sizeof(kAppStateActionCoverageInvariantRefs3) / sizeof(kAppStateActionCoverageInvariantRefs3[0]),
   kAppStateActionCoverageGenerationDomainRefs3,
   sizeof(kAppStateActionCoverageGenerationDomainRefs3) / sizeof(kAppStateActionCoverageGenerationDomainRefs3[0]),
   kAppStateActionCoverageDiffHarnessRefs3,
   sizeof(kAppStateActionCoverageDiffHarnessRefs3) / sizeof(kAppStateActionCoverageDiffHarnessRefs3[0]),
   "covered_by_transition_record",
   kAppStateActionCoverageMigrationNotes3,
   sizeof(kAppStateActionCoverageMigrationNotes3) / sizeof(kAppStateActionCoverageMigrationNotes3[0])},
};

static const AppStateCompatibilityShimMetadata kAppStateCompatibilityShims[] = {
  {"shim.focused-window-session-flag",
   "ViewContext session routing",
   "ViewContext.focused_window",
   "Allowed for layout routing and footer context selection while AppState focus_shape migration is incomplete.",
   "Write only from transition commit after the active panel focus_shape has been updated.",
   "write_capable",
   kAppStateCompatibilityShimInvariantChecks1,
   sizeof(kAppStateCompatibilityShimInvariantChecks1) /
       sizeof(kAppStateCompatibilityShimInvariantChecks1[0]),
   kAppStateCompatibilityShimOwnerFieldRefs1,
   sizeof(kAppStateCompatibilityShimOwnerFieldRefs1) /
       sizeof(kAppStateCompatibilityShimOwnerFieldRefs1[0]),
   kAppStateCompatibilityShimGenerationDomainRefs1,
    sizeof(kAppStateCompatibilityShimGenerationDomainRefs1) /
        sizeof(kAppStateCompatibilityShimGenerationDomainRefs1[0]),
    kAppStateCompatibilityShimDiffHarnessRefs1,
    sizeof(kAppStateCompatibilityShimDiffHarnessRefs1) /
        sizeof(kAppStateCompatibilityShimDiffHarnessRefs1[0]),
    kAppStateCompatibilityShimSourceBoundaryRefs1,
    sizeof(kAppStateCompatibilityShimSourceBoundaryRefs1) /
        sizeof(kAppStateCompatibilityShimSourceBoundaryRefs1[0]),
    "All Enter, Tab, and F8 paths route through the canonical AppState transition entry point.",
    "transition.keybinding.navigate-tree",
    "Move focus-shape authority from session mirrors into panel-local transition records.",
   "check_appstate_contract.py validates shim coverage and links it to an existing transition id."},
  {"shim-render-derived-row-position",
   "Render projection temporary",
   "disp_begin_pos + cursor_pos render-derived lookup",
   "Allowed only inside render projection or bounds-correction code after identity restore has run.",
   "Never write authoritative selection from this calculation.",
   "read_only_projection",
   kAppStateCompatibilityShimInvariantChecks2,
   sizeof(kAppStateCompatibilityShimInvariantChecks2) /
       sizeof(kAppStateCompatibilityShimInvariantChecks2[0]),
   kAppStateCompatibilityShimOwnerFieldRefs2,
   sizeof(kAppStateCompatibilityShimOwnerFieldRefs2) /
       sizeof(kAppStateCompatibilityShimOwnerFieldRefs2[0]),
   kAppStateCompatibilityShimGenerationDomainRefs2,
    sizeof(kAppStateCompatibilityShimGenerationDomainRefs2) /
        sizeof(kAppStateCompatibilityShimGenerationDomainRefs2[0]),
    kAppStateCompatibilityShimDiffHarnessRefs2,
    sizeof(kAppStateCompatibilityShimDiffHarnessRefs2) /
        sizeof(kAppStateCompatibilityShimDiffHarnessRefs2[0]),
    kAppStateCompatibilityShimSourceBoundaryRefs2,
    sizeof(kAppStateCompatibilityShimSourceBoundaryRefs2) /
        sizeof(kAppStateCompatibilityShimSourceBoundaryRefs2[0]),
    "Render paths accept explicit projection inputs and no longer inspect restore authority fields directly.",
    "transition.render-reflow.project-state",
    "Audit render/reflow call sites for projection-only behavior during runtime migration.",
   "check_appstate_contract.py requires render shims to declare no-write authority and target transition linkage."},
};

static int AppStateValidateActionCoverage(
    YtreeNovaAction action, const AppStateActionCoverageMetadata *coverage);
static int AppStateLookupIdMatches(const char *candidate,
                                   const char *requested);
int AppStateValidatedEvent(const char *event_id);
int AppStateValidatedTransition(const char *transition_id);
int AppStateValidatedDispatchSurface(const char *surface_id);
int AppStateValidatedCompatibilityShim(const char *shim_id);
int AppStateValidatedInvariant(const char *invariant_id);
int AppStateValidatedOwnerField(const char *field);
int AppStateValidatedTransitionSequence(const char *scenario_id);

size_t AppStateActionTransitionCount(void) {
  return sizeof(kAppStateActionTransitions) / sizeof(kAppStateActionTransitions[0]);
}

size_t AppStateActionCoverageCount(void) {
  return sizeof(kAppStateActionCoverages) / sizeof(kAppStateActionCoverages[0]);
}

size_t AppStateEventCoverageCount(void) {
  return sizeof(kAppStateEventCoverages) / sizeof(kAppStateEventCoverages[0]);
}

size_t AppStateTransitionCount(void) {
  return sizeof(kAppStateTransitions) / sizeof(kAppStateTransitions[0]);
}

size_t AppStateDispatchSurfaceCount(void) {
  return sizeof(kAppStateDispatchSurfaces) / sizeof(kAppStateDispatchSurfaces[0]);
}

size_t AppStateCompatibilityShimCount(void) {
  return sizeof(kAppStateCompatibilityShims) /
         sizeof(kAppStateCompatibilityShims[0]);
}

size_t AppStateInvariantCount(void) {
  return sizeof(kAppStateInvariants) / sizeof(kAppStateInvariants[0]);
}

size_t AppStateOwnerFieldCount(void) {
  return sizeof(kAppStateOwnerFields) / sizeof(kAppStateOwnerFields[0]);
}

size_t AppStateGenerationDomainCount(void) {
  return sizeof(kAppStateGenerationDomains) / sizeof(kAppStateGenerationDomains[0]);
}

size_t AppStateDiffHarnessCount(void) {
  return sizeof(kAppStateDiffHarnesses) / sizeof(kAppStateDiffHarnesses[0]);
}

size_t AppStateTransitionSequenceCount(void) {
  return sizeof(kAppStateTransitionSequences) /
         sizeof(kAppStateTransitionSequences[0]);
}

const AppStateOwnerFieldMetadata *AppStateOwnerFieldAt(size_t index) {
  if (index >= AppStateOwnerFieldCount())
    return NULL;

  if (!AppStateValidatedOwnerField(kAppStateOwnerFields[index].field))
    return NULL;

  return &kAppStateOwnerFields[index];
}

const AppStateGenerationDomainMetadata *
AppStateGenerationDomainAt(size_t index) {
  if (index >= AppStateGenerationDomainCount())
    return NULL;

  if (!AppStateValidatedGenerationDomain(
          kAppStateGenerationDomains[index].domain_id))
    return NULL;

  return &kAppStateGenerationDomains[index];
}

const AppStateDiffHarnessMetadata *AppStateDiffHarnessAt(size_t index) {
  if (index >= AppStateDiffHarnessCount())
    return NULL;

  if (!AppStateValidatedDiffHarness(kAppStateDiffHarnesses[index].harness_id))
    return NULL;

  return &kAppStateDiffHarnesses[index];
}

const AppStateTransitionSequenceMetadata *
AppStateTransitionSequenceAt(size_t index) {
  if (index >= AppStateTransitionSequenceCount())
    return NULL;

  if (!AppStateValidatedTransitionSequence(
          kAppStateTransitionSequences[index].scenario_id))
    return NULL;

  return &kAppStateTransitionSequences[index];
}

const AppStateActionCoverageMetadata *AppStateActionCoverageAt(size_t index) {
  if (index >= AppStateActionCoverageCount())
    return NULL;

  if (!AppStateValidateActionCoverage(kAppStateActionCoverages[index].action,
                                      &kAppStateActionCoverages[index]))
    return NULL;

  return &kAppStateActionCoverages[index];
}

const AppStateEventCoverageMetadata *AppStateEventCoverageAt(size_t index) {
  if (index >= AppStateEventCoverageCount())
    return NULL;

  if (!AppStateValidatedEvent(kAppStateEventCoverages[index].event_id))
    return NULL;

  return &kAppStateEventCoverages[index];
}

const AppStateTransitionMetadata *AppStateTransitionAt(size_t index) {
  if (index >= AppStateTransitionCount())
    return NULL;

  if (!AppStateValidatedTransition(kAppStateTransitions[index].id))
    return NULL;

  return &kAppStateTransitions[index];
}

const AppStateDispatchSurfaceMetadata *AppStateDispatchSurfaceAt(size_t index) {
  if (index >= AppStateDispatchSurfaceCount())
    return NULL;

  if (!AppStateValidatedDispatchSurface(
          kAppStateDispatchSurfaces[index].surface_id))
    return NULL;

  return &kAppStateDispatchSurfaces[index];
}

const AppStateCompatibilityShimMetadata *
AppStateCompatibilityShimAt(size_t index) {
  if (index >= AppStateCompatibilityShimCount())
    return NULL;

  if (!AppStateValidatedCompatibilityShim(kAppStateCompatibilityShims[index].id))
    return NULL;

  return &kAppStateCompatibilityShims[index];
}

const AppStateInvariantMetadata *AppStateInvariantAt(size_t index) {
  if (index >= AppStateInvariantCount())
    return NULL;

  if (!AppStateValidatedInvariant(kAppStateInvariants[index].invariant_id))
    return NULL;

  return &kAppStateInvariants[index];
}

const AppStateOwnerFieldMetadata *
AppStateOwnerFieldLookup(const char *field) {
  size_t index;

  if (field == NULL || field[0] == '\0')
    return NULL;

  for (index = 0; index < AppStateOwnerFieldCount(); index++) {
    if (AppStateLookupIdMatches(kAppStateOwnerFields[index].field, field))
      return &kAppStateOwnerFields[index];
  }

  return NULL;
}

const AppStateGenerationDomainMetadata *
AppStateGenerationDomainLookup(const char *domain_id) {
  size_t index;

  if (domain_id == NULL || domain_id[0] == '\0')
    return NULL;

  for (index = 0; index < AppStateGenerationDomainCount(); index++) {
    if (AppStateLookupIdMatches(kAppStateGenerationDomains[index].domain_id,
                                domain_id))
      return &kAppStateGenerationDomains[index];
  }

  return NULL;
}

const AppStateDiffHarnessMetadata *
AppStateDiffHarnessLookup(const char *harness_id) {
  size_t index;

  if (harness_id == NULL || harness_id[0] == '\0')
    return NULL;

  for (index = 0; index < AppStateDiffHarnessCount(); index++) {
    if (AppStateLookupIdMatches(kAppStateDiffHarnesses[index].harness_id,
                                harness_id))
      return &kAppStateDiffHarnesses[index];
  }

  return NULL;
}

const AppStateTransitionSequenceMetadata *
AppStateTransitionSequenceLookup(const char *scenario_id) {
  size_t index;

  if (scenario_id == NULL || scenario_id[0] == '\0')
    return NULL;

  for (index = 0; index < AppStateTransitionSequenceCount(); index++) {
    if (AppStateLookupIdMatches(
            kAppStateTransitionSequences[index].scenario_id, scenario_id))
      return &kAppStateTransitionSequences[index];
  }

  return NULL;
}

const AppStateTransitionMetadata *
AppStateTransitionLookup(const char *transition_id) {
  size_t index;

  if (transition_id == NULL || transition_id[0] == '\0')
    return NULL;

  for (index = 0; index < AppStateTransitionCount(); index++) {
    if (AppStateLookupIdMatches(kAppStateTransitions[index].id,
                                transition_id))
      return &kAppStateTransitions[index];
  }

  return NULL;
}

const AppStateDispatchSurfaceMetadata *
AppStateDispatchSurfaceLookup(const char *surface_id) {
  size_t index;

  if (surface_id == NULL || surface_id[0] == '\0')
    return NULL;

  for (index = 0; index < AppStateDispatchSurfaceCount(); index++) {
    if (AppStateLookupIdMatches(kAppStateDispatchSurfaces[index].surface_id,
                                surface_id))
      return &kAppStateDispatchSurfaces[index];
  }

  return NULL;
}

const AppStateCompatibilityShimMetadata *
AppStateCompatibilityShimLookup(const char *shim_id) {
  size_t index;

  if (shim_id == NULL || shim_id[0] == '\0')
    return NULL;

  for (index = 0; index < AppStateCompatibilityShimCount(); index++) {
    if (AppStateLookupIdMatches(kAppStateCompatibilityShims[index].id,
                                shim_id))
      return &kAppStateCompatibilityShims[index];
  }

  return NULL;
}

const AppStateInvariantMetadata *
AppStateInvariantLookup(const char *invariant_id) {
  size_t index;

  if (invariant_id == NULL || invariant_id[0] == '\0')
    return NULL;

  for (index = 0; index < AppStateInvariantCount(); index++) {
    if (AppStateLookupIdMatches(kAppStateInvariants[index].invariant_id,
                                invariant_id))
      return &kAppStateInvariants[index];
  }

  return NULL;
}

const AppStateActionTransitionMetadata *
AppStateActionTransitionLookup(YtreeNovaAction action) {
  const AppStateActionTransitionMetadata *metadata;

  if ((int)action < 0 || (size_t)action >= AppStateActionTransitionCount())
    return NULL;

  metadata = &kAppStateActionTransitions[(size_t)action];
  if (metadata->action != action)
    return NULL;

  return metadata;
}

const AppStateActionCoverageMetadata *
AppStateActionCoverageLookup(YtreeNovaAction action) {
  const AppStateActionCoverageMetadata *metadata;

  if ((int)action < 0 || (size_t)action >= AppStateActionCoverageCount())
    return NULL;

  metadata = &kAppStateActionCoverages[(size_t)action];
  if (metadata->action != action)
    return NULL;

  return metadata;
}

static int AppStateNonEmptyString(const char *value) {
  return value != NULL && value[0] != '\0';
}

static int AppStateLookupIdMatches(const char *candidate,
                                   const char *requested) {
  return AppStateNonEmptyString(candidate) &&
         AppStateNonEmptyString(requested) && !strcmp(candidate, requested);
}

static int AppStateNonEmptyStringList(const char *const *values,
                                      size_t value_count) {
  size_t index;

  if (values == NULL || value_count == 0)
    return 0;

  for (index = 0; index < value_count; index++) {
    if (!AppStateNonEmptyString(values[index]))
      return 0;
  }

  return 1;
}

static int AppStateStringListContains(const char *const *values,
                                      size_t value_count,
                                      const char *value) {
  size_t index;

  if (!AppStateNonEmptyStringList(values, value_count) ||
      !AppStateNonEmptyString(value))
    return 0;

  for (index = 0; index < value_count; index++) {
    if (!strcmp(values[index], value))
      return 1;
  }

  return 0;
}

static int AppStateStringListsOverlap(const char *const *left,
                                      size_t left_count,
                                      const char *const *right,
                                      size_t right_count) {
  size_t index;

  if (!AppStateNonEmptyStringList(left, left_count) ||
      !AppStateNonEmptyStringList(right, right_count))
    return 0;

  for (index = 0; index < left_count; index++) {
    if (AppStateStringListContains(right, right_count, left[index]))
      return 1;
  }

  return 0;
}

static int AppStateKnownBoundaryStatus(const char *status) {
  if (!AppStateNonEmptyString(status))
    return 0;
  return strcmp(status, "documented_foundation_only") == 0 ||
         strcmp(status, "covered_by_transition_record") == 0 ||
         strcmp(status, "mapped_to_existing_broad_transition") == 0;
}

static int AppStateKnownFoundationStatus(const char *status) {
  if (!AppStateNonEmptyString(status))
    return 0;
  return strcmp(status, "documented_foundation_only") == 0 ||
         strcmp(status, "covered_by_runtime_registry") == 0;
}

static int AppStateKnownMigrationStatus(const char *status) {
  if (!AppStateNonEmptyString(status))
    return 0;
  return strcmp(status, "runtime_backed") == 0;
}

static const AppStateActionCoverageMetadata *
AppStateActionCoverageIdLookup(const char *action_id) {
  size_t index;

  if (!AppStateNonEmptyString(action_id))
    return NULL;

  for (index = 0; index < AppStateActionCoverageCount(); index++) {
    if (AppStateLookupIdMatches(kAppStateActionCoverages[index].action_name,
                                action_id))
      return &kAppStateActionCoverages[index];
  }

  return NULL;
}

static const AppStateEventCoverageMetadata *
AppStateEventCoverageIdLookup(const char *event_id) {
  size_t index;

  if (!AppStateNonEmptyString(event_id))
    return NULL;

  for (index = 0; index < AppStateEventCoverageCount(); index++) {
    if (AppStateLookupIdMatches(kAppStateEventCoverages[index].event_id,
                                event_id))
      return &kAppStateEventCoverages[index];
  }

  return NULL;
}

static int AppStateTransitionFieldsRegistered(const char *const *fields,
                                              size_t field_count) {
  size_t index;

  if (!AppStateNonEmptyStringList(fields, field_count))
    return 0;

  for (index = 0; index < field_count; index++) {
    if (AppStateOwnerFieldLookup(fields[index]) == NULL)
      return 0;
  }

  return 1;
}

static int AppStateStringListEquals(const char *const *left, size_t left_count,
                                    const char *const *right,
                                    size_t right_count) {
  size_t index;

  if (!AppStateNonEmptyStringList(left, left_count) ||
      !AppStateNonEmptyStringList(right, right_count) ||
      left_count != right_count)
    return 0;

  for (index = 0; index < left_count; index++) {
    if (strcmp(left[index], right[index]) != 0)
      return 0;
  }

  return 1;
}

static int AppStateCoverageOwnerFieldsMatchWriteSet(
    const char *const *owner_field_refs, size_t owner_field_ref_count,
    const char *const *declared_write_set, size_t declared_write_set_count) {
  size_t index;

  if (!AppStateTransitionFieldsRegistered(owner_field_refs,
                                          owner_field_ref_count) ||
      !AppStateTransitionFieldsRegistered(declared_write_set,
                                          declared_write_set_count))
    return 0;

  for (index = 0; index < owner_field_ref_count; index++) {
    if (!AppStateStringListContains(declared_write_set,
                                    declared_write_set_count,
                                    owner_field_refs[index]))
      return 0;
  }

  for (index = 0; index < declared_write_set_count; index++) {
    if (!AppStateStringListContains(owner_field_refs, owner_field_ref_count,
                                    declared_write_set[index]))
      return 0;
  }

  return 1;
}

static int
AppStateTransitionIdsRegistered(const char *const *transition_ids,
                                size_t transition_id_count) {
  size_t index;

  if (!AppStateNonEmptyStringList(transition_ids, transition_id_count))
    return 0;

  for (index = 0; index < transition_id_count; index++) {
    if (!AppStateValidatedTransition(transition_ids[index]))
      return 0;
  }

  return 1;
}

static int
AppStateValidateTransition(const char *transition_id,
                           const AppStateTransitionMetadata *metadata) {
  if (!AppStateNonEmptyString(transition_id))
    return 0;
  if (metadata == NULL || !AppStateNonEmptyString(metadata->id) ||
      strcmp(metadata->id, transition_id))
    return 0;
  if (!AppStateNonEmptyString(metadata->category) ||
      !AppStateNonEmptyString(metadata->source_state) ||
      !AppStateNonEmptyString(metadata->event) ||
      !AppStateNonEmptyString(metadata->guard) ||
      !AppStateNonEmptyString(metadata->allowed_result) ||
      !AppStateNonEmptyString(metadata->blocked_result) ||
      !AppStateNonEmptyString(metadata->target_state) ||
      !AppStateNonEmptyString(metadata->owner) ||
      !AppStateNonEmptyString(metadata->generation_effect) ||
      !AppStateNonEmptyStringList(metadata->side_effects,
                                  metadata->side_effect_count) ||
      !AppStateNonEmptyString(metadata->render_invalidation) ||
      !AppStateNonEmptyString(metadata->boundary_status) ||
      !AppStateNonEmptyString(metadata->notes_follow_up))
    return 0;
  if (!AppStateKnownBoundaryStatus(metadata->boundary_status))
    return 0;
  if (!AppStateTransitionFieldsRegistered(metadata->declared_write_set,
                                          metadata->declared_write_set_count))
    return 0;

  return 1;
}

int AppStateValidatedTransition(const char *transition_id) {
  return AppStateValidateTransition(transition_id,
                                    AppStateTransitionLookup(transition_id));
}

static int AppStateInvariantIdsRegistered(const char *const *invariant_ids,
                                          size_t invariant_id_count) {
  size_t index;

  if (!AppStateNonEmptyStringList(invariant_ids, invariant_id_count))
    return 0;

  for (index = 0; index < invariant_id_count; index++) {
    if (!AppStateValidatedInvariant(invariant_ids[index]))
      return 0;
  }

  return 1;
}

static int
AppStateValidateOwnerField(const char *field,
                           const AppStateOwnerFieldMetadata *metadata) {
  if (!AppStateNonEmptyString(field))
    return 0;
  if (metadata == NULL || !AppStateNonEmptyString(metadata->field) ||
      strcmp(metadata->field, field))
    return 0;
  if (!AppStateNonEmptyString(metadata->owner_region) ||
      !AppStateNonEmptyString(metadata->canonical_owner) ||
      !AppStateNonEmptyString(metadata->runtime_carrier) ||
      !AppStateNonEmptyString(metadata->mutation_rule) ||
      !AppStateNonEmptyString(metadata->migration_status))
    return 0;
  if (!AppStateKnownMigrationStatus(metadata->migration_status))
    return 0;
  if (!AppStateInvariantIdsRegistered(metadata->invariant_checks,
                                      metadata->invariant_check_count))
    return 0;

  return 1;
}

int AppStateValidatedOwnerField(const char *field) {
  return AppStateValidateOwnerField(field, AppStateOwnerFieldLookup(field));
}

static int
AppStateDispatchSurfaceIdsRegistered(const char *const *surface_ids,
                                     size_t surface_id_count) {
  size_t index;

  if (!AppStateNonEmptyStringList(surface_ids, surface_id_count))
    return 0;

  for (index = 0; index < surface_id_count; index++) {
    if (!AppStateValidatedDispatchSurface(surface_ids[index]))
      return 0;
  }

  return 1;
}

static int
AppStateValidateInvariant(const char *invariant_id,
                          const AppStateInvariantMetadata *metadata) {
  if (!AppStateNonEmptyString(invariant_id))
    return 0;
  if (metadata == NULL || !AppStateNonEmptyString(metadata->invariant_id) ||
      strcmp(metadata->invariant_id, invariant_id))
    return 0;
  if (!AppStateNonEmptyString(metadata->category) ||
      !AppStateNonEmptyString(metadata->owner_region) ||
      !AppStateNonEmptyString(metadata->failure_mode) ||
      !AppStateNonEmptyString(metadata->enforcement_status) ||
      !AppStateNonEmptyString(metadata->test_strategy))
    return 0;
  if (!AppStateKnownFoundationStatus(metadata->enforcement_status))
    return 0;
  if (strcmp(metadata->enforcement_status, "documented_foundation_only") == 0)
    return 0;
  if (!AppStateTransitionFieldsRegistered(metadata->protected_fields,
                                          metadata->protected_field_count))
    return 0;
  if (!AppStateTransitionIdsRegistered(metadata->transition_ids,
                                       metadata->transition_id_count))
    return 0;
  if (!AppStateDispatchSurfaceIdsRegistered(metadata->dispatch_surface_ids,
                                            metadata->dispatch_surface_id_count))
    return 0;
  if (!AppStateNonEmptyStringList(metadata->migration_notes,
                                  metadata->migration_note_count))
    return 0;

  return 1;
}

int AppStateValidatedInvariant(const char *invariant_id) {
  return AppStateValidateInvariant(invariant_id,
                                   AppStateInvariantLookup(invariant_id));
}

static int
AppStateValidateGenerationDomain(
    const char *domain_id, const AppStateGenerationDomainMetadata *metadata) {
  size_t index;

  if (!AppStateNonEmptyString(domain_id))
    return 0;
  if (metadata == NULL || !AppStateNonEmptyString(metadata->domain_id) ||
      strcmp(metadata->domain_id, domain_id))
    return 0;
  if (!AppStateNonEmptyString(metadata->category) ||
      !AppStateNonEmptyString(metadata->owner_region) ||
      !AppStateNonEmptyString(metadata->generation_owner_field) ||
      !AppStateNonEmptyString(metadata->stale_snapshot_policy) ||
      !AppStateNonEmptyString(metadata->fail_closed_fallback) ||
      !AppStateNonEmptyString(metadata->restore_boundary) ||
      !AppStateNonEmptyString(metadata->enforcement_status))
    return 0;
  if (!AppStateKnownFoundationStatus(metadata->enforcement_status))
    return 0;
  if (AppStateOwnerFieldLookup(metadata->generation_owner_field) == NULL)
    return 0;
  if (!AppStateTransitionFieldsRegistered(metadata->identity_fields,
                                          metadata->identity_field_count))
    return 0;
  if (!AppStateTransitionIdsRegistered(metadata->coverage_transition_ids,
                                       metadata->coverage_transition_id_count))
    return 0;
  if (!AppStateTransitionIdsRegistered(
          metadata->advances_on_transition_ids,
          metadata->advances_on_transition_id_count))
    return 0;
  for (index = 0; index < metadata->advances_on_transition_id_count; index++) {
    if (!AppStateStringListContains(metadata->coverage_transition_ids,
                                    metadata->coverage_transition_id_count,
                                    metadata->advances_on_transition_ids[index]))
      return 0;
  }
  if (!AppStateNonEmptyStringList(metadata->migration_notes,
                                  metadata->migration_note_count))
    return 0;

  return 1;
}

int AppStateValidatedGenerationDomain(const char *domain_id) {
  return AppStateValidateGenerationDomain(
      domain_id, AppStateGenerationDomainLookup(domain_id));
}

static int
AppStateGenerationDomainIdsRegistered(const char *const *domain_ids,
                                      size_t domain_id_count) {
  size_t index;

  if (!AppStateNonEmptyStringList(domain_ids, domain_id_count))
    return 0;

  for (index = 0; index < domain_id_count; index++) {
    if (!AppStateValidatedGenerationDomain(domain_ids[index]))
      return 0;
  }

  return 1;
}

static int
AppStateValidateDiffHarness(const char *harness_id,
                            const AppStateDiffHarnessMetadata *metadata) {
  if (!AppStateNonEmptyString(harness_id))
    return 0;
  if (metadata == NULL || !AppStateNonEmptyString(metadata->harness_id) ||
      strcmp(metadata->harness_id, harness_id))
    return 0;
  if (!AppStateNonEmptyString(metadata->check_category) ||
      !AppStateNonEmptyString(metadata->expected_behavior) ||
      !AppStateNonEmptyString(metadata->failure_mode) ||
      !AppStateNonEmptyString(metadata->enforcement_status))
    return 0;
  if (!AppStateKnownFoundationStatus(metadata->enforcement_status))
    return 0;
  if (!AppStateNonEmptyStringList(metadata->snapshot_phases,
                                  metadata->snapshot_phase_count))
    return 0;
  if (!AppStateNonEmptyStringList(metadata->snapshot_regions,
                                  metadata->snapshot_region_count))
    return 0;
  if (!AppStateTransitionIdsRegistered(metadata->transition_ids,
                                       metadata->transition_id_count))
    return 0;
  if (!AppStateTransitionFieldsRegistered(metadata->owner_field_refs,
                                          metadata->owner_field_ref_count))
    return 0;
  if (!AppStateInvariantIdsRegistered(metadata->invariant_ids,
                                      metadata->invariant_id_count))
    return 0;
  if (!AppStateGenerationDomainIdsRegistered(
          metadata->generation_domain_ids, metadata->generation_domain_id_count))
    return 0;
  if (!AppStateNonEmptyStringList(metadata->migration_notes,
                                  metadata->migration_note_count))
    return 0;

  return 1;
}

int AppStateValidatedDiffHarness(const char *harness_id) {
  return AppStateValidateDiffHarness(harness_id,
                                     AppStateDiffHarnessLookup(harness_id));
}

static int AppStateExpectedResultValid(const char *expected_result) {
  if (!AppStateNonEmptyString(expected_result))
    return 0;
  return strcmp(expected_result, "allowed") == 0 ||
         strcmp(expected_result, "blocked") == 0 ||
         strcmp(expected_result, "fallback") == 0 ||
         strcmp(expected_result, "invalid") == 0;
}

static int AppStateFallbackPreconditionValid(const char *precondition) {
  if (precondition == NULL)
    return 1;
  if (!AppStateNonEmptyString(precondition))
    return 0;
  return strcmp(precondition, "generation_mismatch") == 0 ||
         strcmp(precondition, "stale_snapshot") == 0;
}

static int AppStateDiffHarnessCoversTransition(const char *harness_id,
                                               const char *transition_id) {
  const AppStateDiffHarnessMetadata *harness;

  if (!AppStateNonEmptyString(harness_id) ||
      !AppStateNonEmptyString(transition_id))
    return 0;

  harness = AppStateDiffHarnessLookup(harness_id);
  if (harness == NULL ||
      !AppStateNonEmptyStringList(harness->transition_ids,
                                  harness->transition_id_count))
    return 0;

  return AppStateStringListContains(harness->transition_ids,
                                    harness->transition_id_count,
                                    transition_id);
}

static int AppStateTransitionSequenceStepRequiresNoUnrelatedMutation(
    const AppStateTransitionSequenceStepMetadata *step) {
  if (step == NULL || !AppStateExpectedResultValid(step->expected_result))
    return 0;
  return strcmp(step->expected_result, "blocked") == 0 ||
         strcmp(step->expected_result, "fallback") == 0 ||
         strcmp(step->expected_result, "invalid") == 0 ||
         step->precondition != NULL;
}

static int AppStateTransitionSequenceStepNoUnrelatedMutationReady(
    const AppStateTransitionSequenceStepMetadata *step) {
  if (step == NULL)
    return 0;
  if (!AppStateTransitionSequenceStepRequiresNoUnrelatedMutation(step) &&
      step->no_unrelated_mutation == NULL)
    return 1;
  if (step->no_unrelated_mutation == NULL)
    return 0;
  if (!AppStateNonEmptyString(step->no_unrelated_mutation->diff_harness_id) ||
      !AppStateNonEmptyString(step->no_unrelated_mutation->expectation))
    return 0;
  if (!AppStateStringListContains(step->diff_harness_ids,
                                  step->diff_harness_id_count,
                                  step->no_unrelated_mutation->diff_harness_id))
    return 0;
  if (!AppStateDiffHarnessCoversTransition(
          step->no_unrelated_mutation->diff_harness_id, step->transition_id))
    return 0;

  return 1;
}

static int AppStateTransitionSequenceStepDeterministicFallbackReady(
    const AppStateTransitionSequenceStepMetadata *step) {
  if (step == NULL)
    return 0;
  if ((step->precondition != NULL ||
       strcmp(step->expected_result, "fallback") == 0) &&
      step->deterministic_fallback == NULL)
    return 0;
  if (step->deterministic_fallback == NULL)
    return 1;
  return AppStateNonEmptyString(step->deterministic_fallback->outcome) &&
         AppStateNonEmptyString(
             step->deterministic_fallback->allowed_mutation_scope);
}

static int AppStateTransitionSequenceStepGenerationDomainOverlaps(
    const AppStateTransitionSequenceStepMetadata *step,
    const char *const *coverage_domain_refs, size_t coverage_domain_ref_count) {
  size_t index;

  if (step == NULL || step->generation_domain_expectations == NULL ||
      step->generation_domain_expectation_count == 0 ||
      !AppStateNonEmptyStringList(coverage_domain_refs,
                                  coverage_domain_ref_count))
    return 0;

  for (index = 0; index < step->generation_domain_expectation_count; index++) {
    const AppStateTransitionSequenceGenerationExpectationMetadata *expectation =
        &step->generation_domain_expectations[index];

    if (!AppStateNonEmptyString(expectation->domain_id))
      return 0;
    if (AppStateStringListContains(coverage_domain_refs,
                                   coverage_domain_ref_count,
                                   expectation->domain_id))
      return 1;
  }

  return 0;
}

static int AppStateTransitionSequenceStepCoverageOverlaps(
    const AppStateTransitionSequenceStepMetadata *step,
    const char *const *coverage_invariant_refs, size_t coverage_invariant_count,
    const char *const *coverage_diff_harness_refs,
    size_t coverage_diff_harness_count,
    const char *const *coverage_generation_domain_refs,
    size_t coverage_generation_domain_count) {
  if (step == NULL)
    return 0;
  if (!AppStateStringListsOverlap(step->invariant_ids,
                                  step->invariant_id_count,
                                  coverage_invariant_refs,
                                  coverage_invariant_count))
    return 0;
  if (!AppStateStringListsOverlap(step->diff_harness_ids,
                                  step->diff_harness_id_count,
                                  coverage_diff_harness_refs,
                                  coverage_diff_harness_count))
    return 0;
  if (!AppStateTransitionSequenceStepGenerationDomainOverlaps(
          step, coverage_generation_domain_refs,
          coverage_generation_domain_count))
    return 0;

  return 1;
}

static int AppStateTransitionSequenceActionRefsReady(
    const AppStateTransitionSequenceStepMetadata *step) {
  size_t index;

  if (step->stimulus_action_id == NULL) {
    return step->action_coverage_refs == NULL &&
           step->action_coverage_ref_count == 0;
  }
  if (!AppStateNonEmptyString(step->stimulus_action_id) ||
      !AppStateNonEmptyStringList(step->action_coverage_refs,
                                  step->action_coverage_ref_count))
    return 0;

  for (index = 0; index < step->action_coverage_ref_count; index++) {
    const AppStateActionCoverageMetadata *coverage =
        AppStateActionCoverageIdLookup(step->action_coverage_refs[index]);

    if (coverage == NULL ||
        strcmp(step->action_coverage_refs[index], step->stimulus_action_id) !=
            0 ||
        strcmp(coverage->transition_id, step->transition_id) != 0)
      return 0;
    if (!AppStateTransitionSequenceStepCoverageOverlaps(
            step, coverage->invariant_refs, coverage->invariant_ref_count,
            coverage->diff_harness_refs, coverage->diff_harness_ref_count,
            coverage->generation_domain_refs,
            coverage->generation_domain_ref_count))
      return 0;
    if (AppStateStringListContains(step->action_coverage_refs, index,
                                   step->action_coverage_refs[index]))
      return 0;
  }

  return 1;
}

static int AppStateTransitionSequenceEventRefsReady(
    const AppStateTransitionSequenceStepMetadata *step) {
  size_t index;

  if (step->stimulus_event_id == NULL) {
    return step->event_coverage_refs == NULL &&
           step->event_coverage_ref_count == 0;
  }
  if (!AppStateNonEmptyString(step->stimulus_event_id) ||
      !AppStateNonEmptyStringList(step->event_coverage_refs,
                                  step->event_coverage_ref_count))
    return 0;

  for (index = 0; index < step->event_coverage_ref_count; index++) {
    const AppStateEventCoverageMetadata *coverage =
        AppStateEventCoverageIdLookup(step->event_coverage_refs[index]);

    if (coverage == NULL ||
        strcmp(step->event_coverage_refs[index], step->stimulus_event_id) !=
            0 ||
        strcmp(coverage->transition_id, step->transition_id) != 0)
      return 0;
    if (!AppStateTransitionSequenceStepCoverageOverlaps(
            step, coverage->invariant_refs, coverage->invariant_ref_count,
            coverage->diff_harness_refs, coverage->diff_harness_ref_count,
            coverage->generation_domain_refs,
            coverage->generation_domain_ref_count))
      return 0;
    if (AppStateStringListContains(step->event_coverage_refs, index,
                                   step->event_coverage_refs[index]))
      return 0;
  }

  return 1;
}

static int
AppStateTransitionSequenceStepReady(
    const AppStateTransitionSequenceStepMetadata *step) {
  size_t index;

  if (step == NULL || !AppStateNonEmptyString(step->step_id) ||
      !AppStateValidatedTransition(step->transition_id) ||
      !AppStateExpectedResultValid(step->expected_result) ||
      !AppStateFallbackPreconditionValid(step->precondition))
    return 0;
  if (step->stimulus_action_id == NULL && step->stimulus_event_id == NULL)
    return 0;
  if (!AppStateTransitionSequenceActionRefsReady(step) ||
      !AppStateTransitionSequenceEventRefsReady(step))
    return 0;
  if (!AppStateInvariantIdsRegistered(step->invariant_ids,
                                      step->invariant_id_count))
    return 0;
  if (!AppStateNonEmptyStringList(step->diff_harness_ids,
                                  step->diff_harness_id_count))
    return 0;
  for (index = 0; index < step->diff_harness_id_count; index++) {
    if (!AppStateValidatedDiffHarness(step->diff_harness_ids[index]))
      return 0;
  }
  if (step->generation_domain_expectations == NULL ||
      step->generation_domain_expectation_count == 0)
    return 0;
  for (index = 0; index < step->generation_domain_expectation_count; index++) {
    const AppStateTransitionSequenceGenerationExpectationMetadata *expectation =
        &step->generation_domain_expectations[index];

    if (!AppStateNonEmptyString(expectation->domain_id) ||
        !AppStateNonEmptyString(expectation->expectation) ||
        !AppStateValidatedGenerationDomain(expectation->domain_id))
      return 0;
  }
  if (!AppStateTransitionSequenceStepNoUnrelatedMutationReady(step))
    return 0;
  if (!AppStateTransitionSequenceStepDeterministicFallbackReady(step))
    return 0;

  return 1;
}

static int AppStateValidateTransitionSequence(
    const char *scenario_id, const AppStateTransitionSequenceMetadata *metadata) {
  size_t index;

  if (!AppStateNonEmptyString(scenario_id))
    return 0;
  if (metadata == NULL || !AppStateNonEmptyString(metadata->scenario_id) ||
      strcmp(metadata->scenario_id, scenario_id))
    return 0;
  if (!AppStateNonEmptyString(metadata->category) ||
      !AppStateNonEmptyString(metadata->flow) ||
      !AppStateNonEmptyString(metadata->description) ||
      !AppStateNonEmptyString(metadata->coverage_status) ||
      strcmp(metadata->coverage_status, "runtime_backed") != 0 ||
      metadata->steps == NULL || metadata->step_count == 0)
    return 0;

  for (index = 0; index < metadata->step_count; index++) {
    if (!AppStateTransitionSequenceStepReady(&metadata->steps[index]))
      return 0;
  }

  return 1;
}

int AppStateValidatedTransitionSequence(const char *scenario_id) {
  return AppStateValidateTransitionSequence(
      scenario_id, AppStateTransitionSequenceLookup(scenario_id));
}

static int AppStateValidateActionCoverage(
    YtreeNovaAction action, const AppStateActionCoverageMetadata *coverage) {
  const AppStateTransitionMetadata *transition;

  if ((int)action < 0 || (size_t)action >= AppStateActionCoverageCount())
    return 0;
  if (coverage == NULL || coverage->action != action)
    return 0;
  transition = AppStateTransitionLookup(coverage->transition_id);
  if (!AppStateValidateTransition(coverage->transition_id, transition))
    return 0;
  if (!AppStateNonEmptyString(coverage->action_name) ||
      !AppStateNonEmptyString(coverage->category) ||
      !AppStateNonEmptyString(coverage->owner) ||
      !AppStateNonEmptyString(coverage->boundary_status) ||
      strcmp(coverage->category, transition->category) != 0 ||
      strcmp(coverage->owner, transition->owner) != 0)
    return 0;
  if (!AppStateKnownBoundaryStatus(coverage->boundary_status))
    return 0;
  if (!AppStateStringListEquals(coverage->declared_write_set,
                                coverage->declared_write_set_count,
                                transition->declared_write_set,
                                transition->declared_write_set_count))
    return 0;
  if (!AppStateCoverageOwnerFieldsMatchWriteSet(
          coverage->owner_field_refs, coverage->owner_field_ref_count,
          coverage->declared_write_set, coverage->declared_write_set_count))
    return 0;
  if (!AppStateNonEmptyStringList(coverage->transition_sequence_refs,
                                  coverage->transition_sequence_ref_count) ||
      !AppStateNonEmptyStringList(coverage->dispatch_surface_refs,
                                  coverage->dispatch_surface_ref_count) ||
      !AppStateNonEmptyStringList(coverage->invariant_refs,
                                  coverage->invariant_ref_count) ||
      !AppStateNonEmptyStringList(coverage->generation_domain_refs,
                                  coverage->generation_domain_ref_count) ||
      !AppStateNonEmptyStringList(coverage->diff_harness_refs,
                                  coverage->diff_harness_ref_count) ||
      !AppStateNonEmptyStringList(coverage->migration_notes,
                                  coverage->migration_note_count))
    return 0;

  return 1;
}

static YtreeNovaAction AppStateValidateKeyActionCoverage(
    YtreeNovaAction action, const AppStateActionCoverageMetadata *coverage) {
  if (!AppStateValidateActionCoverage(action, coverage))
    return ACTION_NONE;

  return action;
}

YtreeNovaAction AppStateValidatedKeyAction(YtreeNovaAction action) {
  return AppStateValidateKeyActionCoverage(action,
                                           AppStateActionCoverageLookup(action));
}

static int
AppStateValidateEventCoverage(const char *event_id,
                              const AppStateEventCoverageMetadata *coverage) {
  const AppStateTransitionMetadata *transition;

  if (event_id == NULL || event_id[0] == '\0')
    return 0;
  if (coverage == NULL || coverage->event_id == NULL ||
      strcmp(coverage->event_id, event_id))
    return 0;
  transition = AppStateTransitionLookup(coverage->transition_id);
  if (!AppStateValidateTransition(coverage->transition_id, transition))
    return 0;
  if (!AppStateNonEmptyString(coverage->event_class) ||
      !AppStateNonEmptyString(coverage->category) ||
      !AppStateNonEmptyString(coverage->source) ||
      !AppStateNonEmptyString(coverage->owner) ||
      !AppStateNonEmptyString(coverage->boundary_status) ||
      strcmp(coverage->category, transition->category) != 0 ||
      strcmp(coverage->owner, transition->owner) != 0)
    return 0;
  if (!AppStateKnownBoundaryStatus(coverage->boundary_status))
    return 0;
  if (!AppStateStringListEquals(coverage->declared_write_set,
                                coverage->declared_write_set_count,
                                transition->declared_write_set,
                                transition->declared_write_set_count))
    return 0;
  if (!AppStateCoverageOwnerFieldsMatchWriteSet(
          coverage->owner_field_refs, coverage->owner_field_ref_count,
          coverage->declared_write_set, coverage->declared_write_set_count))
    return 0;
  if (!AppStateNonEmptyStringList(coverage->trigger_paths,
                                  coverage->trigger_path_count) ||
      !AppStateNonEmptyStringList(coverage->transition_sequence_refs,
                                  coverage->transition_sequence_ref_count) ||
      !AppStateNonEmptyStringList(coverage->dispatch_surface_refs,
                                  coverage->dispatch_surface_ref_count) ||
      !AppStateNonEmptyStringList(coverage->invariant_refs,
                                  coverage->invariant_ref_count) ||
      !AppStateNonEmptyStringList(coverage->generation_domain_refs,
                                  coverage->generation_domain_ref_count) ||
      !AppStateNonEmptyStringList(coverage->diff_harness_refs,
                                  coverage->diff_harness_ref_count) ||
      !AppStateNonEmptyStringList(coverage->migration_notes,
                                  coverage->migration_note_count))
    return 0;

  return 1;
}

int AppStateValidatedEvent(const char *event_id) {
  return AppStateValidateEventCoverage(event_id,
                                       AppStateEventCoverageLookup(event_id));
}

static int AppStateTransitionSequenceRefsCoverTransition(
    const char *const *sequence_refs, size_t sequence_ref_count,
    const char *transition_id) {
  size_t ref_index;

  if (!AppStateNonEmptyStringList(sequence_refs, sequence_ref_count) ||
      !AppStateNonEmptyString(transition_id))
    return 0;

  for (ref_index = 0; ref_index < sequence_ref_count; ref_index++) {
    const AppStateTransitionSequenceMetadata *sequence =
        AppStateTransitionSequenceLookup(sequence_refs[ref_index]);
    size_t previous_index;
    size_t step_index;
    int transition_seen = 0;

    if (sequence == NULL || sequence->steps == NULL ||
        sequence->step_count == 0)
      return 0;
    for (previous_index = 0; previous_index < ref_index; previous_index++) {
      if (strcmp(sequence_refs[previous_index], sequence_refs[ref_index]) == 0)
        return 0;
    }
    for (step_index = 0; step_index < sequence->step_count; step_index++) {
      const AppStateTransitionSequenceStepMetadata *step =
          &sequence->steps[step_index];

      if (!AppStateNonEmptyString(step->transition_id))
        return 0;
      if (strcmp(step->transition_id, transition_id) == 0)
        transition_seen = 1;
    }
    if (!transition_seen)
      return 0;
  }

  return 1;
}

static int AppStateDispatchSurfaceAllowedWritesReady(
    const AppStateDispatchSurfaceMetadata *metadata,
    const AppStateTransitionMetadata *transition) {
  size_t write_index;

  if (metadata == NULL || transition == NULL)
    return 0;
  if (metadata->allowed_direct_write_count == 0)
    return metadata->allowed_direct_writes == NULL;
  if (metadata->allowed_direct_writes == NULL)
    return 0;

  for (write_index = 0; write_index < metadata->allowed_direct_write_count;
       write_index++) {
    const char *field = metadata->allowed_direct_writes[write_index];

    if (!AppStateNonEmptyString(field))
      return 0;
    if (AppStateOwnerFieldLookup(field) == NULL)
      return 0;
    if (!AppStateStringListContains(transition->declared_write_set,
                                    transition->declared_write_set_count,
                                    field))
      return 0;
    if (AppStateStringListContains(metadata->allowed_direct_writes, write_index,
                                   field))
      return 0;
  }

  return 1;
}

static int AppStateValidateDispatchSurface(
    const char *surface_id, const AppStateDispatchSurfaceMetadata *metadata) {
  const AppStateTransitionMetadata *transition;

  if (surface_id == NULL || surface_id[0] == '\0')
    return 0;
  if (metadata == NULL || metadata->surface_id == NULL ||
      strcmp(metadata->surface_id, surface_id))
    return 0;
  transition = AppStateTransitionLookup(metadata->transition_id);
  if (!AppStateValidateTransition(metadata->transition_id, transition))
    return 0;
  if (!AppStateNonEmptyString(metadata->category) ||
      !AppStateNonEmptyString(metadata->source_path) ||
      !AppStateNonEmptyString(metadata->entry_symbol_or_path) ||
      !AppStateNonEmptyString(metadata->boundary_status))
    return 0;
  if (!AppStateKnownBoundaryStatus(metadata->boundary_status))
    return 0;
  if (!AppStateDispatchSurfaceAllowedWritesReady(metadata, transition))
    return 0;
  if (!AppStateTransitionSequenceRefsCoverTransition(
          metadata->transition_sequence_refs,
          metadata->transition_sequence_ref_count, metadata->transition_id))
    return 0;
  if (!AppStateNonEmptyStringList(metadata->migration_notes,
                                  metadata->migration_note_count))
    return 0;

  return 1;
}

int AppStateValidatedDispatchSurface(const char *surface_id) {
  return AppStateValidateDispatchSurface(
      surface_id, AppStateDispatchSurfaceLookup(surface_id));
}

static int AppStateStringListHasDuplicate(const char *const *values,
                                          size_t value_count) {
  size_t index;

  if (!AppStateNonEmptyStringList(values, value_count))
    return 1;

  for (index = 0; index < value_count; index++) {
    if (AppStateStringListContains(values, index, values[index]))
      return 1;
  }

  return 0;
}

static int AppStateCompatibilityShimWriteCapabilityKnown(
    const AppStateCompatibilityShimMetadata *metadata) {
  if (metadata == NULL || !AppStateNonEmptyString(metadata->write_capability))
    return 0;

  return strcmp(metadata->write_capability, "write_capable") == 0 ||
         strcmp(metadata->write_capability, "read_only_projection") == 0 ||
         strcmp(metadata->write_capability, "no_write") == 0;
}

static int AppStateCompatibilityShimWriteCapable(
    const AppStateCompatibilityShimMetadata *metadata) {
  return metadata != NULL && AppStateNonEmptyString(metadata->write_capability) &&
         strcmp(metadata->write_capability, "write_capable") == 0;
}

static int AppStateCompatibilityShimReadOnlyProjection(
    const AppStateCompatibilityShimMetadata *metadata) {
  return metadata != NULL && AppStateNonEmptyString(metadata->write_capability) &&
         strcmp(metadata->write_capability, "read_only_projection") == 0;
}

static int AppStateCompatibilityShimInvariantRefsReady(
    const AppStateCompatibilityShimMetadata *metadata) {
  size_t index;

  if (metadata == NULL ||
      !AppStateNonEmptyStringList(metadata->invariant_checks,
                                  metadata->invariant_check_count) ||
      AppStateStringListHasDuplicate(metadata->invariant_checks,
                                     metadata->invariant_check_count))
    return 0;

  for (index = 0; index < metadata->invariant_check_count; index++) {
    if (!AppStateValidatedInvariant(metadata->invariant_checks[index]))
      return 0;
  }

  return 1;
}

static int AppStateCompatibilityShimGenerationDomainRefsReady(
    const AppStateCompatibilityShimMetadata *metadata) {
  size_t index;

  if (metadata == NULL ||
      !AppStateNonEmptyStringList(metadata->generation_domain_refs,
                                  metadata->generation_domain_ref_count) ||
      AppStateStringListHasDuplicate(metadata->generation_domain_refs,
                                     metadata->generation_domain_ref_count))
    return 0;

  for (index = 0; index < metadata->generation_domain_ref_count; index++) {
    if (!AppStateValidatedGenerationDomain(
            metadata->generation_domain_refs[index]))
      return 0;
  }

  return 1;
}

static int AppStateCompatibilityShimDiffHarnessRefsReady(
    const AppStateCompatibilityShimMetadata *metadata) {
  size_t index;

  if (metadata == NULL ||
      !AppStateNonEmptyStringList(metadata->diff_harness_refs,
                                  metadata->diff_harness_ref_count) ||
      AppStateStringListHasDuplicate(metadata->diff_harness_refs,
                                     metadata->diff_harness_ref_count))
    return 0;

  for (index = 0; index < metadata->diff_harness_ref_count; index++) {
    if (!AppStateValidatedDiffHarness(metadata->diff_harness_refs[index]))
      return 0;
  }

  return 1;
}

static int AppStateValidateCompatibilityShim(
    const char *shim_id, const AppStateCompatibilityShimMetadata *metadata) {
  const AppStateTransitionMetadata *transition;
  size_t owner_field_index;

  if (!AppStateNonEmptyString(shim_id))
    return 0;
  if (metadata == NULL || !AppStateNonEmptyString(metadata->id) ||
      strcmp(metadata->id, shim_id))
    return 0;
  if (!AppStateNonEmptyString(metadata->owner) ||
      !AppStateNonEmptyString(metadata->old_authority_path) ||
      !AppStateNonEmptyString(metadata->read_permission) ||
      !AppStateNonEmptyString(metadata->write_permission) ||
      !AppStateCompatibilityShimWriteCapabilityKnown(metadata) ||
      !AppStateNonEmptyString(metadata->removal_trigger) ||
      !AppStateNonEmptyString(metadata->target_transition) ||
      !AppStateNonEmptyString(metadata->follow_up_task) ||
      !AppStateNonEmptyString(metadata->qa_enforcement))
    return 0;
  transition = AppStateTransitionLookup(metadata->target_transition);
  if (!AppStateValidateTransition(metadata->target_transition, transition))
    return 0;
  if (!AppStateCompatibilityShimInvariantRefsReady(metadata))
    return 0;
  if (!AppStateTransitionFieldsRegistered(metadata->owner_field_refs,
                                          metadata->owner_field_ref_count))
    return 0;
  if (AppStateStringListHasDuplicate(metadata->owner_field_refs,
                                     metadata->owner_field_ref_count))
    return 0;
  if (!AppStateCompatibilityShimGenerationDomainRefsReady(metadata))
    return 0;
  if (!AppStateCompatibilityShimDiffHarnessRefsReady(metadata))
    return 0;

  for (owner_field_index = 0;
       owner_field_index < metadata->owner_field_ref_count;
       owner_field_index++) {
    int field_in_transition = AppStateStringListContains(
        transition->declared_write_set, transition->declared_write_set_count,
        metadata->owner_field_refs[owner_field_index]);

    if (AppStateCompatibilityShimWriteCapable(metadata) && !field_in_transition)
      return 0;
    if (AppStateCompatibilityShimReadOnlyProjection(metadata) &&
        field_in_transition)
      return 0;
  }

  return 1;
}

int AppStateValidatedCompatibilityShim(const char *shim_id) {
  return AppStateValidateCompatibilityShim(
      shim_id, AppStateCompatibilityShimLookup(shim_id));
}

const AppStateEventCoverageMetadata *
AppStateEventCoverageLookup(const char *event_id) {
  size_t index;

  if (event_id == NULL || event_id[0] == '\0')
    return NULL;

  for (index = 0; index < AppStateEventCoverageCount(); index++) {
    if (AppStateLookupIdMatches(kAppStateEventCoverages[index].event_id,
                                event_id))
      return &kAppStateEventCoverages[index];
  }

  return NULL;
}
