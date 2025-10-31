/***************************************************************************
 *
 * Funktionen zum Lesen des Dateibaumes aus RAR-Dateien
 *
 ***************************************************************************/


#include "ytree.h"
#include <ctype.h>


static int GetStatFromRAR(char *rar_line, char *name, struct stat *stat);
static int GetStatFromNewRAR(char *rar_line, char *name, struct stat *stat);



/* Dateibaum aus RAR-Listing lesen */
/*---------------------------------*/

int ReadTreeFromRAR(DirEntry *dir_entry, FILE *f)
{ 
  char rar_line[RAR_LINE_LENGTH + 1];
  char path_name[PATH_LENGTH +1];
  struct stat stat;

  *dir_entry->name = '\0';

  while( fgets( rar_line, RAR_LINE_LENGTH, f ) != NULL )
  {
    /* \n loeschen */
    /*-------------*/

    rar_line[ strlen( rar_line ) - 1 ] = '\0';

    if( strlen( rar_line ) > (unsigned) 48 && rar_line[48] == ':' )
    {
      /* gueltiger Eintrag */
      /*-------------------*/

      if( GetStatFromRAR( rar_line, path_name, &stat ) )
      {
        (void) sprintf( message, "unknown rarinfo*%s", rar_line );
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

    } else if( strlen( rar_line ) > (unsigned) 36 )
    {
      /* valid entry */
      /*-------------*/

      if( GetStatFromNewRAR( rar_line, path_name, &stat ) )
      {
        (void) sprintf( message, "unknown rarinfo*%s", rar_line );
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






static int GetStatFromRAR(char *rar_line, char *name, struct stat *stat)
{
  char *t, *old;
  int  id;
  struct tm tm_struct;


  (void) memset( stat, 0, sizeof( struct stat ) );
  *name = '\0';

  stat->st_nlink = 1;

  t = Strtok_r( rar_line, " \t", &old ); if( t == NULL ) return( -1 );

  /* Dateiname */
  /*-----------*/

  (void) strcpy( name, t );
  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Dateilaenge */
  /*-------------*/

  if( !isdigit( *t ) ) return( -1 );
  stat->st_size = AtoLL( t );
  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Packed */
  /*--------*/

  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Ratio */
  /*-------*/

  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* M-Datum */
  /*---------*/

  if(strlen(t) == 8) {
    t[2] = t[5] = '\0';    
    tm_struct.tm_mday = atoi( &t[0] );
    tm_struct.tm_mon  = atoi( &t[3] );
    tm_struct.tm_year = atoi( &t[6] );

    if(tm_struct.tm_year < 70)
       tm_struct.tm_year += 100;
  }

  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* M-time */
  /*--------*/

  if(strlen(t) == 5) {
    t[2] = '\0';
    tm_struct.tm_hour = atoi( &t[0] );
    tm_struct.tm_min  = atoi( &t[2] );
  }

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  stat->st_atime = 0;
  stat->st_ctime = 0;

  stat->st_mtime = Mktime( &tm_struct );

  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

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



static int GetStatFromNewRAR(char *rar_line, char *name, struct stat *stat)
{
  char *t, *old;
  int  id;
  struct tm tm_struct;


  (void) memset( stat, 0, sizeof( struct stat ) );
  *name = '\0';

  stat->st_nlink = 1;

  /* attributes */
  /*------------*/

  t = Strtok_r( rar_line, " \t", &old ); if( t == NULL ) return( -1 );

  if((strlen(t) == 7) && t[2] != 'A')
    return 0;  /* skip non file entries */

  if(strlen(t) == 10)
    stat->st_mode = GetModus(t);
  else if(strlen(t) == 7)
    stat->st_mode = GetModus("-rw-rw-rw-");
  else
    return 0;  /* invalid entry */

  /* size */
  /*------*/

  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( 0 );
  if( !isdigit( *t ) ) return( 0 ); /* skip invalid entry */
  stat->st_size = AtoLL( t );


  /* date */
  /*------*/

  memset(&tm_struct, 0, sizeof(tm_struct));

  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( 0 );

  if(strlen(t) == 10) {
    t[4] = t[7] = '\0';    
    tm_struct.tm_mday = atoi( &t[8]);
    tm_struct.tm_mon  = atoi( &t[5]) - 1;
    tm_struct.tm_year = atoi( &t[0]) - 1900;
  }

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  /* time */
  /*------*/

  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( 0 );

  if(strlen(t) == 5 && t[2] == ':') {
    t[2] = '\0';    
    tm_struct.tm_hour = atoi( &t[0] );
    tm_struct.tm_min  = atoi( &t[3] );
  }

  stat->st_atime = 0;
  stat->st_ctime = 0;

  stat->st_mtime = Mktime( &tm_struct );


  /* filename */
  /*-----------*/

  t = Strtok_r( NULL, "", &old ); if( t == NULL ) return( 0 );
  while(isspace(*t))
    t++;
  (void) strcpy( name, t );

  /* owner */
  /*-------*/

  id = getuid();
  if( id == -1 ) id = atoi( t );
  stat->st_uid = (unsigned) id;

  /* group */
  /*-------*/

  id = getgid();
  stat->st_gid = (unsigned) id;
  
  return( 0 );
}



