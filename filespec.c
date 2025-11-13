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
      if( Match( fe_ptr->name ) )
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
Take in the user-specified new filespec.
As modified, it defaults to '*'; the original version offered the
current value as default, but that's just an up-arrow away.
Returns 0 on success, -1 on failure (empty string).
<<***************************************************************/

int ReadFileSpec(void)
{
  int result = -1;

  char buffer[FILE_SPEC_LENGTH * 2 + 1];

  ClearHelp();

  (void) strcpy( buffer, "*" );
  MvAddStr( LINES - 2, 1, "New filespec:" );
  if( InputString( buffer, LINES - 2, 15, 0, FILE_SPEC_LENGTH, "\r\033" ) == CR )
  {
    if( SetFileSpec( buffer ) )
    {
      MESSAGE( "Invalid Filespec" );
    }
    else
    {
      (void) strcpy( statistic.file_spec, buffer );
      result = 0;
    }
  }
  move( LINES - 2, 1 ); clrtoeol();
  return(result);
}


