/***************************************************************************
 *
 * src/cmd/pipe.c
 * Redirecting file and directory contents to a command
 *
 ***************************************************************************/

#include "ytree_cmd.h"
#include "ytree_fs.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define PIPE_ARGV_MAX 64

static int ParsePipeCommand(char *command_line, char **argv, size_t argv_len) {
  size_t argc = 0;
  char *p = command_line;

  if (!command_line || !argv || argv_len < 2) {
    return -1;
  }

  while (*p) {
    while (*p == ' ' || *p == '\t') {
      p++;
    }
    if (!*p) {
      break;
    }
    if (argc + 1 >= argv_len) {
      return -1;
    }
    argv[argc++] = p;
    while (*p && *p != ' ' && *p != '\t') {
      p++;
    }
    if (*p) {
      *p = '\0';
      p++;
    }
  }

  if (argc == 0) {
    return -1;
  }

  argv[argc] = NULL;
  return 0;
}

static int OpenPipeWriter(const char *pipe_command, FILE **pipe_fp_out,
                          pid_t *child_pid_out) {
  int pipefd[2];
  int copied_len;
  FILE *pipe_fp;
  pid_t child_pid;
  size_t argc;
  size_t i;
  char command_line[PATH_LENGTH + 1];
  char *argv[PIPE_ARGV_MAX];
  char *redirect_path = NULL;
  BOOL redirect_append = FALSE;

  if (!pipe_command || !pipe_fp_out || !child_pid_out) {
    return -1;
  }

  copied_len = snprintf(command_line, sizeof(command_line), "%s", pipe_command);
  if (copied_len < 0 || (size_t)copied_len >= sizeof(command_line)) {
    errno = EINVAL;
    return -1;
  }
  if (ParsePipeCommand(command_line, argv, PIPE_ARGV_MAX) != 0) {
    errno = EINVAL;
    return -1;
  }
  for (argc = 0; argc < PIPE_ARGV_MAX && argv[argc]; argc++) {
  }
  for (i = 0; i < argc; i++) {
    if (!strcmp(argv[i], ">") || !strcmp(argv[i], ">>")) {
      if (i + 1 >= argc) {
        errno = EINVAL;
        return -1;
      }
      redirect_path = argv[i + 1];
      redirect_append = (argv[i][1] == '>');
      argv[i] = NULL;
      argc = i;
      break;
    }
  }
  if (argc == 0 || !argv[0]) {
    errno = EINVAL;
    return -1;
  }

  if (pipe(pipefd) != 0) {
    return -1;
  }

  child_pid = fork();
  if (child_pid == -1) {
    close(pipefd[0]);
    close(pipefd[1]);
    return -1;
  }

  if (child_pid == 0) {
    if (dup2(pipefd[0], STDIN_FILENO) == -1) {
      _exit(127);
    }
    if (redirect_path) {
      int out_fd;
      int flags = O_WRONLY | O_CREAT | (redirect_append ? O_APPEND : O_TRUNC);
      out_fd = open(redirect_path, flags, 0666);
      if (out_fd == -1) {
        _exit(127);
      }
      if (dup2(out_fd, STDOUT_FILENO) == -1) {
        close(out_fd);
        _exit(127);
      }
      close(out_fd);
    }
    close(pipefd[0]);
    close(pipefd[1]);
    execvp(argv[0], argv);
    _exit(127);
  }

  close(pipefd[0]);
  pipe_fp = fdopen(pipefd[1], "w");
  if (!pipe_fp) {
    close(pipefd[1]);
    (void)waitpid(child_pid, NULL, 0);
    return -1;
  }

  *pipe_fp_out = pipe_fp;
  *child_pid_out = child_pid;
  return 0;
}

static int ClosePipeWriter(FILE *pipe_fp, pid_t child_pid) {
  int wait_status;
  int close_status;

  if (!pipe_fp) {
    return -1;
  }

  close_status = fclose(pipe_fp);
  do {
    wait_status = waitpid(child_pid, NULL, 0);
  } while (wait_status == -1 && errno == EINTR);

  if (close_status != 0 || wait_status == -1) {
    return -1;
  }
  return 0;
}

int Pipe(ViewContext *ctx, DirEntry *dir_entry, FileEntry *file_entry,
         char *pipe_command) {
  char file_name_path[PATH_LENGTH + 1];
  int result = -1;
  FILE *pipe_fp = NULL;
  pid_t child_pid = -1;
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
  if (ctx->hook_suspend_clock)
    ctx->hook_suspend_clock(ctx);

  if (OpenPipeWriter(pipe_command, &pipe_fp, &child_pid) != 0) {
    /* Restore curses mode */
    if (ctx->hook_init_clock)
      ctx->hook_init_clock(ctx);
    touchwin(stdscr);
    wnoutrefresh(stdscr);
    if (ctx->hook_refresh_ui)
      ctx->hook_refresh_ui();

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
    result = ClosePipeWriter(pipe_fp, child_pid);

    /* Wait for user to see output */
    if (ctx->hook_hit_return_to_continue)
      ctx->hook_hit_return_to_continue();
  }

  /* Restore curses mode */
  if (ctx->hook_init_clock)
    ctx->hook_init_clock(ctx);
  touchwin(stdscr);
  wnoutrefresh(stdscr);
  if (ctx->hook_refresh_ui)
    ctx->hook_refresh_ui();

  if (fchdir(start_dir_fd) == -1) {
  }
  close(start_dir_fd);

  return (result);
}

int PipeDirectory(ViewContext *ctx, DirEntry *dir_entry, char *pipe_command) {
  char path[PATH_LENGTH + 1];
  FILE *pipe_fp = NULL;
  FileEntry *fe;
  int result = -1;
  int start_dir_fd;
  pid_t child_pid = -1;

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
  if (ctx->hook_suspend_clock)
    ctx->hook_suspend_clock(ctx);

  if (OpenPipeWriter(pipe_command, &pipe_fp, &child_pid) != 0) {
    /* Restore curses mode */
    if (ctx->hook_init_clock)
      ctx->hook_init_clock(ctx);
    touchwin(stdscr);
    wnoutrefresh(stdscr);
    if (ctx->hook_refresh_ui)
      ctx->hook_refresh_ui();

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

  if (ClosePipeWriter(pipe_fp, child_pid) != 0) {
    goto PIPE_CLOSE_FAILURE;
  }

  /* Wait for user to see output */
  if (ctx->hook_hit_return_to_continue)
    ctx->hook_hit_return_to_continue();

  result = 0;

  /* Restore curses mode */
  if (ctx->hook_init_clock)
    ctx->hook_init_clock(ctx);
  touchwin(stdscr);
  wnoutrefresh(stdscr);
  if (ctx->hook_refresh_ui)
    ctx->hook_refresh_ui();

  /* Restore CWD */
  if (fchdir(start_dir_fd) == -1) {
  }
  close(start_dir_fd);

  return (result);

PIPE_CLOSE_FAILURE:
  /* Restore curses mode */
  if (ctx->hook_init_clock)
    ctx->hook_init_clock(ctx);
  touchwin(stdscr);
  wnoutrefresh(stdscr);
  if (ctx->hook_refresh_ui)
    ctx->hook_refresh_ui();

  /* Restore CWD */
  if (fchdir(start_dir_fd) == -1) {
  }
  close(start_dir_fd);
  return -1;
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
