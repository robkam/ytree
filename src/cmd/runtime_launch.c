/***************************************************************************
 *
 * src/cmd/runtime_launch.c
 * Shared runtime child-process launch helpers
 *
 ***************************************************************************/

#include "ytnova_runtime_launch.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#define RUNTIME_LAUNCH_READ_CHUNK 4096
#define RUNTIME_LAUNCH_INITIAL_BUFFER 8192

static int RuntimeLaunchWaitInternal(pid_t child_pid, int *status_out) {
  int wait_status;
  pid_t wait_result;

  do {
    wait_result = waitpid(child_pid, &wait_status, 0);
  } while (wait_result == -1 && errno == EINTR);

  if (wait_result == -1) {
    return -1;
  }
  if (status_out != NULL) {
    *status_out = wait_status;
  }
  return 0;
}

static void RuntimeLaunchCloseOwnedFds(int fd_a, int fd_b, int fd_c) {
  if (fd_a > STDERR_FILENO) {
    close(fd_a);
  }
  if (fd_b > STDERR_FILENO && fd_b != fd_a) {
    close(fd_b);
  }
  if (fd_c > STDERR_FILENO && fd_c != fd_a && fd_c != fd_b) {
    close(fd_c);
  }
}

static int RuntimeLaunchExecArgvChild(const char *working_directory,
                                      char *const argv[], int stdin_fd,
                                      int stdout_fd, int stderr_fd,
                                      BOOL create_session) {
  if (argv == NULL || argv[0] == NULL) {
    errno = EINVAL;
    return -1;
  }

  if (create_session && setsid() == -1) {
    return -1;
  }
  if (working_directory != NULL && *working_directory != '\0' &&
      chdir(working_directory) != 0) {
    return -1;
  }

  if (stdin_fd >= 0 && stdin_fd != STDIN_FILENO &&
      dup2(stdin_fd, STDIN_FILENO) == -1) {
    return -1;
  }
  if (stdout_fd >= 0 && stdout_fd != STDOUT_FILENO &&
      dup2(stdout_fd, STDOUT_FILENO) == -1) {
    return -1;
  }
  if (stderr_fd >= 0 && stderr_fd != STDERR_FILENO &&
      dup2(stderr_fd, STDERR_FILENO) == -1) {
    return -1;
  }

  RuntimeLaunchCloseOwnedFds(stdin_fd, stdout_fd, stderr_fd);
  execvp(argv[0], argv);
  return -1;
}

static int RuntimeLaunchReadAllFromFd(int fd, char **output_ptr) {
  char *buffer = NULL;
  size_t used = 0;
  size_t capacity = 0;

  if (output_ptr == NULL) {
    errno = EINVAL;
    return -1;
  }

  for (;;) {
    ssize_t read_now;
    size_t chunk_size = RUNTIME_LAUNCH_READ_CHUNK;
    size_t remaining;
    char *new_buffer;

    if (capacity - used < chunk_size + 1) {
      size_t new_capacity =
          (capacity == 0) ? RUNTIME_LAUNCH_INITIAL_BUFFER : capacity * 2;
      while (new_capacity - used < chunk_size + 1) {
        new_capacity *= 2;
      }
      new_buffer = (char *)realloc(buffer, new_capacity);
      if (new_buffer == NULL) {
        free(buffer);
        return -1;
      }
      buffer = new_buffer;
      capacity = new_capacity;
    }

    remaining = capacity - used - 1;
    read_now = read(fd, buffer + used, remaining);
    if (read_now == -1) {
      if (errno == EINTR) {
        continue;
      }
      free(buffer);
      return -1;
    }
    if (read_now == 0) {
      break;
    }
    used += (size_t)read_now;
  }

  if (buffer == NULL) {
    buffer = (char *)malloc(1);
    if (buffer == NULL) {
      return -1;
    }
  }
  buffer[used] = '\0';
  *output_ptr = buffer;
  return 0;
}

int RuntimeLaunchWait(pid_t child_pid, int *status_out) {
  return RuntimeLaunchWaitInternal(child_pid, status_out);
}

int RuntimeLaunchRunShell(const char *command_line,
                         const char *working_directory, int stdin_fd,
                         int stdout_fd, int stderr_fd, BOOL create_session,
                         int *status_out) {
  pid_t child_pid;

  if (command_line == NULL || *command_line == '\0') {
    errno = EINVAL;
    return -1;
  }

  child_pid = fork();
  if (child_pid == -1) {
    return -1;
  }
  if (child_pid == 0) {
    (void)RuntimeLaunchExecShellChild(command_line, working_directory, stdin_fd,
                                      stdout_fd, stderr_fd,
                                      create_session);
    _exit(127);
  }

  return RuntimeLaunchWaitInternal(child_pid, status_out);
}

int RuntimeLaunchExecShellChild(const char *command_line,
                                const char *working_directory, int stdin_fd,
                                int stdout_fd, int stderr_fd,
                                BOOL create_session) {
  char *argv[4];

  if (command_line == NULL || *command_line == '\0') {
    errno = EINVAL;
    return -1;
  }

  argv[0] = "/bin/sh";
  argv[1] = "-c";
  argv[2] = (char *)command_line;
  argv[3] = NULL;
  return RuntimeLaunchExecArgvChild(working_directory, argv, stdin_fd, stdout_fd,
                                    stderr_fd, create_session);
}

int RuntimeLaunchStartShellWriter(const char *command_line,
                                  const char *working_directory,
                                  FILE **pipe_fp_out, pid_t *child_pid_out) {
  int pipefd[2];
  pid_t child_pid;
  FILE *pipe_fp;

  if (command_line == NULL || *command_line == '\0' || pipe_fp_out == NULL ||
      child_pid_out == NULL) {
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
    close(pipefd[1]);
    (void)RuntimeLaunchExecShellChild(command_line, working_directory, pipefd[0],
                                      -1, -1, FALSE);
    _exit(127);
  }

  close(pipefd[0]);
  pipe_fp = fdopen(pipefd[1], "w");
  if (pipe_fp == NULL) {
    close(pipefd[1]);
    (void)RuntimeLaunchWaitInternal(child_pid, NULL);
    return -1;
  }

  *pipe_fp_out = pipe_fp;
  *child_pid_out = child_pid;
  return 0;
}

int RuntimeLaunchStartArgvWriter(char *const argv[],
                                 const char *working_directory,
                                 const char *redirect_path,
                                 BOOL redirect_append, FILE **pipe_fp_out,
                                 pid_t *child_pid_out) {
  int pipefd[2];
  pid_t child_pid;
  FILE *pipe_fp;

  if (argv == NULL || argv[0] == NULL || pipe_fp_out == NULL ||
      child_pid_out == NULL) {
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
    int out_fd = -1;

    close(pipefd[1]);
    if (redirect_path != NULL) {
      int flags;

      flags = O_WRONLY | O_CREAT | (redirect_append ? O_APPEND : O_TRUNC);
      out_fd = open(redirect_path, flags, 0666);
      if (out_fd == -1) {
        _exit(127);
      }
    }
    (void)RuntimeLaunchExecArgvChild(working_directory, argv, pipefd[0], out_fd,
                                     -1, FALSE);
    _exit(127);
  }

  close(pipefd[0]);
  pipe_fp = fdopen(pipefd[1], "w");
  if (pipe_fp == NULL) {
    close(pipefd[1]);
    (void)RuntimeLaunchWaitInternal(child_pid, NULL);
    return -1;
  }

  *pipe_fp_out = pipe_fp;
  *child_pid_out = child_pid;
  return 0;
}

int RuntimeLaunchCloseWriter(FILE *pipe_fp, pid_t child_pid, int *status_out) {
  int close_status;
  int wait_status;

  if (pipe_fp == NULL) {
    errno = EINVAL;
    return -1;
  }

  close_status = fclose(pipe_fp);
  if (RuntimeLaunchWaitInternal(child_pid, &wait_status) != 0) {
    return -1;
  }
  if (status_out != NULL) {
    *status_out = wait_status;
  }
  if (close_status != 0) {
    return -1;
  }
  return 0;
}

int RuntimeLaunchCaptureShellOutput(const char *command_line,
                                    const char *working_directory,
                                    char **output_ptr) {
  int pipefd[2];
  pid_t child_pid;
  int child_status;
  char *buffer = NULL;

  if (command_line == NULL || *command_line == '\0' || output_ptr == NULL) {
    errno = EINVAL;
    return -1;
  }

  *output_ptr = NULL;
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
    close(pipefd[0]);
    (void)RuntimeLaunchExecShellChild(command_line, working_directory, -1,
                                      pipefd[1], -1, FALSE);
    _exit(127);
  }

  close(pipefd[1]);
  if (RuntimeLaunchReadAllFromFd(pipefd[0], &buffer) != 0) {
    close(pipefd[0]);
    (void)RuntimeLaunchWaitInternal(child_pid, NULL);
    return -1;
  }
  close(pipefd[0]);

  if (RuntimeLaunchWaitInternal(child_pid, &child_status) != 0) {
    free(buffer);
    return -1;
  }
  if (child_status != 0) {
    free(buffer);
    return -1;
  }

  *output_ptr = buffer;
  return 0;
}
