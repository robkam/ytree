/***************************************************************************
 *
 * src/cmd/print_ops.c
 * Write/Print Dialog and Formatting Logic
 *
 ***************************************************************************/

#include "ytree_cmd.h"
#include "ytree_fs.h"
#include "ytree_ui.h"
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
      struct passwd *pw = getpwuid(getuid());
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
static int PrintFileContent(ViewContext *ctx, FileEntry *file_entry,
                            FILE *out_fp, PrintConfig *config, BOOL is_first,
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

static BOOL WritePrintOutput(ViewContext *ctx, DirEntry *dir_entry, BOOL tagged,
                             PrintConfig *config) {
  FILE *out_fp = NULL;
  BOOL is_pipe = TRUE;
  char *dest_raw = config->print_to;
  char expanded[PATH_LENGTH + 1];
  char path[PATH_LENGTH + 1];
  int start_dir_fd;

  /* Strip leading spaces */
  while (*dest_raw == ' ')
    dest_raw++;

  if (*dest_raw == '\0') {
    UI_Message(ctx, "No destination specified");
    return FALSE;
  }

  /* Robustly save current working directory */
  start_dir_fd = open(".", O_RDONLY);
  if (start_dir_fd == -1) {
    return FALSE;
  }

  /* Change to the current panel's directory so bare relative paths work */
  if (ctx->view_mode == DISK_MODE || ctx->view_mode == USER_MODE) {
    if (chdir(GetPath(dir_entry, path))) {
      close(start_dir_fd);
      return FALSE;
    }
  }

  /* Exit curses mode */
  endwin();
  SuspendClock(ctx);

  const char *dest = dest_raw;
  if (*dest == '>') {
    is_pipe = FALSE;
    dest++; /* Skip '>' */
    while (*dest == ' ')
      dest++; /* Skip spaces after '>' */
    ExpandPath(dest, expanded, sizeof(expanded));
    out_fp = fopen(expanded, "a");
  } else {
    out_fp = popen(dest, "w");
  }

  if (out_fp == NULL) {
    InitClock(ctx);
    touchwin(stdscr);
    wnoutrefresh(stdscr);
    doupdate();
    if (fchdir(start_dir_fd) == -1) {
    }
    close(start_dir_fd);
    if (is_pipe) {
      UI_Message(ctx, "execution of command*%s*failed", dest);
    } else {
      UI_Message(ctx, "Failed to open file*%s*", is_pipe ? dest : expanded);
    }
    return FALSE;
  }

  /* Walk and write files, passing is_first/is_last for separator logic */
  if (tagged) {
    /* Count tagged files first */
    int total_tagged = 0, written = 0;
    int i;
    for (i = 0; i < (int)ctx->active->file_count; i++) {
      FileEntry *fe = ctx->active->file_entry_list[i].file;
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

  if (is_pipe) {
    pclose(out_fp);
    /* Only pause for pipes, since file writes are silent */
    HitReturnToContinue();
  } else {
    fclose(out_fp);
  }

  /* Restore curses mode */
  InitClock(ctx);
  touchwin(stdscr);
  wnoutrefresh(stdscr);
  doupdate();

  if (fchdir(start_dir_fd) == -1) {
  }
  close(start_dir_fd);

  return TRUE;
}

void UI_HandlePrint(ViewContext *ctx, DirEntry *dir_entry, BOOL tagged) {
  PrintConfig config;
  int term;

  memset(&config, 0, sizeof(config));
  config.format = PRINT_FORMAT_RAW;

  if (tagged && dir_entry->tagged_files == 0) {
    UI_Beep(ctx, FALSE);
    return;
  }

  ClearHelp(ctx);

  term = InputChoice(
      ctx, "Format: (R)aw, (F)ramed, (P)age break  (Esc) cancel  ", "RFP\033");
  if (term == ESC || term == '\033') {
    wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
    wclrtoeol(ctx->ctx_border_window);
    wnoutrefresh(ctx->ctx_border_window);
    return;
  }

  if (term == 'R')
    config.format = PRINT_FORMAT_RAW;
  else if (term == 'F')
    config.format = PRINT_FORMAT_FRAMED;
  else
    config.format = PRINT_FORMAT_PAGEBREAK;

  if (config.format == PRINT_FORMAT_FRAMED ||
      config.format == PRINT_FORMAT_PAGEBREAK) {
    char frame_sep[32] = "";
    if (UI_ReadString(ctx, ctx->active,
                      "Frame separator (default: ```): ", frame_sep,
                      sizeof(frame_sep) - 1, HST_PRINT_FRAME) == ESC) {
      wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
      wclrtoeol(ctx->ctx_border_window);
      wnoutrefresh(ctx->ctx_border_window);
      return;
    }
    if (frame_sep[0] == '\0') {
      snprintf(frame_sep, sizeof(frame_sep), "```");
    }
    snprintf(config.frame_separator, sizeof(config.frame_separator), "%s",
             frame_sep);
  }

  {
    char prompt[COMMAND_LINE_LENGTH + 1];
    char cwd[PATH_LENGTH + 1];
    if (getcwd(cwd, sizeof(cwd))) {
      /* Safe truncation for prompt */
      snprintf(prompt, sizeof(prompt), "Write to (CWD: %.200s): ", cwd);
    } else {
      snprintf(prompt, sizeof(prompt), "Write to (command or >file): ");
    }

    if (UI_ReadString(ctx, ctx->active, prompt, config.print_to, PATH_LENGTH,
                      HST_FILE) != CR) {
      wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
      wclrtoeol(ctx->ctx_border_window);
      wnoutrefresh(ctx->ctx_border_window);
      return;
    }
  }

  WritePrintOutput(ctx, dir_entry, tagged, &config);

  /* Clear prompt */
  wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
  wclrtoeol(ctx->ctx_border_window);
  wnoutrefresh(ctx->ctx_border_window);
}
