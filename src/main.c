/***************************************************************************
 *
 * main.c
 * Main module
 *
 ***************************************************************************/


#include "ytree.h"
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


static char buffer[PATH_LENGTH+1];

static void EmergencyExit(int sig)
{
  endwin();
  fprintf(stderr, "\nINTERNAL ERROR: Signal %d caught. Terminal restored.\n", sig);
  exit(1);
}

int main(int argc, char **argv)
{
  int argi, i;
  char *hist;
  char *conf;
  int main_loop_exit_char;
  int *path_indexes;
  int path_count = 0;

  /* Register Signal Handlers */
  signal(SIGSEGV, EmergencyExit); /* Segfault */
  signal(SIGABRT, EmergencyExit); /* Abort */
  signal(SIGINT, EmergencyExit);  /* Ctrl-C safety */

  /* setlocale is now handled in Init */

  hist = NULL;
  conf = NULL;

  /* Pass 1: Pre-scan Loop - Parse Options (-p, -h) */
  /* Note: -d is validated here to prevent usage error, but processed after Init */
  for (argi = 1; argi < argc; argi++)
  {
    if (argv[argi][0] == '-')
    {
      switch(argv[argi][1]) {
        case 'p':
        case 'P':
          if (argv[argi][2] <= ' ') {
             if (argi + 1 < argc) conf = argv[++argi];
             else {
                 fprintf(stderr, "Option -p requires an argument\n");
                 exit(1);
             }
          } else {
             conf = argv[argi]+2;
          }
          break;
        case 'h':
        case 'H':
          if (argv[argi][2] <= ' ') {
             if (argi + 1 < argc) hist = argv[++argi];
             else {
                 fprintf(stderr, "Option -h requires an argument\n");
                 exit(1);
             }
          } else {
             hist = argv[argi]+2;
          }
          break;
        case 'd':
        case 'D':
          /* Skip -d here, processed after Init */
          if (argv[argi][2] <= ' ') {
             if (argi + 1 < argc) argi++;
             else {
                 fprintf(stderr, "Option -d requires an argument\n");
                 exit(1);
             }
          }
          break;
        default:
          fprintf(stderr, "Usage: %s [-p profile_file] [-h hist_file] [-d depth] [directory ...]\n", argv[0]);
          exit(1);
      }
    }
  }

  if (Init(conf, hist))
      exit(1);

  /* Pass 1.5: Post-Init Option Parsing (-d) */
  /* Process overrides that must happen after Init */
  for (argi = 1; argi < argc; argi++)
  {
    if (argv[argi][0] == '-')
    {
      switch(argv[argi][1]) {
        case 'p':
        case 'P':
        case 'h':
        case 'H':
          /* Skip already processed options */
          if (argv[argi][2] <= ' ') argi++;
          break;
        case 'd':
        case 'D':
          {
             char *d_arg = NULL;
             if (argv[argi][2] <= ' ') {
                 if (argi + 1 < argc) {
                     d_arg = argv[++argi];
                 }
             } else {
                 d_arg = argv[argi]+2;
             }

             if (d_arg) {
                 if (strcasecmp(d_arg, "all") == 0 || strcasecmp(d_arg, "max") == 0) {
                     SetProfileValue("TREEDEPTH", "100");
                 } else if (strcasecmp(d_arg, "min") == 0 || strcasecmp(d_arg, "root") == 0) {
                     SetProfileValue("TREEDEPTH", "0");
                 } else {
                     SetProfileValue("TREEDEPTH", d_arg);
                 }
             }
          }
          break;
      }
    }
  }

  /* Allocate memory for path indexes to support multiple volumes */
  path_indexes = (int *)malloc(sizeof(int) * argc);
  if (!path_indexes) {
      endwin();
      fprintf(stderr, "Memory allocation failed\n");
      exit(1);
  }

  /* Pass 2: Path Collection Loop */
  for (argi = 1; argi < argc; argi++)
  {
    if (argv[argi][0] == '-')
    {
       /* Skip flags and their values to ensure we only collect positional args */
       char c = argv[argi][1];
       if ((c == 'p' || c == 'P' || c == 'h' || c == 'H' || c == 'd' || c == 'D') && argv[argi][2] <= ' ') {
          argi++;
       }
       continue;
    }
    path_indexes[path_count++] = argi;
  }

  /* Processing Paths or Default */
  if (path_count == 0)
  {
      /* Case 0: No paths provided, default to current working directory */
      if (getcwd(buffer, sizeof(buffer)) == NULL)
      {
          endwin();
          fprintf(stderr, "Error: getcwd failed: %s\n", strerror(errno));
          free(path_indexes);
          exit(1);
      }

      /* Reset stats logic is handled by LoginDisk/Init */
      if (LoginDisk(buffer) == -1) {
          endwin();
          /* If defaulting to CWD fails, it's a fatal error */
          fprintf(stderr, "Error: Failed to login to current directory\n");
          free(path_indexes);
          exit(1);
      }
  }
  else
  {
      /* Case > 0: Load paths in REVERSE order.
       * LoginDisk pushes volumes onto a stack (LIFO).
       * By loading the last command-line arg first, the first command-line arg
       * ends up at the top of the stack and becomes the active volume.
       */
      for (i = path_count - 1; i >= 0; i--)
      {
          /* LoginDisk returns -1 on failure but handles its own error messaging.
           * We proceed to try loading the other requested volumes. */
          LoginDisk(argv[path_indexes[i]]);
      }
  }

  free(path_indexes);

  /* Ensure we have at least one active volume before entering main loop */
  if (CurrentVolume == NULL || CurrentVolume->vol_stats.tree == NULL) {
      endwin();
      fprintf(stderr, "Error: No valid volumes could be loaded.\n");
      exit(1);
  }

  /* Main application loop */
  while( 1 )
  {
    main_loop_exit_char = HandleDirWindow(CurrentVolume->vol_stats.tree);
    if (main_loop_exit_char == 'q' || main_loop_exit_char == 'Q') {
        /* User requested to quit. Break the loop to proceed with cleanup. */
        break;
    }
  }

  /* Explicit cleanup */
  SuspendClock(); /* Stop SIGALRM before touching curses/memory */

  attrset(0);  /* Reset attributes */
  clear();     /* Clear internal buffer */
  refresh();   /* Push clear to screen */
  curs_set(1); /* Restore visible cursor */
  endwin();    /* Restore terminal settings */

  Volume_FreeAll(); /* Explicitly free memory */

  return 0;
}