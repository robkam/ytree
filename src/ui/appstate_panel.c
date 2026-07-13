/***************************************************************************
 *
 * src/ui/appstate_panel.c
 * Panel-local transition commits for AppState boundaries.
 *
 ***************************************************************************/

#define NO_YTNOVA_MACROS

#include "ytnova_appstate_actions.h"
#include "ytnova_appstate_panel.h"
#include <string.h>

BOOL AppStateCommitPanelGeneration(YtreeNovaPanel *panel) {
  if (!AppStateValidatedGenerationDomain("generation.panel.local-authority"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.panel_generation"))
    return FALSE;
  if (!panel)
    return FALSE;

  panel->panel_generation++;
  return TRUE;
}

BOOL AppStateRestorePanelGeneration(YtreeNovaPanel *panel,
                                    unsigned int panel_generation) {
  if (!AppStateValidatedGenerationDomain("generation.panel.local-authority"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.panel_generation"))
    return FALSE;
  if (!panel)
    return FALSE;

  panel->panel_generation = panel_generation;
  return TRUE;
}

BOOL AppStateCommitPanelVolume(YtreeNovaPanel *panel, struct Volume *vol) {
  if (!AppStateValidatedGenerationDomain("generation.panel.local-authority"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.volume_key"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.panel_generation"))
    return FALSE;
  if (!panel)
    return FALSE;

  panel->vol = vol;
  return AppStateCommitPanelGeneration(panel);
}

BOOL AppStateSetPanelVolumeFileStateList(
    YtreeNovaPanel *panel, PanelVolumeFileState *volume_file_state) {
  if (!AppStateValidatedGenerationDomain("generation.panel.local-authority"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.restore_snapshot"))
    return FALSE;
  if (!panel)
    return FALSE;

  panel->volume_file_state = volume_file_state;
  return TRUE;
}

BOOL AppStateCommitPanelDirectoryDisplayMode(YtreeNovaPanel *panel, int dir_mode) {
  if (!AppStateValidatedOwnerField("panel.file_display_state"))
    return FALSE;
  if (!panel)
    return FALSE;
  if (dir_mode < MODE_1 || dir_mode > MODE_4)
    return FALSE;

  panel->dir_mode = dir_mode;
  return TRUE;
}

BOOL AppStateCommitPanelFileDisplayMode(YtreeNovaPanel *panel, int file_mode) {
  if (!AppStateValidatedOwnerField("panel.file_display_state"))
    return FALSE;
  if (!panel)
    return FALSE;
  if (file_mode < MODE_1 || file_mode > MODE_5)
    return FALSE;

  panel->file_mode = file_mode;
  return TRUE;
}

BOOL AppStateCommitPanelFileInfoOverlayMode(YtreeNovaPanel *panel,
                                            int overlay_mode) {
  if (!AppStateValidatedOwnerField("panel.file_display_state"))
    return FALSE;
  if (!panel)
    return FALSE;
  if (overlay_mode < FILEINFO_OVERLAY_NONE ||
      overlay_mode > FILEINFO_OVERLAY_GIT)
    return FALSE;

  panel->fileinfo_overlay_mode = overlay_mode;
  return TRUE;
}

BOOL AppStateCommitPanelFixedColumnWidth(YtreeNovaPanel *panel,
                                         int fixed_col_width) {
  if (!AppStateValidatedOwnerField("panel.file_display_state"))
    return FALSE;
  if (!panel || fixed_col_width < 0)
    return FALSE;

  panel->fixed_col_width = fixed_col_width;
  return TRUE;
}

BOOL AppStateCommitPanelFileMaxColumn(YtreeNovaPanel *panel,
                                      unsigned max_column) {
  if (!AppStateValidatedOwnerField("panel.file_display_state"))
    return FALSE;
  if (!panel || max_column == 0)
    return FALSE;

  panel->max_column = max_column;
  return TRUE;
}

BOOL AppStateCommitPanelFileRenderingMetrics(YtreeNovaPanel *panel,
                                             unsigned max_filename,
                                             unsigned max_linkname,
                                             unsigned max_userview,
                                             BOOL update_userview) {
  if (!AppStateValidatedOwnerField("panel.file_display_state"))
    return FALSE;
  if (!panel)
    return FALSE;

  panel->max_visual_filename_len = max_filename;
  panel->max_visual_linkname_len = max_linkname;
  if (update_userview)
    panel->max_visual_userview_len = max_userview;
  return TRUE;
}

BOOL AppStateCommitPanelFileSortOrder(YtreeNovaPanel *panel,
                                      BOOL reverse_sort) {
  if (!AppStateValidatedOwnerField("panel.file_display_state"))
    return FALSE;
  if (!panel)
    return FALSE;

  panel->reverse_sort = reverse_sort;
  return TRUE;
}

BOOL AppStateCommitPanelSizeUnitMode(YtreeNovaPanel *panel,
                                     BOOL human_size_units) {
  if (!AppStateValidatedOwnerField("panel.file_display_state"))
    return FALSE;
  if (!panel)
    return FALSE;

  panel->human_size_units = human_size_units ? TRUE : FALSE;
  return TRUE;
}

BOOL AppStateCommitPanelSymlinkTargetMode(YtreeNovaPanel *panel,
                                          BOOL show_symlink_targets) {
  if (!AppStateValidatedOwnerField("panel.file_display_state"))
    return FALSE;
  if (!panel)
    return FALSE;

  panel->show_symlink_targets = show_symlink_targets ? TRUE : FALSE;
  return TRUE;
}

BOOL AppStateCommitPanelFileSelection(YtreeNovaPanel *panel,
                                      const char *dir_path,
                                      const char *file_name) {
  if (!AppStateValidatedGenerationDomain("generation.panel.local-authority"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.file_selection_key"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.panel_generation"))
    return FALSE;
  if (!panel)
    return FALSE;

  panel->file_selection_dir_path[0] = '\0';
  if (dir_path) {
    (void)snprintf(panel->file_selection_dir_path,
                   sizeof(panel->file_selection_dir_path), "%s", dir_path);
    panel->file_selection_dir_path[PATH_LENGTH] = '\0';
  }

  panel->file_selection_name[0] = '\0';
  if (file_name) {
    (void)snprintf(panel->file_selection_name,
                   sizeof(panel->file_selection_name), "%s", file_name);
    panel->file_selection_name[PATH_LENGTH] = '\0';
  }

  return AppStateCommitPanelGeneration(panel);
}

BOOL AppStateCommitPanelTreeSelection(YtreeNovaPanel *panel,
                                      int current_dir_entry) {
  if (!AppStateValidatedGenerationDomain("generation.panel.local-authority"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.tree_selection_key"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.panel_generation"))
    return FALSE;
  if (!panel)
    return FALSE;

  panel->current_dir_entry = current_dir_entry;
  return AppStateCommitPanelGeneration(panel);
}

BOOL AppStateCommitPanelTreeViewportTopPath(YtreeNovaPanel *panel, int slot,
                                            const char *top_path) {
  if (!AppStateValidatedGenerationDomain("generation.panel.local-authority"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.restore_snapshot"))
    return FALSE;
  if (!panel || slot < 0 || slot >= 2)
    return FALSE;

  panel->tree_viewport_top_dir_path[slot][0] = '\0';
  if (top_path) {
    (void)snprintf(panel->tree_viewport_top_dir_path[slot],
                   sizeof(panel->tree_viewport_top_dir_path[slot]), "%s",
                   top_path);
    panel->tree_viewport_top_dir_path[slot][PATH_LENGTH] = '\0';
  }
  return TRUE;
}

BOOL AppStateCommitPanelTreeViewportTopPaths(YtreeNovaPanel *panel,
                                             const YtreeNovaPanel *source) {
  if (!AppStateValidatedGenerationDomain("generation.panel.local-authority"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.restore_snapshot"))
    return FALSE;
  if (!panel || !source)
    return FALSE;

  memcpy(panel->tree_viewport_top_dir_path, source->tree_viewport_top_dir_path,
         sizeof(panel->tree_viewport_top_dir_path));
  return TRUE;
}

BOOL AppStateCommitPanelVolumeTreeViewportSnapshot(
    PanelVolumeFileState *state, unsigned int panel_generation,
    unsigned int volume_generation, BOOL has_selected_dir_path,
    const char *selected_dir_path, BOOL has_top_dir_path,
    const char *top_dir_path) {
  if (!AppStateValidatedGenerationDomain("generation.panel.local-authority"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.restore_snapshot"))
    return FALSE;
  if (!state)
    return FALSE;

  state->saved_tree_panel_generation = panel_generation;
  state->saved_tree_volume_generation = volume_generation;
  state->has_saved_tree_selection = has_selected_dir_path;
  state->has_saved_tree_top = has_top_dir_path;
  state->saved_tree_selected_dir_path[0] = '\0';
  state->saved_tree_top_dir_path[0] = '\0';
  if (has_selected_dir_path && selected_dir_path) {
    (void)snprintf(state->saved_tree_selected_dir_path,
                   sizeof(state->saved_tree_selected_dir_path), "%s",
                   selected_dir_path);
    state->saved_tree_selected_dir_path[PATH_LENGTH] = '\0';
  }
  if (has_top_dir_path && top_dir_path) {
    (void)snprintf(state->saved_tree_top_dir_path,
                   sizeof(state->saved_tree_top_dir_path), "%s",
                   top_dir_path);
    state->saved_tree_top_dir_path[PATH_LENGTH] = '\0';
  }
  return TRUE;
}

BOOL AppStateCommitPanelVolumeFileSnapshot(
    PanelVolumeFileState *state, int start_file, int file_cursor_pos,
    unsigned int panel_generation, unsigned int volume_generation,
    ViewFocus focus, BOOL big_file_view, const char *file_dir_path,
    const char *file_selection_dir_path, const char *file_selection_name) {
  if (!AppStateValidatedGenerationDomain("generation.panel.local-authority"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.restore_snapshot"))
    return FALSE;
  if (!state)
    return FALSE;

  state->saved_file_start = start_file;
  state->saved_file_cursor = file_cursor_pos;
  state->saved_panel_generation = panel_generation;
  state->saved_volume_generation = volume_generation;
  state->saved_focus = focus;
  state->saved_big_file_view = big_file_view;
  state->saved_file_dir_path[0] = '\0';
  state->saved_file_selection_dir_path[0] = '\0';
  state->saved_file_selection_name[0] = '\0';
  if (file_dir_path) {
    (void)snprintf(state->saved_file_dir_path,
                   sizeof(state->saved_file_dir_path), "%s", file_dir_path);
    state->saved_file_dir_path[PATH_LENGTH] = '\0';
  }
  if (file_selection_dir_path) {
    (void)snprintf(state->saved_file_selection_dir_path,
                   sizeof(state->saved_file_selection_dir_path), "%s",
                   file_selection_dir_path);
    state->saved_file_selection_dir_path[PATH_LENGTH] = '\0';
  }
  if (file_selection_name) {
    (void)snprintf(state->saved_file_selection_name,
                   sizeof(state->saved_file_selection_name), "%s",
                   file_selection_name);
    state->saved_file_selection_name[PATH_LENGTH] = '\0';
  }
  return TRUE;
}

BOOL AppStateCommitDirEntryFileViewport(DirEntry *dir_entry, int start_file,
                                        int cursor_pos) {
  if (!AppStateValidatedGenerationDomain("generation.panel.local-authority"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.file_viewport_origin"))
    return FALSE;
  if (!dir_entry)
    return FALSE;

  dir_entry->start_file = start_file;
  dir_entry->cursor_pos = cursor_pos;
  return TRUE;
}

BOOL AppStateCommitPanelFileViewport(YtreeNovaPanel *panel, int start_file,
                                     int file_cursor_pos) {
  if (!AppStateValidatedGenerationDomain("generation.panel.local-authority"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.file_viewport_origin"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.panel_generation"))
    return FALSE;
  if (!panel)
    return FALSE;

  panel->start_file = start_file;
  panel->file_cursor_pos = file_cursor_pos;
  return AppStateCommitPanelGeneration(panel);
}

BOOL AppStateCommitPanelFileAnchor(YtreeNovaPanel *panel,
                                   DirEntry *file_dir_entry) {
  if (!AppStateValidatedGenerationDomain("generation.panel.local-authority"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.file_viewport_origin"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.panel_generation"))
    return FALSE;
  if (!panel)
    return FALSE;

  panel->file_dir_entry = file_dir_entry;
  return AppStateCommitPanelGeneration(panel);
}

BOOL AppStateCommitPanelTreeViewport(YtreeNovaPanel *panel, int disp_begin_pos,
                                     int cursor_pos) {
  if (!AppStateValidatedGenerationDomain("generation.panel.local-authority"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.tree_viewport_origin"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.tree_cursor_pos"))
    return FALSE;
  if (!AppStateValidatedOwnerField("panel.panel_generation"))
    return FALSE;
  if (!panel)
    return FALSE;

  panel->disp_begin_pos = disp_begin_pos;
  panel->cursor_pos = cursor_pos;
  return AppStateCommitPanelGeneration(panel);
}
