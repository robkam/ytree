/***************************************************************************
 *
 * src/core/main.c
 * Main module
 *
 ***************************************************************************/

#include "ytnova_defs.h"
#include "ytnova_appstate_actions.h"
#include "default_profile_template.h"
#include "default_commands_catalog.h"
#include "default_theme_catalog.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

volatile sig_atomic_t ytnova_shutdown_flag = 0;

static int CoreMainOpsReady(const CoreMainOps *ops) {
  return ops != NULL && ops->init != NULL && ops->set_profile_value != NULL &&
         ops->log_disk != NULL && ops->set_filter != NULL &&
         ops->recalculate_sys_stats != NULL && ops->handle_dir_window != NULL &&
         ops->suspend_clock != NULL && ops->shutdown_curses != NULL &&
         ops->volume_free_all != NULL;
}

static int NonEmptyString(const char *value) {
  return value != NULL && value[0] != '\0';
}

static int NonEmptyStringList(const char *const *values, size_t count) {
  size_t index;

  if (values == NULL || count == 0)
    return 0;

  for (index = 0; index < count; index++) {
    if (!NonEmptyString(values[index]))
      return 0;
  }

  return 1;
}

static const struct {
  const char *action_id;
  YtreeNovaAction action;
} kAppStateActionIds[] = {
  {"ACTION_NONE", ACTION_NONE},
  {"ACTION_MOVE_UP", ACTION_MOVE_UP},
  {"ACTION_MOVE_DOWN", ACTION_MOVE_DOWN},
  {"ACTION_MOVE_SIBLING_NEXT", ACTION_MOVE_SIBLING_NEXT},
  {"ACTION_MOVE_SIBLING_PREV", ACTION_MOVE_SIBLING_PREV},
  {"ACTION_MOVE_LEFT", ACTION_MOVE_LEFT},
  {"ACTION_MOVE_RIGHT", ACTION_MOVE_RIGHT},
  {"ACTION_PAGE_UP", ACTION_PAGE_UP},
  {"ACTION_PAGE_DOWN", ACTION_PAGE_DOWN},
  {"ACTION_HOME", ACTION_HOME},
  {"ACTION_END", ACTION_END},
  {"ACTION_TREE_EXPAND", ACTION_TREE_EXPAND},
  {"ACTION_TREE_COLLAPSE", ACTION_TREE_COLLAPSE},
  {"ACTION_TREE_EXPAND_ALL", ACTION_TREE_EXPAND_ALL},
  {"ACTION_ENTER", ACTION_ENTER},
  {"ACTION_ESCAPE", ACTION_ESCAPE},
  {"ACTION_LOG", ACTION_LOG},
  {"ACTION_QUIT", ACTION_QUIT},
  {"ACTION_QUIT_DIR", ACTION_QUIT_DIR},
  {"ACTION_TAG", ACTION_TAG},
  {"ACTION_UNTAG", ACTION_UNTAG},
  {"ACTION_TAG_ALL", ACTION_TAG_ALL},
  {"ACTION_UNTAG_ALL", ACTION_UNTAG_ALL},
  {"ACTION_TAG_REST", ACTION_TAG_REST},
  {"ACTION_UNTAG_REST", ACTION_UNTAG_REST},
  {"ACTION_FILTER", ACTION_FILTER},
  {"ACTION_TOGGLE_MODE", ACTION_TOGGLE_MODE},
  {"ACTION_REFRESH", ACTION_REFRESH},
  {"ACTION_RESIZE", ACTION_RESIZE},
  {"ACTION_VOL_MENU", ACTION_VOL_MENU},
  {"ACTION_VOL_PREV", ACTION_VOL_PREV},
  {"ACTION_VOL_NEXT", ACTION_VOL_NEXT},
  {"ACTION_CMD_A", ACTION_CMD_A},
  {"ACTION_CMD_B", ACTION_CMD_B},
  {"ACTION_CMD_C", ACTION_CMD_C},
  {"ACTION_CMD_D", ACTION_CMD_D},
  {"ACTION_CMD_E", ACTION_CMD_E},
  {"ACTION_CMD_G", ACTION_CMD_G},
  {"ACTION_CMD_H", ACTION_CMD_H},
  {"ACTION_CMD_I", ACTION_CMD_I},
  {"ACTION_CMD_M", ACTION_CMD_M},
  {"ACTION_CMD_O", ACTION_CMD_O},
  {"ACTION_CMD_P", ACTION_CMD_P},
  {"ACTION_CMD_R", ACTION_CMD_R},
  {"ACTION_CMD_S", ACTION_CMD_S},
  {"ACTION_CMD_V", ACTION_CMD_V},
  {"ACTION_CMD_X", ACTION_CMD_X},
  {"ACTION_CMD_Y", ACTION_CMD_Y},
  {"ACTION_CMD_PRINT", ACTION_CMD_PRINT},
  {"ACTION_TOGGLE_HIDDEN", ACTION_TOGGLE_HIDDEN},
  {"ACTION_TOGGLE_COMPACT", ACTION_TOGGLE_COMPACT},
  {"ACTION_CMD_MKFILE", ACTION_CMD_MKFILE},
  {"ACTION_CMD_TAGGED_A", ACTION_CMD_TAGGED_A},
  {"ACTION_CMD_TAGGED_C", ACTION_CMD_TAGGED_C},
  {"ACTION_CMD_TAGGED_D", ACTION_CMD_TAGGED_D},
  {"ACTION_CMD_TAGGED_G", ACTION_CMD_TAGGED_G},
  {"ACTION_CMD_TAGGED_M", ACTION_CMD_TAGGED_M},
  {"ACTION_CMD_TAGGED_O", ACTION_CMD_TAGGED_O},
  {"ACTION_CMD_TAGGED_P", ACTION_CMD_TAGGED_P},
  {"ACTION_CMD_TAGGED_R", ACTION_CMD_TAGGED_R},
  {"ACTION_CMD_TAGGED_S", ACTION_CMD_TAGGED_S},
  {"ACTION_CMD_TAGGED_V", ACTION_CMD_TAGGED_V},
  {"ACTION_CMD_TAGGED_X", ACTION_CMD_TAGGED_X},
  {"ACTION_CMD_TAGGED_Y", ACTION_CMD_TAGGED_Y},
  {"ACTION_CMD_TAGGED_PRINT", ACTION_CMD_TAGGED_PRINT},
  {"ACTION_LIST_JUMP", ACTION_LIST_JUMP},
  {"ACTION_TO_DIR", ACTION_TO_DIR},
  {"ACTION_TOGGLE_TAGGED_MODE", ACTION_TOGGLE_TAGGED_MODE},
  {"ACTION_TOGGLE_STATS", ACTION_TOGGLE_STATS},
  {"ACTION_ASTERISK", ACTION_ASTERISK},
  {"ACTION_INVERT", ACTION_INVERT},
  {"ACTION_SPLIT_SCREEN", ACTION_SPLIT_SCREEN},
  {"ACTION_SWITCH_PANEL", ACTION_SWITCH_PANEL},
  {"ACTION_VIEW_PREVIEW", ACTION_VIEW_PREVIEW},
  {"ACTION_PREVIEW_SCROLL_UP", ACTION_PREVIEW_SCROLL_UP},
  {"ACTION_PREVIEW_SCROLL_DOWN", ACTION_PREVIEW_SCROLL_DOWN},
  {"ACTION_PREVIEW_HOME", ACTION_PREVIEW_HOME},
  {"ACTION_PREVIEW_END", ACTION_PREVIEW_END},
  {"ACTION_PREVIEW_PAGE_UP", ACTION_PREVIEW_PAGE_UP},
  {"ACTION_PREVIEW_PAGE_DOWN", ACTION_PREVIEW_PAGE_DOWN},
  {"ACTION_COMPARE_FILE", ACTION_COMPARE_FILE},
  {"ACTION_COMPARE_DIR", ACTION_COMPARE_DIR},
  {"ACTION_COMPARE_TREE", ACTION_COMPARE_TREE},
  {"ACTION_HELP", ACTION_HELP},
  {"ACTION_EDIT_CONFIG", ACTION_EDIT_CONFIG},
  {"ACTION_FILEINFO_1", ACTION_FILEINFO_1},
  {"ACTION_FILEINFO_2", ACTION_FILEINFO_2},
  {"ACTION_FILEINFO_3", ACTION_FILEINFO_3},
  {"ACTION_FILEINFO_4", ACTION_FILEINFO_4},
  {"ACTION_FILEINFO_5", ACTION_FILEINFO_5},
  {"ACTION_FILEINFO_6", ACTION_FILEINFO_6},
  {"ACTION_FILEINFO_7", ACTION_FILEINFO_7},
  {"ACTION_FILEINFO_8", ACTION_FILEINFO_8},
  {"ACTION_FILEINFO_9", ACTION_FILEINFO_9},
  {"ACTION_FILEINFO_0", ACTION_FILEINFO_0},
  {"ACTION_USER_CMD", ACTION_USER_CMD},
};

static const char *const kAppStateRequiredEventClasses[] = {
  "terminal_resize_signal",
  "refresh_rebuild",
  "rebuild_rebind_callback",
  "filesystem_mutation_result",
  "watcher_live_refresh",
  "command_completion",
  "modal_completion",
  "volume_lifecycle",
  "render_reflow",
};

static const char *const kAppStateRequiredEventIds[] = {
  "event.terminal-resize-signal",
  "event.refresh-rebuild",
  "event.rebuild-rebind-callback",
  "event.filesystem-mutation-result",
  "event.watcher-live-refresh",
  "event.command-completion",
  "event.modal-completion",
  "event.volume-lifecycle",
  "event.render-reflow",
};

static const char *const kAppStateRequiredTransitionIds[] = {
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

static const char *const kAppStateRequiredOwnerFieldIds[] = {
  "ctx.active",
  "ctx.command_state",
  "ctx.refresh_mode",
  "ctx.view_mode",
  "ctx.dir_mode",
  "ctx.message_state",
  "ctx.modal_state",
  "ctx.pending_transition",
  "ctx.volumes_head",
  "ctx.layout",
  "ctx.render_dirty_flags",
  "ctx.window_handles",
  "panel.file_selection_key",
  "panel.file_display_state",
  "panel.file_viewport_origin",
  "panel.focus_shape",
  "panel.panel_generation",
  "panel.restore_snapshot",
  "panel.tree_cursor_pos",
  "panel.tree_selection_key",
  "panel.tree_viewport_origin",
  "panel.volume_key",
  "volume.dir_tree",
  "volume.logged_state",
  "volume.payload_cache",
  "volume.volume_generation",
};

static const char *const kAppStateRequiredGenerationDomainIds[] = {
  "generation.panel.local-authority",
  "generation.volume.shared-authority",
  "identity.directory.stable-key",
  "identity.file.stable-key",
  "shape.panel.focus",
  "target.modal-command.session",
  "state.visibility-filter.panel-volume",
  "state.topology.volume",
  "state.file-payload.volume",
  "lifecycle.volume.registry",
  "reflow.layout.projection",
};

static const char *const kAppStateRequiredDispatchSurfaceIds[] = {
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
  "surface.command-completion-dispatch",
  "surface.volume-menu-selection",
  "surface.panel-anchor-rebind",
};

static const char *const kAppStateRequiredInvariantIds[] = {
  "invariant.inactive-panel-frozen",
  "invariant.render-projection-read-only",
  "invariant.hidden-entry-visible-navigation",
  "invariant.panel-local-focus-restore",
  "invariant.viewport-identity-rebind",
  "invariant.shared-state-panel-local-isolation",
  "invariant.stale-snapshot-fail-closed",
  "invariant.blocked-transition-determinism",
};

static const char *const kAppStateRequiredDiffHarnessIds[] = {
  "harness.transition-before-after-snapshot",
  "harness.declared-write-set-diff",
  "harness.render-projection-read-only-diff",
  "harness.generation-mismatch-check",
  "harness.blocked-transition-no-unrelated-mutation",
};

static const char *const kAppStateRequiredTransitionSequenceScenarioIds[] = {
  "sequence.split-toggle-f8",
  "sequence.tab-panel-switch",
  "sequence.enter-directory-file-transition",
  "sequence.esc-modal-dismissal",
  "sequence.modal-completion",
  "sequence.dotfile-reveal-conceal",
  "sequence.refresh-rebuild",
  "sequence.watcher-live-refresh",
  "sequence.filesystem-mutation-result",
  "sequence.search-jump",
  "sequence.showall-global-tagged-only",
  "sequence.file-small-big-transitions",
  "sequence.volume-cycling-release",
  "sequence.split-close-reopen",
  "sequence.terminal-resize-reflow",
  "sequence.render-reflow-projection",
  "sequence.volume-menu-select",
};

static int StringListContains(const char *const *values, size_t count,
                              const char *value) {
  size_t index;

  if (!NonEmptyString(value))
    return 0;

  for (index = 0; index < count; index++) {
    if (NonEmptyString(values[index]) && strcmp(values[index], value) == 0)
      return 1;
  }

  return 0;
}

static int StringListsOverlap(const char *const *left, size_t left_count,
                              const char *const *right, size_t right_count) {
  size_t index;

  if (!NonEmptyStringList(left, left_count) ||
      !NonEmptyStringList(right, right_count))
    return 0;

  for (index = 0; index < left_count; index++) {
    if (StringListContains(right, right_count, left[index]))
      return 1;
  }

  return 0;
}

static int AppStateExpectedResultValid(const char *expected_result) {
  return strcmp(expected_result, "allowed") == 0 ||
         strcmp(expected_result, "blocked") == 0 ||
         strcmp(expected_result, "fallback") == 0 ||
         strcmp(expected_result, "invalid") == 0;
}

static const AppStateActionTransitionMetadata *
AppStateActionIdLookup(const char *action_id) {
  size_t index;

  if (!NonEmptyString(action_id))
    return NULL;

  for (index = 0; index < sizeof(kAppStateActionIds) / sizeof(kAppStateActionIds[0]);
       index++) {
    if (strcmp(kAppStateActionIds[index].action_id, action_id) == 0)
      return AppStateActionTransitionLookup(kAppStateActionIds[index].action);
  }

  return NULL;
}

static const char *AppStateEventTransitionLookup(const char *event_id) {
  const AppStateEventCoverageMetadata *coverage =
      AppStateEventCoverageLookup(event_id);

  if (coverage == NULL)
    return NULL;

  return coverage->transition_id;
}

static const AppStateActionCoverageMetadata *
AppStateActionCoverageIdLookup(const char *action_id) {
  size_t index;

  if (!NonEmptyString(action_id))
    return NULL;

  for (index = 0; index < sizeof(kAppStateActionIds) / sizeof(kAppStateActionIds[0]);
       index++) {
    if (strcmp(kAppStateActionIds[index].action_id, action_id) == 0)
      return AppStateActionCoverageLookup(kAppStateActionIds[index].action);
  }

  return NULL;
}

static int
AppStateInvariantRefsReady(const char *const *refs, size_t ref_count,
                           const char *transition_id,
                           const char *const *declared_write_set,
                           size_t declared_write_set_count);
static int
AppStateGenerationDomainRefsReady(const char *const *refs, size_t ref_count,
                                  const char *transition_id);

static int AppStateFallbackPreconditionValid(const char *precondition) {
  if (precondition == NULL)
    return 1;
  if (!NonEmptyString(precondition))
    return 0;
  return strcmp(precondition, "generation_mismatch") == 0 ||
         strcmp(precondition, "stale_snapshot") == 0;
}

static int AppStateTransitionSequenceStepDiffHarnessCoversTransition(
    const AppStateTransitionSequenceStepMetadata *step) {
  size_t ref_index;

  if (step == NULL || !NonEmptyString(step->transition_id) ||
      !NonEmptyStringList(step->diff_harness_ids, step->diff_harness_id_count))
    return 0;

  for (ref_index = 0; ref_index < step->diff_harness_id_count; ref_index++) {
    const AppStateDiffHarnessMetadata *harness =
        AppStateDiffHarnessLookup(step->diff_harness_ids[ref_index]);

    if (harness == NULL || harness->transition_ids == NULL)
      return 0;
    if (StringListContains(harness->transition_ids, harness->transition_id_count,
                           step->transition_id))
      return 1;
  }

  return 0;
}

static int AppStateTransitionSequenceStepInvariantCoversTransition(
    const AppStateTransitionSequenceStepMetadata *step) {
  size_t ref_index;

  if (step == NULL || !NonEmptyString(step->transition_id) ||
      !NonEmptyStringList(step->invariant_ids, step->invariant_id_count))
    return 0;

  for (ref_index = 0; ref_index < step->invariant_id_count; ref_index++) {
    const AppStateInvariantMetadata *invariant =
        AppStateInvariantLookup(step->invariant_ids[ref_index]);

    if (invariant == NULL || invariant->transition_ids == NULL)
      return 0;
    if (StringListContains(invariant->transition_ids,
                           invariant->transition_id_count,
                           step->transition_id))
      return 1;
  }

  return 0;
}

static int AppStateTransitionSequenceStepGenerationDomainOverlaps(
    const AppStateTransitionSequenceStepMetadata *step,
    const char *const *coverage_domain_refs, size_t coverage_domain_ref_count) {
  size_t ref_index;

  if (step == NULL || step->generation_domain_expectations == NULL ||
      step->generation_domain_expectation_count == 0 ||
      !NonEmptyStringList(coverage_domain_refs, coverage_domain_ref_count))
    return 0;

  for (ref_index = 0; ref_index < step->generation_domain_expectation_count;
       ref_index++) {
    const AppStateTransitionSequenceGenerationExpectationMetadata *expectation =
        &step->generation_domain_expectations[ref_index];

    if (!NonEmptyString(expectation->domain_id))
      return 0;
    if (StringListContains(coverage_domain_refs, coverage_domain_ref_count,
                           expectation->domain_id))
      return 1;
  }

  return 0;
}

static int AppStateTransitionSequenceStepCoverageOverlaps(
    const AppStateTransitionSequenceStepMetadata *step,
    const char *const *coverage_invariant_refs, size_t coverage_invariant_ref_count,
    const char *const *coverage_diff_harness_refs,
    size_t coverage_diff_harness_ref_count,
    const char *const *coverage_generation_domain_refs,
    size_t coverage_generation_domain_ref_count) {
  if (step == NULL)
    return 0;
  if (!StringListsOverlap(step->invariant_ids, step->invariant_id_count,
                          coverage_invariant_refs,
                          coverage_invariant_ref_count))
    return 0;
  if (!StringListsOverlap(step->diff_harness_ids, step->diff_harness_id_count,
                          coverage_diff_harness_refs,
                          coverage_diff_harness_ref_count))
    return 0;
  if (!AppStateTransitionSequenceStepGenerationDomainOverlaps(
          step, coverage_generation_domain_refs,
          coverage_generation_domain_ref_count))
    return 0;

  return 1;
}

static int AppStateTransitionSequenceStepRequiresNoUnrelatedMutation(
    const AppStateTransitionSequenceStepMetadata *step) {
  if (step == NULL || !NonEmptyString(step->expected_result))
    return 0;
  return strcmp(step->expected_result, "blocked") == 0 ||
         strcmp(step->expected_result, "fallback") == 0 ||
         strcmp(step->expected_result, "invalid") == 0 ||
         step->precondition != NULL;
}

static int AppStateTransitionSequenceStepNoUnrelatedMutationReady(
    const AppStateTransitionSequenceStepMetadata *step) {
  const AppStateDiffHarnessMetadata *harness;

  if (step == NULL)
    return 1;
  if (!AppStateTransitionSequenceStepRequiresNoUnrelatedMutation(step) &&
      step->no_unrelated_mutation == NULL)
    return 1;
  if (step->no_unrelated_mutation == NULL)
    return 0;
  if (!NonEmptyString(step->no_unrelated_mutation->diff_harness_id) ||
      !NonEmptyString(step->no_unrelated_mutation->expectation))
    return 0;
  if (!StringListContains(step->diff_harness_ids, step->diff_harness_id_count,
                          step->no_unrelated_mutation->diff_harness_id))
    return 0;

  harness = AppStateDiffHarnessLookup(step->no_unrelated_mutation->diff_harness_id);
  if (harness == NULL || harness->transition_ids == NULL ||
      !StringListContains(harness->transition_ids, harness->transition_id_count,
                          step->transition_id))
    return 0;

  return 1;
}

static int AppStateTransitionSequenceStepReady(
    const AppStateTransitionSequenceMetadata *sequence,
    const AppStateTransitionSequenceStepMetadata *step, size_t step_index,
    size_t previous_ordinal) {
  size_t ref_index;

  if (step == NULL || step->ordinal == 0 || step->ordinal <= previous_ordinal ||
      !NonEmptyString(step->step_id) || !NonEmptyString(step->transition_id) ||
      !NonEmptyString(step->expected_result) ||
      !AppStateExpectedResultValid(step->expected_result) ||
      AppStateTransitionLookup(step->transition_id) == NULL ||
      !NonEmptyStringList(step->invariant_ids, step->invariant_id_count) ||
      !NonEmptyStringList(step->diff_harness_ids, step->diff_harness_id_count) ||
      step->generation_domain_expectations == NULL ||
      step->generation_domain_expectation_count == 0 ||
      !AppStateFallbackPreconditionValid(step->precondition))
    return 0;

  for (ref_index = 0; ref_index < step_index; ref_index++) {
    if (strcmp(sequence->steps[ref_index].step_id, step->step_id) == 0)
      return 0;
  }

  if (step->stimulus_action_id == NULL && step->stimulus_event_id == NULL)
    return 0;
  if (step->stimulus_action_id != NULL) {
    const AppStateActionTransitionMetadata *action_metadata =
        AppStateActionIdLookup(step->stimulus_action_id);

    if (action_metadata == NULL ||
        strcmp(action_metadata->transition_id, step->transition_id) != 0)
      return 0;
    if (!NonEmptyStringList(step->action_coverage_refs,
                            step->action_coverage_ref_count))
      return 0;
  } else if (step->action_coverage_refs != NULL ||
             step->action_coverage_ref_count != 0) {
    return 0;
  }
  if (step->stimulus_event_id != NULL) {
    const char *event_transition =
        AppStateEventTransitionLookup(step->stimulus_event_id);

    if (event_transition == NULL || strcmp(event_transition, step->transition_id) != 0)
      return 0;
    if (!NonEmptyStringList(step->event_coverage_refs,
                            step->event_coverage_ref_count))
      return 0;
  } else if (step->event_coverage_refs != NULL ||
             step->event_coverage_ref_count != 0) {
    return 0;
  }

  for (ref_index = 0; ref_index < step->action_coverage_ref_count; ref_index++) {
    const AppStateActionCoverageMetadata *coverage =
        AppStateActionCoverageIdLookup(step->action_coverage_refs[ref_index]);

    if (coverage == NULL ||
        strcmp(step->action_coverage_refs[ref_index],
               step->stimulus_action_id) != 0 ||
        strcmp(coverage->transition_id, step->transition_id) != 0)
      return 0;
    if (!AppStateTransitionSequenceStepCoverageOverlaps(
            step, coverage->invariant_refs, coverage->invariant_ref_count,
            coverage->diff_harness_refs, coverage->diff_harness_ref_count,
            coverage->generation_domain_refs,
            coverage->generation_domain_ref_count))
      return 0;
    if (StringListContains(step->action_coverage_refs, ref_index,
                           step->action_coverage_refs[ref_index]))
      return 0;
  }
  for (ref_index = 0; ref_index < step->event_coverage_ref_count; ref_index++) {
    const AppStateEventCoverageMetadata *coverage =
        AppStateEventCoverageLookup(step->event_coverage_refs[ref_index]);

    if (coverage == NULL ||
        strcmp(step->event_coverage_refs[ref_index], step->stimulus_event_id) !=
            0 ||
        strcmp(coverage->transition_id, step->transition_id) != 0)
      return 0;
    if (!AppStateTransitionSequenceStepCoverageOverlaps(
            step, coverage->invariant_refs, coverage->invariant_ref_count,
            coverage->diff_harness_refs, coverage->diff_harness_ref_count,
            coverage->generation_domain_refs,
            coverage->generation_domain_ref_count))
      return 0;
    if (StringListContains(step->event_coverage_refs, ref_index,
                           step->event_coverage_refs[ref_index]))
      return 0;
  }

  for (ref_index = 0; ref_index < step->invariant_id_count; ref_index++) {
    if (AppStateInvariantLookup(step->invariant_ids[ref_index]) == NULL)
      return 0;
    if (StringListContains(step->invariant_ids, ref_index,
                           step->invariant_ids[ref_index]))
      return 0;
  }
  if (!AppStateTransitionSequenceStepInvariantCoversTransition(step))
    return 0;
  for (ref_index = 0; ref_index < step->diff_harness_id_count; ref_index++) {
    if (AppStateDiffHarnessLookup(step->diff_harness_ids[ref_index]) == NULL)
      return 0;
    if (StringListContains(step->diff_harness_ids, ref_index,
                           step->diff_harness_ids[ref_index]))
      return 0;
  }
  if (!AppStateTransitionSequenceStepDiffHarnessCoversTransition(step))
    return 0;

  for (ref_index = 0; ref_index < step->generation_domain_expectation_count;
       ref_index++) {
    const AppStateTransitionSequenceGenerationExpectationMetadata *expectation =
        &step->generation_domain_expectations[ref_index];
    size_t previous_index;

    if (!NonEmptyString(expectation->domain_id) ||
        !NonEmptyString(expectation->expectation) ||
        AppStateGenerationDomainLookup(expectation->domain_id) == NULL)
      return 0;
    for (previous_index = 0; previous_index < ref_index; previous_index++) {
      if (strcmp(step->generation_domain_expectations[previous_index].domain_id,
                 expectation->domain_id) == 0)
        return 0;
    }
  }

  if (!AppStateTransitionSequenceStepNoUnrelatedMutationReady(step))
    return 0;
  if (step->no_unrelated_mutation != NULL) {
    if (!NonEmptyString(step->no_unrelated_mutation->diff_harness_id) ||
        !NonEmptyString(step->no_unrelated_mutation->expectation) ||
        AppStateDiffHarnessLookup(step->no_unrelated_mutation->diff_harness_id) ==
            NULL)
      return 0;
  }

  if (step->precondition != NULL || strcmp(step->expected_result, "fallback") == 0) {
    if (step->deterministic_fallback == NULL)
      return 0;
  }
  if (step->deterministic_fallback != NULL) {
    if (!NonEmptyString(step->deterministic_fallback->outcome) ||
        !NonEmptyString(step->deterministic_fallback->allowed_mutation_scope))
      return 0;
  }

  return 1;
}

static int AppStateTransitionSequencesReady(void) {
  size_t index;
  size_t required_sequence_id_count =
      sizeof(kAppStateRequiredTransitionSequenceScenarioIds) /
      sizeof(kAppStateRequiredTransitionSequenceScenarioIds[0]);

  if (AppStateTransitionSequenceCount() != required_sequence_id_count)
    return 0;

  for (index = 0; index < AppStateTransitionSequenceCount(); index++) {
    const AppStateTransitionSequenceMetadata *sequence =
        AppStateTransitionSequenceAt(index);
    size_t previous_index;
    size_t step_index;
    size_t previous_ordinal = 0;

    if (sequence == NULL || !NonEmptyString(sequence->scenario_id) ||
        !NonEmptyString(sequence->category) || !NonEmptyString(sequence->flow) ||
        !NonEmptyString(sequence->description) || sequence->steps == NULL ||
        sequence->step_count == 0)
      return 0;
    if (AppStateTransitionSequenceLookup(sequence->scenario_id) != sequence)
      return 0;
    for (previous_index = 0; previous_index < index; previous_index++) {
      const AppStateTransitionSequenceMetadata *previous =
          AppStateTransitionSequenceAt(previous_index);

      if (previous == NULL ||
          strcmp(previous->scenario_id, sequence->scenario_id) == 0)
        return 0;
    }

    for (step_index = 0; step_index < sequence->step_count; step_index++) {
      const AppStateTransitionSequenceStepMetadata *step =
          &sequence->steps[step_index];

      if (!AppStateTransitionSequenceStepReady(sequence, step, step_index,
                                               previous_ordinal))
        return 0;
      previous_ordinal = step->ordinal;
    }
  }

  for (index = 0; index < required_sequence_id_count; index++) {
    if (AppStateTransitionSequenceLookup(
            kAppStateRequiredTransitionSequenceScenarioIds[index]) == NULL)
      return 0;
  }

  if (AppStateTransitionSequenceAt(AppStateTransitionSequenceCount()) != NULL)
    return 0;
  if (AppStateTransitionSequenceLookup(NULL) != NULL)
    return 0;
  if (AppStateTransitionSequenceLookup("") != NULL)
    return 0;
  if (AppStateTransitionSequenceLookup("sequence.__ytnova_unknown__") != NULL)
    return 0;

  return 1;
}

static int AppStateInvariantDispatchSurfacesReady(
    const AppStateInvariantMetadata *metadata) {
  size_t note_index;

  if (metadata->dispatch_surface_ids == NULL)
    return 0;
  if (metadata->dispatch_surface_id_count > 0)
    return NonEmptyStringList(metadata->dispatch_surface_ids,
                              metadata->dispatch_surface_id_count);

  for (note_index = 0; note_index < metadata->migration_note_count;
       note_index++) {
    if (strstr(metadata->migration_notes[note_index], "cross-cutting") != NULL)
      return 1;
  }

  return 0;
}

static int AppStateInvariantProtectsField(const char *invariant_id,
                                          const char *field) {
  const AppStateInvariantMetadata *metadata;

  if (!NonEmptyString(invariant_id) || !NonEmptyString(field))
    return 0;

  metadata = AppStateInvariantLookup(invariant_id);
  if (metadata == NULL)
    return 0;
  if (!NonEmptyStringList(metadata->protected_fields,
                          metadata->protected_field_count))
    return 0;

  return StringListContains(metadata->protected_fields,
                            metadata->protected_field_count, field);
}

static int AppStateDiffHarnessOwnerFieldCovered(const char *owner_field) {
  size_t harness_index;

  if (!NonEmptyString(owner_field))
    return 0;

  for (harness_index = 0; harness_index < AppStateDiffHarnessCount();
       harness_index++) {
    const AppStateDiffHarnessMetadata *harness =
        AppStateDiffHarnessAt(harness_index);
    size_t ref_index;

    if (harness == NULL || harness->owner_field_refs == NULL)
      return 0;

    for (ref_index = 0; ref_index < harness->owner_field_ref_count;
         ref_index++) {
      const char *harness_owner_field = harness->owner_field_refs[ref_index];

      if (!NonEmptyString(harness_owner_field))
        return 0;
      if (strcmp(harness_owner_field, owner_field) == 0)
        return 1;
    }
  }

  return 0;
}

static int AppStateOwnerFieldsReady(void) {
  size_t index;
  size_t required_owner_field_id_count =
      sizeof(kAppStateRequiredOwnerFieldIds) /
      sizeof(kAppStateRequiredOwnerFieldIds[0]);

  if (AppStateOwnerFieldCount() != required_owner_field_id_count)
    return 0;

  for (index = 0; index < AppStateOwnerFieldCount(); index++) {
    const AppStateOwnerFieldMetadata *metadata = AppStateOwnerFieldAt(index);
    int field_protected = 0;
    size_t invariant_index;
    size_t previous_index;

    if (metadata == NULL || !NonEmptyString(metadata->field) ||
        !NonEmptyString(metadata->owner_region) ||
        !NonEmptyString(metadata->canonical_owner) ||
        !NonEmptyString(metadata->runtime_carrier) ||
        !NonEmptyString(metadata->mutation_rule) ||
        !NonEmptyString(metadata->migration_status) ||
        !NonEmptyStringList(metadata->invariant_checks,
                            metadata->invariant_check_count))
      return 0;
    if (AppStateOwnerFieldLookup(metadata->field) != metadata)
      return 0;

    for (invariant_index = 0;
         invariant_index < metadata->invariant_check_count; invariant_index++) {
      if (AppStateInvariantLookup(metadata->invariant_checks[invariant_index]) ==
          NULL)
        return 0;
      if (AppStateInvariantProtectsField(
              metadata->invariant_checks[invariant_index], metadata->field))
        field_protected = 1;
    }
    if (!field_protected)
      return 0;

    for (previous_index = 0; previous_index < index; previous_index++) {
      const AppStateOwnerFieldMetadata *previous =
          AppStateOwnerFieldAt(previous_index);

      if (previous == NULL || strcmp(previous->field, metadata->field) == 0)
        return 0;
    }
  }

  for (index = 0; index < required_owner_field_id_count; index++) {
    if (AppStateOwnerFieldLookup(kAppStateRequiredOwnerFieldIds[index]) == NULL)
      return 0;
  }

  if (AppStateOwnerFieldAt(AppStateOwnerFieldCount()) != NULL)
    return 0;
  if (AppStateOwnerFieldLookup(NULL) != NULL)
    return 0;
  if (AppStateOwnerFieldLookup("") != NULL)
    return 0;
  if (AppStateOwnerFieldLookup("field.__ytnova_unknown__") != NULL)
    return 0;

  return 1;
}

static int AppStateGenerationWriteCovered(const char *owner_field,
                                          const char *transition_id) {
  size_t domain_index;
  int generation_owner_seen = 0;

  if (!NonEmptyString(owner_field) || !NonEmptyString(transition_id))
    return 0;

  for (domain_index = 0; domain_index < AppStateGenerationDomainCount();
       domain_index++) {
    const AppStateGenerationDomainMetadata *domain =
        AppStateGenerationDomainAt(domain_index);
    size_t transition_index;

    if (domain == NULL || !NonEmptyString(domain->generation_owner_field))
      return 0;
    if (strcmp(domain->generation_owner_field, owner_field) != 0)
      continue;

    generation_owner_seen = 1;
    if (domain->advances_on_transition_ids == NULL)
      return 0;
    for (transition_index = 0;
         transition_index < domain->advances_on_transition_id_count;
         transition_index++) {
      const char *domain_transition_id =
          domain->advances_on_transition_ids[transition_index];

      if (!NonEmptyString(domain_transition_id))
        return 0;
      if (strcmp(domain_transition_id, transition_id) == 0)
        return 1;
    }
  }

  return !generation_owner_seen;
}

static int AppStateTransitionWriteHasInvariantCoverage(
    const char *transition_id, const char *field) {
  size_t invariant_index;

  if (!NonEmptyString(transition_id) || !NonEmptyString(field))
    return 0;

  for (invariant_index = 0; invariant_index < AppStateInvariantCount();
       invariant_index++) {
    const AppStateInvariantMetadata *invariant =
        AppStateInvariantAt(invariant_index);

    if (invariant == NULL)
      continue;
    if (!NonEmptyStringList(invariant->transition_ids,
                            invariant->transition_id_count) ||
        !NonEmptyStringList(invariant->protected_fields,
                            invariant->protected_field_count))
      continue;
    if (StringListContains(invariant->transition_ids,
                           invariant->transition_id_count, transition_id) &&
        StringListContains(invariant->protected_fields,
                           invariant->protected_field_count, field))
      return 1;
  }

  return 0;
}

static int AppStateTransitionRegistryReady(void) {
  size_t index;
  size_t required_transition_id_count =
      sizeof(kAppStateRequiredTransitionIds) /
      sizeof(kAppStateRequiredTransitionIds[0]);

  if (AppStateTransitionCount() != required_transition_id_count)
    return 0;

  for (index = 0; index < AppStateTransitionCount(); index++) {
    const AppStateTransitionMetadata *metadata = AppStateTransitionAt(index);
    size_t previous_index;
    size_t write_index;

    if (metadata == NULL || !NonEmptyString(metadata->id) ||
        !NonEmptyString(metadata->category) ||
        !NonEmptyString(metadata->source_state) ||
        !NonEmptyString(metadata->event) || !NonEmptyString(metadata->guard) ||
        !NonEmptyString(metadata->allowed_result) ||
        !NonEmptyString(metadata->blocked_result) ||
        !NonEmptyString(metadata->target_state) ||
        !NonEmptyString(metadata->owner) ||
        !NonEmptyString(metadata->generation_effect) ||
        !NonEmptyStringList(metadata->side_effects,
                            metadata->side_effect_count) ||
        !NonEmptyString(metadata->render_invalidation) ||
        !NonEmptyString(metadata->boundary_status) ||
        !NonEmptyString(metadata->notes_follow_up) ||
        metadata->declared_write_set == NULL ||
        metadata->declared_write_set_count == 0)
      return 0;
    if (AppStateTransitionLookup(metadata->id) != metadata)
      return 0;

    for (previous_index = 0; previous_index < index; previous_index++) {
      const AppStateTransitionMetadata *previous =
          AppStateTransitionAt(previous_index);

      if (previous == NULL || strcmp(previous->id, metadata->id) == 0)
        return 0;
    }

    for (write_index = 0; write_index < metadata->declared_write_set_count;
         write_index++) {
      const char *field = metadata->declared_write_set[write_index];

      if (!NonEmptyString(field))
        return 0;
      if (AppStateOwnerFieldLookup(field) == NULL)
        return 0;
      if (!AppStateGenerationWriteCovered(field, metadata->id))
        return 0;
      if (!AppStateTransitionWriteHasInvariantCoverage(metadata->id, field))
        return 0;
    }
  }

  for (index = 0; index < required_transition_id_count; index++) {
    if (AppStateTransitionLookup(kAppStateRequiredTransitionIds[index]) == NULL)
      return 0;
  }

  if (AppStateTransitionAt(AppStateTransitionCount()) != NULL)
    return 0;
  if (AppStateTransitionLookup(NULL) != NULL)
    return 0;
  if (AppStateTransitionLookup("") != NULL)
    return 0;
  if (AppStateTransitionLookup("transition.__ytnova_unknown__") != NULL)
    return 0;

  return 1;
}

static int AppStateGenerationCoverageOnlyHasProjectionNote(
    const AppStateGenerationDomainMetadata *metadata) {
  size_t coverage_index;
  int has_projection_note = 0;
  size_t note_index;

  if (metadata == NULL)
    return 0;

  for (note_index = 0; note_index < metadata->migration_note_count;
       note_index++) {
    const char *note = metadata->migration_notes[note_index];

    if (note != NULL &&
        (strstr(note, "read-only") != NULL ||
         strstr(note, "projection-only") != NULL)) {
      has_projection_note = 1;
      break;
    }
  }

  for (coverage_index = 0;
       coverage_index < metadata->coverage_transition_id_count;
       coverage_index++) {
    if (!StringListContains(metadata->advances_on_transition_ids,
                            metadata->advances_on_transition_id_count,
                            metadata->coverage_transition_ids[coverage_index]) &&
        !has_projection_note)
      return 0;
  }

  return 1;
}

static int AppStateGenerationDomainsReady(void) {
  size_t index;
  size_t required_generation_domain_id_count =
      sizeof(kAppStateRequiredGenerationDomainIds) /
      sizeof(kAppStateRequiredGenerationDomainIds[0]);

  if (AppStateGenerationDomainCount() != required_generation_domain_id_count)
    return 0;

  for (index = 0; index < AppStateGenerationDomainCount(); index++) {
    const AppStateGenerationDomainMetadata *metadata =
        AppStateGenerationDomainAt(index);
    size_t field_index;
    size_t previous_index;
    size_t transition_index;

    if (metadata == NULL || !NonEmptyString(metadata->domain_id) ||
        !NonEmptyString(metadata->category) ||
        !NonEmptyString(metadata->owner_region) ||
        !NonEmptyString(metadata->generation_owner_field) ||
        !NonEmptyString(metadata->stale_snapshot_policy) ||
        !NonEmptyString(metadata->fail_closed_fallback) ||
        !NonEmptyString(metadata->restore_boundary) ||
        !NonEmptyString(metadata->enforcement_status) ||
        !NonEmptyStringList(metadata->identity_fields,
                            metadata->identity_field_count) ||
        !NonEmptyStringList(metadata->coverage_transition_ids,
                            metadata->coverage_transition_id_count) ||
        (metadata->advances_on_transition_id_count > 0 &&
         !NonEmptyStringList(metadata->advances_on_transition_ids,
                             metadata->advances_on_transition_id_count)) ||
        !NonEmptyStringList(metadata->migration_notes,
                            metadata->migration_note_count))
      return 0;
    if (AppStateGenerationDomainLookup(metadata->domain_id) != metadata)
      return 0;
    if (AppStateOwnerFieldLookup(metadata->generation_owner_field) == NULL)
      return 0;

    for (previous_index = 0; previous_index < index; previous_index++) {
      const AppStateGenerationDomainMetadata *previous =
          AppStateGenerationDomainAt(previous_index);

      if (previous == NULL ||
          strcmp(previous->domain_id, metadata->domain_id) == 0)
        return 0;
    }

    for (field_index = 0; field_index < metadata->identity_field_count;
         field_index++) {
      if (AppStateOwnerFieldLookup(metadata->identity_fields[field_index]) ==
          NULL)
        return 0;
    }
    for (transition_index = 0;
         transition_index < metadata->coverage_transition_id_count;
         transition_index++) {
      if (AppStateTransitionLookup(
              metadata->coverage_transition_ids[transition_index]) == NULL)
        return 0;
    }
    for (transition_index = 0;
         transition_index < metadata->advances_on_transition_id_count;
         transition_index++) {
      if (AppStateTransitionLookup(
              metadata->advances_on_transition_ids[transition_index]) == NULL)
        return 0;
      if (!StringListContains(metadata->coverage_transition_ids,
                              metadata->coverage_transition_id_count,
                              metadata->advances_on_transition_ids
                                  [transition_index]))
        return 0;
    }
    if (!AppStateGenerationCoverageOnlyHasProjectionNote(metadata))
      return 0;
  }

  for (index = 0; index < required_generation_domain_id_count; index++) {
    if (AppStateGenerationDomainLookup(
            kAppStateRequiredGenerationDomainIds[index]) == NULL)
      return 0;
  }

  if (AppStateGenerationDomainAt(AppStateGenerationDomainCount()) != NULL)
    return 0;
  if (AppStateGenerationDomainLookup(NULL) != NULL)
    return 0;
  if (AppStateGenerationDomainLookup("") != NULL)
    return 0;
  if (AppStateGenerationDomainLookup("generation.__ytnova_unknown__") != NULL)
    return 0;

  return 1;
}

static int AppStateDispatchSurfaceWriteHasInvariantCoverage(
    const char *surface_id, const char *field) {
  size_t invariant_index;

  if (!NonEmptyString(surface_id) || !NonEmptyString(field))
    return 0;

  for (invariant_index = 0; invariant_index < AppStateInvariantCount();
       invariant_index++) {
    const AppStateInvariantMetadata *invariant =
        AppStateInvariantAt(invariant_index);

    if (invariant == NULL)
      continue;
    if (!NonEmptyStringList(invariant->dispatch_surface_ids,
                            invariant->dispatch_surface_id_count) ||
        !NonEmptyStringList(invariant->protected_fields,
                            invariant->protected_field_count))
      continue;
    if (StringListContains(invariant->dispatch_surface_ids,
                           invariant->dispatch_surface_id_count, surface_id) &&
        StringListContains(invariant->protected_fields,
                           invariant->protected_field_count, field))
      return 1;
  }

  return 0;
}

static int AppStateTransitionSequenceStepDiffHarnessCoversField(
    const AppStateTransitionSequenceStepMetadata *step, const char *field) {
  size_t harness_index;

  if (step == NULL || !NonEmptyString(field) ||
      !NonEmptyStringList(step->diff_harness_ids, step->diff_harness_id_count))
    return 0;

  for (harness_index = 0; harness_index < step->diff_harness_id_count;
       harness_index++) {
    const AppStateDiffHarnessMetadata *harness =
        AppStateDiffHarnessLookup(step->diff_harness_ids[harness_index]);

    if (harness == NULL ||
        !NonEmptyStringList(harness->owner_field_refs,
                            harness->owner_field_ref_count))
      return 0;
    if (StringListContains(harness->owner_field_refs,
                           harness->owner_field_ref_count, field))
      return 1;
  }

  return 0;
}

static int AppStateTransitionSequenceStepInvariantCoversField(
    const AppStateTransitionSequenceStepMetadata *step, const char *field) {
  size_t invariant_index;

  if (step == NULL || !NonEmptyString(field) ||
      !NonEmptyStringList(step->invariant_ids, step->invariant_id_count))
    return 0;

  for (invariant_index = 0; invariant_index < step->invariant_id_count;
       invariant_index++) {
    const AppStateInvariantMetadata *invariant =
        AppStateInvariantLookup(step->invariant_ids[invariant_index]);

    if (invariant == NULL ||
        !NonEmptyStringList(invariant->protected_fields,
                            invariant->protected_field_count))
      return 0;
    if (StringListContains(invariant->protected_fields,
                           invariant->protected_field_count, field))
      return 1;
  }

  return 0;
}

static int AppStateTransitionSequenceRefsReady(const char *const *refs,
                                               size_t ref_count,
                                               const char *transition_id) {
  size_t ref_index;

  if (!NonEmptyString(transition_id) || !NonEmptyStringList(refs, ref_count))
    return 0;

  for (ref_index = 0; ref_index < ref_count; ref_index++) {
    const AppStateTransitionSequenceMetadata *sequence =
        AppStateTransitionSequenceLookup(refs[ref_index]);
    int transition_step_found = 0;
    size_t previous_index;
    size_t step_index;

    if (sequence == NULL)
      return 0;
    for (previous_index = 0; previous_index < ref_index; previous_index++) {
      if (strcmp(refs[previous_index], refs[ref_index]) == 0)
        return 0;
    }
    for (step_index = 0; step_index < sequence->step_count; step_index++) {
      const AppStateTransitionSequenceStepMetadata *step =
          &sequence->steps[step_index];

      if (!NonEmptyString(step->transition_id))
        return 0;
      if (strcmp(step->transition_id, transition_id) == 0)
        transition_step_found = 1;
    }
    if (!transition_step_found)
      return 0;
  }

  return 1;
}

static int AppStateDispatchSurfaceRefsReady(const char *const *refs,
                                            size_t ref_count,
                                            const char *transition_id) {
  size_t ref_index;

  if (!NonEmptyString(transition_id) || !NonEmptyStringList(refs, ref_count))
    return 0;

  for (ref_index = 0; ref_index < ref_count; ref_index++) {
    const AppStateDispatchSurfaceMetadata *surface =
        AppStateDispatchSurfaceLookup(refs[ref_index]);
    size_t previous_index;

    if (surface == NULL)
      return 0;
    if (!NonEmptyString(surface->transition_id) ||
        strcmp(surface->transition_id, transition_id) != 0)
      return 0;
    for (previous_index = 0; previous_index < ref_index; previous_index++) {
      if (strcmp(refs[previous_index], refs[ref_index]) == 0)
        return 0;
    }
  }

  return 1;
}

static int AppStateDispatchSurfaceSequenceRefsReady(
    const AppStateDispatchSurfaceMetadata *metadata) {
  if (metadata == NULL)
    return 0;

  return AppStateTransitionSequenceRefsReady(
      metadata->transition_sequence_refs, metadata->transition_sequence_ref_count,
      metadata->transition_id);
}

static int AppStateDispatchSurfaceSequenceRefsCoverField(
    const AppStateDispatchSurfaceMetadata *metadata, const char *field) {
  size_t ref_index;
  int diff_covered = 0;
  int invariant_covered = 0;

  if (metadata == NULL || !NonEmptyString(field))
    return 0;

  for (ref_index = 0; ref_index < metadata->transition_sequence_ref_count;
       ref_index++) {
    const AppStateTransitionSequenceMetadata *sequence =
        AppStateTransitionSequenceLookup(metadata->transition_sequence_refs[ref_index]);
    size_t step_index;

    if (sequence == NULL)
      return 0;
    for (step_index = 0; step_index < sequence->step_count; step_index++) {
      const AppStateTransitionSequenceStepMetadata *step =
          &sequence->steps[step_index];

      if (!NonEmptyString(step->transition_id))
        return 0;
      if (strcmp(step->transition_id, metadata->transition_id) != 0)
        continue;
      if (AppStateTransitionSequenceStepDiffHarnessCoversField(step, field))
        diff_covered = 1;
      if (AppStateTransitionSequenceStepInvariantCoversField(step, field))
        invariant_covered = 1;
    }
  }

  return diff_covered && invariant_covered;
}

static int AppStateDispatchSurfaceWritesReady(
    const AppStateDispatchSurfaceMetadata *metadata) {
  const AppStateTransitionMetadata *transition =
      AppStateTransitionLookup(metadata->transition_id);
  size_t write_index;

  if (transition == NULL)
    return 0;

  if (metadata->allowed_direct_write_count == 0)
    return metadata->allowed_direct_writes == NULL;

  if (metadata->allowed_direct_writes == NULL)
    return 0;

  for (write_index = 0; write_index < metadata->allowed_direct_write_count;
       write_index++) {
    const char *field = metadata->allowed_direct_writes[write_index];

    if (!NonEmptyString(field))
      return 0;
    if (AppStateOwnerFieldLookup(field) == NULL)
      return 0;
    if (!StringListContains(transition->declared_write_set,
                            transition->declared_write_set_count, field))
      return 0;
    if (!AppStateDispatchSurfaceWriteHasInvariantCoverage(metadata->surface_id,
                                                          field))
      return 0;
    if (!AppStateDispatchSurfaceSequenceRefsCoverField(metadata, field))
      return 0;
  }

  return 1;
}

static int AppStateDispatchSurfacesReady(void) {
  size_t index;
  size_t required_surface_id_count =
      sizeof(kAppStateRequiredDispatchSurfaceIds) /
      sizeof(kAppStateRequiredDispatchSurfaceIds[0]);

  if (AppStateDispatchSurfaceCount() != required_surface_id_count)
    return 0;

  for (index = 0; index < AppStateDispatchSurfaceCount(); index++) {
    const AppStateDispatchSurfaceMetadata *metadata =
        AppStateDispatchSurfaceAt(index);
    size_t previous_index;

    if (metadata == NULL || !NonEmptyString(metadata->surface_id) ||
        !NonEmptyString(metadata->category) ||
        !NonEmptyString(metadata->source_path) ||
        !NonEmptyString(metadata->entry_symbol_or_path) ||
        !NonEmptyString(metadata->transition_id) ||
        !NonEmptyString(metadata->boundary_status) ||
        !AppStateDispatchSurfaceSequenceRefsReady(metadata) ||
        !AppStateDispatchSurfaceWritesReady(metadata) ||
        !NonEmptyStringList(metadata->migration_notes,
                            metadata->migration_note_count))
      return 0;
    if (AppStateDispatchSurfaceLookup(metadata->surface_id) != metadata)
      return 0;
    if (AppStateTransitionLookup(metadata->transition_id) == NULL)
      return 0;

    for (previous_index = 0; previous_index < index; previous_index++) {
      const AppStateDispatchSurfaceMetadata *previous =
          AppStateDispatchSurfaceAt(previous_index);

      if (previous == NULL ||
          strcmp(previous->surface_id, metadata->surface_id) == 0)
        return 0;
    }
  }

  for (index = 0; index < required_surface_id_count; index++) {
    if (AppStateDispatchSurfaceLookup(
            kAppStateRequiredDispatchSurfaceIds[index]) == NULL)
      return 0;
  }

  if (AppStateDispatchSurfaceAt(AppStateDispatchSurfaceCount()) != NULL)
    return 0;
  if (AppStateDispatchSurfaceLookup(NULL) != NULL)
    return 0;
  if (AppStateDispatchSurfaceLookup("") != NULL)
    return 0;
  if (AppStateDispatchSurfaceLookup("surface.__ytnova_unknown__") != NULL)
    return 0;

  return 1;
}

static int AppStateInvariantRegistryReady(void) {
  size_t index;
  size_t required_invariant_id_count = sizeof(kAppStateRequiredInvariantIds) /
                                       sizeof(kAppStateRequiredInvariantIds[0]);

  if (AppStateInvariantCount() != required_invariant_id_count)
    return 0;

  for (index = 0; index < AppStateInvariantCount(); index++) {
    const AppStateInvariantMetadata *metadata = AppStateInvariantAt(index);
    size_t protected_field_index;
    size_t transition_index;
    size_t previous_index;

    if (metadata == NULL || !NonEmptyString(metadata->invariant_id) ||
        !NonEmptyString(metadata->category) ||
        !NonEmptyString(metadata->owner_region) ||
        !NonEmptyString(metadata->failure_mode) ||
        !NonEmptyString(metadata->enforcement_status) ||
        !NonEmptyString(metadata->test_strategy) ||
        !NonEmptyStringList(metadata->protected_fields,
                            metadata->protected_field_count) ||
        !NonEmptyStringList(metadata->transition_ids,
                            metadata->transition_id_count) ||
        !NonEmptyStringList(metadata->migration_notes,
                            metadata->migration_note_count) ||
        !AppStateInvariantDispatchSurfacesReady(metadata))
      return 0;
    if (AppStateInvariantLookup(metadata->invariant_id) != metadata)
      return 0;

    for (previous_index = 0; previous_index < index; previous_index++) {
      const AppStateInvariantMetadata *previous =
          AppStateInvariantAt(previous_index);

      if (previous == NULL ||
          strcmp(previous->invariant_id, metadata->invariant_id) == 0)
        return 0;
    }

    for (protected_field_index = 0;
         protected_field_index < metadata->protected_field_count;
         protected_field_index++) {
      const char *field = metadata->protected_fields[protected_field_index];

      if (AppStateOwnerFieldLookup(field) == NULL)
        return 0;
      if (!AppStateDiffHarnessOwnerFieldCovered(field))
        return 0;
    }

    for (transition_index = 0;
         transition_index < metadata->transition_id_count; transition_index++) {
      if (AppStateTransitionLookup(metadata->transition_ids[transition_index]) ==
          NULL)
        return 0;
    }
    for (transition_index = 0;
         transition_index < metadata->dispatch_surface_id_count;
         transition_index++) {
      if (AppStateDispatchSurfaceLookup(
              metadata->dispatch_surface_ids[transition_index]) == NULL)
        return 0;
    }
  }

  for (index = 0; index < required_invariant_id_count; index++) {
    if (AppStateInvariantLookup(kAppStateRequiredInvariantIds[index]) == NULL)
      return 0;
  }

  if (AppStateInvariantAt(AppStateInvariantCount()) != NULL)
    return 0;
  if (AppStateInvariantLookup(NULL) != NULL)
    return 0;
  if (AppStateInvariantLookup("") != NULL)
    return 0;
  if (AppStateInvariantLookup("invariant.__ytnova_unknown__") != NULL)
    return 0;

  return 1;
}

static int AppStateInvariantRefsReady(const char *const *refs, size_t ref_count,
                                      const char *transition_id,
                                      const char *const *declared_write_set,
                                      size_t declared_write_set_count) {
  size_t ref_index;
  size_t write_index;

  if (!NonEmptyString(transition_id) || !NonEmptyStringList(refs, ref_count) ||
      !NonEmptyStringList(declared_write_set, declared_write_set_count))
    return 0;

  for (ref_index = 0; ref_index < ref_count; ref_index++) {
    const AppStateInvariantMetadata *invariant =
        AppStateInvariantLookup(refs[ref_index]);
    size_t previous_index;

    if (invariant == NULL)
      return 0;
    if (!NonEmptyStringList(invariant->transition_ids,
                            invariant->transition_id_count) ||
        !NonEmptyStringList(invariant->protected_fields,
                            invariant->protected_field_count))
      return 0;
    if (!StringListContains(invariant->transition_ids,
                            invariant->transition_id_count, transition_id))
      return 0;
    for (previous_index = 0; previous_index < ref_index; previous_index++) {
      if (strcmp(refs[previous_index], refs[ref_index]) == 0)
        return 0;
    }
  }

  for (write_index = 0; write_index < declared_write_set_count; write_index++) {
    if (!AppStateTransitionWriteHasInvariantCoverage(
            transition_id, declared_write_set[write_index]))
      return 0;
    for (ref_index = 0; ref_index < ref_count; ref_index++) {
      if (AppStateInvariantProtectsField(refs[ref_index],
                                         declared_write_set[write_index]))
        break;
    }
    if (ref_index == ref_count)
      return 0;
  }

  return 1;
}

static int AppStateGenerationDomainRefsReady(const char *const *refs,
                                             size_t ref_count,
                                             const char *transition_id) {
  size_t ref_index;

  if (!NonEmptyString(transition_id) || !NonEmptyStringList(refs, ref_count))
    return 0;

  for (ref_index = 0; ref_index < ref_count; ref_index++) {
    const AppStateGenerationDomainMetadata *domain =
        AppStateGenerationDomainLookup(refs[ref_index]);
    size_t previous_index;

    if (domain == NULL || !NonEmptyStringList(domain->coverage_transition_ids,
                                              domain->coverage_transition_id_count))
      return 0;
    if (!StringListContains(domain->coverage_transition_ids,
                            domain->coverage_transition_id_count,
                            transition_id))
      return 0;
    for (previous_index = 0; previous_index < ref_index; previous_index++) {
      if (strcmp(refs[previous_index], refs[ref_index]) == 0)
        return 0;
    }
  }

  return 1;
}

static int AppStateActionTransitionsReady(void) {
  size_t index;

  if (AppStateActionTransitionCount() != (size_t)ACTION_USER_CMD + 1)
    return 0;

  for (index = 0; index < AppStateActionTransitionCount(); index++) {
    const AppStateActionTransitionMetadata *action_metadata;
    const AppStateTransitionMetadata *transition_metadata;

    action_metadata = AppStateActionTransitionLookup((YtreeNovaAction)index);
    if (action_metadata == NULL ||
        !NonEmptyString(action_metadata->transition_id) ||
        !NonEmptyString(action_metadata->category))
      return 0;

    transition_metadata =
        AppStateTransitionLookup(action_metadata->transition_id);
    if (transition_metadata == NULL)
      return 0;
    if (strcmp(action_metadata->category, transition_metadata->category) != 0)
      return 0;
  }

  return 1;
}

static int AppStateActionCoverageWriteSetMatches(
    const AppStateActionCoverageMetadata *coverage,
    const AppStateTransitionMetadata *transition) {
  size_t index;

  if (coverage->declared_write_set_count != transition->declared_write_set_count)
    return 0;

  for (index = 0; index < coverage->declared_write_set_count; index++) {
    if (strcmp(coverage->declared_write_set[index],
               transition->declared_write_set[index]) != 0)
      return 0;
  }

  return 1;
}

static int AppStateEventCoverageWriteSetMatches(
    const AppStateEventCoverageMetadata *coverage,
    const AppStateTransitionMetadata *transition) {
  size_t index;

  if (coverage->declared_write_set_count != transition->declared_write_set_count)
    return 0;

  for (index = 0; index < coverage->declared_write_set_count; index++) {
    if (strcmp(coverage->declared_write_set[index],
               transition->declared_write_set[index]) != 0)
      return 0;
  }

  return 1;
}

static int AppStateCoverageOwnerFieldRefsReady(
    const char *const *owner_field_refs, size_t owner_field_ref_count,
    const char *const *declared_write_set, size_t declared_write_set_count) {
  size_t ref_index;
  size_t write_index;

  if (!NonEmptyStringList(owner_field_refs, owner_field_ref_count) ||
      !NonEmptyStringList(declared_write_set, declared_write_set_count))
    return 0;

  for (ref_index = 0; ref_index < owner_field_ref_count; ref_index++) {
    const char *owner_field = owner_field_refs[ref_index];

    if (AppStateOwnerFieldLookup(owner_field) == NULL)
      return 0;
    if (StringListContains(owner_field_refs, ref_index, owner_field))
      return 0;
    if (!StringListContains(declared_write_set, declared_write_set_count,
                            owner_field))
      return 0;
  }

  for (write_index = 0; write_index < declared_write_set_count; write_index++) {
    if (!StringListContains(owner_field_refs, owner_field_ref_count,
                            declared_write_set[write_index]))
      return 0;
  }

  return 1;
}

static int AppStateRequiredEventClassCovered(const char *event_class) {
  size_t index;

  for (index = 0; index < AppStateEventCoverageCount(); index++) {
    const AppStateEventCoverageMetadata *coverage = AppStateEventCoverageAt(index);

    if (coverage != NULL && strcmp(coverage->event_class, event_class) == 0)
      return 1;
  }

  return 0;
}

static int AppStateRequiredEventIdCovered(const char *event_id) {
  return AppStateEventCoverageLookup(event_id) != NULL;
}

static int AppStateDiffHarnessRefsReady(
    const char *const *diff_harness_refs, size_t diff_harness_ref_count,
    const char *transition_id, const char *const *owner_field_refs,
    size_t owner_field_ref_count, const char *const *invariant_refs,
    size_t invariant_ref_count, const char *const *generation_domain_refs,
    size_t generation_domain_ref_count) {
  size_t ref_index;

  if (!NonEmptyString(transition_id) ||
      !NonEmptyStringList(diff_harness_refs, diff_harness_ref_count) ||
      !NonEmptyStringList(owner_field_refs, owner_field_ref_count) ||
      !NonEmptyStringList(invariant_refs, invariant_ref_count) ||
      !NonEmptyStringList(generation_domain_refs,
                          generation_domain_ref_count))
    return 0;

  for (ref_index = 0; ref_index < diff_harness_ref_count; ref_index++) {
    const AppStateDiffHarnessMetadata *harness =
        AppStateDiffHarnessLookup(diff_harness_refs[ref_index]);
    size_t previous_index;

    if (harness == NULL ||
        !NonEmptyStringList(harness->transition_ids,
                            harness->transition_id_count) ||
        !NonEmptyStringList(harness->owner_field_refs,
                            harness->owner_field_ref_count) ||
        !NonEmptyStringList(harness->invariant_ids,
                            harness->invariant_id_count) ||
        !NonEmptyStringList(harness->generation_domain_ids,
                            harness->generation_domain_id_count))
      return 0;
    if (!StringListContains(harness->transition_ids,
                            harness->transition_id_count, transition_id))
      return 0;
    for (previous_index = 0; previous_index < ref_index; previous_index++) {
      if (strcmp(diff_harness_refs[previous_index],
                 diff_harness_refs[ref_index]) == 0)
        return 0;
    }
  }

  for (ref_index = 0; ref_index < owner_field_ref_count; ref_index++) {
    size_t harness_index;
    int ref_covered = 0;

    for (harness_index = 0; harness_index < diff_harness_ref_count;
         harness_index++) {
      const AppStateDiffHarnessMetadata *harness =
          AppStateDiffHarnessLookup(diff_harness_refs[harness_index]);

      if (harness == NULL)
        return 0;
      if (StringListContains(harness->owner_field_refs,
                             harness->owner_field_ref_count,
                             owner_field_refs[ref_index])) {
        ref_covered = 1;
        break;
      }
    }
    if (!ref_covered)
      return 0;
  }

  for (ref_index = 0; ref_index < invariant_ref_count; ref_index++) {
    size_t harness_index;
    int ref_covered = 0;

    for (harness_index = 0; harness_index < diff_harness_ref_count;
         harness_index++) {
      const AppStateDiffHarnessMetadata *harness =
          AppStateDiffHarnessLookup(diff_harness_refs[harness_index]);

      if (harness == NULL)
        return 0;
      if (StringListContains(harness->invariant_ids,
                             harness->invariant_id_count,
                             invariant_refs[ref_index])) {
        ref_covered = 1;
        break;
      }
    }
    if (!ref_covered)
      return 0;
  }

  for (ref_index = 0; ref_index < generation_domain_ref_count; ref_index++) {
    size_t harness_index;
    int ref_covered = 0;

    for (harness_index = 0; harness_index < diff_harness_ref_count;
         harness_index++) {
      const AppStateDiffHarnessMetadata *harness =
          AppStateDiffHarnessLookup(diff_harness_refs[harness_index]);

      if (harness == NULL)
        return 0;
      if (StringListContains(harness->generation_domain_ids,
                             harness->generation_domain_id_count,
                             generation_domain_refs[ref_index])) {
        ref_covered = 1;
        break;
      }
    }
    if (!ref_covered)
      return 0;
  }

  return 1;
}

static int AppStateEventCoverageReady(void) {
  size_t index;
  size_t required_class_count =
      sizeof(kAppStateRequiredEventClasses) /
      sizeof(kAppStateRequiredEventClasses[0]);
  size_t required_event_id_count =
      sizeof(kAppStateRequiredEventIds) / sizeof(kAppStateRequiredEventIds[0]);

  if (AppStateEventCoverageCount() != required_class_count ||
      AppStateEventCoverageCount() != required_event_id_count)
    return 0;

  for (index = 0; index < AppStateEventCoverageCount(); index++) {
    const AppStateEventCoverageMetadata *coverage;
    const AppStateTransitionMetadata *transition;
    size_t previous_index;

    coverage = AppStateEventCoverageAt(index);
    if (coverage == NULL || !NonEmptyString(coverage->event_id) ||
        !NonEmptyString(coverage->event_class) ||
        !NonEmptyString(coverage->transition_id) ||
        !NonEmptyString(coverage->category) || !NonEmptyString(coverage->source) ||
        !NonEmptyString(coverage->owner) ||
        !NonEmptyString(coverage->boundary_status) ||
        !NonEmptyStringList(coverage->declared_write_set,
                            coverage->declared_write_set_count) ||
        !AppStateCoverageOwnerFieldRefsReady(
            coverage->owner_field_refs, coverage->owner_field_ref_count,
            coverage->declared_write_set, coverage->declared_write_set_count) ||
        !NonEmptyStringList(coverage->trigger_paths,
                            coverage->trigger_path_count) ||
        !AppStateTransitionSequenceRefsReady(
            coverage->transition_sequence_refs,
            coverage->transition_sequence_ref_count, coverage->transition_id) ||
        !AppStateDispatchSurfaceRefsReady(coverage->dispatch_surface_refs,
                                         coverage->dispatch_surface_ref_count,
                                         coverage->transition_id) ||
        !AppStateGenerationDomainRefsReady(
            coverage->generation_domain_refs,
            coverage->generation_domain_ref_count, coverage->transition_id) ||
        !AppStateInvariantRefsReady(coverage->invariant_refs,
                                    coverage->invariant_ref_count,
                                    coverage->transition_id,
                                    coverage->declared_write_set,
                                    coverage->declared_write_set_count) ||
        !AppStateDiffHarnessRefsReady(
            coverage->diff_harness_refs, coverage->diff_harness_ref_count,
            coverage->transition_id, coverage->owner_field_refs,
            coverage->owner_field_ref_count, coverage->invariant_refs,
            coverage->invariant_ref_count, coverage->generation_domain_refs,
            coverage->generation_domain_ref_count) ||
        !NonEmptyStringList(coverage->migration_notes,
                            coverage->migration_note_count))
      return 0;
    if (AppStateEventCoverageLookup(coverage->event_id) != coverage)
      return 0;

    for (previous_index = 0; previous_index < index; previous_index++) {
      const AppStateEventCoverageMetadata *previous =
          AppStateEventCoverageAt(previous_index);

      if (previous == NULL ||
          strcmp(previous->event_id, coverage->event_id) == 0 ||
          strcmp(previous->event_class, coverage->event_class) == 0)
        return 0;
    }

    transition = AppStateTransitionLookup(coverage->transition_id);
    if (transition == NULL)
      return 0;
    if (strcmp(coverage->category, transition->category) != 0)
      return 0;
    if (strcmp(coverage->owner, transition->owner) != 0)
      return 0;
    if (!AppStateEventCoverageWriteSetMatches(coverage, transition))
      return 0;
  }

  for (index = 0; index < required_class_count; index++) {
    if (!AppStateRequiredEventClassCovered(kAppStateRequiredEventClasses[index]))
      return 0;
  }

  for (index = 0; index < required_event_id_count; index++) {
    if (!AppStateRequiredEventIdCovered(kAppStateRequiredEventIds[index]))
      return 0;
  }

  if (AppStateEventCoverageAt(AppStateEventCoverageCount()) != NULL)
    return 0;
  if (AppStateEventCoverageLookup(NULL) != NULL)
    return 0;
  if (AppStateEventCoverageLookup("") != NULL)
    return 0;
  if (AppStateEventCoverageLookup("event.__ytnova_unknown__") != NULL)
    return 0;

  return 1;
}

static int AppStateActionCoverageReady(void) {
  size_t index;
  size_t action_id_count =
      sizeof(kAppStateActionIds) / sizeof(kAppStateActionIds[0]);

  if (AppStateActionCoverageCount() != (size_t)ACTION_USER_CMD + 1)
    return 0;
  if (AppStateActionCoverageCount() != action_id_count)
    return 0;

  for (index = 0; index < AppStateActionCoverageCount(); index++) {
    const AppStateActionCoverageMetadata *coverage;
    const AppStateActionTransitionMetadata *action_transition;
    const AppStateTransitionMetadata *transition;
    size_t previous_index;

    coverage = AppStateActionCoverageAt(index);
    if (coverage == NULL || coverage->action != (YtreeNovaAction)index ||
        !NonEmptyString(coverage->action_name) ||
        strcmp(coverage->action_name, kAppStateActionIds[index].action_id) != 0 ||
        !NonEmptyString(coverage->transition_id) ||
        !NonEmptyString(coverage->category) || !NonEmptyString(coverage->owner) ||
        !NonEmptyString(coverage->boundary_status) ||
        !NonEmptyStringList(coverage->declared_write_set,
                            coverage->declared_write_set_count) ||
        !AppStateCoverageOwnerFieldRefsReady(
            coverage->owner_field_refs, coverage->owner_field_ref_count,
            coverage->declared_write_set, coverage->declared_write_set_count) ||
        !AppStateTransitionSequenceRefsReady(
            coverage->transition_sequence_refs,
            coverage->transition_sequence_ref_count, coverage->transition_id) ||
        !AppStateDispatchSurfaceRefsReady(coverage->dispatch_surface_refs,
                                         coverage->dispatch_surface_ref_count,
                                         coverage->transition_id) ||
        !AppStateGenerationDomainRefsReady(
            coverage->generation_domain_refs,
            coverage->generation_domain_ref_count, coverage->transition_id) ||
        !AppStateInvariantRefsReady(coverage->invariant_refs,
                                    coverage->invariant_ref_count,
                                    coverage->transition_id,
                                    coverage->declared_write_set,
                                    coverage->declared_write_set_count) ||
        !AppStateDiffHarnessRefsReady(
            coverage->diff_harness_refs, coverage->diff_harness_ref_count,
            coverage->transition_id, coverage->owner_field_refs,
            coverage->owner_field_ref_count, coverage->invariant_refs,
            coverage->invariant_ref_count, coverage->generation_domain_refs,
            coverage->generation_domain_ref_count) ||
        !NonEmptyStringList(coverage->migration_notes,
                            coverage->migration_note_count))
      return 0;
    if (AppStateActionCoverageLookup((YtreeNovaAction)index) != coverage)
      return 0;

    for (previous_index = 0; previous_index < index; previous_index++) {
      const AppStateActionCoverageMetadata *previous =
          AppStateActionCoverageAt(previous_index);

      if (previous == NULL || previous->action == coverage->action)
        return 0;
    }

    transition = AppStateTransitionLookup(coverage->transition_id);
    if (transition == NULL)
      return 0;
    if (strcmp(coverage->category, transition->category) != 0)
      return 0;
    if (strcmp(coverage->owner, transition->owner) != 0)
      return 0;
    if (!AppStateActionCoverageWriteSetMatches(coverage, transition))
      return 0;

    action_transition = AppStateActionTransitionLookup(coverage->action);
    if (action_transition == NULL)
      return 0;
    if (strcmp(coverage->transition_id, action_transition->transition_id) != 0)
      return 0;
    if (strcmp(coverage->category, action_transition->category) != 0)
      return 0;
  }

  if (AppStateActionCoverageAt(AppStateActionCoverageCount()) != NULL)
    return 0;
  if (AppStateActionCoverageLookup((YtreeNovaAction)-1) != NULL)
    return 0;
  if (AppStateActionCoverageLookup((YtreeNovaAction)(ACTION_USER_CMD + 1)) !=
      NULL)
    return 0;

  return 1;
}

static int AppStateDiffHarnessWriteCovered(const char *owner_field,
                                           const char *transition_id) {
  size_t harness_index;

  if (!NonEmptyString(owner_field) || !NonEmptyString(transition_id))
    return 0;

  for (harness_index = 0; harness_index < AppStateDiffHarnessCount();
       harness_index++) {
    const AppStateDiffHarnessMetadata *harness =
        AppStateDiffHarnessAt(harness_index);
    size_t ref_index;
    int transition_seen = 0;
    int owner_field_seen = 0;

    if (harness == NULL || harness->transition_ids == NULL ||
        harness->owner_field_refs == NULL)
      return 0;

    for (ref_index = 0; ref_index < harness->transition_id_count;
         ref_index++) {
      const char *harness_transition_id = harness->transition_ids[ref_index];

      if (!NonEmptyString(harness_transition_id))
        return 0;
      if (strcmp(harness_transition_id, transition_id) == 0)
        transition_seen = 1;
    }
    for (ref_index = 0; ref_index < harness->owner_field_ref_count;
         ref_index++) {
      const char *harness_owner_field = harness->owner_field_refs[ref_index];

      if (!NonEmptyString(harness_owner_field))
        return 0;
      if (strcmp(harness_owner_field, owner_field) == 0)
        owner_field_seen = 1;
    }
    if (transition_seen && owner_field_seen)
      return 1;
  }

  return 0;
}

static int AppStateGenerationAdvanceHasDiffHarnessCoverage(
    const char *domain_id, const char *transition_id) {
  size_t harness_index;

  if (!NonEmptyString(domain_id) || !NonEmptyString(transition_id))
    return 0;

  for (harness_index = 0; harness_index < AppStateDiffHarnessCount();
       harness_index++) {
    const AppStateDiffHarnessMetadata *harness =
        AppStateDiffHarnessAt(harness_index);
    size_t ref_index;
    int domain_seen = 0;
    int transition_seen = 0;

    if (harness == NULL || harness->generation_domain_ids == NULL ||
        harness->transition_ids == NULL)
      return 0;

    for (ref_index = 0; ref_index < harness->generation_domain_id_count;
         ref_index++) {
      const char *harness_domain_id = harness->generation_domain_ids[ref_index];

      if (!NonEmptyString(harness_domain_id))
        return 0;
      if (strcmp(harness_domain_id, domain_id) == 0)
        domain_seen = 1;
    }
    for (ref_index = 0; ref_index < harness->transition_id_count;
         ref_index++) {
      const char *harness_transition_id = harness->transition_ids[ref_index];

      if (!NonEmptyString(harness_transition_id))
        return 0;
      if (strcmp(harness_transition_id, transition_id) == 0)
        transition_seen = 1;
    }
    if (domain_seen && transition_seen)
      return 1;
  }

  return 0;
}

static int AppStateDiffHarnessInvariantCoversTransition(
    const AppStateDiffHarnessMetadata *metadata, const char *transition_id) {
  size_t ref_index;

  if (metadata == NULL || !NonEmptyString(transition_id) ||
      !NonEmptyStringList(metadata->invariant_ids, metadata->invariant_id_count))
    return 0;

  for (ref_index = 0; ref_index < metadata->invariant_id_count; ref_index++) {
    const AppStateInvariantMetadata *invariant =
        AppStateInvariantLookup(metadata->invariant_ids[ref_index]);

    if (invariant == NULL || invariant->transition_ids == NULL)
      return 0;
    if (StringListContains(invariant->transition_ids,
                           invariant->transition_id_count, transition_id))
      return 1;
  }

  return 0;
}

static int AppStateDiffHarnessInvariantProtectsOwnerField(
    const AppStateDiffHarnessMetadata *metadata, const char *owner_field) {
  size_t ref_index;

  if (metadata == NULL || !NonEmptyString(owner_field) ||
      !NonEmptyStringList(metadata->invariant_ids,
                          metadata->invariant_id_count))
    return 0;

  for (ref_index = 0; ref_index < metadata->invariant_id_count; ref_index++) {
    if (AppStateInvariantProtectsField(metadata->invariant_ids[ref_index],
                                       owner_field))
      return 1;
  }

  return 0;
}

static int AppStateDiffHarnessRegistryReady(void) {
  size_t generation_index;
  size_t index;
  size_t required_diff_harness_id_count =
      sizeof(kAppStateRequiredDiffHarnessIds) /
      sizeof(kAppStateRequiredDiffHarnessIds[0]);

  if (AppStateDiffHarnessCount() != required_diff_harness_id_count)
    return 0;

  for (index = 0; index < AppStateDiffHarnessCount(); index++) {
    const AppStateDiffHarnessMetadata *metadata =
        AppStateDiffHarnessAt(index);
    size_t ref_index;
    size_t previous_index;

    if (metadata == NULL || !NonEmptyString(metadata->harness_id) ||
        !NonEmptyString(metadata->check_category) ||
        !NonEmptyStringList(metadata->snapshot_phases,
                            metadata->snapshot_phase_count) ||
        !NonEmptyStringList(metadata->snapshot_regions,
                            metadata->snapshot_region_count) ||
        !NonEmptyStringList(metadata->transition_ids,
                            metadata->transition_id_count) ||
        !NonEmptyStringList(metadata->owner_field_refs,
                            metadata->owner_field_ref_count) ||
        !NonEmptyStringList(metadata->invariant_ids,
                            metadata->invariant_id_count) ||
        !NonEmptyStringList(metadata->generation_domain_ids,
                            metadata->generation_domain_id_count) ||
        !NonEmptyString(metadata->expected_behavior) ||
        !NonEmptyString(metadata->failure_mode) ||
        !NonEmptyString(metadata->enforcement_status) ||
        !NonEmptyStringList(metadata->migration_notes,
                            metadata->migration_note_count))
      return 0;
    if (AppStateDiffHarnessLookup(metadata->harness_id) != metadata)
      return 0;

    for (previous_index = 0; previous_index < index; previous_index++) {
      const AppStateDiffHarnessMetadata *previous =
          AppStateDiffHarnessAt(previous_index);

      if (previous == NULL ||
          strcmp(previous->harness_id, metadata->harness_id) == 0)
        return 0;
    }

    for (ref_index = 0; ref_index < metadata->transition_id_count;
         ref_index++) {
      if (AppStateTransitionLookup(metadata->transition_ids[ref_index]) ==
          NULL)
        return 0;
      if (!AppStateDiffHarnessInvariantCoversTransition(
              metadata, metadata->transition_ids[ref_index]))
        return 0;
    }
    for (ref_index = 0; ref_index < metadata->owner_field_ref_count;
         ref_index++) {
      if (AppStateOwnerFieldLookup(metadata->owner_field_refs[ref_index]) ==
          NULL)
        return 0;
    }
    for (ref_index = 0; ref_index < metadata->invariant_id_count;
         ref_index++) {
      if (AppStateInvariantLookup(metadata->invariant_ids[ref_index]) == NULL)
        return 0;
    }
    for (ref_index = 0; ref_index < metadata->owner_field_ref_count;
         ref_index++) {
      if (!AppStateDiffHarnessInvariantProtectsOwnerField(
              metadata, metadata->owner_field_refs[ref_index]))
        return 0;
    }
    for (ref_index = 0; ref_index < metadata->generation_domain_id_count;
         ref_index++) {
      if (AppStateGenerationDomainLookup(
              metadata->generation_domain_ids[ref_index]) == NULL)
        return 0;
    }
  }

  for (index = 0; index < required_diff_harness_id_count; index++) {
    if (AppStateDiffHarnessLookup(kAppStateRequiredDiffHarnessIds[index]) ==
        NULL)
      return 0;
  }

  for (generation_index = 0; generation_index < AppStateGenerationDomainCount();
       generation_index++) {
    const AppStateGenerationDomainMetadata *domain =
        AppStateGenerationDomainAt(generation_index);
    size_t transition_index;

    if (domain == NULL || !NonEmptyString(domain->domain_id) ||
        domain->advances_on_transition_ids == NULL)
      return 0;
    for (transition_index = 0;
         transition_index < domain->advances_on_transition_id_count;
         transition_index++) {
      if (!AppStateGenerationAdvanceHasDiffHarnessCoverage(
              domain->domain_id,
              domain->advances_on_transition_ids[transition_index]))
        return 0;
    }
  }

  for (index = 0; index < AppStateTransitionCount(); index++) {
    const AppStateTransitionMetadata *transition = AppStateTransitionAt(index);
    size_t write_index;

    if (transition == NULL || !NonEmptyString(transition->id) ||
        transition->declared_write_set == NULL)
      return 0;
    for (write_index = 0; write_index < transition->declared_write_set_count;
         write_index++) {
      const char *field = transition->declared_write_set[write_index];

      if (!AppStateDiffHarnessWriteCovered(field, transition->id))
        return 0;
    }
  }

  if (AppStateDiffHarnessAt(AppStateDiffHarnessCount()) != NULL)
    return 0;
  if (AppStateDiffHarnessLookup(NULL) != NULL)
    return 0;
  if (AppStateDiffHarnessLookup("") != NULL)
    return 0;
  if (AppStateDiffHarnessLookup("harness.__ytnova_unknown__") != NULL)
    return 0;

  return 1;
}

static void SigIntHandler(int sig) {
  (void)sig;
  ytnova_shutdown_flag = 1;
}

static int GetDefaultSurfacePath(ConfigSurface surface, char *path,
                                 size_t path_size) {
  const char *home;

  home = getenv("HOME");
  if (path == NULL || path_size == 0 || home == NULL || *home == '\0')
    return -1;
  return ConfigPaths_ResolveBootstrapPath(surface, path, path_size, FALSE);
}

/*
 * Return values:
 *   0 = file created
 *   1 = file already exists (left untouched)
 *  -1 = hard error
 */
static int InitDefaultFile(const char *path, const char *contents) {
  int fd;
  FILE *fp;
  size_t len;
  size_t written;

  if (!path || !*path || !contents)
    return -1;

  fd = open(path, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    if (errno == EEXIST)
      return 1;
    return -1;
  }

  fp = fdopen(fd, "w");
  if (!fp) {
    close(fd);
    unlink(path);
    return -1;
  }

  len = strlen(contents);
  written = fwrite(contents, 1, len, fp);
  if (written != len || fclose(fp) != 0) {
    unlink(path);
    return -1;
  }

  return 0;
}

int main(int argc, char **argv) {
  int argi;
  const char *hist;
  const char *conf;
  BOOL init_requested = FALSE;
  const char *filter_arg = NULL; /* Added for -f option */
  int *path_indexes;
  int path_count = 0;
  ViewContext ctx;

  memset(&ctx, 0, sizeof(ViewContext));
  CoreMainOps_Register(&ctx);
  if (!CoreMainOpsReady(&ctx.core_main_ops) ||
      !AppStateOwnerFieldsReady() ||
      !AppStateTransitionRegistryReady() ||
      !AppStateGenerationDomainsReady() ||
      !AppStateDispatchSurfacesReady() ||
      !AppStateInvariantRegistryReady() ||
      !AppStateDiffHarnessRegistryReady() ||
      !AppStateTransitionSequencesReady() ||
      !AppStateActionCoverageReady() ||
      !AppStateEventCoverageReady() ||
      !AppStateActionTransitionsReady()) {
    fprintf(stderr, "EXIT: startup invariants not configured\n");
    exit(1);
  }

  /* Register Signal Handlers */
  /* signal(SIGSEGV, EmergencyExit); */ /* Segfault */
  /* signal(SIGABRT, EmergencyExit); */ /* Abort */
  signal(SIGINT, SigIntHandler);        /* Ctrl-C safety */

  /* setlocale is now handled in Init */

  hist = NULL;
  conf = NULL;

  /* Pass 1: Pre-scan Loop - Parse Options (-p, -h) */
  /* Note: -d and -f are validated here to prevent usage error, but processed
   * after Init */
  for (argi = 1; argi < argc; argi++) {
    if (!strcmp(argv[argi], "-v") || !strcmp(argv[argi], "-V") ||
        !strcmp(argv[argi], "--version")) {
      fprintf(stdout, "ytnova %s (%s)\n", VERSION, VERSIONDATE);
      return 0;
    }
    if (!strcmp(argv[argi], "--init")) {
      init_requested = TRUE;
      continue;
    }

    if (argv[argi][0] == '-') {
      switch (argv[argi][1]) {
      case 'p':
      case 'P':
        if (argv[argi][2] <= ' ') {
          if (argi + 1 < argc)
            conf = argv[++argi];
          else {
            fprintf(stderr, "Option -p requires an argument\n");
            exit(1);
          }
        } else {
          conf = argv[argi] + 2;
        }
        break;
      case 'h':
      case 'H':
        if (argv[argi][2] <= ' ') {
          if (argi + 1 < argc)
            hist = argv[++argi];
          else {
            fprintf(stderr, "Option -h requires an argument\n");
            exit(1);
          }
        } else {
          hist = argv[argi] + 2;
        }
        break;
      case 'd':
      case 'D':
        /* Skip -d here, processed after Init */
        if (argv[argi][2] <= ' ') {
          if (argi + 1 < argc)
            argi++;
          else {
            fprintf(stderr, "Option -d requires an argument\n");
            exit(1);
          }
        }
        break;
      case 'f':
      case 'F':
        /* Skip -f here, processed after Init */
        if (argv[argi][2] <= ' ') {
          if (argi + 1 < argc)
            argi++;
          else {
            fprintf(stderr, "Option -f requires an argument\n");
            exit(1);
          }
        }
        break;
      default:
        fprintf(stderr,
                "Usage: %s [--init] [-v|-V|--version] [-p profile_file] "
                "[-h hist_file] [-d depth] [-f filter] [directory ...]\n",
                argv[0]);
        exit(1);
      }
    }
  }

  if (init_requested) {
    char init_path_buffer[PATH_LENGTH + 1];
    char init_theme_path_buffer[PATH_LENGTH + 1];
    char init_commands_path_buffer[PATH_LENGTH + 1];
    const char *init_path = conf;
    const char *init_theme_path = init_theme_path_buffer;
    const char *init_commands_path = init_commands_path_buffer;
    int init_profile_status;
    int init_theme_status;
    int init_commands_status;

    if (GetDefaultSurfacePath(CONFIG_SURFACE_THEME, init_theme_path_buffer,
                              sizeof(init_theme_path_buffer)) != 0) {
      fprintf(stderr,
              "Cannot resolve target themes path. Set HOME before --init.\n");
      exit(1);
    }
    if (GetDefaultSurfacePath(CONFIG_SURFACE_COMMANDS,
                              init_commands_path_buffer,
                              sizeof(init_commands_path_buffer)) != 0) {
      fprintf(stderr,
              "Cannot resolve target commands path. Set HOME before --init.\n");
      exit(1);
    }

    if (!init_path) {
      if (GetDefaultSurfacePath(CONFIG_SURFACE_PROFILE, init_path_buffer,
                                sizeof(init_path_buffer)) != 0) {
        fprintf(
            stderr,
            "Cannot resolve target profile path. Set HOME or pass -p <file>.\n");
        exit(1);
      }
      init_path = init_path_buffer;
    }
    if (!init_path) {
      fprintf(stderr,
              "Cannot resolve target profile path. Set HOME or pass -p <file>.\n");
      exit(1);
    }

    init_profile_status = InitDefaultFile(init_path, default_profile_template);
    if (init_profile_status == -1) {
      fprintf(stderr, "Failed to initialize profile %s: %s\n", init_path,
              strerror(errno));
      exit(1);
    }
    init_theme_status = InitDefaultFile(init_theme_path, default_theme_catalog);
    if (init_theme_status == -1) {
      fprintf(stderr, "Failed to initialize themes %s: %s\n", init_theme_path,
              strerror(errno));
      exit(1);
    }
    init_commands_status =
        InitDefaultFile(init_commands_path, default_commands_catalog);
    if (init_commands_status == -1) {
      fprintf(stderr, "Failed to initialize commands %s: %s\n",
              init_commands_path, strerror(errno));
      exit(1);
    }

    if (init_profile_status == 0)
      fprintf(stdout, "Created profile: %s\n", init_path);
    else
      fprintf(stdout, "%s already exists; not overwritten\n", init_path);
    if (init_commands_status == 0)
      fprintf(stdout, "Created commands: %s\n", init_commands_path);
    else
      fprintf(stdout, "%s already exists; not overwritten\n",
              init_commands_path);

    if (init_theme_status == 0)
      fprintf(stdout, "Created themes: %s\n", init_theme_path);
    else
      fprintf(stdout, "%s already exists; not overwritten\n", init_theme_path);
    return 0;
  }

  if (ctx.core_main_ops.init(&ctx, conf, hist)) {
    fprintf(stderr, "EXIT: Init failed\n");
    exit(1);
  }
  if (!CoreMainOpsReady(&ctx.core_main_ops)) {
    fprintf(stderr, "EXIT: CoreMainOps registration lost\n");
    exit(1);
  }

  /* Pass 1.5: Post-Init Option Parsing (-d, -f) */
  /* Process overrides that must happen after Init */
  for (argi = 1; argi < argc; argi++) {
    if (argv[argi][0] == '-') {
      switch (argv[argi][1]) {
      case 'p':
      case 'P':
      case 'h':
      case 'H':
        /* Skip already processed options */
        if (argv[argi][2] <= ' ')
          argi++;
        break;
      case 'd':
      case 'D': {
        char *d_arg = NULL;
        if (argv[argi][2] <= ' ') {
          if (argi + 1 < argc) {
            d_arg = argv[++argi];
          }
        } else {
          d_arg = argv[argi] + 2;
        }

        if (d_arg) {
          if (strcasecmp(d_arg, "all") == 0 || strcasecmp(d_arg, "max") == 0) {
            ctx.core_main_ops.set_profile_value(&ctx, "TREEDEPTH", "100");
          } else if (strcasecmp(d_arg, "min") == 0 ||
                     strcasecmp(d_arg, "root") == 0) {
            ctx.core_main_ops.set_profile_value(&ctx, "TREEDEPTH", "0");
          } else {
            ctx.core_main_ops.set_profile_value(&ctx, "TREEDEPTH", d_arg);
          }
        }
      } break;
      case 'f':
      case 'F': {
        if (argv[argi][2] <= ' ') {
          if (argi + 1 < argc) {
            filter_arg = argv[++argi];
          }
        } else {
          filter_arg = argv[argi] + 2;
        }
      } break;
      }
    }
  }

  /* Allocate memory for path indexes to support multiple volumes */
  path_indexes = (int *)malloc(sizeof(int) * argc);
  if (!path_indexes) {
    ctx.core_main_ops.shutdown_curses(&ctx);
    fprintf(stderr, "Memory allocation failed\n");
    exit(1);
  }

  /* Pass 2: Path Collection Loop */
  for (argi = 1; argi < argc; argi++) {
    if (argv[argi][0] == '-') {
      /* Skip flags and their values to ensure we only collect positional args
       */
      char c = argv[argi][1];
      if ((c == 'p' || c == 'P' || c == 'h' || c == 'H' || c == 'd' ||
           c == 'D' || c == 'f' || c == 'F') &&
          argv[argi][2] <= ' ') {
        argi++;
      }
      continue;
    }
    path_indexes[path_count++] = argi;
  }

  /* Processing Paths or Default */
  if (path_count == 0) {
    char cwd_path[PATH_LENGTH + 1];

    /* Case 0: No paths provided, default to current working directory */
    if (getcwd(cwd_path, sizeof(cwd_path)) == NULL) {
      ctx.core_main_ops.shutdown_curses(&ctx);
      fprintf(stderr, "Error: getcwd failed: %s\n", strerror(errno));
      free(path_indexes);
      exit(1);
    }

    /* Use LogDisk (wrapper around Volume_Load) to load the initial path */
    if (ctx.core_main_ops.log_disk(&ctx, ctx.left, cwd_path) == -1) {
      ctx.core_main_ops.shutdown_curses(&ctx);
      /* If defaulting to CWD fails, it's a fatal error */
      fprintf(stderr, "EXIT: LogDisk failed for CWD\n");
      free(path_indexes);
      exit(1);
    }
  } else {
    for (int i = path_count - 1; i >= 0; i--) {
      /* LogDisk returns -1 on failure but handles its own error messaging via
       * UI. We proceed to try loading the other requested volumes. */
      ctx.core_main_ops.log_disk(&ctx, ctx.left, argv[path_indexes[i]]);
    }
  }

  free(path_indexes);

  /* Ensure we have at least one active volume before entering main loop */
  if (ctx.active->vol == NULL || ctx.active->vol->vol_stats.tree == NULL) {
    ctx.core_main_ops.shutdown_curses(&ctx);
    fprintf(stderr, "EXIT: No active volume\n");
    exit(1);
  }

  /* Apply command line filter if provided */
  if (filter_arg) {
    /* Safe copy with truncation */
    strncpy(ctx.active->vol->vol_stats.file_spec, filter_arg, FILE_SPEC_LENGTH);
    ctx.active->vol->vol_stats.file_spec[FILE_SPEC_LENGTH] = '\0';

    ctx.core_main_ops.set_filter(ctx.active->vol->vol_stats.file_spec,
                                 &ctx.active->vol->vol_stats);
    ctx.core_main_ops.recalculate_sys_stats(&ctx, &ctx.active->vol->vol_stats);
  }

  /* Main application loop */
  DEBUG_LOG("STARTING MAIN LOOP: ctx.active->vol=%p", (void *)ctx.active->vol);
  if (ctx.active->vol) {
    DEBUG_LOG("STARTING MAIN LOOP: tree=%p",
              (void *)ctx.active->vol->vol_stats.tree);
  }

  /* Main application loop */

  while (1) {
    if (ctx.active == NULL || ctx.active->vol == NULL ||
        ctx.active->vol->vol_stats.tree == NULL) {
      break;
    }
    DEBUG_LOG("Calling HandleDirWindow...");
    int main_loop_exit_char = ctx.core_main_ops.handle_dir_window(
        &ctx, ctx.active->vol->vol_stats.tree);
    DEBUG_LOG("HandleDirWindow returned %d", main_loop_exit_char);
    if (main_loop_exit_char == 'q' || main_loop_exit_char == 'Q') {
      /* User requested to quit. Break the loop to proceed with cleanup. */
      break;
    }
    /* Also break if shutdown flag was set by SIGINT handler but not caught
     * inside HandleDirWindow yet */
    if (ytnova_shutdown_flag) {
      break;
    }
  }

  /* Explicit cleanup */
  ctx.core_main_ops.suspend_clock(
      &ctx); /* Stop SIGALRM (now no-op but kept for API consistency) before
                touching curses/memory */

  attrset(0);  /* Reset attributes */
  clear();     /* Clear internal buffer */
  refresh();   /* Push clear to screen */
  curs_set(1); /* Restore visible cursor */
  ctx.core_main_ops.shutdown_curses(&ctx);

  ctx.core_main_ops.volume_free_all(&ctx); /* Explicitly free memory */

  return 0;
}
