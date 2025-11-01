/***************************************************************************
 *
 * Read RPM-Files
 * Currently no extraction supported
 *
 ***************************************************************************/


#include "ytree.h"



static int GetStatFromRPM(char *rpm_line, char *name, struct stat *stat);



/* Dateibaum aus RPM-Listing lesen */
/*---------------------------------*/

int ReadTreeFromRPM(DirEntry *dir_entry, FILE *f)
{ 
  char rpm_line[RPM_LINE_LENGTH + 1];
  char path_name[PATH_LENGTH +1];
  struct stat stat;

  *dir_entry->name = '\0';

  while( fgets( rpm_line, RPM_LINE_LENGTH, f ) != NULL )
  {
    /* \n loeschen */
    /*-------------*/

    rpm_line[ strlen( rpm_line ) - 1 ] = '\0';

    if( GetStatFromRPM( rpm_line, path_name, &stat ) )
    {
      (void) sprintf( message, "unknown rpminfo*%s", rpm_line );
      MESSAGE( message );
    }
    else
    {
      if( (path_name[strlen( path_name ) - 1] == FILE_SEPARATOR_CHAR) ||
	  !strcmp( path_name, "." ) || S_ISDIR(stat.st_mode)
	)
      {
        /* Directory */
        /*-----------*/
        
#ifdef DEBUG
  fprintf( stderr, "DIR: %s\n", path_name );
#endif

	if( strcmp( path_name, "./" ) )
	{
	  /* "./" wird ignoriert */
	  /*---------------------*/

          (void) TryInsertArchiveDirEntry( dir_entry, path_name, &stat );
	  DisplayDiskStatistic();
	  doupdate();
	}
      }
      else
      {
        /* File */
        /*------*/
  
#ifdef DEBUG
  fprintf( stderr, "FILE: \"%s\"\n", path_name );
#endif
        (void) InsertArchiveFileEntry( dir_entry, path_name, &stat );
      }
    }
  }
  
  MinimizeArchiveTree( dir_entry );

  return( 0 );
}





static int GetStatFromRPM(char *rpm_line, char *name, struct stat *stat)
{
  char *t, *old;
  int  id;


  (void) memset( stat, 0, sizeof( struct stat ) );

  stat->st_nlink = 1;

  t = strtok_r( rpm_line, " \t", &old ); if( t == NULL ) return( -1 );

  /* Dateiname */
  /*-----------*/

  (void) strcpy( name, t );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Dateilaenge */
  /*-------------*/

  if( !isdigit( *t ) ) return( -1 );
  stat->st_size = AtoLL( t );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* M-Datum */
  /*---------*/

  stat->st_atime = 0;
  stat->st_ctime = 0;

  stat->st_mtime = atoi( t );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );
  
  /* M5 */
  if(strlen(t) > 20) {
    t = strtok_r( NULL, " \t/", &old ); if( t == NULL ) return( -1 );
  }

  /* Attribute */
  /*-----------*/

  stat->st_mode = strtoul(t, NULL, 8);
  t = strtok_r( NULL, " \t/", &old ); if( t == NULL ) return( -1 );


  /* Owner */
  /*-------*/

  id = GetPasswdUid( t );
  if( id == -1 ) id = atoi( t );
  stat->st_uid = (unsigned) id;

  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Group */
  /*-------*/

  id = GetGroupId( t );
  if( id == -1 ) id = atoi( t );
  stat->st_gid = (unsigned) id;
  

  if( S_ISLNK( stat->st_mode ) )
  {
    /* Symbolischer Link */
    /*-------------------*/

    t = "symlink";
    (void) strcpy( &name[ strlen( name ) + 1 ], t );
  }

  return( 0 );
}