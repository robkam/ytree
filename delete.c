/***************************************************************************
 *
 * Loeschen von Dateien / Verzeichnissen
 *
 ***************************************************************************/


#include "ytree.h"


extern int unlink(const char *);
extern int rmdir(const char *);



int DeleteFile(FileEntry *fe_ptr)
{
  char     filepath[PATH_LENGTH+1];
  char	   buffer[PATH_LENGTH+1];
  int      result;
  int      term;

  result = -1;

  (void) GetFileNamePath( fe_ptr, filepath );

  if( !S_ISLNK( fe_ptr->stat_struct.st_mode ) )
  {
    if( access( filepath, W_OK ) )
    {
      if( access( filepath, F_OK ) )
      {
        /* Datei existiert nicht ==> fertig */

        goto UNLINK_DONE;
      }

      sprintf( buffer, "overriding mode %04o for \"%s\" (Y/N) ? ", 
               fe_ptr->stat_struct.st_mode & 0777, fe_ptr->name);

      term = InputChoise( buffer, "YN\033" );

      if( term != 'Y' )
      {
        (void) sprintf( message, 
		        "Can't delete file*\"%s\"*%s", 
		        filepath, 
		        strerror(errno) 
   		      );
        MESSAGE( message );
        ESCAPE;
      }
    }
  }

  if( unlink( filepath ) )
  {
    (void) sprintf( message, 
		    "Can't delete file*\"%s\"*%s", 
		    filepath, 
		    strerror(errno) 
		  );
    MESSAGE( message );
    ESCAPE;
  }

UNLINK_DONE:

  /* File austragen */
  /*----------------*/

  result = RemoveFile( fe_ptr );
  (void) GetAvailBytes( &statistic.disk_space );

FNC_XIT:
  
  return( result );
}





int RemoveFile(FileEntry *fe_ptr)
{
  DirEntry *de_ptr;
  LONGLONG file_size;

  de_ptr = fe_ptr->dir_entry;

  file_size = fe_ptr->stat_struct.st_size;

  de_ptr->total_bytes -= file_size;
  de_ptr->total_files--;
  statistic.disk_total_bytes -= file_size;
  statistic.disk_total_files--;
  if( fe_ptr->matching ) {
    de_ptr->matching_bytes -= file_size;
    de_ptr->matching_files--;
    statistic.disk_matching_bytes -= file_size;
    statistic.disk_matching_files--;
  }
  if( fe_ptr->tagged )
  {
    de_ptr->tagged_bytes -= file_size;
    de_ptr->tagged_files--;
    statistic.disk_tagged_bytes -= file_size;
    statistic.disk_tagged_files--;
  }

  /* File austragen */
  /*----------------*/

  if( fe_ptr->next ) fe_ptr->next->prev = fe_ptr->prev;
  if( fe_ptr->prev ) fe_ptr->prev->next = fe_ptr->next;
  else
    de_ptr->file = fe_ptr->next;

  free( (char *) fe_ptr );

  return( 0 );
}







