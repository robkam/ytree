/***************************************************************************
 *
 * path_utils.c
 * Path construction and manipulation functions
 *
 ***************************************************************************/


#include "ytree.h"


char *GetPath(DirEntry *dir_entry, char *buffer)
{
  char *components[256];
  int i, depth = 0;
  DirEntry *de_ptr;

  *buffer = '\0';
  if (dir_entry == NULL) {
    return buffer;
  }

  /* Collect path components in reverse order (from leaf to root) */
  for (de_ptr = dir_entry; de_ptr != NULL && depth < 256; de_ptr = de_ptr->up_tree) {
    components[depth++] = de_ptr->name;
  }

  if (depth > 0) {
    /* The last component collected is the root. Start the path with it. */
    strcpy(buffer, components[depth - 1]);

    /* Append the rest of the components in correct order */
    for (i = depth - 2; i >= 0; i--) {
      /* Add separator if the current path is not just the root "/" */
      if (strcmp(buffer, FILE_SEPARATOR_STRING) != 0) {
        strcat(buffer, FILE_SEPARATOR_STRING);
      }
      strcat(buffer, components[i]);
    }
  }
  return buffer;
}


char *GetFileNamePath(FileEntry *file_entry, char *buffer)
{
  (void) GetPath( file_entry->dir_entry, buffer );
  if( *buffer && strcmp( buffer, FILE_SEPARATOR_STRING ) )
    (void) strcat( buffer, FILE_SEPARATOR_STRING );
  return( strcat( buffer, file_entry->name ) );
}


char *GetRealFileNamePath(FileEntry *file_entry, char *buffer)
{
  char *sym_name;

  if( mode == DISK_MODE || mode == USER_MODE )
    return( GetFileNamePath( file_entry, buffer ) );

  if( S_ISLNK( file_entry->stat_struct.st_mode ) )
  {
    sym_name = &file_entry->name[ strlen( file_entry->name ) + 1 ];
    if( *sym_name == FILE_SEPARATOR_CHAR )
      return( strcpy( buffer, sym_name ) );
  }

  (void) GetPath( file_entry->dir_entry, buffer );
  if( *buffer && strcmp( buffer, FILE_SEPARATOR_STRING ) )
    (void) strcat( buffer, FILE_SEPARATOR_STRING );
  if( S_ISLNK( file_entry->stat_struct.st_mode ) )
    return( strcat( buffer, &file_entry->name[ strlen( file_entry->name ) + 1 ] ) );
  else
    return( strcat( buffer, file_entry->name ) );
}


/* Aufsplitten des Dateinamens in die einzelnen Komponenten */
void Fnsplit(char *path, char *dir, char *name)
{
  int  i;
  char *name_begin;
  char *trunc_name;
  int dir_len = 0;

  while( *path == ' ' || *path == '\t' ) path++;

  while( strchr(path, FILE_SEPARATOR_CHAR ) || strchr(path, '\\') )
  {
    if (dir_len < PATH_LENGTH) {
        *(dir++) = *(path++);
        dir_len++;
    } else {
        /* Skip remaining directory part if buffer is full */
        path++;
    }
  }

  *dir = '\0';

  name_begin = path;
  trunc_name = name;

  for(i=0; i < PATH_LENGTH && *path; i++ )
    *(name++) = *(path++);

  *name = '\0';

  if( i == PATH_LENGTH && *path )
  {
    (void) snprintf( message, MESSAGE_LENGTH, "filename too long:*%s*truncating to*%s",
		    name_begin, trunc_name
		  );
    WARNING( message );
  }
}


char *GetExtension(char *filename)
{
  char *cptr;

  cptr = strrchr(filename, '.');

  if(cptr == NULL) return "";

  if(cptr == filename) return "";
  /* filenames beginning with a dot are not an extension */

  return(cptr + 1);
}


void NormPath( char *in_path, char *out_path )
{
  char *s, *d;
  char *old, *opath;
  int  level;
  char *in_path_dup;

  if( ( in_path_dup = malloc( strlen( in_path ) + 1 ) ) == NULL ) {
    ERROR_MSG( "Malloc Failed*ABORT" );
    exit( 1 );
  }

  level = 0;
  opath = out_path;

  if( *in_path == FILE_SEPARATOR_CHAR ) {
    s = in_path + 1;
    *opath++ = FILE_SEPARATOR_CHAR;
  } else {
    s = in_path;
  }

  for( d=in_path_dup; *s; d++ ) {
    *d = *s++;
    while( *d == FILE_SEPARATOR_CHAR && *s == FILE_SEPARATOR_CHAR )
      s++;
  }
  *d = '\0';

  d = opath;
  s = strtok_r( in_path_dup, FILE_SEPARATOR_STRING, &old );
  while( s ) {
    if( strcmp( s, "." ) ) {		/* skip "." */
      if( !strcmp( s, ".." ) ) {	/* optimize ".." */
        if( level > 0 ) {
          if( level == 1 ) {
	    d = out_path;
	  } else {
	    for( d -= 2; *d != FILE_SEPARATOR_CHAR; d-- )
	      ;
	    d++;
	  }
        } else {
          /* level <= 0 */
	  *d++ = '.';
	  *d++ = '.';
	  *d++ = FILE_SEPARATOR_CHAR;
        }
        level--;
      } else {				/* add component */
        strcpy( d, s );
        d += strlen( s );
        *d++ = FILE_SEPARATOR_CHAR;
        level++;
      }
    }
    s = strtok_r( NULL, FILE_SEPARATOR_STRING, &old );
  }
  if( level != 0 )
    d--;
  *d = '\0';
  if( *out_path == '\0' )
    strcpy(out_path, "." );

  free( in_path_dup );
}