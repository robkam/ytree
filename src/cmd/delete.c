/***************************************************************************
 *
 * src/cmd/delete.c
 * Deleting files / directories
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
#include <unistd.h>

#ifdef HAVE_LIBARCHIVE
#endif

/* Helper for Archive Callback */
static int ArchiveUICallback(int status, const char *msg, void *user_data) {
  (void)status;
  (void)msg;
  (void)user_data;
  return ARCHIVE_CB_CONTINUE;
}

int DeleteFile(ViewContext *ctx, FileEntry *fe_ptr, int *auto_override,
               Statistic *s, ChoiceCallback choice_cb) {
  char filepath[PATH_LENGTH + 1];
  char buffer[PATH_LENGTH + 1];
  int result;
  int term;

  (void)GetFileNamePath(fe_ptr, filepath);

/* Handle Archive Mode Deletion Hook */
#ifdef HAVE_LIBARCHIVE
  if (s->login_mode == ARCHIVE_MODE) {
    /* In archive mode, permissions are virtual. We skip access checks and
     * unlink. We rely on the Rewrite Engine to handle the deletion.
     */
    if (Archive_DeleteEntry(s->login_path, filepath, ArchiveUICallback, NULL) ==
        0) {
      /* Success. The archive container has been rewritten.
      /* Success. The archive container has been rewritten.
      * The UI must handle refreshing.
      */

      return 0;
    } else {
      return -1;
    }
  }
#endif

  if (!S_ISLNK(fe_ptr->stat_struct.st_mode)) {
    if (access(filepath, W_OK)) {
      if (access(filepath, F_OK)) {
        /* File does not exist ==> done */

        goto UNLINK_DONE;
      }

      if (auto_override && *auto_override == 1) {
        term = 'Y';
      } else {
        (void)snprintf(buffer, sizeof(buffer),
                       "overriding mode %04o for \"%s\" (Y/N/A) ? ",
                       fe_ptr->stat_struct.st_mode & 0777, fe_ptr->name);

        if (choice_cb) {
          term = choice_cb(ctx, buffer, "YNA\033");
        } else {
          term = 'N';
        }
      }

      if (term == 'A') {
        if (auto_override)
          *auto_override = 1;
        term = 'Y';
      }

      if (term != 'Y') {
        return -1;
      }
    }
  }

  if (unlink(filepath)) {
    return -1;
  }

UNLINK_DONE:

  /* Remove file record */
  /*----------------*/

  result = RemoveFile(ctx, fe_ptr, s);
  (void)GetAvailBytes(&s->disk_space, s);
  return (result);
}

int RemoveFile(ViewContext *ctx, FileEntry *fe_ptr, Statistic *s) {
  DirEntry *de_ptr;
  long long file_size;

  de_ptr = fe_ptr->dir_entry;

  file_size = fe_ptr->stat_struct.st_size;

  de_ptr->total_bytes -= file_size;
  de_ptr->total_files--;
  s->disk_total_bytes -= file_size;
  s->disk_total_files--;
  if (fe_ptr->matching) {
    de_ptr->matching_bytes -= file_size;
    de_ptr->matching_files--;
    s->disk_matching_bytes -= file_size;
    s->disk_matching_files--;
  }
  if (fe_ptr->tagged) {
    de_ptr->tagged_bytes -= file_size;
    de_ptr->tagged_files--;
    s->disk_tagged_bytes -= file_size;
    s->disk_tagged_files--;
  }

  /* Remove file record */
  /*----------------*/

  if (fe_ptr->next)
    fe_ptr->next->prev = fe_ptr->prev;
  if (fe_ptr->prev)
    fe_ptr->prev->next = fe_ptr->next;
  else
    de_ptr->file = fe_ptr->next;

  free((char *)fe_ptr);

  return (0);
}

int DeleteTaggedFiles(ViewContext *ctx, FileEntry *fe_ptr,
                      WalkingPackage *walking_package) {
  int result = -1;
  int *auto_override = &walking_package->function_data.del.auto_override;
  Statistic *s = walking_package->function_data.del.statistic_ptr
                     ? walking_package->function_data.del.statistic_ptr
                     : &ctx->active->vol->vol_stats;
  ChoiceCallback choice_cb =
      (ChoiceCallback)walking_package->function_data.del.choice_cb;

  result = DeleteFile(ctx, fe_ptr, auto_override, s, choice_cb);

  return (result);
}
