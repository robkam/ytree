/***************************************************************************
 *
 * src/ui/file_tags.c
 * File Tagging & Walking Operations (Walk, Delete, Invert)
 *
 ***************************************************************************/

#define NO_YTREE_MACROS
#include "ytree.h"
#include "ytree_ui.h"

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

void FileTags_WalkTaggedFiles(ViewContext *ctx, int start_file, int cursor_pos,
                              int (*fkt)(ViewContext *, FileEntry *,
                                         WalkingPackage *),
                              WalkingPackage *walking_package) {
  FileEntry *fe_ptr;
  int i;
  int start_x = 0;
  int result = 0;
  BOOL maybe_change_x = FALSE;
  int height, width;
  int max_disp_files; /* Local — computed from window geometry */

  if (baudrate() >= QUICK_BAUD_RATE)
    typeahead(0);

  GetMaxYX(ctx->ctx_file_window, &height, &width);

  max_disp_files = height * GetPanelMaxColumn(ctx->active);

  for (i = 0; i < (int)ctx->active->file_count && result == 0; i++) {
    fe_ptr = ctx->active->file_entry_list[i].file;

    if (fe_ptr->tagged && fe_ptr->matching) {
      if (maybe_change_x == FALSE && i >= start_file &&
          i < start_file + max_disp_files) {
        /* Walk possible without scrolling */
        /*---------------------------*/
        DisplayFiles(ctx, ctx->active, fe_ptr->dir_entry, start_file, i,
                     start_x, ctx->ctx_file_window);
        cursor_pos = i - start_file;
      } else {
        /* Scrolling necessary */
        /*---------------*/

        start_file = MAX(0, i - max_disp_files + 1);
        cursor_pos = i - start_file;

        DisplayFiles(ctx, ctx->active, fe_ptr->dir_entry, start_file,
                     start_file + cursor_pos, start_x, ctx->ctx_file_window);
        maybe_change_x = FALSE;
      }

      if (fe_ptr->dir_entry->global_flag)
        DisplayGlobalFileParameter(ctx, fe_ptr);
      else
        DisplayFileParameter(ctx, fe_ptr);

      RefreshWindow(ctx->ctx_file_window);
      doupdate();
      result = fkt(ctx, fe_ptr, walking_package);

      /* Handle case where file was removed/moved away */
      if (walking_package->new_fe_ptr == NULL) {
        FileList_RemoveFileEntry(ctx, i);
        i--; /* Adjust index since we removed current element */
        maybe_change_x = TRUE;
      } else if (walking_package->new_fe_ptr != fe_ptr) {
        ctx->active->file_entry_list[i].file = walking_package->new_fe_ptr;
        FileList_ChangeFileEntry(ctx);
        max_disp_files = height * GetPanelMaxColumn(ctx->active);
        maybe_change_x = TRUE;
      }
    }
  }

  if (baudrate() >= QUICK_BAUD_RATE)
    typeahead(-1);
}

/*
 ExecuteCommand (*fkt) had its retval zeroed as found.
 ^S needs that value, so it was unzeroed. forloop below
 was modified to not care about retval instead?

 global flag for stop-on-error? does anybody want it?

 --crb3 12mar04
*/

void FileTags_SilentWalkTaggedFiles(ViewContext *ctx,
                                    int (*fkt)(ViewContext *, FileEntry *,
                                               WalkingPackage *, Statistic *),
                                    WalkingPackage *walking_package) {
  FileEntry *fe_ptr;
  int i;

  for (i = 0; i < (int)ctx->active->file_count; i++) {
    fe_ptr = ctx->active->file_entry_list[i].file;

    if (fe_ptr->tagged && fe_ptr->matching) {
      (void)fkt(ctx, fe_ptr, walking_package, &ctx->active->vol->vol_stats);
    }
  }
}

/*

SilentTagWalkTaggedFiles.
revision of above function to provide something like
XTG's <search>- [x] Decouple execution: `execute.c`, `system.c`, `pipe.c`.
- [x] Decouple remaining utilities (`filter.c`, `sort.c`, `hex.c`, `view.c`,
etc.).
- [x] Sanitize headers (`ytree_defs.h`, `ytree_fs.h`) for headless compilation.
- [ ] Address lint errors and final verification.
repeated calls can be used to pare down tags, each with a different
string, until only the intended target files are tagged.

ExecuteCommand must have its retval unzeroed.

--crb3 31dec03

*/

void FileTags_SilentTagWalkTaggedFiles(ViewContext *ctx,
                                       int (*fkt)(ViewContext *, FileEntry *,
                                                  WalkingPackage *,
                                                  Statistic *),
                                       WalkingPackage *walking_package) {
  FileEntry *fe_ptr;
  int i;
  int result = 0;

  for (i = 0; i < (int)ctx->active->file_count; i++) {
    fe_ptr = ctx->active->file_entry_list[i].file;

    if (fe_ptr->tagged && fe_ptr->matching) {
      result = fkt(ctx, fe_ptr, walking_package, &ctx->active->vol->vol_stats);

      if (result != 0) {
        fe_ptr->tagged = FALSE;
        /* Update Stats */
        fe_ptr->dir_entry->tagged_files--;
        fe_ptr->dir_entry->tagged_bytes -= fe_ptr->stat_struct.st_size;
        ctx->active->vol->vol_stats.disk_tagged_files--;
        ctx->active->vol->vol_stats.disk_tagged_bytes -=
            fe_ptr->stat_struct.st_size;
      }
    }
  }
}

BOOL FileTags_IsMatchingTaggedFiles(ViewContext *ctx) {
  FileEntry *fe_ptr;
  int i;

  for (i = 0; i < (int)ctx->active->file_count; i++) {
    fe_ptr = ctx->active->file_entry_list[i].file;

    if (fe_ptr->matching && fe_ptr->tagged)
      return (TRUE);
  }

  return (FALSE);
}

int FileTags_UI_DeleteTaggedFiles(ViewContext *ctx, int max_disp_files,
                                  Statistic *s) {
  FileEntry *fe_ptr;
  DirEntry *de_ptr;
  int i;
  int start_file;
  int cursor_pos;
  BOOL deleted;
  BOOL confirm_each = FALSE;
  int term;
  int start_x = 0;
  int result = 0;
  int tagged_count = 0;
  int override_mode = 0; /* Auto-override mode for read-only files */
  char prompt_buffer[MESSAGE_LENGTH];

  /* 1. Count tagged files */
  for (i = 0; i < (int)ctx->active->file_count; i++) {
    if (ctx->active->file_entry_list[i].file->tagged &&
        ctx->active->file_entry_list[i].file->matching) {
      tagged_count++;
    }
  }

  if (tagged_count == 0)
    return 0;

  /* 2. Prompt for Intent */
  (void)snprintf(prompt_buffer, sizeof(prompt_buffer),
                 "Delete %d tagged files (Y/N) ?", tagged_count);
  term = InputChoice(ctx, prompt_buffer, "YN\033");
  if (term != 'Y')
    return -1;

  /* 3. Prompt for Mode (Confirm Each?) */
  term =
      InputChoice(ctx, "Ask for confirmation for each file (Y/N) ? ", "YN\033");
  if (term == ESC)
    return -1;
  confirm_each = (term == 'Y') ? TRUE : FALSE;
  if (!confirm_each)
    override_mode = 1;

  if (baudrate() >= QUICK_BAUD_RATE)
    typeahead(0);

  for (i = 0; i < (int)ctx->active->file_count && result == 0;) {
    deleted = FALSE;

    fe_ptr = ctx->active->file_entry_list[i].file;
    de_ptr = fe_ptr->dir_entry;

    if (fe_ptr->tagged && fe_ptr->matching) {
      /* Spinner to indicate progress during bulk deletion */
      DrawSpinner(ctx);
      doupdate();
      start_file = MAX(0, i - max_disp_files + 1);
      cursor_pos = i - start_file;

      DisplayFiles(ctx, ctx->active, de_ptr, start_file,
                   start_file + cursor_pos, start_x, ctx->ctx_file_window);

      if (fe_ptr->dir_entry->global_flag)
        DisplayGlobalFileParameter(ctx, fe_ptr);
      else
        DisplayFileParameter(ctx, fe_ptr);

      RefreshWindow(ctx->ctx_file_window);
      doupdate();

      term = 'Y';
      if (confirm_each) {
        term = InputChoice(ctx, "Delete this file (Y/N) ? ", "YN\033");
      }

      if (term == ESC) {
        if (baudrate() >= QUICK_BAUD_RATE)
          typeahead(-1);
        result = -1;
        break;
      }

      if (term == 'Y') {
        /* Pass the override mode pointer to suppress read-only prompts if 'A'
         * was selected previously */
        if ((result = DeleteFile(ctx, fe_ptr, &override_mode, s,
                                 (ChoiceCallback)UI_ChoiceResolver)) == 0) {
          /* File was deleted */
          /*----------------------*/

          deleted = TRUE;

          if (de_ptr->global_flag)
            DisplayDiskStatistic(ctx, s);
          else
            DisplayDirStatistic(ctx, de_ptr, NULL, s); /* Updated call */

          DisplayAvailBytes(ctx, s);

          FileList_RemoveFileEntry(ctx, start_file + cursor_pos);
        }
      }
    }
    if (!deleted)
      i++;
  }
  if (baudrate() >= QUICK_BAUD_RATE)
    typeahead(-1);

  return (result);
}

void FileTags_HandleInvertTags(ViewContext *ctx, DirEntry *dir_entry,
                               Statistic *s) {
  FileEntry *fe;
  for (fe = dir_entry->file; fe; fe = fe->next) {
    if (fe->matching) {
      fe->tagged = !fe->tagged;
      if (fe->tagged) {
        s->disk_tagged_files++;
        dir_entry->tagged_files++;
      } else {
        s->disk_tagged_files--;
        dir_entry->tagged_files--;
      }
    }
  }
}
