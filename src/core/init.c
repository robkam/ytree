/***************************************************************************
 *
 * src/core/init.c
 * Application Initialization and Layout Management
 *
 * Handles ncurses startup, configuration loading (profile/history),
 * and dynamic window geometry calculation.
 *
 ***************************************************************************/

#include "config.h"
#include "watcher.h"
#include "ytree_cmd.h"
#include "ytree_debug.h"
#include "ytree_fs.h"
#include "ytree_ui.h"

#define SORT_BY_NAME 1

#define F2_WINDOW_X(ctx) ((ctx)->layout.dir_win_x)
#define F2_WINDOW_Y(ctx) ((ctx)->layout.dir_win_y)
#define F2_WINDOW_WIDTH(ctx) ((ctx)->layout.dir_win_width)
#define F2_WINDOW_HEIGHT(ctx) ((ctx)->layout.dir_win_height + 1)

#define ERROR_WINDOW_WIDTH 40
#define ERROR_WINDOW_HEIGHT 10
#define ERROR_WINDOW_X ((COLS - ERROR_WINDOW_WIDTH) >> 1)
#define ERROR_WINDOW_Y ((LINES - ERROR_WINDOW_HEIGHT) >> 1)

#define HISTORY_WINDOW_X 1
#define HISTORY_WINDOW_Y 2
#define HISTORY_WINDOW_WIDTH(ctx) ((ctx)->layout.main_win_width)
#define HISTORY_WINDOW_HEIGHT (LINES - 6)

#define TIME_WINDOW_X (COLS - 20)
#define TIME_WINDOW_Y 0
#define TIME_WINDOW_WIDTH 20
#define TIME_WINDOW_HEIGHT 1

static WINDOW *Subwin(WINDOW *orig, int nlines, int ncols, int begin_y,
                      int begin_x);
static WINDOW *Newwin(int nlines, int ncols, int begin_y, int begin_x);

#ifdef XCURSES
char *XCursesProgramName = "ytree";
#endif

void Layout_Recalculate(ViewContext *ctx) {
  if (!ctx)
    return;

  /* Centralize UI vertical geometry */
  ctx->layout.header_y = 0;
  ctx->layout.message_y = LINES - 3;
  ctx->layout.prompt_y = LINES - 2;
  ctx->layout.status_y = LINES - 1;
  ctx->layout.bottom_border_y = LINES - 4;

  /*
   * Calculate available vertical space for windows.
   * Top Border is at row 1. Windows start at row 2.
   * Bottom Border is at row LINES-4. Windows end at row LINES-5.
   * Available height = (LINES - 4) - 2 = LINES - 6.
   */
  int available_height = LINES - 6;
  if (available_height < 1)
    available_height = 1;

  /*
   * Preview Mode Logic:
   * If Preview Mode is active, we override the standard layout.
   * Left Panel: Narrow File List (approx 20% width).
   * Right Panel: Preview Window (Remaining width).
   * Stats and Directory Tree are hidden.
   */
  if (ctx->preview_mode) {
    ctx->layout.stats_width = 0;
    ctx->layout.dir_win_height = 0; /* Hidden */

    /* Calculate File List Width (20% of COLS, min 16 chars) */
    int file_list_width = COLS * 0.20;
    if (file_list_width < 16)
      file_list_width = 16;
    /* Ensure it doesn't take up the whole screen */
    if (file_list_width > COLS - 4)
      file_list_width = COLS - 4;

    ctx->layout.main_win_width = COLS - 2; /* Full width for history etc */

    /* Left Panel (File List) Geometry - Uses Big File Window slots */
    ctx->layout.big_file_win_x = 1;
    ctx->layout.big_file_win_y = 2;
    ctx->layout.big_file_win_width = file_list_width;
    ctx->layout.big_file_win_height = available_height;

    /* Unused windows in this mode, set to minimal valid values */
    ctx->layout.dir_win_x = 1;
    ctx->layout.dir_win_y = 2;
    ctx->layout.dir_win_width = file_list_width;
    ctx->layout.dir_win_height = 0; /* Will be clamped to 1 by Subwin */

    ctx->layout.small_file_win_x = 1;
    ctx->layout.small_file_win_y = 2;
    ctx->layout.small_file_win_width = file_list_width;
    ctx->layout.small_file_win_height = 0;

    /* Preview Window Geometry */
    ctx->layout.preview_win_x = file_list_width + 2;
    ctx->layout.preview_win_y = 2;
    ctx->layout.preview_win_width = COLS - file_list_width - 3;
    ctx->layout.preview_win_height = available_height;

    if (ctx->layout.preview_win_width < 1)
      ctx->layout.preview_win_width = 1;

    /* Update ActivePanel Geometry */
    if (ctx->active) {
      ctx->active->dir_x = ctx->layout.dir_win_x;
      ctx->active->dir_y = ctx->layout.dir_win_y;
      ctx->active->dir_w = ctx->layout.dir_win_width;
      ctx->active->dir_h = ctx->layout.dir_win_height;

      ctx->active->small_file_x = ctx->layout.small_file_win_x;
      ctx->active->small_file_y = ctx->layout.small_file_win_y;
      ctx->active->small_file_w = ctx->layout.small_file_win_width;
      ctx->active->small_file_h = ctx->layout.small_file_win_height;

      ctx->active->big_file_x = ctx->layout.big_file_win_x;
      ctx->active->big_file_y = ctx->layout.big_file_win_y; /* FIXED */
      ctx->active->big_file_w = ctx->layout.big_file_win_width;
      ctx->active->big_file_h = available_height;
    }

    /* Inactive Panel is not used for file listing in Preview Mode */
    return;
  }

  /*
   * Stats Panel Logic:
   * If SplitScreen is active, force stats off to save space.
   * Otherwise respect user preference.
   */
  if (ctx->is_split_screen) {
    ctx->layout.stats_width = 0;
  } else {
    if (ctx->show_stats) {
      ctx->layout.stats_width = 24;
    } else {
      ctx->layout.stats_width = 0;
    }
  }

  /* Removed unused stats_margin calculation */
  ctx->layout.main_win_width = (ctx->layout.stats_width > 0)
                                   ? (COLS - ctx->layout.stats_width - 2)
                                   : (COLS - 2);

  /* Left Panel Geometry (Always active) */
  int panel_width;
  if (ctx->is_split_screen) {
    /* Reserve space for separator (the -1) */
    panel_width = (ctx->layout.main_win_width - 1) / 2;
  } else {
    panel_width = ctx->layout.main_win_width;
  }

  /* Common Heights */
  int dir_h = (available_height * 6) / 10;
  if (dir_h < 1)
    dir_h = 1;
  int small_file_h = available_height - dir_h - 1;
  if (small_file_h < 1)
    small_file_h = 1;

  /* Combined height of both stacked panels must fit in available_height */
  ctx->layout.dir_win_height = dir_h;
  ctx->layout.small_file_win_height = small_file_h;

  /* Left Panel Geometry: STRICT NESTING
     Left border at x=0, Mid/Right border at main_win_width+1.
     Inner window x=1, width=panel_width.
  */
  ctx->layout.dir_win_x = 1;
  ctx->layout.dir_win_y = 2; /* Row 0=Header, Row 1=Top Border */
  ctx->layout.dir_win_width = panel_width;

  ctx->layout.small_file_win_x = 1;
  ctx->layout.small_file_win_y =
      ctx->layout.dir_win_y + dir_h + 1; /* +1 for horizontal separator */
  ctx->layout.small_file_win_width = panel_width;

  ctx->layout.big_file_win_x = 1;
  ctx->layout.big_file_win_y = 2;
  ctx->layout.big_file_win_width = panel_width;
  ctx->layout.big_file_win_height = available_height;

  /* Update LeftPanel Geometry */
  if (ctx->left) {
    ctx->left->dir_x = ctx->layout.dir_win_x;
    ctx->left->dir_y = ctx->layout.dir_win_y;
    ctx->left->dir_w = ctx->layout.dir_win_width;
    ctx->left->dir_h = ctx->layout.dir_win_height;

    ctx->left->small_file_x = ctx->layout.small_file_win_x;
    ctx->left->small_file_y = ctx->layout.small_file_win_y;
    ctx->left->small_file_w = ctx->layout.small_file_win_width;
    ctx->left->small_file_h = ctx->layout.small_file_win_height;

    ctx->left->big_file_x = ctx->layout.big_file_win_x;
    ctx->left->big_file_y = ctx->layout.big_file_win_y;
    ctx->left->big_file_w = ctx->layout.big_file_win_width;
    ctx->left->big_file_h = available_height;
  }

  /* Update RightPanel Geometry */
  if (ctx->right && ctx->is_split_screen) {
    /* Right panel starts after left panel + vertical separator */
    int right_x = ctx->layout.dir_win_x + panel_width + 1;
    int right_w = ctx->layout.main_win_width - panel_width - 1;

    ctx->right->dir_x = right_x;
    ctx->right->dir_y = 2;
    ctx->right->dir_w = right_w;
    ctx->right->dir_h = dir_h;

    ctx->right->small_file_x = right_x;
    ctx->right->small_file_y = ctx->right->dir_y + dir_h + 1;
    ctx->right->small_file_w = right_w;
    ctx->right->small_file_h = small_file_h;

    ctx->right->big_file_x = right_x;
    ctx->right->big_file_y = 2;
    ctx->right->big_file_w = right_w;
    ctx->right->big_file_h = available_height;
  }
}

void InitView(ViewContext *ctx) {
  DEBUG_LOG("ENTER InitView");
  memset(ctx, 0, sizeof(ViewContext));
  ctx->viewer.inhex = TRUE;
  ctx->view_mode = DISK_MODE;
  ctx->dir_mode = MODE_3;
  ctx->is_split_screen = FALSE;
  ctx->focused_window = FOCUS_TREE;

  /* Initialize Panels */
  ctx->left = (YtreePanel *)calloc(1, sizeof(YtreePanel));
  if (ctx->left == NULL) {
    fprintf(stderr, "InitView: failed to allocate left panel\n");
    exit(1);
  }
  DEBUG_LOG("InitView: setup left panel=%p", (void *)ctx->left);
  ctx->left->file_mode = MODE_1;
  ctx->left->saved_focus = FOCUS_TREE;
  ctx->left->file_cursor_pos = 0;
  ctx->left->file_dir_entry = NULL;
  ctx->left->saved_big_file_view = FALSE;

  ctx->right = (YtreePanel *)calloc(1, sizeof(YtreePanel));
  if (ctx->right == NULL) {
    fprintf(stderr, "InitView: failed to allocate right panel\n");
    free(ctx->left);
    ctx->left = NULL;
    exit(1);
  }
  DEBUG_LOG("InitView: setup right panel=%p", (void *)ctx->right);
  ctx->right->file_mode = MODE_1;
  ctx->right->saved_focus = FOCUS_TREE;
  ctx->right->file_cursor_pos = 0;
  ctx->right->file_dir_entry = NULL;
  ctx->right->saved_big_file_view = FALSE;

  ctx->active = ctx->left;

  DEBUG_LOG("EXIT InitView");
}

void ReCreateWindows(ViewContext *ctx) {
  DEBUG_LOG("ENTER ReCreateWindows");
  if (ctx == NULL || ctx->left == NULL || ctx->right == NULL)
    return;
  if (ctx->active == NULL)
    ctx->active = ctx->left;
  /* 1. Recalculate Layout based on current SplitScreen state */
  Layout_Recalculate(ctx);

  BOOL left_is_big = FALSE;
  BOOL right_is_big = FALSE;

  /* Capture INDEPENDENT Panel Mode State */
  /* If panels exist, check if they are currently zoomed */
  if (ctx->left && ctx->left->pan_file_window) {
    if (ctx->left->pan_file_window == ctx->left->pan_big_file_window)
      left_is_big = TRUE;
  }
  if (ctx->right && ctx->right->pan_file_window) {
    if (ctx->right->pan_file_window == ctx->right->pan_big_file_window)
      right_is_big = TRUE;
  }

  /* Force 'big' (zoom) mode for ActivePanel if Preview Mode is active */
  if (ctx->preview_mode) {
    if (ctx->active == ctx->left)
      left_is_big = TRUE;
    if (ctx->active == ctx->right)
      right_is_big = TRUE;
  }

  /* 2. Cleanup: Destroy ALL existing panel windows */
  if (ctx->left->pan_dir_window) {
    delwin(ctx->left->pan_dir_window);
    ctx->left->pan_dir_window = NULL;
  }
  if (ctx->left->pan_small_file_window) {
    delwin(ctx->left->pan_small_file_window);
    ctx->left->pan_small_file_window = NULL;
  }
  if (ctx->left->pan_big_file_window) {
    delwin(ctx->left->pan_big_file_window);
    ctx->left->pan_big_file_window = NULL;
  }

  if (ctx->right->pan_dir_window) {
    delwin(ctx->right->pan_dir_window);
    ctx->right->pan_dir_window = NULL;
  }
  if (ctx->right->pan_small_file_window) {
    delwin(ctx->right->pan_small_file_window);
    ctx->right->pan_small_file_window = NULL;
  }
  if (ctx->right->pan_big_file_window) {
    delwin(ctx->right->pan_big_file_window);
    ctx->right->pan_big_file_window = NULL;
  }

  /* Cleanup Preview Window */
  if (ctx->ctx_preview_window) {
    delwin(ctx->ctx_preview_window);
    ctx->ctx_preview_window = NULL;
  }

  /* 3. Create Primary Panel Windows (Always Created) */
  YtreePanel *primary = (ctx->preview_mode) ? ctx->active : ctx->left;

  primary->pan_dir_window = Subwin(stdscr, primary->dir_h, primary->dir_w,
                                   primary->dir_y, primary->dir_x);
  keypad(primary->pan_dir_window, TRUE);
  scrollok(primary->pan_dir_window, TRUE);

  leaveok(primary->pan_dir_window, TRUE);
  WbkgdSet(ctx, primary->pan_dir_window, COLOR_PAIR(CPAIR_WINDIR));

  primary->pan_small_file_window =
      Subwin(stdscr, primary->small_file_h, primary->small_file_w,
             primary->small_file_y, primary->small_file_x);
  keypad(primary->pan_small_file_window, TRUE);

  leaveok(primary->pan_small_file_window, TRUE);
  WbkgdSet(ctx, primary->pan_small_file_window, COLOR_PAIR(CPAIR_WINFILE));

  primary->pan_big_file_window =
      Subwin(stdscr, primary->big_file_h, primary->big_file_w,
             primary->big_file_y, primary->big_file_x);
  keypad(primary->pan_big_file_window, TRUE);

  leaveok(primary->pan_big_file_window, TRUE);
  WbkgdSet(ctx, primary->pan_big_file_window, COLOR_PAIR(CPAIR_WINFILE));

  /* Determine current file window for primary based on its OWN state */
  primary->pan_file_window = (primary == ctx->left ? left_is_big : right_is_big)
                                 ? primary->pan_big_file_window
                                 : primary->pan_small_file_window;

  /* 4. Create Right Panel Windows (Only if Split Screen and NOT Preview Mode)
   */
  if (ctx->is_split_screen && !ctx->preview_mode) {
    ctx->right->pan_dir_window =
        Subwin(stdscr, ctx->right->dir_h, ctx->right->dir_w, ctx->right->dir_y,
               ctx->right->dir_x);
    keypad(ctx->right->pan_dir_window, TRUE);
    scrollok(ctx->right->pan_dir_window, TRUE);

    leaveok(ctx->right->pan_dir_window, TRUE);
    WbkgdSet(ctx, ctx->right->pan_dir_window, COLOR_PAIR(CPAIR_WINDIR));

    ctx->right->pan_small_file_window =
        Subwin(stdscr, ctx->right->small_file_h, ctx->right->small_file_w,
               ctx->right->small_file_y, ctx->right->small_file_x);
    keypad(ctx->right->pan_small_file_window, TRUE);

    leaveok(ctx->right->pan_small_file_window, TRUE);
    WbkgdSet(ctx, ctx->right->pan_small_file_window, COLOR_PAIR(CPAIR_WINFILE));

    ctx->right->pan_big_file_window =
        Subwin(stdscr, ctx->right->big_file_h, ctx->right->big_file_w,
               ctx->right->big_file_y, ctx->right->big_file_x);
    keypad(ctx->right->pan_big_file_window, TRUE);

    leaveok(ctx->right->pan_big_file_window, TRUE);
    WbkgdSet(ctx, ctx->right->pan_big_file_window, COLOR_PAIR(CPAIR_WINFILE));

    /* Determine current file window for RightPanel based on its OWN state */
    ctx->right->pan_file_window = (right_is_big)
                                      ? ctx->right->pan_big_file_window
                                      : ctx->right->pan_small_file_window;
  }

  /* 5. Create Preview Window (If Preview Mode) */
  if (ctx->preview_mode) {
    ctx->ctx_preview_window =
        Newwin(ctx->layout.preview_win_height, ctx->layout.preview_win_width,
               ctx->layout.preview_win_y, ctx->layout.preview_win_x);
    if (ctx->ctx_preview_window) {
      keypad(ctx->ctx_preview_window, TRUE);

      leaveok(ctx->ctx_preview_window, TRUE);
      WbkgdSet(ctx, ctx->ctx_preview_window, COLOR_PAIR(CPAIR_WINFILE));
    }
  }

  /* 6. Sync ctx->active Pointers */
  if (ctx->active == NULL)
    ctx->active = ctx->left;

  if (ctx->active == ctx->left) {
    ctx->active->pan_dir_window = ctx->left->pan_dir_window;
    ctx->active->pan_small_file_window = ctx->left->pan_small_file_window;
    ctx->active->pan_big_file_window = ctx->left->pan_big_file_window;
    ctx->active->pan_file_window = ctx->left->pan_file_window;
  } else if (ctx->active == ctx->right &&
             (ctx->is_split_screen || ctx->preview_mode)) {
    /* In Preview Mode, RightPanel might not have windows, but ActivePanel
     * should still point to it if it was selected. Layout_Recalculate handles
     * hiding the right-side UI, but we shouldn't force-switch focus to Left.
     */
    ctx->active->pan_dir_window = ctx->right->pan_dir_window;
    ctx->active->pan_small_file_window = ctx->right->pan_small_file_window;
    ctx->active->pan_big_file_window = ctx->right->pan_big_file_window;
    ctx->active->pan_file_window = ctx->right->pan_file_window;
  } else {
    /* Fallback if something went wrong (e.g. RightPanel active but
     * SplitScreen/Preview disabled) */
    ctx->active = ctx->left;
    ctx->active->pan_dir_window = ctx->left->pan_dir_window;
    ctx->active->pan_small_file_window = ctx->left->pan_small_file_window;
    ctx->active->pan_big_file_window = ctx->left->pan_big_file_window;
    ctx->active->pan_file_window = ctx->left->pan_file_window;
  }

  /* 7. Sync Global View Context */
  ctx->ctx_dir_window = ctx->active->pan_dir_window;
  ctx->ctx_small_file_window = ctx->active->pan_small_file_window;
  ctx->ctx_big_file_window = ctx->active->pan_big_file_window;
  ctx->ctx_file_window = ctx->active->pan_file_window;

  /* 8. Utility Windows */
  if (ctx->ctx_border_window)
    delwin(ctx->ctx_border_window);

  ctx->ctx_border_window = Newwin(LINES, COLS, 0, 0);
  if (ctx->ctx_border_window) {
    WbkgdSet(ctx, ctx->ctx_border_window, COLOR_PAIR(CPAIR_BORDERS));

    leaveok(ctx->ctx_border_window, TRUE);

    /* Header Path Window: subwindow of border */
    if (ctx->ctx_path_window)
      delwin(ctx->ctx_path_window);
    ctx->ctx_path_window = Subwin(ctx->ctx_border_window, 1, COLS - 26, 0, 6);
    leaveok(ctx->ctx_path_window, TRUE);
  }

  if (ctx->ctx_error_window)
    delwin(ctx->ctx_error_window);

  ctx->ctx_error_window = Newwin(ERROR_WINDOW_HEIGHT, ERROR_WINDOW_WIDTH,
                                 ERROR_WINDOW_Y, ERROR_WINDOW_X);
  WbkgdSet(ctx, ctx->ctx_error_window, COLOR_PAIR(CPAIR_WINERR));

  leaveok(ctx->ctx_error_window, TRUE);

#ifdef CLOCK_SUPPORT

  if (ctx->ctx_time_window)
    delwin(ctx->ctx_time_window);

  ctx->ctx_time_window =
      Subwin(ctx->ctx_border_window, TIME_WINDOW_HEIGHT, TIME_WINDOW_WIDTH,
             TIME_WINDOW_Y, TIME_WINDOW_X);

  scrollok(ctx->ctx_time_window, FALSE);
  leaveok(ctx->ctx_time_window, TRUE);
  WbkgdSet(ctx, ctx->ctx_time_window, COLOR_PAIR(CPAIR_WINDIR | A_BOLD));
  /* Keep clock redraws in the normal wnoutrefresh/doupdate pipeline.
   * Immediate refresh causes visible blinking during rapid navigation.
   */
  immedok(ctx->ctx_time_window, FALSE);
#endif

  if (ctx->ctx_history_window)
    delwin(ctx->ctx_history_window);

  ctx->ctx_history_window =
      Newwin(HISTORY_WINDOW_HEIGHT, HISTORY_WINDOW_WIDTH(ctx), HISTORY_WINDOW_Y,
             HISTORY_WINDOW_X);
  scrollok(ctx->ctx_history_window, TRUE);

  leaveok(ctx->ctx_history_window, TRUE);
  WbkgdSet(ctx, ctx->ctx_history_window, COLOR_PAIR(CPAIR_WINHST));

  ctx->ctx_matches_window = ctx->ctx_history_window;

  if (ctx->ctx_menu_window)
    delwin(ctx->ctx_menu_window);

  /* Menu area: Y_MESSAGE down to bottom (typically 3 lines) */
  ctx->ctx_menu_window = Newwin(3, COLS, ctx->layout.message_y, 0);
  if (ctx->ctx_menu_window) {
    WbkgdSet(ctx, ctx->ctx_menu_window, COLOR_PAIR(CPAIR_MENU));

    leaveok(ctx->ctx_menu_window, TRUE);
  }

  if (ctx->ctx_f2_window)
    delwin(ctx->ctx_f2_window);

  ctx->ctx_f2_window = Newwin(F2_WINDOW_HEIGHT(ctx), F2_WINDOW_WIDTH(ctx),
                              F2_WINDOW_Y(ctx), F2_WINDOW_X(ctx));

  keypad(ctx->ctx_f2_window, TRUE);
  scrollok(ctx->ctx_f2_window, FALSE);

  leaveok(ctx->ctx_f2_window, TRUE);
  WbkgdSet(ctx, ctx->ctx_f2_window, COLOR_PAIR(CPAIR_WINHST));
  DEBUG_LOG("EXIT ReCreateWindows");
}

void ShutdownCurses(ViewContext *ctx) {
  SCREEN *screen = (ctx != NULL) ? ctx->curses_screen : NULL;

  if (screen != NULL)
    set_term(screen);

  endwin();

  if (screen != NULL) {
    delscreen(screen);
    ctx->curses_screen = NULL;
  }
}

int Init(ViewContext *ctx, const char *configuration_file,
         const char *history_file) {
  InitView(ctx);
  DEBUG_LOG("ENTER Init");
  char buffer[PATH_LENGTH + 1];
  const char *home = NULL;

  /* ctx already assigned in main.c */

  /* Initial Panel Defaults */
  ctx->is_split_screen = FALSE;
  ctx->active = ctx->left;
  /* Explicitly initialize panel file lists to zero */
  ctx->left->file_entry_list = NULL;
  ctx->left->file_entry_list_capacity = 0;
  ctx->left->file_count = 0;

  ctx->right->file_entry_list = NULL;
  ctx->right->file_entry_list_capacity = 0;
  ctx->right->file_count = 0;

  /* Initialize Panel Defaults for Rendering */
  ctx->left->file_mode = MODE_1;
  ctx->left->max_column = 1;

  ctx->right->file_mode = MODE_1;
  ctx->right->max_column = 1;

  /* Allocate and initialize the first volume using the dedicated module */
  struct Volume *initial_vol = Volume_Create(ctx);
  /* Assign initial volume to ActivePanel */
  ctx->active = ctx->left; /* Default active panel */
  ctx->active->vol = initial_vol;

  ctx->show_stats = TRUE;
  ctx->fixed_col_width = 0; /* ADDED */
  ctx->refresh_mode =
      0; /* Will be set after ReadProfile initializes profile_data */
  ctx->preview_mode = FALSE; /* Initialize Preview Mode */
  ctx->ctx_preview_window = NULL;

  /* Initialize global mode default */
  ctx->view_mode = DISK_MODE;

  /* Use setlocale to correctly initialize for WITH_UTF8 or system locale */
  setlocale(LC_ALL, "");

  ctx->user_umask = umask(0);
  setenv("ESCDELAY", "25", 1);
  ctx->curses_screen = newterm(NULL, stdout, stdin);
  if (ctx->curses_screen == NULL) {
    fprintf(stderr, "Init: failed to initialize terminal\n");
    return (1);
  }
  set_term(ctx->curses_screen);
  ctx->cached_lines = LINES;
  ctx->cached_cols = COLS;
  Layout_Recalculate(ctx);
  StartColors(ctx); /* even on b/w terminals... */

  UI_Dialog_Init(); /* Initialize Dialog Manager */

  cbreak();
  noecho();
  nonl();
  raw();
  keypad(stdscr, TRUE);
  clearok(stdscr, TRUE);
  leaveok(stdscr, FALSE);
  curs_set(0);

  WbkgdSet(ctx, stdscr, COLOR_PAIR(CPAIR_WINDIR) | A_BOLD);

  ReCreateWindows(ctx);
  DEBUG_LOG("Init: ReCreateWindows done");

  /* Use the simpler constant value */
  if (baudrate() >= QUICK_BAUD_RATE)
    typeahead(-1);
  DEBUG_LOG("Init: typeahead done");

  ctx->ctx_file_window = ctx->ctx_small_file_window;

  DEBUG_LOG("Init: Calling ReadGroupEntries");
  if (ReadGroupEntries()) {
    UI_Notice(ctx, "ReadGroupEntries failed*ABORT");
    exit(1);
  }
  DEBUG_LOG("Init: ReadGroupEntries done");

  DEBUG_LOG("Init: Calling ReadPasswdEntries");
  if (ReadPasswdEntries()) {
    UI_Notice(ctx, "ReadPasswdEntries failed*ABORT");
    exit(1);
  }
  DEBUG_LOG("Init: ReadPasswdEntries done");

  if (configuration_file != NULL) {
    DEBUG_LOG("Init: Reading profile %s", configuration_file);
    ReadProfile(ctx, configuration_file);
  } else if ((home = getenv("HOME"))) {
    snprintf(buffer, sizeof(buffer), "%s%c%s", home, FILE_SEPARATOR_CHAR,
             PROFILE_FILENAME);
    DEBUG_LOG("Init: Reading profile %s", buffer);
    ReadProfile(ctx, buffer);
  }
  DEBUG_LOG("Init: ReadProfile done");

  if (history_file != NULL) {
    DEBUG_LOG("Init: Reading history %s", history_file);
    ReadHistory(ctx, history_file);
  } else if (home) {
    snprintf(buffer, sizeof(buffer), "%s%c%s", home, FILE_SEPARATOR_CHAR,
             HISTORY_FILENAME);
    DEBUG_LOG("Init: Reading history %s", buffer);
    ReadHistory(ctx, buffer);
  }
  DEBUG_LOG("Init: ReadHistory done");

  ReinitColorPairs(ctx);
  DEBUG_LOG("Init: ReinitColorPairs done");

  /* Initial Mode Setup for both panels */
  int initial_mode = strtol(GetProfileValue(ctx, "FILEMODE"), NULL, 0);
  SetPanelFileMode(ctx, ctx->left, initial_mode);
  SetPanelFileMode(ctx, ctx->right, initial_mode);
  DEBUG_LOG("Init: SetPanelFileMode done");

  SetKindOfSort(SORT_BY_NAME, &ctx->active->vol->vol_stats);
  DEBUG_LOG("Init: SetKindOfSort done");

  /* Use System Locale for number separator */
  struct lconv *lc = localeconv();
  if (lc && lc->thousands_sep && *lc->thousands_sep)
    ctx->number_seperator = *lc->thousands_sep;
  else
    ctx->number_seperator = ','; /* Fallback to English/Comma */
  DEBUG_LOG("Init: locale fallback done");

  ctx->bypass_small_window =
      (strtol(GetProfileValue(ctx, "SMALLWINDOWSKIP"), NULL, 0)) ? TRUE
                                                                  : FALSE;
  ctx->highlight_full_line =
      (strtol(GetProfileValue(ctx, "HIGHLIGHT_FULL_LINE"), NULL, 0)) ? TRUE
                                                                     : FALSE;
  ctx->hide_dot_files =
      (strtol(GetProfileValue(ctx, "HIDEDOTFILES"), NULL, 0)) ? TRUE : FALSE;
  ctx->animation_method = strtol(GetProfileValue(ctx, "ANIMATION"), NULL, 0);
  ctx->initial_directory = GetProfileValue(ctx, "INITIALDIR");

  ctx->refresh_mode = strtol(GetProfileValue(ctx, "AUTO_REFRESH"), NULL, 0);
  DEBUG_LOG("Init: Profile variables done");

  InitClock(ctx);
  DEBUG_LOG("Init: InitClock done");
  if (ctx->refresh_mode & REFRESH_WATCHER)
    Watcher_Init(ctx);
  DEBUG_LOG("Init: Watcher_Init done");

  DEBUG_LOG("EXIT Init");
  return (0);
}

static WINDOW *Subwin(WINDOW *orig, int nlines, int ncols, int begin_y,
                      int begin_x) {
  int x, y, h, w;
  WINDOW *win;

  if (nlines > LINES)
    nlines = LINES;
  if (ncols > COLS)
    ncols = COLS;

  h = MAXIMUM(nlines, 1);
  w = MAXIMUM(ncols, 1);
  x = MAXIMUM(begin_x, 0);
  y = MAXIMUM(begin_y, 0);

  if (x + w > COLS)
    x = COLS - w;
  if (y + h > LINES)
    y = LINES - h;

  win = subwin(orig, h, w, y, x);

  return (win);
}

static WINDOW *Newwin(int nlines, int ncols, int begin_y, int begin_x) {
  int x, y, h, w;
  WINDOW *win;

  if (nlines > LINES)
    nlines = LINES;
  if (ncols > COLS)
    ncols = COLS;

  h = MAXIMUM(nlines, 1);
  w = MAXIMUM(ncols, 1);
  x = MAXIMUM(begin_x, 0);
  y = MAXIMUM(begin_y, 0);

  if (x + w > COLS)
    x = COLS - w;
  if (y + h > LINES)
    y = LINES - h;

  win = newwin(h, w, y, x);
  if (win) {
    keypad(win, TRUE);
  }

  return (win);
}
