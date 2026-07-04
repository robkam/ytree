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
    {"THEME", "classic-blue", NULL, NULL},
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
static int ProfileSectionFromHeader(const char *name);
static BOOL ParseProfileAssignment(char *line, char **name, char **value);
static Viewer *CloneViewerList(const Viewer *head);
static Dirmenu *CloneDirmenuList(const Dirmenu *head);
static Filemenu *CloneFilemenuList(const Filemenu *head);
static FileColorRule *CloneFileColorRules(const FileColorRule *head);
static void FreeViewerList(Viewer *head);
static void FreeDirmenuList(Dirmenu *head);
static void FreeFilemenuList(Filemenu *head);
static void FreeProfileFileColorRules(FileColorRule *head);
static void BindProfileRuntimeData(ViewContext *ctx);

struct _profile_runtime_snapshot {
  char *values[PROFILE_ENTRIES];
  Viewer *viewer_next;
  Dirmenu *dirmenu_next;
  Filemenu *filemenu_next;
  FileColorRule *file_color_rules_head;
};

static void BindProfileRuntimeData(ViewContext *ctx) {
  if (ctx == NULL)
    return;

  ctx->profile_data = profile;
  ctx->viewer_list = &viewer;
  ctx->dirmenu_list = &dirmenu;
  ctx->filemenu_list = &filemenu;
}

void FreeProfileRuntimeData(ViewContext *ctx) {
  size_t i;
  BindProfileRuntimeData(ctx);

  for (i = 0; i < PROFILE_ENTRIES; ++i) {
    if (profile[i].value != NULL) {
      free(profile[i].value);
      profile[i].value = NULL;
    }
  }

  FreeViewerList(viewer.next);
  viewer.next = NULL;
  FreeFilemenuList(filemenu.next);
  filemenu.next = NULL;
  FreeDirmenuList(dirmenu.next);
  dirmenu.next = NULL;
}


static Viewer *CloneViewerList(const Viewer *head) {
  Viewer *copy_head = NULL;
  Viewer *copy_tail = NULL;

  for (; head != NULL; head = head->next) {
    Viewer *node = xmalloc(sizeof(*node));
    node->ext = head->ext ? xstrdup(head->ext) : NULL;
    node->cmd = head->cmd ? xstrdup(head->cmd) : NULL;
    node->next = NULL;
    if (copy_tail == NULL)
      copy_head = node;
    else
      copy_tail->next = node;
    copy_tail = node;
  }

  return copy_head;
}

static Dirmenu *CloneDirmenuList(const Dirmenu *head) {
  Dirmenu *copy_head = NULL;
  Dirmenu *copy_tail = NULL;

  for (; head != NULL; head = head->next) {
    Dirmenu *node = xmalloc(sizeof(*node));
    node->chkey = head->chkey;
    node->chremap = head->chremap;
    node->cmd = head->cmd ? xstrdup(head->cmd) : NULL;
    node->next = NULL;
    if (copy_tail == NULL)
      copy_head = node;
    else
      copy_tail->next = node;
    copy_tail = node;
  }

  return copy_head;
}

static Filemenu *CloneFilemenuList(const Filemenu *head) {
  Filemenu *copy_head = NULL;
  Filemenu *copy_tail = NULL;

  for (; head != NULL; head = head->next) {
    Filemenu *node = xmalloc(sizeof(*node));
    node->chkey = head->chkey;
    node->chremap = head->chremap;
    node->cmd = head->cmd ? xstrdup(head->cmd) : NULL;
    node->next = NULL;
    if (copy_tail == NULL)
      copy_head = node;
    else
      copy_tail->next = node;
    copy_tail = node;
  }

  return copy_head;
}

static FileColorRule *CloneFileColorRules(const FileColorRule *head) {
  FileColorRule *copy_head = NULL;
  FileColorRule *copy_tail = NULL;

  for (; head != NULL; head = head->next) {
    FileColorRule *node = xmalloc(sizeof(*node));
    node->pattern = head->pattern ? xstrdup(head->pattern) : NULL;
    node->fg = head->fg;
    node->bg = head->bg;
    node->pair_id = head->pair_id;
    node->next = NULL;
    if (copy_tail == NULL)
      copy_head = node;
    else
      copy_tail->next = node;
    copy_tail = node;
  }

  return copy_head;
}

static void FreeViewerList(Viewer *head) {
  Viewer *next;

  for (; head != NULL; head = next) {
    next = head->next;
    if (head->ext != NULL)
      free(head->ext);
    if (head->cmd != NULL)
      free(head->cmd);
    free(head);
  }
}

static void FreeDirmenuList(Dirmenu *head) {
  Dirmenu *next;

  for (; head != NULL; head = next) {
    next = head->next;
    if (head->cmd != NULL)
      free(head->cmd);
    free(head);
  }
}

static void FreeFilemenuList(Filemenu *head) {
  Filemenu *next;

  for (; head != NULL; head = next) {
    next = head->next;
    if (head->cmd != NULL)
      free(head->cmd);
    free(head);
  }
}

static void FreeProfileFileColorRules(FileColorRule *head) {
  FileColorRule *next;

  for (; head != NULL; head = next) {
    next = head->next;
    if (head->pattern != NULL)
      free(head->pattern);
    free(head);
  }
}

ProfileRuntimeSnapshot *ProfileRuntimeSnapshot_Create(ViewContext *ctx) {
  size_t i;
  ProfileRuntimeSnapshot *snapshot;

  BindProfileRuntimeData(ctx);
  snapshot = xmalloc(sizeof(*snapshot));
  memset(snapshot, 0, sizeof(*snapshot));

  for (i = 0; i < PROFILE_ENTRIES; ++i) {
    if (profile[i].value != NULL)
      snapshot->values[i] = xstrdup(profile[i].value);
  }
  snapshot->viewer_next = CloneViewerList(viewer.next);
  snapshot->dirmenu_next = CloneDirmenuList(dirmenu.next);
  snapshot->filemenu_next = CloneFilemenuList(filemenu.next);
  if (ctx != NULL) {
    snapshot->file_color_rules_head =
        CloneFileColorRules((const FileColorRule *)ctx->file_color_rules_head);
  }

  return snapshot;
}

void ProfileRuntimeSnapshot_Restore(ViewContext *ctx,
                                    ProfileRuntimeSnapshot *snapshot) {
  size_t i;

  if (snapshot == NULL)
    return;

  FreeProfileRuntimeData(ctx);
  for (i = 0; i < PROFILE_ENTRIES; ++i) {
    profile[i].value = snapshot->values[i];
    snapshot->values[i] = NULL;
  }
  viewer.next = snapshot->viewer_next;
  snapshot->viewer_next = NULL;
  dirmenu.next = snapshot->dirmenu_next;
  snapshot->dirmenu_next = NULL;
  filemenu.next = snapshot->filemenu_next;
  snapshot->filemenu_next = NULL;
  if (ctx != NULL) {
    FreeProfileFileColorRules((FileColorRule *)ctx->file_color_rules_head);
    ctx->file_color_rules_head = snapshot->file_color_rules_head;
    snapshot->file_color_rules_head = NULL;
  }
  BindProfileRuntimeData(ctx);
}

void ProfileRuntimeSnapshot_Free(ProfileRuntimeSnapshot *snapshot) {
  size_t i;

  if (snapshot == NULL)
    return;

  for (i = 0; i < PROFILE_ENTRIES; ++i) {
    if (snapshot->values[i] != NULL)
      free(snapshot->values[i]);
  }
  FreeViewerList(snapshot->viewer_next);
  FreeDirmenuList(snapshot->dirmenu_next);
  FreeFilemenuList(snapshot->filemenu_next);
  FreeProfileFileColorRules(snapshot->file_color_rules_head);
  free(snapshot);
}

int ValidateProfileFile(ViewContext *ctx, const char *filename) {
  FILE *f;
  char buffer[1024];
  int section = NO_SECTION;
  BOOL invalid = FALSE;
  BOOL read_failed;

  BindProfileRuntimeData(ctx);

  if (filename == NULL)
    return -1;

  f = fopen(filename, "r");
  if (f == NULL)
    return -1;

  while (fgets(buffer, sizeof(buffer), f) != NULL) {
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
      section = ProfileSectionFromHeader(line);
      if (section == -1) {
        invalid = TRUE;
        break;
      }
      continue;
    }

    if (section == NO_SECTION ||
        !ParseProfileAssignment(line, &name, &value)) {
      invalid = TRUE;
      break;
    }

    if (section == GLOBAL_SECTION) {
      Profile key;

      key.name = name;
      if (ctx == NULL || bsearch(&key, (Profile *)ctx->profile_data,
                                 PROFILE_ENTRIES, sizeof(Profile),
                                 Compare) == NULL) {
        invalid = TRUE;
        break;
      }
    }
  }

  read_failed = ferror(f) ? TRUE : FALSE;
  fclose(f);
  if (read_failed)
    return -1;
  return invalid ? 1 : 0;
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

  BindProfileRuntimeData(ctx);

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

static int ProfileSectionFromHeader(const char *name) {
  if (!strcmp(name, "[GLOBAL]"))
    return GLOBAL_SECTION;
  if (!strcmp(name, "[VIEWER]"))
    return VIEWER_SECTION;
  if (!strcmp(name, "[MENU]"))
    return MENU_SECTION;
  if (!strcmp(name, "[FILEMAP]"))
    return FILEMAP_SECTION;
  if (!strcmp(name, "[FILECMD]"))
    return FILECMD_SECTION;
  if (!strcmp(name, "[DIRMAP]"))
    return DIRMAP_SECTION;
  if (!strcmp(name, "[DIRCMD]"))
    return DIRCMD_SECTION;
  return -1;
}

static BOOL ParseProfileAssignment(char *line, char **name, char **value) {
  char *cptr;

  if (line == NULL || name == NULL || value == NULL)
    return FALSE;

  *name = line;
  for (cptr = *name; *cptr && !isspace((unsigned char)*cptr) && *cptr != '=';
       cptr++)
    ;
  if (cptr == *name)
    return FALSE;

  *value = cptr;
  if (**value == '=') {
    *(*value)++ = '\0';
  } else if (**value != '\0') {
    *(*value)++ = '\0';
    while (**value && isspace((unsigned char)**value))
      ++*value;
    if (**value == '=')
      *(*value)++ = '\0';
    else
      *value = NULL;
  } else {
    *value = NULL;
  }
  if (*value == NULL)
    return FALSE;
  while (**value && isspace((unsigned char)**value))
    ++*value;
  return TRUE;
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
