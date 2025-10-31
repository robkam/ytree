/***************************************************************************
 *
 * Allg. Funktionen zum Bearbeiten von Archiven
 *
 ***************************************************************************/


#include "ytree.h"



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






void MinimizeArchiveTree(DirEntry *tree)
{
  DirEntry  *de_ptr, *de1_ptr;
  DirEntry  *next_ptr;
  FileEntry *fe_ptr;


  /* Falls tree einen Nachfolger hat und
   * tree selbst leer ist, wird tree gestrichen 
   */

  if( tree->prev == NULL &&
      tree->next != NULL &&
      tree->file == NULL )
  {
    next_ptr = tree->next;
    (void) memcpy( (char *) tree, 
		   (char *) tree->next, 
		   sizeof( DirEntry ) + strlen( tree->next->name ) 
		 );
    tree->prev = NULL;
    if( tree->next ) tree->next->prev = tree;
    statistic.disk_total_directories--; 
    free( next_ptr );
    for( fe_ptr=tree->file; fe_ptr; fe_ptr=fe_ptr->next)
      fe_ptr->dir_entry = tree;
    for( de_ptr=tree->sub_tree; de_ptr; de_ptr=de_ptr->next)
      de_ptr->up_tree = tree;
  }


  /* Test, ob *de_ptr weder Vorgaenger noch Nachfolger noch Dateien hat */
  /*--------------------------------------------------------------------*/

  for( de_ptr = tree->sub_tree; de_ptr; de_ptr = next_ptr )
  {
    if( de_ptr->prev == NULL && de_ptr->next == NULL && de_ptr->file == NULL )
    {
      /* Zusammenfassung moeglich */
      /*--------------------------*/

      if( strcmp( tree->name, FILE_SEPARATOR_STRING ) ) 
	(void) strcat( tree->name, FILE_SEPARATOR_STRING );
      (void) strcat( tree->name, de_ptr->name );
      statistic.disk_total_directories--;
      tree->sub_tree = de_ptr->sub_tree;
      for( de1_ptr = de_ptr->sub_tree; de1_ptr; de1_ptr = de1_ptr->next )
	de1_ptr->up_tree = tree;
      next_ptr = de_ptr->sub_tree;
      free( de_ptr );
#ifdef DEBUG
  fprintf( stderr, "new root-dir: \"%s\"\n", tree->name );
#endif
      continue;
    }
    break;
  }

  /* Letzter Optimierungsschritt:
   * Falls tree weder Vorgaenger noch Nachfolger hat, aber
   * einen Subtree der Files hat, wird zusammengefasst
   */

  if( tree->prev == NULL && 
      tree->next == NULL &&
      tree->file == NULL &&
      tree->sub_tree     &&
      tree->sub_tree->prev == NULL &&
      tree->sub_tree->next == NULL
    )
  {
    de_ptr = tree->sub_tree;
    (void) strcat( tree->name, FILE_SEPARATOR_STRING );
    (void) strcat( tree->name, de_ptr->name );
    tree->file = de_ptr->file;
    for( fe_ptr=tree->file; fe_ptr; fe_ptr=fe_ptr->next )
      fe_ptr->dir_entry = tree;
    (void) memcpy( (char *) &tree->stat_struct, 
		   (char *) &de_ptr->stat_struct, 
		   sizeof( struct stat ) 
		  );
    statistic.disk_total_directories--;
    tree->sub_tree = de_ptr->sub_tree;
    for( de1_ptr = de_ptr->sub_tree; de1_ptr; de1_ptr = de1_ptr->next )
      de1_ptr->up_tree = tree;
    free( de_ptr );
  }
  return;
}



void MakeExtractCommandLine(char *command_line, char *path, char *file, char *cmd)
{
  int  l;
  char cat_path[PATH_LENGTH+1];
  int  compress_method;

  compress_method = GetFileMethod( path );

  l = strlen( path );

  if( compress_method == ZOO_COMPRESS )
  {
    /* zoo xp FILE ?? */
    /*----------------*/

    (void) sprintf( command_line, "%s '%s' '%s' %s", 
		    ZOOEXPAND,
		    path,
		    file,
		    cmd
		  );
  }
  else if( compress_method == LHA_COMPRESS )
  {
    /* xlharc p FILE ?? */
    /*------------------*/

    (void) sprintf( command_line, "%s '%s' '%s' %s", 
		    LHAEXPAND,
		    path,
		    file,
		    cmd
		  );
  }
  else if( compress_method == ZIP_COMPRESS )
  {
    /* unzip -c FILE ?? */
    /*------------------*/

    (void) sprintf( command_line, "%s '%s' '%s' %s", 
		    ZIPEXPAND,
		    path,
		    file,
		    cmd
		  );
  }
  else if( compress_method == SEVENZIP_COMPRESS )
  {
    /* 7z x FILE ?? */
    /*--------------*/

    (void) sprintf( command_line, "%s '%s' '%s' %s", 
		    SEVENZIPEXPAND,
		    path,
		    file,
		    cmd
		  );
  }
  else if( compress_method == ISO_COMPRESS )
  {
    /* 7z x FILE ?? */
    /*--------------*/

    (void) sprintf( command_line, "%s '%s' '%s' %s", 
		    ISOEXPAND,
		    path,
		    file,
		    cmd
		  );
  }
  else if( compress_method == ARC_COMPRESS )
  {
    /* arc p FILE ?? */
    /*---------------*/

    (void) sprintf( command_line, "%s '%s' '%s' %s", 
		    ARCEXPAND,
		    path,
		    file,
		    cmd
		  );
  }
  else if( compress_method == RPM_COMPRESS )
  {
    /* TF=/tmp/ytree.$$; mkdir $TF; cd $TF; rpm2cpio RPM_FILE | cpio -id FILE; 
     * cat $TF/$2; cd /tmp; rm -rf $TF; exit 0
     */
    

    if(!strcmp(RPMEXPAND, "builtin")) {
      (void) sprintf( command_line, 
    		    "(TF=/tmp/ytree.$$; mkdir $TF; rpm2cpio '%s' | (cd $TF; cpio --no-absolute-filenames -i -d '%s'); cat \"$TF/%s\"; cd /tmp; rm -rf $TF; exit 0) %s",
		    path,
		    (*file == FILE_SEPARATOR_CHAR) ? &file[1] : file,
		    file,
		    cmd
		  );
    } else {
      (void) sprintf( command_line, "%s '%s' '%s' %s", 
		    RPMEXPAND,
		    path,
		    file,
		    cmd
		  );
    }
  }
  else if( compress_method == RAR_COMPRESS )
  {
    /* rar p FILE ?? */
    /*---------------*/

    (void) sprintf( command_line, "%s '%s' '%s' %s", 
		    RAREXPAND,
		    path,
		    file,
		    cmd
		  );
  }
  else if( compress_method == FREEZE_COMPRESS )
  {
    /* melt < TAR_FILE | gtar xOf - FILE ?? */
    /*--------------------------------------*/

    (void) sprintf( command_line, "%s < '%s' | %s '%s' %s", 
		    MELT, 
		    path,
		    TAREXPAND,
		    file,
		    cmd
		  );
  }
  else if( compress_method == MULTIPLE_FREEZE_COMPRESS )
  {
    /* CAT TAR_FILEs | melt | gtar xOf - FILE ?? */
    /*-------------------------------------------*/

    (void) strncpy( cat_path, path, l - 2 );
    (void) strcpy( &cat_path[l-2], "*" );

    (void) sprintf( command_line, "%s %s | %s | %s '%s' %s", 
		    CAT,
		    cat_path,
		    MELT, 
		    TAREXPAND,
		    file,
		    cmd
		  );
  }
  else if( compress_method == COMPRESS_COMPRESS )
  {
    /* uncompress < TAR_FILE | gtar xOf - FILE ?? */
    /*--------------------------------------------*/

    (void) sprintf( command_line, "%s < %s | %s '%s' %s",
		    UNCOMPRESS, 
		    path,
		    TAREXPAND,
		    file,
		    cmd
		  );
  }
  else if( compress_method == MULTIPLE_COMPRESS_COMPRESS )
  {
    /* CAT TAR_FILEs | uncompress | gtar xOf - FILE ?? */
    /*-------------------------------------------------*/

    (void) strncpy( cat_path, path, l - 2 );
    (void) strcpy( &cat_path[l-2], "*" );

    (void) sprintf( command_line, "%s %s | %s | %s '%s' %s", 
		    CAT,
		    cat_path,
		    UNCOMPRESS, 
		    TAREXPAND,
		    file,
		    cmd
		  );
  }
  else if( compress_method == GZIP_COMPRESS )
  {
    /* gunzip < TAR_FILE | gtar xOf - FILE ?? */
    /*----------------------------------------*/

    (void) sprintf( command_line, "%s < '%s' | %s '%s' %s",
		    GNUUNZIP, 
		    path,
		    TAREXPAND,
		    file,
		    cmd
		  );
  }
  else if( compress_method == MULTIPLE_GZIP_COMPRESS )
  {
    /* CAT TAR_FILEs | gunzip | gtar xOf - FILE ?? */
    /*---------------------------------------------*/

    (void) strncpy( cat_path, path, l - 2 );
    (void) strcpy( &cat_path[l-2], "*" );

    (void) sprintf( command_line, "%s %s | %s | %s '%s' %s", 
		    CAT,
		    cat_path,
		    GNUUNZIP, 
		    TAREXPAND,
		    file,
		    cmd
		  );
  }
  else if( compress_method == BZIP_COMPRESS )
  {
    /* bunzip2 < TAR_FILE | gtar xOf - FILE ?? */
    /*----------------------------------------*/

    (void) sprintf( command_line, "%s < '%s' | %s '%s' %s",
		    BUNZIP, 
		    path,
		    TAREXPAND,
		    file,
		    cmd
		  );
  }
  else if( compress_method == ZSTD_COMPRESS )
  {
    /* zstdcat < TAR_FILE | gtar xOf - FILE ?? */
    /*----------------------------------------*/

    (void) sprintf( command_line, "%s < '%s' | %s '%s' %s",
		    ZSTDCAT, 
		    path,
		    TAREXPAND,
		    file,
		    cmd
		  );
  }
  else if( compress_method == LZIP_COMPRESS )
  {
    /* lzip -cd < TAR_FILE | gtar xOf - FILE ?? */
    /*----------------------------------------*/

    (void) sprintf( command_line, "%s < '%s' | %s '%s' %s",
		    LUNZIP, 
		    path,
		    TAREXPAND,
		    file,
		    cmd
		  );
  }
  else
  {
    /* gtar xOf - FILE < TAR_FILE ?? */
    /*-------------------------------*/

    (void) sprintf( command_line, "%s '%s' < '%s' %s",
		    TAREXPAND,
		    file, 
		    path,
		    cmd
		  );
  }

#ifdef DEBUG
  fprintf( stderr, "system( \"%s\" )\n", command_line );
#endif

}

