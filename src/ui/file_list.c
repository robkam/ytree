/***************************************************************************
 *
 * src/ui/file_list.c
 * File Entry List Management (Build, Read, Free, Query, Display)
 *
 ***************************************************************************/

#define NO_YTREE_MACROS
#include "ytree.h"
#include "sort.h"
#include "ytree_ui.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

/* Display Metrics statics — owned by this module */
static unsigned max_visual_filename_len;
static unsigned max_visual_linkname_len;
static unsigned max_visual_userview_len;
static unsigned global_max_visual_filename_len;
static unsigned global_max_visual_linkname_len;

static void ReadFileList(ViewContext *ctx, YtreePanel *panel, BOOL tagged_only,
                         DirEntry *dir_entry) {
  FileEntry *fe_ptr;
  unsigned int name_len;
  unsigned int visual_name_len;
  unsigned int visual_linkname_len;

  max_visual_filename_len = 0;
  max_visual_linkname_len = 0;

  for (fe_ptr = dir_entry->file; fe_ptr; fe_ptr = fe_ptr->next) {
    if (fe_ptr->matching && (!tagged_only || fe_ptr->tagged)) {
      /* Ensure hidden dot files are skipped unless option is disabled.
         This applies to both FS mode and Archive mode. */
      if (ctx->hide_dot_files && fe_ptr->name[0] == '.')
        continue;

      /* Bounds check */
      if (panel->file_count >= panel->file_entry_list_capacity) {
        size_t new_capacity = panel->file_entry_list_capacity * 2;
        if (new_capacity == 0)
          new_capacity = 128;

        FileEntryList *new_list = (FileEntryList *)realloc(
            panel->file_entry_list, new_capacity * sizeof(FileEntryList));
        if (!new_list) {
          UI_Error(ctx, "", 0, "Realloc failed in ReadFileList*ABORT");
          exit(1);
        }
        memset(new_list + panel->file_entry_list_capacity, 0,
               (new_capacity - panel->file_entry_list_capacity) *
                   sizeof(FileEntryList));
        panel->file_entry_list = new_list;
        panel->file_entry_list_capacity = new_capacity;
      }

      panel->file_entry_list[panel->file_count++].file = fe_ptr;
      visual_name_len = StrVisualLength(fe_ptr->name);
      name_len = strlen(fe_ptr->name);
      if (S_ISLNK(fe_ptr->stat_struct.st_mode)) {
        visual_linkname_len = StrVisualLength(&fe_ptr->name[name_len + 1]);
        max_visual_linkname_len =
            MAX((int)max_visual_linkname_len, (int)visual_linkname_len);
      }
      max_visual_filename_len =
          MAX((int)max_visual_filename_len, (int)visual_name_len);
    }
  }
}

static void ReadGlobalFileListInternal(ViewContext *ctx, YtreePanel *panel,
                                       BOOL tagged_only, DirEntry *dir_entry) {
  DirEntry *de_ptr;

  for (de_ptr = dir_entry; de_ptr; de_ptr = de_ptr->next) {
    if (ctx->hide_dot_files && de_ptr->name[0] == '.')
      continue;
    if (de_ptr->sub_tree)
      ReadGlobalFileListInternal(ctx, panel, tagged_only, de_ptr->sub_tree);
    ReadFileList(ctx, panel, tagged_only, de_ptr);
    global_max_visual_filename_len =
        MAX((int)global_max_visual_filename_len, (int)max_visual_filename_len);
    global_max_visual_linkname_len =
        MAX((int)global_max_visual_linkname_len, (int)max_visual_linkname_len);
  }
}

static void ReadGlobalFileList(ViewContext *ctx, YtreePanel *panel,
                               BOOL tagged_only, DirEntry *dir_entry) {
  global_max_visual_filename_len = 0;
  global_max_visual_linkname_len = 0;
  ReadGlobalFileListInternal(ctx, panel, tagged_only, dir_entry);
  max_visual_filename_len = global_max_visual_filename_len;
  max_visual_linkname_len = global_max_visual_linkname_len;
}

/* Removed SetKindOfSort definition - implemented in sort.c */

void FileList_RemoveFileEntry(ViewContext *ctx, int entry_no) {
  int i, n;
  FileEntry *fe_ptr;
  int visual_name_len, name_len;

  max_visual_filename_len = 0;
  max_visual_linkname_len = 0;
  n = ctx->active->file_count - 1;

  for (i = 0; i < n; i++) {
    if (i >= entry_no)
      ctx->active->file_entry_list[i] = ctx->active->file_entry_list[i + 1];
    fe_ptr = ctx->active->file_entry_list[i].file;
    visual_name_len = StrVisualLength(fe_ptr->name);
    name_len = strlen(fe_ptr->name);
    /* FIX: Cast StrVisualLength to int for MAX macro */
    max_visual_filename_len =
        MAX((int)max_visual_filename_len, (int)visual_name_len);
    if (S_ISLNK(fe_ptr->stat_struct.st_mode)) {
      /* FIX: Cast StrVisualLength to int for MAX macro */
      max_visual_linkname_len =
          MAX((int)max_visual_linkname_len,
              (int)StrVisualLength(&fe_ptr->name[name_len + 1]));
    }
  }

  SetFileRenderingMetrics(ctx->active, max_visual_filename_len,
                          max_visual_linkname_len, max_visual_userview_len);
  SetPanelFileMode(ctx, ctx->active, GetPanelFileMode(ctx->active));

  ctx->active->file_count--; /* no realloc */
}

void FileList_ChangeFileEntry(ViewContext *ctx) {
  int i, n;
  FileEntry *fe_ptr;
  int visual_name_len, name_len;

  max_visual_filename_len = 0;
  max_visual_linkname_len = 0;
  n = ctx->active->file_count - 1;

  for (i = 0; i < n; i++) {
    fe_ptr = ctx->active->file_entry_list[i].file;
    if (fe_ptr) {
      visual_name_len = StrVisualLength(fe_ptr->name);
      name_len = strlen(fe_ptr->name);
      /* FIX: Cast StrVisualLength to int for MAX macro */
      max_visual_filename_len =
          MAX((int)max_visual_filename_len, (int)visual_name_len);
      if (S_ISLNK(fe_ptr->stat_struct.st_mode)) {
        /* FIX: Cast StrVisualLength to int for MAX macro */
        max_visual_linkname_len =
            MAX((int)max_visual_linkname_len,
                (int)StrVisualLength(&fe_ptr->name[name_len + 1]));
      }
    }
  }

  SetFileRenderingMetrics(ctx->active, max_visual_filename_len,
                          max_visual_linkname_len, max_visual_userview_len);
  SetPanelFileMode(ctx, ctx->active, GetPanelFileMode(ctx->active));
}

void FreeFileEntryList(YtreePanel *panel) {
  if (panel && panel->file_entry_list) {
    free(panel->file_entry_list);
    panel->file_entry_list = NULL;
    panel->file_entry_list_capacity = 0;
    panel->file_count = 0;
  }
}

void InvalidateVolumePanels(ViewContext *ctx, struct Volume *vol) {
  if (ctx->left && ctx->left->vol == vol)
    FreeFileEntryList(ctx->left);
  if (ctx->right && ctx->right->vol == vol)
    FreeFileEntryList(ctx->right);
}

void DisplayFileWindow(ViewContext *ctx, YtreePanel *panel,
                       DirEntry *dir_entry) {
  if (!panel || !panel->pan_file_window)
    return;

  BuildFileEntryList(ctx, panel);

  /* ADDED INSTRUCTION: Focus-Aware Highlighting */
  int highlight_idx = -1;
  if (ctx->focused_window == FOCUS_FILE) {
    highlight_idx = dir_entry->start_file + dir_entry->cursor_pos;
  }

  DisplayFiles(ctx, panel, dir_entry, dir_entry->start_file, highlight_idx, 0,
               panel->pan_file_window);
}

void BuildFileEntryList(ViewContext *ctx, YtreePanel *panel) {
  DirEntry *dir_entry;
  if (!panel || !panel->vol || panel->vol->total_dirs == 0)
    return;

  /* Safety: If Volume's dir_entry_list was recently freed (e.g. by
     LogDisk/Rescan), rebuild it before attempting to access it. */
  if (panel->vol->dir_entry_list == NULL) {
    BuildDirEntryList(ctx, panel->vol, &panel->current_dir_entry);
  }

  int idx = panel->disp_begin_pos + panel->cursor_pos;
  if (idx >= panel->vol->total_dirs)
    idx = panel->vol->total_dirs - 1;
  dir_entry = panel->vol->dir_entry_list[idx].dir_entry;

  /* Apply filter to current directory or entire tree before building list */
  if (dir_entry->global_flag) {
    ApplyFilterToTree(panel->vol->vol_stats.tree, &panel->vol->vol_stats);
  } else {
    ApplyFilter(dir_entry, &panel->vol->vol_stats);
  }

  panel->file_count = 0;
  if (dir_entry->global_flag) {
    ReadGlobalFileList(ctx, panel, FALSE, panel->vol->vol_stats.tree);
  } else {
    ReadFileList(ctx, panel, FALSE, dir_entry);
  }
  Panel_Sort(panel, panel->vol->vol_stats.kind_of_sort);

  /* Propagate metrics to the renderer */
  SetFileRenderingMetrics(panel, max_visual_filename_len,
                          max_visual_linkname_len, max_visual_userview_len);
  SetPanelFileMode(ctx, panel, GetPanelFileMode(panel));
}
