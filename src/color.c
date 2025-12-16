/***************************************************************************
 *
 * color.c
 * Dynamic Colors-Support
 *
 ***************************************************************************/


#include "ytree.h"


#ifdef COLOR_SUPPORT

static BOOL color_enabled = FALSE;

UIColor ui_colors[] = {
    {"DIR_COLOR",      CPAIR_DIR,      COLOR_WHITE,   COLOR_BLUE},
    {"HIDIR_COLOR",    CPAIR_HIDIR,    COLOR_BLACK,   COLOR_WHITE},
    {"WINDIR_COLOR",   CPAIR_WINDIR,   COLOR_CYAN,    COLOR_BLUE},
    {"FILE_COLOR",     CPAIR_FILE,     COLOR_WHITE,   COLOR_BLUE},
    {"HIFILE_COLOR",   CPAIR_HIFILE,   COLOR_BLACK,   COLOR_WHITE},
    {"WINFILE_COLOR",  CPAIR_WINFILE,  COLOR_CYAN,    COLOR_BLUE},
    {"STATS_COLOR",    CPAIR_STATS,    COLOR_BLUE,    COLOR_CYAN},
    {"WINSTATS_COLOR", CPAIR_WINSTATS, COLOR_BLUE,    COLOR_CYAN},
    {"BORDERS_COLOR",  CPAIR_BORDERS,  COLOR_BLUE,    COLOR_CYAN},
    {"HIMENUS_COLOR",  CPAIR_HIMENUS,  COLOR_WHITE,   COLOR_BLUE},
    {"MENU_COLOR",     CPAIR_MENU,     COLOR_CYAN,    COLOR_BLUE},
    {"WINERR_COLOR",   CPAIR_WINERR,   COLOR_BLUE,    COLOR_WHITE},
    {"HST_COLOR",      CPAIR_HST,      COLOR_YELLOW,  COLOR_CYAN},
    {"HIHST_COLOR",    CPAIR_HIHST,    COLOR_WHITE,   COLOR_WHITE},
    {"WINHST_COLOR",   CPAIR_WINHST,   COLOR_YELLOW,  COLOR_CYAN},
    {"HIGLOBAL_COLOR", CPAIR_HIGLOBAL, COLOR_BLUE,    COLOR_WHITE},
    {"GLOBAL_COLOR",   CPAIR_GLOBAL,   COLOR_YELLOW,  COLOR_CYAN}
};

int NUM_UI_COLORS = sizeof(ui_colors) / sizeof(ui_colors[0]);

typedef struct {
    const char *name;
    int value;
} ColorMapEntry;

static const ColorMapEntry color_map[] = {
    {"black",   COLOR_BLACK},
    {"red",     COLOR_RED},
    {"green",   COLOR_GREEN},
    {"yellow",  COLOR_YELLOW},
    {"blue",    COLOR_BLUE},
    {"magenta", COLOR_MAGENTA},
    {"cyan",    COLOR_CYAN},
    {"white",   COLOR_WHITE},
    {NULL, -1}
};

void ParseColorString(const char *color_str, int *fg, int *bg)
{
    char *dup, *token, *saveptr;
    int i;
    int *target;

    if (!color_str || !fg || !bg) return;

    dup = strdup(color_str);
    if (!dup) return;

    target = fg;
    token = strtok_r(dup, " ,", &saveptr);
    while (token) {
        for (i = 0; color_map[i].name; i++) {
            if (strcasecmp(token, color_map[i].name) == 0) {
                *target = color_map[i].value;
                break;
            }
        }
        target = bg; /* Second token is the background color */
        token = strtok_r(NULL, " ,", &saveptr);
    }
    free(dup);
}

void UpdateUIColor(const char *name, int fg, int bg)
{
    int i;
    for (i = 0; i < NUM_UI_COLORS; i++) {
        if (strcasecmp(name, ui_colors[i].name) == 0) {
            ui_colors[i].fg = fg;
            ui_colors[i].bg = bg;
            return;
        }
    }
}

void AddFileColorRule(const char *pattern, int fg, int bg)
{
    FileColorRule *new_rule = malloc(sizeof(FileColorRule));
    if (!new_rule) {
        ERROR_MSG("malloc failed in AddFileColorRule");
        return;
    }
    new_rule->pattern = strdup(pattern);
    if (!new_rule->pattern) {
        ERROR_MSG("strdup failed in AddFileColorRule");
        free(new_rule);
        return;
    }
    new_rule->fg = fg;
    new_rule->bg = bg;
    new_rule->pair_id = 0;
    new_rule->next = file_color_rules_head;
    file_color_rules_head = new_rule;
}

void StartColors()
{
    int i;
    FileColorRule *rule;
    int next_pair_id = F_COLOR_PAIR_BASE;

    start_color();
    if (COLORS < 8 || COLOR_PAIRS < 64) { /* Check for a reasonable number of pairs */
        return; /* No color support */
    }

    /* Initialize UI colors */
    for (i = 0; i < NUM_UI_COLORS; i++) {
        init_pair(ui_colors[i].id, ui_colors[i].fg, ui_colors[i].bg);
    }

    /* Initialize file type colors */
    for (rule = file_color_rules_head; rule != NULL; rule = rule->next) {
        if (next_pair_id < COLOR_PAIRS) {
            rule->pair_id = next_pair_id++;
            init_pair(rule->pair_id, rule->fg, rule->bg);
        } else {
            rule->pair_id = CPAIR_FILE; /* Fallback if we run out of pairs */
        }
    }

    color_enabled = TRUE;
}


int GetFileTypeColor(FileEntry *fe_ptr)
{
    FileColorRule *rule;

    if (!fe_ptr) return CPAIR_FILE;

    for (rule = file_color_rules_head; rule != NULL; rule = rule->next) {
        /* Check special keywords first */
        if (S_ISDIR(fe_ptr->stat_struct.st_mode) && strcmp(rule->pattern, "DIR") == 0) {
            return rule->pair_id;
        }
        if (S_ISLNK(fe_ptr->stat_struct.st_mode) && strcmp(rule->pattern, "LINK") == 0) {
            return rule->pair_id;
        }
        if ((fe_ptr->stat_struct.st_mode & (S_IXUSR | S_IXGRP | S_IXOTH)) && strcmp(rule->pattern, "EXEC") == 0) {
            return rule->pair_id;
        }

        /* Check for wildcard extension match */
        if (rule->pattern[0] == '*' && rule->pattern[1] == '.') {
            const char *ext = rule->pattern + 1;
            int name_len = strlen(fe_ptr->name);
            int ext_len = strlen(ext);
            if (name_len > ext_len && strcasecmp(fe_ptr->name + name_len - ext_len, ext) == 0) {
                return rule->pair_id;
            }
        }
    }

    return CPAIR_FILE; /* Default */
}


void WbkgdSet(WINDOW *w, chtype c)
{
  if(color_enabled) {
    wbkgdset(w, c);
  } else {
    c &= ~A_BOLD;
    if(c == COLOR_PAIR(CPAIR_HIDIR)   ||
       c == COLOR_PAIR(CPAIR_HIFILE)  ||
       c == COLOR_PAIR(CPAIR_HIMENUS) ||
       c == COLOR_PAIR(CPAIR_HIHST)) {

      wattrset(w, A_REVERSE);
    } else {
      wattrset(w, 0);
    }
  }
}


#endif /* COLOR_SUPPORT */