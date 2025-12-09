/***************************************************************************
 *
 * main.c
 * Hauptmodul
 *
 ***************************************************************************/


#include "ytree.h"
#include <signal.h> /* Required for signal handling */
#include <stdlib.h> /* Required for exit() */


static char buffer[PATH_LENGTH+1];
static char path[PATH_LENGTH+1];

/*
 * EmergencyExit
 * Signal handler to ensure terminal is restored on unexpected program termination.
 * This prevents the terminal from being left in a "frozen" graphics mode.
 */
static void EmergencyExit(int sig)
{
  endwin(); /* Restore terminal to normal mode */
  fprintf(stderr, "\nINTERNAL ERROR: Signal %d caught. Terminal restored.\n", sig);
  exit(1); /* Exit with an error status */
}


int main(int argc, char **argv)
{
  char *p;
  int argi;
  char *hist;
  char *conf;
  int main_loop_exit_char; // Variable to store the return value of HandleDirWindow

  /* Register signal handlers for graceful exit on crashes or Ctrl-C.
   * This ensures endwin() is called to restore the terminal. */
  signal(SIGSEGV, EmergencyExit); /* Segmentation Fault */
  signal(SIGABRT, EmergencyExit); /* Abort signal */
  signal(SIGINT, EmergencyExit);  /* Interrupt (Ctrl-C) for safety */

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
   * All allocated volumes and associated memory are freed before program exit.
   */
  Volume_FreeAll(); /* Explicitly free memory */
  FreeDirEntryList(); /* Free the global dir_entry_list array */
  attrset(0);  /* Reset attributes */
  clear();     /* Clear internal buffer */
  refresh();   /* Push clear to screen */
  curs_set(1); /* Restore visible cursor */
  endwin();
  return 0;
}