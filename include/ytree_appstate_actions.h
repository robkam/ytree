/***************************************************************************
 *
 * ytree_appstate_actions.h
 * Runtime AppState transition metadata for canonical actions.
 *
 ***************************************************************************/

#ifndef YTREE_APPSTATE_ACTIONS_H
#define YTREE_APPSTATE_ACTIONS_H

#include "ytree_defs.h"
#include <stddef.h>

typedef struct {
  YtreeAction action;
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

const AppStateActionTransitionMetadata *
AppStateActionTransitionLookup(YtreeAction action);
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

#endif /* YTREE_APPSTATE_ACTIONS_H */
