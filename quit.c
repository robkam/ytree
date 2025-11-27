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

QuitTo:

- get parent's pid
- get owning uid's homedir
- compose shell stub name
- test:
  - exists?
  - regular file?
  - non-zero length?
- write dir_entry to it, thus replacing the starting-cwd line
- chain to Quit

if user elects not to quit, no harm done:

- rewrite that file to starting-cwd if it passes those checks again
  (avoiding race opening vuln)
- return to main program loop

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

#define MAXPATH	(PATH_LENGTH+1)

static int QuitFileCheck(char *fname);
static void PerformQuit(DirEntry *dir_entry);


/*
 * The core quit logic. It asks for user confirmation and then performs all
 * necessary exit procedures. If dir_entry is not NULL, it also handles the
 * "quit-to" functionality by writing the target directory to a file for
 * the calling shell script.
 */
static void PerformQuit(DirEntry *dir_entry)
{
    int term;
    char path[PATH_LENGTH + 1];
    char *p;

    if (strtol(CONFIRMQUIT, NULL, 0) != 0) {
        term = InputChoise("quit ytree (Y/N) ?", "YNQq\r\033");
    } else {
        term = 'Y';
    }


    if (term == 'Y' || term == 'Q' || term == 'q')
    {
        /* If dir_entry is provided, this is a "QuitTo" action. */
        if (dir_entry != NULL) {
            int parpid;
            FILE *qfile;
            char nbuf[MAXPATH], qfilename[MAXPATH];
            struct passwd *pwp;

            GetPath(dir_entry, nbuf);
            parpid = getppid();
            pwp = getpwuid(getuid());
            if (pwp != NULL && pwp->pw_dir != NULL) {
                sprintf(qfilename, "%s/.ytree-%d.chdir", pwp->pw_dir, parpid);

                if (QuitFileCheck(qfilename) == 0) {
                    qfile = fopen(qfilename, "w");
                    if (qfile != NULL) {
                        /* Add quotes to handle paths with spaces */
                        fprintf(qfile, "cd \"%s\"\n", nbuf);
                        fclose(qfile);
                    }
                }
            }
        }

        /* Common exit procedure for all quit types */
        if ((p = getenv("HOME"))) {
            sprintf(path, "%s%c%s", p, FILE_SEPARATOR_CHAR, HISTORY_FILENAME);
            SaveHistory(path);
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
 * Public interface for "QuitTo". Invokes the main quit logic
 * with the target directory.
 */
void QuitTo(DirEntry *dir_entry)
{
    PerformQuit(dir_entry);
}


/*
 * QuitFileCheck.
 * quick safety-check of the shell stub we want to write to.
 * if any of these tests fail, somebody's trying to use us for a
 * security breach.
 */
static int QuitFileCheck(char *fname)
{
    struct stat fstat;

    if (stat(fname, &fstat) != 0)
        return (1);
    if (!S_ISREG(fstat.st_mode))
        return (1);
    if (!fstat.st_size)
        return (2);
    if (fstat.st_uid != getuid())
        return (3);
    return (0);
}


/*
 * Public interface for a normal quit. Invokes the main quit logic
 * without a target directory.
 */
void Quit(void)
{
    PerformQuit(NULL);
}