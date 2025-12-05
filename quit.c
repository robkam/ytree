/***************************************************************************
 *
 * quit.c
 * Verlassen von ytree
 *
 ***************************************************************************/


#include "ytree.h"

/*
quit-to-directory (xtree: alt-Q)(ytree: shift-Q, ^Q)
this uses the Ztree method.
for this to work, a ytree is called via a function in ~/.bashrc
instead of directly. that function generates a pid-unique
file containing a "cd cwd" line before calling ytree, and
sources-in the pid-unique file afterward, thus jumping to the
directory specified.
if ytree rewrites that file to the quit-to dir, the eventual
command-prompt will be in that chosen dir.
thus, a Quit exit jumps to the dir it's already in. a QuitTo
exit jumps to the dir_entry-specified dir.

QuitTo (Ctrl-Q) implementation:

- Resolves the target directory path (either selected or current).
- Constructs a PID-unique filename in the user's home directory (e.g., ~/.ytree-PID.chdir).
- Opens this file for writing, overwriting any existing content.
- Writes the 'cd "target_path"' command to the file.
- Closes the file.
- Proceeds with the standard ytree exit procedure (via Quit()).
- No strict security checks (symlink/ownership/existence) are performed on the file,
  as the wrapper script is expected to manage its creation and permissions.
  Failure to open/write the file is logged as a warning but does not prevent ytree from exiting.

Quit (q/Q) implementation:

- Prompts for confirmation if configured.
- Proceeds with the standard ytree exit procedure.
- Does NOT write any 'cd' file.

a user can have a bunch of ytrees open, but they'll be run one-per-
shell (i.e. each xterm singletasking) unless the user's ASKING for
trouble, so a given shell-pid will be unique for the xterm that
owns it, even though they're all clustered in the user's homedir.

changes:
- ytree.h		add	prototype for QuitTo
- filewin.c		add call to QuitTo, for Q and ^Q
- dirwin.c		add call to QuitTo, for Q and ^Q (q still falls to main)

i've mapped this to ^Q for now on my home machine, but it's probably best to
use just SHIFT-Q for this, following the trend of mapping XtreeGold
Alt-commands to SHIFT-commands in ytree/xtc. that leaves ytree usable in
serial terminals with soft (XOFF/XON) line control protocol.

in .bashrc:
#
# "yt" is part of getting ytree to quit to the target directory rather
# than the starting one (Alt-Q rather than Q, in XTG terms).
# the pid-numbered chdir "bat" will get rewritten to the selected
# dir if ytree is Quit or ^Quit rather than quit, if ytree-1.75b
# (or later version, assuming Werner incorporates my changes) is used.
# This method is based on Kim Henkel's alt-Q batfile method for ZtreeWin.
#  --crb3 14jan00/12mar03
#
function yt
{
  echo cd $PWD >~/.ytree-$$.chdir
  /usr/bin/ytree $1 $2 $3 $4 $5
  source ~/.ytree-$$.chdir
  rm ~/.ytree-$$.chdir
}

*/

/*
 * The core quit logic. It asks for user confirmation and then performs all
 * necessary exit procedures. This function no longer handles "quit-to"
 * functionality; that is now handled by QuitTo() directly.
 */
static void PerformQuit(void)
{
    int term;
    char path_for_history[PATH_LENGTH + 1];
    char *p;

    if (strtol(CONFIRMQUIT, NULL, 0) != 0) {
        term = InputChoise("quit ytree (Y/N) ?", "YNQq\r\033");
    } else {
        term = 'Y';
    }

    if (term == 'Y' || term == 'Q' || term == 'q')
    {
        /* Common exit procedure for all quit types */
        if ((p = getenv("HOME"))) {
            sprintf(path_for_history, "%s%c%s", p, FILE_SEPARATOR_CHAR, HISTORY_FILENAME);
            SaveHistory(path_for_history);
        }
        endwin();
#ifdef XCURSES
        XCursesExit();
#endif
        /* Free all allocated volumes to prevent memory leaks on exit. */
        Volume_FreeAll();
        exit(0);
    }
    /* If user cancels, the function simply returns. */
}


/*
 * Public interface for "QuitTo". Implements the "quit-to-directory"
 * functionality by writing the target directory to a file for the
 * calling shell script, then performs a standard quit.
 */
void QuitTo(DirEntry *dir)
{
    char path_to_write[PATH_LENGTH + 1];
    char qfilename[PATH_LENGTH + 1];
    char *home_dir;
    FILE *fp;

    /* 2. Resolve Path: */
    if (dir != NULL) {
        GetPath(dir, path_to_write);
    } else {
        /* If dir is NULL, quit to current directory. */
        strcpy(path_to_write, statistic.path);
    }

    /* 3. Construct Filename: */
    home_dir = getenv("HOME");
    if (home_dir != NULL) {
        sprintf(qfilename, "%s/.ytree-%d.chdir", home_dir, (int)getppid());

        /* 4. Write (Simple): */
        fp = fopen(qfilename, "w");
        if (fp != NULL) {
            /* Add quotes to handle paths with spaces */
            fprintf(fp, "cd \"%s\"\n", path_to_write);
            fclose(fp);
        } else {
            /* Handle fopen error, but do not prevent quitting. */
            WARNING("Failed to open .ytree-PID.chdir file for writing.");
        }
    } else {
        /* Handle HOME not found, but do not prevent quitting. */
        WARNING("HOME environment variable not set. Cannot write .ytree-PID.chdir file.");
    }

    /* 5. Cleanup & Exit: */
    Quit();
}


/*
 * Public interface for a normal quit. Invokes the main quit logic
 * without writing a target directory file.
 */
void Quit(void)
{
    PerformQuit();
}