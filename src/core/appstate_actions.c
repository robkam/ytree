/***************************************************************************
 *
 * src/core/appstate_actions.c
 * Runtime lookup table for YtreeNovaAction AppState transition metadata.
 *
 ***************************************************************************/

#include "ytnova_appstate_actions.h"
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

static const char *const kAppStateOwnerFieldInvariantChecks0[] = {
  "Active panel changes must preserve inactive panel-local records.",
  "Panel routing must resolve to an existing YtreeNovaPanel carrier before render projection.",
};
static const char *const kAppStateOwnerFieldInvariantChecks1[] = {
  "Command completion must not mutate panel or volume fields outside the declared follow-up transition.",
  "Command failure or cancellation must preserve authoritative panel and volume state.",
};
static const char *const kAppStateOwnerFieldInvariantChecks2[] = {
  "Blocked transitions may write message state only when needed for outcome clarity.",
  "Message writes must not imply filesystem or panel state success.",
};
static const char *const kAppStateOwnerFieldInvariantChecks3[] = {
  "Modal dismissal must leave underlying panel and volume authority unchanged unless declared.",
  "Destructive confirmations must remain explicit and default-safe.",
};
static const char *const kAppStateOwnerFieldInvariantChecks4[] = {
  "Pending transitions must name a registered transition before later mutation.",
  "Queueing a transition must not advance panel or volume generations by itself.",
};
static const char *const kAppStateOwnerFieldInvariantChecks5[] = {
  "Releasing a volume must not leave active or inactive panels with stale volume pointers.",
  "Volume registry changes must rebind affected panels through stable identity or deterministic fallback.",
};
static const char *const kAppStateOwnerFieldInvariantChecks6[] = {
  "Resize handling must not choose a new authoritative selection from rendered rows.",
  "Layout changes alone must not advance volume_generation.",
};
static const char *const kAppStateOwnerFieldInvariantChecks7[] = {
  "Render invalidation writes must not mutate selection, focus, or identity state.",
  "Render projection may clear only surfaces produced from settled AppState records.",
};
static const char *const kAppStateOwnerFieldInvariantChecks8[] = {
  "Ncurses window recreation must happen outside signal handlers.",
  "Window handles must not become restore authority for panel selection or viewport state.",
};
static const char *const kAppStateOwnerFieldInvariantChecks9[] = {
  "File selection identity must be path/name based, not a stale FileEntry pointer.",
  "Inactive panel file selection must remain frozen across active-only actions.",
};
static const char *const kAppStateOwnerFieldInvariantChecks10[] = {
  "File viewport correction must preserve the stored selection when it remains visible.",
  "Render paths may compute temporary file projection without overwriting this origin.",
};
static const char *const kAppStateOwnerFieldInvariantChecks11[] = {
  "Reactivation must restore the recorded tree/small-file/big-file shape directly.",
  "Focus shape changes must not import the inactive panel's state.",
};
static const char *const kAppStateOwnerFieldInvariantChecks12[] = {
  "Render projection alone must not advance panel_generation.",
  "Snapshot restore must compare panel_generation before applying saved state.",
};
static const char *const kAppStateOwnerFieldInvariantChecks13[] = {
  "Restore snapshots must use stable identities plus generation validation.",
  "Stale snapshots must fail closed through the deterministic fallback order.",
};
static const char *const kAppStateOwnerFieldInvariantChecks14[] = {
  "Tree cursor position is interpreted against visible rows, not raw topology indices.",
  "Inactive panel cursor must remain unchanged unless the transition explicitly targets that panel.",
};
static const char *const kAppStateOwnerFieldInvariantChecks15[] = {
  "Tree selection identity must be stable and path based within the current volume namespace.",
  "Selection restore must not derive authority from disp_begin_pos plus cursor_pos.",
};
static const char *const kAppStateOwnerFieldInvariantChecks16[] = {
  "Hidden dotfile visibility must not cause extra viewport shifts when selection remains visible.",
  "Render-side temporary placement must not overwrite the saved viewport origin.",
};
static const char *const kAppStateOwnerFieldInvariantChecks17[] = {
  "Panel volume binding must point to an existing volume/archive namespace or deterministic fallback.",
  "A panel must not import the opposite panel's volume-specific snapshot.",
};
static const char *const kAppStateOwnerFieldInvariantChecks18[] = {
  "Shared topology changes must re-anchor each panel by its own stable identity.",
  "Topology mutation must advance volume_generation before restore consumers observe it.",
};
static const char *const kAppStateOwnerFieldInvariantChecks19[] = {
  "Logged state is shared topology and must not store panel-local cursor or tag authority.",
  "Unlogged placeholders must not degrade a frozen file-view anchor without rebind or payload reload.",
};
static const char *const kAppStateOwnerFieldInvariantChecks20[] = {
  "Payload cache changes must not overwrite panel-local file selection identity.",
  "Mutation-result commits must distinguish filesystem side effects from AppState metadata updates.",
};
static const char *const kAppStateOwnerFieldInvariantChecks21[] = {
  "Render projection alone must not advance volume_generation.",
  "Generation mismatch must force stable-identity re-resolution before snapshot restore.",
};

static const AppStateOwnerFieldMetadata kAppStateOwnerFields[] = {
  {"ctx.active",
   "ctx/session state",
   "ViewContext.session routing",
   "ViewContext.active",
   "May change only during an allowed panel-routing or volume/menu transition commit.",
   "documented_foundation_only",
   kAppStateOwnerFieldInvariantChecks0,
   sizeof(kAppStateOwnerFieldInvariantChecks0) / sizeof(kAppStateOwnerFieldInvariantChecks0[0])},
  {"ctx.command_state",
   "ctx/session state",
   "ViewContext.command_region",
   "ViewContext command state fields",
   "May change only through command start/completion/cancel transitions.",
   "documented_foundation_only",
   kAppStateOwnerFieldInvariantChecks1,
   sizeof(kAppStateOwnerFieldInvariantChecks1) / sizeof(kAppStateOwnerFieldInvariantChecks1[0])},
  {"ctx.message_state",
   "ctx/session state",
   "ViewContext.message_region",
   "ViewContext message/footer state fields",
   "May change only through transitions that declare user-visible outcome or constraint messaging.",
   "documented_foundation_only",
   kAppStateOwnerFieldInvariantChecks2,
   sizeof(kAppStateOwnerFieldInvariantChecks2) / sizeof(kAppStateOwnerFieldInvariantChecks2[0])},
  {"ctx.modal_state",
   "ctx/session state",
   "ViewContext.modal_region",
   "ViewContext modal/dialog state fields",
   "May change only when entering, completing, or dismissing a registered modal transition.",
   "documented_foundation_only",
   kAppStateOwnerFieldInvariantChecks3,
   sizeof(kAppStateOwnerFieldInvariantChecks3) / sizeof(kAppStateOwnerFieldInvariantChecks3[0])},
  {"ctx.pending_transition",
   "ctx/session state",
   "ViewContext.transition_queue",
   "ViewContext pending transition marker",
   "May be queued only by transitions that declare a deterministic follow-up boundary.",
   "documented_foundation_only",
   kAppStateOwnerFieldInvariantChecks4,
   sizeof(kAppStateOwnerFieldInvariantChecks4) / sizeof(kAppStateOwnerFieldInvariantChecks4[0])},
  {"ctx.volumes_head",
   "volume/shared topology and payload state",
   "ViewContext.volume_registry",
   "ViewContext.volumes_head",
   "May change only through volume lifecycle transitions that validate panel bindings.",
   "documented_foundation_only",
   kAppStateOwnerFieldInvariantChecks5,
   sizeof(kAppStateOwnerFieldInvariantChecks5) / sizeof(kAppStateOwnerFieldInvariantChecks5[0])},
  {"ctx.layout",
   "render/projection/invalidation state",
   "ViewContext.layout_region",
   "ViewContext layout geometry fields",
   "May change only during resize/reflow transitions executed in the main loop.",
   "documented_foundation_only",
   kAppStateOwnerFieldInvariantChecks6,
   sizeof(kAppStateOwnerFieldInvariantChecks6) / sizeof(kAppStateOwnerFieldInvariantChecks6[0])},
  {"ctx.render_dirty_flags",
   "render/projection/invalidation state",
   "ViewContext.render_region",
   "ViewContext render invalidation fields",
   "May change only when a transition marks or clears declared projection surfaces.",
   "documented_foundation_only",
   kAppStateOwnerFieldInvariantChecks7,
   sizeof(kAppStateOwnerFieldInvariantChecks7) / sizeof(kAppStateOwnerFieldInvariantChecks7[0])},
  {"ctx.window_handles",
   "render/projection/invalidation state",
   "ViewContext ncurses window/layout handles",
   "ViewContext window handle fields",
   "May change only during main-loop layout/reflow or render projection setup.",
   "documented_foundation_only",
   kAppStateOwnerFieldInvariantChecks8,
   sizeof(kAppStateOwnerFieldInvariantChecks8) / sizeof(kAppStateOwnerFieldInvariantChecks8[0])},
  {"panel.file_selection_key",
   "panel-local state",
   "YtreeNovaPanel.file identity owner",
   "YtreeNovaPanel file selection identity fields",
   "May change only for the targeted panel through navigation, restore, or rebind transitions.",
   "documented_foundation_only",
   kAppStateOwnerFieldInvariantChecks9,
   sizeof(kAppStateOwnerFieldInvariantChecks9) / sizeof(kAppStateOwnerFieldInvariantChecks9[0])},
  {"panel.file_viewport_origin",
   "panel-local state",
   "YtreeNovaPanel.file viewport owner",
   "YtreeNovaPanel file viewport fields",
   "May change only for the targeted panel when navigation, bounds correction, or reflow declares it.",
   "documented_foundation_only",
   kAppStateOwnerFieldInvariantChecks10,
   sizeof(kAppStateOwnerFieldInvariantChecks10) / sizeof(kAppStateOwnerFieldInvariantChecks10[0])},
  {"panel.focus_shape",
   "panel-local state",
   "YtreeNovaPanel.focus owner",
   "YtreeNovaPanel focus/window-shape fields",
   "May change only during allowed focus, modal restore, split, or file/tree transition commits.",
   "documented_foundation_only",
   kAppStateOwnerFieldInvariantChecks11,
   sizeof(kAppStateOwnerFieldInvariantChecks11) / sizeof(kAppStateOwnerFieldInvariantChecks11[0])},
  {"panel.panel_generation",
   "panel-local state",
   "YtreeNovaPanel.generation owner",
   "YtreeNovaPanel panel generation field",
   "May increment only when panel-local restore authority changes.",
   "documented_foundation_only",
   kAppStateOwnerFieldInvariantChecks12,
   sizeof(kAppStateOwnerFieldInvariantChecks12) / sizeof(kAppStateOwnerFieldInvariantChecks12[0])},
  {"panel.restore_snapshot",
   "panel-local state",
   "YtreeNovaPanel.restore snapshot owner",
   "YtreeNovaPanel restore snapshot fields",
   "May change only through snapshot capture, rebind, fallback, or volume-binding transitions.",
   "documented_foundation_only",
   kAppStateOwnerFieldInvariantChecks13,
   sizeof(kAppStateOwnerFieldInvariantChecks13) / sizeof(kAppStateOwnerFieldInvariantChecks13[0])},
  {"panel.tree_cursor_pos",
   "panel-local state",
   "YtreeNovaPanel.tree cursor owner",
   "YtreeNovaPanel tree cursor field",
   "May change only for the targeted panel during tree navigation or rebind/fallback transitions.",
   "documented_foundation_only",
   kAppStateOwnerFieldInvariantChecks14,
   sizeof(kAppStateOwnerFieldInvariantChecks14) / sizeof(kAppStateOwnerFieldInvariantChecks14[0])},
  {"panel.tree_selection_key",
   "panel-local state",
   "YtreeNovaPanel.tree selection owner",
   "YtreeNovaPanel tree selection identity fields",
   "May change only through tree navigation, restore, or topology rebind transitions for the targeted panel.",
   "documented_foundation_only",
   kAppStateOwnerFieldInvariantChecks15,
   sizeof(kAppStateOwnerFieldInvariantChecks15) / sizeof(kAppStateOwnerFieldInvariantChecks15[0])},
  {"panel.tree_viewport_origin",
   "panel-local state",
   "YtreeNovaPanel.tree viewport owner",
   "YtreeNovaPanel tree viewport fields",
   "May change only when navigation, bounds correction, resize, or rebind declares viewport mutation.",
   "documented_foundation_only",
   kAppStateOwnerFieldInvariantChecks16,
   sizeof(kAppStateOwnerFieldInvariantChecks16) / sizeof(kAppStateOwnerFieldInvariantChecks16[0])},
  {"panel.volume_key",
   "panel-local state",
   "YtreeNovaPanel.volume binding owner",
   "YtreeNovaPanel volume binding fields",
   "May change only through volume selection, cycle, release, or restore transitions.",
   "documented_foundation_only",
   kAppStateOwnerFieldInvariantChecks17,
   sizeof(kAppStateOwnerFieldInvariantChecks17) / sizeof(kAppStateOwnerFieldInvariantChecks17[0])},
  {"volume.dir_tree",
   "volume/shared topology and payload state",
   "Volume.topology owner",
   "Volume directory tree fields",
   "May change only through logging, rebuild, refresh, release, or completed filesystem mutation transitions.",
   "documented_foundation_only",
   kAppStateOwnerFieldInvariantChecks18,
   sizeof(kAppStateOwnerFieldInvariantChecks18) / sizeof(kAppStateOwnerFieldInvariantChecks18[0])},
  {"volume.logged_state",
   "volume/shared topology and payload state",
   "Volume.logged topology owner",
   "Volume logged/unlogged directory state",
   "May change only through explicit log, relog, release, collapse, refresh, or rebuild transitions.",
   "documented_foundation_only",
   kAppStateOwnerFieldInvariantChecks19,
   sizeof(kAppStateOwnerFieldInvariantChecks19) / sizeof(kAppStateOwnerFieldInvariantChecks19[0])},
  {"volume.payload_cache",
   "volume/shared topology and payload state",
   "Volume.payload cache owner",
   "Volume file payload/statistics cache fields",
   "May change only through payload load, refresh, archive, or completed filesystem mutation transitions.",
   "documented_foundation_only",
   kAppStateOwnerFieldInvariantChecks20,
   sizeof(kAppStateOwnerFieldInvariantChecks20) / sizeof(kAppStateOwnerFieldInvariantChecks20[0])},
  {"volume.volume_generation",
   "volume/shared topology and payload state",
   "Volume.generation owner",
   "Volume topology/payload generation field",
   "May increment only when shared topology, payload identity, logged state, or namespace mapping changes.",
   "documented_foundation_only",
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

static const char *const kAppStateGenerationDomainAdvancesOnTransitionIds0[] = {
  "transition.keybinding.navigate-tree",
  "transition.menu-action.volume-select",
  "transition.modal-action.dismiss",
  "transition.refresh-rebuild.manual-refresh",
  "transition.volume-operation.release-cycle",
  "transition.terminal-signal-resize",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
  "transition.rebuild-rebind-callback.panel-anchor",
};

static const char *const kAppStateGenerationDomainMigrationNotes0[] = {
  "Runtime panel_generation is still documented foundation; later runtime migration must wire this field to the canonical panel UI state record.",
};

static const char *const kAppStateGenerationDomainIdentityFields1[] = {
  "ctx.volumes_head",
  "volume.dir_tree",
  "volume.logged_state",
  "volume.payload_cache",
  "panel.volume_key",
};

static const char *const kAppStateGenerationDomainAdvancesOnTransitionIds1[] = {
  "transition.refresh-rebuild.manual-refresh",
  "transition.volume-operation.release-cycle",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
};

static const char *const kAppStateGenerationDomainMigrationNotes1[] = {
  "Runtime volume_generation is still documented foundation; later runtime migration must attach it to Volume topology and payload commits.",
};

static const char *const kAppStateGenerationDomainIdentityFields2[] = {
  "panel.tree_selection_key",
  "panel.tree_cursor_pos",
  "panel.tree_viewport_origin",
  "volume.dir_tree",
  "volume.logged_state",
};

static const char *const kAppStateGenerationDomainAdvancesOnTransitionIds2[] = {
  "transition.refresh-rebuild.manual-refresh",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
  "transition.volume-operation.release-cycle",
  "transition.rebuild-rebind-callback.panel-anchor",
};

static const char *const kAppStateGenerationDomainMigrationNotes2[] = {
  "Directory identity has no dedicated runtime generation counter yet; volume.volume_generation is the closest authoritative invalidation owner for topology changes.",
};

static const char *const kAppStateGenerationDomainIdentityFields3[] = {
  "panel.file_selection_key",
  "panel.file_viewport_origin",
  "volume.payload_cache",
  "volume.dir_tree",
};

static const char *const kAppStateGenerationDomainAdvancesOnTransitionIds3[] = {
  "transition.refresh-rebuild.manual-refresh",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
  "transition.rebuild-rebind-callback.panel-anchor",
};

static const char *const kAppStateGenerationDomainMigrationNotes3[] = {
  "File identity has no dedicated runtime generation counter yet; volume.volume_generation is the closest authoritative owner for payload and namespace invalidation.",
};

static const char *const kAppStateGenerationDomainIdentityFields4[] = {
  "panel.focus_shape",
  "panel.restore_snapshot",
};

static const char *const kAppStateGenerationDomainAdvancesOnTransitionIds4[] = {
  "transition.keybinding.navigate-tree",
  "transition.modal-action.dismiss",
  "transition.menu-action.volume-select",
  "transition.rebuild-rebind-callback.panel-anchor",
};

static const char *const kAppStateGenerationDomainMigrationNotes4[] = {
  "Focus-shape invalidation is represented by panel.panel_generation until a narrower runtime focus-shape generation exists.",
};

static const char *const kAppStateGenerationDomainIdentityFields5[] = {
  "ctx.modal_state",
  "ctx.command_state",
  "ctx.pending_transition",
  "ctx.message_state",
};

static const char *const kAppStateGenerationDomainAdvancesOnTransitionIds5[] = {
  "transition.modal-action.dismiss",
  "transition.command-completion.user-command",
};

static const char *const kAppStateGenerationDomainMigrationNotes5[] = {
  "No dedicated ctx command/modal generation exists yet; panel.panel_generation is the closest guard for command paths that restore focus or queue panel-local follow-up work.",
};

static const char *const kAppStateGenerationDomainIdentityFields6[] = {
  "panel.tree_selection_key",
  "panel.tree_viewport_origin",
  "panel.file_selection_key",
  "panel.file_viewport_origin",
  "volume.logged_state",
};

static const char *const kAppStateGenerationDomainAdvancesOnTransitionIds6[] = {
  "transition.refresh-rebuild.manual-refresh",
  "transition.keybinding.navigate-tree",
  "transition.rebuild-rebind-callback.panel-anchor",
};

static const char *const kAppStateGenerationDomainMigrationNotes6[] = {
  "Dedicated filter and dotfile visibility owner fields are not registered yet; panel.panel_generation is the closest panel-local invalidation owner.",
};

static const char *const kAppStateGenerationDomainIdentityFields7[] = {
  "volume.dir_tree",
  "volume.logged_state",
  "ctx.volumes_head",
};

static const char *const kAppStateGenerationDomainAdvancesOnTransitionIds7[] = {
  "transition.refresh-rebuild.manual-refresh",
  "transition.volume-operation.release-cycle",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
};

static const char *const kAppStateGenerationDomainMigrationNotes7[] = {
  "Topology generation is represented by volume.volume_generation until runtime Volume commit hooks are migrated.",
};

static const char *const kAppStateGenerationDomainIdentityFields8[] = {
  "volume.payload_cache",
  "panel.file_selection_key",
  "panel.file_viewport_origin",
};

static const char *const kAppStateGenerationDomainAdvancesOnTransitionIds8[] = {
  "transition.refresh-rebuild.manual-refresh",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
  "transition.rebuild-rebind-callback.panel-anchor",
};

static const char *const kAppStateGenerationDomainMigrationNotes8[] = {
  "File payload invalidation uses volume.volume_generation until a narrower payload generation field is registered.",
};

static const char *const kAppStateGenerationDomainIdentityFields9[] = {
  "ctx.volumes_head",
  "panel.volume_key",
  "panel.restore_snapshot",
  "volume.logged_state",
};

static const char *const kAppStateGenerationDomainAdvancesOnTransitionIds9[] = {
  "transition.volume-operation.release-cycle",
  "transition.menu-action.volume-select",
  "transition.refresh-rebuild.manual-refresh",
};

static const char *const kAppStateGenerationDomainMigrationNotes9[] = {
  "Volume lifecycle has no separate registry generation yet; volume.volume_generation is the closest shared invalidation owner while ctx.volumes_head remains the registry identity field.",
};

static const char *const kAppStateGenerationDomainIdentityFields10[] = {
  "ctx.layout",
  "ctx.window_handles",
  "ctx.render_dirty_flags",
  "panel.tree_viewport_origin",
  "panel.file_viewport_origin",
};

static const char *const kAppStateGenerationDomainAdvancesOnTransitionIds10[] = {
  "transition.terminal-signal-resize",
};

static const char *const kAppStateGenerationDomainMigrationNotes10[] = {
  "Render projection itself is read-only/projection-only and does not advance generations; terminal resize uses panel.panel_generation only when saved viewport origins are corrected.",
};

static const AppStateGenerationDomainMetadata kAppStateGenerationDomains[] = {
  {"generation.panel.local-authority",
   "panel_generation",
   "panel-local state",
   "panel.panel_generation",
   kAppStateGenerationDomainIdentityFields0,
   sizeof(kAppStateGenerationDomainIdentityFields0) /
       sizeof(kAppStateGenerationDomainIdentityFields0[0]),
   kAppStateGenerationDomainAdvancesOnTransitionIds0,
   sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds0) /
       sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds0[0]),
   "Reject snapshots whose saved panel_generation does not match the panel-local generation marker before restore authority is applied.",
   "Re-resolve by stable identity, then nearest visible ancestor, next visible sibling, previous visible sibling, and finally root visible node.",
   "Canonical panel-anchor restore helpers commit panel-local selection, viewport, focus shape, and snapshot generation together.",
   "documented_foundation_only",
   kAppStateGenerationDomainMigrationNotes0,
   sizeof(kAppStateGenerationDomainMigrationNotes0) /
       sizeof(kAppStateGenerationDomainMigrationNotes0[0])},
  {"generation.volume.shared-authority",
   "volume_generation",
   "volume/shared topology and payload state",
   "volume.volume_generation",
   kAppStateGenerationDomainIdentityFields1,
   sizeof(kAppStateGenerationDomainIdentityFields1) /
       sizeof(kAppStateGenerationDomainIdentityFields1[0]),
   kAppStateGenerationDomainAdvancesOnTransitionIds1,
   sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds1) /
       sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds1[0]),
   "Treat any saved volume_generation mismatch as stale and require topology/payload identity re-resolution before panel snapshots are reused.",
   "Keep the previous settled topology on blocked transitions; after invalidation, rebind panels through stable identities or fall back to root visible node.",
   "Refresh/rebuild and mutation-result commits advance volume generation before panel restore consumers can observe the changed volume.",
   "documented_foundation_only",
   kAppStateGenerationDomainMigrationNotes1,
   sizeof(kAppStateGenerationDomainMigrationNotes1) /
       sizeof(kAppStateGenerationDomainMigrationNotes1[0])},
  {"identity.directory.stable-key",
   "directory_identity",
   "panel-local state plus volume/shared topology and payload state",
   "volume.volume_generation",
   kAppStateGenerationDomainIdentityFields2,
   sizeof(kAppStateGenerationDomainIdentityFields2) /
       sizeof(kAppStateGenerationDomainIdentityFields2[0]),
   kAppStateGenerationDomainAdvancesOnTransitionIds2,
   sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds2) /
       sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds2[0]),
   "Directory snapshots must compare stable path identity against the current volume generation before row or cursor state is trusted.",
   "Exact directory identity, nearest visible ancestor, next visible sibling, previous visible sibling, then root visible node.",
   "Directory rebind runs through panel-anchor helpers after the shared topology generation has settled.",
   "documented_foundation_only",
   kAppStateGenerationDomainMigrationNotes2,
   sizeof(kAppStateGenerationDomainMigrationNotes2) /
       sizeof(kAppStateGenerationDomainMigrationNotes2[0])},
  {"identity.file.stable-key",
   "file_identity",
   "panel-local state plus volume/shared topology and payload state",
   "volume.volume_generation",
   kAppStateGenerationDomainIdentityFields3,
   sizeof(kAppStateGenerationDomainIdentityFields3) /
       sizeof(kAppStateGenerationDomainIdentityFields3[0]),
   kAppStateGenerationDomainAdvancesOnTransitionIds3,
   sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds3) /
       sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds3[0]),
   "File snapshots must revalidate path/name identity and payload membership when volume generation changes.",
   "Preserve the directory anchor when possible and choose a valid visible file only after exact file identity is unavailable.",
   "File identity restore is committed with the panel snapshot after topology or payload mutation has settled.",
   "documented_foundation_only",
   kAppStateGenerationDomainMigrationNotes3,
   sizeof(kAppStateGenerationDomainMigrationNotes3) /
       sizeof(kAppStateGenerationDomainMigrationNotes3[0])},
  {"shape.panel.focus",
   "focus_shape",
   "panel-local state",
   "panel.panel_generation",
   kAppStateGenerationDomainIdentityFields4,
   sizeof(kAppStateGenerationDomainIdentityFields4) /
       sizeof(kAppStateGenerationDomainIdentityFields4[0]),
   kAppStateGenerationDomainAdvancesOnTransitionIds4,
   sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds4) /
       sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds4[0]),
   "Saved focus shape is stale when panel_generation differs and must not be restored by transient render shape.",
   "Restore the recorded panel shape directly or keep the current settled shape without rendering an intermediate guess.",
   "Modal dismissal, panel reactivation, and rebind callbacks commit focus shape only through panel-local state.",
   "documented_foundation_only",
   kAppStateGenerationDomainMigrationNotes4,
   sizeof(kAppStateGenerationDomainMigrationNotes4) /
       sizeof(kAppStateGenerationDomainMigrationNotes4[0])},
  {"target.modal-command.session",
   "modal_command_target",
   "ctx/session state",
   "panel.panel_generation",
   kAppStateGenerationDomainIdentityFields5,
   sizeof(kAppStateGenerationDomainIdentityFields5) /
       sizeof(kAppStateGenerationDomainIdentityFields5[0]),
   kAppStateGenerationDomainAdvancesOnTransitionIds5,
   sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds5) /
       sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds5[0]),
   "Modal and command targets may write panel or volume state only through a declared follow-up transition boundary.",
   "On blocked or failed command completion, preserve authoritative panel and volume state and write only declared message/modal fields.",
   "Modal dismissal and command completion settle session state before any panel restore or refresh follow-up observes the result.",
   "documented_foundation_only",
   kAppStateGenerationDomainMigrationNotes5,
   sizeof(kAppStateGenerationDomainMigrationNotes5) /
       sizeof(kAppStateGenerationDomainMigrationNotes5[0])},
  {"state.visibility-filter.panel-volume",
   "visibility_filter_state",
   "panel-local state plus volume/shared topology and payload state",
   "panel.panel_generation",
   kAppStateGenerationDomainIdentityFields6,
   sizeof(kAppStateGenerationDomainIdentityFields6) /
       sizeof(kAppStateGenerationDomainIdentityFields6[0]),
   kAppStateGenerationDomainAdvancesOnTransitionIds6,
   sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds6) /
       sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds6[0]),
   "Visibility/filter changes that alter rendered rows invalidate saved panel anchors before any snapshot can be reused.",
   "Rebind visible identity through the deterministic fallback order rather than deriving from hidden or filtered row indexes.",
   "Visibility-filter commits update panel generation before rebind and render projection.",
   "documented_foundation_only",
   kAppStateGenerationDomainMigrationNotes6,
   sizeof(kAppStateGenerationDomainMigrationNotes6) /
       sizeof(kAppStateGenerationDomainMigrationNotes6[0])},
  {"state.topology.volume",
   "topology_state",
   "volume/shared topology and payload state",
   "volume.volume_generation",
   kAppStateGenerationDomainIdentityFields7,
   sizeof(kAppStateGenerationDomainIdentityFields7) /
       sizeof(kAppStateGenerationDomainIdentityFields7[0]),
   kAppStateGenerationDomainAdvancesOnTransitionIds7,
   sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds7) /
       sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds7[0]),
   "Topology snapshots are invalid after any volume_generation advance and must be rebuilt or rebound by identity.",
   "Blocked topology changes retain the previous settled tree; completed invalidation falls back panel anchors only after exact identity fails.",
   "Topology rebuild commits settle volume.dir_tree and volume.logged_state before panel rebind callbacks run.",
   "documented_foundation_only",
   kAppStateGenerationDomainMigrationNotes7,
   sizeof(kAppStateGenerationDomainMigrationNotes7) /
       sizeof(kAppStateGenerationDomainMigrationNotes7[0])},
  {"state.file-payload.volume",
   "file_payload_state",
   "volume/shared topology and payload state",
   "volume.volume_generation",
   kAppStateGenerationDomainIdentityFields8,
   sizeof(kAppStateGenerationDomainIdentityFields8) /
       sizeof(kAppStateGenerationDomainIdentityFields8[0]),
   kAppStateGenerationDomainAdvancesOnTransitionIds8,
   sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds8) /
       sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds8[0]),
   "Payload cache identity must be revalidated on generation mismatch before saved file selection is restored.",
   "If payload cannot be loaded safely, preserve directory selection and leave file authority unchanged or empty according to current AppState.",
   "Payload changes settle in Volume before panel file anchors are rebound.",
   "documented_foundation_only",
   kAppStateGenerationDomainMigrationNotes8,
   sizeof(kAppStateGenerationDomainMigrationNotes8) /
       sizeof(kAppStateGenerationDomainMigrationNotes8[0])},
  {"lifecycle.volume.registry",
   "volume_lifecycle",
   "ctx/session state plus volume/shared topology and payload state",
   "volume.volume_generation",
   kAppStateGenerationDomainIdentityFields9,
   sizeof(kAppStateGenerationDomainIdentityFields9) /
       sizeof(kAppStateGenerationDomainIdentityFields9[0]),
   kAppStateGenerationDomainAdvancesOnTransitionIds9,
   sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds9) /
       sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds9[0]),
   "Panel volume bindings must resolve to an existing volume identity before per-volume snapshots can be restored.",
   "Do not orphan panel bindings; use deterministic volume fallback and then panel identity fallback after release or cycle invalidation.",
   "Volume lifecycle transitions update registry/binding state before panel-local restore snapshots are applied.",
   "documented_foundation_only",
   kAppStateGenerationDomainMigrationNotes9,
   sizeof(kAppStateGenerationDomainMigrationNotes9) /
       sizeof(kAppStateGenerationDomainMigrationNotes9[0])},
  {"reflow.layout.projection",
   "layout_reflow",
   "render/projection/invalidation state",
   "panel.panel_generation",
   kAppStateGenerationDomainIdentityFields10,
   sizeof(kAppStateGenerationDomainIdentityFields10) /
       sizeof(kAppStateGenerationDomainIdentityFields10[0]),
   kAppStateGenerationDomainAdvancesOnTransitionIds10,
   sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds10) /
       sizeof(kAppStateGenerationDomainAdvancesOnTransitionIds10[0]),
   "Layout reflow must project from settled AppState and may advance panel generation only for viewport bounds correction.",
   "If safe projection cannot be computed, degrade or skip render without choosing new authoritative identities.",
   "Resize handling runs in the main loop and commits any viewport correction before render projection.",
   "documented_foundation_only",
   kAppStateGenerationDomainMigrationNotes10,
   sizeof(kAppStateGenerationDomainMigrationNotes10) /
       sizeof(kAppStateGenerationDomainMigrationNotes10[0])},
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
  "transition.volume-operation.release-cycle",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
  "transition.rebuild-rebind-callback.panel-anchor",
};

static const char *const kAppStateDiffHarnessOwnerFieldRefs0[] = {
  "ctx.active",
  "ctx.volumes_head",
  "panel.tree_selection_key",
  "panel.file_selection_key",
  "panel.restore_snapshot",
  "panel.panel_generation",
  "volume.dir_tree",
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
  "transition.command-completion.user-command",
};

static const char *const kAppStateDiffHarnessOwnerFieldRefs1[] = {
  "ctx.command_state",
  "ctx.message_state",
  "ctx.modal_state",
  "ctx.pending_transition",
  "panel.tree_selection_key",
  "panel.tree_cursor_pos",
  "panel.tree_viewport_origin",
  "panel.focus_shape",
  "panel.panel_generation",
};

static const char *const kAppStateDiffHarnessInvariantIds1[] = {
  "invariant.panel-local-focus-restore",
  "invariant.shared-state-panel-local-isolation",
};

static const char *const kAppStateDiffHarnessGenerationDomainIds1[] = {
  "generation.panel.local-authority",
  "shape.panel.focus",
  "target.modal-command.session",
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
  "transition.filesystem-mutation-result.mkdir-copy-delete",
  "transition.rebuild-rebind-callback.panel-anchor",
  "transition.terminal-signal-resize",
};

static const char *const kAppStateDiffHarnessOwnerFieldRefs3[] = {
  "panel.restore_snapshot",
  "panel.tree_selection_key",
  "panel.file_selection_key",
  "panel.panel_generation",
  "volume.dir_tree",
  "volume.payload_cache",
  "volume.volume_generation",
};

static const char *const kAppStateDiffHarnessInvariantIds3[] = {
  "invariant.stale-snapshot-fail-closed",
  "invariant.viewport-identity-rebind",
  "invariant.hidden-entry-visible-navigation",
};

static const char *const kAppStateDiffHarnessGenerationDomainIds3[] = {
  "generation.panel.local-authority",
  "generation.volume.shared-authority",
  "identity.directory.stable-key",
  "identity.file.stable-key",
  "state.topology.volume",
  "state.file-payload.volume",
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
   "documented_foundation_only",
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
   "documented_foundation_only",
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
   "documented_foundation_only",
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
   "documented_foundation_only",
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
   "documented_foundation_only",
   kAppStateDiffHarnessMigrationNotes4,
   sizeof(kAppStateDiffHarnessMigrationNotes4) / sizeof(kAppStateDiffHarnessMigrationNotes4[0])},
};
static const char *const kAppStateDispatchSurfaceMigrationNotes0[] = {
  "Current input polling and key normalization feed controller dispatch; AppState mutation remains in downstream handlers until runtime transition objects are introduced.",
};

static const char *const kAppStateDispatchSurfaceAllowedDirectWrites1[] = {
  "panel.tree_selection_key",
  "panel.tree_cursor_pos",
  "panel.tree_viewport_origin",
  "panel.focus_shape",
  "panel.panel_generation",
};

static const char *const kAppStateDispatchSurfaceMigrationNotes1[] = {
  "Current directory-window switch dispatch owns tree navigation and focus updates; later migration should route each action through canonical transition boundaries.",
};

static const char *const kAppStateDispatchSurfaceAllowedDirectWrites2[] = {
  "panel.file_selection_key",
  "panel.file_viewport_origin",
  "panel.focus_shape",
  "panel.panel_generation",
};

static const char *const kAppStateDispatchSurfaceMigrationNotes2[] = {
  "Current file-window switch dispatch updates file selection and viewport state under the broad keybinding foundation record until file-specific transitions are split out.",
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

static const char *const kAppStateDispatchSurfaceMigrationNotes7[] = {
  "Current loaded-volume selection and cycling update panel bindings directly; migration must preserve inactive panel restore records.",
};

static const char *const kAppStateDispatchSurfaceMigrationNotes8[] = {
  "Current watcher processing reports settled filesystem activity to input dispatch; topology mutation remains mapped to the broad refresh/rebuild transition.",
};

static const char *const kAppStateDispatchSurfaceMigrationNotes9[] = {
  "Current render refresh projects settled state to ncurses windows; projection must not become selection, viewport, or topology authority.",
};

static const AppStateDispatchSurfaceMetadata kAppStateDispatchSurfaces[] = {
  {"surface.key-decode-input-dispatch",
   "key_decode_input_dispatch",
   "src/ui/key_engine.c",
   "GetEventOrKey",
   "transition.keybinding.navigate-tree",
   "documented_foundation_only",
   NULL,
   0,
   kAppStateDispatchSurfaceMigrationNotes0,
   sizeof(kAppStateDispatchSurfaceMigrationNotes0) / sizeof(kAppStateDispatchSurfaceMigrationNotes0[0])},
  {"surface.directory-window-action-dispatch",
   "directory_window_action_dispatch",
   "src/ui/ctrl_dir.c",
   "HandleDirWindow",
   "transition.keybinding.navigate-tree",
   "documented_foundation_only",
   kAppStateDispatchSurfaceAllowedDirectWrites1,
   sizeof(kAppStateDispatchSurfaceAllowedDirectWrites1) /
       sizeof(kAppStateDispatchSurfaceAllowedDirectWrites1[0]),
   kAppStateDispatchSurfaceMigrationNotes1,
   sizeof(kAppStateDispatchSurfaceMigrationNotes1) / sizeof(kAppStateDispatchSurfaceMigrationNotes1[0])},
  {"surface.file-window-action-dispatch",
   "file_window_action_dispatch",
   "src/ui/ctrl_file.c",
   "HandleFileWindow",
   "transition.keybinding.navigate-tree",
   "documented_foundation_only",
   kAppStateDispatchSurfaceAllowedDirectWrites2,
   sizeof(kAppStateDispatchSurfaceAllowedDirectWrites2) /
       sizeof(kAppStateDispatchSurfaceAllowedDirectWrites2[0]),
   kAppStateDispatchSurfaceMigrationNotes2,
   sizeof(kAppStateDispatchSurfaceMigrationNotes2) / sizeof(kAppStateDispatchSurfaceMigrationNotes2[0])},
  {"surface.menu-modal-completion",
   "menu_modal_completion",
   "src/ui/key_engine.c",
   "InputChoice",
   "transition.modal-action.dismiss",
   "documented_foundation_only",
   NULL,
   0,
   kAppStateDispatchSurfaceMigrationNotes3,
   sizeof(kAppStateDispatchSurfaceMigrationNotes3) / sizeof(kAppStateDispatchSurfaceMigrationNotes3[0])},
  {"surface.resize-signal-handling",
   "resize_signal_handling",
   "src/ui/key_engine.c",
   "GetEventOrKey",
   "transition.terminal-signal-resize",
   "documented_foundation_only",
   kAppStateDispatchSurfaceAllowedDirectWrites4,
   sizeof(kAppStateDispatchSurfaceAllowedDirectWrites4) /
       sizeof(kAppStateDispatchSurfaceAllowedDirectWrites4[0]),
   kAppStateDispatchSurfaceMigrationNotes4,
   sizeof(kAppStateDispatchSurfaceMigrationNotes4) / sizeof(kAppStateDispatchSurfaceMigrationNotes4[0])},
  {"surface.refresh-rebuild-rebind",
   "refresh_rebuild_rebind",
   "src/ui/dir_ops.c",
   "RefreshTreeSafe",
   "transition.refresh-rebuild.manual-refresh",
   "documented_foundation_only",
   kAppStateDispatchSurfaceAllowedDirectWrites5,
   sizeof(kAppStateDispatchSurfaceAllowedDirectWrites5) /
       sizeof(kAppStateDispatchSurfaceAllowedDirectWrites5[0]),
   kAppStateDispatchSurfaceMigrationNotes5,
   sizeof(kAppStateDispatchSurfaceMigrationNotes5) / sizeof(kAppStateDispatchSurfaceMigrationNotes5[0])},
  {"surface.filesystem-mutation-result",
   "filesystem_mutation_result",
   "src/ui/dir_ops.c",
   "HandleDirMakeDirectory",
   "transition.filesystem-mutation-result.mkdir-copy-delete",
   "documented_foundation_only",
   kAppStateDispatchSurfaceAllowedDirectWrites6,
   sizeof(kAppStateDispatchSurfaceAllowedDirectWrites6) /
       sizeof(kAppStateDispatchSurfaceAllowedDirectWrites6[0]),
   kAppStateDispatchSurfaceMigrationNotes6,
   sizeof(kAppStateDispatchSurfaceMigrationNotes6) / sizeof(kAppStateDispatchSurfaceMigrationNotes6[0])},
  {"surface.volume-operation",
   "volume_operation",
   "src/ui/volume_menu.c",
   "SelectLoadedVolume",
   "transition.volume-operation.release-cycle",
   "documented_foundation_only",
   kAppStateDispatchSurfaceAllowedDirectWrites7,
   sizeof(kAppStateDispatchSurfaceAllowedDirectWrites7) /
       sizeof(kAppStateDispatchSurfaceAllowedDirectWrites7[0]),
   kAppStateDispatchSurfaceMigrationNotes7,
   sizeof(kAppStateDispatchSurfaceMigrationNotes7) / sizeof(kAppStateDispatchSurfaceMigrationNotes7[0])},
  {"surface.watcher-live-refresh",
   "watcher_live_refresh",
   "src/fs/watcher.c",
   "Watcher_ProcessEvents",
   "transition.refresh-rebuild.manual-refresh",
   "documented_foundation_only",
   NULL,
   0,
   kAppStateDispatchSurfaceMigrationNotes8,
   sizeof(kAppStateDispatchSurfaceMigrationNotes8) / sizeof(kAppStateDispatchSurfaceMigrationNotes8[0])},
  {"surface.render-reflow-projection",
   "render_reflow_projection",
   "src/ui/display.c",
   "RefreshView",
   "transition.render-reflow.project-state",
   "documented_foundation_only",
   NULL,
   0,
   kAppStateDispatchSurfaceMigrationNotes9,
   sizeof(kAppStateDispatchSurfaceMigrationNotes9) / sizeof(kAppStateDispatchSurfaceMigrationNotes9[0])},
};
static const char *const kAppStateCompatibilityShimInvariantChecks0[] = {
  "YtreeNovaPanel(active).dotfile_visibility is authoritative",
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

static const char *const kAppStateInvariantProtectedFields0[] = {
  "ctx.active",
  "panel.volume_key",
  "panel.tree_selection_key",
  "panel.tree_cursor_pos",
  "panel.tree_viewport_origin",
  "panel.file_selection_key",
  "panel.file_viewport_origin",
  "panel.focus_shape",
  "panel.restore_snapshot",
  "panel.panel_generation",
};

static const char *const kAppStateInvariantTransitionIds0[] = {
  "transition.keybinding.navigate-tree",
  "transition.menu-action.volume-select",
  "transition.modal-action.dismiss",
  "transition.refresh-rebuild.manual-refresh",
  "transition.volume-operation.release-cycle",
  "transition.terminal-signal-resize",
};

static const char *const kAppStateInvariantDispatchSurfaceIds0[] = {
  "surface.directory-window-action-dispatch",
  "surface.file-window-action-dispatch",
  "surface.menu-modal-completion",
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
  "transition.rebuild-rebind-callback.panel-anchor",
};

static const char *const kAppStateInvariantDispatchSurfaceIds2[] = {
  "surface.directory-window-action-dispatch",
  "surface.refresh-rebuild-rebind",
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
  "transition.rebuild-rebind-callback.panel-anchor",
};

static const char *const kAppStateInvariantDispatchSurfaceIds3[] = {
  "surface.directory-window-action-dispatch",
  "surface.file-window-action-dispatch",
  "surface.menu-modal-completion",
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
  "transition.terminal-signal-resize",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
  "transition.rebuild-rebind-callback.panel-anchor",
};

static const char *const kAppStateInvariantDispatchSurfaceIds4[] = {
  "surface.refresh-rebuild-rebind",
  "surface.resize-signal-handling",
  "surface.filesystem-mutation-result",
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
  "transition.volume-operation.release-cycle",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
};

static const char *const kAppStateInvariantDispatchSurfaceIds5[] = {
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
  "transition.volume-operation.release-cycle",
  "transition.filesystem-mutation-result.mkdir-copy-delete",
  "transition.rebuild-rebind-callback.panel-anchor",
};

static const char *const kAppStateInvariantDispatchSurfaceIds6[] = {
  "surface.refresh-rebuild-rebind",
  "surface.volume-operation",
  "surface.filesystem-mutation-result",
  "surface.watcher-live-refresh",
};

static const char *const kAppStateInvariantMigrationNotes6[] = {
  "Generation validation must happen before any restore snapshot is applied to a panel record.",
};

static const char *const kAppStateInvariantProtectedFields7[] = {
  "ctx.command_state",
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
  "transition.refresh-rebuild.manual-refresh",
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
   "documented_foundation_only",
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
   "documented_foundation_only",
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
   "documented_foundation_only",
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
   "documented_foundation_only",
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
   "documented_foundation_only",
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
   "documented_foundation_only",
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
   "documented_foundation_only",
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
   "documented_foundation_only",
   "Negative state-sequence tests force guard failures and unavailable targets, then assert no unrelated owner fields differ and any message/modal output is declared.",
   kAppStateInvariantMigrationNotes7,
   sizeof(kAppStateInvariantMigrationNotes7) /
       sizeof(kAppStateInvariantMigrationNotes7[0])},
};

static const AppStateTransitionMetadata kAppStateTransitions[] = {
  {"transition.keybinding.navigate-tree",
   "keybinding",
   "YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet0,
   sizeof(kAppStateTransitionWriteSet0) / sizeof(kAppStateTransitionWriteSet0[0])},
  {"transition.menu-action.volume-select",
   "menu_action",
   "ViewContext(session routing) and YtreeNovaPanel(active)",
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
   "ViewContext.volume_registry and YtreeNovaPanel(active)",
   kAppStateTransitionWriteSet4,
   sizeof(kAppStateTransitionWriteSet4) / sizeof(kAppStateTransitionWriteSet4[0])},
  {"transition.terminal-signal-resize",
   "terminal_signal_or_resize",
   "ViewContext.layout_region",
   kAppStateTransitionWriteSet5,
   sizeof(kAppStateTransitionWriteSet5) / sizeof(kAppStateTransitionWriteSet5[0])},
  {"transition.filesystem-mutation-result.mkdir-copy-delete",
   "filesystem_mutation_result",
   "Volume(shared topology) plus YtreeNovaPanel(active) for active selection",
   kAppStateTransitionWriteSet6,
   sizeof(kAppStateTransitionWriteSet6) / sizeof(kAppStateTransitionWriteSet6[0])},
  {"transition.command-completion.user-command",
   "command_completion",
   "ViewContext.command_region",
   kAppStateTransitionWriteSet7,
   sizeof(kAppStateTransitionWriteSet7) / sizeof(kAppStateTransitionWriteSet7[0])},
  {"transition.rebuild-rebind-callback.panel-anchor",
   "rebuild_rebind_callback",
   "YtreeNovaPanel(affected) and Volume(current)",
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
   "Allowed only as a derived compatibility mirror for helpers that have not yet accepted YtreeNovaPanel dotfile visibility.",
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

const AppStateOwnerFieldMetadata *AppStateOwnerFieldAt(size_t index) {
  if (index >= AppStateOwnerFieldCount())
    return NULL;

  return &kAppStateOwnerFields[index];
}

const AppStateGenerationDomainMetadata *
AppStateGenerationDomainAt(size_t index) {
  if (index >= AppStateGenerationDomainCount())
    return NULL;

  return &kAppStateGenerationDomains[index];
}

const AppStateDiffHarnessMetadata *AppStateDiffHarnessAt(size_t index) {
  if (index >= AppStateDiffHarnessCount())
    return NULL;

  return &kAppStateDiffHarnesses[index];
}

const AppStateTransitionMetadata *AppStateTransitionAt(size_t index) {
  if (index >= AppStateTransitionCount())
    return NULL;

  return &kAppStateTransitions[index];
}

const AppStateDispatchSurfaceMetadata *AppStateDispatchSurfaceAt(size_t index) {
  if (index >= AppStateDispatchSurfaceCount())
    return NULL;

  return &kAppStateDispatchSurfaces[index];
}

const AppStateCompatibilityShimMetadata *
AppStateCompatibilityShimAt(size_t index) {
  if (index >= AppStateCompatibilityShimCount())
    return NULL;

  return &kAppStateCompatibilityShims[index];
}

const AppStateInvariantMetadata *AppStateInvariantAt(size_t index) {
  if (index >= AppStateInvariantCount())
    return NULL;

  return &kAppStateInvariants[index];
}

const AppStateOwnerFieldMetadata *
AppStateOwnerFieldLookup(const char *field) {
  size_t index;

  if (field == NULL || field[0] == '\0')
    return NULL;

  for (index = 0; index < AppStateOwnerFieldCount(); index++) {
    if (!strcmp(kAppStateOwnerFields[index].field, field))
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
    if (!strcmp(kAppStateGenerationDomains[index].domain_id, domain_id))
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
    if (!strcmp(kAppStateDiffHarnesses[index].harness_id, harness_id))
      return &kAppStateDiffHarnesses[index];
  }

  return NULL;
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

const AppStateDispatchSurfaceMetadata *
AppStateDispatchSurfaceLookup(const char *surface_id) {
  size_t index;

  if (surface_id == NULL || surface_id[0] == '\0')
    return NULL;

  for (index = 0; index < AppStateDispatchSurfaceCount(); index++) {
    if (!strcmp(kAppStateDispatchSurfaces[index].surface_id, surface_id))
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
    if (!strcmp(kAppStateCompatibilityShims[index].id, shim_id))
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
    if (!strcmp(kAppStateInvariants[index].invariant_id, invariant_id))
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
