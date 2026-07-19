/***************************************************************************
 *
 * src/util/atomic_file.c
 * Crash-safe file replacement helper for config/history persistence.
 *
 ***************************************************************************/

#include "ytnova_defs.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int AtomicFileBuildTempPath(const char *path, char *temp_path,
                                   size_t temp_path_size) {
  int written;

  if (path == NULL || *path == '\0' || temp_path == NULL || temp_path_size == 0)
    return -1;

  written = snprintf(temp_path, temp_path_size, "%s.tmpXXXXXX", path);
  if (written < 0 || (size_t)written >= temp_path_size) {
    temp_path[0] = '\0';
    return -1;
  }
  return 0;
}

static int AtomicFileFsyncParentDirectory(const char *path) {
  char parent_path[PATH_LENGTH + 16];
  char *slash;
  int dir_fd;
  int result = 0;

  if (path == NULL || *path == '\0')
    return -1;
  if (snprintf(parent_path, sizeof(parent_path), "%s", path) < 0 ||
      strlen(path) >= sizeof(parent_path))
    return -1;

  slash = strrchr(parent_path, FILE_SEPARATOR_CHAR);
  if (slash == NULL) {
    parent_path[0] = '.';
    parent_path[1] = '\0';
  } else if (slash == parent_path) {
    slash[1] = '\0';
  } else {
    *slash = '\0';
  }

  dir_fd = open(parent_path, O_RDONLY);
  if (dir_fd == -1)
    return -1;
  if (fsync(dir_fd) != 0)
    result = -1;
  close(dir_fd);
  return result;
}

int AtomicFileWrite(const char *path, AtomicFileWriteCallback writer,
                    void *user_data) {
  char temp_path[PATH_LENGTH + 16];
  int fd;
  FILE *fp;
  int saved_errno;
  int result = -1;

  if (AtomicFileBuildTempPath(path, temp_path, sizeof(temp_path)) != 0 ||
      writer == NULL)
    return -1;

  fd = mkstemp(temp_path);
  if (fd == -1)
    return -1;

  fp = fdopen(fd, "w");
  if (fp == NULL) {
    saved_errno = errno;
    close(fd);
    unlink(temp_path);
    errno = saved_errno;
    return -1;
  }

  if (writer(fp, user_data) != 0) {
    saved_errno = errno ? errno : EIO;
    goto cleanup;
  }
  if (fflush(fp) != 0) {
    saved_errno = errno;
    goto cleanup;
  }
  if (fsync(fileno(fp)) != 0) {
    saved_errno = errno;
    goto cleanup;
  }
  if (fclose(fp) != 0) {
    fp = NULL;
    saved_errno = errno;
    goto cleanup_closed;
  }
  fp = NULL;

  if (rename(temp_path, path) != 0) {
    saved_errno = errno;
    goto cleanup_closed;
  }
  if (AtomicFileFsyncParentDirectory(path) != 0) {
    saved_errno = errno ? errno : EIO;
    goto cleanup_closed;
  }

  return 0;

cleanup:
  fclose(fp);
cleanup_closed:
  unlink(temp_path);
  errno = saved_errno;
  return result;
}
