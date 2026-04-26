/***************************************************************************
 *
 * src/ui/interactions_panel_paths.c
 * Shared panel path helpers for UI interaction modules.
 *
 ***************************************************************************/

#include "interactions_panel_paths.h"
#include "ytree_fs.h"

YtreePanel *UI_GetInactivePanel(ViewContext *ctx) {
  if (!ctx || !ctx->is_split_screen || !ctx->active || !ctx->left ||
      !ctx->right)
    return NULL;
  return (ctx->active == ctx->left) ? ctx->right : ctx->left;
}

int UI_GetPanelSelectedDirPath(ViewContext *ctx, YtreePanel *panel,
                                   char *out_path) {
  DirEntry *dir_entry = NULL;

  if (!ctx || !panel || !panel->vol || !out_path)
    return -1;

  if (!panel->vol->dir_entry_list || panel->vol->total_dirs <= 0) {
    BuildDirEntryList(ctx, panel->vol, &panel->current_dir_entry);
  }
  if (!panel->vol->dir_entry_list || panel->vol->total_dirs <= 0)
    return -1;

  dir_entry = GetPanelDirEntry(panel);
  if (!dir_entry)
    return -1;

  GetPath(dir_entry, out_path);
  out_path[PATH_LENGTH] = '\0';
  return 0;
}

int UI_GetPanelLoggedRootPath(YtreePanel *panel, char *out_path) {
  if (!panel || !panel->vol || !panel->vol->vol_stats.tree || !out_path)
    return -1;

  GetPath(panel->vol->vol_stats.tree, out_path);
  out_path[PATH_LENGTH] = '\0';
  return 0;
}

int UI_GetPanelSelectedFilePath(ViewContext *ctx, YtreePanel *panel,
                                    char *out_path) {
  const DirEntry *dir_entry = NULL;
  FileEntry *file_entry = NULL;
  int file_idx;

  if (!ctx || !panel || !panel->vol || !out_path)
    return -1;

  if (!panel->vol->dir_entry_list || panel->vol->total_dirs <= 0) {
    BuildDirEntryList(ctx, panel->vol, &panel->current_dir_entry);
  }
  if (!panel->vol->dir_entry_list || panel->vol->total_dirs <= 0)
    return -1;

  BuildFileEntryList(ctx, panel);
  if (!panel->file_entry_list || panel->file_count == 0)
    return -1;

  dir_entry = GetPanelDirEntry(panel);
  if (!dir_entry)
    return -1;

  if (panel->saved_focus == FOCUS_FILE && panel->file_dir_entry == dir_entry) {
    file_idx = panel->start_file + panel->file_cursor_pos;
  } else {
    file_idx = dir_entry->start_file + dir_entry->cursor_pos;
  }
  if (file_idx < 0)
    file_idx = 0;
  if ((unsigned int)file_idx >= panel->file_count)
    file_idx = (int)panel->file_count - 1;
  if (file_idx < 0)
    return -1;

  file_entry = panel->file_entry_list[file_idx].file;
  if (!file_entry)
    return -1;

  GetFileNamePath(file_entry, out_path);
  out_path[PATH_LENGTH] = '\0';
  return 0;
}
