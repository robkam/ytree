/***************************************************************************
 *
 * src/cmd/theme.c
 * Semantic theme loading
 *
 ***************************************************************************/

#include "../core/default_theme_catalog.h"
#include "config.h"
#include "ytnova_cmd.h"
#include "ytnova_ui.h"
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>

#define THEME_STYLE_LENGTH 128
#define THEME_ROLE_COUNT 16

typedef enum {
  THEME_SECTION_NONE = 0,
  THEME_SECTION_ROLES,
  THEME_SECTION_FILE_TYPES
} ThemeSection;

typedef enum {
  THEME_LOAD_INVALID = -1,
  THEME_LOAD_OK = 0,
  THEME_LOAD_NOT_FOUND = 1
} ThemeLoadStatus;

typedef struct {
  char name[32];
  char value[THEME_STYLE_LENGTH];
  BOOL is_set;
} ThemeRoleValue;

typedef struct _theme_palette_line {
  char value[2048];
  struct _theme_palette_line *next;
} ThemePaletteLine;

static const char *required_roles[THEME_ROLE_COUNT] = {
    "background",  "box_lines", "tree_lines",  "margin",
    "static_text", "dynamic_text", "keybind",   "selection",
    "dialog",      "picker",    "help",        "info",
    "warning",     "error",     "search_hit",  "disabled"};

static char *TrimInPlace(char *text);
static BOOL SplitAssignment(char *line, char **name, char **value);
static BOOL SectionMatches(const char *line, const char *prefix,
                           const char *theme_name);
static ThemeSection ParseSection(const char *line, const char *theme_name);
static ThemeRoleValue *FindRole(ThemeRoleValue *roles, const char *name);
static BOOL CopyThemeValueStrict(char *dest, size_t dest_size,
                                 const char *value);
static const char *ResolveRoleStyle(ThemeRoleValue *roles, const char *value,
                                    int depth);
static BOOL ParseThemeStyle(ViewContext *ctx, ThemeRoleValue *roles,
                            const char *value, int background, int *fg,
                            int *bg);
static int ThemeBackground(ViewContext *ctx, ThemeRoleValue *roles);
static void ApplyThemeRoles(ViewContext *ctx, ThemeRoleValue *roles);
static void ApplySemanticRole(ViewContext *ctx, const char *role, int fg,
                              int bg);
static BOOL BuildFileColorPattern(const char *selector, char *pattern,
                                  size_t pattern_size);
static BOOL ParseCompactFileColorRules(ViewContext *ctx, char *value,
                                       ViewContext *target_ctx);
static ThemePaletteLine *AppendThemePaletteLine(ThemePaletteLine **head,
                                                ThemePaletteLine **tail,
                                                const char *value);
static void FreeThemePaletteLines(ThemePaletteLine *head);
static BOOL ValidateThemeRoles(ViewContext *ctx, ThemeRoleValue *roles);
static BOOL ValidateThemePaletteLines(ViewContext *ctx,
                                      ThemePaletteLine *head);
static BOOL StageThemePaletteLines(ViewContext *ctx, ThemePaletteLine *head,
                                   ViewContext *target_ctx);
static void FreeThemeFileColorRules(FileColorRule *rule);
static ThemeLoadStatus ReadThemeStreamInternal(ViewContext *ctx, FILE *fp,
                                               const char *theme_name);
static ThemeLoadStatus ReadThemeFileInternal(ViewContext *ctx,
                                             const char *filename,
                                             const char *theme_name);
static int ThemeWriteAll(int fd, const char *buf, size_t len);
static int EnsureThemeConfigHomeDirectory(const char *home);
static int ResolveSeedThemePath(char *path, size_t path_size,
                                const char *home);
static int SeedConfiguredThemePath(const char *path);
static ThemeLoadStatus ReadCompiledThemeCatalog(ViewContext *ctx,
                                                const char *theme_name);
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

static BOOL CopyThemeValueStrict(char *dest, size_t dest_size,
                                 const char *value) {
  size_t len;

  if (dest == NULL || dest_size == 0 || value == NULL)
    return FALSE;

  len = strlen(value);
  if (len >= dest_size)
    return FALSE;

  memcpy(dest, value, len + 1);
  return TRUE;
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

  if (ctx == NULL || value == NULL || fg == NULL || bg == NULL)
    return FALSE;

  style = ResolveRoleStyle(roles, value, 0);
#ifndef COLOR_SUPPORT
  (void)background;
  *fg = -1;
  *bg = -1;
  return style != NULL && *style != '\0';
#else
  *fg = -1;
  *bg = -1;
  if (!ParseColorStringStrict(style, fg, bg))
    return FALSE;
  if (*fg == -1)
    return FALSE;
  if (*bg == -1)
    *bg = background;
  return TRUE;
#endif
}

static int ThemeBackground(ViewContext *ctx, ThemeRoleValue *roles) {
  ThemeRoleValue *background_role;
  int fg = -1;
  int bg = -1;

  background_role = FindRole(roles, "background");
  if (background_role == NULL || !background_role->is_set)
    return COLOR_BLACK;

  (void)ParseColorStringStrict(background_role->value, &fg, &bg);
  if (bg != -1)
    return bg;
  return (fg == -1) ? COLOR_BLACK : fg;
}

static void ApplyThemeRoles(ViewContext *ctx, ThemeRoleValue *roles) {
#ifndef COLOR_SUPPORT
  (void)ctx;
  (void)roles;
  return;
#else
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
      ApplySemanticRole(ctx, roles[i].name, fg, bg);
  }
#endif
}

static void ApplySemanticRole(ViewContext *ctx, const char *role, int fg,
                              int bg) {
  if (ctx == NULL || role == NULL || ctx->hook_update_ui_color == NULL)
    return;

  ctx->hook_update_ui_color(role, fg, bg);
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
  } else if (strchr(selector, '*') != NULL || strchr(selector, '?') != NULL) {
    return FALSE;
  } else {
    written = snprintf(pattern, pattern_size, "*.%s", selector);
  }

  return written >= 0 && (size_t)written < pattern_size;
}

static BOOL ParseCompactFileColorRules(ViewContext *ctx, char *value,
                                       ViewContext *target_ctx) {
  char *colon;
  char *style;
  char *selectors;
  char *saveptr;
  char *selector;
#ifdef COLOR_SUPPORT
  int fg = -1;
  int bg = -1;
#endif
  BOOL added = FALSE;

  if (ctx == NULL || value == NULL)
    return FALSE;
  if (target_ctx != NULL && target_ctx->hook_add_file_color_rule == NULL)
    return FALSE;

  colon = strchr(value, ':');
  if (colon == NULL)
    return FALSE;

  *colon = '\0';
  style = TrimInPlace(value);
  selectors = TrimInPlace(colon + 1);
  if (style == NULL || selectors == NULL || *style == '\0' ||
      *selectors == '\0')
    return FALSE;

#ifdef COLOR_SUPPORT
  if (!ParseColorStringStrict(style, &fg, &bg) || fg == -1)
    return FALSE;
#endif

  selector = strtok_r(selectors, ",", &saveptr);
  while (selector != NULL) {
    char pattern[FILE_SPEC_LENGTH + 1];
    char *trimmed = TrimInPlace(selector);

    if (!BuildFileColorPattern(trimmed, pattern, sizeof(pattern)))
      return FALSE;
#ifdef COLOR_SUPPORT
    if (target_ctx != NULL)
      target_ctx->hook_add_file_color_rule(target_ctx, pattern, fg, bg);
#else
    (void)target_ctx;
#endif
    added = TRUE;

    selector = strtok_r(NULL, ",", &saveptr);
  }

  return added;
}

static ThemePaletteLine *AppendThemePaletteLine(ThemePaletteLine **head,
                                                ThemePaletteLine **tail,
                                                const char *value) {
  ThemePaletteLine *line;

  if (head == NULL || tail == NULL || value == NULL)
    return NULL;

  line = xmalloc(sizeof(*line));
  if (!CopyThemeValueStrict(line->value, sizeof(line->value), value)) {
    free(line);
    return NULL;
  }
  line->next = NULL;

  if (*head == NULL) {
    *head = line;
  } else {
    (*tail)->next = line;
  }
  *tail = line;

  return line;
}

static void FreeThemePaletteLines(ThemePaletteLine *head) {
  while (head != NULL) {
    ThemePaletteLine *next = head->next;
    free(head);
    head = next;
  }
}

static BOOL ValidateThemeRoles(ViewContext *ctx, ThemeRoleValue *roles) {
  ThemeRoleValue *background_role;
  int i;
#ifdef COLOR_SUPPORT
  int background;
  int background_fg = -1;
  int background_bg = -1;
#endif

  if (ctx == NULL || roles == NULL)
    return FALSE;

  for (i = 0; i < THEME_ROLE_COUNT; ++i) {
    if (!roles[i].is_set)
      return FALSE;
  }

  background_role = FindRole(roles, "background");
  if (background_role == NULL)
    return FALSE;
#ifndef COLOR_SUPPORT
  (void)ctx;
  return TRUE;
#else
  if (!ParseColorStringStrict(background_role->value, &background_fg,
                              &background_bg))
    return FALSE;
  if (background_fg == -1)
    return FALSE;

  background = ThemeBackground(ctx, roles);

  for (i = 0; i < THEME_ROLE_COUNT; ++i) {
    int fg;
    int bg;

    if (strcmp(roles[i].name, "background") == 0)
      continue;
    if (!ParseThemeStyle(ctx, roles, roles[i].value, background, &fg, &bg))
      return FALSE;
  }

  return TRUE;
#endif
}

static BOOL ValidateThemePaletteLines(ViewContext *ctx,
                                      ThemePaletteLine *head) {
  ThemePaletteLine *line;

  for (line = head; line != NULL; line = line->next) {
    char value[sizeof(line->value)];

    snprintf(value, sizeof(value), "%s", line->value);
    if (!ParseCompactFileColorRules(ctx, value, NULL))
      return FALSE;
  }

  return TRUE;
}

static BOOL StageThemePaletteLines(ViewContext *ctx, ThemePaletteLine *head,
                                   ViewContext *target_ctx) {
  ThemePaletteLine *line;

  for (line = head; line != NULL; line = line->next) {
    char value[sizeof(line->value)];

    snprintf(value, sizeof(value), "%s", line->value);
    if (!ParseCompactFileColorRules(ctx, value, target_ctx))
      return FALSE;
  }

  return TRUE;
}

static void FreeThemeFileColorRules(FileColorRule *rule) {
  while (rule != NULL) {
    FileColorRule *next = rule->next;

    free(rule->pattern);
    free(rule);
    rule = next;
  }
}

static ThemeLoadStatus ReadThemeStreamInternal(ViewContext *ctx, FILE *fp,
                                               const char *theme_name) {
  char buffer[2048];
  ThemeRoleValue roles[THEME_ROLE_COUNT];
  ThemeSection section = THEME_SECTION_NONE;
  ThemePaletteLine *palette_head = NULL;
  ThemePaletteLine *palette_tail = NULL;
  ViewContext staging_ctx;
#ifdef COLOR_SUPPORT
  UIColorSnapshot *color_snapshot;
#endif
  FileColorRule *old_file_rules;
  BOOL found_theme = FALSE;
  BOOL invalid_theme = FALSE;
  BOOL read_failed;
  int i;

  if (ctx == NULL || fp == NULL || theme_name == NULL || *theme_name == '\0')
    return THEME_LOAD_INVALID;

  memset(roles, 0, sizeof(roles));
  for (i = 0; i < THEME_ROLE_COUNT; ++i)
    snprintf(roles[i].name, sizeof(roles[i].name), "%s", required_roles[i]);
  {
    ThemeRoleValue *margin_role = FindRole(roles, "margin");

    if (margin_role != NULL) {
      snprintf(margin_role->value, sizeof(margin_role->value), "%s",
               "dynamic_text");
      margin_role->is_set = TRUE;
    }
  }

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

      section = new_section;
      if (section == THEME_SECTION_ROLES)
        found_theme = TRUE;
      continue;
    }

    if (section == THEME_SECTION_ROLES) {
      ThemeRoleValue *role;

      if (!SplitAssignment(line, &name, &value)) {
        invalid_theme = TRUE;
        break;
      }
      role = FindRole(roles, name);
      if (role == NULL) {
        invalid_theme = TRUE;
        break;
      }
      if (!CopyThemeValueStrict(role->value, sizeof(role->value), value)) {
        invalid_theme = TRUE;
        break;
      }
      role->is_set = TRUE;
    } else if (section == THEME_SECTION_FILE_TYPES) {
      if (!SplitAssignment(line, &name, &value)) {
        invalid_theme = TRUE;
        break;
      }
      (void)name;
      if (AppendThemePaletteLine(&palette_head, &palette_tail, value) == NULL) {
        invalid_theme = TRUE;
        break;
      }
    }
  }

  read_failed = ferror(fp) ? TRUE : FALSE;
  if (read_failed) {
    FreeThemePaletteLines(palette_head);
    return THEME_LOAD_INVALID;
  }

  if (!found_theme) {
    FreeThemePaletteLines(palette_head);
    return THEME_LOAD_NOT_FOUND;
  }

  if (invalid_theme || !ValidateThemeRoles(ctx, roles) ||
      !ValidateThemePaletteLines(ctx, palette_head)) {
    FreeThemePaletteLines(palette_head);
    return THEME_LOAD_INVALID;
  }

#ifndef COLOR_SUPPORT
  FreeThemePaletteLines(palette_head);
  return THEME_LOAD_OK;
#else
  staging_ctx = *ctx;
  staging_ctx.file_color_rules_head = NULL;
  old_file_rules = (FileColorRule *)ctx->file_color_rules_head;
  color_snapshot = UIColorSnapshot_Create();

  ApplyThemeRoles(ctx, roles);
  if (!StageThemePaletteLines(ctx, palette_head, &staging_ctx)) {
    ctx->file_color_rules_head = old_file_rules;
    UIColorSnapshot_Restore(color_snapshot);
    UIColorSnapshot_Free(color_snapshot);
    FreeThemeFileColorRules((FileColorRule *)staging_ctx.file_color_rules_head);
    FreeThemePaletteLines(palette_head);
    return THEME_LOAD_INVALID;
  }

  ctx->file_color_rules_head = staging_ctx.file_color_rules_head;
  FreeThemeFileColorRules(old_file_rules);
  UIColorSnapshot_Free(color_snapshot);
  FreeThemePaletteLines(palette_head);
  return THEME_LOAD_OK;
#endif
}

static ThemeLoadStatus ReadThemeFileInternal(ViewContext *ctx,
                                             const char *filename,
                                             const char *theme_name) {
  FILE *fp;
  ThemeLoadStatus status;

  if (ctx == NULL || filename == NULL || theme_name == NULL ||
      *theme_name == '\0')
    return THEME_LOAD_INVALID;

  fp = fopen(filename, "r");
  if (fp == NULL)
    return (errno == ENOENT) ? THEME_LOAD_NOT_FOUND : THEME_LOAD_INVALID;

  status = ReadThemeStreamInternal(ctx, fp, theme_name);
  fclose(fp);
  return status;
}

int ReadThemeFile(ViewContext *ctx, const char *filename,
                  const char *theme_name) {
  return ReadThemeFileInternal(ctx, filename, theme_name) == THEME_LOAD_OK ? 0
                                                                          : -1;
}

static int ThemeWriteAll(int fd, const char *buf, size_t len) {
  size_t written_total = 0;

  while (written_total < len) {
    ssize_t written_now = write(fd, buf + written_total, len - written_total);

    if (written_now <= 0)
      return -1;
    written_total += (size_t)written_now;
  }

  return 0;
}

static int EnsureThemeConfigHomeDirectory(const char *home) {
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

static int ResolveSeedThemePath(char *path, size_t path_size,
                                const char *home) {
  char legacy_path[PATH_LENGTH + 1];
  int written;

  if (path == NULL || path_size == 0 || home == NULL || *home == '\0')
    return -1;

  written = snprintf(path, path_size, "%s/%s", home, THEME_CONFIG_HOME_PATH);
  if (written < 0 || written >= (int)path_size)
    return -1;

  written =
      snprintf(legacy_path, sizeof(legacy_path), "%s/%s", home, THEME_FILENAME);
  if (written < 0 || written >= (int)sizeof(legacy_path))
    return -1;
  if (access(path, F_OK) != 0 && access(legacy_path, F_OK) == 0) {
    (void)snprintf(path, path_size, "%s", legacy_path);
    return 0;
  }

  if (EnsureThemeConfigHomeDirectory(home) == 0)
    return 0;

  (void)snprintf(path, path_size, "%s", legacy_path);
  return 0;
}

static int SeedConfiguredThemePath(const char *path) {
  size_t default_len;
  int fd;
  int close_result;

  if (path == NULL || *path == '\0')
    return -1;

  fd = open(path, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
  if (fd == -1) {
    if (errno == EEXIST)
      return 0;
    return -1;
  }

  default_len = strlen(default_theme_catalog);
  if (ThemeWriteAll(fd, default_theme_catalog, default_len) != 0) {
    int saved_errno = errno;

    close(fd);
    unlink(path);
    errno = saved_errno;
    return -1;
  }
  close_result = close(fd);
  if (close_result != 0) {
    int saved_errno = errno;

    unlink(path);
    errno = saved_errno;
    return -1;
  }

  return 0;
}

static ThemeLoadStatus ReadCompiledThemeCatalog(ViewContext *ctx,
                                                const char *theme_name) {
  FILE *fp;
  size_t default_len;
  ThemeLoadStatus status;

  if (ctx == NULL || theme_name == NULL || *theme_name == '\0')
    return THEME_LOAD_INVALID;

  fp = tmpfile();
  if (fp == NULL)
    return THEME_LOAD_INVALID;

  default_len = strlen(default_theme_catalog);
  if (fwrite(default_theme_catalog, 1, default_len, fp) != default_len) {
    fclose(fp);
    return THEME_LOAD_INVALID;
  }
  rewind(fp);

  status = ReadThemeStreamInternal(ctx, fp, theme_name);
  fclose(fp);
  return status;
}

static int TryConfiguredThemePath(ViewContext *ctx, char *path,
                                  size_t path_size, const char *home,
                                  const char *suffix,
                                  const char *theme_name) {
  int written;
  ThemeLoadStatus status;

  if (home == NULL || *home == '\0')
    return -1;

  written = snprintf(path, path_size, "%s%c%s", home, FILE_SEPARATOR_CHAR,
                     suffix);
  if (written < 0 || (size_t)written >= path_size)
    return -1;
  if (access(path, F_OK) != 0)
    return 1;

  status = ReadThemeFileInternal(ctx, path, theme_name);
  if (status == THEME_LOAD_OK)
    return 0;
  if (status == THEME_LOAD_NOT_FOUND)
    return -1;
  return -2;
}

int LoadConfiguredTheme(ViewContext *ctx) {
  const char *theme_name;
  const char *home;
  char path[PATH_LENGTH + 1];
  int result;
  int user_catalog_found = 0;

  if (ctx == NULL)
    return -1;

  theme_name = "classic-blue";
  if (ctx->core_init_ops.get_profile_value != NULL) {
    const char *configured = ctx->core_init_ops.get_profile_value(ctx, "THEME");
    if (configured != NULL && *configured != '\0')
      theme_name = configured;
  }

  home = getenv("HOME");
  result = TryConfiguredThemePath(ctx, path, sizeof(path), home,
                                  THEME_CONFIG_HOME_PATH, theme_name);
  if (result == 0)
    return 0;
  if (result == -2)
    return -1;
  if (result == -1)
    user_catalog_found = 1;

  result = TryConfiguredThemePath(ctx, path, sizeof(path), home, THEME_FILENAME,
                                  theme_name);
  if (result == 0)
    return 0;
  if (result == -2)
    return -1;
  if (result == -1)
    user_catalog_found = 1;

  if (!user_catalog_found && ResolveSeedThemePath(path, sizeof(path), home) == 0 &&
      SeedConfiguredThemePath(path) == 0)
    return ReadThemeFile(ctx, path, theme_name);

  return ReadCompiledThemeCatalog(ctx, theme_name) == THEME_LOAD_OK ? 0 : -1;
}

static int CoreInit_LoadTheme(ViewContext *ctx) {
  return LoadConfiguredTheme(ctx);
}

void CoreInitOps_RegisterCmdTheme(CoreInitOps *ops) {
  if (ops == NULL)
    return;

  ops->load_theme = CoreInit_LoadTheme;
}
