/***************************************************************************
 *
 * src/cmd/move.c
 * Move files
 *
 ***************************************************************************/

#ifndef YTREE_H
#include "../../include/ytree.h"
#endif

#include "../../include/ytree_cmd.h"
#include "../../include/ytree_fs.h"
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

static int Move(ViewContext *ctx, char *to_path, char *from_path);

int MoveFile(ViewContext *ctx, FileEntry *fe_ptr, const char *to_file,
             DirEntry *dest_dir_entry, const char *to_dir_path,
             FileEntry **new_fe_ptr, int *dir_create_mode, int *overwrite_mode,
             ConflictCallback cb, ChoiceCallback choice_cb) {
  DirEntry *de_ptr;
  long long file_size;
  char from_path[PATH_LENGTH + 1];
  char to_path[PATH_LENGTH + 1];
  FileEntry *dest_file_entry;
  FileEntry *fen_ptr;
  struct stat stat_struct;
  int result;
  /* New variables for EnsureDirectoryExists */
  char abs_path[PATH_LENGTH + 1];
  char from_dir[PATH_LENGTH + 1];
  int force = 1; /* Force delete for overwrite */
  int conflict_res;
  int written;

  /* Context-Aware Variables */
  struct Volume *target_vol = NULL;
  DirEntry *target_tree = NULL;
  Statistic *target_stats_ptr = NULL;
  const Statistic *s = &ctx->active->vol->vol_stats;

  result = -1;
  *new_fe_ptr = NULL;
  de_ptr = fe_ptr->dir_entry;

  (void)GetPath(de_ptr, from_dir); /* Get clean source directory path */
  written = snprintf(from_path, sizeof(from_path), "%s%s%s", from_dir,
                     FILE_SEPARATOR_STRING, fe_ptr->name);
  if (written < 0 || (size_t)written >= sizeof(from_path)) {
    return -1;
  }

  /* Construct base destination path */
  written = snprintf(to_path, sizeof(to_path), "%s%s", to_dir_path,
                     FILE_SEPARATOR_STRING);
  if (written < 0 || (size_t)written >= sizeof(to_path)) {
    return -1;
  }

  /* Handle relative path: make absolute based on source directory */
  if (*to_path != FILE_SEPARATOR_CHAR) {
    written = snprintf(abs_path, sizeof(abs_path), "%s%s%s", from_dir,
                       FILE_SEPARATOR_STRING, to_path);
    if (written < 0 || (size_t)written >= sizeof(abs_path)) {
      return -1;
    }
    written = snprintf(to_path, sizeof(to_path), "%s", abs_path);
    if (written < 0 || (size_t)written >= sizeof(to_path)) {
      return -1;
    }
  }

  /* Identify Target Volume */
  target_vol = Volume_GetByPath(ctx, to_path);
  if (target_vol) {
    target_tree = target_vol->vol_stats.tree;
    target_stats_ptr = &target_vol->vol_stats;
  } else {
    /* Fallback to current if path matches current tree prefix, else NULL
     * (external) */
    if (strncmp(s->tree->name, to_path, strlen(s->tree->name)) == 0) {
      target_tree = s->tree;
      target_stats_ptr = s;
    } else {
      target_tree = NULL;
      target_stats_ptr = NULL;
    }
  }

  /* Ensure the destination directory exists */
  {
    BOOL created = FALSE;
    /* FIX: Pass &dest_dir_entry to update the pointer */
    if (EnsureDirectoryExists(ctx, to_path, target_tree, &created,
                              &dest_dir_entry, dir_create_mode,
                              choice_cb) == -1) {
      return -1;
    }
    /* if (created) refresh_dirwindow = TRUE; */
  }

  {
    size_t to_path_len = strlen(to_path);
    written =
        snprintf(to_path + to_path_len, sizeof(to_path) - to_path_len, "%s",
                 to_file);
    if (written < 0 ||
        (size_t)written >= (sizeof(to_path) - to_path_len)) {
      return -1;
    }
  }

  if (!strcmp(to_path, from_path)) {
    /* MESSAGE( "Can't move file into itself" ); */
    ESCAPE;
  }

  if (access(from_path, W_OK)) {
    /* MESSAGE( "Unmoveable file*\"%s\"*%s", from_path, strerror(errno) ); */
    ESCAPE;
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
          result = 0; /* Success code, but skipped */
          ESCAPE;
        }
      }

      (void)DeleteFile(
          ctx, dest_file_entry,
          (overwrite_mode && (*overwrite_mode == 1 || *overwrite_mode == 2))
              ? &force
              : NULL,
          target_stats_ptr ? target_stats_ptr : s, choice_cb);
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
          result = 0; /* Success code, but skipped */
          ESCAPE;
        }
      }

      if (unlink(to_path)) {
        /* MESSAGE( "Can't unlink*\"%s\"*%s", to_path, strerror(errno) ); */
        ESCAPE;
      }
    }
  }

  if (!Move(ctx, to_path, from_path)) {
    /* File moved */
    /*-----------*/

    /* Remove original from tree */
    /*---------------------------*/

    (void)RemoveFile(ctx, fe_ptr, s);

    if (dest_dir_entry) {
      if (STAT_(to_path, &stat_struct)) {
        /* ERROR_MSG( "Stat Failed*ABORT" ); */
        exit(1);
      }

      file_size = stat_struct.st_size;

      /* Update Total Stats for TARGET volume */
      dest_dir_entry->total_bytes += file_size;
      dest_dir_entry->total_files++;

      if (target_stats_ptr) {
        target_stats_ptr->disk_total_bytes += file_size;
        target_stats_ptr->disk_total_files++;
      }

      /* Create File Entry manually */
      /* FIX: Added +1 to allocation for null terminator */
      fen_ptr = (FileEntry *)xmalloc(sizeof(FileEntry) + strlen(to_file) + 1);

      (void)strcpy(fen_ptr->name, to_file);

      (void)memcpy(&fen_ptr->stat_struct, &stat_struct, sizeof(stat_struct));

      fen_ptr->dir_entry = dest_dir_entry;
      fen_ptr->tagged = FALSE;
      fen_ptr->matching =
          Match(fen_ptr, target_stats_ptr ? target_stats_ptr : s);

      /* Update Matching Stats for TARGET volume */
      if (fen_ptr->matching) {
        dest_dir_entry->matching_bytes += file_size;
        dest_dir_entry->matching_files++;
        if (target_stats_ptr) {
          target_stats_ptr->disk_matching_bytes += file_size;
          target_stats_ptr->disk_matching_files++;
        }
      }

      /* Link into list (Head) */
      fen_ptr->next = dest_dir_entry->file;
      fen_ptr->prev = NULL;
      if (dest_dir_entry->file)
        dest_dir_entry->file->prev = fen_ptr;
      dest_dir_entry->file = fen_ptr;
      *new_fe_ptr = fen_ptr;

      /* Force refresh if we modified the tree structure or contents */
      /* refresh_dirwindow = TRUE; */
    }

    (void)GetAvailBytes(&s->disk_space, s);

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

/* GetMoveParameter was used by the UI directly, but we will leave it or remove
it? For now just keep but comment out UI calls or leave it since it will be in
ctrl_file soon. Actually get_move_parameter should be in the UI. We can comment
it out here or leave it. Wait, if it uses UI_ReadString, it might still need
ytree_ui.h, which we removed. Let's comment out the whole GetMoveParameter
function because it belongs in UI */

static int Move(ViewContext *ctx, char *to_path, char *from_path) {
  /* Statistic *s = &ctx->active->vol->vol_stats; */ /* Not needed here unless
                                                     CopyFileContent needs it */
  /* CopyFileContent does take a Statistic *s now. We need access to it. */
  /* Since Move is static and called from MoveFile, we should pass it or use
   * global ctx->active->vol */
  const Statistic *s = &ctx->active->vol->vol_stats;

  if (!strcmp(to_path, from_path)) {
    /* MESSAGE( "Can't move file into itself" ); */
    return (-1);
  }

  /* Try atomic rename first */
  if (rename(from_path, to_path) == 0) {
    return 0;
  }

  /* Handle Cross-Device link error by copying and deleting */
  if (errno == EXDEV) {
    if (CopyFileContent(ctx, to_path, from_path, s) == 0) {
      if (unlink(from_path) == 0) {
        return 0;
      } else {
        /* MESSAGE( "Move failed during unlink*\"%s\"*%s", from_path,
         * strerror(errno) ); */
        return -1;
      }
    } else {
      /* Copy failed, error message already shown by CopyFileContent */
      return -1;
    }
  }

  /* Fallback for other errors (e.g. permission denied) */
  /* MESSAGE( "Can't move (rename) \"%s\"*to \"%s\"*%s", from_path, to_path,
   * strerror(errno) ); */
  return (-1);
}

int MoveTaggedFiles(ViewContext *ctx, FileEntry *fe_ptr,
                    WalkingPackage *walking_package) {
  int result = -1;
  char new_name[PATH_LENGTH + 1];

  if (BuildFilename(fe_ptr->name, walking_package->function_data.mv.to_file,
                    new_name) == 0)

  {
    if (*new_name == '\0') {
      /* MESSAGE( "Can't move file to*empty name" ); */
    } else {
      const char *to_path = walking_package->function_data.mv.to_path;
      DirEntry *dest_dir_entry =
          walking_package->function_data.mv.dest_dir_entry;
      FileEntry *new_fe_ptr;
      int *dir_create_mode = &walking_package->function_data.mv.dir_create_mode;
      int *overwrite_mode = &walking_package->function_data.mv.overwrite_mode;
      ChoiceCallback choice_cb =
          (ChoiceCallback)walking_package->function_data.mv.choice_cb;

      /* Since we decoupled, we assume the conflict callback is set in
       * walking_package. */
      ConflictCallback cb =
          (ConflictCallback)walking_package->function_data.mv.conflict_cb;

      result =
          MoveFile(ctx, fe_ptr, new_name, dest_dir_entry, to_path, &new_fe_ptr,
                   dir_create_mode, overwrite_mode, cb, choice_cb);
    }
  }

  return (result);
}
