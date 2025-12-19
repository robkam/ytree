/***************************************************************************
 *
 * archive.c
 * Allg. Funktionen zum Bearbeiten von Archiven
 *
 ***************************************************************************/


#include "ytree.h"


#ifdef HAVE_LIBARCHIVE

/*
 * Helper to normalize archive internal paths for consistent comparison.
 * It handles:
 * - Empty path or "." -> canonical "."
 * - Leading "./" -> stripped
 * The result is copied into the provided buffer.
 */
static const char *normalize_archive_path_for_comparison(const char *path, char *buffer, size_t buffer_size) {
    if (!path || *path == '\0') {
        strncpy(buffer, ".", buffer_size); /* Canonical representation for archive root */
        buffer[buffer_size - 1] = '\0';
        return buffer;
    }
    /* Skip leading "./" if present */
    if (strncmp(path, "./", 2) == 0) {
        path += 2;
    }
    strncpy(buffer, path, buffer_size);
    buffer[buffer_size - 1] = '\0';
    return buffer;
}


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
    const char *target_name;
    int spin_counter = 0; /* Activity spinner counter */

    const char *effective_entry_path_segment; /* Points into entry_path or a derived segment */
    char normalized_target_name_buf[PATH_LENGTH + 1];
    const char *final_target_name;

    /*
     * 1. Determine the effective target path segment within the archive.
     *    This strips the archive_path prefix from entry_path if present,
     *    and skips any leading path separators.
     */
    size_t archive_path_len = strlen(archive_path);
    if (strncmp(entry_path, archive_path, archive_path_len) == 0) {
        effective_entry_path_segment = entry_path + archive_path_len;
        /* Skip any leading path separators after the archive path */
        while (*effective_entry_path_segment == FILE_SEPARATOR_CHAR) {
            effective_entry_path_segment++;
        }
    } else {
        /* If entry_path does not start with archive_path, it's assumed to be
         * a direct path within the archive (e.g., "file.txt" or "dir/file.txt"). */
        effective_entry_path_segment = entry_path;
    }

    /*
     * 2. Normalize the effective_entry_path_segment for consistent comparison.
     *    This handles cases like empty string or "." for the archive root,
     *    and strips any leading "./" if present (though less likely for target_name).
     */
    final_target_name = normalize_archive_path_for_comparison(
        effective_entry_path_segment, normalized_target_name_buf, sizeof(normalized_target_name_buf));

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
        const char *pathname_in_archive = archive_entry_pathname(entry);
        char normalized_archive_path_buf[PATH_LENGTH + 1];
        const char *clean_path;

        /*
         * 3. Normalize the path obtained from the archive entry for comparison.
         *    This primarily strips any leading "./" that some archives might store.
         */
        clean_path = normalize_archive_path_for_comparison(
            pathname_in_archive, normalized_archive_path_buf, sizeof(normalized_archive_path_buf));

        /* Compare the normalized paths to find the desired entry */
        if (clean_path && strcmp(clean_path, final_target_name) == 0) {
            /* Found the entry, now write its data to the fd */
            while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK) {
                /* Update Spinner during extraction */
                if ((++spin_counter % 100) == 0) {
                    DrawSpinner();
                    doupdate();
                }
                if (write(out_fd, buff, size) != (ssize_t)size) {
                    /* Write error */
                    archive_read_free(a);
                    return -1;
                }
            }
            archive_read_free(a);
            return (r == ARCHIVE_EOF) ? 0 : -1; /* Return success only on clean EOF */
        }
        /* Update spinner while searching headers too */
        if ((++spin_counter % 50) == 0) {
            DrawSpinner();
            doupdate();
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
    /* If there is no separator, it's a top level dir.
       In TryInsertArchiveDirEntry loop or manual calls, we might get here.
       If path has no slash, father is root.
    */
    p = NULL;
    /* Proceed to check if we are inserting top level or failed split */
  }

  /* Recalculate p based on string operations above */
  if (p) {
      /* We replaced the last slash with \0. Now p points to that \0.
         Wait, strrchr returns pointer to char.
         If we did *p = '\0', then strrchr(father_path) again might fail or find prev slash.
         Logic:
         Input: "A/B" -> father_path "A", name "B"
         Input: "A" -> father_path "A" (no slash found initially?)

         Actually, standard convention here:
         path="A/B" -> father="A", name="B"
         path="A" -> father="", name="A" (Root)
      */
  }

  /* Reset and do it cleanly */
  strcpy(father_path, path);
  p = strrchr(father_path, FILE_SEPARATOR_CHAR);

  if (p) {
      *p = '\0';
      strcpy(name, p + 1);

      /* Find father directory */
      if( GetArchiveDirEntry( tree, father_path, &df_ptr ) )
      {
        (void) snprintf( message, MESSAGE_LENGTH, "can't find subdir*%s", father_path );
        ERROR_MSG( message );
        return( -1 );
      }
  } else {
      /* Top Level Directory */
      df_ptr = tree;
      strcpy(name, path);
  }

  /*
   * FIX: Allocate exact size for name + null terminator.
   */
  if( ( de_ptr = (DirEntry *) calloc( 1, sizeof( DirEntry ) + strlen(name) + 1 ) ) == NULL )
  {
    ERROR_MSG( "Malloc failed*ABORT" );
    exit( 1 );
  }

  (void) strcpy( de_ptr->name, name );
  (void) memcpy( (char *) &de_ptr->stat_struct, (char *) stat, sizeof( struct stat ) );

  /* Directory einklinken (Link Directory into Tree) */
  /*-------------------------------------------------*/

  if( df_ptr->sub_tree == NULL )
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
        else de_ptr->up_tree->sub_tree = de_ptr; /* Fix head pointer if inserting at start */
        ds_ptr->prev = de_ptr;
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
    fprintf( stderr, "can't get directory for file*%s*trying recover\n", path );
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
      (void) snprintf( message, MESSAGE_LENGTH, "again: can't get directory for file*%s*giving up", path );
      ERROR_MSG( message );
      return( -1 );
    }
  }

  if( S_ISLNK( stat->st_mode ) )
    n = strlen( &path[ strlen( path ) + 1 ] ) + 1;
  else
    n = 0;

  /* FIX: Allocate exact size for name + null + link data */
  if( ( fe_ptr = (FileEntry *) calloc( 1, sizeof( FileEntry ) + strlen(file) + 1 + n + 1 ) ) == NULL )
  {
    ERROR_MSG( "Malloc failed" );
    return -1;
  }

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


/*
 * GetArchiveDirEntry
 * Robustly searching for a path within the tree.
 * tree: The Root directory entry.
 * path: The path to find (e.g., "A/B" or "A").
 * dir_entry: Output pointer.
 */
static int GetArchiveDirEntry(DirEntry *tree, char *path, DirEntry **dir_entry)
{
    char *path_copy;
    char *token, *saveptr;
    DirEntry *current = tree;
    DirEntry *child;
    int found;

    if (!path || *path == '\0' || strcmp(path, ".") == 0) {
        *dir_entry = tree;
        return 0;
    }

    path_copy = strdup(path);
    if (!path_copy) return -1;

    token = strtok_r(path_copy, FILE_SEPARATOR_STRING, &saveptr);
    while (token) {
        found = 0;
        /* Search children of 'current' */
        for (child = current->sub_tree; child; child = child->next) {
            if (strcmp(child->name, token) == 0) {
                current = child;
                found = 1;
                break;
            }
        }

        if (!found) {
            free(path_copy);
            return -1;
        }

        token = strtok_r(NULL, FILE_SEPARATOR_STRING, &saveptr);
    }

    free(path_copy);
    *dir_entry = current;
    return 0;
}


/*
 * TryInsertArchiveDirEntry
 * Iteratively ensures every component of the path exists in the tree.
 */
int TryInsertArchiveDirEntry(DirEntry *tree, char *dir, struct stat *stat)
{
    char *path_copy;
    char *token, *saveptr;
    char current_path[PATH_LENGTH + 1];
    DirEntry *dummy;

    if (!dir || *dir == '\0') return 0;

    path_copy = strdup(dir);
    if (!path_copy) return -1;

    current_path[0] = '\0';

    token = strtok_r(path_copy, FILE_SEPARATOR_STRING, &saveptr);
    while (token) {
        /* Append token to current_path */
        if (current_path[0] != '\0') {
            strcat(current_path, FILE_SEPARATOR_STRING);
        }
        strcat(current_path, token);

        /* Check if this partial path exists */
        if (GetArchiveDirEntry(tree, current_path, &dummy) != 0) {
            /* Not found, insert it */
            if (InsertArchiveDirEntry(tree, current_path, stat) != 0) {
                free(path_copy);
                return -1;
            }
        }

        token = strtok_r(NULL, FILE_SEPARATOR_STRING, &saveptr);
    }

    free(path_copy);
    return 0;
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
      /* The DirEntry->name buffer is now allocated with PATH_LENGTH + 1,
       * providing sufficient space for these strcat operations. */
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
    /* The DirEntry->name buffer is now allocated with PATH_LENGTH + 1,
     * providing sufficient space for these strcat operations. */
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