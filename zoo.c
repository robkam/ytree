/***************************************************************************
 *
 * Funktionen zum Lesen des Dateibaumes aus ZOO-Dateien
 *
 ***************************************************************************/


#include "ytree.h"


static int GetStatFromZOO(char *zoo_line, char *name, struct stat *stat);



/* Dateibaum aus ZOO-Listing lesen */
/*---------------------------------*/

int ReadTreeFromZOO(DirEntry *dir_entry, FILE *f)
{ 
  char zoo_line[ZOO_LINE_LENGTH + 1];
  char path_name[PATH_LENGTH +1];
  struct stat stat;

  *dir_entry->name = '\0';

  while( fgets( zoo_line, ZOO_LINE_LENGTH, f ) != NULL )
  {
    /* \n loeschen */
    /*-------------*/

    zoo_line[ strlen( zoo_line ) - 1 ] = '\0';

    if( strlen( zoo_line ) > (unsigned) 50 )
    {
      /* gueltiger Eintrag */
      /*-------------------*/

      if( GetStatFromZOO( zoo_line, path_name, &stat ) )
      {
        (void) sprintf( message, "unknown zooinfo*%s", zoo_line );
        MESSAGE( message );
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






static int GetStatFromZOO(char *zoo_line, char *name, struct stat *stat)
{
  char *t, *old;
  int  i, id;
  struct tm tm_struct;
  static char *month[] = { "Jan", "Feb", "Mar", "Apr",  "May",  "Jun",
	 	           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };


  (void) memset( stat, 0, sizeof( struct stat ) );

  stat->st_nlink = 1;

  t = strtok_r( zoo_line, " \t", &old ); if( t == NULL ) return( -1 );

  /* Dateilaenge */
  /*-------------*/

  if( !isdigit( *t ) ) return( -1 );
  stat->st_size = AtoLL( t );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* CF */
  /*----*/

  if( !isdigit( *t ) ) return( -1 );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Size Now */
  /*----------*/

  if( !isdigit( *t ) ) return( -1 );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* M-Datum */
  /*---------*/

  tm_struct.tm_mday = atoi( t );
  t = strtok_r( NULL, " \t:", &old ); if( t == NULL ) return( -1 );

  for( i=0; i < 12; i++ )
  {
    if( !strcmp( t, month[i] ) ) break;
  }
  if( i >= 12 ) i = 0;

  tm_struct.tm_mon = i;
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  tm_struct.tm_year = atoi( t );
  if(tm_struct.tm_year < 70)
     tm_struct.tm_year += 100;

  t = strtok_r( NULL, " \t:", &old ); if( t == NULL ) return( -1 );
  
  tm_struct.tm_hour = atoi( t );
  t = strtok_r( NULL, " \t:", &old ); if( t == NULL ) return( -1 );

  tm_struct.tm_min = atoi( t );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );
  
  tm_struct.tm_sec = atoi( t );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );
  
  tm_struct.tm_isdst = -1;

  stat->st_atime = 0;
  stat->st_ctime = 0;

  /* Use standard mktime */
  stat->st_mtime = mktime( &tm_struct );

  /* Attributes */
  /*------------*/

  (void) sscanf( t, "%o", (unsigned int *) &stat->st_mode );
  stat->st_mode |= S_IFREG;
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Owner */
  /*-------*/

  id = getuid();
  if( id == -1 ) id = atoi( t );
  stat->st_uid = (unsigned) id;

  /* Group */
  /*-------*/

  id = getgid();
  stat->st_gid = (unsigned) id;
  
  /* Dateiname */
  /*-----------*/

  (void) strcpy( name, t );

  return( 0 );
}