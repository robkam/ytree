/***************************************************************************
 *
 * ytnova_split_transition.h
 * Split-panel transition owner API.
 *
 ***************************************************************************/

#ifndef YTNOVA_SPLIT_TRANSITION_H
#define YTNOVA_SPLIT_TRANSITION_H

#include "ytnova_ui.h"

BOOL SplitTransition_HandleFileWindowAction(
    ViewContext *ctx, YtreeNovaAction action, DirEntry *dir_entry,
    YtreeNovaPanel *owner_panel, BOOL *switched_panel_ptr,
    YtreeNovaAction *loop_action_ptr, BOOL *return_esc_ptr);
BOOL SplitTransition_HandleDirWindowAction(
    ViewContext *ctx, YtreeNovaAction action, DirEntry **dir_entry_ptr,
    Statistic **s_ptr, const struct Volume **start_vol_ptr,
    BOOL *need_dsp_help_ptr, const int *ch_ptr, int *unput_char_ptr);

#endif /* YTNOVA_SPLIT_TRANSITION_H */
