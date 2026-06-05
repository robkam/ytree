#ifndef YTREE_PANEL_ANCHOR_H
#define YTREE_PANEL_ANCHOR_H

#include "ytree_defs.h"

typedef struct {
  char selected_dir_path[PATH_LENGTH + 1];
  char top_dir_path[PATH_LENGTH + 1];
  BOOL has_selected_dir_path;
  BOOL has_top_dir_path;
} PanelViewportSnapshot;

BOOL CapturePanelAnchorPath(const YtreePanel *panel, const struct Volume *vol,
                            char *out_path, size_t out_path_size);
void CapturePanelViewportSnapshot(YtreePanel *panel, const struct Volume *vol,
                                  PanelViewportSnapshot *snapshot);
int FindDirIndexByPath(const struct Volume *vol, const char *path);
int FindDirIndexByPathOrAncestor(const struct Volume *vol, const char *path);
DirEntry *ResolvePanelAnchorTarget(const YtreePanel *panel,
                                   const struct Volume *vol,
                                   const char *anchor_path);
void PositionPanelAtIndex(YtreePanel *panel, int idx);
BOOL RestorePanelViewportSnapshot(const struct Volume *vol, YtreePanel *panel,
                                  const PanelViewportSnapshot *snapshot,
                                  const char *preferred_top_path);
void RememberPanelViewportTop(YtreePanel *panel);
void RestorePanelAnchorPath(const struct Volume *vol, YtreePanel *panel,
                            const char *anchor_path);
void DonatePanelState(YtreePanel *dst, const YtreePanel *src);
DirEntry *FindDirByPathInTree(DirEntry *entry, const char *path);
void EnsurePanelAnchorVisible(ViewContext *ctx, const struct Volume *vol,
                              YtreePanel *panel, const char *label);
void DebugLogDirLoopState(const char *label, const ViewContext *ctx,
                          const DirEntry *dir_entry, int ch,
                          YtreeAction action, int unput_char);

#endif
