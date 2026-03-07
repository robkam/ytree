/***************************************************************************
 *
 * src/fs/archive_read.c
 * Archive scanning, reading, and extraction functions
 *
 ***************************************************************************/

#include "ytree.h"

#ifdef HAVE_LIBARCHIVE

/* Helper macros for callback reporting */
#define REPORT_PROGRESS(cb, user_data)                                         \
  do {                                                                         \
    if ((cb) &&                                                                \
        (cb)(ARCHIVE_STATUS_PROGRESS, NULL, (user_data)) == ARCHIVE_CB_ABORT)  \
      return ARCHIVE_CB_ABORT;                                                 \
  } while (0)

#define REPORT_ERROR(cb, user_data, fmt, ...)                                  \
  do {                                                                         \
    char _msg[1024];                                                           \
    snprintf(_msg, sizeof(_msg), fmt, ##__VA_ARGS__);                          \
    if (cb)                                                                    \
      (cb)(ARCHIVE_STATUS_ERROR, _msg, (user_data));                           \
  } while (0)

#define REPORT_WARNING(cb, user_data, fmt, ...)                                \
  do {                                                                         \
    char _msg[1024];                                                           \
    snprintf(_msg, sizeof(_msg), fmt, ##__VA_ARGS__);                          \
    if (cb)                                                                    \
      (cb)(ARCHIVE_STATUS_WARNING, _msg, (user_data));                         \
  } while (0)

/*
 * Uses libarchive to find a specific entry within an archive and write its
 * contents to the provided output file descriptor.
 * Returns 0 on success, -1 on failure.
 */
int ExtractArchiveEntry(const char *archive_path, const char *entry_path,
                        int out_fd, ArchiveProgressCallback cb,
                        void *user_data) {
  struct archive *a;
  struct archive_entry *entry;
  int r;
  const void *buff;
  size_t size;
  la_int64_t offset;
  int spin_counter = 0; /* Activity spinner counter */
  size_t entry_len;
  int found = 0;

  if (!entry_path)
    return -1;
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

    size_t clean_len = strlen(clean_path);

    if (entry_len >= clean_len) {
      const char *suffix = entry_path + (entry_len - clean_len);

      if (strcmp(suffix, clean_path) == 0) {
        /* The suffix matches. Now verify boundary to prevent partial name
         * matches. */
        if (suffix == entry_path || *(suffix - 1) == FILE_SEPARATOR_CHAR) {
          /* MATCH FOUND! */
          found = 1;
          while ((r = archive_read_data_block(a, &buff, &size, &offset)) ==
                 ARCHIVE_OK) {
            if ((++spin_counter % 100) == 0) {
              if (cb && cb(ARCHIVE_STATUS_PROGRESS, NULL, user_data) ==
                            ARCHIVE_CB_ABORT) {
                REPORT_ERROR(cb, user_data, "Operation Interrupted");
                found = 0;
                break;
              }
            }
            if (write(out_fd, buff, size) != (ssize_t)size) {
              found = 0; /* Write error */
              break;
            }
          }
          if (r != ARCHIVE_EOF && r != ARCHIVE_OK && found)
            found = 0; /* Read error or interrupt */
          break;       /* Stop searching */
        }
      }
    }

    /* Update spinner while searching headers too */
    if ((++spin_counter % 50) == 0) {
      if (cb &&
          cb(ARCHIVE_STATUS_PROGRESS, NULL, user_data) == ARCHIVE_CB_ABORT) {
        REPORT_ERROR(cb, user_data, "Operation Interrupted");
        found = 0;
        break;
      }
    }
  }

  archive_read_free(a);
  return (found) ? 0 : -1;
}

int ExtractArchiveNode(const char *archive_path, const char *entry_path,
                       const char *dest_path, ArchiveProgressCallback cb,
                       void *user_data) {
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

  if (!entry_path || !dest_path)
    return -1;
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
    /* Update spinner and check ESC */
    if ((++spin_counter % 50) == 0) {
      if (cb &&
          cb(ARCHIVE_STATUS_PROGRESS, NULL, user_data) == ARCHIVE_CB_ABORT) {
        REPORT_ERROR(cb, user_data, "Operation Interrupted");
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
                  if (symlink(target, dest_path) != 0)
                    found = 0;
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
              while ((r = archive_read_data_block(a, &buff, &size, &offset)) ==
                     ARCHIVE_OK) {
                if ((++spin_counter % 100) == 0) {
                  if (cb && cb(ARCHIVE_STATUS_PROGRESS, NULL, user_data) ==
                                ARCHIVE_CB_ABORT) {
                    REPORT_ERROR(cb, user_data, "Operation Interrupted");
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
  }

  archive_read_free(a);

  if (!found) {
    /* Optionally report warning, but found=0 acts as return -1 */
  }
  return (found) ? 0 : -1;
}

#else
/* Dummy implementations if libarchive is not available */
int ExtractArchiveEntry(const char *archive_path, const char *entry_path,
                        int out_fd, ArchiveProgressCallback cb,
                        void *user_data) {
  return -1;
}
int ExtractArchiveNode(const char *archive_path, const char *entry_path,
                       const char *dest_path, ArchiveProgressCallback cb,
                       void *user_data) {
  return -1;
}
#endif /* HAVE_LIBARCHIVE */

static int GetArchiveDirEntry(DirEntry *tree, char *path, DirEntry **dir_entry);

static int InsertArchiveDirEntry(ViewContext *ctx, DirEntry *tree, char *path,
                                 struct stat *stat, Statistic *s) {
  DirEntry *df_ptr, *de_ptr, *ds_ptr;
  char father_path[PATH_LENGTH + 1];
  char name[PATH_LENGTH + 1];

  if (strlen(path) >= PATH_LENGTH) {
    MESSAGE(ctx, "Archive path too long*skipping directory insert");
    return -1;
  }

  Fnsplit(path, father_path, name);

  if (GetArchiveDirEntry(tree, father_path, &df_ptr)) {
    MESSAGE(ctx, "can't find subdir*%s", father_path);
    return (-1);
  }

  /* FIX: Allocate exact size for name + null terminator */
  de_ptr = (DirEntry *)xcalloc(1, sizeof(DirEntry) + strlen(name) + 1);

  (void)strcpy(de_ptr->name, name);
  (void)memcpy((char *)&de_ptr->stat_struct, (char *)stat, sizeof(struct stat));

  if (df_ptr->sub_tree == NULL) {
    de_ptr->up_tree = df_ptr;
    df_ptr->sub_tree = de_ptr;
  } else {
    de_ptr->up_tree = df_ptr;

    for (ds_ptr = df_ptr->sub_tree; ds_ptr; ds_ptr = ds_ptr->next) {
      if (strcmp(ds_ptr->name, de_ptr->name) > 0) {
        de_ptr->next = ds_ptr;
        de_ptr->prev = ds_ptr->prev;
        if (ds_ptr->prev)
          ds_ptr->prev->next = de_ptr;
        else
          de_ptr->up_tree->sub_tree = de_ptr;
        ds_ptr->prev = de_ptr;
        break;
      }

      if (ds_ptr->next == NULL) {
        de_ptr->prev = ds_ptr;
        de_ptr->next = ds_ptr->next;
        ds_ptr->next = de_ptr;
        break;
      }
    }
  }
  s->disk_total_directories++;
  return (0);
}

int InsertArchiveFileEntry(ViewContext *ctx, DirEntry *tree, char *path,
                           struct stat *stat, Statistic *s) {
  char dir[PATH_LENGTH + 1];
  char file[PATH_LENGTH + 1];
  DirEntry *de_ptr;
  FileEntry *fs_ptr, *fe_ptr;
  struct stat stat_struct;
  int n;

  if (KeyPressed()) {
    Quit(ctx);
  }

  Fnsplit(path, dir, file);

  if (GetArchiveDirEntry(tree, dir, &de_ptr)) {
#ifdef DEBUG
    fprintf(stderr, "can't get directory for file*%s*trying recover\n", path);
#endif

    (void)memset((char *)&stat_struct, 0, sizeof(struct stat));
    stat_struct.st_mode = S_IFDIR;

    if (TryInsertArchiveDirEntry(ctx, tree, dir, &stat_struct, s)) {
      MESSAGE(ctx, "inserting directory failed");
      return (-1);
    }
    if (GetArchiveDirEntry(tree, dir, &de_ptr)) {
      MESSAGE(ctx, "again: can't get directory for file*%s*giving up", path);
      return (-1);
    }
  }

  if (S_ISLNK(stat->st_mode))
    n = strlen(&path[strlen(path) + 1]) + 1;
  else
    n = 0;

  /* FIX: Allocate exact size for name + null + link data */
  fe_ptr =
      (FileEntry *)xcalloc(1, sizeof(FileEntry) + strlen(file) + 1 + n + 1);

  (void)memcpy((char *)&fe_ptr->stat_struct, (char *)stat, sizeof(struct stat));
  (void)strcpy(fe_ptr->name, file);

  if (S_ISLNK(stat->st_mode)) {
    (void)strcpy(&fe_ptr->name[strlen(fe_ptr->name) + 1],
                 &path[strlen(path) + 1]);
  }

  fe_ptr->dir_entry = de_ptr;
  de_ptr->total_files++;
  de_ptr->total_bytes += stat->st_size;
  s->disk_total_files++;
  s->disk_total_bytes += stat->st_size;

  if (de_ptr->file == NULL) {
    de_ptr->file = fe_ptr;
  } else {
    for (fs_ptr = de_ptr->file; fs_ptr->next; fs_ptr = fs_ptr->next)
      ;

    fe_ptr->prev = fs_ptr;
    fs_ptr->next = fe_ptr;
  }
  return (0);
}

static int GetArchiveDirEntry(DirEntry *tree, char *path,
                              DirEntry **dir_entry) {
  char *path_copy;
  char *token, *saveptr;
  DirEntry *current = tree;
  DirEntry *child;
  int found;

  if (!path || *path == '\0' || strcmp(path, ".") == 0) {
    *dir_entry = tree;
    return 0;
  }

  path_copy = xstrdup(path);

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

int TryInsertArchiveDirEntry(ViewContext *ctx, DirEntry *tree, char *dir,
                             struct stat *stat, Statistic *s) {
  char *path_copy;
  char *token, *saveptr;
  char current_path[PATH_LENGTH + 1];
  DirEntry *dummy;

  if (!dir || *dir == '\0')
    return 0;

  path_copy = xstrdup(dir);

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
      if (InsertArchiveDirEntry(ctx, tree, current_path, stat, s) != 0) {
        free(path_copy);
        return -1;
      }
    }

    token = strtok_r(NULL, FILE_SEPARATOR_STRING, &saveptr);
  }

  free(path_copy);
  return 0;
}

void MinimizeArchiveTree(DirEntry **tree_ptr, Statistic *s) {
  DirEntry *tree = *tree_ptr;
  DirEntry *de_ptr, *de1_ptr;
  DirEntry *next_ptr;
  FileEntry *fe_ptr;

  /* 1. Collapse Root if empty and has siblings */
  if (tree->prev == NULL && tree->next != NULL && tree->file == NULL &&
      tree->sub_tree == NULL) {
    DirEntry *new_root = tree->next;
    *tree_ptr = new_root;
    new_root->prev = NULL;
    new_root->up_tree = NULL;
    s->disk_total_directories--;
    free(tree);
    tree = new_root;
  }

  /* 2. Collapse empty leaf directories in sub-trees */
  for (de_ptr = tree->sub_tree; de_ptr; de_ptr = next_ptr) {
    next_ptr = de_ptr->next;

    if (de_ptr->prev == NULL && de_ptr->next == NULL && de_ptr->file == NULL) {
      if (strcmp(tree->name, FILE_SEPARATOR_STRING))
        (void)strcat(tree->name, FILE_SEPARATOR_STRING);
      (void)strcat(tree->name, de_ptr->name);

      s->disk_total_directories--;

      tree->sub_tree = de_ptr->sub_tree;

      for (de1_ptr = de_ptr->sub_tree; de1_ptr; de1_ptr = de1_ptr->next)
        de1_ptr->up_tree = tree;

      free(de_ptr);
      continue;
    }
    break;
  }

  /* 3. Collapse root into its single child if applicable */
  if (tree->prev == NULL && tree->next == NULL && tree->file == NULL &&
      tree->sub_tree && tree->sub_tree->prev == NULL &&
      tree->sub_tree->next == NULL) {
    de_ptr = tree->sub_tree;

    if (strcmp(tree->name, FILE_SEPARATOR_STRING))
      (void)strcat(tree->name, FILE_SEPARATOR_STRING);
    (void)strcat(tree->name, de_ptr->name);

    tree->file = de_ptr->file;
    for (fe_ptr = tree->file; fe_ptr; fe_ptr = fe_ptr->next)
      fe_ptr->dir_entry = tree;

    (void)memcpy((char *)&tree->stat_struct, (char *)&de_ptr->stat_struct,
                 sizeof(struct stat));

    s->disk_total_directories--;

    tree->sub_tree = de_ptr->sub_tree;
    for (de1_ptr = de_ptr->sub_tree; de1_ptr; de1_ptr = de1_ptr->next)
      de1_ptr->up_tree = tree;

    free(de_ptr);
  }
  return;
}

/*
 * Copy stat data from libarchive's const struct to our mutable one.
 */
static void copy_stat_from_entry(struct stat *dest,
                                 struct archive_entry *entry) {
  const struct stat *st;

  memset(dest, 0, sizeof(struct stat));
  st = archive_entry_stat(entry);

  if (st) {
    dest->st_mode = st->st_mode;
    dest->st_uid = st->st_uid;
    dest->st_gid = st->st_gid;
    dest->st_size = st->st_size;
    dest->st_mtime = st->st_mtime;
    dest->st_nlink = st->st_nlink;
  } else {
    /* Fallback if stat is not available */
    dest->st_mode = archive_entry_mode(entry);
    dest->st_uid = archive_entry_uid(entry);
    dest->st_gid = archive_entry_gid(entry);
    dest->st_size = archive_entry_size(entry);
    dest->st_mtime = archive_entry_mtime(entry);
    dest->st_nlink = archive_entry_nlink(entry);
  }

  /*
   * Strictly enforce file type bits based on libarchive's explicit filetype.
   * This prevents regular files from being misidentified as directories if
   * the archive header's mode bits are ambiguous or non-standard.
   */
  switch (archive_entry_filetype(entry)) {
  case AE_IFREG:
    dest->st_mode = (dest->st_mode & ~S_IFMT) | S_IFREG;
    break;
  case AE_IFDIR:
    dest->st_mode = (dest->st_mode & ~S_IFMT) | S_IFDIR;
    break;
  case AE_IFLNK:
    dest->st_mode = (dest->st_mode & ~S_IFMT) | S_IFLNK;
    break;
  default:
    /* Keep existing mode for sockets, block devices, etc. */
    break;
  }
}

/*
 * Dispatcher function to read file tree from archive using libarchive.
 * This replaces all old ReadTreeFrom... and GetStatFrom... functions.
 *
 * Now accepts DirEntry **dir_entry_ptr to allow updating the root pointer
 * if MinimizeArchiveTree swaps it.
 */
int ReadTreeFromArchive(ViewContext *ctx, DirEntry **dir_entry_ptr,
                        const char *filename, Statistic *s,
                        ScanProgressCallback cb, void *cb_data) {
  struct archive *a;
  struct archive_entry *entry;
  int r;
  char path_buffer[PATH_LENGTH * 2]; /* Buffer for path + symlink target */
  struct stat stat_buf;
  int count = 0;
  const char *clean_path;
  DirEntry *dir_entry = *dir_entry_ptr;

  /* Removed: *dir_entry->name = '\0'; to preserve name set by  LogDisk */

  a = archive_read_new();
  if (a == NULL) {
    MESSAGE(ctx, "archive_read_new() failed");
    return -1;
  }

  archive_read_support_filter_all(a);
  archive_read_support_format_all(a);

  r = archive_read_open_filename(a, filename, 10240);
  if (r != ARCHIVE_OK) {
    MESSAGE(ctx, "Could not open archive*'%s'*%s", filename,
            archive_error_string(a));
    archive_read_free(a);
    return -1;
  }

  while (archive_read_next_header(a, &entry) == ARCHIVE_OK) {
    const char *pathname = archive_entry_pathname(entry);

    if (pathname == NULL) {
      continue;
    }

    /* Normalize path: strip leading "./" if present to avoid confusion in
     * Fnsplit */
    if (pathname[0] == '.' && pathname[1] == FILE_SEPARATOR_CHAR) {
      clean_path = pathname + 2;
    } else {
      clean_path = pathname;
    }

    /* Strip leading separator if present to ensure internal path relative to
     * root */
    while (*clean_path == FILE_SEPARATOR_CHAR) {
      clean_path++;
    }

    /* Skip if the path is empty or just "." (root) */
    if (*clean_path == '\0' || (strcmp(clean_path, ".") == 0)) {
      continue;
    }

    copy_stat_from_entry(&stat_buf, entry);

    if (S_ISDIR(stat_buf.st_mode)) {
      /* Safer string copy */
      snprintf(path_buffer, sizeof(path_buffer), "%s", clean_path);
      /* Ensure directory paths end with a separator for
       * TryInsertArchiveDirEntry */
      size_t len = strlen(path_buffer);
      if (len > 0 && path_buffer[len - 1] != FILE_SEPARATOR_CHAR) {
        if (len < sizeof(path_buffer) - 2) {
          strcat(path_buffer, FILE_SEPARATOR_STRING);
        }
      }
      (void)TryInsertArchiveDirEntry(ctx, dir_entry, path_buffer, &stat_buf, s);

    } else if (S_ISREG(stat_buf.st_mode) || S_ISLNK(stat_buf.st_mode)) {
      /* Safer string copy */
      snprintf(path_buffer, sizeof(path_buffer), "%s", clean_path);

      if (S_ISLNK(stat_buf.st_mode)) {
        const char *link_target = archive_entry_symlink(entry);
        if (link_target) {
          /* Append symlink target after the null terminator of the path */
          size_t len = strlen(path_buffer);
          if (len + 1 + strlen(link_target) < sizeof(path_buffer)) {
            strcpy(&path_buffer[len + 1], link_target);
          }
        }
      }
      (void)InsertArchiveFileEntry(ctx, dir_entry, path_buffer, &stat_buf, s);
    }

    if (KeyPressed()) {
      Quit(ctx);
    }

    /* Update statistics / animation every 20 files */
    if ((count++ % 20) == 0) {
      if (cb)
        cb(ctx, cb_data);
    }
  }

  /* Pass the double pointer so it can update the root if needed */
  MinimizeArchiveTree(dir_entry_ptr, s);

  archive_read_free(a);

  /* Important: Recalculate visibility based on current settings immediately
   * after load */
  RecalculateSysStats(ctx, s);

  return 0;
}
