/***************************************************************************
 *
 * src/cmd/rename.c
 * Renaming files/directories
 *
 ***************************************************************************/

#include "ytree.h"
#include "ytree_cmd.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef HAVE_LIBARCHIVE
#include "ytree_fs.h"
#endif

#define FILE_SEPARATOR_CHAR '/'
#define FILE_SEPARATOR_STRING "/"

#if defined(S_IFLNK)
#define STAT_(a, b) lstat(a, b)
#else
#define STAT_(a, b) stat(a, b)
#endif

static int ArchiveUICallback(int status, const char *msg, void *user_data) {
  (void)status;
  (void)msg;
  (void)user_data;
  return ARCHIVE_CB_CONTINUE;
}

static int RenameDirEntry(char *to_path, char *from_path);
static int RenameFileEntry(char *to_path, char *from_path);

int RenameDirectory(ViewContext *ctx, DirEntry *de_ptr, const char *new_name) {
  DirEntry *den_ptr;
  DirEntry *sde_ptr;
  DirEntry *ude_ptr;
  FileEntry *fe_ptr;
  char from_path[PATH_LENGTH + 1];
  char to_path[PATH_LENGTH + 1];
  struct stat stat_struct;
  int result;
  char *cptr;
  int len;

  result = -1;

  /* Get the full path of the directory to be renamed */
  (void)GetPath(de_ptr, from_path);

/* ARCHIVE MODE HANDLER */
#ifdef HAVE_LIBARCHIVE
  if (ctx->active->vol->vol_stats.login_mode == ARCHIVE_MODE) {
    if (Archive_RenameEntry(ctx->active->vol->vol_stats.login_path, from_path,
                            new_name, ArchiveUICallback, NULL) == 0) {
      return 0;
    }
    return -1;
  }
#endif

  /*
   * Safety Fix: Construct the new path using snprintf instead of modifying
   * the string in place with strcpy/strcat.
   */

  /* Find the parent directory by locating the last separator */
  cptr = strrchr(from_path, FILE_SEPARATOR_CHAR);

  if (!cptr) {
    return -1;
  }

  if (cptr == from_path) {
    /* Parent is root '/' */
    len = snprintf(to_path, sizeof(to_path), "%c%s", FILE_SEPARATOR_CHAR,
                   new_name);
  } else {
    /* Calculate length of parent directory string */
    int parent_len = cptr - from_path;
    /* Construct new path: parent_dir + separator + new_name */
    len = snprintf(to_path, sizeof(to_path), "%.*s%c%s", parent_len, from_path,
                   FILE_SEPARATOR_CHAR, new_name);
  }

  if (len >= (int)sizeof(to_path)) {
    return -1;
  }

  if (access(from_path, W_OK)) {
    return -1;
  }

  if (!RenameDirEntry(to_path, from_path)) {
    /* Rename successful */
    /*--------------------*/

    if (STAT_(to_path, &stat_struct)) {
      return -1;
    }

    /* FIX: Added +1 to allocation for null terminator */
    den_ptr = (DirEntry *)xmalloc(sizeof(DirEntry) + strlen(new_name) + 1);

    (void)memcpy(den_ptr, de_ptr, sizeof(DirEntry));

    (void)strcpy(den_ptr->name, new_name);

    (void)memcpy(&den_ptr->stat_struct, &stat_struct, sizeof(stat_struct));

    /* Link structure */
    /*---------------------*/

    if (den_ptr->prev)
      den_ptr->prev->next = den_ptr;
    if (den_ptr->next)
      den_ptr->next->prev = den_ptr;

    /* Subtree */
    /*---------*/

    for (sde_ptr = den_ptr->sub_tree; sde_ptr; sde_ptr = sde_ptr->next)
      sde_ptr->up_tree = den_ptr;

    /* Files */
    /*-------*/

    for (fe_ptr = den_ptr->file; fe_ptr; fe_ptr = fe_ptr->next)
      fe_ptr->dir_entry = den_ptr;

    /* Uptree */
    /*--------*/

    for (ude_ptr = den_ptr->up_tree; ude_ptr; ude_ptr = ude_ptr->next)
      if (ude_ptr->sub_tree == de_ptr)
        ude_ptr->sub_tree = den_ptr;

    /* Free old structure */
    /*-------------------------*/

    free(de_ptr);

    /* Warning: de_ptr is invalid from now on !!! */
    /*--------------------------------------------*/

    result = 0;
  }

  return (result);
}

int RenameFile(ViewContext *ctx, FileEntry *fe_ptr, const char *new_name,
               FileEntry **new_fe_ptr) {
  DirEntry *de_ptr;
  FileEntry *fen_ptr;
  char from_path[PATH_LENGTH + 1];
  char to_path[PATH_LENGTH + 1];
  char parent_path[PATH_LENGTH + 1];
  struct stat stat_struct;
  int result;
  int len;

  result = -1;

  *new_fe_ptr = fe_ptr;

  de_ptr = fe_ptr->dir_entry;

  (void)GetFileNamePath(fe_ptr, from_path);

/* ARCHIVE MODE HANDLER */
#ifdef HAVE_LIBARCHIVE
  if (ctx->active->vol->vol_stats.login_mode == ARCHIVE_MODE) {
    if (Archive_RenameEntry(ctx->active->vol->vol_stats.login_path, from_path,
                            new_name, ArchiveUICallback, NULL) == 0) {
      return 0;
    }
    return -1;
  }
#endif

  /* Safety Fix: Use snprintf to construct to_path */
  (void)GetPath(de_ptr, parent_path);

  /* Handle root path case correctly (avoid double slash if parent is "/") */
  if (strcmp(parent_path, FILE_SEPARATOR_STRING) == 0) {
    len = snprintf(to_path, sizeof(to_path), "%c%s", FILE_SEPARATOR_CHAR,
                   new_name);
  } else {
    len = snprintf(to_path, sizeof(to_path), "%s%c%s", parent_path,
                   FILE_SEPARATOR_CHAR, new_name);
  }

  if (len >= (int)sizeof(to_path)) {
    return -1;
  }

  if (access(from_path, W_OK)) {
    return -1;
  }

  if (!RenameFileEntry(to_path, from_path)) {
    /* Rename successful */
    /*--------------------*/

    if (STAT_(to_path, &stat_struct)) {
      return -1;
    }

    /* FIX: Added +1 to allocation for null terminator */
    fen_ptr = (FileEntry *)xmalloc(sizeof(FileEntry) + strlen(new_name) + 1);

    (void)memcpy(fen_ptr, fe_ptr, sizeof(FileEntry));

    (void)strcpy(fen_ptr->name, new_name);

    (void)memcpy(&fen_ptr->stat_struct, &stat_struct, sizeof(stat_struct));

    /* Link structure */
    /*---------------------*/

    if (fen_ptr->prev)
      fen_ptr->prev->next = fen_ptr;
    if (fen_ptr->next)
      fen_ptr->next->prev = fen_ptr;
    if (fen_ptr->dir_entry->file == fe_ptr)
      fen_ptr->dir_entry->file = fen_ptr;

    /* Free old structure */
    /*-------------------------*/

    free(fe_ptr);

    /* Warning: fe_ptr is invalid from now on !!! */
    /*--------------------------------------------*/

    result = 0;

    *new_fe_ptr = fen_ptr;
  }

  return (result);
}

/* GetRenameParameter moved to UI layer */

static int RenameDirEntry(char *to_path, char *from_path) {
  struct stat fdstat;

  if (!strcmp(to_path, from_path)) {
    return (0);
  }

  if (stat(to_path, &fdstat) == 0) {
    return (-1);
  }

  /*
   * Modernized: Always use rename().
   * Removed obsolete link()/unlink() fallback which fails on directories.
   */
  if (rename(from_path, to_path)) {
    return (-1);
  }

  return (0);
}

static int RenameFileEntry(char *to_path, char *from_path) {
  if (!strcmp(to_path, from_path)) {
    return (-1);
  }

  /*
   * Modernized: Use rename() instead of link()/unlink().
   * rename() is atomic and safer.
   */
  if (rename(from_path, to_path)) {
    return (-1);
  }

  return (0);
}

int RenameTaggedFiles(ViewContext *ctx, FileEntry *fe_ptr,
                      WalkingPackage *walking_package) {
  int result = -1;
  char new_name[PATH_LENGTH + 1];

  if (BuildFilename(fe_ptr->name,
                    walking_package->function_data.rename.new_name,
                    new_name) == 0) {
    if (*new_name == '\0') {
      return -1;
    } else {
      result = RenameFile(ctx, fe_ptr, new_name, &walking_package->new_fe_ptr);
    }
  }
  return (result);
}