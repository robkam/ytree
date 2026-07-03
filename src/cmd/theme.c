/***************************************************************************
 *
 * src/cmd/theme.c
 * Semantic theme loading
 *
 ***************************************************************************/

#include "config.h"
#include "ytnova_cmd.h"

#define THEME_STYLE_LENGTH 128
#define THEME_ROLE_COUNT 16

typedef enum {
  THEME_SECTION_NONE = 0,
  THEME_SECTION_ROLES,
  THEME_SECTION_FILE_TYPES
} ThemeSection;

typedef struct {
  char name[32];
  char value[THEME_STYLE_LENGTH];
  BOOL is_set;
} ThemeRoleValue;

typedef struct {
  const char *role;
  const char *legacy_names[8];
} ThemeMigrationRoleShim;

static const char *required_roles[THEME_ROLE_COUNT] = {
    "background",  "box_lines", "tree_lines",  "margin",
    "static_text", "dynamic_text", "keybind",   "selection",
    "dialog",      "picker",    "help",        "info",
    "warning",     "error",     "search_hit",  "disabled"};

/* Migration-only bridge: theme files expose semantic roles while current
   render paths still consume legacy color-pair names. */
static const ThemeMigrationRoleShim migration_role_shims[] = {
    {"box_lines", {"BORDERS_COLOR", NULL}},
    {"static_text", {"MENU_COLOR", NULL}},
    {"dynamic_text",
     {"DIR_COLOR", "WINDIR_COLOR", "FILE_COLOR", "WINFILE_COLOR",
      "STATS_COLOR", "WINSTATS_COLOR", NULL}},
    {"keybind", {"HIMENUS_COLOR", NULL}},
    {"selection", {"HIDIR_COLOR", "HIFILE_COLOR", "HIHST_COLOR", NULL}},
    {"dialog", {"DIALOG_COLOR", NULL}},
    {"help", {"HELP_COLOR", NULL}},
    {"picker", {"HST_COLOR", "WINHST_COLOR", NULL}},
    {"info", {"INFO_COLOR", NULL}},
    {"warning", {"WARN_COLOR", NULL}},
    {"error", {"ERR_COLOR", NULL}},
    {"search_hit", {"GLOBAL_COLOR", "HIGLOBAL_COLOR", NULL}},
    {NULL, {NULL}}};

static char *TrimInPlace(char *text);
static BOOL SplitAssignment(char *line, char **name, char **value);
static BOOL SectionMatches(const char *line, const char *prefix,
                           const char *theme_name);
static ThemeSection ParseSection(const char *line, const char *theme_name);
static ThemeRoleValue *FindRole(ThemeRoleValue *roles, const char *name);
static const char *ResolveRoleStyle(ThemeRoleValue *roles, const char *value,
                                    int depth);
static BOOL ParseThemeStyle(ViewContext *ctx, ThemeRoleValue *roles,
                            const char *value, int background, int *fg,
                            int *bg);
static int ThemeBackground(ViewContext *ctx, ThemeRoleValue *roles);
static void ApplyThemeRoles(ViewContext *ctx, ThemeRoleValue *roles);
static void ApplyMigrationRoleShim(ViewContext *ctx, const char *role, int fg,
                                   int bg);
static BOOL BuildFileColorPattern(const char *selector, char *pattern,
                                  size_t pattern_size);
static void AddCompactFileColorRules(ViewContext *ctx, char *value);
static int TryConfiguredThemePath(ViewContext *ctx, char *path,
                                  size_t path_size, const char *home,
                                  const char *suffix,
                                  const char *theme_name);
static int CoreInit_LoadTheme(ViewContext *ctx);

static char *TrimInPlace(char *text) {
  char *end;

  if (text == NULL)
    return NULL;

  while (*text && isspace((unsigned char)*text))
    ++text;

  end = text + strlen(text);
  while (end > text && isspace((unsigned char)end[-1]))
    *--end = '\0';

  return text;
}

static BOOL SplitAssignment(char *line, char **name, char **value) {
  char *eq;

  if (line == NULL || name == NULL || value == NULL)
    return FALSE;

  eq = strchr(line, '=');
  if (eq == NULL)
    return FALSE;

  *eq = '\0';
  *name = TrimInPlace(line);
  *value = TrimInPlace(eq + 1);

  return *name != NULL && **name != '\0' && *value != NULL && **value != '\0';
}

static BOOL SectionMatches(const char *line, const char *prefix,
                           const char *theme_name) {
  size_t prefix_len;
  size_t theme_len;

  if (line == NULL || prefix == NULL || theme_name == NULL)
    return FALSE;

  prefix_len = strlen(prefix);
  theme_len = strlen(theme_name);
  if (line[0] != '[' || strncmp(line + 1, prefix, prefix_len) != 0)
    return FALSE;
  if (line[1 + prefix_len] != ' ')
    return FALSE;

  return strncmp(line + 2 + prefix_len, theme_name, theme_len) == 0 &&
         line[2 + prefix_len + theme_len] == ']';
}

static ThemeSection ParseSection(const char *line, const char *theme_name) {
  if (SectionMatches(line, "theme", theme_name))
    return THEME_SECTION_ROLES;
  if (SectionMatches(line, "file-types", theme_name))
    return THEME_SECTION_FILE_TYPES;
  if (line != NULL && line[0] == '[')
    return THEME_SECTION_NONE;
  return THEME_SECTION_NONE;
}

static ThemeRoleValue *FindRole(ThemeRoleValue *roles, const char *name) {
  int i;

  if (roles == NULL || name == NULL)
    return NULL;

  for (i = 0; i < THEME_ROLE_COUNT; ++i) {
    if (strcmp(roles[i].name, name) == 0)
      return &roles[i];
  }

  return NULL;
}

static const char *ResolveRoleStyle(ThemeRoleValue *roles, const char *value,
                                    int depth) {
  ThemeRoleValue *role;

  if (value == NULL || depth > THEME_ROLE_COUNT)
    return value;

  role = FindRole(roles, value);
  if (role == NULL || !role->is_set)
    return value;

  return ResolveRoleStyle(roles, role->value, depth + 1);
}

static BOOL ParseThemeStyle(ViewContext *ctx, ThemeRoleValue *roles,
                            const char *value, int background, int *fg,
                            int *bg) {
  const char *style;

  if (ctx == NULL || ctx->hook_parse_color == NULL || value == NULL ||
      fg == NULL || bg == NULL)
    return FALSE;

  style = ResolveRoleStyle(roles, value, 0);
  *fg = -1;
  *bg = -1;
  ctx->hook_parse_color(style, fg, bg);
  if (*fg == -1)
    return FALSE;
  if (*bg == -1)
    *bg = background;
  return TRUE;
}

static int ThemeBackground(ViewContext *ctx, ThemeRoleValue *roles) {
  ThemeRoleValue *background_role;
  int fg = -1;
  int bg = -1;

  background_role = FindRole(roles, "background");
  if (background_role == NULL || !background_role->is_set ||
      ctx->hook_parse_color == NULL)
    return COLOR_BLACK;

  ctx->hook_parse_color(background_role->value, &fg, &bg);
  return (fg == -1) ? COLOR_BLACK : fg;
}

static void ApplyThemeRoles(ViewContext *ctx, ThemeRoleValue *roles) {
  int i;
  int background;

  if (ctx == NULL || roles == NULL)
    return;

  background = ThemeBackground(ctx, roles);

  for (i = 0; i < THEME_ROLE_COUNT; ++i) {
    int fg;
    int bg;

    if (!roles[i].is_set || strcmp(roles[i].name, "background") == 0)
      continue;

    if (ParseThemeStyle(ctx, roles, roles[i].value, background, &fg, &bg))
      ApplyMigrationRoleShim(ctx, roles[i].name, fg, bg);
  }
}

static void ApplyMigrationRoleShim(ViewContext *ctx, const char *role, int fg,
                                   int bg) {
  int i;
  int j;

  if (ctx == NULL || role == NULL || ctx->hook_update_ui_color == NULL)
    return;

  for (i = 0; migration_role_shims[i].role != NULL; ++i) {
    if (strcmp(migration_role_shims[i].role, role) != 0)
      continue;

    for (j = 0; migration_role_shims[i].legacy_names[j] != NULL; ++j)
      ctx->hook_update_ui_color(migration_role_shims[i].legacy_names[j], fg,
                                bg);
    return;
  }
}

static BOOL BuildFileColorPattern(const char *selector, char *pattern,
                                  size_t pattern_size) {
  int written;

  if (selector == NULL || pattern == NULL || pattern_size == 0 ||
      *selector == '\0')
    return FALSE;

  if (strcasecmp(selector, "LINK") == 0) {
    written = snprintf(pattern, pattern_size, "%s", "LINK");
  } else if (strcasecmp(selector, "EXEC") == 0) {
    written = snprintf(pattern, pattern_size, "%s", "EXEC");
  } else if (strcasecmp(selector, "DIR") == 0) {
    written = snprintf(pattern, pattern_size, "%s", "DIR");
  } else if (strchr(selector, '*') != NULL || strchr(selector, '?') != NULL) {
    written = snprintf(pattern, pattern_size, "%s", selector);
  } else {
    written = snprintf(pattern, pattern_size, "*.%s", selector);
  }

  return written >= 0 && (size_t)written < pattern_size;
}

static void AddCompactFileColorRules(ViewContext *ctx, char *value) {
  char *colon;
  char *style;
  char *selectors;
  char *saveptr;
  char *selector;
  int fg = -1;
  int bg = -1;

  if (ctx == NULL || value == NULL || ctx->hook_parse_color == NULL ||
      ctx->hook_add_file_color_rule == NULL)
    return;

  colon = strchr(value, ':');
  if (colon == NULL)
    return;

  *colon = '\0';
  style = TrimInPlace(value);
  selectors = TrimInPlace(colon + 1);
  if (style == NULL || selectors == NULL || *style == '\0' ||
      *selectors == '\0')
    return;

  ctx->hook_parse_color(style, &fg, &bg);
  if (fg == -1)
    return;

  selector = strtok_r(selectors, ",", &saveptr);
  while (selector != NULL) {
    char pattern[FILE_SPEC_LENGTH + 1];
    char *trimmed = TrimInPlace(selector);

    if (BuildFileColorPattern(trimmed, pattern, sizeof(pattern)))
      ctx->hook_add_file_color_rule(ctx, pattern, fg, bg);

    selector = strtok_r(NULL, ",", &saveptr);
  }
}

int ReadThemeFile(ViewContext *ctx, const char *filename,
                  const char *theme_name) {
  FILE *fp;
  char buffer[2048];
  ThemeRoleValue roles[THEME_ROLE_COUNT];
  ThemeSection section = THEME_SECTION_NONE;
  BOOL found_theme = FALSE;
  BOOL roles_applied = FALSE;
  int i;

  if (ctx == NULL || filename == NULL || theme_name == NULL || *theme_name == '\0')
    return -1;

  fp = fopen(filename, "r");
  if (fp == NULL)
    return -1;

  memset(roles, 0, sizeof(roles));
  for (i = 0; i < THEME_ROLE_COUNT; ++i)
    snprintf(roles[i].name, sizeof(roles[i].name), "%s", required_roles[i]);

  while (fgets(buffer, sizeof(buffer), fp) != NULL) {
    char *comment;
    char *line;
    char *name;
    char *value;

    comment = strchr(buffer, '#');
    if (comment != NULL)
      *comment = '\0';

    line = TrimInPlace(buffer);
    if (line == NULL || *line == '\0')
      continue;

    if (*line == '[') {
      ThemeSection new_section = ParseSection(line, theme_name);

      if (found_theme && !roles_applied && section == THEME_SECTION_ROLES &&
          new_section != THEME_SECTION_ROLES) {
        ApplyThemeRoles(ctx, roles);
        roles_applied = TRUE;
      }
      section = new_section;
      if (section == THEME_SECTION_ROLES)
        found_theme = TRUE;
      continue;
    }

    if (section == THEME_SECTION_ROLES) {
      ThemeRoleValue *role;

      if (!SplitAssignment(line, &name, &value))
        continue;
      role = FindRole(roles, name);
      if (role == NULL)
        continue;
      snprintf(role->value, sizeof(role->value), "%s", value);
      role->is_set = TRUE;
    } else if (section == THEME_SECTION_FILE_TYPES) {
      if (!SplitAssignment(line, &name, &value))
        continue;
      (void)name;
      AddCompactFileColorRules(ctx, value);
    }
  }

  fclose(fp);

  if (!found_theme)
    return -1;

  if (!roles_applied)
    ApplyThemeRoles(ctx, roles);
  return 0;
}

static int TryConfiguredThemePath(ViewContext *ctx, char *path,
                                  size_t path_size, const char *home,
                                  const char *suffix,
                                  const char *theme_name) {
  int written;

  if (home == NULL || *home == '\0')
    return -1;

  written = snprintf(path, path_size, "%s%c%s", home, FILE_SEPARATOR_CHAR,
                     suffix);
  if (written < 0 || (size_t)written >= path_size)
    return -1;

  return ReadThemeFile(ctx, path, theme_name);
}

int LoadConfiguredTheme(ViewContext *ctx) {
  const char *theme_name;
  const char *home;
  char path[PATH_LENGTH + 1];

  if (ctx == NULL)
    return -1;

  theme_name = "classic-blue";
  if (ctx->core_init_ops.get_profile_value != NULL) {
    const char *configured = ctx->core_init_ops.get_profile_value(ctx, "THEME");
    if (configured != NULL && *configured != '\0')
      theme_name = configured;
  }

  home = getenv("HOME");
  if (TryConfiguredThemePath(ctx, path, sizeof(path), home,
                             ".config/ytnova/themes.conf", theme_name) == 0)
    return 0;
  if (TryConfiguredThemePath(ctx, path, sizeof(path), home, ".ytnova.themes",
                             theme_name) == 0)
    return 0;

  return ReadThemeFile(ctx, "etc/ytnova.themes", theme_name);
}

static int CoreInit_LoadTheme(ViewContext *ctx) {
  return LoadConfiguredTheme(ctx);
}

void CoreInitOps_RegisterCmdTheme(CoreInitOps *ops) {
  if (ops == NULL)
    return;

  ops->load_theme = CoreInit_LoadTheme;
}
