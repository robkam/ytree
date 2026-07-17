/***************************************************************************
 *
 * src/cmd/commands.c
 * commands.conf loader and discovery helpers.
 *
 ***************************************************************************/

#include "ytnova_cmd.h"
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

  if (strcmp(action_id, "user-command") == 0) {
    if (strcmp(context, "dir") == 0)
      return Profile_SetDirUserAction(ctx, binding_key, -1, command);
    if (strcmp(context, "file") == 0)
      return Profile_SetFileUserAction(ctx, binding_key, -1, command);
    return -1;
  }

  default_key = CommandActionDefaultKeyCode(context, action_id);
  if (default_key < 0)
    return -1;

  if (strcmp(context, "dir") == 0)
    return Profile_SetDirUserAction(ctx, binding_key, default_key, NULL);
  if (strcmp(context, "file") == 0)
    return Profile_SetFileUserAction(ctx, binding_key, default_key, NULL);
  return -1;
}

static int IsSupportedContextName(const char *context_name) {
  return context_name != NULL &&
         (strcmp(context_name, "dir") == 0 || strcmp(context_name, "file") == 0);
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

  if (strcmp(context_name, "dir") == 0) {
    entries = ctx->dir_command_presentations;
    entry_count = &ctx->dir_command_presentation_count;
  } else if (strcmp(context_name, "file") == 0) {
    entries = ctx->file_command_presentations;
    entry_count = &ctx->file_command_presentation_count;
  } else {
    return -1;
  }

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
  int line_no = 0;

  active_context[0] = '\0';
  while (fgets(buffer, sizeof(buffer), fp) != NULL) {
    char *parts[6];
    char *cursor;
    char *line;
    char *comment;
    int index;
    int line_error;
    int separator_count;
    int section_result;

    ++line_no;
    if ((comment = strchr(buffer, '#')) != NULL)
      *comment = '\0';
    line = TrimInPlace(buffer);
    if (line == NULL || *line == '\0')
      continue;

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

static int TryLoadCommandsFile(ViewContext *ctx, const char *path) {
  int read_result;

  if (path == NULL || *path == '\0')
    return 1;
  if (access(path, F_OK) != 0) {
    if (errno == ENOENT)
      return 1;
    return -1;
  }

  read_result = ReadCommandsFile(ctx, path);
  if (read_result != 0)
    return -1;
  (void)snprintf(ctx->commands_file_path, sizeof(ctx->commands_file_path), "%s",
                 path);
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
  result = ProcessCommandsFile(NULL, fp);
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
  result = ProcessCommandsFile(ctx, fp);
  fclose(fp);
  return result;
}

int LoadConfiguredCommands(ViewContext *ctx) {
  char path[PATH_LENGTH + 1];
  int result;

  if (ctx == NULL)
    return -1;

  ctx->commands_file_path[0] = '\0';
  ctx->dir_command_presentation_count = 0;
  ctx->file_command_presentation_count = 0;
  if (ConfigPaths_ResolvePreferredPath(CONFIG_SURFACE_COMMANDS, path,
                                       sizeof(path)) == 0) {
    result = TryLoadCommandsFile(ctx, path);
    if (result != 1)
      return result;
  }

  if (ConfigPaths_ResolveLegacyPath(CONFIG_SURFACE_COMMANDS, path, sizeof(path),
                                    FALSE) == 0) {
    result = TryLoadCommandsFile(ctx, path);
    if (result != 1)
      return result;
  }

  return 0;
}

static int CoreInit_LoadCommands(ViewContext *ctx) {
  return LoadConfiguredCommands(ctx);
}

void CoreInitOps_RegisterCmdCommands(CoreInitOps *ops) {
  if (ops == NULL)
    return;

  ops->load_commands = CoreInit_LoadCommands;
}
