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

FNC_XIT:

  return( result );
}



static int ViewHexArchiveFile(char *file_path)
{
    char *archive;
    char temp_filename[] = "/tmp/ytree_hex_XXXXXX";
    int fd;
    int result = -1;

    /* Create a temporary file to hold the extracted content */
    fd = mkstemp(temp_filename);
    if (fd == -1) {
        ERROR_MSG("Could not create temporary file for hex view");
        return -1;
    }

    archive = (mode == TAPE_MODE) ? statistic.tape_name : statistic.login_path;

#ifdef HAVE_LIBARCHIVE
    if (ExtractArchiveEntry(archive, file_path, fd) != 0) {
        (void)sprintf(message, "Could not extract entry*'%s'*from archive", file_path);
        MESSAGE(message);
        close(fd);
        unlink(temp_filename);
        return -1;
    }
#endif
    close(fd);

    /* Now call InternalView on the temporary file */
    InternalView(temp_filename);
    
    result = 0;

    /* Clean up the temporary file */
    unlink(temp_filename);

    return result;
}