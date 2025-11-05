/***************************************************************************
 *
 * functions to read filetree from various archives (TAR, ZIP, ZOO, etc.)
 *
 ***************************************************************************/


#include "ytree.h"
#include <ctype.h>

/* --- Forward Declarations for Internal Parsers --- */

static int GetStatFrom7z(char *zip_line, char *name, struct stat *stat);
static int GetStatFromARC(char *arc_line, char *name, struct stat *stat);
static int GetStatFromLHA(char *lha_line, char *name, struct stat *stat);
static int GetStatFromRAR(char *rar_line, char *name, struct stat *stat);
static int GetStatFromNewRAR(char *rar_line, char *name, struct stat *stat);
static int GetStatFromRPM(char *rpm_line, char *name, struct stat *stat);
static int GetStatFromTAR(char *tar_line, char *name, struct stat *stat);
static int GetStatFromZIP(char *zip_line, char *name, struct stat *stat);
static int GetStatFromZOO(char *zoo_line, char *name, struct stat *stat);


/* 
 * Dispatcher function to read file tree from archive listing.
 * This replaces all ReadTreeFrom... functions.
 */
int ReadTreeFromArchive(DirEntry *dir_entry, FILE *f, int file_mode)
{
  char line[ZIP_LINE_LENGTH + 1]; /* Use largest known line length */
  char path_name[PATH_LENGTH + 1];
  struct stat stat_struct;
  int (*GetStatFunc)(char *, char *, struct stat *) = NULL;
  int line_length_threshold = 0;
  char *line_length_check = NULL;
  BOOL skip_initial_lines = FALSE;
  
  *dir_entry->name = '\0';

  /* Determine parser and line check parameters based on file_mode */
  switch (file_mode)
  {
    /* All 7z-compatible archive modes use the 7z parser */
    case SEVENZIP_FILE_MODE:
    case ISO_FILE_MODE:
    case ZIP_FILE_MODE: 
    case LHA_FILE_MODE:
    case RAR_FILE_MODE:
      GetStatFunc = GetStatFrom7z;
      line_length_threshold = 52;
      break;

    case ARC_FILE_MODE:
      GetStatFunc = GetStatFromARC;
      line_length_threshold = 63;
      line_length_check = ":";
      break;

    case RPM_FILE_MODE:
      GetStatFunc = GetStatFromRPM;
      line_length_threshold = 0;
      break;

    case TAR_FILE_MODE:
    case TAPE_MODE:
      GetStatFunc = GetStatFromTAR;
      line_length_threshold = 0;
      break;

    case ZOO_FILE_MODE:
      GetStatFunc = GetStatFromZOO;
      line_length_threshold = 50;
      break;

    default:
      (void) sprintf(message, "Internal Error*Unknown archive mode %d", file_mode);
      ERROR_MSG(message);
      return -1;
  }
  
  /* For 7z-based outputs, skip header until likely file entries start */
  if (file_mode == ZIP_FILE_MODE || file_mode == SEVENZIP_FILE_MODE || 
      file_mode == ISO_FILE_MODE || file_mode == LHA_FILE_MODE || 
      file_mode == RAR_FILE_MODE)
  {
      /* The 7z output starts with a header, skip lines until valid pattern found */
      while (fgets(line, ZIP_LINE_LENGTH, f) != NULL)
      {
          line[strlen(line) - 1] = '\0';
          /* Check for 7z format: YYYY-MM-DD HH:MM:SS */
          if (strlen(line) > (unsigned)52 && line[13] == ':' && line[16] == ':')
          {
              /* This is the first file entry line (or a directory). 
                 Break and let the main loop re-process it. */
              break; 
          }
      }
  }


  while (fgets(line, ZIP_LINE_LENGTH, f) != NULL)
  {
    /* remove trailing \n */
    line[strlen(line) - 1] = '\0';

    /* Custom line filtering for efficiency/robustness */
    if (strlen(line) < (unsigned)line_length_threshold)
      continue;

    /* Execute parser */
    if (GetStatFunc && !GetStatFunc(line, path_name, &stat_struct))
    {
      /* Success: Insert into the tree */
      if (*path_name)
      {
        /* Directories need to be pre-inserted by ReadTreeFrom... or handled here for RPM/TAR */
        if (file_mode == RPM_FILE_MODE || file_mode == TAR_FILE_MODE)
        {
            if (S_ISDIR(stat_struct.st_mode) || 
                (path_name[strlen(path_name) - 1] == FILE_SEPARATOR_CHAR))
            {
                if (strcmp(path_name, "./"))
                {
                    (void)TryInsertArchiveDirEntry(dir_entry, path_name, &stat_struct);
                    DisplayDiskStatistic();
                    doupdate();
                }
                continue;
            }
        }
        
        (void) InsertArchiveFileEntry(dir_entry, path_name, &stat_struct);
      }
    }
    else
    {
        /* Error/Unknown format (expected for many headers/footers) */
        ; 
    }
  }

  MinimizeArchiveTree(dir_entry);
  return 0;
}


/* ------------------------------------------------------------------------- */
/* --- Internal GetStatFrom... Implementations (from 7z.c, arc.c, etc.) ---- */
/* ------------------------------------------------------------------------- */

/* From 7z.c (Now reused for ZIP, ISO, LHA, RAR, 7Z, GZ, BZ2) */
static int GetStatFrom7z(char *zip_line, char *name, struct stat *stat)
{
  struct tm tm_struct;
  char entry[32];

  (void) memset( stat, 0, sizeof( struct stat ) );

  stat->st_nlink = 1;
  *name = '\0';

  /* 7z format filter: must have a '.' at position 20 and valid time at 13/16/20 */
  if(!(strlen( zip_line ) > (unsigned) 52 && (zip_line[13] == ':' && zip_line[16] == ':') && zip_line[20] != ' '))
  {
      /* Not a standard 7z line, possibly a header/footer or directory. Return non-zero to treat as unknown. */
      return -1; 
  }
  
  if(zip_line[20] != '.')
    return 0;  /* skip non file entries (like directory markers) */
  

  /* modification timestamp */
  tm_struct.tm_year  = atoi( SubString(entry, zip_line, 0, 4) ) - 1900;
  tm_struct.tm_mon   = atoi( SubString(entry, zip_line, 5, 2) ) - 1; 
  tm_struct.tm_mday  = atoi( SubString(entry, zip_line, 8, 2) );
  tm_struct.tm_hour  = atoi( SubString(entry, zip_line, 11, 2) );
  tm_struct.tm_min   = atoi( SubString(entry, zip_line, 14, 2));
  tm_struct.tm_sec   = atoi( SubString(entry, zip_line, 17, 2) );
  tm_struct.tm_isdst = -1;

  stat->st_atime = 0;
  stat->st_ctime = 0;

  /* Use standard mktime */
  stat->st_mtime = mktime( &tm_struct );

  /* attributes: "DRHSA" (Windows) or "D...." (else) */
  if(zip_line[21] == 'R')
    stat->st_mode = GetModus( "-r--r--r--" );
  else
    stat->st_mode = GetModus( "-rw-rw-rw-" );


  /* file length */
  stat->st_size = strtoll( SubString(entry, zip_line, 26, 12), NULL, 10 );

  /* simulate owner and group */
  stat->st_uid = (unsigned) getuid();
  stat->st_gid = (unsigned) getgid();
  
  /* filename */
  (void) strcpy( name, &zip_line[53] );

  return( 0 );
}

/* From arc.c */
static int GetStatFromARC(char *arc_line, char *name, struct stat *stat)
{
  char *t, *old;
  int  i, id;
  struct tm tm_struct;
  static char *month[] = { "Jan", "Feb", "Mar", "Apr",  "May",  "Jun",
	 	           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };


  (void) memset( stat, 0, sizeof( struct stat ) );

  stat->st_nlink = 1;

  t = strtok_r( arc_line, " \t", &old ); if( t == NULL ) return( -1 );

  /* Dateiname */
  (void) strcpy( name, t );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Dateilaenge */
  if( !isdigit( *t ) ) return( -1 );
  stat->st_size = strtoll( t, NULL, 10 );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Stowage */
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* SF */
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Size Now */
  if( !isdigit( *t ) ) return( -1 );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* M-Datum */
  tm_struct.tm_mday = atoi( t );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

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
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  if( t[strlen(t)-1] == 'p' ) tm_struct.tm_hour += 12;
  t[strlen(t)-1] ='\0';
  
  tm_struct.tm_min = atoi( t );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );
  
  tm_struct.tm_sec = 0;
  
  tm_struct.tm_isdst = -1;

  stat->st_atime = 0;
  stat->st_ctime = 0;

  /* Use standard mktime */
  stat->st_mtime = mktime( &tm_struct );

  /* Attributes */
  stat->st_mode = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

  /* Owner */
  id = getuid();
  if( id == -1 ) id = atoi( t );
  stat->st_uid = (unsigned) id;

  /* Group */
  id = getgid();
  stat->st_gid = (unsigned) id;
  
  return( 0 );
}

/* From lha.c (now fallback only) */
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

  t = strtok_r( lha_line, " \t", &old ); if( t == NULL ) return( -1 );

  /* Attributes */
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

  t = strtok_r( NULL, " \t/", &old ); if( t == NULL ) return( -1 );

  if( dos_mode )
  {
    stat->st_uid = getuid();
    stat->st_gid = getgid();
  }
  else
  {
    /* Owner */
    id = atoi( t );
    stat->st_uid = (unsigned) id;
    t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );
  
    /* Group */
    id = atoi( t );
    stat->st_gid = (unsigned) id;
    t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );
  }

  /* Packed-Size */
  if( !isdigit( *t ) ) return( -1 );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Dateilaenge */
  if( !isdigit( *t ) ) return( -1 );
  stat->st_size = strtoll( t, NULL, 10 );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Ratio */
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* CRC */
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  if( lha_line[61] == ':' )
  {
    /* ??? */
    t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );
  }

  /* M-Datum */
  for( i=0; i < 12; i++ )
  {
    if( !strcmp( t, month[i] ) ) break;
  }
  if( i >= 12 ) i = 0;

  tm_struct.tm_mon = i;
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  tm_struct.tm_mday = atoi( t );
  t = strtok_r( NULL, " \t:", &old ); if( t == NULL ) return( -1 );

  tm_struct.tm_hour = atoi( t );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  tm_struct.tm_min = atoi( t );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );
  
  if( lha_line[41] == '-' )
  {
    /* ??? */
    tm_struct.tm_year = 70; /* Start UNIX-Time */
  }
  else
  {
    tm_struct.tm_year = atoi( t ) - 1900;
    t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );
  }

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  stat->st_atime = 0;
  stat->st_ctime = 0;

  /* Use standard mktime */
  stat->st_mtime = mktime( &tm_struct );

  /* Dateiname */
  (void) strcpy( name, t );

  return( 0 );
}

/* From rar.c (old format - now fallback only) */
static int GetStatFromRAR(char *rar_line, char *name, struct stat *stat)
{
  char *t, *old;
  int  id;
  struct tm tm_struct;


  (void) memset( stat, 0, sizeof( struct stat ) );
  *name = '\0';

  stat->st_nlink = 1;

  t = strtok_r( rar_line, " \t", &old ); if( t == NULL ) return( -1 );

  /* Dateiname */
  (void) strcpy( name, t );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Dateilaenge */
  if( !isdigit( *t ) ) return( -1 );
  stat->st_size = strtoll( t, NULL, 10 );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Packed */
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Ratio */
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* M-Datum */
  if(strlen(t) == 8) {
    t[2] = t[5] = '\0';    
    tm_struct.tm_mday = atoi( &t[0] );
    tm_struct.tm_mon  = atoi( &t[3] );
    tm_struct.tm_year = atoi( &t[6] );

    if(tm_struct.tm_year < 70)
       tm_struct.tm_year += 100;
  } else {
      /* Fallback for missing date part */
      memset(&tm_struct, 0, sizeof(tm_struct));
      tm_struct.tm_year = 70;
  }


  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* M-time */
  if(strlen(t) == 5) {
    t[2] = '\0';
    tm_struct.tm_hour = atoi( &t[0] );
    tm_struct.tm_min  = atoi( &t[3] );
  }

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  stat->st_atime = 0;
  stat->st_ctime = 0;

  /* Use standard mktime */
  stat->st_mtime = mktime( &tm_struct );

  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Attributes */
  stat->st_mode = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

  /* Owner */
  id = getuid();
  if( id == -1 ) id = atoi( t );
  stat->st_uid = (unsigned) id;

  /* Group */
  id = getgid();
  stat->st_gid = (unsigned) id;
  
  return( 0 );
}

/* From rar.c (new format - now fallback only) */
static int GetStatFromNewRAR(char *rar_line, char *name, struct stat *stat)
{
  char *t, *old;
  int  id;
  struct tm tm_struct;


  (void) memset( stat, 0, sizeof( struct stat ) );
  *name = '\0';

  stat->st_nlink = 1;

  /* attributes */
  t = strtok_r( rar_line, " \t", &old ); if( t == NULL ) return( 0 );

  if((strlen(t) == 7) && t[2] != 'A')
    return 0;  /* skip non file entries */

  if(strlen(t) == 10)
    stat->st_mode = GetModus(t);
  else if(strlen(t) == 7)
    stat->st_mode = GetModus("-rw-rw-rw-");
  else
    return 0;  /* invalid entry */

  /* size */
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( 0 );
  if( !isdigit( *t ) ) return( 0 ); /* skip invalid entry */
  stat->st_size = strtoll( t, NULL, 10 );


  /* date */
  memset(&tm_struct, 0, sizeof(tm_struct));

  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( 0 );

  if(strlen(t) == 10) {
    t[4] = t[7] = '\0';    
    tm_struct.tm_mday = atoi( &t[8]);
    tm_struct.tm_mon  = atoi( &t[5]) - 1;
    tm_struct.tm_year = atoi( &t[0]) - 1900;
  }

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  /* time */
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( 0 );

  if(strlen(t) == 5 && t[2] == ':') {
    t[2] = '\0';    
    tm_struct.tm_hour = atoi( &t[0] );
    tm_struct.tm_min  = atoi( &t[3] );
  }

  stat->st_atime = 0;
  stat->st_ctime = 0;

  /* Use standard mktime */
  stat->st_mtime = mktime( &tm_struct );


  /* filename */
  t = strtok_r( NULL, "", &old ); if( t == NULL ) return( 0 );
  while(isspace(*t))
    t++;
  (void) strcpy( name, t );

  /* owner */
  id = getuid();
  if( id == -1 ) id = atoi( t );
  stat->st_uid = (unsigned) id;

  /* group */
  id = getgid();
  stat->st_gid = (unsigned) id;
  
  return( 0 );
}

/* From rpm.c */
static int GetStatFromRPM(char *rpm_line, char *name, struct stat *stat)
{
  char *t, *old;
  int  id;


  (void) memset( stat, 0, sizeof( struct stat ) );

  stat->st_nlink = 1;

  t = strtok_r( rpm_line, " \t", &old ); if( t == NULL ) return( -1 );

  /* Dateiname */
  (void) strcpy( name, t );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Dateilaenge */
  if( !isdigit( *t ) ) return( -1 );
  stat->st_size = strtoll( t, NULL, 10 );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* M-Datum */
  stat->st_atime = 0;
  stat->st_ctime = 0;

  stat->st_mtime = atoi( t );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );
  
  /* M5 */
  if(strlen(t) > 20) {
    t = strtok_r( NULL, " \t/", &old ); if( t == NULL ) return( -1 );
  }

  /* Attribute */
  stat->st_mode = strtoul(t, NULL, 8);
  t = strtok_r( NULL, " \t/", &old ); if( t == NULL ) return( -1 );


  /* Owner */
  id = GetPasswdUid( t );
  if( id == -1 ) id = atoi( t );
  stat->st_uid = (unsigned) id;

  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Group */
  id = GetGroupId( t );
  if( id == -1 ) id = atoi( t );
  stat->st_gid = (unsigned) id;
  

  if( S_ISLNK( stat->st_mode ) )
  {
    /* Symbolischer Link */
    t = "symlink";
    (void) strcpy( &name[ strlen( name ) + 1 ], t );
  }

  return( 0 );
}

/* From tar.c */
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
  t = strtok_r( tar_line, " \t", &old ); if( t == NULL ) return( -1 );
  if( strlen( t ) != 10 ) return( -1 );
  stat->st_mode = GetModus( t );


  /* owner */
  t = strtok_r( NULL, " \t/", &old ); if( t == NULL ) return( -1 );
  id = GetPasswdUid( t );
  if( id == -1 ) id = atoi( t );
  stat->st_uid = (unsigned) id;


  /* group */
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );  id = GetGroupId( t );
  if( id == -1 ) id = atoi( t );
  stat->st_gid = (unsigned) id;
  

  /* file length */
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );
  if( !isdigit( *t ) ) return( -1 );
  stat->st_size = strtoll( t, NULL, 10 );

  /* mod. date */
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  for( i=0; i < 12; i++ )
  {
    if( !strcmp( t, month[i] ) ) break;
  }
  if( i >= 12 ) 
  {
    /* new date format e.g. 2004-03-21 00:00 */
    t[4] = t[7] = '\0';
      
    tm_struct.tm_year = atoi(t) - 1900;
    tm_struct.tm_mon  = atoi(&t[5]) - 1;
    tm_struct.tm_mday = atoi(&t[8]);
    
    t = strtok_r( NULL, " \t:", &old ); if( t == NULL ) return( -1 );
    tm_struct.tm_hour = atoi( t );
    t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );
    tm_struct.tm_min = atoi( t );
    goto XDATE;
  } 
    

  tm_struct.tm_mon = i;
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  tm_struct.tm_mday = atoi( t );
  t = strtok_r( NULL, " \t:", &old ); if( t == NULL ) return( -1 );

  tm_struct.tm_hour = atoi( t );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  tm_struct.tm_min = atoi( t );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );
  
  tm_struct.tm_year = atoi( t ) - 1900;
  
XDATE:

  tm_struct.tm_sec = 0;
  tm_struct.tm_isdst = -1;

  /* Use standard mktime */
  stat->st_mtime = mktime( &tm_struct );

  /* filename */
  t = strtok_r( NULL, "", &old ); if( t == NULL ) return( -1 );

  if( S_ISLNK( stat->st_mode ) )
  {
    /* symbolic link */
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

/* From zip.c (now fallback only) */
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
  if( strlen( t ) == 10 )
  {
    stat->st_mode = GetModus( t );
  }
  else
  {
    /* DOS-Zip-File ? */
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
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* BS */
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Dateilaenge */
  if( !isdigit( *t ) ) return( -1 );
  stat->st_size = strtoll( t, NULL, 10 );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* ?? */
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Compressed-Laenge */
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Methode */
  t = strtok_r( NULL, " \t-", &old ); if( t == NULL ) return( -1 );

  /* M-Datum */
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
  id = getuid();
  if( id == -1 ) id = atoi( t );
  stat->st_uid = (unsigned) id;

  /* Group */
  id = getgid();
  stat->st_gid = (unsigned) id;
  
  /* Dateiname */
  t = strtok_r( NULL, "", &old ); if( t == NULL ) return( -1 ); /* ...to end of line */
  (void) strcpy( name, t );

  return( 0 );
}

/* From zoo.c */
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
  if( !isdigit( *t ) ) return( -1 );
  stat->st_size = strtoll( t, NULL, 10 );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* CF */
  if( !isdigit( *t ) ) return( -1 );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Size Now */
  if( !isdigit( *t ) ) return( -1 );
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* M-Datum */
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
  (void) sscanf( t, "%o", (unsigned int *) &stat->st_mode );
  stat->st_mode |= S_IFREG;
  t = strtok_r( NULL, " \t", &old ); if( t == NULL ) return( -1 );

  /* Owner */
  id = getuid();
  if( id == -1 ) id = atoi( t );
  stat->st_uid = (unsigned) id;

  /* Group */
  id = getgid();
  stat->st_gid = (unsigned) id;
  
  /* Dateiname */
  (void) strcpy( name, t );

  return( 0 );
}