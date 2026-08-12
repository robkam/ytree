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
#include "../core/default_applications_catalog.h"
#include "../core/default_commands_catalog.h"
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
    {UI_COMMAND_LAYOUT_MNEMONIC, "config", "C", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "commands", "M", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "themes", "T", NULL},
    {UI_COMMAND_LAYOUT_MNEMONIC, "reload", "R", NULL},
    {UI_COMMAND_LAYOUT_ALT_MNEMONIC, "quit", "Esc", "Q"}};

typedef struct {
  BOOL bypass_small_window;
  BOOL highlight_full_line;
  BOOL left_hide_dot_files;
  BOOL right_hide_dot_files;
  int animation_method;
  int refresh_mode;
} ReloadableProfileState;

typedef struct {
  const char *contents;
} StarterFileWriteContext;

typedef struct {
  int source_fd;
} StarterFileCopyContext;

static int WriteStarterFileContents(FILE *fp, const void *user_data) {
  const StarterFileWriteContext *write_ctx =
      (const StarterFileWriteContext *)user_data;
  size_t template_len;

  if (fp == NULL || write_ctx == NULL || write_ctx->contents == NULL)
    return -1;

  template_len = strlen(write_ctx->contents);
  if (template_len == 0)
    return 0;
  return fwrite(write_ctx->contents, 1, template_len, fp) == template_len ? 0 : -1;
}

static int CopyStarterFileContents(FILE *fp, const void *user_data) {
  const StarterFileCopyContext *copy_ctx =
      (const StarterFileCopyContext *)user_data;
  char buffer[4096];

  if (fp == NULL || copy_ctx == NULL || copy_ctx->source_fd == -1)
    return -1;

  for (;;) {
    ssize_t read_now = read(copy_ctx->source_fd, buffer, sizeof(buffer));

    if (read_now == 0)
      break;
    if (read_now < 0)
      return -1;
    if (fwrite(buffer, 1, (size_t)read_now, fp) != (size_t)read_now)
      return -1;
  }
  return 0;
}

static int WriteStarterFile(ViewContext *ctx, const char *path,
                            const char *contents, const char *label) {
  StarterFileWriteContext write_ctx;

  if (path == NULL || *path == '\0' || contents == NULL || label == NULL)
    return -1;
  if (access(path, F_OK) == 0)
    return 0;

  write_ctx.contents = contents;
  if (AtomicFileWrite(path, (AtomicFileWriteCallback)WriteStarterFileContents,
                      &write_ctx) != 0) {
    MESSAGE(ctx, "Can't create default %s \"%s\"*%s", label, path,
            strerror(errno));
    return -1;
  }
  return 1;
}

static int IsHomeLegacyProfilePath(const char *profile_path) {
  if (profile_path == NULL || *profile_path == '\0')
    return 0;
  return ConfigPaths_IsLegacyPath(CONFIG_SURFACE_PROFILE, profile_path);
}

static int IsPreferredProfilePath(const char *profile_path) {
  if (profile_path == NULL || *profile_path == '\0')
    return 0;
  return ConfigPaths_IsPreferredPath(CONFIG_SURFACE_PROFILE, profile_path);
}

static int CopyStarterFile(ViewContext *ctx, const char *source_path,
                           const char *target_path, const char *label) {
  StarterFileCopyContext copy_ctx;
  int result;

  if (source_path == NULL || *source_path == '\0' || target_path == NULL ||
      *target_path == '\0' || label == NULL)
    return -1;

  if (access(target_path, F_OK) == 0)
    return 0;

  copy_ctx.source_fd = open(source_path, O_RDONLY);
  if (copy_ctx.source_fd == -1) {
    MESSAGE(ctx, "Can't migrate %s \"%s\"*%s", label, source_path,
            strerror(errno));
    return -1;
  }

  result = AtomicFileWrite(target_path,
                           (AtomicFileWriteCallback)CopyStarterFileContents,
                           &copy_ctx);
  if (close(copy_ctx.source_fd) != 0 && result == 0)
    result = -1;
  if (result != 0) {
    int saved_errno = errno;

    MESSAGE(ctx, "Can't create migrated %s \"%s\"*%s", label, target_path,
            strerror(saved_errno));
    return -1;
  }

  return 1;
}

static int EnsureConfigStarterFile(ViewContext *ctx, const char *profile_path) {
  int result;

  if (profile_path == NULL || *profile_path == '\0')
    return -1;

  if (ctx != NULL && ctx->configuration_file_path[0] != '\0' &&
      !ctx->configuration_file_path_is_explicit &&
      IsHomeLegacyProfilePath(ctx->configuration_file_path) &&
      IsPreferredProfilePath(profile_path)) {
    if (access(ctx->configuration_file_path, F_OK) == 0) {
      result = CopyStarterFile(ctx, ctx->configuration_file_path, profile_path,
                               "config");

      return result < 0 ? -1 : 0;
    }
  }

  if (ctx != NULL) {
    result = CreateProfileFromRuntimeState(ctx, profile_path);
    if (result < 0) {
      MESSAGE(ctx, "Can't create config \"%s\"", profile_path);
      return -1;
    }
    return 0;
  }

  result =
      WriteStarterFile(ctx, profile_path, default_profile_template, "config");
  return result < 0 ? -1 : 0;
}

static int EnsureThemesStarterFile(ViewContext *ctx, const char *themes_path) {
  int result =
      WriteStarterFile(ctx, themes_path, default_theme_catalog, "themes");
  return result < 0 ? -1 : 0;
}

static int EnsureCommandsStarterFile(ViewContext *ctx,
                                     const char *commands_path) {
  int result =
      WriteStarterFile(ctx, commands_path, default_commands_catalog, "commands");
  return result < 0 ? -1 : 0;
}

static int EnsureApplicationsStarterFile(ViewContext *ctx,
                                         const char *applications_path) {
  int result = WriteStarterFile(ctx, applications_path,
                                default_applications_catalog, "applications");
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

static void ResolveProfilePath(const ViewContext *ctx, char *profile_path,
                               size_t profile_path_size) {
  if (ConfigPaths_ResolveActiveEditPath(ctx, CONFIG_SURFACE_PROFILE, profile_path,
                                        profile_path_size) != 0 &&
      profile_path != NULL && profile_path_size > 0)
    profile_path[0] = '\0';
}

static int ResolveThemesPath(const ViewContext *ctx, char *themes_path,
                             size_t themes_path_size) {
  return ConfigPaths_ResolveLoadedOrBootstrapPath(
      ctx, CONFIG_SURFACE_THEME, themes_path, themes_path_size, TRUE);
}

static int ResolveCommandsPath(const ViewContext *ctx, char *commands_path,
                               size_t commands_path_size) {
  return ConfigPaths_ResolveLoadedOrBootstrapPath(
      ctx, CONFIG_SURFACE_COMMANDS, commands_path, commands_path_size, TRUE);
}

static int ResolveApplicationsPath(const ViewContext *ctx,
                                   char *applications_path,
                                   size_t applications_path_size) {
  return ConfigPaths_ResolveLoadedOrBootstrapPath(
      ctx, CONFIG_SURFACE_APPLICATIONS, applications_path,
      applications_path_size, TRUE);
}

static int ReloadConfigAndTheme(ViewContext *ctx, DirEntry *dir_entry,
                                const char *profile_path) {
  ProfileRuntimeSnapshot *profile_snapshot;
  ReloadableProfileState runtime_state;
  char previous_profile_path[PATH_LENGTH + 1];
  BOOL previous_profile_path_is_explicit;
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
  previous_profile_path_is_explicit = ctx->configuration_file_path_is_explicit;
  (void)snprintf(previous_profile_path, sizeof(previous_profile_path), "%s",
                 ctx->configuration_file_path);

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
      (void)snprintf(ctx->configuration_file_path,
                     sizeof(ctx->configuration_file_path), "%s", profile_path);
      ctx->configuration_file_path_is_explicit =
          previous_profile_path_is_explicit &&
          strcmp(previous_profile_path, profile_path) == 0;
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

  if (ctx->core_init_ops.load_commands != NULL &&
      ctx->core_init_ops.load_commands(ctx) != 0) {
    ProfileRuntimeSnapshot_Restore(ctx, profile_snapshot);
    RestoreReloadableProfileState(ctx, dir_entry, &runtime_state);
#ifdef COLOR_SUPPORT
    UIColorSnapshot_Restore(color_snapshot);
    UIColorSnapshot_Free(color_snapshot);
#endif
    ProfileRuntimeSnapshot_Free(profile_snapshot);
    UI_ShowStatusLineError(ctx, "Reload failed: can't load commands");
    return -1;
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

static void EditReloadableRuntimeFile(ViewContext *ctx, DirEntry *dir_entry,
                                      char *file_path) {
  if (Edit(ctx, dir_entry, file_path) != 0) {
    MESSAGE(ctx, "Can't edit \"%s\"", file_path);
    return;
  }

  ReloadConfigAndTheme(ctx, dir_entry, NULL);
}

void UI_EditCommandsCatalog(ViewContext *ctx, DirEntry *dir_entry) {
  char commands_path[PATH_LENGTH + 1];

  if (ResolveCommandsPath(ctx, commands_path, sizeof(commands_path)) != 0) {
    MESSAGE(ctx, "Can't resolve commands file path");
    return;
  }
  if (EnsureCommandsStarterFile(ctx, commands_path) != 0)
    return;

  EditReloadableRuntimeFile(ctx, dir_entry, commands_path);
}

void UI_EditApplicationsCatalog(ViewContext *ctx, DirEntry *dir_entry) {
  char applications_path[PATH_LENGTH + 1];

  if (ResolveApplicationsPath(ctx, applications_path, sizeof(applications_path)) !=
      0) {
    MESSAGE(ctx, "Can't resolve applications file path");
    return;
  }
  if (EnsureApplicationsStarterFile(ctx, applications_path) != 0)
    return;

  EditReloadableRuntimeFile(ctx, dir_entry, applications_path);
}

void UI_OpenConfigProfile(ViewContext *ctx, DirEntry *dir_entry) {
  char profile_path[PATH_LENGTH + 1];
  int term;

  ResolveProfilePath(ctx, profile_path, sizeof(profile_path));

  term = InputChoiceCommandStrip(
      ctx, config_command_strip,
      sizeof(config_command_strip) / sizeof(config_command_strip[0]),
      "CMTRQ\r\n\033");

  switch (term) {
  case CR:
  case LF:
  case 'C':
    if (EnsureConfigStarterFile(ctx, profile_path) != 0)
      break;
    EditConfigProfile(ctx, dir_entry, profile_path);
    break;
  case 'M':
    UI_EditCommandsCatalog(ctx, dir_entry);
    break;
  case 'T':
    {
      char themes_path[PATH_LENGTH + 1];

      if (ResolveThemesPath(ctx, themes_path, sizeof(themes_path)) != 0) {
        MESSAGE(ctx, "Can't resolve themes file path");
        break;
      }
      if (EnsureThemesStarterFile(ctx, themes_path) != 0)
        break;
      EditReloadableRuntimeFile(ctx, dir_entry, themes_path);
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
