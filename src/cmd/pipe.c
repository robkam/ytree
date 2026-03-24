/***************************************************************************
 *
 * src/cmd/pipe.c
 * Redirecting file and directory contents to a command
 *
 ***************************************************************************/

#include "ytree.h"
#include "ytree_cmd.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifdef HAVE_LIBARCHIVE
#include "ytree_fs.h"
#endif

int Pipe(ViewContext *ctx, DirEntry *dir_entry, FileEntry *file_entry,
         char *pipe_command) {
  char file_name_path[PATH_LENGTH + 1];
  int result = -1;
  FILE *pipe_fp;
  char path[PATH_LENGTH + 1];
  int start_dir_fd;

  (void)GetRealFileNamePath(file_entry, file_name_path, ctx->view_mode);

  /* Robustly save current working directory using a file descriptor */
  start_dir_fd = open(".", O_RDONLY);
  if (start_dir_fd == -1) {
    return -1;
  }

  if (ctx->view_mode == DISK_MODE || ctx->view_mode == USER_MODE) {
    if (chdir(GetPath(dir_entry, path))) {
      close(start_dir_fd);
      return -1;
    }
  } else { /* ARCHIVE_MODE */
    char archive_dir[PATH_LENGTH + 1];
    char *last_slash;
    int copied_len = snprintf(archive_dir, sizeof(archive_dir), "%s",
                              ctx->active->vol->vol_stats.log_path);
    if (copied_len < 0) {
      close(start_dir_fd);
      return -1;
    }
    if ((size_t)copied_len >= sizeof(archive_dir)) {
      close(start_dir_fd);
      return -1;
    }
    last_slash = strrchr(archive_dir, '/');
    if (last_slash) {
      if (last_slash ==
          archive_dir) { /* Root directory, e.g., "/archive.zip" */
        *(last_slash + 1) = '\0';
      } else {
        *last_slash = '\0';
      }
      if (chdir(archive_dir) != 0) {
        close(start_dir_fd);
        return -1;
      }
    }
  }

  /* Exit curses mode for external command */
  endwin();
  SuspendClock(ctx);

  pipe_fp = popen(pipe_command, "w");
  if (pipe_fp == NULL) {
    /* Restore curses mode */
    InitClock(ctx);
    touchwin(stdscr);
    wnoutrefresh(stdscr);
    doupdate();

    /* Restore CWD before returning */
    if (fchdir(start_dir_fd) == -1) {
    }
    close(start_dir_fd);
    return -1;
  } else {
    if (ctx->view_mode == DISK_MODE || ctx->view_mode == USER_MODE) {
      int in_fd;
      in_fd = open(file_name_path, O_RDONLY);
      if (in_fd != -1) {
        char buffer[4096];
        ssize_t bytes_read;
        while ((bytes_read = read(in_fd, buffer, sizeof(buffer))) > 0) {
          if (fwrite(buffer, 1, bytes_read, pipe_fp) < (size_t)bytes_read) {
            break;
          }
        }
        close(in_fd);
      }
    } else {
      /* ARCHIVE_MODE */
#ifdef HAVE_LIBARCHIVE
      const char *archive = ctx->active->vol->vol_stats.log_path;
      ExtractArchiveEntry(archive, file_name_path, fileno(pipe_fp), NULL, NULL);
#endif
    }
    result = pclose(pipe_fp);

    /* Wait for user to see output */
    HitReturnToContinue();
  }

  /* Restore curses mode */
  InitClock(ctx);
  touchwin(stdscr);
  wnoutrefresh(stdscr);
  doupdate();

  if (fchdir(start_dir_fd) == -1) {
  }
  close(start_dir_fd);

  return (result);
}

int PipeDirectory(ViewContext *ctx, DirEntry *dir_entry, char *pipe_command) {
  char path[PATH_LENGTH + 1];
  FILE *pipe_fp;
  FileEntry *fe;
  int result = -1;
  int start_dir_fd;

  (void)GetPath(dir_entry, path);

  /* Robustly save current working directory using a file descriptor */
  start_dir_fd = open(".", O_RDONLY);
  if (start_dir_fd == -1) {
    return -1;
  }

  if (ctx->view_mode == DISK_MODE || ctx->view_mode == USER_MODE) {
    if (chdir(path)) {
      close(start_dir_fd);
      return (-1);
    }
  }

  /* Exit curses mode for external command */
  endwin();
  SuspendClock(ctx);

  if ((pipe_fp = popen(pipe_command, "w")) == NULL) {
    /* Restore curses mode */
    InitClock(ctx);
    touchwin(stdscr);
    wnoutrefresh(stdscr);
    doupdate();

    /* Restore CWD */
    if (fchdir(start_dir_fd) == -1) {
    }
    close(start_dir_fd);
    return (-1);
  }

  for (fe = dir_entry->file; fe; fe = fe->next) {
    if (fe->matching) {
      if (!ctx->hide_dot_files || fe->name[0] != '.') {
        fprintf(pipe_fp, "%s\n", fe->name);
      }
    }
  }

  pclose(pipe_fp);

  /* Wait for user to see output */
  HitReturnToContinue();

  result = 0;

  /* Restore curses mode */
  InitClock(ctx);
  touchwin(stdscr);
  wnoutrefresh(stdscr);
  doupdate();

  /* Restore CWD */
  if (fchdir(start_dir_fd) == -1) {
  }
  close(start_dir_fd);

  return (result);
}

/* GetPipeCommand moved to UI layer */

int PipeTaggedFiles(ViewContext *ctx, FileEntry *fe_ptr,
                    WalkingPackage *walking_package, Statistic *s) {
  int i, n;
  char from_path[PATH_LENGTH + 1];
  char buffer[2048];

  (void)s; /* Unused */

  walking_package->new_fe_ptr = fe_ptr; /* unchanged */

  (void)GetRealFileNamePath(fe_ptr, from_path, ctx->view_mode);
  if ((i = open(from_path, O_RDONLY)) == -1) {
    return (-1);
  }

  while ((n = read(i, buffer, sizeof(buffer))) > 0) {
    if (fwrite(buffer, n, 1,
               walking_package->function_data.pipe_cmd.pipe_file) != 1) {
      (void)close(i);
      return (-1);
    }
  }

  (void)close(i);

  return (0);
}
