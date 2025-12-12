/***************************************************************************
 *
 * main.c
 * Hauptmodul
 *
 ***************************************************************************/


#include "ytree.h"
#include <signal.h>
#include <stdlib.h>


static char buffer[PATH_LENGTH+1];
static char path[PATH_LENGTH+1];

/* atexit handler for post-return crash diagnosis */
static void OnExit(void) {
  fprintf(stderr, "DEBUG: atexit handler reached\n");
  fflush(stderr);
}

static void EmergencyExit(int sig)
{
  /* Instrument: Log signal immediately */
  fprintf(stderr, "DEBUG: EmergencyExit caught signal %d\n", sig);
  fflush(stderr);

  endwin();

  /* Force terminal reset to prevent "Blue Screen" hang */
  fprintf(stderr, "DEBUG: EmergencyExit calling system(stty sane)\n");
  fflush(stderr);
  system("stty sane");

  fprintf(stderr, "\nINTERNAL ERROR: Signal %d caught. Terminal restored.\n", sig);
  exit(1);
}

int main(int argc, char **argv)
{
  char *p;
  int argi;
  char *hist;
  char *conf;
  int main_loop_exit_char; // Variable to store the return value of HandleDirWindow

  /* Register Signal Handlers */
  signal(SIGSEGV, EmergencyExit); /* Segfault */
  signal(SIGABRT, EmergencyExit); /* Abort */
  signal(SIGINT, EmergencyExit);  /* Ctrl-C safety */

  /* Register atexit handler for final exit diagnosis */
  atexit(OnExit);

  /* setlocale is now handled in Init */


  hist = NULL;
  conf = NULL;
  p = DEFAULT_TREE;
  for (argi = 1; argi < argc; argi++)
  {
    if (*(argv[argi]) != '-')
    {
      p = argv[argi];
      break;
    }
    switch(*(argv[argi]+1)) {
    case 'p':
    case 'P':
      if (*(argv[argi]+2) <= ' ')
        conf = argv[++argi];
      else
        conf = argv[argi]+2;
      break;
/*    case 'e':
    case 'E':
       Hex dump (builtin)
      if(argi == argc) {
        return(BuiltinHexDump(NULL));
      }

      if (*(argv[argi]+2) <= ' ')
        hex_file = argv[++argi];
      else
        hex_file = argv[argi]+2;
        return(BuiltinHexDump(hex_file));
      break;*/
    case 'h':
    case 'H':
      if (*(argv[argi]+2) <= ' ')
        hist = argv[++argi];
      else
        hist = argv[argi]+2;
      break;
    default:
      printf("Usage: %s [-p profile_file] [-h hist_file] [initial_dir]\n",
          argv[0]);
      exit(1);
    }
  }

  if (Init(conf, hist))
      exit(1);

  if( *p != FILE_SEPARATOR_CHAR )
  {
    /* rel. Pfad */
    /*-----------*/

    if( getcwd( buffer, sizeof( buffer ) - 2 ) == NULL )
    {
        endwin();
        fprintf(stderr, "Error: getcwd failed: %s\n", strerror(errno));
        exit(1);
    }
    (void) strcat( buffer, FILE_SEPARATOR_STRING );
    (void) strcat( buffer, p );
    p = buffer;
  }

  /* Normalize path */

  NormPath( p, path );

  statistic.login_path[0] = '\0';
  statistic.path[0] = '0';

  if( LoginDisk( path ) == -1 )
  {
    endwin();
#ifdef XCURSES
    XCursesExit();
#endif
    // Note: If LoginDisk fails, a volume might have been created and added to VolumeList
    // but not properly cleaned up. This is an early exit that bypasses Volume_FreeAll()
    // at the end of main. For this specific task, we are focusing on the normal exit path.
    exit( 1 );
  }

  // Main application loop. It continues until HandleDirWindow signals a quit.
  while( 1 )
  {
    main_loop_exit_char = HandleDirWindow(statistic.tree);
    if (main_loop_exit_char == 'q' || main_loop_exit_char == 'Q') {
        // User requested to quit. Break the loop to proceed with cleanup.
        break;
    }
    // If HandleDirWindow returns other characters (e.g., CR, ESC, -1 for resize),
    // the loop continues. 'l' or 'L' for LoginDisk also causes the loop to continue
    // after LoginDisk has updated the tree.
    // Note: Ctrl-Q ('Q' & 0x1F) is handled inside HandleDirWindow by calling QuitTo(),
    // which performs cleanup and exits directly.
  }

  /*
   * Explicit cleanup: The main loop has terminated (e.g., user pressed 'q').
   * Safe Shutdown Order:
   * 1. Suspend Clock (Stop signals)
   * 2. Restore terminal
   * 3. Free memory
   * This prevents "Blue Screen" freezes if memory cleanup crashes or signals fire during exit.
   */
  SuspendClock(); /* CRITICAL FIX: Stop SIGALRM before touching curses/memory */

  attrset(0);  /* Reset attributes */
  clear();     /* Clear internal buffer */
  refresh();   /* Push clear to screen */
  curs_set(1); /* Restore visible cursor */
  endwin();    /* Restore terminal settings */

  Volume_FreeAll(); /* Explicitly free memory */
  FreeDirEntryList(); /* Free the global dir_entry_list array */

  /* Final safety net for terminal state */
  fprintf(stderr, "DEBUG: main reached return 0\n");
  fflush(stderr);
  system("stty sane");
  fprintf(stderr, "DEBUG: stty sane finished\n");
  fflush(stderr);
  return 0;
}