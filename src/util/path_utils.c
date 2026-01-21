/***************************************************************************
 *
 * src/util/path_utils.c
 * Path construction and manipulation functions
 *
 ***************************************************************************/


#include "ytree.h"
#include <stdlib.h>
#include <string.h>


char *GetPath(DirEntry *dir_entry, char *buffer)
{
  char *components[256];
  int i, depth = 0;
  DirEntry *de_ptr;

  *buffer = '\0';
  if (dir_entry == NULL) {
    return buffer;
  }

  /* Collect path components in reverse order (from leaf to root) */
  for (de_ptr = dir_entry; de_ptr != NULL && depth < 256; de_ptr = de_ptr->up_tree) {
    components[depth++] = de_ptr->name;
  }

  if (depth > 0) {
    /* The last component collected is the root. Start the path with it. */
    strcpy(buffer, components[depth - 1]);

    /* Append the rest of the components in correct order */
    for (i = depth - 2; i >= 0; i--) {
      /* Add separator if the current path is not just the root "/" */
      if (strcmp(buffer, FILE_SEPARATOR_STRING) != 0) {
        strcat(buffer, FILE_SEPARATOR_STRING);
      }
      strcat(buffer, components[i]);
    }
  }
  return buffer;
}


char *GetFileNamePath(FileEntry *file_entry, char *buffer)
{
  (void) GetPath( file_entry->dir_entry, buffer );
  if( *buffer && strcmp( buffer, FILE_SEPARATOR_STRING ) )
    (void) strcat( buffer, FILE_SEPARATOR_STRING );
  return( strcat( buffer, file_entry->name ) );
}


char *GetRealFileNamePath(FileEntry *file_entry, char *buffer)
{
  char *sym_name;

  if( mode == DISK_MODE || mode == USER_MODE )
    return( GetFileNamePath( file_entry, buffer ) );

  if( S_ISLNK( file_entry->stat_struct.st_mode ) )
  {
    sym_name = &file_entry->name[ strlen( file_entry->name ) + 1 ];
    if( *sym_name == FILE_SEPARATOR_CHAR )
      return( strcpy( buffer, sym_name ) );
  }

  (void) GetPath( file_entry->dir_entry, buffer );
  if( *buffer && strcmp( buffer, FILE_SEPARATOR_STRING ) )
    (void) strcat( buffer, FILE_SEPARATOR_STRING );
  if( S_ISLNK( file_entry->stat_struct.st_mode ) )
    return( strcat( buffer, &file_entry->name[ strlen( file_entry->name ) + 1 ] ) );
  else
    return( strcat( buffer, file_entry->name ) );
}


/* Aufsplitten des Dateinamens in die einzelnen Komponenten */
void Fnsplit(char *path, char *dir, char *name)
{
  int  i;
  char *name_begin;
  char *trunc_name;
  int dir_len = 0;

  while( *path == ' ' || *path == '\t' ) path++;

  while( strchr(path, FILE_SEPARATOR_CHAR ) || strchr(path, '\\') )
  {
    if (dir_len < PATH_LENGTH) {
        *(dir++) = *(path++);
        dir_len++;
    } else {
        /* Skip remaining directory part if buffer is full */
        path++;
    }
  }

  *dir = '\0';

  name_begin = path;
  trunc_name = name;

  for(i=0; i < PATH_LENGTH && *path; i++ )
    *(name++) = *(path++);

  *name = '\0';

  if( i == PATH_LENGTH && *path )
  {
    (void) snprintf( message, MESSAGE_LENGTH, "filename too long:*%s*truncating to*%s",
		    name_begin, trunc_name
		  );
    WARNING( message );
  }
}


char *GetExtension(char *filename)
{
  char *cptr;

  cptr = strrchr(filename, '.');

  if(cptr == NULL) return "";

  if(cptr == filename) return "";
  /* filenames beginning with a dot are not an extension */

  return(cptr + 1);
}


void NormPath( char *in_path, char *out_path )
{
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
  char *token, *saveptr;
  int is_absolute = 0;
  int i;

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
      ERROR_MSG("strdup failed*ABORT");
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
  }

  for (i = 0; i < stack_top; i++) {
      if (i > 0 || (is_absolute && out_path[1] == '\0')) {
          /* Append separator if not empty absolute root or first relative */
          /* Logic:
             Absolute: "/" -> "/usr" (no sep needed if just root? No, root is length 1)
             If out_path is "/" (len 1), don't add separator, result "/usr"
             If out_path is "/usr" (len 4), add separator, result "/usr/bin"
           */
          if (strcmp(out_path, FILE_SEPARATOR_STRING) != 0 && out_path[0] != '\0') {
               strcat(out_path, FILE_SEPARATOR_STRING);
          }
      }
      /* Ensure absolute path handles root correctly */
      /* Actually:
         Abs: "/" + "usr" -> "/usr"
         Abs: "/usr" + "bin" -> "/usr/bin"
         Rel: "usr"
         Rel: "usr" + "bin" -> "usr/bin"
      */
       if (is_absolute && strcmp(out_path, FILE_SEPARATOR_STRING) == 0) {
           /* out_path is currently just "/" */
       } else if (i > 0 || (out_path[0] != '\0')) {
           strcat(out_path, FILE_SEPARATOR_STRING);
       }
       strcat(out_path, stack[i]);
  }
}
