/***************************************************************************
 *
 * src/ui/ui_edit_config.c
 * Shared UI helper: open the user configuration profile in the editor.
 *
 * Ownership invariant: this helper is called from any controller that
 * handles ACTION_EDIT_CONFIG; it must not acquire any controller-private
 * state.
 *
 ***************************************************************************/

#define NO_YTREE_MACROS
#include "../core/default_profile_template.h"
#include "ytree_cmd.h"
#include "ytree_ui.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static int WriteAll(int fd, const char *buf, size_t len) {
  size_t written_total = 0;

  while (written_total < len) {
    ssize_t written_now =
        write(fd, buf + written_total, len - written_total);
    if (written_now <= 0)
      return -1;
    written_total += (size_t)written_now;
  }
  return 0;
}

static int FileMatchesDefaultProfileTemplate(const char *path) {
  FILE *fp;
  size_t expected_len;
  char *buffer;
  size_t bytes_read;
  int matches = 0;

  fp = fopen(path, "r");
  if (!fp)
    return -1;

  expected_len = strlen(default_profile_template);
  buffer = (char *)malloc(expected_len + 1);
  if (!buffer) {
    fclose(fp);
    return -1;
  }

  bytes_read = fread(buffer, 1, expected_len + 1, fp);
  if (!ferror(fp) && bytes_read == expected_len &&
      memcmp(buffer, default_profile_template, expected_len) == 0) {
    matches = 1;
  }

  free(buffer);
  fclose(fp);
  return matches;
}

static int WasConfigBufferSaved(const char *path, const struct stat *before) {
  struct stat after;
  int template_match;

  if (stat(path, &after) != 0)
    return 0;

  if (after.st_ino != before->st_ino || after.st_size != before->st_size ||
      after.st_mtime != before->st_mtime || after.st_ctime != before->st_ctime)
    return 1;

  template_match = FileMatchesDefaultProfileTemplate(path);
  if (template_match < 0)
    return 1;
  return template_match == 0;
}

static int EditMissingProfileFromDefault(ViewContext *ctx, DirEntry *dir_entry,
                                         const char *profile_path) {
  char temp_path[PATH_LENGTH + 1];
  int fd;
  size_t template_len;
  struct stat temp_before_edit;
  int written;

  written = snprintf(temp_path, sizeof(temp_path), "%s.tmp.XXXXXX",
                     profile_path);
  if (written < 0 || written >= (int)sizeof(temp_path)) {
    MESSAGE(ctx, "Can't stage default config for \"%s\"", profile_path);
    return -1;
  }
  if (!strstr(temp_path, "XXXXXX")) {
    MESSAGE(ctx, "Can't stage default config for \"%s\"", profile_path);
    return -1;
  }

  fd = mkstemp(temp_path);
  if (fd == -1) {
    MESSAGE(ctx, "Can't stage default config for \"%s\"*%s", profile_path,
            strerror(errno));
    return -1;
  }

  template_len = strlen(default_profile_template);
  if (WriteAll(fd, default_profile_template, template_len) != 0 ||
      fstat(fd, &temp_before_edit) != 0) {
    int saved_errno = errno;
    close(fd);
    unlink(temp_path);
    MESSAGE(ctx, "Can't stage default config for \"%s\"*%s", profile_path,
            strerror(saved_errno));
    return -1;
  }
  close(fd);

  if (Edit(ctx, dir_entry, temp_path) != 0) {
    unlink(temp_path);
    return -1;
  }

  if (!WasConfigBufferSaved(temp_path, &temp_before_edit)) {
    unlink(temp_path);
    return 0;
  }

  if (rename(temp_path, profile_path) != 0) {
    int saved_errno = errno;
    unlink(temp_path);
    MESSAGE(ctx, "Can't save \"%s\"*%s", profile_path, strerror(saved_errno));
    return -1;
  }

  return 0;
}

void UI_OpenConfigProfile(ViewContext *ctx, DirEntry *dir_entry) {
  char profile_path[PATH_LENGTH + 1];
  const char *home;
  int profile_exists;

  profile_path[0] = '\0';
  home = getenv("HOME");
  if (home && *home) {
    int written;
    written = snprintf(profile_path, sizeof(profile_path), "%s/%s", home,
                       PROFILE_FILENAME);
    if (written < 0 || written >= (int)sizeof(profile_path))
      profile_path[0] = '\0';
  }
  if (!profile_path[0])
    (void)snprintf(profile_path, sizeof(profile_path), "%s", PROFILE_FILENAME);

  profile_exists = (access(profile_path, F_OK) == 0);
  if (profile_exists) {
    if (Edit(ctx, dir_entry, profile_path) != 0) {
      MESSAGE(ctx, "Can't edit \"%s\"", profile_path);
      return;
    }
  } else {
    if (EditMissingProfileFromDefault(ctx, dir_entry, profile_path) != 0) {
      MESSAGE(ctx, "Can't edit \"%s\"", profile_path);
      return;
    }
  }

  if (ctx->core_init_ops.read_profile != NULL &&
      access(profile_path, F_OK) == 0) {
    if (ctx->core_init_ops.read_profile(ctx, profile_path) == 0) {
      ctx->bypass_small_window =
          ParseSmallWindowSkipValue(GetProfileValue(ctx, "SMALLWINDOWSKIP"));
      if (ctx->core_init_ops.reinit_color_pairs != NULL) {
        ctx->core_init_ops.reinit_color_pairs(ctx);
      }
    }
  }
}
