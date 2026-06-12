/***************************************************************************
 *
 * src/core/main.c
 * Main module
 *
 ***************************************************************************/

#include "ytnova_defs.h"
#include "ytnova_appstate_actions.h"
#include "default_profile_template.h"
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
  {"ACTION_EDIT_CONFIG", ACTION_EDIT_CONFIG},
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

static const char *const kAppStateRequiredDispatchSurfaceIds[] = {
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

static int AppStateFallbackPreconditionValid(const char *precondition) {
  if (precondition == NULL)
    return 1;
  if (!NonEmptyString(precondition))
    return 0;
  return strcmp(precondition, "generation_mismatch") == 0 ||
         strcmp(precondition, "stale_snapshot") == 0;
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
  }
  if (step->stimulus_event_id != NULL) {
    const char *event_transition =
        AppStateEventTransitionLookup(step->stimulus_event_id);

    if (event_transition == NULL || strcmp(event_transition, step->transition_id) != 0)
      return 0;
  }

  for (ref_index = 0; ref_index < step->invariant_id_count; ref_index++) {
    if (AppStateInvariantLookup(step->invariant_ids[ref_index]) == NULL)
      return 0;
    if (StringListContains(step->invariant_ids, ref_index,
                           step->invariant_ids[ref_index]))
      return 0;
  }
  for (ref_index = 0; ref_index < step->diff_harness_id_count; ref_index++) {
    if (AppStateDiffHarnessLookup(step->diff_harness_ids[ref_index]) == NULL)
      return 0;
    if (StringListContains(step->diff_harness_ids, ref_index,
                           step->diff_harness_ids[ref_index]))
      return 0;
  }
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

  if (strcmp(step->expected_result, "blocked") == 0 ||
      strcmp(step->expected_result, "invalid") == 0) {
    if (step->no_unrelated_mutation == NULL)
      return 0;
  }
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

  if (AppStateTransitionSequenceCount() == 0)
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

static int AppStateOwnerFieldsReady(void) {
  size_t index;

  if (AppStateOwnerFieldCount() == 0)
    return 0;

  for (index = 0; index < AppStateOwnerFieldCount(); index++) {
    const AppStateOwnerFieldMetadata *metadata = AppStateOwnerFieldAt(index);

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

static int AppStateTransitionRegistryReady(void) {
  size_t index;

  if (AppStateTransitionCount() == 0)
    return 0;

  for (index = 0; index < AppStateTransitionCount(); index++) {
    const AppStateTransitionMetadata *metadata = AppStateTransitionAt(index);
    size_t write_index;

    if (metadata == NULL || !NonEmptyString(metadata->id) ||
        !NonEmptyString(metadata->category) ||
        !NonEmptyString(metadata->owner) ||
        metadata->declared_write_set == NULL ||
        metadata->declared_write_set_count == 0)
      return 0;
    if (AppStateTransitionLookup(metadata->id) != metadata)
      return 0;

    for (write_index = 0; write_index < metadata->declared_write_set_count;
         write_index++) {
      const char *field = metadata->declared_write_set[write_index];

      if (!NonEmptyString(field))
        return 0;
    }
  }

  if (AppStateTransitionLookup("transition.__ytnova_unknown__") != NULL)
    return 0;

  return 1;
}

static int AppStateGenerationDomainsReady(void) {
  size_t index;

  if (AppStateGenerationDomainCount() == 0)
    return 0;

  for (index = 0; index < AppStateGenerationDomainCount(); index++) {
    const AppStateGenerationDomainMetadata *metadata =
        AppStateGenerationDomainAt(index);
    size_t field_index;
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
        !NonEmptyStringList(metadata->advances_on_transition_ids,
                            metadata->advances_on_transition_id_count) ||
        !NonEmptyStringList(metadata->migration_notes,
                            metadata->migration_note_count))
      return 0;
    if (AppStateGenerationDomainLookup(metadata->domain_id) != metadata)
      return 0;
    if (AppStateOwnerFieldLookup(metadata->generation_owner_field) == NULL)
      return 0;

    for (field_index = 0; field_index < metadata->identity_field_count;
         field_index++) {
      if (AppStateOwnerFieldLookup(metadata->identity_fields[field_index]) ==
          NULL)
        return 0;
    }
    for (transition_index = 0;
         transition_index < metadata->advances_on_transition_id_count;
         transition_index++) {
      if (AppStateTransitionLookup(
              metadata->advances_on_transition_ids[transition_index]) == NULL)
        return 0;
    }
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

static int AppStateDispatchSurfaceWritesReady(
    const AppStateDispatchSurfaceMetadata *metadata) {
  if (metadata->allowed_direct_write_count == 0)
    return metadata->allowed_direct_writes == NULL;

  return NonEmptyStringList(metadata->allowed_direct_writes,
                            metadata->allowed_direct_write_count);
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

  if (AppStateInvariantCount() == 0)
    return 0;

  for (index = 0; index < AppStateInvariantCount(); index++) {
    const AppStateInvariantMetadata *metadata = AppStateInvariantAt(index);
    size_t transition_index;

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

static int AppStateCompatibilityShimsReady(void) {
  size_t index;

  if (AppStateCompatibilityShimCount() == 0)
    return 0;

  for (index = 0; index < AppStateCompatibilityShimCount(); index++) {
    const AppStateCompatibilityShimMetadata *metadata =
        AppStateCompatibilityShimAt(index);
    size_t invariant_index;

    if (metadata == NULL || !NonEmptyString(metadata->id) ||
        !NonEmptyString(metadata->owner) ||
        !NonEmptyString(metadata->old_authority_path) ||
        !NonEmptyString(metadata->read_permission) ||
        !NonEmptyString(metadata->write_permission) ||
        metadata->invariant_checks == NULL ||
        metadata->invariant_check_count == 0 ||
        !NonEmptyString(metadata->removal_trigger) ||
        !NonEmptyString(metadata->target_transition) ||
        !NonEmptyString(metadata->follow_up_task) ||
        !NonEmptyString(metadata->qa_enforcement))
      return 0;
    if (AppStateCompatibilityShimLookup(metadata->id) != metadata)
      return 0;
    if (AppStateTransitionLookup(metadata->target_transition) == NULL)
      return 0;

    for (invariant_index = 0;
         invariant_index < metadata->invariant_check_count; invariant_index++) {
      if (!NonEmptyString(metadata->invariant_checks[invariant_index]))
        return 0;
    }
  }

  if (AppStateCompatibilityShimAt(AppStateCompatibilityShimCount()) != NULL)
    return 0;
  if (AppStateCompatibilityShimLookup(NULL) != NULL)
    return 0;
  if (AppStateCompatibilityShimLookup("") != NULL)
    return 0;
  if (AppStateCompatibilityShimLookup("shim.__ytnova_unknown__") != NULL)
    return 0;

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
        !NonEmptyStringList(coverage->trigger_paths,
                            coverage->trigger_path_count) ||
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

static int AppStateDiffHarnessRegistryReady(void) {
  size_t index;

  if (AppStateDiffHarnessCount() == 0)
    return 0;

  for (index = 0; index < AppStateDiffHarnessCount(); index++) {
    const AppStateDiffHarnessMetadata *metadata =
        AppStateDiffHarnessAt(index);
    size_t ref_index;

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

    for (ref_index = 0; ref_index < metadata->transition_id_count;
         ref_index++) {
      if (AppStateTransitionLookup(metadata->transition_ids[ref_index]) ==
          NULL)
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
    for (ref_index = 0; ref_index < metadata->generation_domain_id_count;
         ref_index++) {
      if (AppStateGenerationDomainLookup(
              metadata->generation_domain_ids[ref_index]) == NULL)
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

static int GetDefaultProfilePath(char *path, size_t path_size) {
  const char *home = getenv("HOME");
  int written;

  if (!path || path_size == 0 || !home || !*home)
    return -1;

  written = snprintf(path, path_size, "%s%c%s", home, FILE_SEPARATOR_CHAR,
                     PROFILE_FILENAME);
  if (written < 0 || (size_t)written >= path_size) {
    return -1;
  }
  return 0;
}

/*
 * Return values:
 *   0 = profile created
 *   1 = profile already exists (left untouched)
 *  -1 = hard error
 */
static int InitProfileFile(const char *path) {
  int fd;
  FILE *fp;
  size_t len;
  size_t written;

  if (!path || !*path)
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

  len = strlen(default_profile_template);
  written = fwrite(default_profile_template, 1, len, fp);
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
      !AppStateCompatibilityShimsReady() ||
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
    const char *init_path = conf;
    int init_status;

    if (!init_path) {
      if (GetDefaultProfilePath(init_path_buffer, sizeof(init_path_buffer)) !=
          0) {
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

    init_status = InitProfileFile(init_path);
    if (init_status == 0) {
      fprintf(stdout, "Created profile: %s\n", init_path);
      return 0;
    }
    if (init_status == 1) {
      fprintf(stdout, "%s already exists; not overwritten\n", init_path);
      return 0;
    }

    fprintf(stderr, "Failed to initialize profile %s: %s\n", init_path,
            strerror(errno));
    exit(1);
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
