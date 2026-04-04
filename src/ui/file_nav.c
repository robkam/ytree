/***************************************************************************
 *
 * src/ui/file_nav.c
 * File Window Navigation and Grid Metrics
 *
 ***************************************************************************/

#include "ytree_ui.h"
#include "ytree_fs.h"

static int GetSafeMaxDispFiles(const ViewContext *ctx) {
  if (!ctx || ctx->ctrl_file_max_disp_files < 1)
    return 1;
  return ctx->ctrl_file_max_disp_files;
}

static int GetSafeXStep(const ViewContext *ctx) {
  if (!ctx || ctx->ctrl_file_x_step < 1)
    return 1;
  return ctx->ctrl_file_x_step;
}

int FileNav_GetMaxDispFiles(const ViewContext *ctx) {
  return GetSafeMaxDispFiles(ctx);
}

int FileNav_GetXStep(const ViewContext *ctx) { return GetSafeXStep(ctx); }

void FileNav_UpdateHeaderPath(ViewContext *ctx, DirEntry *dir_entry) {
  char path[PATH_LENGTH];
  DirEntry *path_dir;

  if (!ctx || !ctx->active || !dir_entry)
    return;

  path_dir = dir_entry;

  if (ctx->active->file_count > 0 && ctx->active->file_entry_list) {
    int idx = dir_entry->start_file + dir_entry->cursor_pos;
    if (idx >= 0 && (unsigned int)idx < ctx->active->file_count) {
      FileEntry *fe = ctx->active->file_entry_list[idx].file;
      if (fe && fe->dir_entry)
        path_dir = fe->dir_entry;
    }
  }

  GetPath(path_dir, path);
  DisplayHeaderPath(ctx, path);
}

static void RefreshFileSelection(ViewContext *ctx, DirEntry *dir_entry,
                                 int start_x) {
  DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
               dir_entry->start_file + dir_entry->cursor_pos, start_x,
               ctx->ctx_file_window);
  FileNav_UpdateHeaderPath(ctx, dir_entry);
  ctx->active->start_file = dir_entry->start_file;
}

void FileNav_MoveDown(ViewContext *ctx, DirEntry *dir_entry, int start_x) {
  if (!ctx || !ctx->active || !dir_entry)
    return;

  Nav_MoveDown(&dir_entry->cursor_pos, &dir_entry->start_file,
               (int)ctx->active->file_count, GetSafeMaxDispFiles(ctx), 1);
  RefreshFileSelection(ctx, dir_entry, start_x);
}

void FileNav_MoveUp(ViewContext *ctx, DirEntry *dir_entry, int start_x) {
  if (!ctx || !ctx->active || !dir_entry)
    return;

  Nav_MoveUp(&dir_entry->cursor_pos, &dir_entry->start_file);
  RefreshFileSelection(ctx, dir_entry, start_x);
}

void FileNav_MoveRight(ViewContext *ctx, DirEntry *dir_entry, int *start_x) {
  int x_step;

  if (!ctx || !ctx->active || !dir_entry || !start_x)
    return;

  x_step = GetSafeXStep(ctx);
  if (x_step == 1) {
    (*start_x)++;
    DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                 dir_entry->start_file + dir_entry->cursor_pos, *start_x,
                 ctx->ctx_file_window);
    if (ctx->ctrl_file_hide_right <= 0)
      (*start_x)--;
  } else {
    int current_index = dir_entry->start_file + dir_entry->cursor_pos;
    int file_count = (int)ctx->active->file_count;

    if (current_index + x_step < file_count) {
      if (dir_entry->cursor_pos + x_step < GetSafeMaxDispFiles(ctx)) {
        dir_entry->cursor_pos += x_step;
      } else {
        dir_entry->start_file += x_step;
      }

      DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                   dir_entry->start_file + dir_entry->cursor_pos, *start_x,
                   ctx->ctx_file_window);
    }
  }

  FileNav_UpdateHeaderPath(ctx, dir_entry);
  ctx->active->start_file = dir_entry->start_file;
}

void FileNav_MoveLeft(ViewContext *ctx, DirEntry *dir_entry, int *start_x) {
  int x_step;

  if (!ctx || !ctx->active || !dir_entry || !start_x)
    return;

  x_step = GetSafeXStep(ctx);
  if (x_step == 1) {
    if (*start_x > 0)
      (*start_x)--;
    DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                 dir_entry->start_file + dir_entry->cursor_pos, *start_x,
                 ctx->ctx_file_window);
  } else {
    int current_index = dir_entry->start_file + dir_entry->cursor_pos;

    if (current_index - x_step >= 0) {
      if (dir_entry->cursor_pos - x_step >= 0) {
        dir_entry->cursor_pos -= x_step;
      } else {
        if ((dir_entry->start_file -= x_step) < 0)
          dir_entry->start_file = 0;
      }

      DisplayFiles(ctx, ctx->active, dir_entry, dir_entry->start_file,
                   dir_entry->start_file + dir_entry->cursor_pos, *start_x,
                   ctx->ctx_file_window);
    }
  }

  FileNav_UpdateHeaderPath(ctx, dir_entry);
  ctx->active->start_file = dir_entry->start_file;
}

void FileNav_PageDown(ViewContext *ctx, DirEntry *dir_entry, int start_x) {
  if (!ctx || !ctx->active || !dir_entry)
    return;

  Nav_PageDown(&dir_entry->cursor_pos, &dir_entry->start_file,
               (int)ctx->active->file_count, GetSafeMaxDispFiles(ctx));
  RefreshFileSelection(ctx, dir_entry, start_x);
}

void FileNav_PageUp(ViewContext *ctx, DirEntry *dir_entry, int start_x) {
  if (!ctx || !ctx->active || !dir_entry)
    return;

  Nav_PageUp(&dir_entry->cursor_pos, &dir_entry->start_file,
             GetSafeMaxDispFiles(ctx));
  RefreshFileSelection(ctx, dir_entry, start_x);
}

void FileNav_SyncGridMetrics(ViewContext *ctx) {
  int height;
  int max_column;

  if (!ctx || !ctx->active || !ctx->ctx_file_window)
    return;

  SetPanelFileMode(ctx, ctx->active, GetPanelFileMode(ctx->active));
  height = getmaxy(ctx->ctx_file_window);
  if (height < 1)
    height = 1;
  max_column = GetPanelMaxColumn(ctx->active);
  if (max_column < 1)
    max_column = 1;

  ctx->ctrl_file_x_step = (max_column > 1) ? height : 1;
  ctx->ctrl_file_max_disp_files = height * max_column;
  if (ctx->ctrl_file_max_disp_files < 1)
    ctx->ctrl_file_max_disp_files = 1;
}

void FileNav_RereadWindowSize(ViewContext *ctx, DirEntry *dir_entry) {
  if (!ctx || !ctx->active || !dir_entry)
    return;

  FileNav_SyncGridMetrics(ctx);

  if (dir_entry->start_file + dir_entry->cursor_pos <
      (int)ctx->active->file_count) {
    while (dir_entry->cursor_pos >= GetSafeMaxDispFiles(ctx)) {
      dir_entry->start_file += GetSafeXStep(ctx);
      dir_entry->cursor_pos -= GetSafeXStep(ctx);
    }
  }
}
