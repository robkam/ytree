/***************************************************************************
 *
 * Header-Datei fuer YTREE
 *
 ***************************************************************************/

/* Default (max) TreeDepth on startup */
#define DEFAULT_TREEDEPTH     "1"
#define DEFAULT_FILEMODE      "2"
#define DEFAULT_NOSMALLWINDOW "0"
#define DEFAULT_NUMBERSEP     "."
#define DEFAULT_INITIALDIR    NULL
#define DEFAULT_DIR1          NULL
#define DEFAULT_DIR2          NULL
#define DEFAULT_FILE1         NULL
#define DEFAULT_FILE2         NULL

#define DEFAULT_HEXEDITOFFSET "HEX"


/* UNIX-Kommandos */
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

#define DEFAULT_ZOOLIST    "zoo vm"
#define DEFAULT_ZOOEXPAND  "zoo xp"

#define DEFAULT_ZIPLIST    "zipinfo -l"
#define DEFAULT_ZIPEXPAND  "unzip -p"

#define DEFAULT_7zLIST     "7z l"
#define DEFAULT_7zEXPAND   "7z -so x"

#define DEFAULT_ISOLIST     "7z l"
#define DEFAULT_ISOEXPAND   "7z -so x"

#define DEFAULT_LHALIST    "lharc v"
#define DEFAULT_LHAEXPAND  "lharc p"

#define DEFAULT_ARCLIST    "arc v"
#define DEFAULT_ARCEXPAND  "arc p"

#define DEFAULT_RARLIST    "unrar l"
#define DEFAULT_RAREXPAND  "unrar p -c- -INUL"

#define DEFAULT_TAPEDEV    "/dev/rmt0"

#define DEFAULT_SEARCHCOMMAND	"grep  {}"
#define DEFAULT_LISTJUMPSEARCH	"0"


/* Attention! Use a TAR known to output to stdout! (GNU/BSD tar) */
#define DEFAULT_TARLIST    "tar tvf -"
#define DEFAULT_TAREXPAND  "tar xOPf -"
#define DEFAULT_RPMLIST    "rpm -q -l --dump -p"
#define DEFAULT_RPMEXPAND  "builtin"
#define DEFAULT_HEXDUMP    "hexdump" /* Defaulting to Linux/BSD standard */
#define DEFAULT_PAGER      "less"    /* Defaulting to modern standard */

/* Removed all specific OS #ifdef blocks for SGI, linux, __GNU__, __NeXT__, __OpenBSD__, __FreeBSD__, __NetBSD__, __DJGPP__ */