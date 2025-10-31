/***************************************************************************
 *
 * Funktionen zum Lesen des Dateibaumes aus ARC-Dateien
 *
 ***************************************************************************/


#include "ytree.h"


static int GetStatFromARC(char *arc_line, char *name, struct stat *stat);



/* Dateibaum aus ARC-Listing lesen */
/*---------------------------------*/

int ReadTreeFromARC(DirEntry *dir_entry, FILE *f)
{ 
  char arc_line[ARC_LINE_LENGTH + 1];
  char path_name[PATH_LENGTH +1];
  struct stat stat;

  *dir_entry->name = '\0';

  while( fgets( arc_line, ARC_LINE_LENGTH, f ) != NULL )
  {
    /* \n loeschen */
    /*-------------*/

    arc_line[ strlen( arc_line ) - 1 ] = '\0';

    if( strlen( arc_line ) > (unsigned) 63 && arc_line[63] == ':' )
    {
      /* gueltiger Eintrag */
      /*-------------------*/

      if( GetStatFromARC( arc_line, path_name, &stat ) )
      {
        (void) sprintf( message, "unknown arcinfo*%s", arc_line );
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






static int GetStatFromARC(char *arc_line, char *name, struct stat *stat)
{
  char *t, *old;
  int  i, id;
  struct tm tm_struct;
  static char *month[] = { "Jan", "Feb", "Mar", "Apr",  "May",  "Jun",
	 	           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };


  (void) memset( stat, 0, sizeof( struct stat ) );

  stat->st_nlink = 1;

  t = Strtok_r( arc_line, " \t", &old ); if( t == NULL ) return( -1 );

  /* Dateiname */
  /*-----------*/

  (void) strcpy( name, t );
  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Dateilaenge */
  /*-------------*/

  if( !isdigit( *t ) ) return( -1 );
  stat->st_size = AtoLL( t );
  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Stowage */
  /*---------*/

  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* SF */
  /*----*/

  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Size Now */
  /*----------*/

  if( !isdigit( *t ) ) return( -1 );
  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* M-Datum */
  /*---------*/

  tm_struct.tm_mday = atoi( t );
  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  for( i=0; i < 12; i++ )
  {
    if( !strcmp( t, month[i] ) ) break;
  }
  if( i >= 12 ) i = 0;

  tm_struct.tm_mon = i;
  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  tm_struct.tm_year = atoi( t );
  if(tm_struct.tm_year < 70)
    tm_struct.tm_year += 100;

  t = Strtok_r( NULL, " \t:", &old ); if( t == NULL ) return( -1 );
  
  tm_struct.tm_hour = atoi( t );
  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  if( t[strlen(t)-1] == 'p' ) tm_struct.tm_hour += 12;
  t[strlen(t)-1] ='\0';
  
  tm_struct.tm_min = atoi( t );
  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );
  
  tm_struct.tm_sec = 0;
  
  tm_struct.tm_isdst = -1;

  stat->st_atime = 0;
  stat->st_ctime = 0;

  stat->st_mtime = Mktime( &tm_struct );

  /* Attributes */
  /*------------*/

  stat->st_mode = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

  /* Owner */
  /*-------*/

  id = getuid();
  if( id == -1 ) id = atoi( t );
  stat->st_uid = (unsigned) id;

  /* Group */
  /*-------*/

  id = getgid();
  stat->st_gid = (unsigned) id;
  
  return( 0 );
}


