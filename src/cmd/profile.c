/***************************************************************************
 *
 * src/cmd/profile.c
 * Profile support
 *
 ***************************************************************************/

#include "config.h"
#include "ytnova_cmd.h"

#define NO_SECTION 0
#define GLOBAL_SECTION 1
#define VIEWER_SECTION 2
#define MENU_SECTION 3
#define FILEMAP_SECTION 4
#define FILECMD_SECTION 5
#define DIRMAP_SECTION 6
#define DIRCMD_SECTION 7
#define COLORS_SECTION 8
#define FILE_COLORS_SECTION 9

typedef struct {
  char *name;
  char *def;
  char *envvar;
  char *value;
} Profile;

typedef struct _viewer {
  char *ext;
  char *cmd;
  struct _viewer *next;
} Viewer;

typedef struct _dirmenu {
  int chkey;
  int chremap;
  char *cmd;
  struct _dirmenu *next;
} Dirmenu;

typedef struct _filemenu {
  int chkey;
  int chremap;
  char *cmd;
  struct _filemenu *next;
} Filemenu;

static Viewer viewer;
static Dirmenu dirmenu;
static Filemenu filemenu;

/* must be sorted! */
static Profile profile[] = {
    {"ANIMATION", DEFAULT_ANIMATION, NULL, NULL},
    {"AUDIBLEERROR", DEFAULT_AUDIBLEERROR, NULL, NULL},
    {"AUTO_REFRESH", DEFAULT_AUTO_REFRESH, NULL, NULL},
    {"BUNZIP", DEFAULT_BUNZIP, NULL, NULL},
    {"CAT", DEFAULT_CAT, NULL, NULL},
    {"CONFIRMQUIT", DEFAULT_CONFIRMQUIT, NULL, NULL},
    {"DIR1", DEFAULT_DIR1, NULL, NULL},
    {"DIR2", DEFAULT_DIR2, NULL, NULL},
    {"DIRDIFF", DEFAULT_DIRDIFF, NULL, NULL},
    {"EDITOR", DEFAULT_EDITOR, "EDITOR", NULL},
    {"FILE1", DEFAULT_FILE1, NULL, NULL},
    {"FILE2", DEFAULT_FILE2, NULL, NULL},
    {"FILEDIFF", DEFAULT_FILEDIFF, NULL, NULL},
    {"FILEMODE", DEFAULT_FILEMODE, NULL, NULL},
    {"GNUUNZIP", DEFAULT_GNUUNZIP, NULL, NULL},
    {"HEXDUMP", DEFAULT_HEXDUMP, NULL, NULL},
    {"HEXEDITOFFSET", DEFAULT_HEXEDITOFFSET, NULL, NULL},
    {"HIDEDOTFILES", DEFAULT_HIDEDOTFILES, NULL, NULL},
    {"HIGHLIGHT_FULL_LINE", DEFAULT_HIGHLIGHT_FULL_LINE, NULL, NULL},
    {"INITIALDIR", DEFAULT_INITIALDIR, NULL, NULL},
    {"LISTJUMPSEARCH", DEFAULT_LISTJUMPSEARCH, NULL, NULL},
    {"LUNZIP", DEFAULT_LUNZIP, NULL, NULL},
    {"MANROFF", DEFAULT_MANROFF, NULL, NULL},
    {"MELD", DEFAULT_MELD, NULL, NULL},
    {"NUMBERSEP", DEFAULT_NUMBERSEP, NULL, NULL},
    {"PAGER", DEFAULT_PAGER, "PAGER", NULL},
    {"SEARCHCOMMAND", DEFAULT_SEARCHCOMMAND, NULL, NULL},
    {"SMALLWINDOWSKIP", DEFAULT_SMALLWINDOWSKIP, NULL, NULL},
    {"TAGGEDVIEWER", DEFAULT_TAGGEDVIEWER, NULL, NULL},
    {"TREEDEPTH", DEFAULT_TREEDEPTH, NULL, NULL},
    {"TREEDIFF", DEFAULT_TREEDIFF, NULL, NULL},
    {"UNCOMPRESS", DEFAULT_UNCOMPRESS, NULL, NULL},
    {"USERVIEW", "", NULL, NULL},
    {"VI_KEYS", DEFAULT_VI_KEYS, NULL, NULL},
    {"ZSTDCAT", DEFAULT_ZSTDCAT, NULL, NULL}};

#define PROFILE_ENTRIES (sizeof(profile) / sizeof(profile[0]))

static int Compare(const void *s1, const void *s2);
static int ChCode(const char *s);
static char *TrimInPlace(char *text);
static void AddProfileFileColorRule(ViewContext *ctx, const char *pattern,
                                    int fg, int bg);
static void AddCompactFileColorRules(ViewContext *ctx, char *value);
static BOOL BuildFileColorPattern(const char *selector, char *pattern,
                                  size_t pattern_size);

void FreeProfileRuntimeData(ViewContext *ctx) {
  size_t i;
  Viewer *v, *next_v;
  Filemenu *m, *next_m;
  Dirmenu *d, *next_d;

  (void)ctx;

  for (i = 0; i < PROFILE_ENTRIES; ++i) {
    if (profile[i].value != NULL) {
      free(profile[i].value);
      profile[i].value = NULL;
    }
  }

  for (v = viewer.next; v != NULL; v = next_v) {
    next_v = v->next;
    if (v->ext != NULL)
      free(v->ext);
    if (v->cmd != NULL)
      free(v->cmd);
    free(v);
  }
  viewer.next = NULL;

  for (m = filemenu.next; m != NULL; m = next_m) {
    next_m = m->next;
    if (m->cmd != NULL)
      free(m->cmd);
    free(m);
  }
  filemenu.next = NULL;

  for (d = dirmenu.next; d != NULL; d = next_d) {
    next_d = d->next;
    if (d->cmd != NULL)
      free(d->cmd);
    free(d);
  }
  dirmenu.next = NULL;
}

int ReadProfile(ViewContext *ctx, const char *filename) {
  int result = -1;
  char buffer[1024], *old;
  const char *n;
  char *name, *value;
  int section;
  Profile *p, key;
  Viewer *v, *new_v;
  Filemenu *m, *new_m;
  Dirmenu *d, *new_d;
  FILE *f;

  ctx->profile_data = profile;
  ctx->viewer_list = &viewer;
  ctx->dirmenu_list = &dirmenu;
  ctx->filemenu_list = &filemenu;

  FreeProfileRuntimeData(ctx);

  section = NO_SECTION;
  v = (Viewer *)ctx->viewer_list;
  m = (Filemenu *)ctx->filemenu_list;
  d = (Dirmenu *)ctx->dirmenu_list;

  if ((f = fopen(filename, "r")) == NULL) {
    return -1;
  }

  while (fgets(buffer, sizeof(buffer), f)) {
    char *cptr;
    int l;

    if ((cptr = strchr(buffer, '#'))) {
      *cptr = '\0';
    }

    l = strlen(buffer);
    while (l > 0 && isspace((unsigned char)buffer[l - 1])) {
      buffer[--l] = '\0';
    }

    if (l == 0)
      continue;

    for (name = buffer; isspace(*name); name++)
      ;

    if (*name == '\0')
      continue;

    for (cptr = name; *cptr && !isspace((unsigned char)*cptr) && *cptr != '=';
         cptr++)
      ;

    if (*name == '[') {
      if (!strcmp(name, "[GLOBAL]"))
        section = GLOBAL_SECTION;
      else if (!strcmp(name, "[VIEWER]"))
        section = VIEWER_SECTION;
      else if (!strcmp(name, "[MENU]"))
        section = MENU_SECTION;
      else if (!strcmp(name, "[FILEMAP]"))
        section = FILEMAP_SECTION;
      else if (!strcmp(name, "[FILECMD]"))
        section = FILECMD_SECTION;
      else if (!strcmp(name, "[DIRMAP]"))
        section = DIRMAP_SECTION;
      else if (!strcmp(name, "[DIRCMD]"))
        section = DIRCMD_SECTION;
      else if (!strcmp(name, "[COLORS]"))
        section = COLORS_SECTION;
      else if (!strcmp(name, "[FILE_COLORS]"))
        section = FILE_COLORS_SECTION;
      else
        section = NO_SECTION;

      continue;
    }

    value = cptr;
    if (*value == '=') {
      *value++ = '\0';
    } else if (*value != '\0') {
      *value++ = '\0';
      while (*value && isspace((unsigned char)*value))
        ++value;
      if (*value == '=')
        *value++ = '\0';
      else
        value = NULL;
    } else {
      value = NULL;
    }
    if (value != NULL) {
      while (*value && isspace((unsigned char)*value))
        ++value;
    }

    if (section == GLOBAL_SECTION) {
      if (value) {
        key.name = name;
        if ((p = bsearch(&key, (Profile *)ctx->profile_data, PROFILE_ENTRIES,
                         sizeof(*p), Compare))) {
          if (p->value)
            free(p->value);
          p->value = xstrdup(value);
        }
      }
    } else if (section == COLORS_SECTION) {
      if (value) {
        int fg = -1, bg = -1;
        if (ctx->hook_parse_color) {
          ctx->hook_parse_color(value, &fg, &bg);
        }
        if (fg != -1 && bg != -1 && ctx->hook_update_ui_color) {
          ctx->hook_update_ui_color(name, fg, bg);
        }
      }
    } else if (section == FILE_COLORS_SECTION) {
      if (value) {
        int fg = -1, bg = -1;
        if (strchr(value, ':') != NULL) {
          AddCompactFileColorRules(ctx, value);
        } else {
          if (ctx->hook_parse_color) {
            ctx->hook_parse_color(value, &fg, &bg);
          }
          AddProfileFileColorRule(ctx, name, fg, bg);
        }
      }
    } else if (section == MENU_SECTION) {
      if (value) {
        if (!strcmp(name, "DIR1") || !strcmp(name, "DIR2") ||
            !strcmp(name, "FILE1") || !strcmp(name, "FILE2")) {
          key.name = name;
          if ((p = bsearch(&key, (Profile *)ctx->profile_data, PROFILE_ENTRIES,
                           sizeof(*p), Compare))) {
            int menu_len = 0;
            char *menu_dst = value;
            for (; *menu_dst; ++menu_dst) {
              if (*menu_dst != '(' && *menu_dst != ')') {
                ++menu_len;
              }
            }
            while (menu_len++ < COLS - 1)
              *menu_dst++ = ' ';
            *menu_dst = '\0';
            p->value = xstrdup(value);
          }
        }
      }
    } else if (section == FILEMAP_SECTION) {
      if (value) {
        while (*value && isspace(*value))
          value++;
        n = strtok_r(name, ",", &old);
        while (n) {
          for (new_m = ((Filemenu *)ctx->filemenu_list)->next; new_m != NULL;
               new_m = new_m->next) {
            if (new_m->chkey == ChCode(n)) {
              new_m->chremap = ChCode(value);
              if (new_m->chremap == 0)
                new_m->chremap = -1;
              break;
            }
          }
          if (new_m == NULL) {
            new_m = xmalloc(sizeof(*new_m));
            new_m->chkey = ChCode(n);
            new_m->chremap = ChCode(value);
            new_m->cmd = NULL;
            new_m->next = NULL;
            m->next = new_m;
            m = new_m;
          }
          n = strtok_r(NULL, ",", &old);
        }
      }
    } else if (section == FILECMD_SECTION) {
      if (value) {
        while (*value && isspace(*value))
          value++;
        for (new_m = ((Filemenu *)ctx->filemenu_list)->next; new_m != NULL;
             new_m = new_m->next) {
          if (new_m->chkey == ChCode(name)) {
            new_m->cmd = xstrdup(value);
            if (new_m->chremap == 0)
              new_m->chremap = -1;
            break;
          }
        }
        if (new_m == NULL) {
          new_m = xmalloc(sizeof(*new_m));
          new_m->chkey = ChCode(name);
          new_m->chremap = new_m->chkey;
          new_m->cmd = xstrdup(value);
          new_m->next = NULL;
          m->next = new_m;
          m = new_m;
        }
      }
    } else if (section == DIRMAP_SECTION) {
      if (value) {
        while (*value && isspace(*value))
          value++;
        n = strtok_r(name, ",", &old);
        while (n) {
          for (new_d = ((Dirmenu *)ctx->dirmenu_list)->next; new_d != NULL;
               new_d = new_d->next) {
            if (new_d->chkey == ChCode(n)) {
              new_d->chremap = ChCode(value);
              if (new_d->chremap == 0)
                new_d->chremap = -1;
              break;
            }
          }
          if (new_d == NULL) {
            new_d = xmalloc(sizeof(*new_d));
            new_d->chkey = ChCode(n);
            new_d->chremap = ChCode(value);
            new_d->cmd = NULL;
            new_d->next = NULL;
            d->next = new_d;
            d = new_d;
          }
          n = strtok_r(NULL, ",", &old);
        }
      }
    } else if (section == DIRCMD_SECTION) {
      if (value) {
        while (*value && isspace(*value))
          value++;
        for (new_d = ((Dirmenu *)ctx->dirmenu_list)->next; new_d != NULL;
             new_d = new_d->next) {
          if (new_d->chkey == ChCode(name)) {
            new_d->cmd = xstrdup(value);
            if (new_d->chremap == 0)
              new_d->chremap = -1;
            break;
          }
        }
        if (new_d == NULL) {
          new_d = xmalloc(sizeof(*new_d));
          new_d->chkey = ChCode(name);
          new_d->chremap = new_d->chkey;
          new_d->cmd = xstrdup(value);
          new_d->next = NULL;
          d->next = new_d;
          d = new_d;
        }
      }
    } else if (section == VIEWER_SECTION) {
      if (value) {
        n = strtok_r(name, ",", &old);
        while (n) {
          new_v = xmalloc(sizeof(*new_v));
          new_v->ext = xstrdup(n);
          new_v->cmd = xstrdup(value);
          new_v->next = NULL;
          if (new_v->ext == NULL || new_v->cmd == NULL) {
            if (new_v->ext)
              free(new_v->ext);
            if (new_v->cmd)
              free(new_v->cmd);
            free(new_v);
          } else {
            v->next = new_v;
            v = new_v;
          }
          n = strtok_r(NULL, ",", &old);
        }
      }
    }
  }
  result = 0;
  if (f)
    fclose(f);
  return (result);
}

void SetProfileValue(const ViewContext *ctx, char *name, const char *value) {
  Profile *p, key;
  memset(&key, 0, sizeof(key));
  key.name = name;
  p = bsearch(&key, (Profile *)ctx->profile_data, PROFILE_ENTRIES, sizeof(*p),
              Compare);
  if (p) {
    if (p->value)
      free(p->value);
    p->value = xstrdup(value);
  }
}

char *GetProfileValue(const ViewContext *ctx, const char *name) {
  Profile *p, key;
  char *cptr;
  memset(&key, 0, sizeof(key));
  key.name = (char *)name;
  p = bsearch(&key, (Profile *)ctx->profile_data, PROFILE_ENTRIES, sizeof(*p),
              Compare);
  if (!p)
    return ("");
  if (p->value)
    return (p->value);
  if (p->envvar && (cptr = getenv(p->envvar)))
    return (cptr);
  return (p->def);
}

static int ChCode(const char *s) {
  if (*s == '^' && *(s + 1) != '^')
    return ((int)((*(s + 1)) & 0x1F));
  else
    return ((int)(*s));
}

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

static void AddProfileFileColorRule(ViewContext *ctx, const char *pattern,
                                    int fg, int bg) {
  if (fg == -1 || ctx == NULL || ctx->hook_add_file_color_rule == NULL)
    return;

  ctx->hook_add_file_color_rule(ctx, pattern, fg, bg);
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

  if (ctx == NULL || value == NULL || ctx->hook_parse_color == NULL)
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
      AddProfileFileColorRule(ctx, pattern, fg, bg);

    selector = strtok_r(NULL, ",", &saveptr);
  }
}

static int Compare(const void *s1, const void *s2) {
  return (strcmp(((Profile *)s1)->name, ((Profile *)s2)->name));
}

char *GetUserFileAction(const ViewContext *ctx, int chkey, int *pchremap) {
  Filemenu *m;
  for (m = ((Filemenu *)ctx->filemenu_list)->next; m; m = m->next) {
    if (chkey == m->chkey) {
      if (pchremap)
        *pchremap = m->chremap;
      return (m->cmd);
    }
  }
  if (pchremap)
    *pchremap = chkey;
  return (NULL);
}

char *GetUserDirAction(const ViewContext *ctx, int chkey, int *pchremap) {
  Dirmenu *d;
  for (d = ((Dirmenu *)ctx->dirmenu_list)->next; d; d = d->next) {
    if (chkey == d->chkey) {
      if (pchremap)
        *pchremap = d->chremap;
      return (d->cmd);
    }
  }
  if (pchremap)
    *pchremap = chkey;
  return (NULL);
}

BOOL IsUserActionDefined(const ViewContext *ctx) {
  return ((BOOL)(((Dirmenu *)ctx->dirmenu_list)->next != NULL ||
                 ((Filemenu *)ctx->filemenu_list)->next != NULL));
}

static int CoreInit_ReadProfile(ViewContext *ctx, const char *filename) {
  return ReadProfile(ctx, filename);
}

static char *CoreInit_GetProfileValue(const ViewContext *ctx, const char *name) {
  return GetProfileValue(ctx, name);
}

static BOOL CoreInit_HasUserAction(const ViewContext *ctx) {
  return IsUserActionDefined(ctx);
}

void CoreInitOps_RegisterCmdProfile(CoreInitOps *ops) {
  if (ops == NULL)
    return;

  ops->read_profile = CoreInit_ReadProfile;
  ops->get_profile_value = CoreInit_GetProfileValue;
  ops->has_user_action = CoreInit_HasUserAction;
}
