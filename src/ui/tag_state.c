#include "ytree_fs.h"
#include "ytree_ui.h"
#include <stdlib.h>
#include <string.h>

static int PathListContains(const PathList *list, const char *path) {
  for (; list; list = list->next) {
    if (list->path && strcmp(list->path, path) == 0)
      return TRUE;
  }
  return FALSE;
}

static int PathListCount(const PathList *list) {
  int count = 0;

  for (; list; list = list->next)
    count++;

  return count;
}

static void AddTaggedPath(YtreePanel *panel, const char *path) {
  PathList *node;

  if (!panel || !path || !*path || PathListContains(panel->tagged_paths, path))
    return;

  node = (PathList *)xcalloc(1, sizeof(PathList));
  node->path = xstrdup(path);
  node->next = panel->tagged_paths;
  panel->tagged_paths = node;
}

static void RemoveTaggedPath(YtreePanel *panel, const char *path) {
  PathList **link;

  if (!panel || !path || !*path)
    return;

  for (link = &panel->tagged_paths; *link;) {
    PathList *node = *link;
    if (node->path && strcmp(node->path, path) == 0) {
      *link = node->next;
      free(node->path);
      free(node);
      return;
    }
    link = &node->next;
  }
}

static BOOL TaggedPathIsUnderDir(const char *tagged_path, const char *dir_path) {
  size_t dir_len;

  if (!tagged_path || !dir_path || tagged_path[0] == '\0' ||
      dir_path[0] == '\0')
    return FALSE;

  if (strcmp(dir_path, FILE_SEPARATOR_STRING) == 0)
    return tagged_path[0] == FILE_SEPARATOR_CHAR ? TRUE : FALSE;

  dir_len = strlen(dir_path);
  return strncmp(tagged_path, dir_path, dir_len) == 0 &&
         tagged_path[dir_len] == FILE_SEPARATOR_CHAR;
}

static void GetTaggedFilePath(FileEntry *file_entry, char *path,
                              size_t path_size) {
  if (!path || path_size == 0)
    return;

  path[0] = '\0';
  if (!file_entry || !file_entry->dir_entry)
    return;

  GetFileNamePath(file_entry, path);
  path[path_size - 1] = '\0';
}

void PanelTags_Clear(YtreePanel *panel) {
  if (!panel)
    return;

  FreePathList(panel->tagged_paths);
  panel->tagged_paths = NULL;
}

void PanelTags_Copy(YtreePanel *dst, const YtreePanel *src) {
  const PathList *node;

  if (!dst || !src || dst == src)
    return;

  PanelTags_Clear(dst);
  for (node = src->tagged_paths; node; node = node->next)
    AddTaggedPath(dst, node->path);
}

void PanelTags_PruneUnderDir(YtreePanel *panel, DirEntry *dir_entry) {
  char dir_path[PATH_LENGTH + 1];
  PathList **link;

  if (!panel || !dir_entry)
    return;

  GetPath(dir_entry, dir_path);
  if (dir_path[0] == '\0')
    return;

  for (link = &panel->tagged_paths; *link;) {
    PathList *node = *link;
    if (TaggedPathIsUnderDir(node->path, dir_path)) {
      *link = node->next;
      free(node->path);
      free(node);
      continue;
    }
    link = &node->next;
  }
}

BOOL PanelTags_FileIsTagged(const YtreePanel *panel, FileEntry *file_entry) {
  char path[PATH_LENGTH + 1];

  if (!panel || !file_entry)
    return FALSE;

  GetTaggedFilePath(file_entry, path, sizeof(path));
  return PathListContains(panel->tagged_paths, path) ? TRUE : FALSE;
}

void PanelTags_RecordFileState(YtreePanel *panel, FileEntry *file_entry,
                               BOOL tagged) {
  char path[PATH_LENGTH + 1];

  if (!panel || !file_entry)
    return;

  GetTaggedFilePath(file_entry, path, sizeof(path));
  if (path[0] == '\0')
    return;

  if (tagged)
    AddTaggedPath(panel, path);
  else
    RemoveTaggedPath(panel, path);
}

static void ApplyPanelTagsToDir(const YtreePanel *panel, DirEntry *dir_entry,
                                Statistic *stats) {
  FileEntry *file_entry;

  if (!panel || !dir_entry || !stats)
    return;

  dir_entry->tagged_files = 0;
  dir_entry->tagged_bytes = 0;
  for (file_entry = dir_entry->file; file_entry; file_entry = file_entry->next) {
    file_entry->tagged = PanelTags_FileIsTagged(panel, file_entry);
    if (file_entry->tagged) {
      dir_entry->tagged_files++;
      dir_entry->tagged_bytes += file_entry->stat_struct.st_size;
      stats->disk_tagged_files++;
      stats->disk_tagged_bytes += file_entry->stat_struct.st_size;
    }
  }
}

static void ApplyPanelTagsToTree(YtreePanel *panel, DirEntry *dir_entry,
                                 Statistic *stats) {
  for (; dir_entry; dir_entry = dir_entry->next) {
    ApplyPanelTagsToDir(panel, dir_entry, stats);
    if (dir_entry->sub_tree)
      ApplyPanelTagsToTree(panel, dir_entry->sub_tree, stats);
  }
}

void PanelTags_ApplyToTree(ViewContext *ctx, YtreePanel *panel) {
  Statistic *stats;

  (void)ctx;
  if (!panel || !panel->vol || !panel->vol->vol_stats.tree)
    return;

  stats = &panel->vol->vol_stats;
  stats->disk_tagged_files = 0;
  stats->disk_tagged_bytes = 0;
  ApplyPanelTagsToTree(panel, stats->tree, stats);
}

void PanelTags_Restore(ViewContext *ctx, YtreePanel *panel) {
  PathList *expanded = NULL;

  if (!ctx || !panel || !panel->vol || !panel->vol->vol_stats.tree)
    return;

  if (panel->tagged_paths) {
    DEBUG_LOG("PanelTags:restore count=%d", PathListCount(panel->tagged_paths));
    RestoreTreeState(ctx, panel->vol->vol_stats.tree, &expanded,
                     panel->tagged_paths, &panel->vol->vol_stats);
    FreePathList(expanded);
  }
  PanelTags_ApplyToTree(ctx, panel);
}
