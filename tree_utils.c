/***************************************************************************
 *
 * Functions for searching and navigating the in-memory directory tree
 *
 ***************************************************************************/


#include "ytree.h"


int GetDirEntry(DirEntry *tree, 
                DirEntry *current_dir_entry, 
                char *dir_path, 
                DirEntry **dir_entry, 
                char *to_path
	       )
{
  char dest_path[PATH_LENGTH+1];
  char current_path[PATH_LENGTH+1];
  char help_path[PATH_LENGTH+1];
  char *token, *old;
  DirEntry *de_ptr, *sde_ptr;
  int n;

  *dir_entry = NULL;
  *to_path   = '\0';

   strcpy(to_path, dir_path);
  if( getcwd( current_path, sizeof( current_path ) - 2 ) == NULL )
  {
    (void) sprintf( message, "getcwd failed*%s", strerror(errno) );
    ERROR_MSG( message );
    return( -1 );
  }

  if( *dir_path != FILE_SEPARATOR_CHAR ) 
  {
    if( chdir( GetPath( current_dir_entry, help_path ) ) )
    {
      ERROR_MSG( "Chdir Failed" );
      return( -1 );
    }
  }

  if( chdir( dir_path ) )
  {
#ifdef DEBUG
    (void) sprintf( message, "Invalid Path!*\"%s\"", dir_path );
    MESSAGE( message );
#endif
    return( -3 );
  }

  if( *dir_path != FILE_SEPARATOR_CHAR ) {
    (void) getcwd( dest_path, sizeof( dest_path ) - 2 );
    (void) strcpy( to_path, dest_path );
  } else {
    strcpy(dest_path, dir_path);
  }


  if( chdir( current_path ) )
  {
    ERROR_MSG( "Chdir failed; Can't resume" );
    return( -1 );
  }

  n = strlen( tree->name );
  if( !strcmp(tree->name, FILE_SEPARATOR_STRING) || 
      (!strncmp( tree->name, dest_path, n )     &&
        ( dest_path[n] == FILE_SEPARATOR_CHAR || dest_path[n] == '\0' ) ) )
  {
    /* Pfad befindet sich im (Sub)-Tree */
    /*----------------------------------*/

    de_ptr = tree;
    token = strtok_r( &dest_path[n], FILE_SEPARATOR_STRING, &old );
    while( token )
    {
      for( sde_ptr = de_ptr->sub_tree; sde_ptr; sde_ptr = sde_ptr->next )
      {
        if( !strcmp( sde_ptr->name, token ) )
	{
	  /* Subtree gefunden */
	  /*------------------*/

	  de_ptr = sde_ptr;
	  break;
	}
      }
      if( sde_ptr == NULL )
      {
#ifdef DEBUG
	(void) sprintf( message, "Can't find directory; token=%s", token );
	ERROR_MSG( message );
#endif
	return( -3 );
      }
      token = strtok_r( NULL, FILE_SEPARATOR_STRING, &old );
    }
    *dir_entry = de_ptr;
  }
  return( 0 );
}




int GetFileEntry(DirEntry *de_ptr, char *file_name, FileEntry **file_entry)
{
  FileEntry *fe_ptr;

  *file_entry = NULL;

  for( fe_ptr = de_ptr->file; fe_ptr; fe_ptr = fe_ptr->next )
  {
    if( !strcmp( fe_ptr->name, file_name ) )
    {
      /* Eintrag gefunden */
      /*------------------*/

      *file_entry = fe_ptr;
      break;
    }
  }
  return( 0 );
}