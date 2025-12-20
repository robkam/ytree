/***************************************************************************
 *
 * tree_utils.c
 * Functions for searching and navigating the in-memory directory tree
 *
 ***************************************************************************/


#include "ytree.h"


int GetDirEntry(DirEntry *tree,
                DirEntry *current_dir_entry,
                char *dir_path,
                DirEntry **dir_entry,
                char *to_path
	       )
{
  /* Increased buffer size to silence snprintf truncation warnings */
  char dest_path[PATH_LENGTH * 2 + 2];
  char resolved_path[PATH_LENGTH + 1];
  char current_path_str[PATH_LENGTH + 1];
  char *token, *old;
  DirEntry *de_ptr, *sde_ptr;
  int n;

  *dir_entry = NULL;
  *to_path   = '\0';

  /* Construct destination path based on input */
  if (*dir_path == FILE_SEPARATOR_CHAR) {
      /* Absolute path */
      strncpy(dest_path, dir_path, sizeof(dest_path) - 1);
      dest_path[sizeof(dest_path) - 1] = '\0';
  } else {
      /* Relative path: combine current directory + input */
      GetPath(current_dir_entry, current_path_str);
      snprintf(dest_path, sizeof(dest_path), "%s%c%s", current_path_str, FILE_SEPARATOR_CHAR, dir_path);
  }

  /* Resolve to absolute path using realpath */
  if (realpath(dest_path, resolved_path) == NULL) {
      /* Resolution failed */
      if (errno == ENOENT) {
          /* Path does not exist - this is valid for creating new directories.
           * We return -3 to indicate "not found in tree, but path syntax is valid"
           * and copy the intended absolute path to to_path. */

           /* Use constructed path as fallback since realpath failed */
           strncpy(to_path, dest_path, PATH_LENGTH);
           to_path[PATH_LENGTH] = '\0';
           return -3;
      } else {
          /* Other error (permission, etc.) */
          (void) snprintf(message, MESSAGE_LENGTH, "Path resolution failed*\"%s\"*%s", dest_path, strerror(errno));
          ERROR_MSG(message);
          return -1;
      }
  }

  /* Success: Path exists on disk */
  strncpy(to_path, resolved_path, PATH_LENGTH);
  to_path[PATH_LENGTH] = '\0';

  /* Now try to find this path in the in-memory tree */
  n = strlen( tree->name );
  if( !strcmp(tree->name, FILE_SEPARATOR_STRING) ||
      (!strncmp( tree->name, resolved_path, n )     &&
        ( resolved_path[n] == FILE_SEPARATOR_CHAR || resolved_path[n] == '\0' ) ) )
  {
    /* Path starts with the tree root, so it might be in the tree */

    de_ptr = tree;
    token = strtok_r( &resolved_path[n], FILE_SEPARATOR_STRING, &old );
    while( token )
    {
      for( sde_ptr = de_ptr->sub_tree; sde_ptr; sde_ptr = sde_ptr->next )
      {
        if( !strcmp( sde_ptr->name, token ) )
	{
	  /* Subtree found */
	  /*--------------*/

	  de_ptr = sde_ptr;
	  break;
	}
      }
      if( sde_ptr == NULL )
      {
	/* Subdirectory not found in memory tree */
	/* This implies the directory exists on disk (realpath succeeded) but not in memory. */
	return( -3 );
      }
      token = strtok_r( NULL, FILE_SEPARATOR_STRING, &old );
    }
    *dir_entry = de_ptr;
  }
  return( 0 );
}




int GetFileEntry(DirEntry *de_ptr, char *file_name, FileEntry **file_entry)
{
  FileEntry *fe_ptr;

  *file_entry = NULL;

  for( fe_ptr = de_ptr->file; fe_ptr; fe_ptr = fe_ptr->next )
  {
    if( !strcmp( fe_ptr->name, file_name ) )
    {
      /* Entry found */
      /*-------------*/

      *file_entry = fe_ptr;
      break;
    }
  }
  return( 0 );
}


void DeleteTree(DirEntry *tree) {
    DirEntry *next_dir;
    FileEntry *f_ptr, *next_f_ptr;

    while (tree) {
        /* 1. Recursively delete sub-directories (Depth-First) */
        if (tree->sub_tree) {
            DeleteTree(tree->sub_tree);
            tree->sub_tree = NULL;
        }

        /* 2. Delete files in this directory */
        for (f_ptr = tree->file; f_ptr; f_ptr = next_f_ptr) {
            next_f_ptr = f_ptr->next;
            free(f_ptr);
        }

        /* 3. Save next sibling before freeing current */
        next_dir = tree->next;

        /* 4. Free the directory entry */
        free(tree);

        /* 5. Move to next sibling */
        tree = next_dir;
    }
}