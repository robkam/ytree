/***************************************************************************
 *
 * src/ui/fileinfo_band.c
 * Unified FileInfo band action helpers.
 *
 ***************************************************************************/

#include "ytnova_appstate_layout.h"
#include "ytnova_appstate_mode.h"
#include "ytnova_appstate_panel.h"
#include "ytnova_ui.h"

#include <stdlib.h>

static void SyncActivePanelFileInfoMirrors(ViewContext *ctx) {
  if (!ctx || !ctx->active)
    return;

  ctx->dir_mode = ctx->active->dir_mode;
  ctx->fixed_col_width = ctx->active->fixed_col_width;
}

static BOOL UseSeparateDirFileViews(const ViewContext *ctx) {
  if (!ctx)
    return FALSE;

  return (strtol(GetProfileValue(ctx, "SEPARATE_DIR_FILE_VIEWS"), NULL, 0))
             ? TRUE
             : FALSE;
}

int FileInfoActionSelection(YtreeNovaAction action) {
  switch (action) {
  case ACTION_FILEINFO_1:
    return 1;
  case ACTION_FILEINFO_2:
    return 2;
  case ACTION_FILEINFO_3:
    return 3;
  case ACTION_FILEINFO_4:
    return 4;
  case ACTION_FILEINFO_5:
    return 5;
  case ACTION_FILEINFO_6:
    return 6;
  case ACTION_FILEINFO_7:
    return 7;
  case ACTION_FILEINFO_8:
    return 8;
  case ACTION_FILEINFO_9:
    return 9;
  case ACTION_FILEINFO_0:
    return 0;
  default:
    return -1;
  }
}

static BOOL TogglePanelSizeUnits(YtreeNovaPanel *panel) {
  return AppStateCommitPanelSizeUnitMode(panel, !panel->human_size_units);
}

static BOOL SyncPanelSymlinkTargetsToFileMode(YtreeNovaPanel *panel) {
  if (!panel)
    return FALSE;
  return AppStateCommitPanelSymlinkTargetMode(panel,
                                              panel->file_mode == MODE_1);
}

static BOOL TogglePanelOverlayMode(YtreeNovaPanel *panel, int overlay_mode) {
  int next_mode = FILEINFO_OVERLAY_NONE;

  if (!panel)
    return FALSE;
  if (panel->fileinfo_overlay_mode != overlay_mode)
    next_mode = overlay_mode;
  return AppStateCommitPanelFileInfoOverlayMode(panel, next_mode);
}

static BOOL ClearPanelCompactWidth(ViewContext *ctx,
                                   const YtreeNovaPanel *panel) {
  if (!ctx || !panel)
    return FALSE;
  if (panel->fixed_col_width == 0)
    return TRUE;
  return AppStateCommitFixedColumnWidth(ctx, 0);
}

static BOOL SelectVisibleOverlayMode(ViewContext *ctx, YtreeNovaPanel *panel,
                                     int overlay_mode) {
  if (!ctx || !panel)
    return FALSE;
  if (!ClearPanelCompactWidth(ctx, panel))
    return FALSE;
  return TogglePanelOverlayMode(panel, overlay_mode);
}

static BOOL TogglePanelBriefWidth(ViewContext *ctx, YtreeNovaPanel *panel,
                                  int compact_gate_mode) {
  int next_width;

  if (!ctx || !panel)
    return FALSE;
  if (compact_gate_mode != MODE_3)
    return TRUE;
  if (panel->fixed_col_width == 0 &&
      panel->fileinfo_overlay_mode != FILEINFO_OVERLAY_NONE &&
      !AppStateCommitPanelFileInfoOverlayMode(panel, FILEINFO_OVERLAY_NONE))
    return FALSE;
  if (panel->fixed_col_width == 0) {
    SetPanelFileMode(ctx, panel, MODE_3);
    if (!SyncPanelSymlinkTargetsToFileMode(panel))
      return FALSE;
  }
  next_width = (panel->fixed_col_width == 0)
                   ? ResolveCompactFileWidth(ctx, panel)
                   : 0;
  if (!AppStateCommitFixedColumnWidth(ctx, next_width))
    return FALSE;
  return TRUE;
}

static BOOL ResetPanelNamedViewState(ViewContext *ctx, YtreeNovaPanel *panel) {
  if (!ctx || !panel)
    return FALSE;
  if (!AppStateCommitPanelFileInfoOverlayMode(panel, FILEINFO_OVERLAY_NONE))
    return FALSE;
  if (!AppStateCommitFixedColumnWidth(ctx, 0))
    return FALSE;
  return TRUE;
}

static BOOL ApplyFileProjectionToggleSelection(ViewContext *ctx,
                                               YtreeNovaPanel *panel,
                                               int selection, int compact_gate_mode,
                                               const DirEntry *dir_entry) {
  BOOL is_disk_scope;

  if (!ctx || !panel)
    return FALSE;

  is_disk_scope = (ctx->view_mode == DISK_MODE || ctx->view_mode == USER_MODE);

  switch (selection) {
  case 5:
    return TogglePanelBriefWidth(ctx, panel, compact_gate_mode);
  case 6:
    return TogglePanelSizeUnits(panel);
  case 7:
    return SelectVisibleOverlayMode(ctx, panel, FILEINFO_OVERLAY_RICH);
  case 8:
    return SelectVisibleOverlayMode(ctx, panel, FILEINFO_OVERLAY_SUMMARY);
  case 9:
    if (!is_disk_scope || !dir_entry || dir_entry->global_flag ||
        dir_entry->tagged_flag)
      return TRUE;
    if (panel->fileinfo_overlay_mode != FILEINFO_OVERLAY_GIT &&
        !FileInfoGitRefresh(ctx, panel, dir_entry))
      return TRUE;
    return SelectVisibleOverlayMode(ctx, panel, FILEINFO_OVERLAY_GIT);
  case 0:
    return TRUE;
  default:
    return FALSE;
  }
}

BOOL FileInfoHandleDirAction(ViewContext *ctx, YtreeNovaAction action,
                             DirEntry *dir_entry, const Statistic *s) {
  int selection;

  if (!ctx || !ctx->active || !dir_entry || !s)
    return FALSE;

  selection = FileInfoActionSelection(action);
  if (selection < 0)
    return FALSE;

  if (ctx->preview_mode)
    return TRUE;

  switch (selection) {
  case 1:
  case 2:
  case 3:
  case 4:
    SelectDirMode(ctx, selection);
    if (!UseSeparateDirFileViews(ctx)) {
      SelectPanelFileMode(ctx, ctx->active, selection);
      if (!SyncPanelSymlinkTargetsToFileMode(ctx->active))
        return FALSE;
      if (!ResetPanelNamedViewState(ctx, ctx->active))
        return FALSE;
    }
    break;
  case 5:
  case 6:
  case 7:
  case 8:
  case 9:
  case 0:
    if (!ApplyFileProjectionToggleSelection(ctx, ctx->active, selection,
                                            ctx->active->dir_mode,
                                            dir_entry))
      return FALSE;
    break;
  default:
    return FALSE;
  }

  SyncActivePanelFileInfoMirrors(ctx);
  DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window, ctx->active->disp_begin_pos,
              ctx->active->disp_begin_pos + ctx->active->cursor_pos, TRUE);
  if (ctx->active->pan_file_window)
    DisplayFileWindow(ctx, ctx->active, dir_entry);
  DisplayDiskStatistic(ctx, s);
  UpdateStatsPanel(ctx, dir_entry, s);
  return TRUE;
}

BOOL FileInfoHandleFileAction(
    ViewContext *ctx, YtreeNovaAction action, DirEntry *dir_entry,
    const Statistic *s, int start_x, long *preview_line_offset_ptr,
    void (*update_preview)(ViewContext *, const DirEntry *)) {
  int selection;
  BOOL is_disk_scope;

  if (!ctx || !ctx->active || !dir_entry || !s)
    return FALSE;

  selection = FileInfoActionSelection(action);
  if (selection < 0)
    return FALSE;

  if (ctx->preview_mode)
    return TRUE;

  is_disk_scope = (ctx->view_mode == DISK_MODE || ctx->view_mode == USER_MODE);

  switch (selection) {
  case 1:
  case 2:
  case 3:
  case 4:
    if (selection == 4 && !is_disk_scope)
      return TRUE;
    SelectPanelFileMode(ctx, ctx->active, selection);
    if (!SyncPanelSymlinkTargetsToFileMode(ctx->active))
      return FALSE;
    if (!ResetPanelNamedViewState(ctx, ctx->active))
      return FALSE;
    if (!UseSeparateDirFileViews(ctx))
      SelectDirMode(ctx, selection);
    break;
  case 5:
  case 0:
  case 6:
  case 7:
  case 8:
  case 9:
    if (!ApplyFileProjectionToggleSelection(ctx, ctx->active, selection,
                                            ctx->active->file_mode,
                                            dir_entry))
      return FALSE;
    break;
  default:
    return FALSE;
  }

  SyncActivePanelFileInfoMirrors(ctx);
  FileNav_SyncGridMetrics(ctx);
  DisplayFileWindow(ctx, ctx->active, dir_entry);
  UpdateStatsPanel(ctx, dir_entry, s);
  if (preview_line_offset_ptr)
    *preview_line_offset_ptr = 0;
  if (update_preview && ctx->preview_mode)
    update_preview(ctx, dir_entry);
  return TRUE;
}
