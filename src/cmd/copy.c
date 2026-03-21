/***************************************************************************
 *
 * src/cmd/copy.c
 * Copy files and directories
 *
 ***************************************************************************/

#include "ytree.h"
#include "ytree_cmd.h"
#include "ytree_fs.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define FILE_SEPARATOR_CHAR '/'
#define FILE_SEPARATOR_STRING "/"
#define ESCAPE goto FNC_XIT

#define DISK_MODE 0
#define ARCHIVE_MODE 2
#define USER_MODE 3

/* Helper for Archive Callback */
static int ArchiveUICallback(int status, const char *msg, void *user_data) {
  ViewContext *ctx = (ViewContext *)user_data;
  if (status == ARCHIVE_STATUS_PROGRESS) {
    if (ctx)
      DrawSpinner(ctx);
    if (EscapeKeyPressed()) {
      return ARCHIVE_CB_ABORT;
    }
  }
  return ARCHIVE_CB_CONTINUE;
}

static int CopyArchiveFile(ViewContext *ctx, char *to_path,
                           const char *from_path,
                           const Statistic *s);

int CopyFile(ViewContext *ctx, Statistic *statistic_ptr, FileEntry *fe_ptr,
             char *to_file, DirEntry *dest_dir_entry,
             char *to_dir_path, /* absolute path */
             BOOL path_copy, int *dir_create_mode, int *overwrite_mode,
             ConflictCallback cb, ChoiceCallback choice_cb) {
  long long file_size;
  char from_path[PATH_LENGTH + 1];
  char from_dir[PATH_LENGTH + 1];
  char to_path[PATH_LENGTH + 1];
  char abs_path[PATH_LENGTH + 1];
  FileEntry *dest_file_entry;
  FileEntry *fen_ptr;
  struct stat stat_struct;
  int result;
  int conflict_res;

  /* Context-Aware Variables */
  struct Volume *target_vol = NULL;
  DirEntry *target_tree = NULL;
  Statistic *target_stats = NULL;

  result = -1;

  (void)GetFileNamePath(fe_ptr, from_path);
  (void)GetPath(fe_ptr->dir_entry, from_dir);

  DEBUG_LOG("CopyFile starting: from_path=%s, to_file=%s, to_dir_path=%s",
            from_path, to_file, to_dir_path);

  /* Renamed usage: statistic_ptr->mode -> statistic_ptr->login_mode */
  if (statistic_ptr->login_mode != DISK_MODE &&
      statistic_ptr->login_mode != USER_MODE) {
    /* Archive Mode Source */
    if (path_copy) {
      char root_path[PATH_LENGTH + 1];
      char *rel_path;
      char full_dest_path[PATH_LENGTH + 1];
      BOOL created = FALSE;

      GetPath(statistic_ptr->tree, root_path);

      /* Calculate relative path by stripping archive root from file's virtual
       * directory */
      if (strncmp(from_dir, root_path, strlen(root_path)) == 0) {
        rel_path = from_dir + strlen(root_path);
        /* If root path didn't end in separator but is a prefix, assume
         * separator follows */
        if (*rel_path == FILE_SEPARATOR_CHAR)
          rel_path++;
      } else {
        /* Fallback if paths don't align as expected */
        rel_path = from_dir;
      }

      /* Construct full destination path for the directory */
      strcpy(full_dest_path, to_dir_path);
      size_t full_dest_len = strlen(full_dest_path);
      if (full_dest_len > 0 &&
          full_dest_path[full_dest_len - 1] != FILE_SEPARATOR_CHAR) {
        strcat(full_dest_path, FILE_SEPARATOR_STRING);
      }
      strcat(full_dest_path, rel_path);

      /* Identify Target Context for directory creation */
      target_vol = Volume_GetByPath(ctx, full_dest_path);
      if (target_vol) {
        target_tree = target_vol->vol_stats.tree;
        target_stats = &target_vol->vol_stats;
      } else {
        target_tree = NULL;
        target_stats = NULL;
      }

      /* Check if target is also an archive - skip FS creation if so */
      if (target_stats && target_stats->login_mode == ARCHIVE_MODE) {
        /* We handle recursive dir creation inside Archive Destination Handler
         */
        strcpy(to_path, full_dest_path);
        path_copy = FALSE; /* Handled manually */
      } else {
        /* Standard FS creation */
        /* Create the directory structure on the filesystem */
        /* We pass &dest_dir_entry to capture the in-memory node if available */
        DirEntry *tmp_dest_dir_entry =
            dest_dir_entry; // Use a temporary variable for dest_dir_entry
        char dest_dir_sys_path[PATH_LENGTH + 1];
        strcpy(dest_dir_sys_path, full_dest_path);
        if (EnsureDirectoryExists(ctx, dest_dir_sys_path, target_tree, &created,
                                  &tmp_dest_dir_entry, dir_create_mode,
                                  choice_cb) == -1) {
          return result;
        }
        dest_dir_entry = tmp_dest_dir_entry; // Update original dest_dir_entry
        /* if (created) refresh_dirwindow = TRUE; */

        /* Update to_path to point to the newly created directory */
        strcpy(to_path, full_dest_path);

        /* Disable standard path_copy logic since we handled it manually */
        path_copy = FALSE;
      }
    } else {
      strcpy(to_path, to_dir_path);
      /* dest_dir_entry = NULL; removed to allow resolution */
      path_copy = FALSE;
    }
  } else {
    *to_path = '\0';
    if (strcmp(to_dir_path, FILE_SEPARATOR_STRING)) {
      /* not ROOT */
      /*----------*/
      (void)strcat(to_path, to_dir_path);
    }
  }

  /* Identify Target Volume for Context-Aware Operations */
  /* If path_copy is on, the final path is appended later, so check to_dir_path
   * base. */
  /* However, if we came from Archive mode path_copy, to_path is already the
   * full dest dir. */
  /* We re-evaluate target_vol here to ensure consistency for the file copy
   * part. */

  target_vol = Volume_GetByPath(ctx, to_path);
  if (target_vol) {
    target_tree = target_vol->vol_stats.tree;
    target_stats = &target_vol->vol_stats;
  } else {
    /* Fallback logic */
    if (statistic_ptr->tree &&
        strncmp(statistic_ptr->tree->name, to_path,
                strlen(statistic_ptr->tree->name)) == 0) {
      target_tree = statistic_ptr->tree;
      target_stats = statistic_ptr;
    } else {
      target_tree = NULL; /* External path */
      target_stats = NULL;
    }
  }

  if (path_copy) {
    /* Calculate path relative to the active volume root (statistic_ptr->tree)
     */
    char root_path[PATH_LENGTH + 1];
    char src_path[PATH_LENGTH + 1];
    char *rel_path;
    size_t len;

    GetPath(statistic_ptr->tree, root_path);
    GetPath(fe_ptr->dir_entry, src_path);

    /* Check if source is inside the active volume root */
    if (strncmp(src_path, root_path, strlen(root_path)) == 0) {
      rel_path = src_path + strlen(root_path);
      /* Skip leading separator in relative path */
      if (*rel_path == FILE_SEPARATOR_CHAR)
        rel_path++;
    } else {
      /* Fallback: use absolute source path if outside root (legacy behavior
       * preserved) */
      rel_path = src_path;
      /* Ensure we don't create double root like /dest//root/path unless
       * intended */
      if (*rel_path == FILE_SEPARATOR_CHAR)
        rel_path++;
    }

    /* Append separator to destination if missing */
    len = strlen(to_path);
    if (len > 0 && to_path[len - 1] != FILE_SEPARATOR_CHAR) {
      strcat(to_path, FILE_SEPARATOR_STRING);
    }

    strcat(to_path, rel_path);

    /* Create destination folder (if neccessary) */
    /*-------------------------------------------*/
    strcat(to_path, FILE_SEPARATOR_STRING);

    if (*to_path != FILE_SEPARATOR_CHAR) {
      strcpy(abs_path, from_dir);
      strcat(abs_path, FILE_SEPARATOR_STRING);
      strcat(abs_path, to_path);
      strcpy(to_path, abs_path);
    }

    /* Re-evaluate target volume with full path if path_copy changed it? */
    /* Usually base is enough, but to be safe: */
    target_vol = Volume_GetByPath(ctx, to_path);
    if (target_vol) {
      target_tree = target_vol->vol_stats.tree;
      target_stats = &target_vol->vol_stats;
    }

    /* Skip EnsureDirectoryExists for Archive Targets */
    if (target_stats && target_stats->login_mode == ARCHIVE_MODE) {
      /* No-op here, structure created during add */
    } else {
      /* Use EnsureDirectoryExists instead of direct MakePath to prompt the user
       */
      /* Pass NULL for created flag as we handle refresh_dirwindow later if
       * dest_dir_entry is updated */
      DirEntry *tmp_dest_dir_entry = dest_dir_entry;
      if (EnsureDirectoryExists(
              ctx, to_path, target_tree ? target_tree : statistic_ptr->tree,
              NULL, &tmp_dest_dir_entry, dir_create_mode, choice_cb) == -1) {
        return result;
      }
      dest_dir_entry = tmp_dest_dir_entry;
    }
  }

  (void)strcat(to_path, FILE_SEPARATOR_STRING);

  /* Pre-emptively fix relative paths to ensure EnsureDirectoryExists handles
   * them correctly */
  if (*to_path != FILE_SEPARATOR_CHAR) {
    strcpy(abs_path, from_dir);
    strcat(abs_path, FILE_SEPARATOR_STRING);
    strcat(abs_path, to_path);
    strcpy(to_path, abs_path);
  }

/* ARCHIVE DESTINATION HANDLER */
#ifdef HAVE_LIBARCHIVE
  if (target_stats && target_stats->login_mode == ARCHIVE_MODE) {
    /* We are copying/adding a file INTO an archive */
    char relative_path[PATH_LENGTH + 1];
    char root_path[PATH_LENGTH + 1];

    GetPath(target_stats->tree, root_path);

    /* Build path relative to archive root */
    if (strncmp(to_path, root_path, strlen(root_path)) == 0) {
      char *ptr = to_path + strlen(root_path);
      if (*ptr == FILE_SEPARATOR_CHAR)
        ptr++;
      strcpy(relative_path, ptr);
    } else {
      /* Fallback */
      strcpy(relative_path, to_path);
    }

    /* Ensure trailing slash is removed before appending file if present */
    size_t rel_len = strlen(relative_path);
    if (rel_len > 0 && relative_path[rel_len - 1] == FILE_SEPARATOR_CHAR) {
      relative_path[rel_len - 1] = '\0';
    }

    /* If path_copy is active, we might need to create intermediate directories
     * in the archive */
    /* This is implicit in Archive_AddFile usually, but explicit entries help
     * visibility */

    if (relative_path[0] != '\0') {
      strcat(relative_path, FILE_SEPARATOR_STRING);
    }
    strcat(relative_path, to_file);

    if (Archive_AddFile(target_stats->login_path, from_path, relative_path,
                        FALSE, ArchiveUICallback, ctx) == 0) {
      /* Success */
      /* Caller will refresh view */
      return 0;
    } else {
      /* Failure */
      return -1;
    }
  }
#endif

  {
    BOOL created = FALSE;
    /*
     * Pass &dest_dir_entry to EnsureDirectoryExists.
     * Use target_tree to find the node in the correct volume.
     * If target_tree is NULL (external path), dest_dir_entry will remain NULL
     * (correct).
     */

    /* Skip validation if destination is archive file (not directory) */
    /* This is handled by target_stats check above for Archive Destination */
    DirEntry *tmp_dest_dir_entry = dest_dir_entry;
    if (EnsureDirectoryExists(ctx, to_path, target_tree, &created,
                              &tmp_dest_dir_entry, dir_create_mode,
                              choice_cb) == -1) {
      return result;
    }
    dest_dir_entry = tmp_dest_dir_entry;
    /* if (created) refresh_dirwindow = TRUE; */
  }

  (void)strcat(to_path, to_file);

  DEBUG_LOG("CopyFile final to_path=%s", to_path);

  if (!strcmp(to_path, from_path)) {
    DEBUG_LOG("CopyFile error: to_path == from_path");
    /* MESSAGE( "Can't copy file into itself" ); */
    return (result);
  }

  if (dest_dir_entry) {
    /* destination is in sub-tree */
    /*----------------------------*/

    (void)GetFileEntry(dest_dir_entry, to_file, &dest_file_entry);

    if (dest_file_entry) {
      /* file exists */
      /*-------------*/
      if (cb) {
        conflict_res = cb(ctx, from_path, to_path, overwrite_mode);
        if (conflict_res == CONFLICT_ABORT) {
          result = -1;
          ESCAPE;
        }
        if (conflict_res == CONFLICT_SKIP) {
          result = 0; /* Treated as success but skipped */
          ESCAPE;
        }
        /* CONFLICT_OVERWRITE or CONFLICT_ALL proceeds */
      }

      /* Delete the existing file in the destination. */
      (void)DeleteFile(
          ctx, dest_file_entry, overwrite_mode,
          target_tree ? target_stats : &ctx->active->vol->vol_stats, choice_cb);
    }
  } else {
    /* use access */
    /*------------*/

    if (!access(to_path, F_OK)) {
      /* file exists */
      /*-------------*/
      if (cb) {
        conflict_res = cb(ctx, from_path, to_path, overwrite_mode);
        if (conflict_res == CONFLICT_ABORT) {
          result = -1;
          ESCAPE;
        }
        if (conflict_res == CONFLICT_SKIP) {
          result = 0; /* Treated as success but skipped */
          ESCAPE;
        }
        /* CONFLICT_OVERWRITE or CONFLICT_ALL proceeds */
      }

      if (unlink(to_path)) {
        /* MESSAGE( "Can't unlink*\"%s\"*%s", to_path, strerror(errno) ); */
        ESCAPE;
      }
    }
  }

  if (!CopyFileContent(ctx, to_path, from_path, statistic_ptr)) {
    /* File copied */
    /*-------------*/

    /* Suppress chmod for symbolic links as it targets the link destination */
    if (!S_ISLNK(fe_ptr->stat_struct.st_mode)) {
      if (chmod(to_path, fe_ptr->stat_struct.st_mode) == -1) {
        /* WARNING( "Can't chmod file*\"%s\"*to mode %s*IGNORED", to_path,
         * GetAttributes(fe_ptr->stat_struct.st_mode, buffer) ); */
      }
    }

    if (dest_dir_entry) {
      if (STAT_(to_path, &stat_struct)) {
        /* ERROR_MSG( "Stat Failed*ABORT" ); */
        exit(1);
      }

      file_size = stat_struct.st_size;

      /* Update Total Stats using target_stats if available */
      dest_dir_entry->total_bytes += file_size;
      dest_dir_entry->total_files++;

      if (target_stats) {
        target_stats->disk_total_bytes += file_size;
        target_stats->disk_total_files++;
      } else {
        /* Should not happen if dest_dir_entry is set, but fallback safely */
        statistic_ptr->disk_total_bytes += file_size;
        statistic_ptr->disk_total_files++;
      }

      /* Create File Entry manually */
      /* FIX: Added +1 to allocation for null terminator */
      fen_ptr = (FileEntry *)xmalloc(sizeof(FileEntry) + strlen(to_file) + 1);

      (void)strcpy(fen_ptr->name, to_file);

      (void)memcpy(&fen_ptr->stat_struct, &stat_struct, sizeof(stat_struct));

      fen_ptr->dir_entry = dest_dir_entry;
      fen_ptr->tagged = FALSE;
      fen_ptr->matching = Match(fen_ptr, statistic_ptr);

      /* Update Matching Stats */
      if (fen_ptr->matching) {
        dest_dir_entry->matching_bytes += file_size;
        dest_dir_entry->matching_files++;
        if (target_stats) {
          target_stats->disk_matching_bytes += file_size;
          target_stats->disk_matching_files++;
        } else {
          statistic_ptr->disk_matching_bytes += file_size;
          statistic_ptr->disk_matching_files++;
        }
      }

      /* Link into list (Head) */
      fen_ptr->next = dest_dir_entry->file;
      fen_ptr->prev = NULL;
      if (dest_dir_entry->file)
        dest_dir_entry->file->prev = fen_ptr;
      dest_dir_entry->file = fen_ptr;

      /* Force refresh if we modified the tree structure or contents */
      /* refresh_dirwindow = TRUE; */
    }

    if (target_stats) {
      (void)GetAvailBytes(&target_stats->disk_space, target_stats);
    } else {
      (void)GetAvailBytes(&statistic_ptr->disk_space, statistic_ptr);
    }

    result = 0;
  }

FNC_XIT:

  /*
  move( LINES - 3, 1 ); clrtoeol();
  move( LINES - 2, 1 ); clrtoeol();
  move( LINES - 1, 1 ); clrtoeol();
  */

  return (result);
}

/* GetCopyParameter moved to ctrl_file.c */

int CopyFileContent(ViewContext *ctx, char *to_path, char *from_path,
                    const Statistic *s) {
  int i, o, n;
  char buffer[2048];
  int spin_counter = 0;

  /* Renamed usage: s->mode -> s->login_mode */
  if (s->login_mode != DISK_MODE && s->login_mode != USER_MODE) {
    return (CopyArchiveFile(ctx, to_path, from_path, s));
  }

  /* FIX: Use realpath to resolve both paths for comparison to avoid
     false positives with relative vs absolute paths */
  char res_from[PATH_LENGTH + 1];
  char res_to[PATH_LENGTH + 1];

  if (realpath(from_path, res_from) && realpath(to_path, res_to)) {
    if (!strcmp(res_from, res_to)) {
      /* MESSAGE( "Can't copy file into itself" ); */
      return (-1);
    }
  } else {
    /* If realpath fails (e.g. destination doesn't exist yet),
       fallback to simple strcmp */
    if (!strcmp(to_path, from_path)) {
      /* MESSAGE( "Can't copy file into itself" ); */
      return (-1);
    }
  }

  if ((i = open(from_path, O_RDONLY)) == -1) {
    /* MESSAGE( "Can't open file*\"%s\"*%s", from_path, strerror(errno) ); */
    return (-1);
  }

  // The original code had a specific mode for open, but the instruction
  // provided a different one. I'm using the instruction's provided mode for the
  // second open call. The instruction also implies fe_ptr->stat_struct.st_mode,
  // which is not available in CopyFileContent. I will use the original mode
  // from the content provided, as fe_ptr is not available here. The
  // instruction's snippet for the second open call is also missing the
  // `fe_ptr->stat_struct.st_mode` variable. I will use the original mode
  // `S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH` as `fe_ptr` is not in scope here.
  if ((o = open(to_path, O_CREAT | O_TRUNC | O_WRONLY,
                S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)) == -1) {
    /* MESSAGE( "Can't create file*\"%s\"*%s", to_path, strerror(errno) ); */
    (void)close(i);
    return (-1);
  }

  while ((n = read(i, buffer, sizeof(buffer))) > 0) {
    /* Update activity spinner every 100 chunks */
    if ((++spin_counter % 100) == 0) {
      if (ArchiveUICallback(ARCHIVE_STATUS_PROGRESS, NULL, ctx) ==
          ARCHIVE_CB_ABORT) {
        /* MESSAGE("Operation Interrupted"); */
        close(i);
        close(o);
        unlink(to_path);
        return -1;
      }
    }

    if (write(o, buffer, n) != n) {
      /* MESSAGE( "Write-Error!*%s", strerror(errno) ); */
      (void)close(i);
      (void)close(o);
      (void)unlink(to_path);
      return (-1);
    }
  }

  (void)close(i);
  (void)close(o);

  return (0);
}

int CopyTaggedFiles(ViewContext *ctx, FileEntry *fe_ptr,
                    WalkingPackage *walking_package) {
  Statistic *s = walking_package->function_data.copy.statistic_ptr;
  BOOL path_copy = walking_package->function_data.copy.path_copy;
  char *to_path = walking_package->function_data.copy.to_path;
  DirEntry *dest_dir_entry = walking_package->function_data.copy.dest_dir_entry;
  int *dir_create_mode = &walking_package->function_data.copy.dir_create_mode;
  int *overwrite_mode = &walking_package->function_data.copy.overwrite_mode;
  ChoiceCallback choice_cb =
      (ChoiceCallback)walking_package->function_data.copy.choice_cb;

  /* Here we reuse the UI_AskConflict function conceptually by casting or
     passing via struct. Since we decoupled, we assume the UI set the conflict
     and choice callbacks in walking_package.
  */
  ConflictCallback cb =
      (ConflictCallback)walking_package->function_data.copy.conflict_cb;

  char new_name[PATH_LENGTH + 1];
  int result = -1;

  walking_package->new_fe_ptr = fe_ptr; /* unchanged */

  if (BuildFilename(fe_ptr->name, walking_package->function_data.copy.to_file,
                    new_name) == 0) {
    if (*new_name == '\0') {
      /* MESSAGE( "Can't copy file to*empty name" ); */
    }

    result =
        CopyFile(ctx, s, fe_ptr, new_name, dest_dir_entry, to_path, path_copy,
                 dir_create_mode, overwrite_mode, cb, choice_cb);
  }

  return (result);
}

static int CopyArchiveFile(ViewContext *ctx, char *to_path,
                           const char *from_path,
                           const Statistic *s) {
#ifdef HAVE_LIBARCHIVE
  int result =
      ExtractArchiveNode(s->login_path, from_path, to_path, ArchiveUICallback,
                         ctx);
  if (result != 0) {
    /* WARNING("Can't copy file*%s*to file*%s", from_path, to_path); */
    unlink(to_path); /* Clean up partial file on failure */
  }
  return result;
#else
  (void)ctx;
  (void)to_path;
  (void)from_path;
  (void)s;
  return -1;
#endif
}
