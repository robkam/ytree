/***************************************************************************
 *
 * src/ui/runtime_ports.c
 * Runtime port providers for core/main orchestration callbacks
 *
 ***************************************************************************/

#include "ytree_cmd.h"
#include "ytree_fs.h"
#include "ytree_ui.h"

int RuntimePort_MainInit(ViewContext *ctx, const char *configuration_file,
                         const char *history_file) {
  return Init(ctx, configuration_file, history_file);
}

void RuntimePort_MainSetProfileValue(const ViewContext *ctx, char *name,
                                     const char *value) {
  SetProfileValue(ctx, name, value);
}

int RuntimePort_MainLogDisk(ViewContext *ctx, YtreePanel *panel, char *path) {
  return LogDisk(ctx, panel, path);
}

int RuntimePort_MainSetFilter(const char *filter_spec, Statistic *s) {
  return SetFilter(filter_spec, s);
}

void RuntimePort_MainRecalculateSysStats(ViewContext *ctx, Statistic *s) {
  RecalculateSysStats(ctx, s);
}

int RuntimePort_MainHandleDirWindow(ViewContext *ctx,
                                    const DirEntry *start_dir_entry) {
  return HandleDirWindow(ctx, start_dir_entry);
}

void RuntimePort_MainSuspendClock(ViewContext *ctx) { SuspendClock(ctx); }

void RuntimePort_MainShutdownCurses(ViewContext *ctx) { ShutdownCurses(ctx); }

void RuntimePort_MainVolumeFreeAll(ViewContext *ctx) { Volume_FreeAll(ctx); }
