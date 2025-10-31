/***************************************************************************
 *
 * Funktionen zum Lesen des Dateibaumes aus LHA-Dateien
 *
 ***************************************************************************/


#include "ytree.h"



static int GetStatFromLHA(char *lha_line, char *name, struct stat *stat);




/* Dateibaum aus LHA-Listing lesen */
/*---------------------------------*/

int ReadTreeFromLHA(DirEntry *dir_entry, FILE *f)
{ 
  char lha_line[LHA_LINE_LENGTH + 1];
  char path_name[PATH_LENGTH +1];
  struct stat stat;

  *dir_entry->name = '\0';

  while( fgets( lha_line, LHA_LINE_LENGTH, f ) != NULL )
  {
    /* \n loeschen */
    /*-------------*/

    lha_line[ strlen( lha_line ) - 1 ] = '\0';

    if( ( (strlen( lha_line ) > (unsigned) 55 && lha_line[55] == ':' ) || 
          (strlen( lha_line ) > (unsigned) 61 && lha_line[61] == ':' ) ) &&
  	  lha_line[34] != '*' && strncmp( &lha_line[1], "Total", 5 ) )
    {
      /* gueltiger Eintrag */
      /*-------------------*/

      if( GetStatFromLHA( lha_line, path_name, &stat ) )
      {
        (void) sprintf( message, "unknown lhainfo*%s", lha_line );
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





static int GetStatFromLHA(char *lha_line, char *name, struct stat *stat)
{
  char *t, *old;
  char modus[11]; 
  BOOL dos_mode = FALSE;
  int  i, id;
  struct tm tm_struct;
  static char *month[] = { "Jan", "Feb", "Mar", "Apr",  "May",  "Jun",
	 	           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };


  (void) memset( stat, 0, sizeof( struct stat ) );

  stat->st_nlink = 1;

  t = Strtok_r( lha_line, " \t", &old ); if( t == NULL ) return( -1 );

  /* Attributes */
  /*------------*/

  if( strlen( t ) == 9 && *t != '[' ) 
  {
    *modus = '-';
    (void) strcpy( &modus[1], t );
    stat->st_mode = GetModus( modus );
  }
  else if( *t == '[' )
  {
    stat->st_mode = GetModus( "-rw-r--r--" );
    dos_mode = TRUE;
  }
  else return( -1 );

  t = Strtok_r( NULL, " \t/", &old ); if( t == NULL ) return( -1 );

  if( dos_mode )
  {
    stat->st_uid = getuid();
    stat->st_gid = getgid();
  }
  else
  {
    /* Owner */
    /*-------*/
  
    id = atoi( t );
    stat->st_uid = (unsigned) id;
    t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );
  
    /* Group */
    /*-------*/
  
    id = atoi( t );
    stat->st_gid = (unsigned) id;
    t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );
  }

  /* Packed-Size */
  /*-------------*/

  if( !isdigit( *t ) ) return( -1 );
  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Dateilaenge */
  /*-------------*/

  if( !isdigit( *t ) ) return( -1 );
  stat->st_size = AtoLL( t );
  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Ratio */
  /*-------*/

  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* CRC */
  /*-----*/

  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  if( lha_line[61] == ':' )
  {
    /* ??? */
    /*-----*/

    t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );
  }

  /* M-Datum */
  /*---------*/

  for( i=0; i < 12; i++ )
  {
    if( !strcmp( t, month[i] ) ) break;
  }
  if( i >= 12 ) i = 0;

  tm_struct.tm_mon = i;
  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  tm_struct.tm_mday = atoi( t );
  t = Strtok_r( NULL, " \t:", &old ); if( t == NULL ) return( -1 );

  tm_struct.tm_hour = atoi( t );
  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  tm_struct.tm_min = atoi( t );
  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );
  
  if( lha_line[41] == '-' )
  {
    /* ??? */
    tm_struct.tm_year = 70; /* Start UNIX-Time */
  }
  else
  {
    tm_struct.tm_year = atoi( t ) - 1900;
    t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );
  }

  tm_struct.tm_sec = 0;
  
  tm_struct.tm_isdst = -1;

  stat->st_atime = 0;
  stat->st_ctime = 0;

  stat->st_mtime = Mktime( &tm_struct );

  /* Dateiname */
  /*-----------*/

  (void) strcpy( name, t );

  return( 0 );
}


