/***************************************************************************
 *
 * Erstellen von Verzeichnissen
 *
 ***************************************************************************/


#include "ytree.h"




int MakeDirectory(DirEntry *father_dir_entry)
{
  char dir_name[PATH_LENGTH * 2 +1];
  int result = -1;

  if( mode != DISK_MODE && mode != USER_MODE )
  {
    beep();
    return( result );
  }

  ClearHelp();

  MvAddStr( LINES - 2, 1, "Make Subdirectory: " );

  *dir_name = '\0';

  if( InputString( dir_name, LINES - 2, 20, 0, COLS - 20 - 1, "\r\033" ) == CR )
  {
    result = MakeDirEntry( father_dir_entry, dir_name );
  }
 
  move( LINES - 2, 1 ); clrtoeol();

  return( result );
}




int MakeDirEntry(DirEntry *father_dir_entry, char *dir_name )
{
  DirEntry *den_ptr, *des_ptr;
  char buffer[PATH_LENGTH+1];
  struct stat stat_struct;
  int result = -1;

  if( mode != DISK_MODE && mode != USER_MODE )
  {
    beep();
    return( result );
  }

  (void) GetPath( father_dir_entry, buffer );
  (void) strcat( buffer, FILE_SEPARATOR_STRING );
  (void) strcat( buffer, dir_name );

  if( ( result = mkdir( buffer, (S_IREAD  |
		                 S_IWRITE |
		                 S_IEXEC  |
		                 S_IRGRP  |
		                 S_IWGRP  |
		                 S_IXGRP  |
		                 S_IROTH  |
		                 S_IWOTH  |
		                 S_IXOTH) & ~user_umask
    ) ) )
  {
    (void) sprintf( message, "Can't create Directory*\"%s\"*%s", 
		    buffer, strerror(errno)
		  );
    MESSAGE( message );
  }
  else
  {
    /* Directory erstellt 
     * ==> einklinken im Baum 
     */
      
    if( ( den_ptr = (DirEntry *) malloc( sizeof( DirEntry ) + strlen( dir_name )  ) ) == NULL )
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

    result = 0;
  }

  return( result );
}



int MakePath( DirEntry *tree, char *dir_path, DirEntry **dest_dir_entry )
{
  DirEntry *de_ptr, *sde_ptr;
  char     path[PATH_LENGTH+1];
  char     *cptr;
  char     *token, *old;
  int      n;
  int      result = -1;

  NormPath( dir_path, path );
  *dest_dir_entry = NULL;

  n = strlen( tree->name );
  if( !strcmp(tree->name, FILE_SEPARATOR_STRING) ||
      ( !strncmp( tree->name, path, n ) &&
       ( path[n] == FILE_SEPARATOR_CHAR || path[n] == '\0' ) ) )
  {
    /* Pfad befindet sich im (Sub)-Tree */
    /*----------------------------------*/

    de_ptr = tree;
    token = Strtok_r( &path[n], FILE_SEPARATOR_STRING, &old );
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
	/* Folgeverzeichnis nicht vorhanden */
	/*----------------------------------*/

#ifdef DEBUG
  fprintf( stderr, "MakeDirEntry: \"%s\"\n", token );
#endif /* DEBUG */

	if( MakeDirEntry( de_ptr, token ) )
	{
	  return( result );
	}
	continue;
      }
      token = Strtok_r( NULL, FILE_SEPARATOR_STRING, &old );
    }
    *dest_dir_entry = de_ptr;
    result = 0;
  }
  else
  {
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
  
#ifdef DEBUG
    fprintf( stderr, "MakePath: \"%s\"\n", path );
#endif /* DEBUG */
  
  
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
  }

  return( result );
}



