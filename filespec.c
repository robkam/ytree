/***************************************************************************
 *
 * filespec.c
 * Setzt neue Datei-Spezifikation
 *
 ***************************************************************************/


#include "ytree.h"




int SetFileSpec(char *file_spec)
{
  if( SetMatchSpec( file_spec ) )
  {
    return( 1 );
  }

  statistic.disk_matching_files = 0L;
  statistic.disk_matching_bytes = 0L;

  SetMatchingParam( statistic.tree );

  return( 0 );
}




void SetMatchingParam(DirEntry *dir_entry)
{
  DirEntry  *de_ptr;
  FileEntry *fe_ptr;
  unsigned long  matching_files;
  LONGLONG matching_bytes;

  for( de_ptr = dir_entry; de_ptr; de_ptr = de_ptr->next )
  {
    matching_files = 0L;
    matching_bytes = 0L;

    for( fe_ptr = de_ptr->file; fe_ptr; fe_ptr = fe_ptr->next )
    {
      if( Match( fe_ptr ) )
      {
	matching_files++;
	matching_bytes += fe_ptr->stat_struct.st_size;
	fe_ptr->matching = TRUE;
      }
      else
      {
	fe_ptr->matching = FALSE;
      }
    }

    de_ptr->matching_files = matching_files;
    de_ptr->matching_bytes = matching_bytes;

    statistic.disk_matching_files += matching_files;
    statistic.disk_matching_bytes += matching_bytes;

    if( de_ptr->sub_tree )
    {
      SetMatchingParam( de_ptr->sub_tree );
    }
  }
}



/***************************************************************>>
ReadFileSpec.
Take in the user-specified new filespec/filter.
Supports patterns, attributes, dates, and sizes.
<<***************************************************************/

int ReadFileSpec(void)
{
  int result = -1;

  char buffer[FILE_SPEC_LENGTH * 2 + 1];

  ClearHelp();

  (void) strcpy( buffer, "*" );

  /* Display help and prompt */
  MvAddStr( LINES - 3, 1, "Patterns: *.c,*.h,-*.o :r :x :l > < = 2025-01-01, 10k, 1M, 1G" );
  MvAddStr( LINES - 2, 1, "FILTER:" );

  /* Use available screen width (COLS - 9) but clamp to buffer size.
     x position is 8, so available width is COLS - 8 - 1 */
  if( InputString( buffer, LINES - 2, 8, 0, MINIMUM(FILE_SPEC_LENGTH, COLS - 9), "\r\033" ) == CR )
  {
    if( SetFileSpec( buffer ) )
    {
      MESSAGE( "Invalid Filter Spec" );
    }
    else
    {
      (void) strcpy( statistic.file_spec, buffer );
      result = 0;
    }
  }
  move( LINES - 3, 1 ); clrtoeol();
  move( LINES - 2, 1 ); clrtoeol();
  return(result);
}