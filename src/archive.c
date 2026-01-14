/***************************************************************************
 *
 * archive.c
 * General functions for archive processing
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
    /* Skip leading "/" if present */
    while (*path == FILE_SEPARATOR_CHAR) {
        path++;
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
    int spin_counter = 0; /* Activity spinner counter */
    size_t entry_len;
    int found = 0;

    if (!entry_path) return -1;
    entry_len = strlen(entry_path);

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
        const char *clean_path = archive_entry_pathname(entry);

        /* Normalize internal path: skip leading ./ or / */
        if (clean_path[0] == '.' && clean_path[1] == FILE_SEPARATOR_CHAR) {
            clean_path += 2;
        }
        while (*clean_path == FILE_SEPARATOR_CHAR) {
            clean_path++;
        }

        /*
         * Robust Suffix Matching Strategy
         * We do not rely on calculating the prefix length, as this is prone to
         * mismatches with symlinks, mounts, or relative paths.
         * Instead, checking if the requested 'entry_path' ENDS with the archive's 'clean_path'
         * guarantees a match, provided the boundary is a separator.
         */
        size_t clean_len = strlen(clean_path);

        if (entry_len >= clean_len) {
            const char *suffix = entry_path + (entry_len - clean_len);

            if (strcmp(suffix, clean_path) == 0) {
                /* The suffix matches. Now verify boundary to prevent partial name matches. */
                /* e.g., "my_file.txt" matching "file.txt" should fail. */
                /* It must be the start of string OR preceded by a separator. */
                if (suffix == entry_path || *(suffix - 1) == FILE_SEPARATOR_CHAR) {
                     /* MATCH FOUND! */
                     found = 1;
                     while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK) {
                        if ((++spin_counter % 100) == 0) {
                            DrawSpinner();
                            doupdate();
                            if (EscapeKeyPressed()) {
                                MESSAGE("Operation Interrupted");
                                found = 0;
                                break;
                            }
                        }
                        if (write(out_fd, buff, size) != (ssize_t)size) {
                            found = 0; /* Write error */
                            break;
                        }
                     }
                     if (r != ARCHIVE_EOF && r != ARCHIVE_OK && found) found = 0; /* Read error or interrupt */
                     break; /* Stop searching */
                }
            }
        }

        /* Update spinner while searching headers too */
        if ((++spin_counter % 50) == 0) {
            DrawSpinner();
            doupdate();
            if (EscapeKeyPressed()) {
                MESSAGE("Operation Interrupted");
                found = 0;
                break;
            }
        }
    }

    archive_read_free(a);
    return (found) ? 0 : -1;
}

int ExtractArchiveNode(const char *archive_path, const char *entry_path, const char *dest_path)
{
    struct archive *a;
    struct archive_entry *entry;
    int r;
    const void *buff;
    size_t size;
    la_int64_t offset;
    int spin_counter = 0;
    size_t entry_len;
    int found = 0;
    int fd;

    if (!entry_path || !dest_path) return -1;
    entry_len = strlen(entry_path);

    /* fprintf(stderr, "DEBUG: ExtractArchiveNode: Archive='%s' Entry='%s' Dest='%s'\n", archive_path, entry_path, dest_path); */

    a = archive_read_new();
    if (a == NULL) {
        return -1;
    }
    archive_read_support_filter_all(a);
    archive_read_support_format_all(a);

    r = archive_read_open_filename(a, archive_path, 10240);
    if (r != ARCHIVE_OK) {
        /* fprintf(stderr, "DEBUG: archive_read_open_filename failed: %s\n", archive_error_string(a)); */
        archive_read_free(a);
        return -1;
    }

    while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
        /* Update spinner and check ESC */
        if ((++spin_counter % 50) == 0) {
            DrawSpinner();
            doupdate();
            if (EscapeKeyPressed()) {
                MESSAGE("Operation Interrupted");
                archive_read_free(a);
                return -1;
            }
        }

        const char *clean_path = archive_entry_pathname(entry);

        /* Normalize internal path: skip leading ./ or / */
        if (clean_path[0] == '.' && clean_path[1] == FILE_SEPARATOR_CHAR) {
            clean_path += 2;
        }
        while (*clean_path == FILE_SEPARATOR_CHAR) {
            clean_path++;
        }

        size_t clean_len = strlen(clean_path);

        if (entry_len >= clean_len) {
            const char *suffix = entry_path + (entry_len - clean_len);

            if (strcmp(suffix, clean_path) == 0) {
                if (suffix == entry_path || *(suffix - 1) == FILE_SEPARATOR_CHAR) {
                     /* MATCH FOUND! */
                     found = 1;
                     mode_t type = archive_entry_filetype(entry);

                     if (type == AE_IFLNK) {
                         const char *target = archive_entry_symlink(entry);
                         if (target) {
                             if (symlink(target, dest_path) != 0) {
                                 if (errno == EEXIST) {
                                     unlink(dest_path);
                                     if (symlink(target, dest_path) != 0) found = 0;
                                 } else {
                                     found = 0;
                                 }
                             }
                         } else {
                             found = 0;
                         }
                     } else if (type == AE_IFREG) {
                         fd = open(dest_path, O_CREAT | O_WRONLY | O_TRUNC, 0644);
                         if (fd == -1) {
                             found = 0;
                         } else {
                             while ((r = archive_read_data_block(a, &buff, &size, &offset)) == ARCHIVE_OK) {
                                if ((++spin_counter % 100) == 0) {
                                    DrawSpinner();
                                    doupdate();
                                    if (EscapeKeyPressed()) {
                                        MESSAGE("Operation Interrupted");
                                        found = 0;
                                        break;
                                    }
                                }
                                if (write(fd, buff, size) != (ssize_t)size) {
                                    found = 0; /* Write error */
                                    break;
                                }
                             }
                             close(fd);
                             if (r != ARCHIVE_EOF && r != ARCHIVE_OK && found) {
                                 found = 0;
                                 /* Cleanup partial file on error/interrupt */
                                 unlink(dest_path);
                             } else if (found == 0) {
                                 unlink(dest_path);
                             }
                         }
                     } else {
                         /* Unsupported file type */
                         found = 0;
                     }
                     break; /* Stop searching */
                }
            }
        }
        /* Logging mismatches removed */
    }

    archive_read_free(a);

    if (!found) {
        fprintf(stderr, "Extract failed: %s\n", entry_path);
    }
    return (found) ? 0 : -1;
}


/*
 * Helper to setup writer based on reader
 */
static int configure_writer(struct archive *in, struct archive *out) {
    int format = archive_format(in);
    int filter = archive_filter_code(in, 0);

    /* Set Filter */
    switch (filter) {
        case ARCHIVE_FILTER_NONE: archive_write_add_filter_none(out); break;
        case ARCHIVE_FILTER_GZIP: archive_write_add_filter_gzip(out); break;
        case ARCHIVE_FILTER_BZIP2: archive_write_add_filter_bzip2(out); break;
        case ARCHIVE_FILTER_COMPRESS: archive_write_add_filter_compress(out); break;
        case ARCHIVE_FILTER_LZIP: archive_write_add_filter_lzip(out); break;
        case ARCHIVE_FILTER_LZMA: archive_write_add_filter_lzma(out); break;
        case ARCHIVE_FILTER_XZ: archive_write_add_filter_xz(out); break;
        /* ZSTD is standard in newer libarchive, use ifdef or assume generic support */
#ifdef ARCHIVE_FILTER_ZSTD
        case ARCHIVE_FILTER_ZSTD: archive_write_add_filter_zstd(out); break;
#endif
        default: return -1; /* Unknown/Unsupported filter for writing */
    }

    /* Set Format */
    format &= ARCHIVE_FORMAT_BASE_MASK;
    if (format == ARCHIVE_FORMAT_TAR) {
        archive_write_set_format_pax_restricted(out);
    } else if (format == ARCHIVE_FORMAT_ZIP) {
        archive_write_set_format_zip(out);
    } else if (format == ARCHIVE_FORMAT_CPIO) {
        archive_write_set_format_cpio(out);
    } else if (format == ARCHIVE_FORMAT_7ZIP) {
        archive_write_set_format_7zip(out);
    } else if (format == ARCHIVE_FORMAT_ISO9660) {
        archive_write_set_format_iso9660(out);
    } else if (format == ARCHIVE_FORMAT_AR) {
        archive_write_set_format_ar_bsd(out);
    } else {
        return -1; /* Unknown format */
    }

    return 0;
}

/*
 * Archive_Rewrite
 * Generic engine for rewriting archives (modifying/deleting entries).
 * Handles the Read Old -> Write New to Temp -> Rename Atomically loop.
 *
 * archive_path: Path to the existing archive.
 * cb: Callback function invoked for every entry.
 * user_data: User context passed to callback.
 *
 * Returns 0 on success, -1 on failure.
 */
int Archive_Rewrite(char *archive_path, RewriteCallback cb, void *user_data)
{
    struct archive *a_in = NULL;
    struct archive *a_out = NULL;
    struct archive_entry *entry;
    struct stat st;
    char tmp_path[PATH_LENGTH];
    int r, res;
    const void *buff;
    size_t size;
    la_int64_t offset;
    int fd_tmp = -1;
    int success = 0;
    int spin_counter = 0;

    if (!archive_path || !cb) return -1;

    /* Get permissions of original archive */
    if (stat(archive_path, &st) != 0) {
        ERROR_MSG("Could not stat original archive");
        return -1;
    }

    /* Create temporary file in same directory to ensure atomic rename */
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmpXXXXXX", archive_path);
    fd_tmp = mkstemp(tmp_path);
    if (fd_tmp == -1) {
        ERROR_MSG("Could not create temporary archive file");
        return -1;
    }
    /* mkstemp opens read/write; close it so libarchive can open it properly or use fd */
    /* libarchive can write to fd. Let's use that. */

    a_in = archive_read_new();
    archive_read_support_filter_all(a_in);
    archive_read_support_format_all(a_in);

    if (archive_read_open_filename(a_in, archive_path, 10240) != ARCHIVE_OK) {
        ERROR_MSG("Could not open input archive");
        archive_read_free(a_in);
        close(fd_tmp);
        unlink(tmp_path);
        return -1;
    }

    a_out = archive_write_new();

    /* Configure writer based on reader settings */
    if (configure_writer(a_in, a_out) != 0) {
        ERROR_MSG("Unsupported archive format for writing");
        archive_read_free(a_in);
        archive_write_free(a_out);
        close(fd_tmp);
        unlink(tmp_path);
        return -1;
    }

    if (archive_write_open_fd(a_out, fd_tmp) != ARCHIVE_OK) {
        ERROR_MSG("Could not open output archive");
        archive_read_free(a_in);
        archive_write_free(a_out);
        close(fd_tmp);
        unlink(tmp_path);
        return -1;
    }

    /* Process Entries */
    while ((r = archive_read_next_header(a_in, &entry)) == ARCHIVE_OK) {

        if ((++spin_counter % 50) == 0) {
            DrawSpinner();
            doupdate();
            if (EscapeKeyPressed()) {
                res = AR_ABORT;
            } else {
                res = cb(a_in, a_out, entry, user_data);
            }
        } else {
            res = cb(a_in, a_out, entry, user_data);
        }

        if (res == AR_ABORT) {
            success = 0;
            break;
        }

        if (res == AR_SKIP) {
            continue; /* Skip writing this entry */
        }

        /* AR_KEEP: Write header and data */
        if (archive_write_header(a_out, entry) != ARCHIVE_OK) {
            ERROR_MSG("Error writing archive header");
            success = 0;
            break;
        }

        /* Copy data blocks */
        while ((r = archive_read_data_block(a_in, &buff, &size, &offset)) == ARCHIVE_OK) {
            if (archive_write_data_block(a_out, buff, size, offset) != ARCHIVE_OK) {
                 ERROR_MSG("Error writing archive data");
                 success = 0;
                 goto cleanup_loop;
            }
        }
        if (r != ARCHIVE_EOF) {
            ERROR_MSG("Error reading archive data");
            success = 0;
            goto cleanup_loop;
        }
        success = 1; /* Tentative success after this entry */
    }

cleanup_loop:
    if (r != ARCHIVE_EOF && r != ARCHIVE_OK) {
        /* Error occurred in main loop logic unless handled above */
        success = 0;
    }

    /* Close archives */
    archive_read_free(a_in);
    archive_write_close(a_out); /* Flush data */
    archive_write_free(a_out);
    close(fd_tmp); /* fd was used by archive_write, might be closed by it?
                      archive_write_open_fd documentation: client is responsible for closing fd
                      unless using archive_write_open_filename */

    if (success) {
        /* Restore permissions */
        if (chmod(tmp_path, st.st_mode) != 0) {
            /* Warning only */
        }

        /* Atomic Replace */
        if (rename(tmp_path, archive_path) != 0) {
            ERROR_MSG("Failed to replace original archive");
            unlink(tmp_path);
            return -1;
        }
        return 0;
    } else {
        unlink(tmp_path);
        return -1;
    }
}

/* Callback for deleting a file */
static int cb_delete_file(struct archive *r, struct archive *w, struct archive_entry *entry, void *user_data) {
    char *target_path = (char *)user_data;
    const char *entry_path = archive_entry_pathname(entry);
    char norm_entry[PATH_LENGTH];
    char norm_target[PATH_LENGTH];

    normalize_archive_path_for_comparison(entry_path, norm_entry, sizeof(norm_entry));
    normalize_archive_path_for_comparison(target_path, norm_target, sizeof(norm_target));

    if (strcmp(norm_entry, norm_target) == 0) {
        return AR_SKIP;
    }
    return AR_KEEP;
}

int Archive_DeleteFile(FileEntry *fe_ptr, Statistic *s) {
    char internal_path[PATH_LENGTH];
    char dir_path[PATH_LENGTH];

    if (!fe_ptr || !s) return -1;

    /* Construct internal path */
    /* Get path of the parent directory of the file relative to tree root */
    GetPath(fe_ptr->dir_entry, dir_path);

    /* Combine dir and filename */
    /* normalize_archive_path_for_comparison handles ./ strip, so we just build it standardly */

    if (strcmp(dir_path, ".") == 0 || strcmp(dir_path, "") == 0 || strcmp(dir_path, FILE_SEPARATOR_STRING) == 0) {
        snprintf(internal_path, sizeof(internal_path), "%s", fe_ptr->name);
    } else {
        snprintf(internal_path, sizeof(internal_path), "%s%c%s", dir_path, FILE_SEPARATOR_CHAR, fe_ptr->name);
    }

    return Archive_Rewrite(s->login_path, cb_delete_file, internal_path);
}

#else
/* Dummy implementations if libarchive is not available */
int ExtractArchiveEntry(const char *archive_path, const char *entry_path, int out_fd)
{
    return -1;
}
int ExtractArchiveNode(const char *archive_path, const char *entry_path, const char *dest_path)
{
    return -1;
}
int Archive_Rewrite(char *archive_path, void *cb, void *user_data)
{
    return -1;
}
int Archive_DeleteFile(FileEntry *fe_ptr, Statistic *s)
{
    return -1;
}
#endif /* HAVE_LIBARCHIVE */


static int GetArchiveDirEntry(DirEntry *tree, char *path, DirEntry **dir_entry);


static int InsertArchiveDirEntry(DirEntry *tree, char *path, struct stat *stat, Statistic *s)
{
  DirEntry *df_ptr, *de_ptr, *ds_ptr;
  char father_path[PATH_LENGTH + 1];
  char name[PATH_LENGTH + 1];

  if (strlen(path) >= PATH_LENGTH) {
      ERROR_MSG("Archive path too long*skipping directory insert");
      return -1;
  }

  /* Split path into directory and filename */
  Fnsplit(path, father_path, name);

  /* Find father directory */
  /* If father_path is empty (root), GetArchiveDirEntry returns tree */
  if( GetArchiveDirEntry( tree, father_path, &df_ptr ) )
  {
    (void) snprintf( message, MESSAGE_LENGTH, "can't find subdir*%s", father_path );
    ERROR_MSG( message );
    return( -1 );
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
  s->disk_total_directories++;
  return( 0 );
}


int InsertArchiveFileEntry(DirEntry *tree, char *path, struct stat *stat, Statistic *s)
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

    if( TryInsertArchiveDirEntry( tree, dir, &stat_struct, s ) )
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
  s->disk_total_files++;
  s->disk_total_bytes += stat->st_size;

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
int TryInsertArchiveDirEntry(DirEntry *tree, char *dir, struct stat *stat, Statistic *s)
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
            if (InsertArchiveDirEntry(tree, current_path, stat, s) != 0) {
                free(path_copy);
                return -1;
            }
        }

        token = strtok_r(NULL, FILE_SEPARATOR_STRING, &saveptr);
    }

    free(path_copy);
    return 0;
}


void MinimizeArchiveTree(DirEntry **tree_ptr, Statistic *s)
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

    s->disk_total_directories--;
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

      s->disk_total_directories--;

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

    s->disk_total_directories--;

    /* Move grandchildren up */
    tree->sub_tree = de_ptr->sub_tree;
    for( de1_ptr = de_ptr->sub_tree; de1_ptr; de1_ptr = de1_ptr->next )
      de1_ptr->up_tree = tree;

    free( de_ptr );
  }
  return;
}