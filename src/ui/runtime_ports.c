/***************************************************************************
 *
 * src/ui/runtime_ports.c
 * Core init callback provider (ui-side)
 *
 ***************************************************************************/

#include "ytree_cmd.h"
#include "ytree_fs.h"
#include "ytree_ui.h"

static void CoreInit_StartColors(ViewContext *ctx) {
#ifdef COLOR_SUPPORT
  StartColors(ctx);
#else
  (void)ctx;
#endif
}

static void CoreInit_DialogInit(void) { UI_Dialog_Init(); }

static void CoreInit_ReinitColorPairs(ViewContext *ctx) {
#ifdef COLOR_SUPPORT
  ReinitColorPairs(ctx);
#else
  (void)ctx;
#endif
}

static void CoreInit_SetPanelFileMode(ViewContext *ctx, YtreePanel *panel,
                                      int new_file_mode) {
  SetPanelFileMode(ctx, panel, new_file_mode);
}

static void CoreInit_WbkgdSet(const ViewContext *ctx, WINDOW *win, chtype c) {
#ifdef COLOR_SUPPORT
  WbkgdSet(ctx, win, c);
#else
  (void)ctx;
  (void)win;
  (void)c;
#endif
}

static int CoreInit_UINotice(ViewContext *ctx, const char *msg) {
  return UI_Notice(ctx, "%s", msg);
}

static void CoreInit_ParseColorString(const char *color_str, int *fg, int *bg) {
#ifdef COLOR_SUPPORT
  ParseColorString(color_str, fg, bg);
#else
  (void)color_str;
  (void)fg;
  (void)bg;
#endif
}

static void CoreInit_UpdateUIColor(const char *name, int fg, int bg) {
#ifdef COLOR_SUPPORT
  UpdateUIColor(name, fg, bg);
#else
  (void)name;
  (void)fg;
  (void)bg;
#endif
}

static void CoreInit_AddFileColorRule(ViewContext *ctx, const char *pattern,
                                      int fg, int bg) {
#ifdef COLOR_SUPPORT
  AddFileColorRule(ctx, pattern, fg, bg);
#else
  (void)ctx;
  (void)pattern;
  (void)fg;
  (void)bg;
#endif
}

static void CoreInit_BindRuntimeHooks(ViewContext *ctx) {
  if (ctx == NULL)
    return;

  ctx->hook_scan_subtree = ScanSubTree;
  ctx->hook_remove_file = RemoveFile;
  ctx->hook_make_path = MakePath;
  ctx->hook_key_pressed = KeyPressed;
  ctx->hook_escape_key_pressed = EscapeKeyPressed;
  ctx->hook_input_choice = InputChoice;
  ctx->hook_quit = Quit;
  ctx->hook_ui_message = UI_Message;
  ctx->hook_ui_notice = UI_Notice;
  ctx->hook_draw_spinner = DrawSpinner;
  ctx->hook_clock_handler = ClockHandler;
  ctx->hook_draw_animation_step = DrawAnimationStep;
  ctx->hook_display_disk_statistic = DisplayDiskStatistic;
  ctx->hook_display_avail_bytes = DisplayAvailBytes;
  ctx->hook_display_menu = DisplayMenu;
  ctx->hook_build_dir_entry_list = BuildDirEntryList;
  ctx->hook_display_tree = DisplayTree;
  ctx->hook_switch_to_big_file_window = SwitchToBigFileWindow;
  ctx->hook_init_animation = InitAnimation;
  ctx->hook_refresh_window = RefreshWindow;
  ctx->hook_stop_animation = StopAnimation;
  ctx->hook_switch_to_small_file_window = SwitchToSmallFileWindow;
  ctx->hook_clear_help = ClearHelp;
  ctx->hook_mv_add_str = MvAddStr;
  ctx->hook_read_string = UI_ReadString;
  ctx->hook_recreate_windows = ReCreateWindows;
  ctx->hook_hit_return_to_continue = HitReturnToContinue;
  ctx->hook_recalculate_sys_stats = RecalculateSysStats;
  ctx->hook_suspend_clock = SuspendClock;
  ctx->hook_init_clock = InitClock;
  ctx->core_quit_ops.confirm_quit = UI_CoreQuitConfirm;
  ctx->core_quit_ops.save_history = UI_CoreQuitSaveHistory;
  ctx->core_quit_ops.close_watcher = UI_CoreQuitCloseWatcher;
  ctx->core_quit_ops.cleanup_volume_tree = UI_CoreQuitCleanupVolumeTree;
  ctx->core_quit_ops.suspend_clock = UI_CoreQuitSuspendClock;
  ctx->core_quit_ops.shutdown_terminal = UI_CoreQuitShutdownTerminal;
}

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

void CoreInitOps_RegisterUIRuntime(CoreInitOps *ops) {
  if (ops == NULL)
    return;

  ops->start_colors = CoreInit_StartColors;
  ops->dialog_init = CoreInit_DialogInit;
  ops->reinit_color_pairs = CoreInit_ReinitColorPairs;
  ops->set_panel_file_mode = CoreInit_SetPanelFileMode;
  ops->wbkgd_set = CoreInit_WbkgdSet;
  ops->ui_notice = CoreInit_UINotice;
  ops->parse_color_string = CoreInit_ParseColorString;
  ops->update_ui_color = CoreInit_UpdateUIColor;
  ops->add_file_color_rule = CoreInit_AddFileColorRule;
  ops->bind_runtime_hooks = CoreInit_BindRuntimeHooks;
}
