/***************************************************************************
 *
 * ytree_split_transition.h
 * Split-panel transition owner API.
 *
 ***************************************************************************/

#ifndef YTREE_SPLIT_TRANSITION_H
#define YTREE_SPLIT_TRANSITION_H

#include "ytree_ui.h"

BOOL SplitTransition_HandleFileWindowAction(
    ViewContext *ctx, YtreeAction action, DirEntry *dir_entry,
    YtreePanel *owner_panel, BOOL *switched_panel_ptr,
    YtreeAction *loop_action_ptr, BOOL *return_esc_ptr);
BOOL SplitTransition_HandleDirWindowAction(
    ViewContext *ctx, YtreeAction action, DirEntry **dir_entry_ptr,
    Statistic **s_ptr, const struct Volume **start_vol_ptr,
    BOOL *need_dsp_help_ptr, int *ch_ptr, int *unput_char_ptr);

#endif /* YTREE_SPLIT_TRANSITION_H */
