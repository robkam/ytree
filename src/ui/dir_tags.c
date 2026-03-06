/***************************************************************************
 *
 * src/ui/dir_tags.c
 * Directory-Level Tagging Operations (Tag/Untag single and all directories)
 *
 ***************************************************************************/

#define NO_YTREE_MACROS
#include "ytree.h"
#include "ytree_ui.h"

void HandleTagDir(ViewContext *ctx, DirEntry *dir_entry, BOOL value,
                  YtreePanel *p) {
  Statistic *s = &p->vol->vol_stats;

  FileEntry *fe_ptr;
  for (fe_ptr = dir_entry->file; fe_ptr; fe_ptr = fe_ptr->next) {
    /* Skip hidden dotfiles if the option is enabled */
    if (ctx->hide_dot_files && fe_ptr->name[0] == '.')
      continue;

    if ((fe_ptr->matching) && (fe_ptr->tagged != value)) {
      fe_ptr->tagged = value;
      if (value) {
        dir_entry->tagged_files++;
        dir_entry->tagged_bytes += fe_ptr->stat_struct.st_size;
        s->disk_tagged_files++;
        s->disk_tagged_bytes += fe_ptr->stat_struct.st_size;
      } else {
        dir_entry->tagged_files--;
        dir_entry->tagged_bytes -= fe_ptr->stat_struct.st_size;
        s->disk_tagged_files--;
        s->disk_tagged_bytes -= fe_ptr->stat_struct.st_size;
      }
    }
  }
  dir_entry->start_file = 0;
  dir_entry->cursor_pos = -1;
  DisplayFileWindow(ctx, p, dir_entry);
  RefreshWindow(p->pan_file_window);
  DisplayDiskStatistic(ctx, s);
  UpdateStatsPanel(ctx, dir_entry, s);
  return;
}

void HandleTagAllDirs(ViewContext *ctx, struct Volume *vol, DirEntry *dir_entry,
                      BOOL value, YtreePanel *p) {
  Statistic *s = &vol->vol_stats;
  FileEntry *fe_ptr;
  long i;
  for (i = 0; i < vol->total_dirs; i++) {
    for (fe_ptr = vol->dir_entry_list[i].dir_entry->file; fe_ptr;
         fe_ptr = fe_ptr->next) {
      /* Skip hidden dotfiles if the option is enabled */
      if (ctx->hide_dot_files && fe_ptr->name[0] == '.')
        continue;

      if ((fe_ptr->matching) && (fe_ptr->tagged != value)) {
        if (value) {
          fe_ptr->tagged = value;
          dir_entry->tagged_files++;
          dir_entry->tagged_bytes += fe_ptr->stat_struct.st_size;
          s->disk_tagged_files++;
          s->disk_tagged_bytes += fe_ptr->stat_struct.st_size;
        } else {
          fe_ptr->tagged = value;
          dir_entry->tagged_files--;
          dir_entry->tagged_bytes -= fe_ptr->stat_struct.st_size;
          s->disk_tagged_files--;
          s->disk_tagged_bytes -= fe_ptr->stat_struct.st_size;
        }
      }
    }
  }
  dir_entry->start_file = 0;
  dir_entry->cursor_pos = -1;
  DisplayFileWindow(ctx, p, dir_entry);
  RefreshWindow(p->pan_file_window);
  DisplayDiskStatistic(ctx, s);
  UpdateStatsPanel(ctx, dir_entry, s);
  return;
}
