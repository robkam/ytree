/***************************************************************************
 *
 * src/cmd/config_paths.c
 * Shared config-family path resolution helpers.
 *
 ***************************************************************************/

#include "ytnova_defs.h"
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

typedef struct {
  ConfigSurface surface;
  const char *preferred_path;
  const char *legacy_filename;
} ConfigSurfacePathSpec;

static const ConfigSurfacePathSpec kConfigSurfacePaths[] = {
    {CONFIG_SURFACE_PROFILE, PROFILE_CONFIG_HOME_PATH, PROFILE_FILENAME},
    {CONFIG_SURFACE_THEME, THEME_CONFIG_HOME_PATH, THEME_FILENAME},
    {CONFIG_SURFACE_COMMANDS, COMMANDS_CONFIG_HOME_PATH, COMMANDS_FILENAME},
    {CONFIG_SURFACE_APPLICATIONS, APPLICATIONS_CONFIG_HOME_PATH,
        APPLICATIONS_FILENAME},
};

static const ConfigSurfacePathSpec *ConfigPaths_FindSpec(ConfigSurface surface) {
  size_t index;

  for (index = 0; index < sizeof(kConfigSurfacePaths) / sizeof(kConfigSurfacePaths[0]);
       ++index) {
    if (kConfigSurfacePaths[index].surface == surface)
      return &kConfigSurfacePaths[index];
  }
  return NULL;
}

static int ConfigPaths_JoinHomePath(char *path, size_t path_size,
                                    const char *home, const char *suffix) {
  int written;

  if (path == NULL || path_size == 0 || home == NULL || *home == '\0' ||
      suffix == NULL || *suffix == '\0')
    return -1;

  written = snprintf(path, path_size, "%s/%s", home, suffix);
  if (written < 0 || written >= (int)path_size) {
    path[0] = '\0';
    return -1;
  }
  return 0;
}

static const char *ConfigPaths_CurrentPath(const ViewContext *ctx,
                                           ConfigSurface surface) {
  if (ctx == NULL)
    return NULL;

  switch (surface) {
  case CONFIG_SURFACE_PROFILE:
    return ctx->configuration_file_path;
  case CONFIG_SURFACE_THEME:
    return ctx->theme_file_path;
  case CONFIG_SURFACE_COMMANDS:
    return ctx->commands_file_path;
  case CONFIG_SURFACE_APPLICATIONS:
    return NULL;
  default:
    return NULL;
  }
}

static BOOL ConfigPaths_UsesXdgConfigHome(void) {
  const char *xdg_config_home;

  xdg_config_home = getenv("XDG_CONFIG_HOME");
  return xdg_config_home != NULL && xdg_config_home[0] == '/';
}

static const char *ConfigPaths_ConfigHome(const char *home) {
  if (ConfigPaths_UsesXdgConfigHome())
    return getenv("XDG_CONFIG_HOME");
  return home;
}

int ConfigPaths_EnsureHomeDirectory(const char *home) {
  char config_dir[PATH_LENGTH + 1];
  char ytnova_dir[PATH_LENGTH + 1];
  struct stat st;

  if (ConfigPaths_UsesXdgConfigHome()) {
    const char *config_home = ConfigPaths_ConfigHome(home);
    int written;

    written = snprintf(config_dir, sizeof(config_dir), "%s", config_home);
    if (written < 0 || written >= (int)sizeof(config_dir))
      return -1;
  } else if (home == NULL || *home == '\0' ||
             ConfigPaths_JoinHomePath(config_dir, sizeof(config_dir), home,
                                      PROFILE_CONFIG_HOME_PARENT) != 0) {
    return -1;
  }
  if (mkdir(config_dir, S_IRWXU) != 0) {
    if (errno != EEXIST || stat(config_dir, &st) != 0 ||
        !S_ISDIR(st.st_mode))
      return -1;
  }

  if (ConfigPaths_JoinHomePath(ytnova_dir, sizeof(ytnova_dir), config_dir,
                               "ytnova") != 0)
    return -1;
  if (mkdir(ytnova_dir, S_IRWXU) != 0) {
    if (errno != EEXIST || stat(ytnova_dir, &st) != 0 ||
        !S_ISDIR(st.st_mode))
      return -1;
  }

  return 0;
}

int ConfigPaths_ResolvePreferredPath(ConfigSurface surface, char *path,
                                     size_t path_size) {
  const ConfigSurfacePathSpec *spec;
  const char *home;

  if (path == NULL || path_size == 0)
    return -1;

  path[0] = '\0';
  spec = ConfigPaths_FindSpec(surface);
  if (spec == NULL)
    return -1;

  home = getenv("HOME");
  if (!ConfigPaths_UsesXdgConfigHome() && (home == NULL || *home == '\0'))
    return -1;
  if (ConfigPaths_EnsureHomeDirectory(home) != 0)
    return -1;
  if (ConfigPaths_UsesXdgConfigHome()) {
    const char *config_home = ConfigPaths_ConfigHome(home);
    const char *xdg_relative_path = strchr(spec->preferred_path, '/');

    if (xdg_relative_path == NULL || xdg_relative_path[1] == '\0')
      return -1;
    return ConfigPaths_JoinHomePath(path, path_size, config_home,
                                    xdg_relative_path + 1);
  }
  return ConfigPaths_JoinHomePath(path, path_size, home, spec->preferred_path);
}

int ConfigPaths_ResolveLegacyPath(ConfigSurface surface, char *path,
                                  size_t path_size, BOOL allow_cwd_fallback) {
  const ConfigSurfacePathSpec *spec;
  const char *home;
  int written;

  if (path == NULL || path_size == 0)
    return -1;

  path[0] = '\0';
  spec = ConfigPaths_FindSpec(surface);
  if (spec == NULL)
    return -1;

  home = getenv("HOME");
  if (home != NULL && *home != '\0') {
    if (ConfigPaths_JoinHomePath(path, path_size, home, spec->legacy_filename) ==
        0)
      return 0;
  }
  if (!allow_cwd_fallback)
    return -1;

  written = snprintf(path, path_size, "%s", spec->legacy_filename);
  if (written < 0 || written >= (int)path_size) {
    path[0] = '\0';
    return -1;
  }
  return 0;
}

int ConfigPaths_ResolveBootstrapPath(ConfigSurface surface, char *path,
                                     size_t path_size,
                                     BOOL allow_cwd_fallback) {
  if (ConfigPaths_ResolvePreferredPath(surface, path, path_size) == 0)
    return 0;
  return ConfigPaths_ResolveLegacyPath(surface, path, path_size,
                                       allow_cwd_fallback);
}

int ConfigPaths_IsPreferredPath(ConfigSurface surface, const char *path) {
  char expected[PATH_LENGTH + 1];

  if (path == NULL || *path == '\0')
    return 0;
  if (ConfigPaths_ResolvePreferredPath(surface, expected, sizeof(expected)) != 0)
    return 0;
  return strcmp(path, expected) == 0;
}

int ConfigPaths_IsLegacyPath(ConfigSurface surface, const char *path) {
  char expected[PATH_LENGTH + 1];

  if (path == NULL || *path == '\0')
    return 0;
  if (ConfigPaths_ResolveLegacyPath(surface, expected, sizeof(expected), FALSE) !=
      0)
    return 0;
  return strcmp(path, expected) == 0;
}

int ConfigPaths_ResolveLoadedOrBootstrapPath(const ViewContext *ctx,
                                             ConfigSurface surface, char *path,
                                             size_t path_size,
                                             BOOL allow_cwd_fallback) {
  const char *current_path;

  if (path == NULL || path_size == 0)
    return -1;

  current_path = ConfigPaths_CurrentPath(ctx, surface);
  if (current_path != NULL && *current_path != '\0') {
    int written;
    written = snprintf(path, path_size, "%s", current_path);
    if (written >= 0 && written < (int)path_size)
      return 0;
    path[0] = '\0';
    return -1;
  }

  return ConfigPaths_ResolveBootstrapPath(surface, path, path_size,
                                          allow_cwd_fallback);
}

int ConfigPaths_ResolveActiveEditPath(const ViewContext *ctx,
                                      ConfigSurface surface, char *path,
                                      size_t path_size) {
  int written;

  if (path == NULL || path_size == 0)
    return -1;

  if (surface == CONFIG_SURFACE_PROFILE && ctx != NULL &&
      ctx->configuration_file_path[0] != '\0' &&
      ctx->configuration_file_path_is_explicit) {
    written = snprintf(path, path_size, "%s", ctx->configuration_file_path);
    if (written >= 0 && written < (int)path_size)
      return 0;
    path[0] = '\0';
    return -1;
  }

  if (surface == CONFIG_SURFACE_PROFILE) {
    if (ConfigPaths_ResolvePreferredPath(surface, path, path_size) == 0)
      return 0;
    if (ctx != NULL && ctx->configuration_file_path[0] != '\0') {
      written = snprintf(path, path_size, "%s", ctx->configuration_file_path);
      if (written >= 0 && written < (int)path_size)
        return 0;
      path[0] = '\0';
      return -1;
    }
    return ConfigPaths_ResolveLegacyPath(surface, path, path_size, TRUE);
  }

  return ConfigPaths_ResolveLoadedOrBootstrapPath(ctx, surface, path, path_size,
                                                  TRUE);
}
