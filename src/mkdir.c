/***************************************************************************
 *
 * mkdir.c
 * Creating directories
 *
 ***************************************************************************/


#include "ytree.h"




int MakeDirectory(DirEntry *father_dir_entry)
{
  char dir_name[PATH_LENGTH * 2 +1];
  int result = -1;

  if( mode != DISK_MODE && mode != USER_MODE )
  {
    return( result );
  }

  ClearHelp();

  MvAddStr( LINES - 2, 1, "MAKE DIRECTORY: " );

  *dir_name = '\0';

  if( InputString( dir_name, LINES - 2, 20, 0, COLS - 20 - 1, "\r\033" ) == CR )
  {
    if (MakeDirEntry( father_dir_entry, dir_name ) != NULL)
    {
      result = 0;
    }
  }

  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}




DirEntry *MakeDirEntry(DirEntry *father_dir_entry, char *dir_name )
{
  DirEntry *den_ptr = NULL, *des_ptr;
  char buffer[PATH_LENGTH+1];
  struct stat stat_struct;

  if( mode != DISK_MODE && mode != USER_MODE )
  {
    return( NULL );
  }

  /* Pre-check: Does a directory with this name (case-insensitive) already exist in the tree? */
  for (des_ptr = father_dir_entry->sub_tree; des_ptr; des_ptr = des_ptr->next) {
      if (strcasecmp(des_ptr->name, dir_name) == 0) {
          /* Found it! Return the existing node */
          return des_ptr;
      }
  }

  (void) GetPath( father_dir_entry, buffer );
  (void) strcat( buffer, FILE_SEPARATOR_STRING );
  (void) strcat( buffer, dir_name );

  if( mkdir( buffer, (S_IREAD  |
		                 S_IWRITE |
		                 S_IEXEC  |
		                 S_IRGRP  |
		                 S_IWGRP  |
		                 S_IXGRP  |
		                 S_IROTH  |
		                 S_IWOTH  |
		                 S_IXOTH) & ~user_umask
    ) )
  {
    /* Modified Logic: Allow existing directories if they are valid. */
    if (errno == EEXIST) {
        if (STAT_(buffer, &stat_struct) == 0 && S_ISDIR(stat_struct.st_mode)) {
            /* It exists and is a directory. Fall through to creation logic. */
            goto CREATE_NODE;
        }
    }

    (void) snprintf( message, MESSAGE_LENGTH, "Can't create Directory*\"%s\"*%s",
		    buffer, strerror(errno)
		  );
    MESSAGE( message );
    return NULL;
  }
  else
  {
CREATE_NODE:
    /* Directory erstellt
     * ==> einklinken im Baum
     */

    /* FIX: Used calloc to ensure all fields (especially tagged_flag) are zeroed */
    /* FIX: Added +1 to allocation for null terminator */
    if( ( den_ptr = (DirEntry *) calloc( 1, sizeof( DirEntry ) + strlen( dir_name ) + 1 ) ) == NULL )
    {
      ERROR_MSG( "Malloc Failed*ABORT" );
      exit( 1 );
    }

    den_ptr->file      = NULL;
    den_ptr->next      = NULL;
    den_ptr->prev      = NULL;
    den_ptr->sub_tree  = NULL;
    den_ptr->total_bytes    = 0L;
    den_ptr->matching_bytes = 0L;
    den_ptr->tagged_bytes   = 0L;
    den_ptr->total_files    = 0;
    den_ptr->matching_files = 0;
    den_ptr->tagged_files   = 0;
    den_ptr->access_denied  = FALSE;
    den_ptr->cursor_pos     = 0;
    den_ptr->start_file     = 0;
    den_ptr->global_flag    = FALSE;
    den_ptr->login_flag     = FALSE;
    den_ptr->big_window     = FALSE;
    den_ptr->up_tree = father_dir_entry;
    den_ptr->not_scanned    = FALSE;

    statistic.disk_total_directories++;

    (void) strcpy( den_ptr->name, dir_name );

    if( STAT_( buffer, &stat_struct ) )
    {
      ERROR_MSG( "Stat Failed*ABORT" );
      exit( 1 );
    }

    (void) memcpy( &den_ptr->stat_struct,
		   &stat_struct,
		   sizeof( stat_struct )
		 );


    /* Sortieren durch direktes Einfuegen */
    /*------------------------------------*/

    for( des_ptr = father_dir_entry->sub_tree; des_ptr; des_ptr = des_ptr->next )
    {
      if( strcmp( des_ptr->name, den_ptr->name ) > 0 )
      {
	/* des-Element ist groesser */
	/*--------------------------*/

	den_ptr->next = des_ptr;
	den_ptr->prev = des_ptr->prev;
	if( des_ptr->prev) des_ptr->prev->next = den_ptr;
	else father_dir_entry->sub_tree = den_ptr;
	des_ptr->prev = den_ptr;
	break;
      }

      if( des_ptr->next == NULL )
      {
        /* Ende der Liste erreicht; ==> einfuegen */
        /*----------------------------------------*/

        den_ptr->prev = des_ptr;
	den_ptr->next = des_ptr->next;
        des_ptr->next = den_ptr;
	break;
      }
    }

    if( father_dir_entry->sub_tree == NULL )
    {
      /* Erstes Element */
      /*----------------*/

      father_dir_entry->sub_tree = den_ptr;
      den_ptr->prev = NULL;
      den_ptr->next = NULL;
    }

    (void) GetAvailBytes( &statistic.disk_space );
  }

  return( den_ptr );
}



int MakePath( DirEntry *tree, char *dir_path, DirEntry **dest_dir_entry )
{
  DirEntry *de_ptr, *sde_ptr;
  char     path[PATH_LENGTH+1];
  char     *cptr;
  char     *token, *old;
  int      n;
  int      result = -1;
  char     *search_start;

  NormPath( dir_path, path );
  *dest_dir_entry = NULL;

  n = strlen( tree->name );
  /*
   * Check if path matches tree root.
   * Special handling for root "/" to ensure "path inside tree" logic works correctly.
   */
  if (strcmp(tree->name, FILE_SEPARATOR_STRING) == 0) {
      /* Tree is "/". Path must start with "/". */
      if (path[0] == FILE_SEPARATOR_CHAR) {
          de_ptr = tree;
          /* Tokenize starting from char 1 to skip leading slash */
          search_start = &path[1];
          goto SEARCH_TREE;
      }
  } else if ( !strncmp( tree->name, path, n ) &&
       ( path[n] == FILE_SEPARATOR_CHAR || path[n] == '\0' ) ) {
      /* Normal case: Path starts with tree root prefix */
      de_ptr = tree;
      search_start = &path[n];
      goto SEARCH_TREE;
  }

  /* Fallback: Destination not in current memory tree */
  goto CREATE_EXTERNAL;

SEARCH_TREE:
    /* Pfad befindet sich im (Sub)-Tree */
    /*----------------------------------*/

    token = strtok_r( search_start, FILE_SEPARATOR_STRING, &old );
    while( token )
    {
      for( sde_ptr = de_ptr->sub_tree; sde_ptr; sde_ptr = sde_ptr->next )
      {
        /* FIX: Case-insensitive search for existing directories */
        if( !strcasecmp( sde_ptr->name, token ) )
	{
	  /* Subtree gefunden */
	  /*------------------*/

	  de_ptr = sde_ptr;
	  break;
	}
      }
      if( sde_ptr == NULL )
      {
	/* Folgeverzeichnis nicht vorhanden */
	/*----------------------------------*/
    /* MakeDirEntry returns the new node (or existing one), or NULL on error */
	if( ( de_ptr = MakeDirEntry( de_ptr, token ) ) == NULL )
	{
	  return( result );
	}
      }
      token = strtok_r( NULL, FILE_SEPARATOR_STRING, &old );
    }
    *dest_dir_entry = de_ptr;
    result = 0;
    return result;

CREATE_EXTERNAL:
    /* Zielverzeichnis ist nicht im Subtree */
    /*--------------------------------------*/

    (void) strcat( path, FILE_SEPARATOR_STRING );

    for( cptr = strchr( path, FILE_SEPARATOR_CHAR );
         cptr;
         cptr = strchr( cptr + 1, FILE_SEPARATOR_CHAR )
       )
    {
      if( cptr == path ) continue;
      if( cptr[-1] == FILE_SEPARATOR_CHAR ) continue;
      if( cptr[-1] == '.' && (cptr == path+1 || cptr[-2] == FILE_SEPARATOR_CHAR ) ) continue;

      *cptr = '\0';

      if( mkdir( path, S_IREAD  |
		       S_IWRITE |
		       S_IEXEC  |
		       S_IRGRP  |
		       S_IWGRP  |
		       S_IXGRP  |
		       S_IROTH  |
		       S_IWOTH  |
		       S_IXOTH ) )
      {
        /* ging nicht... */
        /*---------------*/

        *cptr = FILE_SEPARATOR_CHAR;
        if( errno == EEXIST ) continue; /* OK, weitermachen */
        break;
      }
      *cptr = FILE_SEPARATOR_CHAR;
    }
    result = 0;

  return( result );
}

int EnsureDirectoryExists(char *dir_path, DirEntry *tree, BOOL *created, DirEntry **result_ptr)
{
  DIR *tmpdir;
  int term;

  if (created) *created = FALSE;
  if (result_ptr) *result_ptr = NULL;

  /* Check if directory exists on disk */
  if ((tmpdir = opendir(dir_path)) == NULL)
  {
    /* If it doesn't exist, ask the user */
    if (errno == ENOENT)
    {
      if ((term = InputChoice("Directory does not exist; create (y/N) ? ", "YN\033")) == 'Y')
      {
         /* Proceed to create */
         if (created) *created = TRUE;
      }
      else
      {
        /* User said NO or Escaped */
        return -1;
      }
    }
    else
    {
       /* Some other error opening directory (e.g. permission) */
       (void) snprintf(message, MESSAGE_LENGTH, "Error opening directory*\"%s\"*%s", dir_path, strerror(errno));
       MESSAGE(message);
       return -1;
    }
  }
  else
  {
     /* Exists on disk */
     closedir(tmpdir);
  }

  /*
   * Directory exists (or user wants to create it).
   * Call MakePath to resolve the DirEntry pointer.
   * MakePath will create the node in memory if it's missing (even if dir exists on disk),
   * thanks to the update in MakeDirEntry.
   */
  return MakePath(tree, dir_path, result_ptr);
}