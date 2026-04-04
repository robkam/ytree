/***************************************************************************
 *
 * src/core/main.c
 * Main module
 *
 ***************************************************************************/

#include "ytree_defs.h"
#include "patchlev.h"
#include "default_profile_template.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

volatile sig_atomic_t ytree_shutdown_flag = 0;

static int CoreMainOpsReady(const CoreMainOps *ops) {
  return ops != NULL && ops->init != NULL && ops->set_profile_value != NULL &&
         ops->log_disk != NULL && ops->set_filter != NULL &&
         ops->recalculate_sys_stats != NULL && ops->handle_dir_window != NULL &&
         ops->suspend_clock != NULL && ops->shutdown_curses != NULL &&
         ops->volume_free_all != NULL;
}

static void SigIntHandler(int sig) {
  (void)sig;
  ytree_shutdown_flag = 1;
}

static int GetDefaultProfilePath(char *path, size_t path_size) {
  const char *home = getenv("HOME");
  int written;

  if (!path || path_size == 0 || !home || !*home)
    return -1;

  written = snprintf(path, path_size, "%s%c%s", home, FILE_SEPARATOR_CHAR,
                     PROFILE_FILENAME);
  if (written < 0 || (size_t)written >= path_size) {
    return -1;
  }
  return 0;
}

/*
 * Return values:
 *   0 = profile created
 *   1 = profile already exists (left untouched)
 *  -1 = hard error
 */
static int InitProfileFile(const char *path) {
  int fd;
  FILE *fp;
  size_t len;
  size_t written;

  if (!path || !*path)
    return -1;

  fd = open(path, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    if (errno == EEXIST)
      return 1;
    return -1;
  }

  fp = fdopen(fd, "w");
  if (!fp) {
    close(fd);
    unlink(path);
    return -1;
  }

  len = strlen(default_profile_template);
  written = fwrite(default_profile_template, 1, len, fp);
  if (written != len || fclose(fp) != 0) {
    unlink(path);
    return -1;
  }

  return 0;
}

int main(int argc, char **argv) {
  int argi;
  const char *hist;
  const char *conf;
  BOOL init_requested = FALSE;
  const char *filter_arg = NULL; /* Added for -f option */
  int *path_indexes;
  int path_count = 0;
  ViewContext ctx;

  memset(&ctx, 0, sizeof(ViewContext));
  CoreMainOps_Register(&ctx);
  if (!CoreMainOpsReady(&ctx.core_main_ops)) {
    fprintf(stderr, "EXIT: CoreMainOps not configured\n");
    exit(1);
  }

  /* Register Signal Handlers */
  /* signal(SIGSEGV, EmergencyExit); */ /* Segfault */
  /* signal(SIGABRT, EmergencyExit); */ /* Abort */
  signal(SIGINT, SigIntHandler);        /* Ctrl-C safety */

  /* setlocale is now handled in Init */

  hist = NULL;
  conf = NULL;

  /* Pass 1: Pre-scan Loop - Parse Options (-p, -h) */
  /* Note: -d and -f are validated here to prevent usage error, but processed
   * after Init */
  for (argi = 1; argi < argc; argi++) {
    if (!strcmp(argv[argi], "-v") || !strcmp(argv[argi], "-V") ||
        !strcmp(argv[argi], "--version")) {
      fprintf(stdout, "ytree %s (%s)\n", VERSION, VERSIONDATE);
      return 0;
    }
    if (!strcmp(argv[argi], "--init")) {
      init_requested = TRUE;
      continue;
    }

    if (argv[argi][0] == '-') {
      switch (argv[argi][1]) {
      case 'p':
      case 'P':
        if (argv[argi][2] <= ' ') {
          if (argi + 1 < argc)
            conf = argv[++argi];
          else {
            fprintf(stderr, "Option -p requires an argument\n");
            exit(1);
          }
        } else {
          conf = argv[argi] + 2;
        }
        break;
      case 'h':
      case 'H':
        if (argv[argi][2] <= ' ') {
          if (argi + 1 < argc)
            hist = argv[++argi];
          else {
            fprintf(stderr, "Option -h requires an argument\n");
            exit(1);
          }
        } else {
          hist = argv[argi] + 2;
        }
        break;
      case 'd':
      case 'D':
        /* Skip -d here, processed after Init */
        if (argv[argi][2] <= ' ') {
          if (argi + 1 < argc)
            argi++;
          else {
            fprintf(stderr, "Option -d requires an argument\n");
            exit(1);
          }
        }
        break;
      case 'f':
      case 'F':
        /* Skip -f here, processed after Init */
        if (argv[argi][2] <= ' ') {
          if (argi + 1 < argc)
            argi++;
          else {
            fprintf(stderr, "Option -f requires an argument\n");
            exit(1);
          }
        }
        break;
      default:
        fprintf(stderr,
                "Usage: %s [--init] [-v|-V|--version] [-p profile_file] "
                "[-h hist_file] [-d depth] [-f filter] [directory ...]\n",
                argv[0]);
        exit(1);
      }
    }
  }

  if (init_requested) {
    char init_path_buffer[PATH_LENGTH + 1];
    const char *init_path = conf;
    int init_status;

    if (!init_path) {
      if (GetDefaultProfilePath(init_path_buffer, sizeof(init_path_buffer)) !=
          0) {
        fprintf(
            stderr,
            "Cannot resolve target profile path. Set HOME or pass -p <file>.\n");
        exit(1);
      }
      init_path = init_path_buffer;
    }
    if (!init_path) {
      fprintf(stderr,
              "Cannot resolve target profile path. Set HOME or pass -p <file>.\n");
      exit(1);
    }

    init_status = InitProfileFile(init_path);
    if (init_status == 0) {
      fprintf(stdout, "Created profile: %s\n", init_path);
      return 0;
    }
    if (init_status == 1) {
      fprintf(stdout, "%s already exists; not overwritten\n", init_path);
      return 0;
    }

    fprintf(stderr, "Failed to initialize profile %s: %s\n", init_path,
            strerror(errno));
    exit(1);
  }

  if (ctx.core_main_ops.init(&ctx, conf, hist)) {
    fprintf(stderr, "EXIT: Init failed\n");
    exit(1);
  }
  if (!CoreMainOpsReady(&ctx.core_main_ops)) {
    fprintf(stderr, "EXIT: CoreMainOps registration lost\n");
    exit(1);
  }

  /* Pass 1.5: Post-Init Option Parsing (-d, -f) */
  /* Process overrides that must happen after Init */
  for (argi = 1; argi < argc; argi++) {
    if (argv[argi][0] == '-') {
      switch (argv[argi][1]) {
      case 'p':
      case 'P':
      case 'h':
      case 'H':
        /* Skip already processed options */
        if (argv[argi][2] <= ' ')
          argi++;
        break;
      case 'd':
      case 'D': {
        char *d_arg = NULL;
        if (argv[argi][2] <= ' ') {
          if (argi + 1 < argc) {
            d_arg = argv[++argi];
          }
        } else {
          d_arg = argv[argi] + 2;
        }

        if (d_arg) {
          if (strcasecmp(d_arg, "all") == 0 || strcasecmp(d_arg, "max") == 0) {
            ctx.core_main_ops.set_profile_value(&ctx, "TREEDEPTH", "100");
          } else if (strcasecmp(d_arg, "min") == 0 ||
                     strcasecmp(d_arg, "root") == 0) {
            ctx.core_main_ops.set_profile_value(&ctx, "TREEDEPTH", "0");
          } else {
            ctx.core_main_ops.set_profile_value(&ctx, "TREEDEPTH", d_arg);
          }
        }
      } break;
      case 'f':
      case 'F': {
        if (argv[argi][2] <= ' ') {
          if (argi + 1 < argc) {
            filter_arg = argv[++argi];
          }
        } else {
          filter_arg = argv[argi] + 2;
        }
      } break;
      }
    }
  }

  /* Allocate memory for path indexes to support multiple volumes */
  path_indexes = (int *)malloc(sizeof(int) * argc);
  if (!path_indexes) {
    ctx.core_main_ops.shutdown_curses(&ctx);
    fprintf(stderr, "Memory allocation failed\n");
    exit(1);
  }

  /* Pass 2: Path Collection Loop */
  for (argi = 1; argi < argc; argi++) {
    if (argv[argi][0] == '-') {
      /* Skip flags and their values to ensure we only collect positional args
       */
      char c = argv[argi][1];
      if ((c == 'p' || c == 'P' || c == 'h' || c == 'H' || c == 'd' ||
           c == 'D' || c == 'f' || c == 'F') &&
          argv[argi][2] <= ' ') {
        argi++;
      }
      continue;
    }
    path_indexes[path_count++] = argi;
  }

  /* Processing Paths or Default */
  if (path_count == 0) {
    char cwd_path[PATH_LENGTH + 1];

    /* Case 0: No paths provided, default to current working directory */
    if (getcwd(cwd_path, sizeof(cwd_path)) == NULL) {
      ctx.core_main_ops.shutdown_curses(&ctx);
      fprintf(stderr, "Error: getcwd failed: %s\n", strerror(errno));
      free(path_indexes);
      exit(1);
    }

    /* Use LogDisk (wrapper around Volume_Load) to load the initial path */
    if (ctx.core_main_ops.log_disk(&ctx, ctx.left, cwd_path) == -1) {
      ctx.core_main_ops.shutdown_curses(&ctx);
      /* If defaulting to CWD fails, it's a fatal error */
      fprintf(stderr, "EXIT: LogDisk failed for CWD\n");
      free(path_indexes);
      exit(1);
    }
  } else {
    for (int i = path_count - 1; i >= 0; i--) {
      /* LogDisk returns -1 on failure but handles its own error messaging via
       * UI. We proceed to try loading the other requested volumes. */
      ctx.core_main_ops.log_disk(&ctx, ctx.left, argv[path_indexes[i]]);
    }
  }

  free(path_indexes);

  /* Ensure we have at least one active volume before entering main loop */
  if (ctx.active->vol == NULL || ctx.active->vol->vol_stats.tree == NULL) {
    ctx.core_main_ops.shutdown_curses(&ctx);
    fprintf(stderr, "EXIT: No active volume\n");
    exit(1);
  }

  /* Apply command line filter if provided */
  if (filter_arg) {
    /* Safe copy with truncation */
    strncpy(ctx.active->vol->vol_stats.file_spec, filter_arg, FILE_SPEC_LENGTH);
    ctx.active->vol->vol_stats.file_spec[FILE_SPEC_LENGTH] = '\0';

    ctx.core_main_ops.set_filter(ctx.active->vol->vol_stats.file_spec,
                                 &ctx.active->vol->vol_stats);
    ctx.core_main_ops.recalculate_sys_stats(&ctx, &ctx.active->vol->vol_stats);
  }

  /* Main application loop */
  DEBUG_LOG("STARTING MAIN LOOP: ctx.active->vol=%p", (void *)ctx.active->vol);
  if (ctx.active->vol) {
    DEBUG_LOG("STARTING MAIN LOOP: tree=%p",
              (void *)ctx.active->vol->vol_stats.tree);
  }

  /* Main application loop */

  while (1) {
    if (ctx.active == NULL || ctx.active->vol == NULL ||
        ctx.active->vol->vol_stats.tree == NULL) {
      break;
    }
    DEBUG_LOG("Calling HandleDirWindow...");
    int main_loop_exit_char = ctx.core_main_ops.handle_dir_window(
        &ctx, ctx.active->vol->vol_stats.tree);
    DEBUG_LOG("HandleDirWindow returned %d", main_loop_exit_char);
    if (main_loop_exit_char == 'q' || main_loop_exit_char == 'Q') {
      /* User requested to quit. Break the loop to proceed with cleanup. */
      break;
    }
    /* Also break if shutdown flag was set by SIGINT handler but not caught
     * inside HandleDirWindow yet */
    if (ytree_shutdown_flag) {
      break;
    }
  }

  /* Explicit cleanup */
  ctx.core_main_ops.suspend_clock(
      &ctx); /* Stop SIGALRM (now no-op but kept for API consistency) before
                touching curses/memory */

  attrset(0);  /* Reset attributes */
  clear();     /* Clear internal buffer */
  refresh();   /* Push clear to screen */
  curs_set(1); /* Restore visible cursor */
  ctx.core_main_ops.shutdown_curses(&ctx);

  ctx.core_main_ops.volume_free_all(&ctx); /* Explicitly free memory */

  return 0;
}
