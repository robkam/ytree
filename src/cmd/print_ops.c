/***************************************************************************
 *
 * src/cmd/print_ops.c
 * Write/Print Dialog and Formatting Logic
 *
 ***************************************************************************/

#include "ytree_cmd.h"
#include "ytree_fs.h"
#include <fcntl.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

/* Expand a leading ~/ to $HOME/. dest must be at least PATH_LENGTH+1 bytes. */
static void ExpandPath(const char *src, char *dest, size_t dest_size) {
  if (src[0] == '~' && (src[1] == '/' || src[1] == '\0' || src[1] == ' ')) {
    const char *home = getenv("HOME");
    if (!home || *home == '\0') {
      const struct passwd *pw = getpwuid(getuid());
      if (pw)
        home = pw->pw_dir;
    }
    if (home) {
      snprintf(dest, dest_size, "%s%s", home, src + 1);
      return;
    }
  }
  snprintf(dest, dest_size, "%s", src);
}

/* Write a single file to out_fp.
 * is_first / is_last control whether to emit the frame separator as a
 * between-files divider: separator appears AFTER each file except the last.
 * A heading (### filename) is always emitted for Framed mode.
 */
static int PrintFileContent(const ViewContext *ctx, FileEntry *file_entry,
                            FILE *out_fp, const PrintConfig *config, BOOL is_first,
                            BOOL is_last) {
  char file_name_path[PATH_LENGTH + 1];
  int in_fd;
  char buffer[4096];
  ssize_t bytes_read;
  int result = 0;

  (void)is_first; /* Reserved for future use (e.g., per-file preamble) */

  (void)GetRealFileNamePath(file_entry, file_name_path, ctx->view_mode);

  if (config->format == PRINT_FORMAT_FRAMED) {
    fprintf(out_fp, "%s %s %s\n\n", config->frame_separator, file_entry->name,
            config->frame_separator);
  } else if (config->format == PRINT_FORMAT_PAGEBREAK) {
    fprintf(out_fp, "### %s\n\n", file_entry->name);
  }

  if (ctx->view_mode == DISK_MODE || ctx->view_mode == USER_MODE) {
    in_fd = open(file_name_path, O_RDONLY);
    if (in_fd != -1) {
      while ((bytes_read = read(in_fd, buffer, sizeof(buffer))) > 0) {
        if (fwrite(buffer, 1, bytes_read, out_fp) < (size_t)bytes_read) {
          result = -1;
          break;
        }
      }
      close(in_fd);
    } else {
      result = -1;
    }
  } else {
    /* ARCHIVE_MODE */
#ifdef HAVE_LIBARCHIVE
    const char *archive = ctx->active->vol->vol_stats.log_path;
    ExtractArchiveEntry(archive, file_name_path, fileno(out_fp), NULL, NULL);
#endif
  }

  /* Separator acts as a page-break divider between files, not a wrapper.
   * Emit it after each file except the last. */
  if (config->format == PRINT_FORMAT_FRAMED) {
    fprintf(out_fp, "\n");
    for (int i = 0; i < 40; i++) {
      fprintf(out_fp, "%s",
              config->frame_separator[0] ? config->frame_separator : "-");
    }
    fprintf(out_fp, "\n\n");
  } else if (config->format == PRINT_FORMAT_PAGEBREAK) {
    if (!is_last) {
      fprintf(out_fp, "\n%s\n\n", config->frame_separator);
    } else {
      fprintf(out_fp, "\n");
    }
  } else {
    /* RAW or Default */
    fprintf(out_fp, "\n");
  }

  return result;
}

PrintWriteStatus Cmd_WritePrintOutput(ViewContext *ctx, DirEntry *dir_entry,
                                      BOOL tagged, PrintConfig *config,
                                      int *is_pipe, char *error_target) {
  FILE *out_fp = NULL;
  int pipe_output = TRUE;
  char *dest_raw = config->print_to;
  char expanded[PATH_LENGTH + 1];
  char path[PATH_LENGTH + 1];
  int start_dir_fd;

  if (is_pipe) {
    *is_pipe = TRUE;
  }
  if (error_target) {
    error_target[0] = '\0';
  }

  /* Strip leading spaces */
  while (*dest_raw == ' ')
    dest_raw++;

  if (*dest_raw == '\0') {
    return PRINT_WRITE_NO_DESTINATION;
  }

  /* Robustly save current working directory */
  start_dir_fd = open(".", O_RDONLY);
  if (start_dir_fd == -1) {
    return PRINT_WRITE_IO_ERROR;
  }

  /* Change to the current panel's directory so bare relative paths work */
  if (ctx->view_mode == DISK_MODE || ctx->view_mode == USER_MODE) {
    if (chdir(GetPath(dir_entry, path))) {
      close(start_dir_fd);
      return PRINT_WRITE_IO_ERROR;
    }
  }

  const char *dest = dest_raw;
  if (*dest == '>') {
    pipe_output = FALSE;
    dest++; /* Skip '>' */
    while (*dest == ' ')
      dest++; /* Skip spaces after '>' */
    ExpandPath(dest, expanded, sizeof(expanded));
    out_fp = fopen(expanded, "a");
  } else {
    out_fp = popen(dest, "w");
  }

  if (out_fp == NULL) {
    if (fchdir(start_dir_fd) == -1) {
    }
    close(start_dir_fd);
    if (error_target) {
      snprintf(error_target, PATH_LENGTH + 1, "%s",
               pipe_output ? dest : expanded);
    }
    if (is_pipe) {
      *is_pipe = pipe_output;
    }
    return PRINT_WRITE_OPEN_FAILED;
  }

  /* Walk and write files, passing is_first/is_last for separator logic */
  if (tagged) {
    /* Count tagged files first */
    int total_tagged = 0, written = 0;
    int i;
    for (i = 0; i < (int)ctx->active->file_count; i++) {
      const FileEntry *fe = ctx->active->file_entry_list[i].file;
      if (fe && fe->tagged)
        total_tagged++;
    }
    for (i = 0; i < (int)ctx->active->file_count; i++) {
      FileEntry *fe_ptr = ctx->active->file_entry_list[i].file;
      if (fe_ptr && fe_ptr->tagged) {
        written++;
        PrintFileContent(ctx, fe_ptr, out_fp, config, written == 1,
                         written == total_tagged);
      }
    }
  } else {
    FileEntry *fe_ptr =
        ctx->active
            ->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]
            .file;
    if (fe_ptr) {
      PrintFileContent(ctx, fe_ptr, out_fp, config, TRUE, TRUE);
    }
  }

  if (pipe_output) {
    pclose(out_fp);
  } else {
    fclose(out_fp);
  }

  if (fchdir(start_dir_fd) == -1) {
  }
  close(start_dir_fd);

  if (is_pipe) {
    *is_pipe = pipe_output;
  }
  return PRINT_WRITE_OK;
}
