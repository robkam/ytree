/***************************************************************************
 *
 * src/cmd/mkdir.c
 * Creating directories
 *
 ***************************************************************************/

#include "ytree.h"
#include "ytree_cmd.h"
#include "ytree_fs.h"
#include <dirent.h>
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

static DirEntry *MakeDirEntry(const ViewContext *ctx, YtreePanel *panel,
                              DirEntry *father_dir_entry, const char *dir_name,
                              Statistic *s);

/* Helper for Archive Callback */
static int ArchiveUICallback(int status, const char *msg, void *user_data) {
  (void)status;
  (void)msg;
  (void)user_data;
  /* UI interactions removed for decoupling */
  return ARCHIVE_CB_CONTINUE;
}

int MakeDirectory(const ViewContext *ctx, YtreePanel *panel,
                  DirEntry *father_dir_entry, const char *dir_name,
                  Statistic *s) {
  int result = -1;

  if (!dir_name || !*dir_name)
    return -1;

  if (MakeDirEntry(ctx, panel, father_dir_entry, dir_name, s) != NULL) {
    result = 0;
  }

  return (result);
}

static DirEntry *MakeDirEntry(const ViewContext *ctx, YtreePanel *panel,
                              DirEntry *father_dir_entry, const char *dir_name,
                              Statistic *s) {
  DirEntry *den_ptr = NULL, *des_ptr;
  char parent_path[PATH_LENGTH + 1];
  char buffer[PATH_LENGTH + 1];
  struct stat stat_struct;

  /* Pre-check: Does a directory with this name (case-insensitive) already exist
   * in the tree? */
  for (des_ptr = father_dir_entry->sub_tree; des_ptr; des_ptr = des_ptr->next) {
    if (strcasecmp(des_ptr->name, dir_name) == 0) {
      /* Found it! Return the existing node */
      return des_ptr;
    }
  }

  (void)GetPath(father_dir_entry, parent_path);
  {
    int n = snprintf(buffer, sizeof(buffer), "%s%s%s", parent_path,
                     FILE_SEPARATOR_STRING, dir_name);
    if (n < 0 || n >= (int)sizeof(buffer))
      return NULL;
  }

/* ARCHIVE MODE HANDLER */
#ifdef HAVE_LIBARCHIVE
  if (panel && panel->vol && panel->vol->vol_stats.login_mode == ARCHIVE_MODE) {
    char root_path[PATH_LENGTH + 1];
    char relative_path[PATH_LENGTH + 1];
    char archive_path[PATH_LENGTH + 1];
    const char *separator = "";

    /* Get full path of parent directory in tree */
    GetPath(father_dir_entry, parent_path);
    /* Get path of the archive file itself */
    {
      int n = snprintf(root_path, sizeof(root_path), "%s",
                       panel->vol->vol_stats.login_path);
      if (n < 0 || n >= (int)sizeof(root_path)) {
        return NULL;
      }
    }

    /* Calculate internal parent path by stripping archive root */
    if (strcmp(parent_path, root_path) == 0) {
      /* Parent is root of archive */
      relative_path[0] = '\0';
    } else if (strncmp(parent_path, root_path, strlen(root_path)) == 0) {
      /* Parent is subdir */
      char *ptr = parent_path + strlen(root_path);
      if (*ptr == FILE_SEPARATOR_CHAR)
        ptr++;
      {
        int n = snprintf(relative_path, sizeof(relative_path), "%s", ptr);
        if (n < 0 || n >= (int)sizeof(relative_path)) {
          return NULL;
        }
      }
    } else {
      /* Fallback (should not happen if logic correct) */
      {
        int n = snprintf(relative_path, sizeof(relative_path), "%s", parent_path);
        if (n < 0 || n >= (int)sizeof(relative_path)) {
          return NULL;
        }
      }
    }

    /* Append new directory name */
    if (relative_path[0] != '\0' &&
        relative_path[strlen(relative_path) - 1] != FILE_SEPARATOR_CHAR) {
      separator = FILE_SEPARATOR_STRING;
    }
    {
      int n = snprintf(archive_path, sizeof(archive_path), "%s%s%s",
                       relative_path, separator, dir_name);
      if (n < 0 || n >= (int)sizeof(archive_path)) {
        return NULL;
      }
    }

    /* Add entry */
    if (Archive_AddFile(panel->vol->vol_stats.login_path, NULL, archive_path,
                        TRUE, ArchiveUICallback, NULL) == 0) {
      /* Success: Return a dummy non-NULL to indicate success.
      The auto-refresh will reload the tree.
      */
      /* The caller is responsible for updating the view, we just return safely
       */
      return father_dir_entry;
    } else {
      return NULL;
    }
  }
#endif

  if (mkdir(buffer, (S_IREAD | S_IWRITE | S_IEXEC | S_IRGRP | S_IWGRP |
                     S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH) &
                        ~ctx->user_umask)) {
    /* Modified Logic: Allow existing directories if they are valid. */
    if (errno == EEXIST) {
      if (STAT_(buffer, &stat_struct) == 0 && S_ISDIR(stat_struct.st_mode)) {
        /* It exists and is a directory. Fall through to creation logic. */
        goto CREATE_NODE;
      }
    }

    return NULL;
  } else {
  CREATE_NODE:
    /* Directory created
     * ==> link into tree
     */

    /* FIX: Added +1 to allocation for null terminator */
    den_ptr = (DirEntry *)xcalloc(1, sizeof(DirEntry) + strlen(dir_name) + 1);

    den_ptr->file = NULL;
    den_ptr->next = NULL;
    den_ptr->prev = NULL;
    den_ptr->sub_tree = NULL;
    den_ptr->total_bytes = 0L;
    den_ptr->matching_bytes = 0L;
    den_ptr->tagged_bytes = 0L;
    den_ptr->total_files = 0;
    den_ptr->matching_files = 0;
    den_ptr->tagged_files = 0;
    den_ptr->access_denied = FALSE;
    den_ptr->cursor_pos = 0;
    den_ptr->start_file = 0;
    den_ptr->global_flag = FALSE;
    den_ptr->login_flag = FALSE;
    den_ptr->big_window = FALSE;
    den_ptr->up_tree = father_dir_entry;
    den_ptr->not_scanned = FALSE;

    if (s)
      s->disk_total_directories++;

    (void)strcpy(den_ptr->name, dir_name);

    if (STAT_(buffer, &stat_struct)) {
      /* ERROR_MSG( "Stat Failed*ABORT" ); */
      exit(1);
    }

    (void)memcpy(&den_ptr->stat_struct, &stat_struct, sizeof(stat_struct));

    /* Sort by direct insertion */
    /*------------------------------------*/

    for (des_ptr = father_dir_entry->sub_tree; des_ptr;
         des_ptr = des_ptr->next) {
      if (strcmp(des_ptr->name, den_ptr->name) > 0) {
        /* des-element is larger */
        /*--------------------------*/

        den_ptr->next = des_ptr;
        den_ptr->prev = des_ptr->prev;
        if (des_ptr->prev)
          des_ptr->prev->next = den_ptr;
        else
          father_dir_entry->sub_tree = den_ptr;
        des_ptr->prev = den_ptr;
        break;
      }

      if (des_ptr->next == NULL) {
        /* End of list reached; ==> insert */
        /*----------------------------------------*/

        den_ptr->prev = des_ptr;
        den_ptr->next = des_ptr->next;
        des_ptr->next = den_ptr;
        break;
      }
    }

    if (father_dir_entry->sub_tree == NULL) {
      /* First element */
      /*----------------*/

      father_dir_entry->sub_tree = den_ptr;
      den_ptr->prev = NULL;
      den_ptr->next = NULL;
    }

    if (s)
      (void)GetAvailBytes(&s->disk_space, s);
  }

  return (den_ptr);
}

int MakePath(const ViewContext *ctx, DirEntry *tree, char *dir_path,
             DirEntry **dest_dir_entry) {
  DirEntry *de_ptr, *sde_ptr;
  char path[PATH_LENGTH + 1];
  char *token, *old;
  int n;
  int result = -1;
  char *search_start;

  /* Variables for external mkdir -p logic */
  char tmp[PATH_LENGTH + 1];
  char *p;
  size_t len;

  if (tree == NULL)
    goto CREATE_EXTERNAL;

  NormPath(dir_path, path);
  if (dest_dir_entry)
    *dest_dir_entry = NULL;

  n = strlen(tree->name);
  /*
   * Check if path matches tree root.
   * Special handling for root "/" to ensure "path inside tree" logic works
   * correctly.
   */
  if (strcmp(tree->name, FILE_SEPARATOR_STRING) == 0) {
    /* Tree is "/". Path must start with "/". */
    if (path[0] == FILE_SEPARATOR_CHAR) {
      de_ptr = tree;
      /* Tokenize starting from char 1 to skip leading slash */
      search_start = &path[1];
      goto SEARCH_TREE;
    }
  } else if (!strncmp(tree->name, path, n) &&
             (path[n] == FILE_SEPARATOR_CHAR || path[n] == '\0')) {
    /* Normal case: Path starts with tree root prefix */
    de_ptr = tree;
    search_start = &path[n];
    goto SEARCH_TREE;
  }

  /* Fallback: Destination not in current memory tree */
  goto CREATE_EXTERNAL;

SEARCH_TREE:
  /* Path is in the (Sub)-Tree */
  /*----------------------------------*/

  token = strtok_r(search_start, FILE_SEPARATOR_STRING, &old);
  while (token) {
    for (sde_ptr = de_ptr->sub_tree; sde_ptr; sde_ptr = sde_ptr->next) {
      /* FIX: Case-insensitive search for existing directories */
      if (!strcasecmp(sde_ptr->name, token)) {
        /* Subtree found */
        /*------------------*/

        de_ptr = sde_ptr;
        break;
      }
    }
    if (sde_ptr == NULL) {
      /* Subsequent directory does not exist */
      /*----------------------------------*/
      /* MakeDirEntry returns the new node (or existing one), or NULL on error
       */
      if ((de_ptr = MakeDirEntry(ctx, NULL, de_ptr, token, NULL)) == NULL) {
        return (result);
      }
    }
    token = strtok_r(NULL, FILE_SEPARATOR_STRING, &old);
  }
  if (dest_dir_entry)
    *dest_dir_entry = de_ptr;
  result = 0;
  return result;

CREATE_EXTERNAL:
  /* Robust mkdir -p implementation */
  snprintf(tmp, sizeof(tmp), "%s", dir_path);
  len = strlen(tmp);
  if (len > 0 && tmp[len - 1] == FILE_SEPARATOR_CHAR)
    tmp[len - 1] = 0;

  /* Handle root: start after the first slash if absolute path */
  p = tmp;
  if (*p == FILE_SEPARATOR_CHAR)
    p++;

  for (; *p; p++) {
    if (*p == FILE_SEPARATOR_CHAR) {
      *p = 0;
      if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
        /* Error handling: ignore intermediate failures if final works */
      }
      *p = FILE_SEPARATOR_CHAR;
    }
  }
  if (mkdir(tmp, 0755) != 0 && errno != EEXIST) {
    /* MESSAGE("Can't create directory*\"%s\"*%s", tmp, strerror(errno)); */
    return -1;
  }
  result = 0;

  return (result);
}

int EnsureDirectoryExists(ViewContext *ctx, char *dir_path, DirEntry *tree,
                          BOOL *created, DirEntry **result_ptr,
                          int *auto_create, ChoiceCallback choice_cb) {
  DIR *tmpdir;

  if (created)
    *created = FALSE;
  if (result_ptr)
    *result_ptr = NULL;

  /* Check if directory exists on disk */
  if ((tmpdir = opendir(dir_path)) == NULL) {
    /* If it doesn't exist, ask the user */
    if (errno == ENOENT) {
      int term;
      if (auto_create && *auto_create) {
        term = 'Y';
      } else {
        if (choice_cb) {
          term = choice_cb(ctx, "Directory does not exist; create (Y/N/A) ? ",
                           "YNA\033");
        } else {
          term = 'N'; /* Default to abort headless missing cb */
        }
      }

      if (term == 'A') {
        if (auto_create)
          *auto_create = 1;
        term = 'Y';
      }

      if (term == 'Y') {
        /* Proceed to create */
        if (created)
          *created = TRUE;
      } else {
        /* User said NO or Escaped */
        return -1;
      }
    } else {
      /* Some other error opening directory (e.g. permission) */
      /* MESSAGE("Error opening directory*\"%s\"*%s", dir_path,
       * strerror(errno)); */
      return -1;
    }
  } else {
    /* Exists on disk */
    closedir(tmpdir);
  }

  /*
   * Directory exists (or user wants to create it).
   * Call MakePath to resolve the DirEntry pointer.
   * MakePath will create the node in memory if it's missing (even if dir exists
   * on disk), thanks to the update in MakeDirEntry.
   */
  return MakePath(ctx, tree, dir_path, result_ptr);
}
