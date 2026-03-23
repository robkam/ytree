/***************************************************************************
 *
 * src/core/quit.c
 * Exiting ytree
 *
 ***************************************************************************/

#include "ytree.h"
#include <stdlib.h>

/*
 * The core quit logic. It asks for user confirmation and then performs all
 * necessary exit procedures. This function no longer handles "quit-to"
 * functionality; that is now handled by QuitTo() directly.
 */
static void PerformQuit(ViewContext *ctx) {
  int term;
  char path_for_history[PATH_LENGTH + 1];
  const char *p;

  if (ctx->confirm_quit && strtol(ctx->confirm_quit, NULL, 0) != 0) {
    term = InputChoice(ctx, "quit ytree (Y/N) ?", "YNQq\r\033");
  } else {
    term = 'Y';
  }

  if (term == 'Y' || term == 'Q' || term == 'q') {
    /* CRITICAL FIX: Stop the clock signal immediately to prevent race
     * conditions. If SIGALRM fires after endwin() but before exit(), it causes
     * a crash. */
    SuspendClock(ctx);
    /* Common exit procedure for all quit types */
    if ((p = getenv("HOME"))) {
      snprintf(path_for_history, sizeof(path_for_history), "%s%c%s", p,
               FILE_SEPARATOR_CHAR, HISTORY_FILENAME);
      SaveHistory(ctx, path_for_history);
    }

    /* Ncurses cleanup sequence: clear screen, restore cursor, and reset
     * terminal */
    /* Performed BEFORE freeing memory to ensure clean terminal state if crash
     * occurs */
    attrset(0);  /* Reset attributes */
    clear();     /* Clear internal buffer */
    refresh();   /* Push clear to screen */
    curs_set(1); /* Restore visible cursor */
    ShutdownCurses(ctx);

    /* Free all allocated volumes to prevent memory leaks on exit. */
    Volume_FreeAll(ctx);

    /* Free the global directory entry list */
    FreeDirEntryList(ctx);

#ifdef XCURSES
    XCursesExit();
#endif
    /* Final safety net for terminal state */
    if (system("stty sane")) {
      ;
    }
    exit(0);
  }
}

/*
 * Public interface for "QuitTo". Implements the "quit-to-directory"
 * functionality by writing the target directory to a file for the
 * calling shell script, then performs a standard quit.
 */
void QuitTo(ViewContext *ctx, DirEntry *dir_entry) {
  char path_to_write[PATH_LENGTH + 1];
  char qfilename[PATH_LENGTH + 1];
  const char *home_dir;

  /* 2. Resolve Path: */
  if (dir_entry != NULL) {
    GetPath(dir_entry, path_to_write);
  } else {
    /* If dir_entry is NULL, quit to current directory. */
    int copied_len = snprintf(path_to_write, sizeof(path_to_write), "%s",
                              ctx->active->vol->vol_stats.path);
    if (copied_len < 0) {
      path_to_write[0] = '\0';
    } else if ((size_t)copied_len >= sizeof(path_to_write)) {
      path_to_write[sizeof(path_to_write) - 1] = '\0';
    }
  }

  /* 3. Construct Filename: */
  home_dir = getenv("HOME");
  if (home_dir != NULL) {
    FILE *fp;
    snprintf(qfilename, sizeof(qfilename), "%s/.ytree-%d.chdir", home_dir,
             (int)getppid());

    /* 4. Write (Simple): */
    fp = fopen(qfilename, "w");
    if (fp != NULL) {
      /* Add quotes to handle paths with spaces */
      fprintf(fp, "cd \"%s\"\n", path_to_write);
      fclose(fp);
    } else {
      /* Handle fopen error, but do not prevent quitting. */
      WARNING(ctx, "Failed to open .ytree-PID.chdir file for writing.");
    }
  } else {
    /* Handle HOME not found, but do not prevent quitting. */
    WARNING(ctx, "HOME environment variable not set. Cannot write "
                 ".ytree-PID.chdir file.");
  }

  /* 5. Cleanup & Exit: */
  Quit(ctx);
}

/*
 * Public interface for a normal quit. Invokes the main quit logic
 * without writing a target directory file.
 */
void Quit(ViewContext *ctx) { PerformQuit(ctx); }
