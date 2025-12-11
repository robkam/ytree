/***************************************************************************
 *
 * rmdir.c
 * Loeschen von Verzeichnissen
 *
 ***************************************************************************/


#include "ytree.h"



static int DeleteSubTree(DirEntry *dir_entry);
static int DeleteSingleDirectory(DirEntry *dir_entry);


int DeleteDirectory(DirEntry *dir_entry)
{
  char buffer[PATH_LENGTH+1];
  int result = -1;

  if( mode != DISK_MODE && mode != USER_MODE )
  {
    return( result );
  }

  ClearHelp();

  if( dir_entry == statistic.tree )
  {
    MESSAGE( "Can't delete ROOT" );
  }
  else if( dir_entry->file || dir_entry->sub_tree )
  {
    if( InputChoise( "Directory not empty, PRUNE ? (Y/N) ? ", "YN\033" ) == 'Y' ) {
      if( dir_entry->sub_tree ) {
        if( ScanSubTree( dir_entry ) ) {
	  ESCAPE;
	}
        if( DeleteSubTree( dir_entry->sub_tree ) ) {
          ESCAPE;
        }
      }
      if( DeleteSingleDirectory( dir_entry ) ) {
	ESCAPE;
      }
      result = 0;
      ESCAPE;
    }
  }
  else if( InputChoise( "Delete this directory (Y/N) ? ", "YN\033" ) == 'Y' )
  {
    (void) GetPath( dir_entry, buffer );

    if( access( buffer, W_OK ) )
    {
      (void) sprintf( message, "Can't delete directory*\"%s\"*%s",
		      buffer, strerror(errno)
		    );
      MESSAGE( message );
    }
    else if( rmdir( buffer ) )
    {
      (void) sprintf( message, "Can't delete directory*\"%s\"*%s",
		      buffer, strerror(errno)
		    );
      MESSAGE( message );
    }
    else
    {
      /* Directory geloescht
       * ==> aus Baum loeschen
       */

      statistic.disk_total_directories--;

      if( dir_entry->prev ) dir_entry->prev->next = dir_entry->next;
      else dir_entry->up_tree->sub_tree = dir_entry->next;

      if( dir_entry->next ) dir_entry->next->prev = dir_entry->prev;

      free( dir_entry );

      (void) GetAvailBytes( &statistic.disk_space );

      result = 0;
    }
  }

FNC_XIT:

  return( result );
}





static int DeleteSubTree( DirEntry *dir_entry )
{
  int result = -1;
  DirEntry *de_ptr, *next_de_ptr;


  for( de_ptr = dir_entry; de_ptr; de_ptr = next_de_ptr ) {
    next_de_ptr = de_ptr->next;

    if( de_ptr->sub_tree ) {
      if( DeleteSubTree( de_ptr->sub_tree ) ) {
        ESCAPE;
      }
    }
    if( DeleteSingleDirectory( de_ptr ) ) {
      ESCAPE;
    }
  }

  result = 0;

FNC_XIT:

    return( result );
}



static int DeleteSingleDirectory( DirEntry *dir_entry )
{
  int result = -1;
  char buffer[PATH_LENGTH+1];
  FileEntry *fe_ptr, *next_fe_ptr;


  (void) GetPath( dir_entry, buffer );


  if( access( buffer, W_OK ) ) {
    (void) sprintf( message, "Can't delete directory*\"%s\"*%s",
		    buffer, strerror(errno)
		    );
    MESSAGE( message );
    ESCAPE;
  }

  for( fe_ptr = dir_entry->file; fe_ptr; fe_ptr=next_fe_ptr ) {
    next_fe_ptr = fe_ptr->next;
    /* Spinner to indicate progress during bulk deletion */
    DrawSpinner();
    doupdate();
    if( DeleteFile( fe_ptr ) ) {
      ESCAPE;
    }
  }

  if( rmdir( buffer ) ) {
    (void) sprintf( message, "Can't delete directory*\"%s\"*%s",
		    buffer, strerror(errno)
		  );
    MESSAGE( message );
    ESCAPE;
  }

  if( !dir_entry->up_tree->not_scanned )
    statistic.disk_total_directories--;

  if( dir_entry->prev ) dir_entry->prev->next = dir_entry->next;
  else dir_entry->up_tree->sub_tree = dir_entry->next;
  if( dir_entry->next ) dir_entry->next->prev = dir_entry->prev;

  free( dir_entry );

  result = 0;

FNC_XIT:

  return( result );
}