/***************************************************************************
 *
 * readtree.c
 * Functions for reading the directory tree
 *
 ***************************************************************************/


#include "ytree.h"



static void UnReadSubTree(DirEntry *dir_entry);



/* Dateibaum lesen: path = "Root"-Pfad
 * dir_entry wird von der Funktion gefuellt
 */


int ReadTree(DirEntry *dir_entry, char *path, int depth)
{
  DIR           *dir;
  struct stat   stat_struct;
  struct dirent *dirent;
  char          new_path[PATH_LENGTH + 1];
  DirEntry      first_dir_entry;
  DirEntry      *des_ptr;
  DirEntry      *den_ptr;
  FileEntry     first_file_entry;
  FileEntry     *fes_ptr;
  FileEntry     *fen_ptr;
  int		file_count;


  /* Safety: If this node already has children/files (e.g. from ScanSubTree),
   * free them before overwriting to prevent memory leaks.
   */
  if (dir_entry->file) {
      FileEntry *f, *n;
      for (f = dir_entry->file; f; f = n) {
          n = f->next;
          RemoveFile(f); /* Updates stats and frees memory */
      }
      dir_entry->file = NULL;
  }
  if (dir_entry->sub_tree) {
      /* Use UnReadSubTree to recursively free children and decrement stats correctly */
      UnReadSubTree(dir_entry->sub_tree);
      dir_entry->sub_tree = NULL;
  }


  /* dir_entry initialisieren */
  /*--------------------------*/

  dir_entry->file           = NULL;
/*
  dir_entry->next           = NULL;
  dir_entry->prev           = NULL;
*/
  dir_entry->sub_tree       = NULL;
  dir_entry->total_bytes    = 0L;
  dir_entry->matching_bytes = 0L;
  dir_entry->tagged_bytes   = 0L;
  dir_entry->total_files    = 0;
  dir_entry->matching_files = 0;
  dir_entry->tagged_files   = 0;
  dir_entry->access_denied  = FALSE;
  dir_entry->start_file     = 0;
  dir_entry->cursor_pos     = 0;
  dir_entry->global_flag    = FALSE;
  dir_entry->login_flag     = FALSE;
  dir_entry->big_window     = FALSE;
  dir_entry->not_scanned    = FALSE;

  if( S_ISBLK( dir_entry->stat_struct.st_mode ) )
    return( 0 ); /* Block-Device */

  if( depth < 0 )
  {
    if( dir_entry->up_tree )
    {
      dir_entry->up_tree->not_scanned = TRUE;
      return( 1 );
    }
  }

  statistic.disk_total_directories++;

  /* Activity Spinner for visual feedback */
  DrawSpinner();

  /* Silently skip unreadable or missing directories */
  if ((dir = opendir(path)) == NULL) {
      dir_entry->access_denied = TRUE;
      return 0;
  }


  first_dir_entry.prev  = NULL;
  first_dir_entry.next  = NULL;
  *first_dir_entry.name = '\0';
  first_file_entry.next = NULL;
  fes_ptr               = &first_file_entry;

  file_count = 0;
  while( ( dirent = readdir( dir ) ) != NULL )
  {
    if( !strcmp( dirent->d_name, "." ) || !strcmp( dirent->d_name, ".." ) )
      continue;

    /* Removed hide_dot_files check here.
       We load ALL files/dirs into memory now, and filter only at display time. */

    if( EscapeKeyPressed() )
    {
      /* Abfrage ob ytree verlassen werden soll */
      int choice = InputChoice("Abort scan (Y/N)?", "YyNn\033"); /* \033 is ESC */
      if (choice == 'Y' || choice == ESC) {
          closedir(dir);
          /* CRITICAL - Attach Partial Results */
          if( first_file_entry.next ) first_file_entry.next->prev = NULL;
          if( first_dir_entry.next )  first_dir_entry.next->prev = NULL;
          dir_entry->file = first_file_entry.next;
          dir_entry->sub_tree = first_dir_entry.next;
          return -1;
      } else {
          /* Clear the prompt line to indicate resumption */
          int y, x;
          getyx(stdscr, y, x); /* Save cursor */
          move(LINES - 2, 0);  /* Move to prompt line (standard position) */
          clrtoeol();          /* Clear it */
          move(y, x);          /* Restore cursor */
          refresh();
      }
    }

    /* Update statistics / animation every 20 files to be smoother */
    if( ( file_count++ % 20 ) == 0 ) {
      /* Update Spinner regardless of animation mode */
      DrawSpinner();

      if (animation_method == 1) {
          DrawAnimationStep(file_window);
          doupdate();
      } else {
          if ((file_count % 100) == 0) { /* Don't flicker stats too fast */
              DisplayDiskStatistic();
              doupdate();
          }
      }
    }


    (void) strcpy( new_path, path );
    if( strcmp( new_path, FILE_SEPARATOR_STRING ) )
      (void) strcat( new_path, FILE_SEPARATOR_STRING );
    (void) strcat( new_path, dirent->d_name );

    if( STAT_( new_path, &stat_struct ) )
    {
      if( errno == EACCES ) continue;
      (void) snprintf( message, MESSAGE_LENGTH, "Stat failed on*%s*IGNORED", new_path );
      ERROR_MSG( message );
      continue;
    }

    if( S_ISDIR( stat_struct.st_mode ) )
    {
      /* Directory-Entry */
      /*-----------------*/

      /* FIX: Added +1 to allocation for null terminator */
      if( ( den_ptr = (DirEntry *)
		       calloc( 1, sizeof( DirEntry ) + strlen( dirent->d_name ) + 1 )
   	   ) == NULL )
      {
        ERROR_MSG( "Malloc Failed*ABORT" );
        exit( 1 );
      }

      den_ptr->up_tree = dir_entry;

      (void) strcpy( den_ptr->name, dirent->d_name );

      (void) memcpy( &den_ptr->stat_struct,
		     &stat_struct,
		     sizeof( stat_struct )
		    );
      den_ptr->prev = den_ptr->next = NULL;

      /* Recursive call with abort check */
      if (ReadTree( den_ptr, new_path, depth - 1) == -1) {
          /* Fix: Free the partially built subtree before returning */
          DeleteTree(den_ptr); /* Free the allocated DirEntry and its children */
          closedir(dir);
          /* Attach Partial Results */
          if( first_file_entry.next ) first_file_entry.next->prev = NULL;
          if( first_dir_entry.next )  first_dir_entry.next->prev = NULL;
          dir_entry->file = first_file_entry.next;
          dir_entry->sub_tree = first_dir_entry.next;
          return -1;
      }

      /* Sortieren durch direktes Einfuegen */
      /*------------------------------------*/

      for( des_ptr = &first_dir_entry; des_ptr; des_ptr = des_ptr->next )
      {
        if( strcmp( des_ptr->name, den_ptr->name ) > 0 )
        {
	  /* des-Element ist groesser */
	  /*--------------------------*/

	  den_ptr->next = des_ptr;
	  den_ptr->prev = des_ptr->prev;
	  des_ptr->prev->next = den_ptr;
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
    }
    else
    {
      /* File-Entry */
      /*------------*/

      int n;
      char link_path[PATH_LENGTH + 1];

      /* Test, ob Eintrag Symbolischer Link ist */
      /*----------------------------------------*/

      n = 0; *link_path = '\0';

      if( S_ISLNK( stat_struct.st_mode ) )
      {
	/* Ja, symbolischer Name wird an "echten" Namen angehaengt */
	/*---------------------------------------------------------*/

	if( ( n = readlink( new_path, link_path, sizeof( link_path ) ) ) == -1 )
	{
          (void) strcpy( link_path, "unknown" );
	  n = strlen( link_path );
	}
	link_path[n] = '\0';

    /* FIX: Added +1 to allocation for name's null terminator (n already includes it for link) */
        if( ( fen_ptr = (FileEntry *)
		        calloc( 1, sizeof( FileEntry ) +
			strlen( dirent->d_name ) + 1 +
			n + 1 )
	    ) == NULL )
        {
          ERROR_MSG( "Malloc Failed*ABORT" );
          exit( 1 );
        }

        (void) strcpy( fen_ptr->name, dirent->d_name );
        (void) strcpy( &fen_ptr->name[strlen(fen_ptr->name) + 1],
		       link_path );
      }
      else
      {
        /* FIX: Added +1 to allocation for null terminator */
        if( ( fen_ptr = (FileEntry *)
		        calloc( 1, sizeof( FileEntry ) + strlen( dirent->d_name ) + 1 )
	    ) == NULL )
        {
          ERROR_MSG( "Malloc Failed*ABORT" );
          exit( 1 );
        }

        (void) strcpy( fen_ptr->name, dirent->d_name );
      }

      fen_ptr->next = NULL;
      fen_ptr->prev = NULL;
      fen_ptr->tagged = FALSE;

      (void) memcpy( &fen_ptr->stat_struct,
		     &stat_struct,
		     sizeof( stat_struct )
		    );

      fen_ptr->dir_entry = dir_entry;
      fes_ptr->next      = fen_ptr;
      fen_ptr->prev      = fes_ptr;
      fes_ptr            = fen_ptr;
      dir_entry->total_files++;
      dir_entry->total_bytes += stat_struct.st_size;
      statistic.disk_total_files++;
      statistic.disk_total_bytes += stat_struct.st_size;
    }
  }

  (void) closedir( dir );

  if( first_file_entry.next ) first_file_entry.next->prev = NULL;
  if( first_dir_entry.next )  first_dir_entry.next->prev = NULL;

  dir_entry->file = first_file_entry.next;
  dir_entry->sub_tree = first_dir_entry.next;

  DisplayDiskStatistic();
  doupdate();

  return( 0 );
}



void UnReadTree(DirEntry *dir_entry)
{
  FileEntry *fe_ptr, *next_fe_ptr;

  if( dir_entry == statistic.tree )
  {
    MESSAGE( "Can't delete ROOT" );
  }
  else
  {
    for( fe_ptr=dir_entry->file; fe_ptr; fe_ptr=next_fe_ptr )
    {
      next_fe_ptr = fe_ptr->next;
      RemoveFile( fe_ptr );
    }
    if( dir_entry->sub_tree )
    {
      UnReadSubTree( dir_entry->sub_tree );
    }
    if (statistic.disk_total_directories > 0)
        statistic.disk_total_directories--;
    (void) GetAvailBytes( &statistic.disk_space );
    DisplayDiskStatistic();
    doupdate();
  }
}


void UnReadSubTree(DirEntry *dir_entry)
{
  DirEntry *de_ptr, *next_de_ptr;
  FileEntry *fe_ptr, *next_fe_ptr;

  for( de_ptr = dir_entry; de_ptr; de_ptr = next_de_ptr )
  {
    next_de_ptr = de_ptr->next;

    for( fe_ptr=de_ptr->file; fe_ptr; fe_ptr=next_fe_ptr )
    {
      next_fe_ptr = fe_ptr->next;
      RemoveFile( fe_ptr );
    }

    if( de_ptr->sub_tree )
    {
      UnReadSubTree( de_ptr->sub_tree );
    }

    if( !de_ptr->up_tree->not_scanned )
      if (statistic.disk_total_directories > 0)
          statistic.disk_total_directories--;

    if( de_ptr->prev ) de_ptr->prev->next = de_ptr->next;
    else de_ptr->up_tree->sub_tree = de_ptr->next;
    if( de_ptr->next ) de_ptr->next->prev = de_ptr->prev;

    free( de_ptr );
  }
}


int RescanDir(DirEntry *dir_entry, int depth)
{
    char path[PATH_LENGTH + 1];
    FileEntry *fe_ptr, *next_fe_ptr;

    if (!dir_entry || (mode != DISK_MODE && mode != USER_MODE)) {
        return -1;
    }

    GetPath(dir_entry, path);

    /* Unlink and free all child directories recursively, updating stats. */
    if (dir_entry->sub_tree) {
        UnReadSubTree(dir_entry->sub_tree);
        dir_entry->sub_tree = NULL;
    }

    /* Unlink and free all files, updating stats. */
    for (fe_ptr = dir_entry->file; fe_ptr != NULL; fe_ptr = next_fe_ptr) {
        next_fe_ptr = fe_ptr->next;
        /* RemoveFile updates stats and frees the entry */
        RemoveFile(fe_ptr);
    }
    dir_entry->file = NULL;

    /*
     * The dir_entry itself still exists and is counted. ReadTree will re-init
     * its contents and re-add its children's stats. It also increments
     * disk_total_directories for itself, so we must pre-decrement to avoid
     * double-counting.
     */
    statistic.disk_total_directories--;
    ReadTree(dir_entry, path, depth);

    /* Since we just reloaded the tree, dot files are present in memory.
       We must now recalculate stats based on the current visibility setting. */
    RecalculateSysStats();

    /* Global matching stats are now incorrect. Reset and recalculate. */
    statistic.disk_matching_files = 0L;
    statistic.disk_matching_bytes = 0L;
    ApplyFilter(statistic.tree);

    return 0;
}