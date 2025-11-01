/***************************************************************************
 *
 * Funktionen zum Lesen des Dateibaumes aus ZIP-Dateien
 *
 ***************************************************************************/


#include "ytree.h"



static int GetStatFromZIP(char *zip_line, char *name, struct stat *stat);



/* Dateibaum aus ZIP-Listing lesen */
/*---------------------------------*/

int ReadTreeFromZIP(DirEntry *dir_entry, FILE *f)
{ 
  char zip_line[ZIP_LINE_LENGTH + 1];
  char path_name[PATH_LENGTH +1];
  struct stat stat;

  *dir_entry->name = '\0';

  while( fgets( zip_line, ZIP_LINE_LENGTH, f ) != NULL )
  {
    /* \n loeschen */
    /*-------------*/

    zip_line[ strlen( zip_line ) - 1 ] = '\0';

    if( strlen( zip_line ) > (unsigned) 58 && (zip_line[56] == ':' || (zip_line[57] != 'd' && zip_line[58] == ':')))
    {
      /* gueltiger Eintrag */
      /*-------------------*/

      if( GetStatFromZIP( zip_line, path_name, &stat ) )
      {
        (void) sprintf( message, "unknown zipinfo*%s", zip_line );
        MESSAGE( message );
      }
      else
      {
        /* File */
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





static int GetStatFromZIP(char *zip_line, char *name, struct stat *stat)
{
  char *t, *old;
  int  i, id;
  struct tm tm_struct;
  static char *month[] = { "Jan", "Feb", "Mar", "Apr",  "May",  "Jun",
	 	           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };


  (void) memset( stat, 0, sizeof( struct stat ) );

  stat->st_nlink = 1;
  *name = '\0';

  t = strtok_r( zip_line, " \t", &old ); if( t == NULL ) return( -1 );

  /* Attributes */
  /*------------*/

  if( strlen( t ) == 10 )
  {
    stat->st_mode = GetModus( t );
  }
  else
  {
    /* DOS-Zip-File ? */
    /*----------------*/
    if(strlen(t) == 7 && *t == 'd')
      stat->st_mode = GetModus( "drw-rw-rw-" );
    else
      stat->st_mode = GetModus( "-rw-rw-rw-" );
  }

  if( !S_ISREG( stat->st_mode ) )
    return 0;  /* skip non file entries */

  
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  if(!strcmp(t, "file,"))  /* skip summary line */
    return 0;

  /* Version */
  /*---------*/
  
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* BS */
  /*----*/
  
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Dateilaenge */
  /*-------------*/

  if( !isdigit( *t ) ) return( -1 );
  stat->st_size = AtoLL( t );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* ?? */
  /*----*/

  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Compressed-Laenge */
  /*-------------------*/

  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Methode */
  /*---------*/

  t = strtok_r( NULL, " \t-", &old ); if( t == NULL ) return( -1 );

  /* M-Datum */
  /*---------*/

  tm_struct.tm_mday = atoi( t );
  t = strtok_r( NULL, " \t-", &old ); if( t == NULL ) return( -1 );

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
  
  tm_struct.tm_sec = 0;
  
  tm_struct.tm_isdst = -1;

  stat->st_atime = 0;
  stat->st_ctime = 0;

  /* Use standard mktime */
  stat->st_mtime = mktime( &tm_struct );

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

  t = strtok_r( NULL, "", &old ); if( t == NULL ) return( -1 ); /* ...to end of line */
  (void) strcpy( name, t );

  return( 0 );
}