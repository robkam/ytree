#ifndef YTNOVA_PANEL_ANCHOR_H
#define YTNOVA_PANEL_ANCHOR_H

#include "ytnova_defs.h"

typedef struct {
  char selected_dir_path[PATH_LENGTH + 1];
  char top_dir_path[PATH_LENGTH + 1];
  BOOL has_selected_dir_path;
  BOOL has_top_dir_path;
} PanelViewportSnapshot;

BOOL CapturePanelAnchorPath(const YtreeNovaPanel *panel, const struct Volume *vol,
                            char *out_path, size_t out_path_size);
void CapturePanelViewportSnapshot(YtreeNovaPanel *panel, const struct Volume *vol,
                                  PanelViewportSnapshot *snapshot);
PanelVolumeFileState *FindPanelVolumeFileState(YtreeNovaPanel *panel,
                                               int volume_id);
PanelVolumeFileState *GetPanelVolumeFileState(YtreeNovaPanel *panel,
                                              int volume_id);
void SavePanelTreeViewportSnapshot(YtreeNovaPanel *panel);
void ResetPanelTreeViewportSnapshot(YtreeNovaPanel *panel);
int FindDirIndexByPath(const struct Volume *vol, const char *path);
int FindDirIndexByPathOrAncestor(const struct Volume *vol, const char *path);
DirEntry *ResolvePanelAnchorTarget(const YtreeNovaPanel *panel,
                                   const struct Volume *vol,
                                   const char *anchor_path);
void PositionPanelAtIndex(YtreeNovaPanel *panel, int idx);
BOOL RestorePanelViewportSnapshot(const struct Volume *vol, YtreeNovaPanel *panel,
                                  const PanelViewportSnapshot *snapshot,
                                  const char *preferred_top_path);
BOOL RestorePanelTreeViewportSnapshot(ViewContext *ctx, YtreeNovaPanel *panel);
void RememberPanelViewportTop(YtreeNovaPanel *panel);
void RestorePanelAnchorPath(const struct Volume *vol, YtreeNovaPanel *panel,
                            const char *anchor_path);
void DonatePanelState(YtreeNovaPanel *dst, const YtreeNovaPanel *src);
DirEntry *FindDirByPathInTree(DirEntry *entry, const char *path);
void EnsurePanelAnchorVisible(ViewContext *ctx, const struct Volume *vol,
                              YtreeNovaPanel *panel, const char *label);
void DebugLogDirLoopState(const char *label, const ViewContext *ctx,
                          const DirEntry *dir_entry, int ch,
                          YtreeNovaAction action, int unput_char);

#endif
