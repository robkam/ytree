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

const AppStateActionTransitionMetadata *
AppStateActionTransitionLookup(YtreeAction action);
size_t AppStateActionTransitionCount(void);
const AppStateTransitionMetadata *
AppStateTransitionLookup(const char *transition_id);
const AppStateTransitionMetadata *AppStateTransitionAt(size_t index);
size_t AppStateTransitionCount(void);

#endif /* YTREE_APPSTATE_ACTIONS_H */
