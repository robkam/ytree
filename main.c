/***************************************************************************
 *
 * Hauptmodul
 *
 ***************************************************************************/


#include "ytree.h"


static char buffer[PATH_LENGTH+1];
static char path[PATH_LENGTH+1];


int main(int argc, char **argv)
{
  char *p;
  int argi;
  char *hist;
  char *conf;

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

    (void) Getcwd( buffer, sizeof( buffer ) - 2 );
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
    exit( 1 );
  }

  while( 1 )
  {
    if( HandleDirWindow(statistic.tree) == 'q' ) Quit();
  }
}