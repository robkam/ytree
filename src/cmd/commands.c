/***************************************************************************
 *
 * src/cmd/commands.c
 * commands.conf loader and discovery helpers.
 *
 ***************************************************************************/

#include "ytnova_cmd.h"
#include "../core/default_command_presets_catalog.h"
#include "../core/default_commands_catalog.h"
#include <ctype.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

typedef struct {
  const char *context;
  const char *action_id;
  int default_key;
} CommandActionSpec;

static const CommandActionSpec kCommandActions[] = {
    {"dir", "ACTION_CMD_A", 'a'},
    {"dir", "ACTION_CMD_C", 'c'},
    {"dir", "ACTION_CMD_D", 'd'},
    {"dir", "ACTION_FILTER", 'f'},
    {"dir", "ACTION_CMD_G", 'g'},
    {"dir", "ACTION_INVERT", 'i'},
    {"dir", "ACTION_COMPARE_DIR", 'j'},
    {"dir", "ACTION_LOG", 'l'},
    {"dir", "ACTION_CMD_M", 'm'},
    {"dir", "ACTION_CMD_MKFILE", 'n'},
    {"dir", "ACTION_TOGGLE_TAGGED_MODE", 'o'},
    {"dir", "ACTION_CMD_P", 'p'},
    {"dir", "ACTION_QUIT", 'q'},
    {"dir", "ACTION_CMD_R", 'r'},
    {"dir", "ACTION_CMD_S", 's'},
    {"dir", "ACTION_TAG", 't'},
    {"dir", "ACTION_UNTAG", 'u'},
    {"dir", "ACTION_CMD_V", 'v'},
    {"dir", "ACTION_CMD_PRINT", 'w'},
    {"dir", "ACTION_CMD_X", 'x'},
    {"dir", "ACTION_CMD_I", 'z'},
    {"dir", "ACTION_LIST_JUMP", '/'},
    {"dir", "ACTION_TOGGLE_HIDDEN", '`'},
    {"archive_dir", "ACTION_CMD_C", 'c'},
    {"archive_dir", "ACTION_CMD_D", 'd'},
    {"archive_dir", "ACTION_FILTER", 'f'},
    {"archive_dir", "ACTION_CMD_G", 'g'},
    {"archive_dir", "ACTION_COMPARE_DIR", 'j'},
    {"archive_dir", "ACTION_LOG", 'l'},
    {"archive_dir", "ACTION_CMD_M", 'm'},
    {"archive_dir", "ACTION_CMD_P", 'p'},
    {"archive_dir", "ACTION_CMD_R", 'r'},
    {"archive_dir", "ACTION_CMD_S", 's'},
    {"archive_dir", "ACTION_TAG", 't'},
    {"archive_dir", "ACTION_UNTAG", 'u'},
    {"archive_dir", "ACTION_CMD_V", 'v'},
    {"archive_dir", "ACTION_QUIT", 'q'},
    {"archive_dir", "ACTION_LIST_JUMP", '/'},
    {"archive_dir", "ACTION_TOGGLE_HIDDEN", '`'},
    {"file", "ACTION_CMD_A", 'a'},
    {"file", "ACTION_CMD_C", 'c'},
    {"file", "ACTION_CMD_TAGGED_C", 0x0B},
    {"file", "ACTION_CMD_D", 'd'},
    {"file", "ACTION_CMD_E", 'e'},
    {"file", "ACTION_FILTER", 'f'},
    {"file", "ACTION_CMD_H", 'h'},
    {"file", "ACTION_INVERT", 'i'},
    {"file", "ACTION_COMPARE_FILE", 'j'},
    {"file", "ACTION_LOG", 'l'},
    {"file", "ACTION_CMD_M", 'm'},
    {"file", "ACTION_CMD_TAGGED_M", 0x0E},
    {"file", "ACTION_CMD_MKFILE", 'n'},
    {"file", "ACTION_TOGGLE_TAGGED_MODE", 'o'},
    {"file", "ACTION_CMD_P", 'p'},
    {"file", "ACTION_QUIT", 'q'},
    {"file", "ACTION_CMD_R", 'r'},
    {"file", "ACTION_CMD_S", 's'},
    {"file", "ACTION_CMD_PRINT", 'w'},
    {"file", "ACTION_CMD_X", 'x'},
    {"file", "ACTION_CMD_Y", 'y'},
    {"file", "ACTION_CMD_I", 'z'},
    {"file", "ACTION_LIST_JUMP", '/'},
    {"file", "ACTION_TOGGLE_HIDDEN", '`'},
    {"archive_file", "ACTION_CMD_C", 'c'},
    {"archive_file", "ACTION_CMD_D", 'd'},
    {"archive_file", "ACTION_FILTER", 'f'},
    {"archive_file", "ACTION_CMD_H", 'h'},
    {"archive_file", "ACTION_INVERT", 'i'},
    {"archive_file", "ACTION_COMPARE_FILE", 'j'},
    {"archive_file", "ACTION_CMD_M", 'm'},
    {"archive_file", "ACTION_CMD_P", 'p'},
    {"archive_file", "ACTION_CMD_R", 'r'},
    {"archive_file", "ACTION_CMD_S", 's'},
    {"archive_file", "ACTION_TAG", 't'},
    {"archive_file", "ACTION_UNTAG", 'u'},
    {"archive_file", "ACTION_CMD_Y", 'y'},
    {"archive_file", "ACTION_LIST_JUMP", '/'},
    {"archive_file", "ACTION_TOGGLE_HIDDEN", '`'},
};

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

static int BindingTokenToKeyCode(const char *token) {
  const unsigned char *text = (const unsigned char *)token;

  if (text == NULL)
    return -1;
  while (*text && isspace(*text))
    ++text;
  if (*text == '\0')
    return -1;

  if (text[0] != '\0' && text[1] == '\0')
    return (int)text[0];

  if ((text[0] == '^' || text[0] == '~') && text[1] != '\0' &&
      text[2] == '\0') {
    unsigned char ch = (unsigned char)toupper(text[1]);
    return (int)(ch & 0x1F);
  }

  if (strncasecmp((const char *)text, "Ctrl+", 5) == 0 && text[5] != '\0' &&
      text[6] == '\0' && isalpha(text[5])) {
    unsigned char ch = (unsigned char)toupper(text[5]);
    return (int)(ch & 0x1F);
  }

  return -1;
}

int CommandKeyCodeToToken(int key_code, char *token, size_t token_size) {
  if (token == NULL || token_size == 0)
    return -1;

  token[0] = '\0';
  if (key_code <= 0)
    return -1;

  if (key_code < 0x20 && isalpha((unsigned char)(key_code | 0x40)))
    return snprintf(token, token_size, "^%c",
                    (char)toupper((unsigned char)(key_code | 0x40))) >=
                   (int)token_size
               ? -1
               : 0;

  if (isprint((unsigned char)key_code))
    return snprintf(token, token_size, "%c", (char)toupper(key_code)) >=
                   (int)token_size
               ? -1
               : 0;

  return -1;
}

int CommandActionDefaultKeyCode(const char *context, const char *action_id) {
  size_t index;

  if (context == NULL || action_id == NULL)
    return -1;

  for (index = 0; index < sizeof(kCommandActions) / sizeof(kCommandActions[0]);
       ++index) {
    if (strcmp(kCommandActions[index].context, context) == 0 &&
        strcmp(kCommandActions[index].action_id, action_id) == 0)
      return kCommandActions[index].default_key;
  }
  return -1;
}

static int ApplyContextBinding(ViewContext *ctx, const char *context,
                               int binding_key, const char *action_id,
                               const char *command) {
  int default_key;

  if (strcmp(action_id, "user-command") == 0)
    return Profile_SetCommandSurfaceUserAction(ctx, context, binding_key, -1,
                                               command);

  default_key = CommandActionDefaultKeyCode(context, action_id);
  if (default_key < 0)
    return -1;

  return Profile_SetCommandSurfaceUserAction(ctx, context, binding_key,
                                             default_key, NULL);
}

static int IsSupportedContextName(const char *context_name) {
  return context_name != NULL &&
         (strcmp(context_name, "dir") == 0 ||
          strcmp(context_name, "archive_dir") == 0 ||
          strcmp(context_name, "file") == 0 ||
          strcmp(context_name, "archive_file") == 0);
}

static int CommandPresentationEntriesForContext(
    ViewContext *ctx, const char *context_name,
    CommandPresentationOverride **entries_out, size_t **entry_count_out) {
  if (ctx == NULL || context_name == NULL || entries_out == NULL ||
      entry_count_out == NULL)
    return -1;

  if (strcmp(context_name, "dir") == 0) {
    *entries_out = ctx->dir_command_presentations;
    *entry_count_out = &ctx->dir_command_presentation_count;
    return 0;
  }
  if (strcmp(context_name, "archive_dir") == 0) {
    *entries_out = ctx->archive_dir_command_presentations;
    *entry_count_out = &ctx->archive_dir_command_presentation_count;
    return 0;
  }
  if (strcmp(context_name, "file") == 0) {
    *entries_out = ctx->file_command_presentations;
    *entry_count_out = &ctx->file_command_presentation_count;
    return 0;
  }
  if (strcmp(context_name, "archive_file") == 0) {
    *entries_out = ctx->archive_file_command_presentations;
    *entry_count_out = &ctx->archive_file_command_presentation_count;
    return 0;
  }

  return -1;
}

static int StoreCommandPresentation(ViewContext *ctx, const char *context_name,
                                    const char *action_id, const char *shown,
                                    const char *label) {
  CommandPresentationOverride *entries;
  size_t *entry_count;
  size_t index;

  if (ctx == NULL || context_name == NULL || action_id == NULL ||
      shown == NULL || label == NULL || strcmp(action_id, "user-command") == 0)
    return 0;
  if (CommandPresentationEntriesForContext(ctx, context_name, &entries,
                                           &entry_count) != 0)
    return -1;

  for (index = 0; index < *entry_count; ++index) {
    if (strcmp(entries[index].action_id, action_id) == 0) {
      if (snprintf(entries[index].shown, sizeof(entries[index].shown), "%s",
                   shown) >= (int)sizeof(entries[index].shown) ||
          snprintf(entries[index].label, sizeof(entries[index].label), "%s",
                   label) >= (int)sizeof(entries[index].label))
        return -1;
      return 0;
    }
  }

  if (*entry_count >= COMMAND_PRESENTATION_OVERRIDES_MAX)
    return -1;

  if (snprintf(entries[*entry_count].action_id,
               sizeof(entries[*entry_count].action_id), "%s", action_id) >=
          (int)sizeof(entries[*entry_count].action_id) ||
      snprintf(entries[*entry_count].shown, sizeof(entries[*entry_count].shown),
               "%s", shown) >= (int)sizeof(entries[*entry_count].shown) ||
      snprintf(entries[*entry_count].label, sizeof(entries[*entry_count].label),
               "%s", label) >= (int)sizeof(entries[*entry_count].label))
    return -1;
  ++*entry_count;
  return 0;
}

static int ParseContextSectionName(char *line, char *context_name,
                                   size_t context_name_size) {
  const char *name;
  char *end;
  size_t index;

  if (line == NULL || line[0] != '[')
    return 0;

  end = strrchr(line, ']');
  if (end == NULL || end[1] != '\0' || end == line + 1)
    return -1;
  *end = '\0';

  name = TrimInPlace(line + 1);
  if (name == NULL || *name == '\0' ||
      snprintf(context_name, context_name_size, "%s", name) >=
          (int)context_name_size) {
    *end = ']';
    return -1;
  }
  for (index = 0; context_name[index] != '\0'; ++index)
    context_name[index] = (char)tolower((unsigned char)context_name[index]);

  *end = ']';
  return IsSupportedContextName(context_name) ? 1 : -1;
}

static int IsValidPresetId(const char *preset_id) {
  size_t index;

  if (preset_id == NULL || *preset_id == '\0' ||
      strlen(preset_id) >= COMMAND_PRESET_ID_LENGTH)
    return 0;

  for (index = 0; preset_id[index] != '\0'; ++index) {
    unsigned char ch = (unsigned char)preset_id[index];

    if (!(isdigit(ch) || (ch >= 'a' && ch <= 'z') || ch == '-'))
      return 0;
  }

  return 1;
}

static int ParsePresetSelectorLine(char *line, char *preset_id,
                                   size_t preset_id_size) {
  char *cursor;
  const char *value;

  if (line == NULL)
    return 0;

  cursor = TrimInPlace(line);
  if (cursor == NULL || *cursor == '\0')
    return 0;
  if (strncasecmp(cursor, "preset", 6) != 0)
    return 0;
  if (cursor[6] != '\0' && !isspace((unsigned char)cursor[6]) &&
      cursor[6] != '=')
    return 0;

  cursor += 6;
  while (*cursor && isspace((unsigned char)*cursor))
    ++cursor;
  if (*cursor != '=')
    return -1;
  ++cursor;
  value = TrimInPlace(cursor);
  if (value == NULL || !IsValidPresetId(value) ||
      snprintf(preset_id, preset_id_size, "%s", value) >=
          (int)preset_id_size)
    return -1;
  return 1;
}

static int ScanPresetSelector(FILE *fp, int allow_selector, char *preset_id,
                              size_t preset_id_size) {
  char buffer[2048];
  int found_selector = 0;
  int saw_rows = 0;

  if (fp == NULL)
    return -1;

  rewind(fp);
  while (fgets(buffer, sizeof(buffer), fp) != NULL) {
    char *comment;
    char *line;
    char parsed_preset[COMMAND_PRESET_ID_LENGTH];
    int preset_result;

    if ((comment = strchr(buffer, '#')) != NULL)
      *comment = '\0';
    line = TrimInPlace(buffer);
    if (line == NULL || *line == '\0')
      continue;

    preset_result =
        ParsePresetSelectorLine(line, parsed_preset, sizeof(parsed_preset));
    if (preset_result < 0) {
      rewind(fp);
      return -1;
    }
    if (preset_result > 0) {
      if (!allow_selector || found_selector || saw_rows ||
          snprintf(preset_id, preset_id_size, "%s", parsed_preset) >=
              (int)preset_id_size) {
        rewind(fp);
        return -1;
      }
      found_selector = 1;
      continue;
    }

    saw_rows = 1;
  }

  rewind(fp);
  return found_selector;
}

static int ProcessCommandsColumns(ViewContext *ctx, char *context_column,
                                  char *binding_column, char *shown_column,
                                  char *label_column, char *action_column,
                                  char *command_column, int *line_error) {
  char contexts_buf[256];
  char bindings_buf[256];
  char *context_token;
  char *binding_token;
  char *context_save = NULL;
  char *binding_save = NULL;
  const char *action_id;
  const char *command;
  int command_is_user;

  *line_error = 0;
  context_column = TrimInPlace(context_column);
  binding_column = TrimInPlace(binding_column);
  shown_column = TrimInPlace(shown_column);
  label_column = TrimInPlace(label_column);
  action_column = TrimInPlace(action_column);
  command_column = TrimInPlace(command_column);

  if (context_column == NULL || *context_column == '\0' || binding_column == NULL ||
      *binding_column == '\0' || shown_column == NULL || *shown_column == '\0' ||
      label_column == NULL || *label_column == '\0' || action_column == NULL ||
      *action_column == '\0') {
    *line_error = 1;
    return -1;
  }

  action_id = action_column;
  command = command_column != NULL ? command_column : "";
  command_is_user = strcmp(action_id, "user-command") == 0;
  if (command_is_user) {
    if (*command == '\0') {
      *line_error = 1;
      return -1;
    }
  } else if (*command != '\0') {
    *line_error = 1;
    return -1;
  }

  if (snprintf(contexts_buf, sizeof(contexts_buf), "%s", context_column) >=
          (int)sizeof(contexts_buf) ||
      snprintf(bindings_buf, sizeof(bindings_buf), "%s", binding_column) >=
          (int)sizeof(bindings_buf)) {
    *line_error = 1;
    return -1;
  }

  for (context_token = strtok_r(contexts_buf, ",", &context_save);
       context_token != NULL;
       context_token = strtok_r(NULL, ",", &context_save)) {
    char context_name[32];

    context_token = TrimInPlace(context_token);
    if (context_token == NULL || *context_token == '\0') {
      *line_error = 1;
      return -1;
    }
    if (snprintf(context_name, sizeof(context_name), "%s", context_token) >=
        (int)sizeof(context_name)) {
      *line_error = 1;
      return -1;
    }
    if (!IsSupportedContextName(context_name)) {
      *line_error = 1;
      return -1;
    }

    binding_save = NULL;
    if (snprintf(bindings_buf, sizeof(bindings_buf), "%s", binding_column) >=
        (int)sizeof(bindings_buf)) {
      *line_error = 1;
      return -1;
    }
    for (binding_token = strtok_r(bindings_buf, ",", &binding_save);
         binding_token != NULL;
         binding_token = strtok_r(NULL, ",", &binding_save)) {
      int binding_key;

      binding_token = TrimInPlace(binding_token);
      binding_key = BindingTokenToKeyCode(binding_token);
      if (binding_token == NULL || *binding_token == '\0' || binding_key < 0) {
        *line_error = 1;
        return -1;
      }
      if (ctx != NULL &&
          ApplyContextBinding(ctx, context_name, binding_key, action_id, command) !=
              0) {
        *line_error = 1;
        return -1;
      }
      if (ctx != NULL &&
          StoreCommandPresentation(ctx, context_name, action_id, shown_column,
                                   label_column) != 0) {
        *line_error = 1;
        return -1;
      }
      if (ctx == NULL && !command_is_user &&
          CommandActionDefaultKeyCode(context_name, action_id) < 0) {
        *line_error = 1;
        return -1;
      }
    }
  }

  return 0;
}

static int ProcessCommandsFile(ViewContext *ctx, FILE *fp) {
  char buffer[2048];
  char active_context[32];

  active_context[0] = '\0';
  while (fgets(buffer, sizeof(buffer), fp) != NULL) {
    char *parts[6];
    char *cursor;
    char *line;
    char *comment;
    char preset_id[COMMAND_PRESET_ID_LENGTH];
    int index;
    int line_error;
    int separator_count;
    int section_result;
    int preset_result;

    if ((comment = strchr(buffer, '#')) != NULL)
      *comment = '\0';
    line = TrimInPlace(buffer);
    if (line == NULL || *line == '\0')
      continue;

    preset_result = ParsePresetSelectorLine(line, preset_id, sizeof(preset_id));
    if (preset_result > 0)
      continue;
    if (preset_result < 0)
      return 1;

    section_result =
        ParseContextSectionName(line, active_context, sizeof(active_context));
    if (section_result > 0)
      continue;
    if (section_result < 0)
      return 1;

    separator_count = 0;
    for (cursor = line; *cursor != '\0'; ++cursor) {
      if (*cursor == '|')
        ++separator_count;
    }
    if (separator_count != 4 && separator_count != 5)
      return 1;

    cursor = line;
    for (index = 0; index < separator_count + 1; ++index) {
      parts[index] = cursor;
      if (index < separator_count) {
        cursor = strchr(cursor, '|');
        if (cursor == NULL)
          return 1;
        *cursor++ = '\0';
      }
    }

    if (separator_count == 5) {
      if (ProcessCommandsColumns(ctx, parts[0], parts[1], parts[2], parts[3],
                                 parts[4], parts[5], &line_error) != 0)
        return line_error ? 1 : -1;
      continue;
    }

    if (active_context[0] == '\0')
      return 1;
    if (ProcessCommandsColumns(ctx, active_context, parts[0], parts[1], parts[2],
                               parts[3], parts[4], &line_error) != 0)
      return line_error ? 1 : -1;
  }

  return ferror(fp) ? -1 : 0;
}

static const char *FindCompiledPresetText(const char *preset_id) {
  size_t index;

  if (preset_id == NULL)
    return NULL;

  for (index = 0; index < default_command_presets_catalog_count; ++index) {
    if (strcmp(default_command_presets_catalog[index].preset_id, preset_id) == 0)
      return default_command_presets_catalog[index].contents;
  }

  return NULL;
}

static int ReadCommandsStream(ViewContext *ctx, FILE *fp, int allow_selector,
                              int apply_selector_preset);

static int LoadPresetSource(ViewContext *ctx, const char *preset_id) {
  char path[PATH_LENGTH + 1];
  const char *compiled_preset;
  FILE *fp;
  int path_len;
  int result;

  if (!IsValidPresetId(preset_id))
    return -1;

  path_len = snprintf(path, sizeof(path), "%s/%s.conf", PACKAGED_COMMAND_PRESET_DIR,
                      preset_id);
  if (path_len < 0 || (size_t)path_len >= sizeof(path))
    return -1;

  fp = fopen(path, "r");
  if (fp != NULL) {
    result = ReadCommandsStream(ctx, fp, FALSE, FALSE);
    fclose(fp);
    return result;
  }
  if (errno != ENOENT)
    return -1;

  compiled_preset = FindCompiledPresetText(preset_id);
  if (compiled_preset == NULL)
    return -1;

  fp = fmemopen((void *)compiled_preset, strlen(compiled_preset), "r");
  if (fp == NULL)
    return -1;
  result = ReadCommandsStream(ctx, fp, FALSE, FALSE);
  fclose(fp);
  return result;
}

static int ReadCommandsStream(ViewContext *ctx, FILE *fp, int allow_selector,
                              int apply_selector_preset) {
  char preset_id[COMMAND_PRESET_ID_LENGTH];
  int selector_result;
  int result;

  if (fp == NULL)
    return -1;

  selector_result =
      ScanPresetSelector(fp, allow_selector, preset_id, sizeof(preset_id));
  if (selector_result < 0)
    return -1;
  if (selector_result > 0 && apply_selector_preset &&
      LoadPresetSource(ctx, preset_id) != 0)
    return -1;

  rewind(fp);
  result = ProcessCommandsFile(ctx, fp);
  return result;
}

static int LoadPackagedDefaultCommands(ViewContext *ctx) {
  FILE *fp;
  int result;

  fp = fopen(PACKAGED_COMMANDS_PATH, "r");
  if (fp != NULL) {
    result = ReadCommandsStream(ctx, fp, TRUE, TRUE);
    fclose(fp);
    return result;
  }
  if (errno != ENOENT)
    return -1;

  fp = fmemopen((void *)default_commands_catalog, strlen(default_commands_catalog),
                "r");
  if (fp == NULL)
    return -1;
  result = ReadCommandsStream(ctx, fp, TRUE, TRUE);
  fclose(fp);
  return result;
}

static int ResolveExistingCommandsPath(char *path, size_t path_size) {
  int result;

  if (path == NULL || path_size == 0)
    return -1;
  path[0] = '\0';

  if (ConfigPaths_ResolvePreferredPath(CONFIG_SURFACE_COMMANDS, path, path_size) ==
      0) {
    if (access(path, F_OK) == 0)
      return 0;
    if (errno != ENOENT)
      return -1;
  }

  result = ConfigPaths_ResolveLegacyPath(CONFIG_SURFACE_COMMANDS, path, path_size,
                                         FALSE);
  if (result == 0) {
    if (access(path, F_OK) == 0)
      return 0;
    if (errno != ENOENT)
      return -1;
  }

  path[0] = '\0';
  return 1;
}

static int ValidateResolvedCommands(const ViewContext *ctx) {
  int resolved_keys[sizeof(kCommandActions) / sizeof(kCommandActions[0])];
  size_t index;
  size_t prior;

  for (index = 0; index < sizeof(kCommandActions) / sizeof(kCommandActions[0]);
       ++index) {
    resolved_keys[index] = ResolveCommandBindingKeyForContext(
        ctx, kCommandActions[index].context, kCommandActions[index].default_key);
    if (resolved_keys[index] < 0)
      return -1;

    for (prior = 0; prior < index; ++prior) {
      if (strcmp(kCommandActions[prior].context, kCommandActions[index].context) ==
              0 &&
          resolved_keys[prior] == resolved_keys[index])
        return -1;
    }
  }

  return 0;
}

int ValidateCommandsFile(const char *filename) {
  FILE *fp;
  int result;

  if (filename == NULL || *filename == '\0')
    return -1;

  fp = fopen(filename, "r");
  if (fp == NULL)
    return -1;
  result = ReadCommandsStream(NULL, fp, TRUE, TRUE);
  fclose(fp);
  return result;
}

int ReadCommandsFile(ViewContext *ctx, const char *filename) {
  FILE *fp;
  int result;

  if (ctx == NULL || filename == NULL || *filename == '\0')
    return -1;

  fp = fopen(filename, "r");
  if (fp == NULL)
    return -1;
  result = ReadCommandsStream(ctx, fp, TRUE, TRUE);
  fclose(fp);
  return result;
}

int LoadConfiguredCommands(ViewContext *ctx) {
  char path[PATH_LENGTH + 1];
  char preset_id[COMMAND_PRESET_ID_LENGTH];
  FILE *fp;
  int path_result;
  int selector_result = 0;

  if (ctx == NULL)
    return -1;

  ctx->commands_file_path[0] = '\0';
  Profile_ClearCommandRuntime(ctx);

  path_result = ResolveExistingCommandsPath(path, sizeof(path));
  if (path_result < 0)
    return -1;

  if (path_result == 0) {
    fp = fopen(path, "r");
    if (fp == NULL)
      return -1;
    selector_result =
        ScanPresetSelector(fp, TRUE, preset_id, sizeof(preset_id));
    fclose(fp);
    if (selector_result < 0)
      return -1;
  }

  if (path_result == 0 && selector_result > 0) {
    if (LoadPresetSource(ctx, preset_id) != 0)
      return -1;
  } else if (LoadPackagedDefaultCommands(ctx) != 0) {
    return -1;
  }

  if (path_result == 0) {
    fp = fopen(path, "r");
    if (fp == NULL)
      return -1;
    if (ReadCommandsStream(ctx, fp, TRUE, FALSE) != 0) {
      fclose(fp);
      return -1;
    }
    fclose(fp);
    (void)snprintf(ctx->commands_file_path, sizeof(ctx->commands_file_path),
                   "%s", path);
  }

  return ValidateResolvedCommands(ctx);
}

static int CoreInit_LoadCommands(ViewContext *ctx) {
  return LoadConfiguredCommands(ctx);
}

void CoreInitOps_RegisterCmdCommands(CoreInitOps *ops) {
  if (ops == NULL)
    return;

  ops->load_commands = CoreInit_LoadCommands;
}
