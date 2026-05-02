#ifndef YTREE_PANEL_ANCHOR_H
#define YTREE_PANEL_ANCHOR_H

#include "ytree_defs.h"

BOOL CapturePanelAnchorPath(const YtreePanel *panel, const struct Volume *vol,
                            char *out_path, size_t out_path_size);
int FindDirIndexByPath(const struct Volume *vol, const char *path);
int FindDirIndexByPathOrAncestor(const struct Volume *vol, const char *path);
void PositionPanelAtIndex(YtreePanel *panel, int idx);
void RestorePanelAnchorPath(const struct Volume *vol, YtreePanel *panel,
                            const char *anchor_path);
DirEntry *FindDirByPathInTree(DirEntry *entry, const char *path);
void EnsurePanelAnchorVisible(ViewContext *ctx, const struct Volume *vol,
                              YtreePanel *panel, const char *label);
void DebugLogDirLoopState(const char *label, const ViewContext *ctx,
                          const DirEntry *dir_entry, int ch,
                          YtreeAction action, int unput_char);

#endif
