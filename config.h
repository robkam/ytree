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
#define DEFAULT_UNCOMPRESS "uncompress"
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

#define DEFAULT_LHALIST    "xlharc v"
#define DEFAULT_LHAEXPAND  "xlharc p"

#define DEFAULT_ARCLIST    "arc v"
#define DEFAULT_ARCEXPAND  "arc p"

#define DEFAULT_RARLIST    "unrar l"
#define DEFAULT_RAREXPAND  "unrar p -c- -INUL"

#define DEFAULT_TAPEDEV    "/dev/rmt0"

#define DEFAULT_SEARCHCOMMAND	"grep  {}"
#define DEFAULT_LISTJUMPSEARCH	"0"


/* Attention! You must use GNU TAR, because only GNU TAR is able to
 * extract to stdout!
 */

#define DEFAULT_TARLIST    "gtar tvf -"
#define DEFAULT_TAREXPAND  "gtar xOPf -"
#define DEFAULT_RPMLIST    "rpm -q -l --dump -p"
#define DEFAULT_RPMEXPAND  "builtin"
#define DEFAULT_HEXDUMP    "hd"
#define DEFAULT_PAGER      "more"

#ifdef __DJGPP__
#undef DEFAULT_PAGER
#define DEFAULT_PAGER      "view"
#endif


#ifdef sgi
#undef  DEFAULT_HEXDUMP
#undef  DEFAULT_PAGER
#define DEFAULT_HEXDUMP     "od -h"
#define DEFAULT_PAGER       "pg"
#endif /* sgi */

#ifdef linux
#undef  DEFAULT_TARLIST
#undef  DEFAULT_TAREXPAND
#undef  DEFAULT_HEXDUMP
#undef  DEFAULT_PAGER
#undef  DEFAULT_LHALIST
#undef  DEFAULT_LHAEXPAND
#undef  DEFAULT_TAPEDEV
#undef  DEFAULT_UNCOMPRESS
#define DEFAULT_TARLIST    "tar tvf -"
#define DEFAULT_TAREXPAND  "tar xOPf -"
#define DEFAULT_HEXDUMP    "hexdump"
#define DEFAULT_PAGER      "less"
#define DEFAULT_LHALIST    "lharc v"
#define DEFAULT_LHAEXPAND  "lharc p"
#define DEFAULT_TAPEDEV    "/dev/rmt0"
#define DEFAULT_UNCOMPRESS "gunzip"
#endif /* linux */

#ifdef __GNU__
#undef  DEFAULT_TARLIST
#undef  DEFAULT_TAREXPAND
#undef  DEFAULT_HEXDUMP
#undef  DEFAULT_PAGER
#undef  DEFAULT_LHALIST
#undef  DEFAULT_LHAEXPAND
#undef  DEFAULT_TAPEDEV
#undef  DEFAULT_UNCOMPRESS
#define DEFAULT_TARLIST    "tar tvf -"
#define DEFAULT_TAREXPAND  "tar xOPf -"
#define DEFAULT_HEXDUMP    "hexdump"
#define DEFAULT_PAGER      "less"
#define DEFAULT_LHALIST    "lharc v"
#define DEFAULT_LHAEXPAND  "lharc p"
#define DEFAULT_TAPEDEV    "/dev/rmt0"
#define DEFAULT_UNCOMPRESS "gunzip"
#endif /* __GNU__ */


#ifdef __NeXT__
/* fifi's NeXT! */
# undef DEFAULT_TARLIST
# undef DEFAULT_TAREXPAND
# undef DEFAULT_HEXDUMP
# undef DEFAULT_PAGER
# define DEFAULT_TARLIST    "gtar tvf -"
# define DEFAULT_TAREXPAND  "gtar xOPf -"
# define DEFAULT_HEXDUMP    "hd"
# define DEFAULT_PAGER      "less"
#endif /* __NeXT__ */

#if defined(__OpenBSD__) || defined(__NetBSD__) ||  defined( __FreeBSD__) || defined(__NetBSD__)
#undef DEFAULT_TARLIST
#undef DEFAULT_TAREXPAND
#undef DEFAULT_TAPEDEV
#define DEFAULT_TARLIST    "tar tvf -"
#define DEFAULT_TAREXPAND  "tar xOPf -"
#define DEFAULT_TAPEDEV    "/dev/rst0"
#endif /* __FreeBSD__ */

