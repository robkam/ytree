/***************************************************************************
 *
 * src/fs/tree_utils.c
 * General Tree Functions and State Save/Restore
 *
 ***************************************************************************/

#include "ytree.h"

static void DeleteFileTree(FileEntry *fe_ptr);
static void DeleteSubTree(DirEntry *de_ptr);

void DeleteTree(DirEntry *tree) {
  if (tree) {
    DeleteSubTree(tree->sub_tree);
    DeleteFileTree(tree->file);
    free(tree);
  }
}

static void DeleteSubTree(DirEntry *de_ptr) {
  DirEntry *next_de_ptr;

  while (de_ptr) {
    next_de_ptr = de_ptr->next;
    DeleteSubTree(de_ptr->sub_tree);
    DeleteFileTree(de_ptr->file);
    free(de_ptr);
    de_ptr = next_de_ptr;
  }
}

static void DeleteFileTree(FileEntry *fe_ptr) {
  FileEntry *next_fe_ptr;

  while (fe_ptr) {
    next_fe_ptr = fe_ptr->next;
    free(fe_ptr);
    fe_ptr = next_fe_ptr;
  }
}

int GetDirEntry(const ViewContext *ctx, DirEntry *tree,
                DirEntry *current_dir_entry, const char *dir_path,
                DirEntry **dir_entry, char *to_path) {
  int result;
  char path[PATH_LENGTH + 1];

  *to_path = '\0';
  if (strcmp(dir_path, FILE_SEPARATOR_STRING)) {
    /* not ROOT */
    /*----------*/
    (void)strcat(to_path, dir_path);
  }

  if (*dir_path != FILE_SEPARATOR_CHAR) {
    /* relative path */
    /*---------------*/

    (void)GetPath(current_dir_entry, path);

    if (strcmp(path, FILE_SEPARATOR_STRING))
      (void)strcat(path, FILE_SEPARATOR_STRING);

    (void)strcat(path, dir_path);
    (void)strcpy(to_path, path);
  } else {
    /* absolute path */
    /*---------------*/

    (void)strcpy(path, dir_path);
  }

  /*
   * NormPath resolves .. and . components to create a canonical absolute path.
   * This handles cases like ../sibling or ./child correctly.
   */
  NormPath(path, to_path);

  if (MakePath(ctx, tree, to_path, dir_entry)) {
    if (errno == ENOENT) {
      return -3;
    }
    return -1;
  }

  result = 0;

  return (result);
}

int GetFileEntry(DirEntry *de_ptr, const char *file_name,
                 FileEntry **file_entry) {
  FileEntry *fe_ptr;
  int result = -1;

  *file_entry = NULL;

  for (fe_ptr = de_ptr->file; fe_ptr; fe_ptr = fe_ptr->next) {
    if (!strcmp(fe_ptr->name, file_name)) {
      *file_entry = fe_ptr;
      result = 0;
      break;
    }
  }

  return (result);
}

/* --- State Save/Restore Implementation --- */

/* Helper to add a path to a list */
static void AddPathToList(PathList **list, const char *path) {
  PathList *node = (PathList *)xmalloc(sizeof(PathList));
  node->path = xstrdup(path);
  node->next = *list;
  *list = node;
}

/* Helper to reverse a PathList (needed for Top-Down restoration) */
static void ReversePathList(PathList **head_ref) {
  PathList *prev = NULL;
  PathList *current = *head_ref;
  PathList *next = NULL;
  while (current != NULL) {
    next = current->next;
    current->next = prev;
    prev = current;
    current = next;
  }
  *head_ref = prev;
}

/* Recursive helper for SaveTreeState */
static void ScanAndSaveState(DirEntry *dir, PathList **expanded,
                             PathList **tagged) {
  FileEntry *fe;
  DirEntry *sub;
  char path[PATH_LENGTH + 1];

  if (!dir)
    return;

  /* If directory is scanned (has sub_tree or marked scanned) and NOT root */
  /* Note: We check !not_scanned. Root is always scanned effectively. */
  /* We save state if it has a sub_tree (meaning it was expanded) */
  if (dir->sub_tree) {
    GetPath(dir, path);
    AddPathToList(expanded, path);
  }

  /* Save tagged files */
  for (fe = dir->file; fe; fe = fe->next) {
    if (fe->tagged) {
      GetFileNamePath(fe, path);
      AddPathToList(tagged, path);
    }
  }

  /* Recurse into subdirectories */
  for (sub = dir->sub_tree; sub; sub = sub->next) {
    ScanAndSaveState(sub, expanded, tagged);
  }
}

void SaveTreeState(DirEntry *root, PathList **expanded, PathList **tagged) {
  *expanded = NULL;
  *tagged = NULL;
  ScanAndSaveState(root, expanded, tagged);
}

void FreePathList(PathList *list) {
  PathList *next;
  while (list) {
    next = list->next;
    if (list->path)
      free(list->path);
    free(list);
    list = next;
  }
}

/*
 * FindOrLoadDir
 * Navigates the tree according to 'path'.
 * If a component is missing in memory, it calls ReadTree on the parent to
 * refresh children from disk. Returns the DirEntry if found, NULL otherwise.
 */
static DirEntry *FindOrLoadDir(ViewContext *ctx, DirEntry *tree,
                               const char *path, Statistic *s) {
  char *path_copy;
  const char *token;
  char *saveptr;
  DirEntry *current = tree;
  DirEntry *child;
  char full_path_buf[PATH_LENGTH +
                     1]; /* Buffer to construct path for ReadTree */

  if (tree == NULL)
    return NULL;

  if (!path || *path == '\0')
    return tree;

  /* Special case for root match */
  char root_path[PATH_LENGTH + 1];
  GetPath(tree, root_path);
  if (strcmp(path, root_path) == 0)
    return tree;

  /*
   * We need to handle absolute paths.
   * Use simple logic: Verify prefix matches root.
   * Then traverse relative components.
   */
  size_t root_len = strlen(root_path);
  const char *rel_path = path;

  if (strncmp(path, root_path, root_len) == 0) {
    rel_path = path + root_len;
    /* Handle trailing slash or separator on root */
    if (root_len > 1 && root_path[root_len - 1] == FILE_SEPARATOR_CHAR) {
      /* Root is like "/mnt/", path is "/mnt/a". rel is "a". Correct. */
    } else if (*rel_path == FILE_SEPARATOR_CHAR) {
      rel_path++;
    }
  } else {
    /* Path outside tree? Should not happen in restoration logic. */
    return NULL;
  }

  if (*rel_path == '\0')
    return tree;

  path_copy = xstrdup(rel_path);

  token = strtok_r(path_copy, FILE_SEPARATOR_STRING, &saveptr);
  while (token) {
    int found = 0;
    if (current == NULL) {
      free(path_copy);
      return NULL;
    }
    /* Search children */
    for (child = current->sub_tree; child; child = child->next) {
      if (strcmp(child->name, token) == 0) {
        current = child;
        found = 1;
        break;
      }
    }

    if (!found) {
      /* Child not in memory. Try to load children of current node from disk. */
      GetPath(current, full_path_buf);
      /* depth=0 loads immediate children */
      /* Pass NULL for callback and callback data (no UI feedback needed here)
       */
      if (ReadTree(ctx, current, full_path_buf, 0, s, NULL, NULL) == 0) {
        /* Apply filter to newly loaded files */
        ApplyFilter(current, s);

        /* Search again */
        for (child = current->sub_tree; child; child = child->next) {
          if (strcmp(child->name, token) == 0) {
            current = child;
            found = 1;
            break;
          }
        }
      }
    }

    if (!found) {
      free(path_copy);
      return NULL; /* Directory physically missing or unreadable */
    }

    token = strtok_r(NULL, FILE_SEPARATOR_STRING, &saveptr);
  }

  free(path_copy);
  return current;
}

void RestoreTreeState(ViewContext *ctx, DirEntry *root, PathList **expanded,
                      PathList *tagged, Statistic *s) {
  PathList *curr;
  DirEntry *de;
  FileEntry *fe;
  char dir_path[PATH_LENGTH + 1];
  char file_name[PATH_LENGTH + 1];

  /* 1. Restore Expanded Directories */
  /* IMPORTANT: Process Top-Down to ensure parents are scanned before children.
   */
  /* Reverse the list (which was built Bottom-Up by recursion) */
  ReversePathList(expanded);

  for (curr = *expanded; curr; curr = curr->next) {
    /*
     * Find the directory node using safe traversal.
     * If nodes are missing (because RescanDir wiped them), FindOrLoadDir will
     * reload them.
     */
    de = FindOrLoadDir(ctx, root, curr->path, s);

    if (de) {
      /* Found the directory. Ensure it is expanded (scanned). */
      /* If it has no sub_tree, it might need scanning (unless it's truly
       * empty). */
      /* ReadTree checks depth. We force a read to ensure persistence of empty
       * folders or leaf nodes status. */
      if (de->sub_tree == NULL) {
        /* Pass NULL for callback and callback data (no UI feedback needed here)
         */
        ReadTree(ctx, de, curr->path, 0, s, NULL, NULL);
        ApplyFilter(de, s);
      }
    }
  }

  /* 2. Restore Tagged Files */
  /* Order matters less here, but directory must exist. */
  for (curr = tagged; curr; curr = curr->next) {
    Fnsplit(curr->path, dir_path, file_name);

    /* Find directory - should be in memory now if it was restored above */
    de = FindOrLoadDir(ctx, root, dir_path, s);

    if (de) {
      /* Find file in the already-loaded directory */
      if (GetFileEntry(de, file_name, &fe) == 0 && fe) {
        if (!fe->tagged) {
          fe->tagged = TRUE;
          /* Update stats */
          de->tagged_files++;
          de->tagged_bytes += fe->stat_struct.st_size;
          s->disk_tagged_files++;
          s->disk_tagged_bytes += fe->stat_struct.st_size;
        }
      }
    }
  }
}

void ApplyFilterToTree(DirEntry *dir_entry, Statistic *s) {
  DirEntry *de_ptr;

  if (!dir_entry)
    return;

  ApplyFilter(dir_entry, s);

  for (de_ptr = dir_entry->sub_tree; de_ptr; de_ptr = de_ptr->next) {
    ApplyFilterToTree(de_ptr, s);
  }
}
