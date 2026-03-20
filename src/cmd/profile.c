/***************************************************************************
 *
 * src/cmd/profile.c
 * Profile support
 *
 ***************************************************************************/

#include "config.h"
#include "ytree.h"

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
    {"NOSMALLWINDOW", DEFAULT_NOSMALLWINDOW, NULL, NULL},
    {"NUMBERSEP", DEFAULT_NUMBERSEP, NULL, NULL},
    {"PAGER", DEFAULT_PAGER, "PAGER", NULL},
    {"SEARCHCOMMAND", DEFAULT_SEARCHCOMMAND, NULL, NULL},
    {"TAGGEDVIEWER", DEFAULT_TAGGEDVIEWER, NULL, NULL},
    {"TREEDIFF", DEFAULT_TREEDIFF, NULL, NULL},
    {"TREEDEPTH", DEFAULT_TREEDEPTH, NULL, NULL},
    {"UNCOMPRESS", DEFAULT_UNCOMPRESS, NULL, NULL},
    {"USERVIEW", "", NULL, NULL},
    {"ZSTDCAT", DEFAULT_ZSTDCAT, NULL, NULL}};

#define PROFILE_ENTRIES (sizeof(profile) / sizeof(profile[0]))

static int Compare(const void *s1, const void *s2);
static int ChCode(const char *s);

int ReadProfile(ViewContext *ctx, char *filename) {
  int l, result = -1;
  char buffer[1024], *n, *old;
  char *name, *value, *cptr;
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

  section = NO_SECTION;
  v = (Viewer *)ctx->viewer_list;
  m = (Filemenu *)ctx->filemenu_list;
  d = (Dirmenu *)ctx->dirmenu_list;

  v->next = NULL;
  m->next = NULL;
  d->next = NULL;

  if ((f = fopen(filename, "r")) == NULL) {
    return -1;
  }

  while (fgets(buffer, sizeof(buffer), f)) {
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
    if (*cptr != '=')
      *cptr = '\0';

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

    if (section == GLOBAL_SECTION) {
      value = strchr(buffer, '=');
      if (*name && value) {
        *value++ = '\0';
        key.name = name;
        if ((p = bsearch(&key, (Profile *)ctx->profile_data, PROFILE_ENTRIES,
                         sizeof(*p), Compare))) {
          p->value = xstrdup(value);
        }
      }
    } else if (section == COLORS_SECTION) {
      value = strchr(buffer, '=');
      if (*name && value) {
        int fg = -1, bg = -1;
        *value++ = '\0';
        ParseColorString(value, &fg, &bg);
        if (fg != -1 && bg != -1) {
          UpdateUIColor(name, fg, bg);
        }
      }
    } else if (section == FILE_COLORS_SECTION) {
      value = strchr(buffer, '=');
      if (*name && value) {
        int fg = -1, bg = -1;
        *value++ = '\0';
        ParseColorString(value, &fg, &bg);
        if (fg != -1 && bg != -1) {
          AddFileColorRule(ctx, name, fg, bg);
        }
      }
    } else if (section == MENU_SECTION) {
      value = strchr(buffer, '=');
      if (*name && value) {
        *value++ = '\0';
        if (!strcmp(name, "DIR1") || !strcmp(name, "DIR2") ||
            !strcmp(name, "FILE1") || !strcmp(name, "FILE2")) {
          key.name = name;
          if ((p = bsearch(&key, (Profile *)ctx->profile_data, PROFILE_ENTRIES,
                           sizeof(*p), Compare))) {
            l = 0;
            for (cptr = value; *cptr; ++cptr) {
              if (*cptr != '(' && *cptr != ')') {
                ++l;
              }
            }
            while (l++ < COLS - 1)
              *cptr++ = ' ';
            *cptr = '\0';
            p->value = xstrdup(value);
          }
        }
      }
    } else if (section == FILEMAP_SECTION) {
      value = strchr(buffer, '=');
      if (*name && value) {
        *value++ = '\0';
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
      value = strchr(buffer, '=');
      if (*name && value) {
        *value++ = '\0';
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
      value = strchr(buffer, '=');
      if (*name && value) {
        *value++ = '\0';
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
      value = strchr(buffer, '=');
      if (*name && value) {
        *value++ = '\0';
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
      value = strchr(buffer, '=');
      if (*name && value) {
        *value++ = '\0';
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

void SetProfileValue(ViewContext *ctx, char *name, char *value) {
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

char *GetProfileValue(ViewContext *ctx, const char *name) {
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

static int Compare(const void *s1, const void *s2) {
  return (strcmp(((Profile *)s1)->name, ((Profile *)s2)->name));
}

char *GetUserFileAction(ViewContext *ctx, int chkey, int *pchremap) {
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

char *GetUserDirAction(ViewContext *ctx, int chkey, int *pchremap) {
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

BOOL IsUserActionDefined(ViewContext *ctx) {
  return ((BOOL)(((Dirmenu *)ctx->dirmenu_list)->next != NULL ||
                 ((Filemenu *)ctx->filemenu_list)->next != NULL));
}

char *GetExtViewer(ViewContext *ctx, char *filename) {
  Viewer *v;
  int l, x;
  l = strlen(filename);
  for (v = ((Viewer *)ctx->viewer_list)->next; v; v = v->next) {
    x = strlen(v->ext);
    if (l > x) {
      if (!strcmp(&filename[l - x], v->ext)) {
        return (v->cmd);
      }
    }
  }
  return (NULL);
}
