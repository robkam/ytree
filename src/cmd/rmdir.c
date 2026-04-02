/***************************************************************************
 *
 * src/cmd/rmdir.c
 * Deleting directories
 *
 ***************************************************************************/

#include "ytree_cmd.h"
#include "ytree_fs.h"
#include <stdlib.h>
#include <sys/stat.h>
#include <unistd.h>

#if defined(S_IFLNK)
#define STAT_(a, b) lstat(a, b)
#else
#define STAT_(a, b) stat(a, b)
#endif

static int DeleteSubTree(ViewContext *ctx, DirEntry *dir_entry,
                         ChoiceCallback choice_cb);
static int DeleteSingleDirectory(ViewContext *ctx, DirEntry *dir_entry,
                                 ChoiceCallback choice_cb);

static int RmdirProgressCallback(int status, const char *msg, void *user_data) {
  (void)status;
  (void)msg;
  (void)user_data;
  return ARCHIVE_CB_CONTINUE;
}

int DeleteDirectory(ViewContext *ctx, DirEntry *dir_entry,
                    ChoiceCallback choice_cb) {
  char buffer[PATH_LENGTH + 1];
  int result = -1;

  /* operation mode check removed as we assume caller handles it or passes
   * is_archive */

  if (dir_entry == ctx->active->vol->vol_stats.tree) {
    return -1;
  }
#ifdef HAVE_LIBARCHIVE
  else if (ctx->active->vol->vol_stats.log_mode == ARCHIVE_MODE) {
    if (dir_entry->file || dir_entry->sub_tree) {
      return -1;
    } else if (choice_cb && choice_cb(ctx, "Delete this directory (Y/N) ? ",
                                      "YN\033") == 'Y') {
      GetPath(dir_entry, buffer);

      if (Archive_DeleteEntry(ctx->active->vol->vol_stats.log_path, buffer,
                              RmdirProgressCallback, NULL) == 0) {
        /* Success - The UI must handle refreshing */
        result = 0;
      }
    }
  }
#endif
  else if (dir_entry->file || dir_entry->sub_tree) {
    if (choice_cb && choice_cb(ctx, "Directory not empty, PRUNE ? (Y/N) ? ",
                               "YN\033") == 'Y') {
      if (dir_entry->sub_tree) {
        if (!ctx->hook_scan_subtree ||
            ctx->hook_scan_subtree(ctx, dir_entry, &ctx->active->vol->vol_stats)) {
          return -1;
        }
        if (DeleteSubTree(ctx, dir_entry->sub_tree, choice_cb)) {
          return -1;
        }
      }
      if (DeleteSingleDirectory(ctx, dir_entry, choice_cb)) {
        return -1;
      }
      return 0;
    }
  } else if (choice_cb && choice_cb(ctx, "Delete this directory (Y/N) ? ",
                                    "YN\033") == 'Y') {
    (void)GetPath(dir_entry, buffer);

    if (access(buffer, W_OK)) {
      return -1;
    } else if (rmdir(buffer)) {
      return -1;
    } else {
      /* Directory geloescht
       * ==> aus Baum loeschen
       */

      ctx->active->vol->vol_stats.disk_total_directories--;

      if (dir_entry->prev)
        dir_entry->prev->next = dir_entry->next;
      else
        dir_entry->up_tree->sub_tree = dir_entry->next;

      if (dir_entry->next)
        dir_entry->next->prev = dir_entry->prev;

      free(dir_entry);

      (void)GetAvailBytes(&ctx->active->vol->vol_stats.disk_space,
                          &ctx->active->vol->vol_stats);

      result = 0;
    }
  }

  return (result);
}

static int DeleteSubTree(ViewContext *ctx, DirEntry *dir_entry,
                         ChoiceCallback choice_cb) {
  int result = -1;
  DirEntry *de_ptr, *next_de_ptr;

  for (de_ptr = dir_entry; de_ptr; de_ptr = next_de_ptr) {
    next_de_ptr = de_ptr->next;

    if (de_ptr->sub_tree) {
      if (DeleteSubTree(ctx, de_ptr->sub_tree, choice_cb)) {
        return -1;
      }
    }
    if (DeleteSingleDirectory(ctx, de_ptr, choice_cb)) {
      return -1;
    }
  }

  result = 0;
  return (result);
}

static int DeleteSingleDirectory(ViewContext *ctx, DirEntry *dir_entry,
                                 ChoiceCallback choice_cb) {
  int result = -1;
  char buffer[PATH_LENGTH + 1];
  FileEntry *fe_ptr, *next_fe_ptr;
  int force = 1;

  (void)GetPath(dir_entry, buffer);

  if (access(buffer, W_OK)) {
    return -1;
  }

  for (fe_ptr = dir_entry->file; fe_ptr; fe_ptr = next_fe_ptr) {
    next_fe_ptr = fe_ptr->next;
    if (DeleteFile(ctx, fe_ptr, &force, &ctx->active->vol->vol_stats,
                   choice_cb)) {
      return -1;
    }
  }

  if (rmdir(buffer)) {
    return -1;
  }

  if (!dir_entry->up_tree->not_scanned)
    ctx->active->vol->vol_stats.disk_total_directories--;

  if (dir_entry->prev)
    dir_entry->prev->next = dir_entry->next;
  else
    dir_entry->up_tree->sub_tree = dir_entry->next;
  if (dir_entry->next)
    dir_entry->next->prev = dir_entry->prev;

  free(dir_entry);

  result = 0;
  return (result);
}
