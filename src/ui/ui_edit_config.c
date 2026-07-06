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
#include "../core/default_theme_catalog.h"
#include "ytnova_appstate_layout.h"
#include "ytnova_appstate_session.h"
#include "ytnova_appstate_visibility.h"
#include "ytnova_cmd.h"
#include "ytnova_fs.h"
#include "ytnova_ui.h"
#include "watcher.h"
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const UICommandStripCommand config_command_strip[] = {
    {UI_COMMAND_LAYOUT_MNEMONIC, "Config", "C", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Themes", "T", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "Reload", "R", NULL},
    {UI_COMMAND_LAYOUT_ALT_MNEMONIC, "Quit", "Esc", "Q"}};

typedef struct {
  const char *path;
  const char *contents;
  const char *label;
  BOOL created;
} ConfigStarterFile;

typedef struct {
  BOOL bypass_small_window;
  BOOL highlight_full_line;
  BOOL left_hide_dot_files;
  BOOL right_hide_dot_files;
  int animation_method;
  int refresh_mode;
} ReloadableProfileState;

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

static int WriteStarterFile(ViewContext *ctx, const char *path,
                            const char *contents, const char *label) {
  int fd;
  size_t template_len;
  int write_status;
  int write_errno;
  int close_status;

  if (path == NULL || *path == '\0' || contents == NULL || label == NULL)
    return -1;

  fd = open(path, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    if (errno == EEXIST)
      return 0;
    MESSAGE(ctx, "Can't create default %s \"%s\"*%s", label, path,
            strerror(errno));
    return -1;
  }

  template_len = strlen(contents);
  write_status = WriteAll(fd, contents, template_len);
  write_errno = errno;
  close_status = close(fd);
  if (write_status != 0 || close_status != 0) {
    int saved_errno = (write_status != 0) ? write_errno : errno;
    unlink(path);
    MESSAGE(ctx, "Can't create default %s \"%s\"*%s", label, path,
            strerror(saved_errno));
    return -1;
  }

  return 1;
}

static int EnsureConfigStarterFiles(ViewContext *ctx, const char *profile_path,
                                    const char *themes_path) {
  ConfigStarterFile starter_files[] = {
      {profile_path, default_profile_template, "config", FALSE},
      {themes_path, default_theme_catalog, "themes", FALSE}};
  size_t i;

  for (i = 0; i < sizeof(starter_files) / sizeof(starter_files[0]); ++i) {
    int result = WriteStarterFile(ctx, starter_files[i].path,
                                  starter_files[i].contents,
                                  starter_files[i].label);
    if (result < 0) {
      while (i > 0) {
        --i;
        if (starter_files[i].created)
          unlink(starter_files[i].path);
      }
      return -1;
    }
    starter_files[i].created = result > 0 ? TRUE : FALSE;
  }
  return 0;
}

static int EnsureConfigStarterFile(ViewContext *ctx, const char *profile_path) {
  int result = WriteStarterFile(ctx, profile_path, default_profile_template, "config");
  return result < 0 ? -1 : 0;
}

static int ApplyRefreshMode(ViewContext *ctx, DirEntry *dir_entry,
                            int refresh_mode) {
  int old_refresh_mode;

  if (ctx == NULL)
    return -1;

  old_refresh_mode = ctx->refresh_mode;
  if (!AppStateCommitRefreshMode(ctx, refresh_mode))
    return -1;

  if ((old_refresh_mode & REFRESH_WATCHER) &&
      !(refresh_mode & REFRESH_WATCHER)) {
    if (ctx->core_quit_ops.close_watcher != NULL)
      ctx->core_quit_ops.close_watcher(ctx);
  } else if (!(old_refresh_mode & REFRESH_WATCHER) &&
             (refresh_mode & REFRESH_WATCHER)) {
    if (ctx->core_storage_ops.watcher_init != NULL)
      ctx->core_storage_ops.watcher_init(ctx);
    if (dir_entry != NULL) {
      char watcher_path[PATH_LENGTH + 1];

      GetPath(dir_entry, watcher_path);
      Watcher_SetDir(ctx, watcher_path);
    }
  }

  return 0;
}

static int ApplyPanelVisibilityFilterIfAvailable(YtreeNovaPanel *panel,
                                                 BOOL hide_dot_files) {
  if (panel == NULL)
    return 0;
  if (panel->vol == NULL)
    return AppStateSeedPanelVisibilityFilter(panel, hide_dot_files) ? 0 : -1;
  return AppStateCommitPanelVisibilityFilter(panel, hide_dot_files) ? 0 : -1;
}

static int ApplyReloadableProfileSettings(ViewContext *ctx,
                                          DirEntry *dir_entry) {
  BOOL hide_dot_files;

  if (ctx == NULL)
    return -1;

  if (!AppStateCommitSmallWindowBypass(
          ctx, ParseSmallWindowSkipValue(GetProfileValue(ctx, "SMALLWINDOWSKIP"))))
    return -1;
  if (!AppStateCommitFullLineHighlight(
          ctx,
          (strtol(GetProfileValue(ctx, "HIGHLIGHT_FULL_LINE"), NULL, 0)) ? TRUE
                                                                         : FALSE))
    return -1;

  hide_dot_files =
      (strtol(GetProfileValue(ctx, "HIDEDOTFILES"), NULL, 0)) ? TRUE : FALSE;
  if (ApplyPanelVisibilityFilterIfAvailable(ctx->left, hide_dot_files) != 0 ||
      ApplyPanelVisibilityFilterIfAvailable(ctx->right, hide_dot_files) != 0)
    return -1;

  ctx->animation_method = strtol(GetProfileValue(ctx, "ANIMATION"), NULL, 0);
  if (ApplyRefreshMode(
          ctx, dir_entry,
          strtol(GetProfileValue(ctx, "AUTO_REFRESH"), NULL, 0)) != 0)
    return -1;

  return 0;
}

static void RestoreReloadableProfileState(
    ViewContext *ctx, DirEntry *dir_entry,
    const ReloadableProfileState *runtime_state) {
  if (ctx == NULL || runtime_state == NULL)
    return;

  (void)AppStateCommitSmallWindowBypass(ctx, runtime_state->bypass_small_window);
  (void)AppStateCommitFullLineHighlight(ctx, runtime_state->highlight_full_line);
  (void)ApplyPanelVisibilityFilterIfAvailable(ctx->left,
                                              runtime_state->left_hide_dot_files);
  (void)ApplyPanelVisibilityFilterIfAvailable(ctx->right,
                                              runtime_state->right_hide_dot_files);
  ctx->animation_method = runtime_state->animation_method;
  (void)ApplyRefreshMode(ctx, dir_entry, runtime_state->refresh_mode);
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

static void ResolveProfilePath(const ViewContext *ctx, char *profile_path,
                               size_t profile_path_size) {
  const char *home;

  if (profile_path == NULL || profile_path_size == 0)
    return;

  profile_path[0] = '\0';
  if (ctx != NULL && ctx->configuration_file_path[0] != '\0') {
    (void)snprintf(profile_path, profile_path_size, "%s",
                   ctx->configuration_file_path);
    return;
  }
  home = getenv("HOME");
  if (home && *home) {
    int written;

    if (EnsureConfigHomeDirectory(home) == 0) {
      written = snprintf(profile_path, profile_path_size, "%s/%s", home,
                         PROFILE_CONFIG_HOME_PATH);
      if (written >= 0 && written < (int)profile_path_size)
        return;
    }

    written = snprintf(profile_path, profile_path_size, "%s/%s", home,
                       PROFILE_FILENAME);
    if (written >= 0 && written < (int)profile_path_size)
      return;
  }
  if (!profile_path[0]) {
    int written =
        snprintf(profile_path, profile_path_size, "%s", PROFILE_FILENAME);
    if (written < 0 || written >= (int)profile_path_size)
      profile_path[0] = '\0';
  }
}

static int ResolveThemesPath(char *themes_path, size_t themes_path_size) {
  const char *home;

  if (themes_path == NULL || themes_path_size == 0)
    return -1;

  themes_path[0] = '\0';
  home = getenv("HOME");
  if (home && *home) {
    int written;

    if (EnsureConfigHomeDirectory(home) == 0) {
      written = snprintf(themes_path, themes_path_size, "%s/%s", home,
                         THEME_CONFIG_HOME_PATH);
      if (written >= 0 && written < (int)themes_path_size)
        return 0;
    }

    written = snprintf(themes_path, themes_path_size, "%s/%s", home,
                       THEME_FILENAME);
    if (written >= 0 && written < (int)themes_path_size)
      return 0;
  }

  {
    int written = snprintf(themes_path, themes_path_size, "%s", THEME_FILENAME);

    if (written >= 0 && written < (int)themes_path_size)
      return 0;
  }
  return -1;
}

static int ReloadConfigAndTheme(ViewContext *ctx, DirEntry *dir_entry,
                                const char *profile_path) {
  ProfileRuntimeSnapshot *profile_snapshot;
  ReloadableProfileState runtime_state;
  int profile_validation;
#ifdef COLOR_SUPPORT
  UIColorSnapshot *color_snapshot;
#endif

  if (ctx == NULL)
    return -1;

  profile_snapshot = ProfileRuntimeSnapshot_Create(ctx);
#ifdef COLOR_SUPPORT
  color_snapshot = UIColorSnapshot_Create();
#endif
  runtime_state.bypass_small_window = ctx->bypass_small_window;
  runtime_state.highlight_full_line = ctx->highlight_full_line;
  runtime_state.left_hide_dot_files =
      ctx->left != NULL ? ctx->left->hide_dot_files : FALSE;
  runtime_state.right_hide_dot_files =
      ctx->right != NULL ? ctx->right->hide_dot_files : FALSE;
  runtime_state.animation_method = ctx->animation_method;
  runtime_state.refresh_mode = ctx->refresh_mode;

  if (ctx->core_init_ops.read_profile != NULL && profile_path != NULL &&
      access(profile_path, F_OK) == 0) {
    profile_validation = ValidateProfileFile(ctx, profile_path);
    if (profile_validation != 0) {
      ProfileRuntimeSnapshot_Restore(ctx, profile_snapshot);
      RestoreReloadableProfileState(ctx, dir_entry, &runtime_state);
#ifdef COLOR_SUPPORT
      UIColorSnapshot_Restore(color_snapshot);
      UIColorSnapshot_Free(color_snapshot);
#endif
      ProfileRuntimeSnapshot_Free(profile_snapshot);
      UI_ShowStatusLineError(ctx, profile_validation < 0
                                      ? "Reload failed: can't read config"
                                      : "Reload failed: malformed config");
      return -1;
    }
    if (ctx->core_init_ops.read_profile(ctx, profile_path) == 0) {
      if (ApplyReloadableProfileSettings(ctx, dir_entry) != 0) {
        ProfileRuntimeSnapshot_Restore(ctx, profile_snapshot);
        RestoreReloadableProfileState(ctx, dir_entry, &runtime_state);
#ifdef COLOR_SUPPORT
        UIColorSnapshot_Restore(color_snapshot);
        UIColorSnapshot_Free(color_snapshot);
#endif
        ProfileRuntimeSnapshot_Free(profile_snapshot);
        UI_ShowStatusLineError(ctx, "Reload failed: can't apply config");
        return -1;
      }
    } else {
      ProfileRuntimeSnapshot_Restore(ctx, profile_snapshot);
      RestoreReloadableProfileState(ctx, dir_entry, &runtime_state);
#ifdef COLOR_SUPPORT
      UIColorSnapshot_Restore(color_snapshot);
      UIColorSnapshot_Free(color_snapshot);
#endif
      ProfileRuntimeSnapshot_Free(profile_snapshot);
      UI_ShowStatusLineError(ctx, "Reload failed: can't read config");
      return -1;
    }
  }

  if (ctx->core_init_ops.load_theme != NULL &&
      ctx->core_init_ops.load_theme(ctx) != 0) {
    ProfileRuntimeSnapshot_Restore(ctx, profile_snapshot);
    RestoreReloadableProfileState(ctx, dir_entry, &runtime_state);
#ifdef COLOR_SUPPORT
    UIColorSnapshot_Restore(color_snapshot);
    UIColorSnapshot_Free(color_snapshot);
#endif
    ProfileRuntimeSnapshot_Free(profile_snapshot);
    UI_ShowStatusLineError(ctx, "Reload failed: can't load theme");
    return -1;
  }
  if (ctx->core_init_ops.reinit_color_pairs != NULL)
    ctx->core_init_ops.reinit_color_pairs(ctx);
  if (stdscr != NULL) {
    if (ctx->core_init_ops.wbkgd_set != NULL)
      ctx->core_init_ops.wbkgd_set(ctx, stdscr,
                                   COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));
    werase(stdscr);
  }
  ReCreateWindows(ctx);
  RefreshView(ctx, dir_entry);
#ifdef COLOR_SUPPORT
  UIColorSnapshot_Free(color_snapshot);
#endif
  ProfileRuntimeSnapshot_Free(profile_snapshot);
  return 0;
}

static void EditConfigProfile(ViewContext *ctx, DirEntry *dir_entry,
                              char *profile_path) {
  if (Edit(ctx, dir_entry, profile_path) != 0) {
    MESSAGE(ctx, "Can't edit \"%s\"", profile_path);
    return;
  }

  ReloadConfigAndTheme(ctx, dir_entry, profile_path);
}

static void EditThemesFile(ViewContext *ctx, DirEntry *dir_entry,
                           char *themes_path) {
  if (Edit(ctx, dir_entry, themes_path) != 0) {
    MESSAGE(ctx, "Can't edit \"%s\"", themes_path);
    return;
  }

  ReloadConfigAndTheme(ctx, dir_entry, NULL);
}

void UI_OpenConfigProfile(ViewContext *ctx, DirEntry *dir_entry) {
  char profile_path[PATH_LENGTH + 1];
  int term;

  ResolveProfilePath(ctx, profile_path, sizeof(profile_path));

  term = InputChoiceCommandStrip(
      ctx, config_command_strip,
      sizeof(config_command_strip) / sizeof(config_command_strip[0]),
      "CTRQ\r\n\033");

  switch (term) {
  case CR:
  case LF:
  case 'C':
    if (EnsureConfigStarterFile(ctx, profile_path) != 0)
      break;
    EditConfigProfile(ctx, dir_entry, profile_path);
    break;
  case 'T':
    {
      char themes_path[PATH_LENGTH + 1];

      if (ResolveThemesPath(themes_path, sizeof(themes_path)) != 0) {
        MESSAGE(ctx, "Can't resolve themes file path");
        break;
      }
    if (EnsureConfigStarterFiles(ctx, profile_path, themes_path) != 0)
      break;
    EditThemesFile(ctx, dir_entry, themes_path);
    }
    break;
  case 'R':
    ReloadConfigAndTheme(ctx, dir_entry, profile_path);
    break;
  case 'Q':
  case ESC:
  default:
    break;
  }
}
