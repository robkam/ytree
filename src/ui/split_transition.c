/***************************************************************************
 *
 * src/ui/split_transition.c
 * Split-panel transition owner path (F8 / Tab).
 *
 ***************************************************************************/

#define NO_YTNOVA_MACROS

#include "ytnova_appstate_actions.h"
#include "ytnova_appstate_focus.h"
#include "ytnova_fs.h"
#include "ytnova_panel_anchor.h"
#include "ytnova_split_transition.h"
#include "ytnova_ui.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static BOOL SplitTransitionActionBoundaryIsValid(YtreeNovaAction action) {
  const AppStateActionTransitionMetadata *action_metadata;
  const AppStateTransitionMetadata *transition_metadata;

  if (action != ACTION_SPLIT_SCREEN && action != ACTION_SWITCH_PANEL)
    return FALSE;

  action_metadata = AppStateActionTransitionLookup(action);
  if (!action_metadata || action_metadata->action != action ||
      !action_metadata->transition_id ||
      action_metadata->transition_id[0] == '\0' || !action_metadata->category ||
      action_metadata->category[0] == '\0')
    return FALSE;

  transition_metadata =
      AppStateTransitionLookup(action_metadata->transition_id);
  if (!transition_metadata || !transition_metadata->category ||
      strcmp(action_metadata->category, transition_metadata->category) != 0)
    return FALSE;

  if (!AppStateValidatedTransition(action_metadata->transition_id))
    return FALSE;

  return TRUE;
}

#ifndef NDEBUG
typedef struct {
  int cursor_pos;
  int disp_begin_pos;
  int start_file;
  int file_cursor_pos;
  const DirEntry *file_dir_entry;
  ViewFocus saved_focus;
  BOOL saved_big_file_view;
  BOOL hide_dot_files;
  char file_selection_name[PATH_LENGTH + 1];
  char file_selection_dir_path[PATH_LENGTH + 1];
} SplitFilePanelSnapshot;

typedef struct {
  int cursor_pos;
  int disp_begin_pos;
  int start_file;
  int file_cursor_pos;
  const DirEntry *file_dir_entry;
  ViewFocus saved_focus;
  BOOL saved_big_file_view;
  BOOL hide_dot_files;
  char file_selection_name[PATH_LENGTH + 1];
  char file_selection_dir_path[PATH_LENGTH + 1];
} SplitDirPanelSnapshot;

static void CaptureSplitFilePanelSnapshot(const YtreeNovaPanel *panel,
                                          SplitFilePanelSnapshot *snapshot) {
  if (!panel || !snapshot)
    return;

  snapshot->cursor_pos = panel->cursor_pos;
  snapshot->disp_begin_pos = panel->disp_begin_pos;
  snapshot->start_file = panel->start_file;
  snapshot->file_cursor_pos = panel->file_cursor_pos;
  snapshot->file_dir_entry = panel->file_dir_entry;
  snapshot->saved_focus = panel->saved_focus;
  snapshot->saved_big_file_view = panel->saved_big_file_view;
  snapshot->hide_dot_files = panel->hide_dot_files;
  (void)snprintf(snapshot->file_selection_name,
                 sizeof(snapshot->file_selection_name), "%s",
                 panel->file_selection_name);
  (void)snprintf(snapshot->file_selection_dir_path,
                 sizeof(snapshot->file_selection_dir_path), "%s",
                 panel->file_selection_dir_path);
}

static void AssertSplitFilePanelSnapshotUnchanged(
    const YtreeNovaPanel *panel, const SplitFilePanelSnapshot *snapshot) {
  (void)panel;
  (void)snapshot;
}

static void CaptureSplitDirPanelSnapshot(const YtreeNovaPanel *panel,
                                         SplitDirPanelSnapshot *snapshot) {
  if (!panel || !snapshot)
    return;

  snapshot->cursor_pos = panel->cursor_pos;
  snapshot->disp_begin_pos = panel->disp_begin_pos;
  snapshot->start_file = panel->start_file;
  snapshot->file_cursor_pos = panel->file_cursor_pos;
  snapshot->file_dir_entry = panel->file_dir_entry;
  snapshot->saved_focus = panel->saved_focus;
  snapshot->saved_big_file_view = panel->saved_big_file_view;
  snapshot->hide_dot_files = panel->hide_dot_files;
  (void)snprintf(snapshot->file_selection_name,
                 sizeof(snapshot->file_selection_name), "%s",
                 panel->file_selection_name);
  (void)snprintf(snapshot->file_selection_dir_path,
                 sizeof(snapshot->file_selection_dir_path), "%s",
                 panel->file_selection_dir_path);
}

static void AssertSplitDirPanelFileStateUnchanged(
    const YtreeNovaPanel *panel, const SplitDirPanelSnapshot *snapshot) {
  (void)panel;
  (void)snapshot;
}
#endif

static void SplitTransitionDebugLogFilePanelState(const char *label,
                                                  const YtreeNovaPanel *panel) {
  char tree_path[PATH_LENGTH + 1];
  char file_dir_path[PATH_LENGTH + 1];
  int idx = -1;
  const DirEntry *tree_de = NULL;
  const char *tree_text = "<none>";
  const char *file_dir_text = "<none>";
  const char *selection_dir_text = "<none>";
  const char *selection_name_text = "<none>";

  tree_path[0] = '\0';
  file_dir_path[0] = '\0';

  if (!panel) {
    DEBUG_LOG("FILE_PANEL[%s] <null>", label ? label : "?");
    return;
  }

  if (panel->vol && panel->vol->total_dirs > 0 && panel->vol->dir_entry_list) {
    idx = panel->disp_begin_pos + panel->cursor_pos;
    if (idx < 0)
      idx = 0;
    if (idx >= panel->vol->total_dirs)
      idx = panel->vol->total_dirs - 1;
    tree_de = panel->vol->dir_entry_list[idx].dir_entry;
    if (tree_de) {
      GetPath((DirEntry *)tree_de, tree_path);
      tree_path[PATH_LENGTH] = '\0';
      tree_text = tree_path;
    }
  }

  if (panel->file_dir_entry && panel->vol && panel->vol->dir_entry_list &&
      panel->vol->total_dirs > 0) {
    BOOL in_volume = FALSE;
    int j;
    for (j = 0; j < panel->vol->total_dirs; j++) {
      if (panel->vol->dir_entry_list[j].dir_entry == panel->file_dir_entry) {
        in_volume = TRUE;
        break;
      }
    }
    if (in_volume) {
      GetPath(panel->file_dir_entry, file_dir_path);
      file_dir_path[PATH_LENGTH] = '\0';
      file_dir_text = file_dir_path;
    } else {
      file_dir_text = "<stale>";
    }
  } else if (panel->file_dir_entry) {
    file_dir_text = "<stale>";
  }

  if (panel->file_selection_dir_path[0] != '\0')
    selection_dir_text = panel->file_selection_dir_path;
  if (panel->file_selection_name[0] != '\0')
    selection_name_text = panel->file_selection_name;

  DEBUG_LOG(
      "FILE_PANEL[%s] saved_focus=%d disp=%d cur=%d idx=%d start=%d fcur=%d "
      "tree='%s' file_dir='%s' sel_dir='%s' sel_name='%s'",
      label ? label : "?", panel->saved_focus, panel->disp_begin_pos,
      panel->cursor_pos, idx, panel->start_file, panel->file_cursor_pos,
      tree_text, file_dir_text, selection_dir_text, selection_name_text);
}

static void SplitTransitionDebugLogFileState(const char *label,
                                             const ViewContext *ctx) {
  const char *active_side = "?";

  if (ctx && ctx->active) {
    if (ctx->active == ctx->left)
      active_side = "LEFT";
    else if (ctx->active == ctx->right)
      active_side = "RIGHT";
  }

  DEBUG_LOG("FILE_SPLIT[%s] is_split=%d active=%s focused=%d",
            label ? label : "?",
            ctx ? (int)ctx->is_split_screen : -1, active_side,
            ctx ? (int)ctx->focused_window : -1);
  if (ctx) {
    SplitTransitionDebugLogFilePanelState("LEFT", ctx->left);
    SplitTransitionDebugLogFilePanelState("RIGHT", ctx->right);
  }
}

static void SplitTransitionDebugLogDirState(const char *label,
                                            const ViewContext *ctx) {
  const char *active_side = "?";

  if (ctx && ctx->active) {
    if (ctx->active == ctx->left)
      active_side = "LEFT";
    else if (ctx->active == ctx->right)
      active_side = "RIGHT";
  }

  DEBUG_LOG("SPLIT[%s] is_split=%d active=%s focused=%d", label ? label : "?",
            ctx ? (int)ctx->is_split_screen : -1, active_side,
            ctx ? (int)ctx->focused_window : -1);
}

static BOOL PanelHasVisibleFiles(ViewContext *ctx, YtreeNovaPanel *panel,
                                 DirEntry *dir_entry) {
  if (!ctx || !panel || !dir_entry)
    return FALSE;
  panel->file_dir_entry = dir_entry;
  BuildFileEntryList(ctx, panel);
  return panel->file_count > 0;
}

BOOL SplitTransition_HandleFileWindowAction(ViewContext *ctx, YtreeNovaAction action,
                                            DirEntry *dir_entry,
                                            YtreeNovaPanel *owner_panel,
                                            BOOL *switched_panel_ptr,
                                            YtreeNovaAction *loop_action_ptr,
                                            BOOL *return_esc_ptr) {
  if (!ctx || !dir_entry || !owner_panel || !switched_panel_ptr ||
      !loop_action_ptr || !return_esc_ptr) {
    return FALSE;
  }
  if ((action == ACTION_SPLIT_SCREEN || action == ACTION_SWITCH_PANEL) &&
      !SplitTransitionActionBoundaryIsValid(action))
    return FALSE;

  *return_esc_ptr = FALSE;

  switch (action) {
  case ACTION_SPLIT_SCREEN:
    SplitTransitionDebugLogFileState("FileAction:split:before", ctx);
    owner_panel->saved_big_file_view =
        (dir_entry->big_window || dir_entry->global_flag ||
         dir_entry->tagged_flag);

    if (!ctx->is_split_screen) {
      owner_panel->file_dir_entry = dir_entry;
      owner_panel->start_file = dir_entry->start_file;
      owner_panel->file_cursor_pos = dir_entry->cursor_pos;
    }
    CapturePanelSelectionAnchor(ctx, owner_panel, dir_entry);

    {
      BOOL closing_split = ctx->is_split_screen;
      ViewFocus preserved_focus = ctx->focused_window;
      BOOL donate_active_state =
          closing_split && ctx->active == ctx->right && ctx->left &&
          ctx->right && preserved_focus == FOCUS_FILE;

#ifndef NDEBUG
      const YtreeNovaPanel *target_panel =
          (ctx->active == ctx->left) ? ctx->right : ctx->left;
      const YtreeNovaPanel *stable_panel;
      SplitFilePanelSnapshot stable_panel_snapshot;

      /*
       * Split ownership boundary: validate the panel that must remain stable
       * for this transaction. Opening a split seeds the peer panel by design;
       * closing a split may donate active state to the peer, so the invariant
       * has to follow the branch-owned panel instead of blindly checking the
       * opposite side.
       */
      if (closing_split) {
        if (donate_active_state) {
          stable_panel = owner_panel;
        } else {
          stable_panel = target_panel;
        }
      } else {
        stable_panel = owner_panel;
      }
      CaptureSplitFilePanelSnapshot(stable_panel, &stable_panel_snapshot);
#endif
      if (donate_active_state && ctx->left && ctx->right &&
          !DonatePanelState(ctx, ctx->left, ctx->right))
        return FALSE;

      ctx->is_split_screen = !ctx->is_split_screen;
      if (closing_split)
        ctx->active = ctx->left;
      ReCreateWindows(ctx);
      if (!ctx->is_split_screen)
        SyncActivePanelWindows(ctx);

      if (ctx->is_split_screen) {
        if (ctx->right && ctx->left) {
          ctx->right->vol = ctx->left->vol;
          ctx->right->cursor_pos = ctx->left->cursor_pos;
          ctx->right->disp_begin_pos = ctx->left->disp_begin_pos;
          memcpy(ctx->right->tree_viewport_top_dir_path,
                 ctx->left->tree_viewport_top_dir_path,
                 sizeof(ctx->right->tree_viewport_top_dir_path));
          ctx->right->hide_dot_files = ctx->left->hide_dot_files;
          /*
           * Split ownership boundary: a file-view split must keep the original
           * file panel active and seed the new peer from the same panel-local
           * file cursor snapshot. The new split pane must not inherit tree
           * focus or it will redraw as a tree panel and break per-panel file
           * state on the first Tab.
           */
          if (!AppStateCommitPanelFocus(ctx, ctx->right, FOCUS_FILE))
            return FALSE;
          ctx->right->start_file = ctx->left->start_file;
          ctx->right->file_cursor_pos = ctx->left->file_cursor_pos;
          (void)snprintf(ctx->right->file_selection_name,
                         sizeof(ctx->right->file_selection_name), "%s",
                         ctx->left->file_selection_name);
          (void)snprintf(ctx->right->file_selection_dir_path,
                         sizeof(ctx->right->file_selection_dir_path), "%s",
                         ctx->left->file_selection_dir_path);
          ctx->right->saved_big_file_view = ctx->left->saved_big_file_view;
          PanelTags_Copy(ctx->right, ctx->left);
          FreeFileEntryList(ctx->right);
        }
        ctx->active = owner_panel;
        if (!AppStateCommitPanelFocus(ctx, ctx->active, FOCUS_FILE))
          return FALSE;
      } else {
        FreeFileEntryList(ctx->right);
        ctx->active = ctx->left;
        if (!AppStateCommitPanelFocus(ctx, ctx->active, preserved_focus))
          return FALSE;
        BuildFileEntryList(ctx, ctx->active);
        if (donate_active_state)
          *switched_panel_ptr = TRUE;
        SplitTransitionDebugLogFileState("FileAction:split:after", ctx);
        *loop_action_ptr =
            (preserved_focus == FOCUS_FILE) ? ACTION_NONE : ACTION_ESCAPE;
        *return_esc_ptr = FALSE;
#ifndef NDEBUG
        if (stable_panel)
          AssertSplitFilePanelSnapshotUnchanged(stable_panel,
                                                &stable_panel_snapshot);
#endif
        return TRUE;
      }

      /*
       * Split is a layout change, not a mode exit. Stay inside the file window
       * loop so the active pane keeps receiving file commands after F8.
       */
      *loop_action_ptr = ACTION_NONE;
      *return_esc_ptr = FALSE;
      SplitTransitionDebugLogFileState("FileAction:split:after", ctx);
#ifndef NDEBUG
      if (stable_panel)
        AssertSplitFilePanelSnapshotUnchanged(stable_panel,
                                              &stable_panel_snapshot);
#endif
      return TRUE;
    }

  case ACTION_SWITCH_PANEL:
    if (!ctx->is_split_screen)
      return TRUE;
    SplitTransitionDebugLogFileState("FileAction:switch:before", ctx);
#ifndef NDEBUG
    {
      const YtreeNovaPanel *target_panel =
          (ctx->active == ctx->left) ? ctx->right : ctx->left;
      SplitFilePanelSnapshot target_panel_snapshot;

      CaptureSplitFilePanelSnapshot(target_panel, &target_panel_snapshot);

      owner_panel->file_dir_entry = dir_entry;
      owner_panel->start_file = dir_entry->start_file;
      owner_panel->file_cursor_pos = dir_entry->cursor_pos;
      CapturePanelSelectionAnchor(ctx, owner_panel, dir_entry);
      if (!AppStateCommitPanelFocus(ctx, ctx->active, FOCUS_FILE))
        return FALSE;
      ctx->active->saved_big_file_view =
          (dir_entry->big_window || dir_entry->global_flag ||
           dir_entry->tagged_flag);
      *switched_panel_ptr = TRUE;
      SwitchToSmallFileWindow(ctx);

      if (ctx->active == ctx->left) {
        ctx->active = ctx->right;
      } else {
        ctx->active = ctx->left;
      }
      if (!AppStateMirrorActivePanelFocus(ctx))
        return FALSE;
      *loop_action_ptr = ACTION_NONE;
      AssertSplitFilePanelSnapshotUnchanged(target_panel,
                                            &target_panel_snapshot);
    }
#else
    owner_panel->file_dir_entry = dir_entry;
    owner_panel->start_file = dir_entry->start_file;
    owner_panel->file_cursor_pos = dir_entry->cursor_pos;
    CapturePanelSelectionAnchor(ctx, owner_panel, dir_entry);
    if (!AppStateCommitPanelFocus(ctx, ctx->active, FOCUS_FILE))
      return FALSE;
    ctx->active->saved_big_file_view =
        (dir_entry->big_window || dir_entry->global_flag ||
         dir_entry->tagged_flag);
    *switched_panel_ptr = TRUE;
    SwitchToSmallFileWindow(ctx);

    if (ctx->active == ctx->left) {
      ctx->active = ctx->right;
    } else {
      ctx->active = ctx->left;
    }
    if (!AppStateMirrorActivePanelFocus(ctx))
      return FALSE;
    *loop_action_ptr = ACTION_NONE;
#endif
    SplitTransitionDebugLogFileState("FileAction:switch:after", ctx);
    return TRUE;

  default:
    return FALSE;
  }
}

BOOL SplitTransition_HandleDirWindowAction(ViewContext *ctx, YtreeNovaAction action,
                                           DirEntry **dir_entry_ptr,
                                           Statistic **s_ptr,
                                           const struct Volume **start_vol_ptr,
                                           BOOL *need_dsp_help_ptr,
                                           const int *ch_ptr,
                                           int *unput_char_ptr) {
  if (!ctx || !ctx->active || !dir_entry_ptr || !*dir_entry_ptr || !s_ptr ||
      !*s_ptr || !start_vol_ptr || !*start_vol_ptr || !need_dsp_help_ptr ||
      !ch_ptr || !unput_char_ptr) {
    return FALSE;
  }
  if ((action == ACTION_SPLIT_SCREEN || action == ACTION_SWITCH_PANEL) &&
      !SplitTransitionActionBoundaryIsValid(action))
    return FALSE;

  switch (action) {
  case ACTION_SPLIT_SCREEN:
    SplitTransitionDebugLogDirState("DirPanelAction:split:before", ctx);
    {
      YtreeNovaPanel *closing_active = ctx->active;
      BOOL closing_split = ctx->is_split_screen;
      BOOL donate_active_state = FALSE;
      BOOL preserve_left_file_state =
          ctx->left && ctx->left->file_selection_dir_path[0] != '\0' &&
          ctx->left->file_selection_name[0] != '\0';
      int source_cursor_pos = ctx->right ? ctx->right->cursor_pos : 0;
      int source_disp_begin_pos = ctx->right ? ctx->right->disp_begin_pos : 0;
      int source_current_dir_entry =
          ctx->right ? ctx->right->current_dir_entry : 0;
      unsigned int source_panel_generation =
          ctx->right ? ctx->right->panel_generation : 0U;
      char left_file_selection_name[PATH_LENGTH + 1];
      char left_file_selection_dir_path[PATH_LENGTH + 1];
      ViewFocus preserved_focus = ctx->focused_window;

      left_file_selection_name[0] = '\0';
      left_file_selection_dir_path[0] = '\0';
      if (preserve_left_file_state) {
        (void)snprintf(left_file_selection_name,
                       sizeof(left_file_selection_name), "%s",
                       ctx->left->file_selection_name);
        (void)snprintf(left_file_selection_dir_path,
                       sizeof(left_file_selection_dir_path), "%s",
                       ctx->left->file_selection_dir_path);
      }

      if (ctx->is_split_screen && ctx->active == ctx->right && ctx->left &&
          ctx->right && !preserve_left_file_state)
        donate_active_state = TRUE;

      if (donate_active_state && ctx->left && ctx->right) {
        if (!DonatePanelState(ctx, ctx->left, ctx->right))
          return FALSE;
        ctx->left->cursor_pos = source_cursor_pos;
        ctx->left->disp_begin_pos = source_disp_begin_pos;
        ctx->left->current_dir_entry = source_current_dir_entry;
        ctx->left->panel_generation = source_panel_generation;
        if (!AppStateCommitPanelFocus(ctx, ctx->left, FOCUS_TREE))
          return FALSE;
      }
      if (preserve_left_file_state && !donate_active_state && ctx->left &&
          ctx->left->vol) {
        const DirEntry *left_dir_entry = GetPanelDirEntry(ctx->left);
        if (left_dir_entry)
          CapturePanelSelectionAnchor(ctx, ctx->left, left_dir_entry);
      }
      if (closing_active &&
          !AppStateCommitPanelFocus(ctx, closing_active, FOCUS_TREE))
        return FALSE;
      ctx->is_split_screen = !ctx->is_split_screen;
      if (closing_split)
        ctx->active = ctx->left;
      ReCreateWindows(ctx);
      if (!ctx->is_split_screen)
        SyncActivePanelWindows(ctx);

      if (ctx->is_split_screen) {
        if (ctx->right && ctx->left) {
          ctx->right->vol = ctx->left->vol;
          ctx->right->cursor_pos = ctx->left->cursor_pos;
          ctx->right->disp_begin_pos = ctx->left->disp_begin_pos;
          memcpy(ctx->right->tree_viewport_top_dir_path,
                 ctx->left->tree_viewport_top_dir_path,
                 sizeof(ctx->right->tree_viewport_top_dir_path));
          ctx->right->start_file = ctx->left->start_file;
          ctx->right->file_cursor_pos = ctx->left->file_cursor_pos;
          ctx->right->saved_big_file_view = ctx->left->saved_big_file_view;
          ctx->right->hide_dot_files = ctx->left->hide_dot_files;
          if (!AppStateCommitPanelFocus(ctx, ctx->right, FOCUS_TREE))
            return FALSE;
          FreeFileEntryList(ctx->right);
        }
      } else {
        FreeFileEntryList(ctx->right);
        ctx->active = ctx->left;
        *dir_entry_ptr = GetPanelDirEntry(ctx->active);
        if (preserve_left_file_state && !donate_active_state &&
            !AppStateCommitPanelFocus(ctx, ctx->active, FOCUS_FILE))
          return FALSE;
        if (ctx->active->saved_focus == FOCUS_FILE) {
          if (ctx->active->file_selection_dir_path[0] != '\0') {
            /*
             * Re-anchor the surviving panel to its saved file-selection path
             * before file restore runs. Without this, restore can rebuild the
             * wrong tree row and fall back to a tree-only view with an empty
             * file list.
             */
            RestorePanelAnchorPath(ctx->active->vol, ctx->active,
                                   ctx->active->file_selection_dir_path);
            *dir_entry_ptr = GetPanelDirEntry(ctx->active);
          }
          *dir_entry_ptr =
              RestorePanelFileSelection(ctx, *dir_entry_ptr, ctx->active);
        }
        *unput_char_ptr = '\0';
        flushinp();
      }

      if (!AppStateCommitPanelFocus(ctx, ctx->active, preserved_focus))
        return FALSE;
      *start_vol_ptr = ctx->active->vol;
      *s_ptr = &ctx->active->vol->vol_stats;
      RefreshView(ctx, *dir_entry_ptr);
      SplitTransitionDebugLogDirState("DirPanelAction:split:after", ctx);
      *need_dsp_help_ptr = TRUE;
      return TRUE;
    }

  case ACTION_SWITCH_PANEL:
    if (!ctx->is_split_screen)
      return TRUE;
    SplitTransitionDebugLogDirState("DirPanelAction:switch:before", ctx);
#ifndef NDEBUG
    {
      const YtreeNovaPanel *previous_active = ctx->active;
      SplitDirPanelSnapshot previous_active_snapshot;

      /*
       * Split ownership boundary (docs/ARCHITECTURE.md §4.2.1):
       * Tab switch may change `ctx->active`/focused window only. The panel
       * becoming inactive must keep its panel-local tree viewport unchanged;
       * file-selection identity can be re-materialized on reactivation.
       */
      CaptureSplitDirPanelSnapshot(previous_active, &previous_active_snapshot);

      if (ctx->active == ctx->left) {
        ctx->active = ctx->right;
      } else {
        ctx->active = ctx->left;
      }

      if (!AppStateMirrorActivePanelFocus(ctx))
        return FALSE;
      *start_vol_ptr = ctx->active->vol;
      *s_ptr = &ctx->active->vol->vol_stats;
      PanelTags_ApplyToTree(ctx, ctx->active);
      DEBUG_LOG("DirPanelAction:switch:post_toggle active_saved_focus=%d",
                ctx->active->saved_focus);

      if (ctx->active->vol->total_dirs > 0) {
        if (ctx->active->disp_begin_pos + ctx->active->cursor_pos >=
            ctx->active->vol->total_dirs) {
          ctx->active->cursor_pos =
              ctx->active->vol->total_dirs - 1 - ctx->active->disp_begin_pos;
        }
        *dir_entry_ptr = ResolveActiveDirEntry(ctx, *s_ptr);
      } else {
        *dir_entry_ptr = (*s_ptr)->tree;
      }

      if (*dir_entry_ptr) {
        char switch_path[PATH_LENGTH + 1];
        GetPath(*dir_entry_ptr, switch_path);
        switch_path[PATH_LENGTH] = '\0';
        DEBUG_LOG("DirPanelAction:switch:resolved_dir='%s'", switch_path);
      } else {
        DEBUG_LOG("DirPanelAction:switch:resolved_dir=<null>");
      }

      DEBUG_LOG("ACTION_SWITCH_PANEL: active panel is now %s with "
                "cursor_pos=%d, dir_entry=%s",
                ctx->active == ctx->left ? "LEFT" : "RIGHT",
                ctx->active->cursor_pos,
                *dir_entry_ptr ? (*dir_entry_ptr)->name : "NULL");

      SyncActivePanelWindows(ctx);
      DEBUG_LOG("DirPanelAction:switch:after_sync_windows");
      if (ctx->active->file_selection_dir_path[0] != '\0') {
        RestorePanelAnchorPath(ctx->active->vol, ctx->active,
                               ctx->active->file_selection_dir_path);
        *dir_entry_ptr = GetPanelDirEntry(ctx->active);
      }
      *dir_entry_ptr =
          RestorePanelFileSelection(ctx, *dir_entry_ptr, ctx->active);
      if (!*dir_entry_ptr && *s_ptr) {
        *dir_entry_ptr = (*s_ptr)->tree;
        DEBUG_LOG("DirPanelAction:switch:restore returned null; fallback tree");
      }
      if (!AppStateMirrorActivePanelFocus(ctx))
        return FALSE;
      if (*dir_entry_ptr) {
        DEBUG_LOG("DirPanelAction:switch:before_refresh dir='%s'",
                  (*dir_entry_ptr)->name ? (*dir_entry_ptr)->name : "<nullname>");
        RefreshView(ctx, *dir_entry_ptr);
        DEBUG_LOG("DirPanelAction:switch:after_refresh");
      } else {
        DEBUG_LOG("DirPanelAction:switch:skip_refresh dir_entry null");
        return TRUE;
      }
      *need_dsp_help_ptr = TRUE;
      if (ctx->focused_window == FOCUS_FILE && *dir_entry_ptr &&
          PanelHasVisibleFiles(ctx, ctx->active, *dir_entry_ptr)) {
        *unput_char_ptr = CR;
      }

      AssertSplitDirPanelFileStateUnchanged(previous_active,
                                            &previous_active_snapshot);
    }
#else
    if (ctx->active == ctx->left) {
      ctx->active = ctx->right;
    } else {
      ctx->active = ctx->left;
    }

    if (!AppStateMirrorActivePanelFocus(ctx))
      return FALSE;
    *start_vol_ptr = ctx->active->vol;
    *s_ptr = &ctx->active->vol->vol_stats;
    PanelTags_ApplyToTree(ctx, ctx->active);
    DEBUG_LOG("DirPanelAction:switch:post_toggle active_saved_focus=%d",
              ctx->active->saved_focus);

    if (ctx->active->vol->total_dirs > 0) {
      if (ctx->active->disp_begin_pos + ctx->active->cursor_pos >=
          ctx->active->vol->total_dirs) {
        ctx->active->cursor_pos =
            ctx->active->vol->total_dirs - 1 - ctx->active->disp_begin_pos;
      }
      *dir_entry_ptr = ResolveActiveDirEntry(ctx, *s_ptr);
    } else {
      *dir_entry_ptr = (*s_ptr)->tree;
    }
    if (*dir_entry_ptr) {
      char switch_path[PATH_LENGTH + 1];
      GetPath(*dir_entry_ptr, switch_path);
      switch_path[PATH_LENGTH] = '\0';
      DEBUG_LOG("DirPanelAction:switch:resolved_dir='%s'", switch_path);
    } else {
      DEBUG_LOG("DirPanelAction:switch:resolved_dir=<null>");
    }

    DEBUG_LOG("ACTION_SWITCH_PANEL: active panel is now %s with "
              "cursor_pos=%d, dir_entry=%s",
              ctx->active == ctx->left ? "LEFT" : "RIGHT",
              ctx->active->cursor_pos,
              *dir_entry_ptr ? (*dir_entry_ptr)->name : "NULL");

    SyncActivePanelWindows(ctx);
    DEBUG_LOG("DirPanelAction:switch:after_sync_windows");
    if (ctx->active->file_selection_dir_path[0] != '\0') {
      RestorePanelAnchorPath(ctx->active->vol, ctx->active,
                             ctx->active->file_selection_dir_path);
      *dir_entry_ptr = GetPanelDirEntry(ctx->active);
    }
    *dir_entry_ptr =
        RestorePanelFileSelection(ctx, *dir_entry_ptr, ctx->active);
    if (!*dir_entry_ptr && *s_ptr) {
      *dir_entry_ptr = (*s_ptr)->tree;
      DEBUG_LOG("DirPanelAction:switch:restore returned null; fallback tree");
    }
    if (!AppStateMirrorActivePanelFocus(ctx))
      return FALSE;
    if (*dir_entry_ptr) {
      DEBUG_LOG("DirPanelAction:switch:before_refresh dir='%s'",
                (*dir_entry_ptr)->name ? (*dir_entry_ptr)->name : "<nullname>");
      RefreshView(ctx, *dir_entry_ptr);
      DEBUG_LOG("DirPanelAction:switch:after_refresh");
    } else {
      DEBUG_LOG("DirPanelAction:switch:skip_refresh dir_entry null");
      return TRUE;
    }
    *need_dsp_help_ptr = TRUE;
    if (ctx->focused_window == FOCUS_FILE && *dir_entry_ptr &&
        PanelHasVisibleFiles(ctx, ctx->active, *dir_entry_ptr)) {
      *unput_char_ptr = CR;
    }
#endif
    SplitTransitionDebugLogDirState("DirPanelAction:switch:after", ctx);
    return TRUE;

  default:
    return FALSE;
  }
}
