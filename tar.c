/***************************************************************************
 *
 * Funktionen zum Lesen des Dateibaumes aus TAR-Dateien
 *
 ***************************************************************************/


#include "ytree.h"



static int GetStatFromTAR(char *tar_line, char *name, struct stat *stat);



/* read filetree from tar file */
/*-----------------------------*/

int ReadTreeFromTAR(DirEntry *dir_entry, FILE *f)
{ 
  char tar_line[TAR_LINE_LENGTH + 1];
  char path_name[PATH_LENGTH +1];
  struct stat stat;

  *dir_entry->name = '\0';

  while( fgets( tar_line, TAR_LINE_LENGTH, f ) != NULL )
  {
    /* remove \n */
    /*-----------*/

    tar_line[ strlen( tar_line ) - 1 ] = '\0';

    if( GetStatFromTAR( tar_line, path_name, &stat ) )
    {
      (void) sprintf( message, "unknown tarinfo*%s", tar_line );
      MESSAGE( message );
    }
    else
    {
      if( (path_name[strlen( path_name ) - 1] == FILE_SEPARATOR_CHAR) ||
	         !strcmp( path_name, "." ) || *tar_line == 'd' )
      {
        /* directory */
        /*-----------*/
        
#ifdef DEBUG
  fprintf( stderr, "DIR: %s\n", path_name );
#endif

	      if( strcmp( path_name, "./" ) )
	      {
	        /* ignore "./" */
	        /*-------------*/

          (void) TryInsertArchiveDirEntry( dir_entry, path_name, &stat );
	        DisplayDiskStatistic();
	        doupdate();
	      }
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





static int GetStatFromTAR(char *tar_line, char *name, struct stat *stat)
{
  char *t, *old;
  int  i, id;
  struct tm tm_struct;
  static char *month[] = { "Jan", "Feb", "Mar", "Apr",  "May",  "Jun",
	 	           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };


  (void) memset( stat, 0, sizeof( struct stat ) );

  *name = '\0';
  stat->st_nlink = 1;


  /* attribute */
  /*-----------*/

  t = Strtok_r( tar_line, " \t", &old ); if( t == NULL ) return( -1 );
  if( strlen( t ) != 10 ) return( -1 );
  stat->st_mode = GetModus( t );


  /* owner */
  /*-------*/

  t = Strtok_r( NULL, " \t/", &old ); if( t == NULL ) return( -1 );
  id = GetPasswdUid( t );
  if( id == -1 ) id = atoi( t );
  stat->st_uid = (unsigned) id;


  /* group */
  /*-------*/

  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );  id = GetGroupId( t );
  if( id == -1 ) id = atoi( t );
  stat->st_gid = (unsigned) id;
  

  /* file length */
  /*-------------*/

  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );
  if( !isdigit( *t ) ) return( -1 );
  stat->st_size = AtoLL( t );

  /* mod. date */
  /*-----------*/

  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  for( i=0; i < 12; i++ )
  {
    if( !strcmp( t, month[i] ) ) break;
  }
  if( i >= 12 ) 
  {
    t[4] = t[7] = '\0';
      
    tm_struct.tm_year = atoi(t) - 1900;
    tm_struct.tm_mon  = atoi(&t[5]) - 1;
    tm_struct.tm_mday = atoi(&t[8]);
    
    t = Strtok_r( NULL, " \t:", &old ); if( t == NULL ) return( -1 );
    tm_struct.tm_hour = atoi( t );
    t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );
    tm_struct.tm_min = atoi( t );
    goto XDATE;
  } 
    

  tm_struct.tm_mon = i;
  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  tm_struct.tm_mday = atoi( t );
  t = Strtok_r( NULL, " \t:", &old ); if( t == NULL ) return( -1 );

  tm_struct.tm_hour = atoi( t );
  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  tm_struct.tm_min = atoi( t );
  t = Strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );
  
  tm_struct.tm_year = atoi( t ) - 1900;
  
XDATE:

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  stat->st_atime = 0;
  stat->st_ctime = 0;

  stat->st_mtime = Mktime( &tm_struct );

  /* filename */
  /*----------*/

  t = Strtok_r( NULL, "", &old ); if( t == NULL ) return( -1 );

  if( S_ISLNK( stat->st_mode ) )
  {
    /* symbolic link */
    /*---------------*/

    char *cptr = strstr(t, " -> ");
    if(cptr)
    {
      *cptr = '\0';
      (void) strcpy( name, t );
      (void) strcpy( &name[ strlen( name ) + 1 ], cptr + 4 );
    }
    else
    {
      cptr = strstr(t, " symbolic link to ");  /* non gnu tar */
      if(cptr)
      {
        *cptr = '\0';
        (void) strcpy( name, t );
        (void) strcpy( &name[ strlen( name ) + 1 ], cptr + 4 );
      }
      else
      {
        /* ... unexpected */
        (void) strcpy( name, t );
      }
    }
  }
  else
  {
    /* ... to end of line */
    (void) strcpy( name, t );
  }

  return( 0 );
}


