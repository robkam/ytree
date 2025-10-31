/***************************************************************************
 *
 * View-Hex-Kommando-Bearbeitung
 *
 ***************************************************************************/


#include "ytree.h"



static int ViewHexFile(char *file_path);
static int ViewHexArchiveFile(char *file_path);



int ViewHex(char *file_path)
{
  switch( mode )
  {
    case DISK_MODE :
    case USER_MODE :     return( ViewHexFile( file_path ) );
    case TAPE_MODE : 
    case RPM_FILE_MODE : 
    case TAR_FILE_MODE : 
    case ZOO_FILE_MODE :
    case ZIP_FILE_MODE :
    case SEVENZIP_FILE_MODE :
    case ISO_FILE_MODE :
    case LHA_FILE_MODE :
    case ARC_FILE_MODE : return( ViewHexArchiveFile( file_path ) );
    default:             beep(); return( -1 );
  }
}




static int ViewHexFile(char *file_path)
{
#if 0
  char *command_line;
  int  compress_method;
#endif

  int result = -1;

  if( access( file_path, R_OK ) )
  {
    (void) sprintf( message, 
		    "HexView not possible!*\"%s\"*%s", 
		    file_path, 
		    strerror(errno) 
		  );
    MESSAGE( message );
    ESCAPE;
  }

  InternalView(file_path);
  result = 0;

#if 0 

  if( ( command_line = (char *)malloc( COMMAND_LINE_LENGTH + 1 ) ) == NULL )
  {
    ERROR_MSG( "Malloc failed*ABORT" );
    exit( 1 );
  }


  compress_method = GetFileMethod( file_path );


  if( compress_method == FREEZE_COMPRESS )
  {
    (void) sprintf( command_line, 
		    "%s < '%s' %s | %s | %s", 
		    MELT, 
		    file_path, 
		    ERR_TO_STDOUT,
		    HEXDUMP, 
		    PAGER 
		  );
  }
  else if( compress_method == COMPRESS_COMPRESS )
  {
    (void) sprintf( command_line, 
		    "%s < '%s' %s | %s | %s", 
		    UNCOMPRESS, 
		    file_path, 
		    ERR_TO_STDOUT,
		    HEXDUMP, 
		    PAGER 
		  );
  }
  else if( compress_method == GZIP_COMPRESS )
  {
    (void) sprintf( command_line, 
		    "%s < '%s' %s | %s | %s", 
		    GNUUNZIP, 
		    file_path, 
		    ERR_TO_STDOUT,
		    HEXDUMP, 
		    PAGER 
		  );
  }
  else
  {
    (void) sprintf( command_line, 
		    "%s '%s' | %s", 
		    HEXDUMP, 
		    file_path, 
		    PAGER 
		  );
  }
    
  
  if((result = SilentSystemCall( command_line )))
  {
    (void) sprintf( message, "can't execute*%s", command_line );
    MESSAGE( message );
  }

  free( command_line );

#endif

FNC_XIT:

  return( result );
}



static int ViewHexArchiveFile(char *file_path)
{
  char *command_line;
  char *archive;
  char buffer[80];
  int result = -1;

  if( ( command_line = (char *)malloc( COMMAND_LINE_LENGTH + 1 ) ) == NULL )
  {
    ERROR_MSG( "Malloc failed*ABORT" );
    exit( 1 );
  }

  (void) sprintf( buffer, "| %s | %s", HEXDUMP, PAGER );

  archive = (mode == TAPE_MODE) ? statistic.tape_name : statistic.login_path;

  MakeExtractCommandLine( command_line, 
			  archive,
			  file_path, 
			  buffer
			);

  if((result = SilentSystemCall( command_line )))
  {
    (void) sprintf( message, "can't execute*%s", command_line );
    MESSAGE( message );
  }

  free( command_line );

  return( result );
}


