/***************************************************************************
 *
 * archive.c
 * Allg. Funktionen zum Bearbeiten von Archiven
 *
 ***************************************************************************/


#include "ytree.h"


#ifdef HAVE_LIBARCHIVE
/*
 * Uses libarchive to find a specific entry within an archive and write its
 * contents to the provided output file descriptor.
 * Returns 0 on success, -1 on failure.
 */
int ExtractArchiveEntry(const char *archive_path, const char *entry_path, int out_fd)
{
    struct archive *a;
    struct archive_entry *entry;
    int r;
    const void *buff;
    size_t size;
    la_int64_t offset;

    a = archive_read_new();
    if (a == NULL) {
        return -1;
    }
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);

    r = archive_read_open_filename(a, archive_path, 10240);
    if (r != ARCHIVE_OK) {
        archive_read_free(a);
        return -1;
    }

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        if (strcmp(archive_entry_pathname(entry), entry_path) == 0) {
            /* Found the entry, now write its data to the fd */
            while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK) {
                if (write(out_fd, buff, size) != (ssize_t)size) {
                    /* Write error */
                    archive_read_free(a);
                    return -1;
                }
            }
            archive_read_free(a);
            return (r == ARCHIVE_EOF) ? 0 : -1; /* Return success only on clean EOF */
        }
    }

    /* Entry not found or other error */
    archive_read_free(a);
    return -1;
}
#endif /* HAVE_LIBARCHIVE */


static int GetArchiveDirEntry(DirEntry *tree, char *path, DirEntry **dir_entry);


static int InsertArchiveDirEntry(DirEntry *tree, char *path, struct stat *stat)
{
  DirEntry *df_ptr, *de_ptr, *ds_ptr;
  char father_path[PATH_LENGTH + 1];
  char *p;
  char name[PATH_LENGTH + 1];


#ifdef DEBUG
  fprintf( stderr, "Insert Dir \"%s\"\n", path );
#endif

  if (strlen(path) >= PATH_LENGTH) {
      ERROR_MSG("Archive path too long*skipping directory insert");
      return -1;
  }

  /* Format: .../dir/ */
  /*------------------*/

  (void) strcpy( father_path, path );

  if( ( p = strrchr( father_path, FILE_SEPARATOR_CHAR ) ) ) *p = '\0';
  else
  {
    (void) sprintf( message, "path mismatch*missing '%c' in*%s",
	            FILE_SEPARATOR_CHAR,
	            path
	          );
    ERROR_MSG( message );
    return( -1 );
  }

  p = strrchr( father_path, FILE_SEPARATOR_CHAR );

  if( p == NULL )
  {
    df_ptr = tree;
    if( !strcmp( path, FILE_SEPARATOR_STRING ) )
      (void) strcpy( name, path );
    else
      (void) strcpy( name, father_path );
  }
  else
  {
    (void) strcpy( name, ++p );
    *p = '\0';
    if( GetArchiveDirEntry( tree, father_path, &df_ptr ) )
    {
      (void) sprintf( message, "can't find subdir*%s", father_path );
      ERROR_MSG( message );
      return( -1 );
    }
  }

  if( ( de_ptr = (DirEntry *) malloc( sizeof( DirEntry ) + strlen( name ) ) ) == NULL )
  {
    ERROR_MSG( "Malloc failed*ABORT" );
    exit( 1 );
  }

  (void) memset( (char *) de_ptr, 0, sizeof( DirEntry ) );
  (void) strcpy( de_ptr->name, name );
  (void) memcpy( (char *) &de_ptr->stat_struct, (char *) stat, sizeof( struct stat ) );

#ifdef DEBUG
  fprintf( stderr, "new dir: \"%s\"\n", name );
#endif



  /* Directory einklinken */
  /*----------------------*/

  if( p == NULL )
  {
    /* in tree (=df_ptr) einklinken */
    /*------------------------------*/

    de_ptr->up_tree = df_ptr->up_tree;

    for( ds_ptr = df_ptr; ds_ptr; ds_ptr = ds_ptr->next )
    {
      if( strcmp( ds_ptr->name, de_ptr->name ) > 0 )
      {
        /* ds-Element ist groesser */
        /*-------------------------*/

        de_ptr->next = ds_ptr;
        de_ptr->prev = ds_ptr->prev;
        if( ds_ptr->prev) ds_ptr->prev->next = de_ptr;
        ds_ptr->prev = de_ptr;
	if( de_ptr->up_tree && de_ptr->up_tree->sub_tree == de_ptr->next )
	  de_ptr->up_tree->sub_tree = de_ptr;
        break;
      }

      if( ds_ptr->next == NULL )
      {
        /* Ende der Liste erreicht; ==> einfuegen */
        /*----------------------------------------*/

        de_ptr->prev = ds_ptr;
        de_ptr->next = ds_ptr->next;
        ds_ptr->next = de_ptr;
        break;
      }
    }
  }
  else if( df_ptr->sub_tree == NULL )
  {
    de_ptr->up_tree = df_ptr;
    df_ptr->sub_tree = de_ptr;
  }
  else
  {
    de_ptr->up_tree = df_ptr;

    for( ds_ptr = df_ptr->sub_tree; ds_ptr; ds_ptr = ds_ptr->next )
    {
      if( strcmp( ds_ptr->name, de_ptr->name ) > 0 )
      {
        /* ds-Element ist groesser */
        /*-------------------------*/

        de_ptr->next = ds_ptr;
        de_ptr->prev = ds_ptr->prev;
        if( ds_ptr->prev ) ds_ptr->prev->next = de_ptr;
        ds_ptr->prev = de_ptr;
	if( de_ptr->up_tree->sub_tree == de_ptr->next )
	  de_ptr->up_tree->sub_tree = de_ptr;
        break;
      }

      if( ds_ptr->next == NULL )
      {
        /* Ende der Liste erreicht; ==> einfuegen */
        /*----------------------------------------*/

        de_ptr->prev = ds_ptr;
        de_ptr->next = ds_ptr->next;
        ds_ptr->next = de_ptr;
        break;
      }
    }
  }
  statistic.disk_total_directories++;
  return( 0 );
}





int InsertArchiveFileEntry(DirEntry *tree, char *path, struct stat *stat)
{
  char dir[PATH_LENGTH + 1];
  char file[PATH_LENGTH + 1];
  DirEntry *de_ptr;
  FileEntry *fs_ptr, *fe_ptr;
  struct stat stat_struct;
  int  n;


  if( KeyPressed() )
  {
    Quit();  /* Abfrage, ob ytree verlassen werden soll */
  }

  /* Fnsplit handles path length checks internally now */
  Fnsplit( path, dir, file );

  if( GetArchiveDirEntry( tree, dir, &de_ptr ) )
  {
#ifdef DEBUG
    fprintf( stderr, "can't get directory for file*%s*trying recover", path );
#endif

    (void) memset( (char *) &stat_struct, 0, sizeof( struct stat ) );
    stat_struct.st_mode = S_IFDIR;

    if( TryInsertArchiveDirEntry( tree, dir, &stat_struct ) )
    {
      ERROR_MSG( "inserting directory failed" );
      return( -1 );
    }
    if( GetArchiveDirEntry( tree, dir, &de_ptr ) )
    {
      (void) sprintf( message, "again: can't get directory for file*%s*giving up", path );
      ERROR_MSG( message );
      return( -1 );
    }
  }

  if( S_ISLNK( stat->st_mode ) )
    n = strlen( &path[ strlen( path ) + 1 ] ) + 1;
  else
    n = 0;

  if( ( fe_ptr = (FileEntry *) malloc( sizeof( FileEntry ) + strlen( file ) + n ) ) == NULL )
  {
    ERROR_MSG( "Malloc failed*ABORT" );
    exit( 1 );
  }

  (void) memset( fe_ptr, 0, sizeof( FileEntry ) );
  (void) memcpy( (char *) &fe_ptr->stat_struct, (char *) stat, sizeof( struct stat ) );
  (void) strcpy( fe_ptr->name, file );

  if( S_ISLNK( stat->st_mode ) )
  {
    (void) strcpy( &fe_ptr->name[ strlen( fe_ptr->name ) + 1 ],
		   &path[ strlen( path ) + 1 ]
		 );
  }

  fe_ptr->dir_entry = de_ptr;
  de_ptr->total_files++;
  de_ptr->total_bytes += stat->st_size;
  statistic.disk_total_files++;
  statistic.disk_total_bytes += stat->st_size;

  /* Einklinken */
  /*------------*/

  if( de_ptr->file == NULL )
  {
    de_ptr->file = fe_ptr;
  }
  else
  {
    for( fs_ptr = de_ptr->file; fs_ptr->next; fs_ptr = fs_ptr->next )
      ;

    fe_ptr->prev = fs_ptr;
    fs_ptr->next = fe_ptr;
  }
  return( 0 );
}





static int GetArchiveDirEntry(DirEntry *tree, char *path, DirEntry **dir_entry)
{
  int n;
  DirEntry *de_ptr;
  BOOL is_root = FALSE;

#ifdef DEBUG
  fprintf( stderr, "GetArchiveDirEntry: tree=%s, path=%s\n",
  (tree) ? tree->name : "NULL", path );
#endif

  if( strchr( path, FILE_SEPARATOR_CHAR ) != NULL )
  {
    for( de_ptr = tree; de_ptr; de_ptr = de_ptr->next )
    {
      n = strlen( de_ptr->name );
      if( !strcmp( de_ptr->name, FILE_SEPARATOR_STRING ) ) is_root = TRUE;

      if( n && !strncmp( de_ptr->name, path, n ) &&
	  (is_root || path[n] == '\0' || path[n] == FILE_SEPARATOR_CHAR ) )
      {
	if( ( is_root && path[n] == '\0' ) ||
	    ( path[n] == FILE_SEPARATOR_CHAR && path[n+1] == '\0' ) )
	{
	  /* Pfad abgearbeitet; ==> fertig */
	  /*-------------------------------*/

	  *dir_entry = de_ptr;
	  return( 0 );
	}
	else
        {
	  return( GetArchiveDirEntry( de_ptr->sub_tree,
				  ( is_root ) ? &path[n] : &path[n+1],
				  dir_entry
				) );
	}
      }
    }
  }
  if( *path == '\0' )
  {
    *dir_entry = tree;
    return( 0 );
  }
  return( -1 );
}





int TryInsertArchiveDirEntry(DirEntry *tree, char *dir, struct stat *stat)
{
  DirEntry *de_ptr;
  char dir_path[PATH_LENGTH + 1];
  char *s, *t;

  /* Guard against excessive length that would overflow dir_path */
  if (strlen(dir) >= PATH_LENGTH) {
      /* Skip dangerous paths silently or warn */
      return -1;
  }

  (void) memset( dir_path, 0, sizeof( dir_path ) );

#ifdef DEBUG
  fprintf( stderr, "Try install start \n" );
#endif

  for( s=dir, t=dir_path; *s; s++, t++ )
  {
    if( (*t = *s) == FILE_SEPARATOR_CHAR )
    {
      if( GetArchiveDirEntry( tree, dir_path, &de_ptr ) == -1 )
      {
	/* Evtl. fehlender teil; ==> einfuegen */
	/*-------------------------------------*/

	if( InsertArchiveDirEntry( tree, dir_path, stat ) ) return( -1 );
      }
    }
  }

#ifdef DEBUG
  fprintf( stderr, "Try install end\n" );
#endif

  return( 0 );
}


void MinimizeArchiveTree(DirEntry **tree_ptr)
{
  DirEntry *tree = *tree_ptr;
  DirEntry *de_ptr, *de1_ptr;
  DirEntry *next_ptr;
  FileEntry *fe_ptr;

  /* 1. Collapse Root if empty and has siblings */
  /* If the root (tree) has no files, no subdirectories, but has a 'next' sibling,
     it's an empty placeholder. We can discard it and promote the sibling to be root. */
  if( tree->prev == NULL && tree->next != NULL && tree->file == NULL && tree->sub_tree == NULL )
  {
    DirEntry *new_root = tree->next;

    /* Update global pointer via the double pointer argument */
    *tree_ptr = new_root;

    /* Fix up pointers for the new root */
    new_root->prev = NULL;
    /* It's a root now, so no up_tree */
    new_root->up_tree = NULL;

    /* Update children of the new root (if any) to point to it as up_tree?
       Wait, they already point to it. 'up_tree' is parent.
       The new root is just taking the place of the old root.
       Existing children of 'new_root' point to 'new_root'. That's fine. */

    statistic.disk_total_directories--;
    free(tree);
    tree = new_root; /* Update local variable for subsequent checks */
  }


  /* 2. Collapse empty leaf directories in sub-trees */
  /* Iterate through children of the current root */
  for( de_ptr = tree->sub_tree; de_ptr; de_ptr = next_ptr )
  {
    next_ptr = de_ptr->next; /* Save next because de_ptr might be freed */

    if( de_ptr->prev == NULL && de_ptr->next == NULL && de_ptr->file == NULL )
    {
      /* de_ptr is a single child (no siblings) and has no files.
         We can merge it into the parent (tree).
         Example: /usr/ -> /usr/bin/  becomes /usr/bin/
      */

      /* Concatenate names: parent/child */
      if( strcmp( tree->name, FILE_SEPARATOR_STRING ) )
	    (void) strcat( tree->name, FILE_SEPARATOR_STRING );
      (void) strcat( tree->name, de_ptr->name );

      statistic.disk_total_directories--;

      /* Move de_ptr's children to be tree's children */
      tree->sub_tree = de_ptr->sub_tree;

      /* Update up_tree pointers for all moved children */
      for( de1_ptr = de_ptr->sub_tree; de1_ptr; de1_ptr = de1_ptr->next )
	    de1_ptr->up_tree = tree;

      /* Update stats */
      /* (tree stats should already encompass de_ptr stats if any, but here de_ptr has no files) */

      free( de_ptr );
      /* Continue loop with the *new* first child (which was de_ptr->sub_tree)
         Wait, the loop variable was de_ptr. We effectively replaced de_ptr with its children.
         But the loop iterates over siblings of de_ptr. de_ptr had no siblings (check above).
         So next_ptr is NULL. Loop terminates. Correct. */

      /* Since we modified the structure, we should restart the scan or check the new children?
         The original code only checked the immediate level. We keep it simple. */
      continue;
    }
    break; /* If we hit a non-collapsible entry, stop attempting to collapse this path */
  }

  /* 3. Collapse root into its single child if applicable */
  /* If tree has no files, no siblings, and exactly one child (sub_tree) which has no siblings */
  if( tree->prev == NULL &&
      tree->next == NULL &&
      tree->file == NULL &&
      tree->sub_tree     &&
      tree->sub_tree->prev == NULL &&
      tree->sub_tree->next == NULL
    )
  {
    de_ptr = tree->sub_tree;

    /* Merge names */
    if( strcmp( tree->name, FILE_SEPARATOR_STRING ) )
        (void) strcat( tree->name, FILE_SEPARATOR_STRING );
    (void) strcat( tree->name, de_ptr->name );

    /* Move files up */
    tree->file = de_ptr->file;
    for( fe_ptr=tree->file; fe_ptr; fe_ptr=fe_ptr->next )
      fe_ptr->dir_entry = tree;

    /* Copy stats */
    (void) memcpy( (char *) &tree->stat_struct,
		   (char *) &de_ptr->stat_struct,
		   sizeof( struct stat )
		  );

    statistic.disk_total_directories--;

    /* Move grandchildren up */
    tree->sub_tree = de_ptr->sub_tree;
    for( de1_ptr = de_ptr->sub_tree; de1_ptr; de1_ptr = de1_ptr->next )
      de1_ptr->up_tree = tree;

    free( de_ptr );
  }
  return;
}