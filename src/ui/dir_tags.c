/***************************************************************************
 *
 * src/ui/dir_tags.c
 * Directory-Level Tagging Operations (Tag/Untag single and all directories)
 *
 ***************************************************************************/

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
      PanelTags_RecordFileState(p, fe_ptr, value);
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
        PanelTags_RecordFileState(p, fe_ptr, value);
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

static void HandleInvertDirTags(ViewContext *ctx, DirEntry *dir_entry,
                                YtreePanel *p) {
  Statistic *s = &p->vol->vol_stats;
  FileEntry *fe_ptr;

  for (fe_ptr = dir_entry->file; fe_ptr; fe_ptr = fe_ptr->next) {
    if (ctx->hide_dot_files && fe_ptr->name[0] == '.')
      continue;

    if (!fe_ptr->matching)
      continue;

    fe_ptr->tagged = !fe_ptr->tagged;
    if (fe_ptr->tagged) {
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
    PanelTags_RecordFileState(p, fe_ptr, fe_ptr->tagged);
  }

  dir_entry->start_file = 0;
  dir_entry->cursor_pos = -1;
  DisplayFileWindow(ctx, p, dir_entry);
  RefreshWindow(p->pan_file_window);
  DisplayDiskStatistic(ctx, s);
  UpdateStatsPanel(ctx, dir_entry, s);
}

static void HandleDirTaggedOnlyToggle(ViewContext *ctx, DirEntry *dir_entry,
                                      YtreePanel *p) {
  if (dir_entry->tagged_files)
    dir_entry->tagged_flag = !dir_entry->tagged_flag;
  else
    dir_entry->tagged_flag = FALSE;

  BuildFileEntryList(ctx, p);
  dir_entry->start_file = 0;
  dir_entry->cursor_pos = 0;
  DisplayFileWindow(ctx, p, dir_entry);
  RefreshWindow(p->pan_file_window);
}

BOOL HandleDirTagActions(ViewContext *ctx, int action,
                         DirEntry **dir_entry_ptr, BOOL *need_dsp_help,
                         int *ch) {
  DirEntry *dir_entry = *dir_entry_ptr;
  switch (action) {
  case ACTION_TAG:
    HandleTagDir(ctx, dir_entry, TRUE, ctx->active);
    DirNav_Movedown(ctx, dir_entry_ptr, ctx->active);
    return TRUE;
  case ACTION_UNTAG:
    HandleTagDir(ctx, dir_entry, FALSE, ctx->active);
    DirNav_Movedown(ctx, dir_entry_ptr, ctx->active);
    return TRUE;
  case ACTION_TAG_ALL:
    HandleTagAllDirs(ctx, ctx->active->vol, dir_entry, TRUE, ctx->active);
    return TRUE;
  case ACTION_UNTAG_ALL:
    HandleTagAllDirs(ctx, ctx->active->vol, dir_entry, FALSE, ctx->active);
    return TRUE;
  case ACTION_INVERT:
    HandleInvertDirTags(ctx, dir_entry, ctx->active);
    if (need_dsp_help)
      *need_dsp_help = TRUE;
    return TRUE;
  case ACTION_TOGGLE_TAGGED_MODE:
    HandleDirTaggedOnlyToggle(ctx, dir_entry, ctx->active);
    if (need_dsp_help)
      *need_dsp_help = TRUE;
    return TRUE;
  case ACTION_CMD_TAGGED_S:
    HandleShowAll(ctx, TRUE, FALSE, dir_entry, need_dsp_help, ch, ctx->active);
    return TRUE;
  }
  return FALSE;
}
