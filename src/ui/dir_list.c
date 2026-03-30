/***************************************************************************
 *
 * src/ui/dir_list.c
 * Directory Entry List Management (Build, Free, Query)
 *
 ***************************************************************************/

#include "ytree_ui.h"

/* Internal recursive helper for BuildDirEntryList */
static void ReadDirList(ViewContext *ctx, DirEntry *dir_entry,
                        struct Volume *vol, int *index_ptr) {
  DirEntry *de_ptr;
  static int level = 0;
  static unsigned long indent = 0L;

  for (de_ptr = dir_entry; de_ptr; de_ptr = de_ptr->next) {
    /* Check visibility. */
    if (ctx->hide_dot_files && de_ptr->name[0] == '.') {
      if (de_ptr != vol->vol_stats.tree)
        continue;
    }

    /* Bounds Checking & Dynamic Reallocation */
    if (*index_ptr >= (int)vol->dir_entry_list_capacity) {
      size_t new_capacity = vol->dir_entry_list_capacity * 2;
      if (new_capacity == 0)
        new_capacity = 128;

      DirEntryList *new_list = (DirEntryList *)xrealloc(
          vol->dir_entry_list, new_capacity * sizeof(DirEntryList));

      memset(new_list + vol->dir_entry_list_capacity, 0,
             (new_capacity - vol->dir_entry_list_capacity) *
                 sizeof(DirEntryList));

      vol->dir_entry_list = new_list;
      vol->dir_entry_list_capacity = new_capacity;
    }

    indent &= ~(1L << level);
    if (de_ptr->next)
      indent |= (1L << level);

    vol->dir_entry_list[*index_ptr].dir_entry = de_ptr;
    vol->dir_entry_list[*index_ptr].level = (unsigned short)level;
    vol->dir_entry_list[*index_ptr].indent = indent;

    (*index_ptr)++;

    if (!de_ptr->not_scanned && de_ptr->sub_tree) {
      level++;
      ReadDirList(ctx, de_ptr->sub_tree, vol, index_ptr);
      level--;
    }
  }
}

void BuildDirEntryList(ViewContext *ctx, struct Volume *vol, int *index_ptr) {
  if (vol->dir_entry_list != NULL) {
    free(vol->dir_entry_list);
    vol->dir_entry_list = NULL;
    vol->dir_entry_list_capacity = 0;
  }

  /* Initialize with the estimated count, but enforce a minimum safety size */
  size_t alloc_count = vol->vol_stats.disk_total_directories;
  if (alloc_count < 16)
    alloc_count = 16;

  vol->dir_entry_list =
      (DirEntryList *)xcalloc(alloc_count, sizeof(DirEntryList));

  vol->dir_entry_list_capacity = alloc_count;
  *index_ptr = 0;

  /* Only read if we have a valid tree structure */
  if (vol->vol_stats.tree) {
    ReadDirList(ctx, vol->vol_stats.tree, vol, index_ptr);
  }

  vol->total_dirs = *index_ptr;

#ifdef DEBUG
  if (vol->vol_stats.disk_total_directories != vol->total_dirs) {
    /* mismatch detected, but safely handled by realloc in ReadDirList */
  }
#endif
}

/*
 * Frees the memory allocated for the dir_entry_list array of a volume.
 */
void FreeVolumeCache(struct Volume *vol) {
  if (vol && vol->dir_entry_list != NULL) {
    free(vol->dir_entry_list);
    vol->dir_entry_list = NULL;
    vol->dir_entry_list_capacity = 0;
    vol->total_dirs = 0;
  }
}

/*
 * Frees the memory allocated for the current volume's dir_entry_list.
 * Retained for compatibility.
 */
void FreeDirEntryList(ViewContext *ctx) {
  if (ctx->active->vol) {
    FreeVolumeCache(ctx->active->vol);
  }
}

/*
 * Helper function to return the currently selected directory entry from a
 * specific panel. Uses the panel's ViewContext (cursor_pos, disp_begin_pos)
 * instead of shared Volume stats.
 */
DirEntry *GetPanelDirEntry(YtreePanel *p) {
  if (p->vol->dir_entry_list != NULL && p->vol->total_dirs > 0) {
    /* Use Panel state directly to avoid leakage via shared Volume stats */
    int idx = p->disp_begin_pos + p->cursor_pos;

    /* Safety bounds check */
    if (idx < 0)
      idx = 0;
    if (idx >= p->vol->total_dirs)
      idx = p->vol->total_dirs - 1;

    return p->vol->dir_entry_list[idx].dir_entry;
  }
  /* Fallback to root if list is empty/invalid */
  return p->vol->vol_stats.tree;
}

/*
 * Helper function to return the currently selected directory entry.
 * Now takes a Volume context.
 */
DirEntry *GetSelectedDirEntry(ViewContext *ctx, struct Volume *vol) {
  if (vol->dir_entry_list != NULL && vol->total_dirs > 0) {
    /* WARNING: Using ctx->active for 'selection' might be risky during
     * async building, but we provide GetPanelDirEntry for explicit panel
     * contexts. */
    int idx = ctx->active->disp_begin_pos + ctx->active->cursor_pos;

    /* Safety bounds check */
    if (idx < 0)
      idx = 0;
    if (idx >= vol->total_dirs)
      idx = vol->total_dirs - 1;

    return vol->dir_entry_list[idx].dir_entry;
  }
  /* Fallback to root if list is empty/invalid */
  return vol->vol_stats.tree;
}
