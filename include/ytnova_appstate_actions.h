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
  const char *id;
  const char *category;
  const char *owner;
  const char *const *declared_write_set;
  size_t declared_write_set_count;
} AppStateTransitionMetadata;

typedef struct {
  const char *id;
  const char *owner;
  const char *old_authority_path;
  const char *read_permission;
  const char *write_permission;
  const char *const *invariant_checks;
  size_t invariant_check_count;
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

const AppStateActionTransitionMetadata *
AppStateActionTransitionLookup(YtreeNovaAction action);
size_t AppStateActionTransitionCount(void);
const AppStateTransitionMetadata *
AppStateTransitionLookup(const char *transition_id);
const AppStateTransitionMetadata *AppStateTransitionAt(size_t index);
size_t AppStateTransitionCount(void);
const AppStateCompatibilityShimMetadata *
AppStateCompatibilityShimLookup(const char *shim_id);
const AppStateCompatibilityShimMetadata *
AppStateCompatibilityShimAt(size_t index);
size_t AppStateCompatibilityShimCount(void);
const AppStateInvariantMetadata *
AppStateInvariantLookup(const char *invariant_id);
const AppStateInvariantMetadata *AppStateInvariantAt(size_t index);
size_t AppStateInvariantCount(void);

#endif /* YTNOVA_APPSTATE_ACTIONS_H */
