/***************************************************************************
 *
 * src/ui/color.c
 * Dynamic Colors-Support
 *
 ***************************************************************************/

#include "ytree.h"

#ifdef COLOR_SUPPORT

UIColor ui_colors[] = {
    {"DIR_COLOR", CPAIR_DIR, 7, 0},
    {"HIDIR_COLOR", CPAIR_HIDIR, 0, 3},
    {"WINDIR_COLOR", CPAIR_WINDIR, 7, 0},
    {"FILE_COLOR", CPAIR_FILE, 7, 0},
    {"HIFILE_COLOR", CPAIR_HIFILE, 0, 3},
    {"WINFILE_COLOR", CPAIR_WINFILE, 7, 0},
    {"STATS_COLOR", CPAIR_STATS, 7, 0},
    {"WINSTATS_COLOR", CPAIR_WINSTATS, 7, 0},
    {"BORDERS_COLOR", CPAIR_BORDERS, 8, 0},
    {"HIMENUS_COLOR", CPAIR_HIMENUS, 15, 0},
    {"MENU_COLOR", CPAIR_MENU, 7, 0},
    {"WINERR_COLOR", CPAIR_WINERR, 15, 1},
    {"HST_COLOR", CPAIR_HST, 7, 0},
    {"HIHST_COLOR", CPAIR_HIHST, 0, 3},
    {"WINHST_COLOR", CPAIR_WINHST, 7, 0},
    {"HIGLOBAL_COLOR", CPAIR_HIGLOBAL, 15, 3},
    {"GLOBAL_COLOR", CPAIR_GLOBAL, 3, 0},
    {"INFO_COLOR", CPAIR_INFO, 15, 4},
    {"WARN_COLOR", CPAIR_WARN, 11, 0},
    {"ERR_COLOR", CPAIR_ERR, 15, 1}};

int NUM_UI_COLORS = sizeof(ui_colors) / sizeof(ui_colors[0]);

typedef struct {
  const char *name;
  int value;
} ColorMapEntry;

static const ColorMapEntry color_map[] = {{"black", COLOR_BLACK},
                                          {"red", COLOR_RED},
                                          {"green", COLOR_GREEN},
                                          {"yellow", COLOR_YELLOW},
                                          {"blue", COLOR_BLUE},
                                          {"magenta", COLOR_MAGENTA},
                                          {"cyan", COLOR_CYAN},
                                          {"white", COLOR_WHITE},
                                          {NULL, -1}};

static int NormalizeColorIndex(int color, int color_limit) {
  if (color == -1)
    return -1;
  if (color < -1)
    return COLOR_WHITE;
  if (color_limit <= 0)
    return color;
  if (color < color_limit)
    return color;

  /* Bright ANSI colors fall back to base colors on 8-color terminals. */
  if (color_limit <= 8 && color <= 15)
    return color - 8;

  return color % color_limit;
}

void ParseColorString(const char *color_str, int *fg, int *bg) {
  char *dup, *token, *saveptr;
  int i;
  int *target;
  char *endptr;
  long val;
  int color_limit;

  if (!color_str || !fg || !bg)
    return;

  dup = xstrdup(color_str);

  target = fg;
  token = strtok_r(dup, " ,", &saveptr);

  /* Determine maximum valid color index.
     Use global COLORS if initialized, otherwise assume 256 for config parsing
     safety. */
  color_limit = (COLORS > 0) ? COLORS : 256;

  while (token) {
    BOOL found = FALSE;

    /* 1. Try named colors */
    for (i = 0; color_map[i].name; i++) {
      if (strcasecmp(token, color_map[i].name) == 0) {
        *target = color_map[i].value;
        found = TRUE;
        break;
      }
    }

    /* 2. Try numeric colors (0-255) */
    if (!found) {
      errno = 0;
      val = strtol(token, &endptr, 10);

      /* Check if valid number: no error, parsed something, and full string
       * consumed */
      if (errno == 0 && endptr != token && *endptr == '\0') {
        if (val >= -1 && val <= 255) {
          *target = NormalizeColorIndex((int)val, color_limit);
        }
      }
    }

    target = bg; /* Second token is the background color */
    token = strtok_r(NULL, " ,", &saveptr);
  }
  free(dup);
}

void UpdateUIColor(const char *name, int fg, int bg) {
  int i;
  for (i = 0; i < NUM_UI_COLORS; i++) {
    if (strcasecmp(name, ui_colors[i].name) == 0) {
      ui_colors[i].fg = fg;
      ui_colors[i].bg = bg;
      return;
    }
  }
}

void AddFileColorRule(ViewContext *ctx, const char *pattern, int fg, int bg) {
  FileColorRule *new_rule = xmalloc(sizeof(FileColorRule));
  new_rule->pattern = xstrdup(pattern);
  new_rule->fg = fg;
  new_rule->bg = bg;
  new_rule->pair_id = 0;
  new_rule->next = ctx->file_color_rules_head;
  ctx->file_color_rules_head = new_rule;
}

void ReinitColorPairs(ViewContext *ctx) {
  int i;
  FileColorRule *rule;
  int next_pair_id = F_COLOR_PAIR_BASE;

  if (!ctx->color_enabled)
    return;

  /* Initialize UI colors */
  for (i = 0; i < NUM_UI_COLORS; i++) {
    init_pair(ui_colors[i].id, NormalizeColorIndex(ui_colors[i].fg, COLORS),
              NormalizeColorIndex(ui_colors[i].bg, COLORS));
  }

  /* Initialize file type colors */
  for (rule = ctx->file_color_rules_head; rule != NULL; rule = rule->next) {
    if (rule->pair_id == 0) {
      if (next_pair_id < COLOR_PAIRS) {
        rule->pair_id = next_pair_id++;
      } else {
        rule->pair_id = CPAIR_FILE; /* Fallback */
      }
    }
    init_pair(rule->pair_id, NormalizeColorIndex(rule->fg, COLORS),
              NormalizeColorIndex(rule->bg, COLORS));
  }
}

void StartColors(ViewContext *ctx) {
  start_color();
  if (COLORS < 8 ||
      COLOR_PAIRS < 64) { /* Check for a reasonable number of pairs */
    return;               /* No color support */
  }

  ctx->color_enabled = TRUE;
  ReinitColorPairs(ctx);
}

int GetFileTypeColor(const ViewContext *ctx, const FileEntry *fe_ptr) {
  FileColorRule *rule;

  if (!fe_ptr)
    return CPAIR_FILE;

  for (rule = ctx->file_color_rules_head; rule != NULL; rule = rule->next) {
    /* Check special keywords first */
    if (S_ISDIR(fe_ptr->stat_struct.st_mode) &&
        strcmp(rule->pattern, "DIR") == 0) {
      return rule->pair_id;
    }
    if (S_ISLNK(fe_ptr->stat_struct.st_mode) &&
        strcmp(rule->pattern, "LINK") == 0) {
      return rule->pair_id;
    }
    if ((fe_ptr->stat_struct.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) &&
        strcmp(rule->pattern, "EXEC") == 0) {
      return rule->pair_id;
    }

    /* Check for wildcard extension match */
    if (rule->pattern[0] == '*' && rule->pattern[1] == '.') {
      const char *ext = rule->pattern + 1;
      int name_len = strlen(fe_ptr->name);
      int ext_len = strlen(ext);
      if (name_len > ext_len &&
          strcasecmp(fe_ptr->name + name_len - ext_len, ext) == 0) {
        return rule->pair_id;
      }
    }
  }

  return CPAIR_FILE; /* Default */
}

void WbkgdSet(const ViewContext *ctx, WINDOW *w, chtype c) {
  if (ctx->color_enabled) {
    wbkgdset(w, c);
  } else {
    c &= ~A_BOLD;
    if (c == COLOR_PAIR(CPAIR_HIDIR) || c == COLOR_PAIR(CPAIR_HIFILE) ||
        c == COLOR_PAIR(CPAIR_HIMENUS) || c == COLOR_PAIR(CPAIR_HIHST)) {

      wattrset(w, A_REVERSE);
    } else {
      wattrset(w, 0);
    }
  }
}

#endif /* COLOR_SUPPORT */
