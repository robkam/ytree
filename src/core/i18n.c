/***************************************************************************
 *
 * src/core/i18n.c
 * Runtime gettext bootstrap and locale selection helpers.
 *
 ***************************************************************************/

#include "ytnova_defs.h"
#include "ytnova_i18n.h"
#include <libintl.h>

#ifndef PACKAGED_LOCALE_DIR
#define PACKAGED_LOCALE_DIR "/usr/share/locale"
#endif

#define YTNOVA_TEXTDOMAIN "ytnova"
#define I18N_PRIMARY_LANG_MAX 15
#define I18N_CONTEXT_BUFFER_COUNT 4
#define I18N_CONTEXT_BUFFER_LENGTH 512
#define I18N_MO_MAGIC_LE 0x950412deUL
#define I18N_MO_MAGIC_BE 0xde120495UL
#define I18N_MO_UINT32_BYTES 4UL
#define I18N_MO_TABLE_ENTRY_BYTES 8UL
#define I18N_MO_HEADER_BYTES 28UL
#define I18N_BITS_PER_BYTE 8UL
#define I18N_MAX_FORMAT_SPECS 16
#define I18N_MAX_FORMAT_TOKEN 32

typedef struct I18nCatalogEntry {
  char *key;
  char *value;
  struct I18nCatalogEntry *next;
} I18nCatalogEntry;

typedef struct I18nFormatSpec {
  char conversion;
  char positional_token[I18N_MAX_FORMAT_TOKEN];
} I18nFormatSpec;

static BOOL g_i18n_initialized = FALSE;
static char g_i18n_language[I18N_PRIMARY_LANG_MAX + 1] = "en";
static char g_i18n_context_buffers[I18N_CONTEXT_BUFFER_COUNT]
                                  [I18N_CONTEXT_BUFFER_LENGTH];
static size_t g_i18n_context_index = 0;
static I18nCatalogEntry *g_i18n_catalog = NULL;

static BOOL I18n_LocaleNameUsable(const char *locale_name) {
  if (locale_name == NULL || *locale_name == '\0')
    return FALSE;
  if (!strcmp(locale_name, "C") || !strcmp(locale_name, "POSIX"))
    return FALSE;
  if (!strncmp(locale_name, "C.", 2) || !strncmp(locale_name, "C@", 2))
    return FALSE;
  return TRUE;
}

static BOOL I18n_IsFormatConversion(char ch) {
  switch (ch) {
  case 'd':
  case 'i':
  case 'o':
  case 'u':
  case 'x':
  case 'X':
  case 'f':
  case 'F':
  case 'e':
  case 'E':
  case 'g':
  case 'G':
  case 'a':
  case 'A':
  case 'c':
  case 's':
  case 'p':
    return TRUE;
  default:
    return FALSE;
  }
}

static const char *I18n_ScanFormatTokenEnd(const char *start) {
  const char *cursor = start;

  if (cursor == NULL || *cursor != '%')
    return NULL;
  ++cursor;
  if (*cursor == '%')
    return cursor;
  while (*cursor != '\0') {
    if (I18n_IsFormatConversion(*cursor))
      return cursor;
    if (!strchr("0123456789$#+- .hljztL", *cursor))
      return NULL;
    ++cursor;
  }
  return NULL;
}

static BOOL I18n_BuildPositionalToken(char *dest, size_t dest_size,
                                      const char *token_start,
                                      const char *token_end,
                                      size_t position) {
  size_t suffix_length;
  int written;

  if (dest == NULL || dest_size == 0 || token_start == NULL || token_end == NULL ||
      token_start[0] != '%' || token_end < token_start)
    return FALSE;
  if (memchr(token_start, '*', (size_t)(token_end - token_start + 1)) != NULL)
    return FALSE;

  suffix_length = (size_t)(token_end - token_start);
  written = snprintf(dest, dest_size, "%%%lu$%.*s", (unsigned long)position,
                     (int)suffix_length, token_start + 1);
  return written >= 0 && written < (int)dest_size;
}

static BOOL I18n_CollectFormatSpecs(const char *msgid, I18nFormatSpec *specs,
                                    size_t specs_size, size_t *out_count) {
  size_t count = 0;
  const char *cursor = msgid;

  if (out_count == NULL)
    return FALSE;
  *out_count = 0;
  if (msgid == NULL)
    return FALSE;

  while (*cursor != '\0') {
    const char *token_end;

    if (*cursor != '%') {
      ++cursor;
      continue;
    }
    token_end = I18n_ScanFormatTokenEnd(cursor);
    if (token_end == NULL)
      return FALSE;
    if (*token_end == '%') {
      cursor = token_end + 1;
      continue;
    }
    if (count >= specs_size)
      return FALSE;
    specs[count].conversion = *token_end;
    if (!I18n_BuildPositionalToken(specs[count].positional_token,
                                   sizeof(specs[count].positional_token), cursor,
                                   token_end, count + 1)) {
      return FALSE;
    }
    ++count;
    cursor = token_end + 1;
  }

  *out_count = count;
  return TRUE;
}

static BOOL I18n_AppendString(char **cursor, size_t *remaining,
                              const char *text) {
  size_t length;

  if (cursor == NULL || *cursor == NULL || remaining == NULL || text == NULL)
    return FALSE;

  length = strlen(text);
  if (*remaining <= length)
    return FALSE;
  memcpy(*cursor, text, length);
  *cursor += length;
  **cursor = '\0';
  *remaining -= length;
  return TRUE;
}

static BOOL I18n_AppendChar(char **cursor, size_t *remaining, char ch) {
  if (cursor == NULL || *cursor == NULL || remaining == NULL || *remaining <= 1)
    return FALSE;
  **cursor = ch;
  ++(*cursor);
  **cursor = '\0';
  --(*remaining);
  return TRUE;
}

static BOOL I18n_ParseTranslatedPlaceholder(const char *token_start,
                                            const char **out_end,
                                            size_t *out_position,
                                            char *out_conversion) {
  const char *cursor;
  const char *token_end;
  unsigned long position = 0;

  if (token_start == NULL || *token_start != '%' || out_end == NULL ||
      out_position == NULL || out_conversion == NULL)
    return FALSE;
  cursor = token_start + 1;

  token_end = I18n_ScanFormatTokenEnd(token_start);
  if (token_end == NULL || *token_end == '%')
    return FALSE;

  while (*cursor >= '0' && *cursor <= '9') {
    position = (position * 10UL) + (unsigned long)(*cursor - '0');
    ++cursor;
  }
  if (*cursor == '$') {
    if (position == 0)
      return FALSE;
  } else {
    position = 0;
  }

  *out_end = token_end + 1;
  *out_position = (size_t)position;
  *out_conversion = *token_end;
  return TRUE;
}

static BOOL I18n_BuildSafeFormat(char *dest, size_t dest_size,
                                 const char *msgid,
                                 const char *translated_fmt) {
  I18nFormatSpec specs[I18N_MAX_FORMAT_SPECS];
  BOOL spec_used[I18N_MAX_FORMAT_SPECS];
  size_t spec_count = 0;
  size_t used_count = 0;
  size_t next_sequential = 0;
  size_t i;
  char *out_cursor = dest;
  size_t remaining = dest_size;
  const char *cursor = translated_fmt;

  if (dest == NULL || dest_size == 0 || msgid == NULL || translated_fmt == NULL)
    return FALSE;
  dest[0] = '\0';

  if (!I18n_CollectFormatSpecs(msgid, specs, I18N_MAX_FORMAT_SPECS, &spec_count))
    return FALSE;
  for (i = 0; i < I18N_MAX_FORMAT_SPECS; ++i)
    spec_used[i] = FALSE;

  while (*cursor != '\0') {
    const char *token_end;
    size_t position;
    size_t spec_index;
    char conversion;

    if (*cursor != '%') {
      if (!I18n_AppendChar(&out_cursor, &remaining, *cursor))
        return FALSE;
      ++cursor;
      continue;
    }
    if (cursor[1] == '%') {
      if (!I18n_AppendString(&out_cursor, &remaining, "%%"))
        return FALSE;
      cursor += 2;
      continue;
    }

    if (!I18n_ParseTranslatedPlaceholder(cursor, &token_end, &position,
                                         &conversion)) {
      return FALSE;
    }

    if (position == 0) {
      if (next_sequential >= spec_count)
        return FALSE;
      spec_index = next_sequential++;
    } else {
      if (position > spec_count)
        return FALSE;
      spec_index = position - 1;
    }
    if (conversion != specs[spec_index].conversion)
      return FALSE;
    if (!spec_used[spec_index]) {
      spec_used[spec_index] = TRUE;
      ++used_count;
    }
    if (!I18n_AppendString(&out_cursor, &remaining,
                           specs[spec_index].positional_token)) {
      return FALSE;
    }
    cursor = token_end;
  }

  return used_count == spec_count;
}

static const char *I18n_CurrentLocaleName(void) {
  const char *locale_name = setlocale(LC_MESSAGES, NULL);

  if (locale_name == NULL || *locale_name == '\0')
    locale_name = setlocale(LC_CTYPE, NULL);
  return locale_name;
}

static const char *I18n_RequestedLocaleName(void) {
  const char *locale_name = getenv("LC_ALL");

  if (locale_name != NULL && *locale_name != '\0')
    return locale_name;
  locale_name = getenv("LC_MESSAGES");
  if (locale_name != NULL && *locale_name != '\0')
    return locale_name;
  locale_name = getenv("LANG");
  if (locale_name != NULL && *locale_name != '\0')
    return locale_name;
  return NULL;
}

static void I18n_SetLanguageFromLocale(const char *locale_name) {
  size_t length = 0;

  g_i18n_language[0] = '\0';
  if (locale_name != NULL) {
    while (locale_name[length] != '\0' && locale_name[length] != '_' &&
           locale_name[length] != '.' && locale_name[length] != '@' &&
           locale_name[length] != '-' && length < I18N_PRIMARY_LANG_MAX) {
      g_i18n_language[length] =
          (char)tolower((unsigned char)locale_name[length]);
      ++length;
    }
  }

  if (length == 0 ||
      !strcmp(g_i18n_language, "c") || !strcmp(g_i18n_language, "posix")) {
    (void)snprintf(g_i18n_language, sizeof(g_i18n_language), "%s", "en");
    return;
  }
  g_i18n_language[length] = '\0';
}

static int I18n_JoinPath(char *dest, size_t dest_size, const char *prefix,
                         const char *suffix) {
  int written;

  if (dest == NULL || dest_size == 0 || prefix == NULL || *prefix == '\0' ||
      suffix == NULL || *suffix == '\0')
    return -1;

  written = snprintf(dest, dest_size, "%s/%s", prefix, suffix);
  if (written < 0 || written >= (int)dest_size) {
    dest[0] = '\0';
    return -1;
  }
  return 0;
}

static int I18n_UserLocaleRoot(char *dest, size_t dest_size) {
  const char *xdg_data_home = getenv("XDG_DATA_HOME");
  const char *home = getenv("HOME");

  if (dest == NULL || dest_size == 0)
    return -1;
  dest[0] = '\0';

  if (xdg_data_home != NULL && *xdg_data_home != '\0')
    return I18n_JoinPath(dest, dest_size, xdg_data_home, "locale");
  if (home == NULL || *home == '\0')
    return -1;
  return I18n_JoinPath(dest, dest_size, home, ".local/share/locale");
}

static int I18n_FormatCatalogPath(char *dest, size_t dest_size,
                                  const char *locale_root,
                                  const char *locale_name) {
  int written;

  if (dest == NULL || dest_size == 0 || locale_root == NULL ||
      *locale_root == '\0' || locale_name == NULL || *locale_name == '\0')
    return -1;

  written = snprintf(dest, dest_size, "%s/%s/LC_MESSAGES/%s.mo", locale_root,
                     locale_name, YTNOVA_TEXTDOMAIN);
  if (written < 0 || written >= (int)dest_size) {
    dest[0] = '\0';
    return -1;
  }
  return 0;
}

static int I18n_CatalogExists(const char *locale_root, const char *locale_name) {
  char mo_path[PATH_LENGTH + 1];

  if (I18n_FormatCatalogPath(mo_path, sizeof(mo_path), locale_root,
                             locale_name) != 0)
    return 0;
  return access(mo_path, R_OK) == 0;
}

static BOOL I18n_ResolveCatalogPath(char *dest, size_t dest_size,
                                    const char *locale_root,
                                    const char *locale_name) {
  char candidate[PATH_LENGTH + 1];

  if (dest == NULL || dest_size == 0 || locale_root == NULL ||
      *locale_root == '\0' || locale_name == NULL || *locale_name == '\0')
    return FALSE;

  snprintf(candidate, sizeof(candidate), "%s", locale_name);
  while (candidate[0] != '\0') {
    if (I18n_CatalogExists(locale_root, candidate) &&
        I18n_FormatCatalogPath(dest, dest_size, locale_root, candidate) == 0)
      return TRUE;

    {
      char *trim = strrchr(candidate, '@');

      if (trim != NULL) {
        *trim = '\0';
        continue;
      }
    }
    {
      char *trim = strrchr(candidate, '.');

      if (trim != NULL) {
        *trim = '\0';
        continue;
      }
    }
    {
      char *trim = strrchr(candidate, '_');

      if (trim != NULL) {
        *trim = '\0';
        continue;
      }
    }
    {
      char *trim = strrchr(candidate, '-');

      if (trim != NULL) {
        *trim = '\0';
        continue;
      }
    }
    break;
  }

  return FALSE;
}

static BOOL I18n_UserCatalogAvailable(const char *locale_name) {
  char locale_root[PATH_LENGTH + 1];
  char mo_path[PATH_LENGTH + 1];

  if (locale_name == NULL || *locale_name == '\0')
    return FALSE;
  if (I18n_UserLocaleRoot(locale_root, sizeof(locale_root)) != 0)
    return FALSE;
  return I18n_ResolveCatalogPath(mo_path, sizeof(mo_path), locale_root,
                                 locale_name);
}

static const char *I18n_BindLocaleDir(const char *locale_name) {
  char locale_root[PATH_LENGTH + 1];

  if (locale_name != NULL && *locale_name != '\0' &&
      I18n_UserCatalogAvailable(locale_name) &&
      I18n_UserLocaleRoot(locale_root, sizeof(locale_root)) == 0) {
    return bindtextdomain(YTNOVA_TEXTDOMAIN, locale_root);
  }
  return bindtextdomain(YTNOVA_TEXTDOMAIN, PACKAGED_LOCALE_DIR);
}

static unsigned long I18n_ReadMoUInt32(const unsigned char *buffer,
                                       BOOL big_endian) {
  unsigned long index;
  unsigned long value = 0;

  if (big_endian) {
    for (index = 0; index < I18N_MO_UINT32_BYTES; ++index)
      value = (value << I18N_BITS_PER_BYTE) | (unsigned long)buffer[index];
    return value;
  }
  for (index = 0; index < I18N_MO_UINT32_BYTES; ++index)
    value |=
        ((unsigned long)buffer[index]) << (index * (unsigned long)I18N_BITS_PER_BYTE);
  return value;
}

static void I18n_ClearCatalog(void) {
  I18nCatalogEntry *entry;
  I18nCatalogEntry *next;

  entry = g_i18n_catalog;
  g_i18n_catalog = NULL;
  while (entry != NULL) {
    next = entry->next;
    free(entry->key);
    free(entry->value);
    free(entry);
    entry = next;
  }
}

static int I18n_LoadCatalogFile(const char *catalog_path) {
  FILE *catalog_file;
  unsigned char *buffer = NULL;
  long file_size;
  BOOL big_endian = FALSE;
  unsigned long entry_count;
  unsigned long orig_table_offset;
  unsigned long trans_table_offset;
  unsigned long index;

  if (catalog_path == NULL || *catalog_path == '\0')
    return -1;

  catalog_file = fopen(catalog_path, "rb");
  if (catalog_file == NULL)
    return -1;
  if (fseek(catalog_file, 0, SEEK_END) != 0) {
    fclose(catalog_file);
    return -1;
  }
  file_size = ftell(catalog_file);
  if (file_size < (long)I18N_MO_HEADER_BYTES) {
    fclose(catalog_file);
    return -1;
  }
  if (fseek(catalog_file, 0, SEEK_SET) != 0) {
    fclose(catalog_file);
    return -1;
  }

  buffer = (unsigned char *)malloc((size_t)file_size);
  if (buffer == NULL) {
    fclose(catalog_file);
    return -1;
  }
  if (fread(buffer, 1, (size_t)file_size, catalog_file) != (size_t)file_size) {
    free(buffer);
    fclose(catalog_file);
    return -1;
  }
  fclose(catalog_file);

  if (I18n_ReadMoUInt32(buffer, FALSE) == I18N_MO_MAGIC_LE) {
    big_endian = FALSE;
  } else if (I18n_ReadMoUInt32(buffer, FALSE) == I18N_MO_MAGIC_BE) {
    big_endian = TRUE;
  } else {
    free(buffer);
    return -1;
  }

  entry_count = I18n_ReadMoUInt32(buffer + 8, big_endian);
  orig_table_offset = I18n_ReadMoUInt32(buffer + 12, big_endian);
  trans_table_offset = I18n_ReadMoUInt32(buffer + 16, big_endian);
  if (orig_table_offset + (entry_count * I18N_MO_TABLE_ENTRY_BYTES) >
          (unsigned long)file_size ||
      trans_table_offset + (entry_count * I18N_MO_TABLE_ENTRY_BYTES) >
          (unsigned long)file_size) {
    free(buffer);
    return -1;
  }

  for (index = 0; index < entry_count; ++index) {
    unsigned long orig_length;
    unsigned long orig_offset;
    unsigned long trans_length;
    unsigned long trans_offset;
    I18nCatalogEntry *entry;

    orig_length =
        I18n_ReadMoUInt32(buffer + orig_table_offset +
                              (index * I18N_MO_TABLE_ENTRY_BYTES),
                          big_endian);
    orig_offset =
        I18n_ReadMoUInt32(buffer + orig_table_offset +
                              (index * I18N_MO_TABLE_ENTRY_BYTES) +
                              I18N_MO_UINT32_BYTES,
                          big_endian);
    trans_length =
        I18n_ReadMoUInt32(buffer + trans_table_offset +
                              (index * I18N_MO_TABLE_ENTRY_BYTES),
                          big_endian);
    trans_offset =
        I18n_ReadMoUInt32(buffer + trans_table_offset +
                              (index * I18N_MO_TABLE_ENTRY_BYTES) +
                              I18N_MO_UINT32_BYTES,
                          big_endian);
    if (orig_offset + orig_length > (unsigned long)file_size ||
        trans_offset + trans_length > (unsigned long)file_size)
      continue;
    if (orig_length == 0)
      continue;

    entry = (I18nCatalogEntry *)calloc(1, sizeof(*entry));
    if (entry == NULL)
      continue;
    entry->key = (char *)malloc((size_t)orig_length + 1);
    entry->value = (char *)malloc((size_t)trans_length + 1);
    if (entry->key == NULL || entry->value == NULL) {
      free(entry->key);
      free(entry->value);
      free(entry);
      continue;
    }

    memcpy(entry->key, buffer + orig_offset, (size_t)orig_length);
    entry->key[orig_length] = '\0';
    memcpy(entry->value, buffer + trans_offset, (size_t)trans_length);
    entry->value[trans_length] = '\0';
    entry->next = g_i18n_catalog;
    g_i18n_catalog = entry;
  }

  free(buffer);
  return 0;
}

static void I18n_LoadFallbackCatalog(const char *locale_name) {
  char locale_root[PATH_LENGTH + 1];
  char mo_path[PATH_LENGTH + 1];

  I18n_ClearCatalog();
  if (!I18n_LocaleNameUsable(locale_name))
    return;

  if (I18n_UserLocaleRoot(locale_root, sizeof(locale_root)) == 0 &&
      I18n_ResolveCatalogPath(mo_path, sizeof(mo_path), locale_root,
                              locale_name)) {
    (void)I18n_LoadCatalogFile(mo_path);
    return;
  }
  if (I18n_ResolveCatalogPath(mo_path, sizeof(mo_path), PACKAGED_LOCALE_DIR,
                              locale_name))
    (void)I18n_LoadCatalogFile(mo_path);
}

static const char *I18n_CatalogLookup(const char *msgid) {
  I18nCatalogEntry *entry;

  if (msgid == NULL || *msgid == '\0')
    return NULL;
  for (entry = g_i18n_catalog; entry != NULL; entry = entry->next) {
    if (!strcmp(entry->key, msgid))
      return entry->value;
  }
  return NULL;
}

const char *I18n_Gettext(const char *msgid) {
  const char *translated;
  const char *fallback;

  if (msgid == NULL)
    return "";

  translated = gettext(msgid);
  if (translated != NULL && strcmp(translated, msgid) != 0)
    return translated;

  fallback = I18n_CatalogLookup(msgid);
  return (fallback != NULL) ? fallback : msgid;
}

int I18n_VFormat(char *dest, size_t dest_size, const char *msgid, va_list ap) {
  const char *translated_fmt;
  size_t safe_fmt_size;
  char *safe_fmt;

  if (dest == NULL || dest_size == 0)
    return -1;
  dest[0] = '\0';
  if (msgid == NULL)
    return 0;

  translated_fmt = I18n_Gettext(msgid);
  safe_fmt_size = (strlen(translated_fmt) * 2U) + 64U;
  safe_fmt = (char *)malloc(safe_fmt_size);
  if (safe_fmt != NULL &&
      I18n_BuildSafeFormat(safe_fmt, safe_fmt_size, msgid, translated_fmt)) {
    int written = vsnprintf(dest, dest_size, safe_fmt, ap);

    free(safe_fmt);
    return written;
  }
  free(safe_fmt);
  return vsnprintf(dest, dest_size, msgid, ap);
}

int I18n_Format(char *dest, size_t dest_size, const char *msgid, ...) {
  va_list ap;
  int written;

  va_start(ap, msgid);
  written = I18n_VFormat(dest, dest_size, msgid, ap);
  va_end(ap);
  return written;
}

const char *I18n_PGettext(const char *context, const char *msgid) {
  const char *translated;
  const char *fallback;
  char *buffer;
  int written;

  if (msgid == NULL)
    return "";
  if (context == NULL || *context == '\0')
    return I18n_Gettext(msgid);

  buffer = g_i18n_context_buffers[g_i18n_context_index];
  g_i18n_context_index =
      (g_i18n_context_index + 1) % I18N_CONTEXT_BUFFER_COUNT;
  written = snprintf(buffer, I18N_CONTEXT_BUFFER_LENGTH, "%s\004%s", context,
                     msgid);
  if (written < 0 || written >= I18N_CONTEXT_BUFFER_LENGTH)
    return I18n_Gettext(msgid);

  translated = gettext(buffer);
  if (translated != NULL && strcmp(translated, buffer) != 0)
    return translated;

  fallback = I18n_CatalogLookup(buffer);
  if (fallback != NULL)
    return fallback;
  return msgid;
}

void I18n_Init(void) {
  const char *locale_name;
  const char *requested_locale_name;

  if (g_i18n_initialized)
    return;

  requested_locale_name = I18n_RequestedLocaleName();
  setlocale(LC_ALL, "");
  locale_name = I18n_CurrentLocaleName();
  if (!I18n_LocaleNameUsable(locale_name) &&
      I18n_LocaleNameUsable(requested_locale_name))
    locale_name = requested_locale_name;
  I18n_SetLanguageFromLocale(locale_name);
  (void)I18n_BindLocaleDir(locale_name);
  I18n_LoadFallbackCatalog(locale_name);
#ifdef HAVE_BIND_TEXTDOMAIN_CODESET
  bind_textdomain_codeset(YTNOVA_TEXTDOMAIN, "UTF-8");
#else
  bind_textdomain_codeset(YTNOVA_TEXTDOMAIN, "UTF-8");
#endif
  textdomain(YTNOVA_TEXTDOMAIN);
  g_i18n_initialized = TRUE;
}

const char *I18n_GetLanguage(void) {
  if (!g_i18n_initialized)
    I18n_Init();
  return g_i18n_language;
}
