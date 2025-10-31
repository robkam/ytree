/***************************************************************************
 *
 * functions to read filetree from 7z archive
 *
 ***************************************************************************/


#include "ytree.h"
#include <ctype.h>



static int GetStatFrom7z(char *zip_line, char *name, struct stat *stat);



/* read file tree from "7z l" output        */
/* depends on fixed output format of "7z l" */
/*------------------------------------------*/

int ReadTreeFrom7z(DirEntry *dir_entry, FILE *f)
{ 
  char zip_line[ZIP_LINE_LENGTH + 1];
  char path_name[PATH_LENGTH +1];
  struct stat stat;

  *dir_entry->name = '\0';

  while( fgets( zip_line, ZIP_LINE_LENGTH, f ) != NULL )
  {
    /* remove trailing \n */
    /*--------------------*/

    zip_line[ strlen( zip_line ) - 1 ] = '\0';

    if( strlen( zip_line ) > (unsigned) 52 && (zip_line[13] == ':' && zip_line[16] == ':') && zip_line[20] != ' ')
    {
      /* most likely valid file entry... */
      /*---------------------------------*/

      if( GetStatFrom7z( zip_line, path_name, &stat ) )
      {
        (void) sprintf( message, "unknown 7z entry*%s", zip_line );
        MESSAGE( message );
      }
      else
      {
        /* file */
        /*------*/
    
#ifdef DEBUG
  fprintf( stderr, "FILE: \"%s\"\n", path_name );
#endif
        if(*path_name)
          (void) InsertArchiveFileEntry( dir_entry, path_name, &stat );
      }
    }
  } 

  MinimizeArchiveTree( dir_entry );
  
  return( 0 );
}





static int GetStatFrom7z(char *zip_line, char *name, struct stat *stat)
{
  struct tm tm_struct;
  char entry[32];

  (void) memset( stat, 0, sizeof( struct stat ) );

  stat->st_nlink = 1;
  *name = '\0';

  if(zip_line[20] != '.')
    return 0;  /* skip non file entries */
  

  /* modification timestamp */
  /*------------------------*/

  tm_struct.tm_year  = atoi( SubString(entry, zip_line, 0, 4) ) - 1900;
  tm_struct.tm_mon   = atoi( SubString(entry, zip_line, 5, 2) ) - 1; 
  tm_struct.tm_mday  = atoi( SubString(entry, zip_line, 8, 2) );
  tm_struct.tm_hour  = atoi( SubString(entry, zip_line, 11, 2) );
  tm_struct.tm_min   = atoi( SubString(entry, zip_line, 14, 2));
  tm_struct.tm_sec   = atoi( SubString(entry, zip_line, 17, 2) );
  tm_struct.tm_isdst = -1;

  stat->st_atime = 0;
  stat->st_ctime = 0;

  stat->st_mtime = Mktime( &tm_struct );

  /* attributes: "DRHSA" (Windows) or "D...." (else) */
  /*-------------------------------------------------*/
 
  if(zip_line[21] == 'R')
    stat->st_mode = GetModus( "-r--r--r--" );
  else
    stat->st_mode = GetModus( "-rw-rw-rw-" );


  /* file length */
  /*-------------*/
  
  stat->st_size = AtoLL( SubString(entry, zip_line, 26, 12) );

  /* simulate owner and group */
  /*--------------------------*/

  stat->st_uid = (unsigned) getuid();
  stat->st_gid = (unsigned) getgid();
  
  /* filename */
  /*----------*/

  (void) strcpy( name, &zip_line[53] );

  return( 0 );
}


