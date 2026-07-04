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

#define NO_YTNOVA_MACROS
#include "../core/default_profile_template.h"
#include "ytnova_appstate_layout.h"
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static const UICommandStripPart config_command_strip[] = {
    {UI_COMMAND_PUNCT, "("},     {UI_COMMAND_KEY, "C"},
    {UI_COMMAND_PUNCT, ")"},     {UI_COMMAND_LABEL, "onfig"},
    {UI_COMMAND_PUNCT, "  ("},   {UI_COMMAND_KEY, "T"},
    {UI_COMMAND_PUNCT, ")"},     {UI_COMMAND_LABEL, "hemes"},
    {UI_COMMAND_PUNCT, "  ("},   {UI_COMMAND_KEY, "R"},
    {UI_COMMAND_PUNCT, ")"},     {UI_COMMAND_LABEL, "eload"},
    {UI_COMMAND_PUNCT, "  ("},   {UI_COMMAND_KEY, "Esc"},
    {UI_COMMAND_PUNCT, ")/("},   {UI_COMMAND_KEY, "Q"},
    {UI_COMMAND_PUNCT, ")"},     {UI_COMMAND_LABEL, "uit"}};

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

static int EnsureConfigHomeDirectory(const char *home) {
  char config_dir[PATH_LENGTH + 1];
  char ytnova_dir[PATH_LENGTH + 1];
  int written;

  if (home == NULL || *home == '\0')
    return -1;

  written = snprintf(config_dir, sizeof(config_dir), "%s/%s", home,
                     PROFILE_CONFIG_HOME_PARENT);
  if (written < 0 || written >= (int)sizeof(config_dir))
    return -1;

  if (mkdir(config_dir, S_IRWXU) != 0 && errno != EEXIST)
    return -1;
  written = snprintf(ytnova_dir, sizeof(ytnova_dir), "%s/%s", home,
                     PROFILE_CONFIG_HOME_DIR);
  if (written < 0 || written >= (int)sizeof(ytnova_dir))
    return -1;
  if (mkdir(ytnova_dir, S_IRWXU) != 0 && errno != EEXIST)
    return -1;
  return 0;
}

static void ResolveProfilePath(char *profile_path, size_t profile_path_size) {
  const char *home;

  if (profile_path == NULL || profile_path_size == 0)
    return;

  profile_path[0] = '\0';
  home = getenv("HOME");
  if (home && *home) {
    int written;
    char legacy_path[PATH_LENGTH + 1];

    written = snprintf(profile_path, profile_path_size, "%s/%s", home,
                       PROFILE_CONFIG_HOME_PATH);
    if (written < 0 || written >= (int)profile_path_size)
      profile_path[0] = '\0';

    written =
        snprintf(legacy_path, sizeof(legacy_path), "%s/%s", home,
                 PROFILE_FILENAME);
    if (written >= 0 && written < (int)sizeof(legacy_path) &&
        profile_path[0] != '\0' && access(profile_path, F_OK) != 0 &&
        access(legacy_path, F_OK) == 0) {
      (void)snprintf(profile_path, profile_path_size, "%s", legacy_path);
    } else if (profile_path[0] != '\0') {
      (void)EnsureConfigHomeDirectory(home);
    }
  }
  if (!profile_path[0])
    (void)snprintf(profile_path, profile_path_size, "%s", PROFILE_FILENAME);
}

static int ResolveThemesPath(char *themes_path, size_t themes_path_size) {
  const char *home;

  if (themes_path == NULL || themes_path_size == 0)
    return -1;

  themes_path[0] = '\0';
  home = getenv("HOME");
  if (home && *home) {
    char legacy_path[PATH_LENGTH + 1];
    int written;

    written = snprintf(themes_path, themes_path_size, "%s/%s", home,
                       THEME_CONFIG_HOME_PATH);
    if (written < 0 || written >= (int)themes_path_size)
      themes_path[0] = '\0';

    written =
        snprintf(legacy_path, sizeof(legacy_path), "%s/%s", home,
                 THEME_FILENAME);
    if (written >= 0 && written < (int)sizeof(legacy_path) &&
        themes_path[0] != '\0' && access(themes_path, F_OK) != 0 &&
        access(legacy_path, F_OK) == 0) {
      (void)snprintf(themes_path, themes_path_size, "%s", legacy_path);
    } else if (themes_path[0] != '\0' &&
               EnsureConfigHomeDirectory(home) != 0) {
      themes_path[0] = '\0';
    }
  }

  if (!themes_path[0])
    (void)snprintf(themes_path, themes_path_size, "%s", THEME_FILENAME);

  return themes_path[0] ? 0 : -1;
}

static int ReloadConfigAndTheme(ViewContext *ctx, const char *profile_path) {
  ProfileRuntimeSnapshot *profile_snapshot;
  BOOL original_bypass_small_window;

  if (ctx == NULL)
    return -1;

  profile_snapshot = ProfileRuntimeSnapshot_Create(ctx);
  original_bypass_small_window = ctx->bypass_small_window;

  if (ctx->core_init_ops.read_profile != NULL && profile_path != NULL &&
      access(profile_path, F_OK) == 0) {
    if (ctx->core_init_ops.read_profile(ctx, profile_path) == 0) {
      if (!AppStateCommitSmallWindowBypass(
              ctx,
              ParseSmallWindowSkipValue(GetProfileValue(ctx, "SMALLWINDOWSKIP")))) {
        ProfileRuntimeSnapshot_Restore(ctx, profile_snapshot);
        ProfileRuntimeSnapshot_Free(profile_snapshot);
        ctx->bypass_small_window = original_bypass_small_window;
        UI_ShowStatusLineError(ctx, "Reload failed: can't apply config");
        return -1;
      }
    } else {
      ProfileRuntimeSnapshot_Restore(ctx, profile_snapshot);
      ProfileRuntimeSnapshot_Free(profile_snapshot);
      ctx->bypass_small_window = original_bypass_small_window;
      UI_ShowStatusLineError(ctx, "Reload failed: can't read config");
      return -1;
    }
  }

  if (ctx->core_init_ops.load_theme != NULL &&
      ctx->core_init_ops.load_theme(ctx) != 0) {
    ProfileRuntimeSnapshot_Restore(ctx, profile_snapshot);
    ProfileRuntimeSnapshot_Free(profile_snapshot);
    ctx->bypass_small_window = original_bypass_small_window;
    UI_ShowStatusLineError(ctx, "Reload failed: can't load theme");
    return -1;
  }
  if (ctx->core_init_ops.reinit_color_pairs != NULL)
    ctx->core_init_ops.reinit_color_pairs(ctx);
  ProfileRuntimeSnapshot_Free(profile_snapshot);
  return 0;
}

static void EditConfigProfile(ViewContext *ctx, DirEntry *dir_entry,
                              char *profile_path) {
  int profile_exists;

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

  ReloadConfigAndTheme(ctx, profile_path);
}

static void EditThemesFile(ViewContext *ctx, DirEntry *dir_entry) {
  char themes_path[PATH_LENGTH + 1];

  if (ResolveThemesPath(themes_path, sizeof(themes_path)) != 0) {
    MESSAGE(ctx, "Can't resolve themes file path");
    return;
  }

  if (Edit(ctx, dir_entry, themes_path) != 0) {
    MESSAGE(ctx, "Can't edit \"%s\"", themes_path);
    return;
  }

  ReloadConfigAndTheme(ctx, NULL);
}

void UI_OpenConfigProfile(ViewContext *ctx, DirEntry *dir_entry) {
  char profile_path[PATH_LENGTH + 1];
  int term;

  ResolveProfilePath(profile_path, sizeof(profile_path));
  term = InputChoiceCommandStrip(
      ctx, config_command_strip,
      sizeof(config_command_strip) / sizeof(config_command_strip[0]),
      "CTRQ\r\n\033");

  switch (term) {
  case CR:
  case LF:
  case 'C':
    EditConfigProfile(ctx, dir_entry, profile_path);
    break;
  case 'T':
    EditThemesFile(ctx, dir_entry);
    break;
  case 'R':
    ReloadConfigAndTheme(ctx, profile_path);
    break;
  case 'Q':
  case ESC:
  default:
    break;
  }
}
