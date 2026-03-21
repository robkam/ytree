/***************************************************************************
 *
 * src/util/path_utils.c
 * Path construction and manipulation functions
 *
 ***************************************************************************/

#include "ytree.h"
#include <libgen.h>
#include <stdlib.h>
#include <string.h>

char *GetPath(DirEntry *dir_entry, char *buffer) {
  char *components[256];
  int depth = 0;
  DirEntry *de_ptr;

  memset(components, 0, sizeof(components));
  *buffer = '\0';
  if (dir_entry == NULL) {
    return buffer;
  }

  /* Collect path components in reverse order (from leaf to root) */
  for (de_ptr = dir_entry; de_ptr != NULL && depth < 256;
       de_ptr = de_ptr->up_tree) {
    components[depth++] = de_ptr->name;
  }

  if (depth > 0 && components[depth - 1] != NULL) {
    /* The last component collected is the root. Start the path with it. */
    strcpy(buffer, components[depth - 1]);

    /* Append the rest of the components in correct order */
    for (int i = depth - 2; i >= 0; i--) {
      /* Add separator if the current path is not just the root "/" */
      if (strcmp(buffer, FILE_SEPARATOR_STRING) != 0) {
        strcat(buffer, FILE_SEPARATOR_STRING);
      }
      strcat(buffer, components[i]);
    }
  }
  return buffer;
}

char *GetFileNamePath(FileEntry *file_entry, char *buffer) {
  (void)GetPath(file_entry->dir_entry, buffer);
  if (*buffer && strcmp(buffer, FILE_SEPARATOR_STRING))
    (void)strcat(buffer, FILE_SEPARATOR_STRING);
  return (strcat(buffer, file_entry->name));
}

char *GetRealFileNamePath(FileEntry *file_entry, char *buffer, int view_mode) {
  const char *sym_name;

  if (view_mode == DISK_MODE || view_mode == USER_MODE)
    return (GetFileNamePath(file_entry, buffer));

  if (S_ISLNK(file_entry->stat_struct.st_mode)) {
    sym_name = &file_entry->name[strlen(file_entry->name) + 1];
    if (*sym_name == FILE_SEPARATOR_CHAR)
      return (strcpy(buffer, sym_name));
  }

  (void)GetPath(file_entry->dir_entry, buffer);
  if (*buffer && strcmp(buffer, FILE_SEPARATOR_STRING))
    (void)strcat(buffer, FILE_SEPARATOR_STRING);
  if (S_ISLNK(file_entry->stat_struct.st_mode))
    return (strcat(buffer, &file_entry->name[strlen(file_entry->name) + 1]));
  else
    return (strcat(buffer, file_entry->name));
}

/* Aufsplitten des Dateinamens in die einzelnen Komponenten */
void Fnsplit(char *path, char *dir, char *name) {
  char *path_copy_dir;
  char *path_copy_base;
  const char *dname;
  const char *bname;
  char *processed_path;
  int has_sep = 0;
  size_t len;

  /* Skip Whitespace */
  while (*path == ' ' || *path == '\t')
    path++;
  processed_path = path;

  /* Check for standard separator to distinguish file in current dir */
  if (strchr(processed_path, FILE_SEPARATOR_CHAR)) {
    has_sep = 1;
  }

  /* Preparation: create copies for dirname/basename */
  path_copy_dir = strdup(processed_path);
  path_copy_base = strdup(processed_path);

  if (!path_copy_dir || !path_copy_base) {
    if (path_copy_dir)
      free(path_copy_dir);
    if (path_copy_base)
      free(path_copy_base);
    UI_Message(NULL, "strdup failed in Fnsplit*ABORT");
    exit(1);
  }

  /* Execution */
  dname = dirname(path_copy_dir);
  bname = basename(path_copy_base);

  /* Directory Output (dir) */
  /* If original path had no separators (and dirname returned .): Write empty
   * string */
  if (!has_sep && strcmp(dname, ".") == 0) {
    *dir = '\0';
  } else {
    strncpy(dir, dname, PATH_LENGTH - 1);
    dir[PATH_LENGTH - 1] = '\0';

    /* Trailing Slash: If result is not root, append separator */
    if (strcmp(dir, FILE_SEPARATOR_STRING) != 0) {
      len = strlen(dir);
      if (len < PATH_LENGTH - 1) {
        dir[len] = FILE_SEPARATOR_CHAR;
        dir[len + 1] = '\0';
      }
    }
  }

  /* Filename Output (name) */
  len = strlen(bname);
  if (len >= PATH_LENGTH) {
    strncpy(name, bname, PATH_LENGTH - 1);
    name[PATH_LENGTH - 1] = '\0';

    UI_Warning(NULL, "filename too long:*%s*truncating to*%s", processed_path,
               name);
  } else {
    strcpy(name, bname);
  }

  /* Cleanup */
  free(path_copy_dir);
  free(path_copy_base);
}

const char *GetExtension(const char *filename) {
  const char *cptr;

  cptr = strrchr(filename, '.');

  if (cptr == NULL)
    return "";

  if (cptr == filename)
    return "";
  /* filenames beginning with a dot are not an extension */

  return (cptr + 1);
}

void NormPath(char *in_path, char *out_path) {
  /* Step 1: Try physical resolution using realpath (if file exists) */
  char *canonical = realpath(in_path, NULL);
  if (canonical != NULL) {
    /* Path exists on disk, use canonical absolute path */
    strncpy(out_path, canonical, PATH_LENGTH);
    out_path[PATH_LENGTH] = '\0';
    free(canonical);
    return;
  }

  /* Step 2: Virtual/Logical Normalization (for archives or new dirs) */
  /* Fallback implementation when realpath fails */

  char stack[256][256]; /* Fixed depth stack for components */
  int stack_top = 0;
  char *path_copy;
  const char *token;
  char *saveptr;
  int is_absolute = 0;

  if (in_path == NULL || *in_path == '\0') {
    strcpy(out_path, ".");
    return;
  }

  if (in_path[0] == FILE_SEPARATOR_CHAR) {
    is_absolute = 1;
  }

  /* Create a mutable copy for tokenization */
  path_copy = strdup(in_path);
  if (path_copy == NULL) {
    UI_Message(NULL, "strdup failed*ABORT");
    exit(1);
  }

  token = strtok_r(path_copy, FILE_SEPARATOR_STRING, &saveptr);
  while (token != NULL) {
    if (strcmp(token, ".") == 0) {
      /* Ignore current dir */
    } else if (strcmp(token, "..") == 0) {
      /* Pop parent dir if stack is not empty */
      if (stack_top > 0) {
        stack_top--;
      } else if (!is_absolute) {
        /* If relative path '..' goes above start, keep '..' */
        /* NOTE: simplified logic here usually just stops at top */
        /* Alternatively: strcpy(stack[stack_top++], ".."); */
      }
    } else {
      /* Push directory */
      if (stack_top < 256) {
        strncpy(stack[stack_top], token, 255);
        stack[stack_top][255] = '\0';
        stack_top++;
      }
    }
    token = strtok_r(NULL, FILE_SEPARATOR_STRING, &saveptr);
  }

  free(path_copy);

  /* Reconstruct Path */
  out_path[0] = '\0';
  if (is_absolute) {
    strcpy(out_path, FILE_SEPARATOR_STRING);
  } else if (stack_top == 0) {
    /* If relative and stack empty (e.g. "./."), result is "." */
    strcpy(out_path, ".");
    return;
  }

  for (int i = 0; i < stack_top; i++) {
    /* If not the first component or if absolute and we already have the root
       slash, add a separator BEFORE appending the next component. */
    if (i > 0 || (is_absolute && out_path[0] == FILE_SEPARATOR_CHAR &&
                  out_path[1] == '\0')) {
      /* If absolute and we are at the first component (i=0), we already have
         the slash in out_path. But if i > 0, we definitely need a slash. */
      if (i > 0) {
        strcat(out_path, FILE_SEPARATOR_STRING);
      }
    } else if (!is_absolute && i == 0) {
      /* First relative component, out_path is empty, nothing to do here */
    }
    strcat(out_path, stack[i]);
  }
}
