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

const AppStateActionTransitionMetadata *
AppStateActionTransitionLookup(YtreeAction action);
size_t AppStateActionTransitionCount(void);

#endif /* YTREE_APPSTATE_ACTIONS_H */
