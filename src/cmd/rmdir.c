/***************************************************************************
 *
 * src/cmd/rmdir.c
 * Deleting directories
 *
 ***************************************************************************/

#include "ytnova_cmd.h"
#include "ytnova_fs.h"
#include <limits.h>
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

static int RmdirProgressCallback(int status, const char *msg,
                                 long long bytes_delta,
                                 unsigned int items_delta, void *user_data) {
  ViewContext *ctx = (ViewContext *)user_data;

  if (ctx && ctx->hook_archive_callback)
    return ctx->hook_archive_callback(status, msg, bytes_delta, items_delta,
                                      user_data);
  return ARCHIVE_CB_CONTINUE;
}

static int CountDirectoryTree(const DirEntry *dir_entry) {
  const DirEntry *child;
  int count = 1;

  for (child = dir_entry ? dir_entry->sub_tree : NULL; child != NULL;
       child = child->next)
    count += CountDirectoryTree(child);
  return count;
}

static unsigned int CountDirectoryItems(const DirEntry *dir_entry) {
  const DirEntry *child;
  const FileEntry *file;
  unsigned int count = 1;

  if (!dir_entry)
    return 0;
  for (file = dir_entry->file; file; file = file->next) {
    if (count == UINT_MAX)
      return UINT_MAX;
    count++;
  }
  for (child = dir_entry->sub_tree; child; child = child->next) {
    unsigned int child_count = CountDirectoryItems(child);

    if (child_count > UINT_MAX - count)
      return UINT_MAX;
    count += child_count;
  }
  return count;
}

static long long CountDirectoryBytes(const DirEntry *dir_entry) {
  const DirEntry *child;
  const FileEntry *file;
  long long bytes = 0;

  if (!dir_entry)
    return 0;
  for (file = dir_entry->file; file; file = file->next) {
    if (file->stat_struct.st_size < 0 ||
        bytes > LLONG_MAX - file->stat_struct.st_size)
      return -1;
    bytes += file->stat_struct.st_size;
  }
  for (child = dir_entry->sub_tree; child; child = child->next) {
    long long child_bytes = CountDirectoryBytes(child);

    if (child_bytes < 0 || bytes > LLONG_MAX - child_bytes)
      return -1;
    bytes += child_bytes;
  }
  return bytes;
}

int DeleteDirectory(ViewContext *ctx, DirEntry *dir_entry,
                    ChoiceCallback choice_cb) {
  char buffer[PATH_LENGTH + 1];
  int result = -1;

  /* Caller dispatch guarantees the correct archive-vs-disk mode here. */

  if (dir_entry == ctx->active->vol->vol_stats.tree) {
    return -1;
  }
#ifdef HAVE_LIBARCHIVE
  else if (ctx->active->vol->vol_stats.log_mode == ARCHIVE_MODE) {
    if (!(ctx->active->vol->vol_stats.archive_capabilities & ARCHIVE_CAP_DELETE))
      return -1;
    if (choice_cb && choice_cb(ctx, "Delete this directory (Y/N) ? ",
                               "YN\033") == 'Y') {
      BOOL owns_progress;
      int archive_result;
      unsigned int selected_items = CountDirectoryItems(dir_entry);

      RefreshView(ctx, dir_entry);
      GetPath(dir_entry, buffer);
      owns_progress = !ctx->progress.active && ctx->hook_progress_start &&
                      ctx->hook_progress_finish;
      if (owns_progress) {
        long long selected_bytes = CountDirectoryBytes(dir_entry);
        long long remaining_bytes =
            ctx->active->vol->vol_stats.disk_total_bytes;

        if (selected_bytes >= 0 && remaining_bytes >= selected_bytes)
          remaining_bytes -= selected_bytes;
        else
          remaining_bytes = 0;
        ctx->hook_progress_start(
            ctx, "ARCHIVE DELETE", buffer, "", remaining_bytes,
            ctx->active->vol->vol_stats.archive_member_count);
      }
      archive_result = Archive_DeleteTree(
          ctx->active->vol->vol_stats.log_path, buffer,
          RmdirProgressCallback, ctx);
      if (archive_result == 0) {
        if (dir_entry->prev)
          dir_entry->prev->next = dir_entry->next;
        else
          dir_entry->up_tree->sub_tree = dir_entry->next;

        if (dir_entry->next)
          dir_entry->next->prev = dir_entry->prev;

        ctx->active->vol->vol_stats.disk_total_directories -=
            CountDirectoryTree(dir_entry);
        if (selected_items <=
            ctx->active->vol->vol_stats.archive_member_count)
          ctx->active->vol->vol_stats.archive_member_count -= selected_items;
        else
          ctx->active->vol->vol_stats.archive_member_count = 0;
        DeleteTree(dir_entry);
        result = 0;
      }
      if (owns_progress)
        ctx->hook_progress_finish(ctx);
    }
  }
#endif
  else if (dir_entry->file || dir_entry->sub_tree) {
    if (choice_cb && choice_cb(ctx, "Directory not empty, PRUNE ? (Y/N) ? ",
                               "YN\033") == 'Y') {
      BOOL owns_progress;

      if (dir_entry->sub_tree) {
        if (!ctx->hook_scan_subtree ||
            ctx->hook_scan_subtree(ctx, dir_entry, &ctx->active->vol->vol_stats)) {
          return -1;
        }
      }
      owns_progress = !ctx->progress.active && ctx->hook_progress_start &&
                      ctx->hook_progress_finish;
      if (owns_progress) {
        GetPath(dir_entry, buffer);
        ctx->hook_progress_start(ctx, "DELETING", buffer, "", 0,
                                 CountDirectoryItems(dir_entry));
      }
      if (dir_entry->sub_tree) {
        if (DeleteSubTree(ctx, dir_entry->sub_tree, choice_cb)) {
          if (owns_progress)
            ctx->hook_progress_finish(ctx);
          return -1;
        }
      }
      if (DeleteSingleDirectory(ctx, dir_entry, choice_cb)) {
        if (owns_progress)
          ctx->hook_progress_finish(ctx);
        return -1;
      }
      if (owns_progress)
        ctx->hook_progress_finish(ctx);
      return 0;
    }
  } else if (choice_cb && choice_cb(ctx, "Delete this directory (Y/N) ? ",
                                    "YN\033") == 'Y') {
    (void)GetPath(dir_entry, buffer);

    if (rmdir(buffer)) {
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

  for (fe_ptr = dir_entry->file; fe_ptr; fe_ptr = next_fe_ptr) {
    next_fe_ptr = fe_ptr->next;
    if (DeleteFile(ctx, fe_ptr, &force, &ctx->active->vol->vol_stats,
                   choice_cb)) {
      return -1;
    }
    if (ctx->progress.active && ctx->progress.items_total > 0 &&
        ctx->hook_progress_update &&
        !ctx->hook_progress_update(ctx, 0, ctx->progress.items_done + 1))
      return -1;
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

  if (ctx->progress.active && ctx->progress.items_total > 0 &&
      ctx->hook_progress_update &&
      !ctx->hook_progress_update(ctx, 0, ctx->progress.items_done + 1))
    return -1;

  result = 0;
  return (result);
}
