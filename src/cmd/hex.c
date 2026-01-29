/***************************************************************************
 *
 * src/cmd/hex.c
 * View hex command processing
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
    case ARCHIVE_MODE :  return( ViewHexArchiveFile( file_path ) );
    default:             beep(); return( -1 );
  }
}




static int ViewHexFile(char *file_path)
{
  int result = -1;

  if( access( file_path, R_OK ) )
  {
    MESSAGE( "HexView not possible!*\"%s\"*%s",
		    file_path,
		    strerror(errno)
		  );
    ESCAPE;
  }

  InternalView(file_path);
  result = 0;

FNC_XIT:

  return( result );
}


static int HexProgressCallback(int status, const char *msg, void *user_data)
{
    (void)msg;
    (void)user_data;

    if (status == ARCHIVE_STATUS_PROGRESS) {
        DrawSpinner();
        if (EscapeKeyPressed()) {
            return ARCHIVE_CB_ABORT;
        }
    }
    return ARCHIVE_CB_CONTINUE;
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

    archive = CurrentVolume->vol_stats.login_path;

#ifdef HAVE_LIBARCHIVE
    if (ExtractArchiveEntry(archive, file_path, fd, HexProgressCallback, NULL) != 0) {
        MESSAGE("Could not extract entry*'%s'*from archive", file_path);
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