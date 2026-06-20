/***************************************************************************
 *
 * ytnova_appstate_actions.h
 * Runtime AppState transition metadata for canonical actions.
 *
 ***************************************************************************/

#ifndef YTNOVA_APPSTATE_ACTIONS_H
#define YTNOVA_APPSTATE_ACTIONS_H

#include "ytnova_defs.h"
#include <stddef.h>

typedef struct {
  YtreeNovaAction action;
  const char *transition_id;
  const char *category;
} AppStateActionTransitionMetadata;

typedef struct {
  YtreeNovaAction action;
  const char *action_name;
  const char *transition_id;
  const char *category;
  const char *owner;
  const char *const *declared_write_set;
  size_t declared_write_set_count;
  const char *const *owner_field_refs;
  size_t owner_field_ref_count;
  const char *const *transition_sequence_refs;
  size_t transition_sequence_ref_count;
  const char *const *dispatch_surface_refs;
  size_t dispatch_surface_ref_count;
  const char *const *invariant_refs;
  size_t invariant_ref_count;
  const char *const *generation_domain_refs;
  size_t generation_domain_ref_count;
  const char *const *diff_harness_refs;
  size_t diff_harness_ref_count;
  const char *boundary_status;
  const char *const *migration_notes;
  size_t migration_note_count;
} AppStateActionCoverageMetadata;

typedef struct {
  const char *event_id;
  const char *event_class;
  const char *transition_id;
  const char *category;
  const char *source;
  const char *owner;
  const char *const *declared_write_set;
  size_t declared_write_set_count;
  const char *const *owner_field_refs;
  size_t owner_field_ref_count;
  const char *boundary_status;
  const char *const *trigger_paths;
  size_t trigger_path_count;
  const char *const *transition_sequence_refs;
  size_t transition_sequence_ref_count;
  const char *const *dispatch_surface_refs;
  size_t dispatch_surface_ref_count;
  const char *const *invariant_refs;
  size_t invariant_ref_count;
  const char *const *generation_domain_refs;
  size_t generation_domain_ref_count;
  const char *const *diff_harness_refs;
  size_t diff_harness_ref_count;
  const char *const *migration_notes;
  size_t migration_note_count;
} AppStateEventCoverageMetadata;

typedef struct {
  const char *id;
  const char *category;
  const char *source_state;
  const char *event;
  const char *guard;
  const char *allowed_result;
  const char *blocked_result;
  const char *target_state;
  const char *owner;
  const char *generation_effect;
  const char *const *side_effects;
  size_t side_effect_count;
  const char *render_invalidation;
  const char *boundary_status;
  const char *notes_follow_up;
  const char *const *declared_write_set;
  size_t declared_write_set_count;
} AppStateTransitionMetadata;

typedef struct {
  const char *surface_id;
  const char *category;
  const char *source_path;
  const char *entry_symbol_or_path;
  const char *transition_id;
  const char *boundary_status;
  const char *const *allowed_direct_writes;
  size_t allowed_direct_write_count;
  const char *const *transition_sequence_refs;
  size_t transition_sequence_ref_count;
  const char *const *migration_notes;
  size_t migration_note_count;
} AppStateDispatchSurfaceMetadata;

typedef struct {
  const char *id;
  const char *owner;
  const char *old_authority_path;
  const char *read_permission;
  const char *write_permission;
  const char *write_capability;
  const char *const *invariant_checks;
  size_t invariant_check_count;
  const char *const *owner_field_refs;
  size_t owner_field_ref_count;
  const char *const *generation_domain_refs;
  size_t generation_domain_ref_count;
  const char *const *diff_harness_refs;
  size_t diff_harness_ref_count;
  const char *removal_trigger;
  const char *target_transition;
  const char *follow_up_task;
  const char *qa_enforcement;
} AppStateCompatibilityShimMetadata;

typedef struct {
  const char *invariant_id;
  const char *category;
  const char *owner_region;
  const char *const *protected_fields;
  size_t protected_field_count;
  const char *const *transition_ids;
  size_t transition_id_count;
  const char *const *dispatch_surface_ids;
  size_t dispatch_surface_id_count;
  const char *failure_mode;
  const char *enforcement_status;
  const char *test_strategy;
  const char *const *migration_notes;
  size_t migration_note_count;
} AppStateInvariantMetadata;

typedef struct {
  const char *field;
  const char *owner_region;
  const char *canonical_owner;
  const char *runtime_carrier;
  const char *mutation_rule;
  const char *migration_status;
  const char *const *invariant_checks;
  size_t invariant_check_count;
} AppStateOwnerFieldMetadata;

typedef struct {
  const char *domain_id;
  const char *category;
  const char *owner_region;
  const char *generation_owner_field;
  const char *const *identity_fields;
  size_t identity_field_count;
  const char *const *coverage_transition_ids;
  size_t coverage_transition_id_count;
  const char *const *advances_on_transition_ids;
  size_t advances_on_transition_id_count;
  const char *stale_snapshot_policy;
  const char *fail_closed_fallback;
  const char *restore_boundary;
  const char *enforcement_status;
  const char *const *migration_notes;
  size_t migration_note_count;
} AppStateGenerationDomainMetadata;

typedef struct {
  const char *harness_id;
  const char *check_category;
  const char *const *snapshot_phases;
  size_t snapshot_phase_count;
  const char *const *snapshot_regions;
  size_t snapshot_region_count;
  const char *const *transition_ids;
  size_t transition_id_count;
  const char *const *owner_field_refs;
  size_t owner_field_ref_count;
  const char *const *invariant_ids;
  size_t invariant_id_count;
  const char *const *generation_domain_ids;
  size_t generation_domain_id_count;
  const char *expected_behavior;
  const char *failure_mode;
  const char *enforcement_status;
  const char *const *migration_notes;
  size_t migration_note_count;
} AppStateDiffHarnessMetadata;

typedef struct {
  const char *domain_id;
  const char *expectation;
} AppStateTransitionSequenceGenerationExpectationMetadata;

typedef struct {
  const char *diff_harness_id;
  const char *expectation;
} AppStateTransitionSequenceNoUnrelatedMutationMetadata;

typedef struct {
  const char *outcome;
  const char *allowed_mutation_scope;
} AppStateTransitionSequenceDeterministicFallbackMetadata;

typedef struct {
  size_t ordinal;
  const char *step_id;
  const char *transition_id;
  const char *stimulus_action_id;
  const char *stimulus_event_id;
  const char *const *action_coverage_refs;
  size_t action_coverage_ref_count;
  const char *const *event_coverage_refs;
  size_t event_coverage_ref_count;
  const char *expected_result;
  const char *const *invariant_ids;
  size_t invariant_id_count;
  const char *const *diff_harness_ids;
  size_t diff_harness_id_count;
  const AppStateTransitionSequenceGenerationExpectationMetadata
      *generation_domain_expectations;
  size_t generation_domain_expectation_count;
  const AppStateTransitionSequenceNoUnrelatedMutationMetadata
      *no_unrelated_mutation;
  const char *precondition;
  const AppStateTransitionSequenceDeterministicFallbackMetadata
      *deterministic_fallback;
} AppStateTransitionSequenceStepMetadata;

typedef struct {
  const char *scenario_id;
  const char *category;
  const char *flow;
  const char *description;
  const AppStateTransitionSequenceStepMetadata *steps;
  size_t step_count;
} AppStateTransitionSequenceMetadata;

const AppStateActionTransitionMetadata *
AppStateActionTransitionLookup(YtreeNovaAction action);
size_t AppStateActionTransitionCount(void);
const AppStateActionCoverageMetadata *
AppStateActionCoverageLookup(YtreeNovaAction action);
YtreeNovaAction AppStateValidatedKeyAction(YtreeNovaAction action);
const AppStateActionCoverageMetadata *AppStateActionCoverageAt(size_t index);
size_t AppStateActionCoverageCount(void);
int AppStateValidatedEvent(const char *event_id);
const AppStateEventCoverageMetadata *
AppStateEventCoverageLookup(const char *event_id);
const AppStateEventCoverageMetadata *AppStateEventCoverageAt(size_t index);
size_t AppStateEventCoverageCount(void);
const AppStateTransitionMetadata *
AppStateTransitionLookup(const char *transition_id);
int AppStateValidatedTransition(const char *transition_id);
const AppStateTransitionMetadata *AppStateTransitionAt(size_t index);
size_t AppStateTransitionCount(void);
const AppStateDispatchSurfaceMetadata *
AppStateDispatchSurfaceLookup(const char *surface_id);
int AppStateValidatedDispatchSurface(const char *surface_id);
const AppStateDispatchSurfaceMetadata *AppStateDispatchSurfaceAt(size_t index);
size_t AppStateDispatchSurfaceCount(void);
const AppStateCompatibilityShimMetadata *
AppStateCompatibilityShimLookup(const char *shim_id);
int AppStateValidatedCompatibilityShim(const char *shim_id);
const AppStateCompatibilityShimMetadata *
AppStateCompatibilityShimAt(size_t index);
size_t AppStateCompatibilityShimCount(void);
const AppStateInvariantMetadata *
AppStateInvariantLookup(const char *invariant_id);
int AppStateValidatedInvariant(const char *invariant_id);
const AppStateInvariantMetadata *AppStateInvariantAt(size_t index);
size_t AppStateInvariantCount(void);
const AppStateOwnerFieldMetadata *AppStateOwnerFieldLookup(const char *field);
int AppStateValidatedOwnerField(const char *field);
const AppStateOwnerFieldMetadata *AppStateOwnerFieldAt(size_t index);
size_t AppStateOwnerFieldCount(void);
const AppStateGenerationDomainMetadata *
AppStateGenerationDomainLookup(const char *domain_id);
int AppStateValidatedGenerationDomain(const char *domain_id);
const AppStateGenerationDomainMetadata *AppStateGenerationDomainAt(size_t index);
size_t AppStateGenerationDomainCount(void);
const AppStateDiffHarnessMetadata *
AppStateDiffHarnessLookup(const char *harness_id);
int AppStateValidatedDiffHarness(const char *harness_id);
const AppStateDiffHarnessMetadata *AppStateDiffHarnessAt(size_t index);
size_t AppStateDiffHarnessCount(void);
const AppStateTransitionSequenceMetadata *
AppStateTransitionSequenceLookup(const char *scenario_id);
int AppStateValidatedTransitionSequence(const char *scenario_id);
const AppStateTransitionSequenceMetadata *
AppStateTransitionSequenceAt(size_t index);
size_t AppStateTransitionSequenceCount(void);

#endif /* YTNOVA_APPSTATE_ACTIONS_H */
