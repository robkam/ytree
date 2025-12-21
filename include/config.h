/***************************************************************************
 *
 * config.h
 * Header file for YTREE
 *
 ***************************************************************************/

/* Default (max) TreeDepth on startup */
#define DEFAULT_TREEDEPTH     "2"
#define DEFAULT_FILEMODE      "2"
#define DEFAULT_NOSMALLWINDOW "1"
#define DEFAULT_NUMBERSEP     "."
#define DEFAULT_INITIALDIR    NULL
#define DEFAULT_DIR1          NULL
#define DEFAULT_DIR2          NULL
#define DEFAULT_FILE1         NULL
#define DEFAULT_FILE2         NULL

#define DEFAULT_HEXEDITOFFSET "HEX"
#define DEFAULT_AUDIBLEERROR  "0"
#define DEFAULT_CONFIRMQUIT   "0"
#define DEFAULT_HIGHLIGHT_FULL_LINE "0"
#define DEFAULT_HIDEDOTFILES  "1"
#define DEFAULT_ANIMATION     "0"


/* UNIX Commands */
/*----------------*/

#define DEFAULT_CAT        "cat"
#define DEFAULT_EDITOR     "vi"
#define DEFAULT_MELT       "melt"
#define DEFAULT_UNCOMPRESS "gunzip" /* Changed from original 'uncompress' on Linux */
#define DEFAULT_GNUUNZIP   "gunzip -c"
#define DEFAULT_BUNZIP     "bunzip2"
#define DEFAULT_ZSTDCAT    "zstdcat"
#define DEFAULT_LUNZIP     "lzip -dc"
#define DEFAULT_MANROFF    "nroff -man"

#define DEFAULT_SEARCHCOMMAND	"grep  {}"
#define DEFAULT_LISTJUMPSEARCH	"0"

#define DEFAULT_HEXDUMP    "hexdump" /* Defaulting to Linux/BSD standard */
#define DEFAULT_PAGER      "less"    /* Defaulting to modern standard */

/* Removed all specific OS #ifdef blocks for SGI, linux, __GNU__, __NeXT__, __OpenBSD__, __FreeBSD__, __NetBSD__, __DJGPP__ */
/* Obsolete archive-specific commands have been removed as libarchive is now used. */