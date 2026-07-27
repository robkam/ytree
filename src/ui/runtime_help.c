/***************************************************************************
 *
 * src/ui/runtime_help.c
 * Generated runtime help topic lookup and popup wiring.
 *
 ***************************************************************************/

#include "../../include/ytnova_ui.h"
#include "../core/generated_help_topics.h"
#include <ctype.h>
#include <string.h>

#define GENERATED_HELP_MAX_FOOTER_COMMANDS 10
#define GENERATED_HELP_MAX_HISTORY 2
#define GENERATED_HELP_NO_SELECTION ((size_t)-1)
#define GENERATED_HELP_MAX_ROWS 128
#define GENERATED_HELP_MAX_TEXT_LINES 128
#define GENERATED_HELP_MAX_TEXT_WIDTH 256
#define GENERATED_HELP_WRAP_WIDTH 72

typedef struct {
  const GeneratedHelpTopic *topic;
  size_t history_count;
  const char *next_topic_id;
  size_t link_command_count;
  size_t active_link_index;
  BOOL back_requested;
  UICommandStripCommand footer_commands[GENERATED_HELP_MAX_FOOTER_COMMANDS];
  char footer_keys[GENERATED_HELP_MAX_FOOTER_COMMANDS][2];
  UIHelpPopupRow rows[GENERATED_HELP_MAX_ROWS];
  char text_lines[GENERATED_HELP_MAX_TEXT_LINES][GENERATED_HELP_MAX_TEXT_WIDTH];
  size_t footer_command_count;
  size_t row_count;
} RuntimeHelpPopupState;

static const GeneratedHelpTopic *FindGeneratedTopicById(const char *topic_id) {
  size_t i;

  if (topic_id == NULL || topic_id[0] == '\0')
    return NULL;

  for (i = 0; i < generated_help_topic_count; ++i) {
    if (generated_help_topics[i].topic_id != NULL &&
        strcmp(generated_help_topics[i].topic_id, topic_id) == 0) {
      return &generated_help_topics[i];
    }
  }

  return NULL;
}

static BOOL ContextListContains(const char *contexts_csv,
                                const char *context_id) {
  const char *cursor;
  size_t context_len;

  if (contexts_csv == NULL || context_id == NULL || context_id[0] == '\0')
    return FALSE;

  context_len = strlen(context_id);
  cursor = contexts_csv;
  while (*cursor != '\0') {
    const char *comma = strchr(cursor, ',');
    size_t len = comma != NULL ? (size_t)(comma - cursor) : strlen(cursor);

    if (len == context_len && strncmp(cursor, context_id, len) == 0)
      return TRUE;
    if (comma == NULL)
      break;
    cursor = comma + 1;
  }

  return FALSE;
}

static const GeneratedHelpTopic *FindGeneratedTopicByContext(
    const char *context_id) {
  size_t i;

  if (context_id == NULL || context_id[0] == '\0')
    return NULL;

  for (i = 0; i < generated_help_topic_count; ++i) {
    if (ContextListContains(generated_help_topics[i].contexts_csv, context_id))
      return &generated_help_topics[i];
  }

  return NULL;
}

static BOOL FooterKeyUsed(const char used_keys[], size_t used_count, char key) {
  size_t i;

  for (i = 0; i < used_count; ++i) {
    if (used_keys[i] == key)
      return TRUE;
  }

  return FALSE;
}

static char PickFooterKey(const char *label, const char used_keys[],
                          size_t used_count) {
  const char *cursor;
  char key;

  if (label == NULL)
    return '\0';

  if (label[0] == 'F' && isdigit((unsigned char)label[1]) &&
      !isdigit((unsigned char)label[2])) {
    key = label[1];
    if (!FooterKeyUsed(used_keys, used_count, key))
      return key;
  }

  for (cursor = label; *cursor != '\0'; ++cursor) {
    if (!isalnum((unsigned char)*cursor))
      continue;
    key = (char)toupper((unsigned char)*cursor);
    if (!FooterKeyUsed(used_keys, used_count, key))
      return key;
  }

  return (char)toupper((unsigned char)label[0]);
}

static size_t BuildFooterCommands(RuntimeHelpPopupState *state) {
  char used_keys[GENERATED_HELP_MAX_FOOTER_COMMANDS];
  size_t reserved_tail;
  size_t command_count = 0;
  size_t i;

  if (state == NULL || state->topic == NULL)
    return 0;

  reserved_tail = state->history_count > 0 ? 2 : 1;
  memset(used_keys, 0, sizeof(used_keys));
  for (i = 0; i < state->topic->explainer_link_count &&
              command_count + reserved_tail < GENERATED_HELP_MAX_FOOTER_COMMANDS;
       ++i) {
    char key = PickFooterKey(state->topic->explainer_links[i].label, used_keys,
                             command_count);

    used_keys[command_count] = key;
    state->footer_keys[command_count][0] = key;
    state->footer_keys[command_count][1] = '\0';
    state->footer_commands[command_count].layout = UI_COMMAND_LAYOUT_KEY_PREFIX;
    state->footer_commands[command_count].label =
        state->topic->explainer_links[i].label;
    state->footer_commands[command_count].primary_key =
        state->footer_keys[command_count];
    state->footer_commands[command_count].secondary_key = NULL;
    command_count++;
  }

  state->link_command_count = command_count;
  state->active_link_index =
      command_count > 0 ? 0 : GENERATED_HELP_NO_SELECTION;

  if (state->history_count > 0) {
    state->footer_commands[command_count].layout = UI_COMMAND_LAYOUT_KEY_PREFIX;
    state->footer_commands[command_count].label = "back";
    state->footer_commands[command_count].primary_key = "Left";
    state->footer_commands[command_count].secondary_key = NULL;
    command_count++;
  }

  state->footer_commands[command_count].layout = UI_COMMAND_LAYOUT_KEY_PREFIX;
  state->footer_commands[command_count].label = "close";
  state->footer_commands[command_count].primary_key = "Esc";
  state->footer_commands[command_count].secondary_key = NULL;
  command_count++;

  return command_count;
}

static void AppendHelpText(RuntimeHelpPopupState *state, size_t *row_count,
                           size_t *line_index, const char *text) {
  size_t len;

  if (state == NULL || row_count == NULL || line_index == NULL || text == NULL ||
      *row_count >= GENERATED_HELP_MAX_ROWS ||
      *line_index >= GENERATED_HELP_MAX_TEXT_LINES)
    return;

  len = strlen(text);
  if (len >= GENERATED_HELP_MAX_TEXT_WIDTH)
    len = GENERATED_HELP_MAX_TEXT_WIDTH - 1;

  memcpy(state->text_lines[*line_index], text, len);
  state->text_lines[*line_index][len] = '\0';
  state->rows[*row_count].kind = UI_HELP_POPUP_TEXT;
  state->rows[*row_count].prefix = NULL;
  state->rows[*row_count].text = state->text_lines[*line_index];
  state->rows[*row_count].commands = NULL;
  state->rows[*row_count].command_count = 0;
  (*row_count)++;
  (*line_index)++;
}

static void AppendHelpTextFragment(char *dest, size_t dest_size,
                                   const char *fragment) {
  size_t used;
  size_t remaining;

  if (dest == NULL || dest_size == 0 || fragment == NULL)
    return;

  used = strlen(dest);
  if (used >= dest_size - 1)
    return;
  remaining = dest_size - used - 1;
  strncat(dest, fragment, remaining);
}

static void StripHelpMarkdown(const char *source, char *dest, size_t dest_size) {
  size_t out = 0;

  if (dest == NULL || dest_size == 0)
    return;
  dest[0] = '\0';
  if (source == NULL)
    return;

  while (*source != '\0' && out + 1 < dest_size) {
    if (source[0] == '*' && source[1] == '*') {
      source += 2;
      continue;
    }
    if (*source == '*' || *source == '`') {
      source++;
      continue;
    }
    if (*source == '\\' && source[1] != '\0' &&
        (source[1] == '*' || source[1] == '`')) {
      source++;
    }
    dest[out++] = *source++;
  }

  while (out > 0 && isspace((unsigned char)dest[out - 1]))
    out--;
  dest[out] = '\0';
}

static void AppendWrappedHelpText(RuntimeHelpPopupState *state, size_t *row_count,
                                  size_t *line_index, const char *text) {
  const char *cursor = text;

  if (state == NULL || row_count == NULL || line_index == NULL || text == NULL)
    return;

  while (*cursor != '\0' && *row_count < GENERATED_HELP_MAX_ROWS &&
         *line_index < GENERATED_HELP_MAX_TEXT_LINES) {
    char wrapped[GENERATED_HELP_MAX_TEXT_WIDTH];
    const char *segment_start;
    size_t len;
    size_t split;

    while (*cursor != '\0' && isspace((unsigned char)*cursor))
      cursor++;
    if (*cursor == '\0')
      break;

    segment_start = cursor;
    len = strlen(segment_start);
    if (len <= GENERATED_HELP_WRAP_WIDTH) {
      AppendHelpText(state, row_count, line_index, segment_start);
      break;
    }

    split = GENERATED_HELP_WRAP_WIDTH;
    while (split > 0 && !isspace((unsigned char)segment_start[split]))
      split--;
    if (split == 0)
      split = GENERATED_HELP_WRAP_WIDTH;

    while (split > 0 && isspace((unsigned char)segment_start[split - 1]))
      split--;
    if (split >= sizeof(wrapped))
      split = sizeof(wrapped) - 1;

    memcpy(wrapped, segment_start, split);
    wrapped[split] = '\0';
    AppendHelpText(state, row_count, line_index, wrapped);
    cursor = segment_start + split;
  }
}

static void FlushSectionParagraph(RuntimeHelpPopupState *state, size_t *row_count,
                                  size_t *line_index, char *paragraph) {
  if (paragraph == NULL || paragraph[0] == '\0')
    return;

  AppendWrappedHelpText(state, row_count, line_index, paragraph);
  paragraph[0] = '\0';
}

static size_t BuildLongFormRows(RuntimeHelpPopupState *state, size_t row_count,
                                size_t *line_index) {
  size_t section_index;

  if (state == NULL || state->topic == NULL || line_index == NULL)
    return row_count;

  for (section_index = 0;
       section_index < state->topic->long_form_section_count &&
       row_count < GENERATED_HELP_MAX_ROWS &&
       *line_index < GENERATED_HELP_MAX_TEXT_LINES;
       ++section_index) {
    const GeneratedHelpLongFormSection *section =
        &state->topic->long_form_sections[section_index];
    char line[GENERATED_HELP_MAX_TEXT_WIDTH];
    char paragraph[GENERATED_HELP_MAX_TEXT_WIDTH * 4];
    char stripped[GENERATED_HELP_MAX_TEXT_WIDTH];
    const char *cursor;

    paragraph[0] = '\0';
    if (section_index > 0)
      AppendHelpText(state, &row_count, line_index, "");
    AppendHelpText(state, &row_count, line_index, section->title);

    cursor = section->body;
    while (cursor != NULL && *cursor != '\0' &&
           row_count < GENERATED_HELP_MAX_ROWS &&
           *line_index < GENERATED_HELP_MAX_TEXT_LINES) {
      const char *line_break = strchr(cursor, '\n');
      size_t len =
          line_break != NULL ? (size_t)(line_break - cursor) : strlen(cursor);
      const char *content = cursor;
      BOOL continuation = (BOOL)(isspace((unsigned char)cursor[0]) != 0);

      if (len >= sizeof(line))
        len = sizeof(line) - 1;
      memcpy(line, cursor, len);
      line[len] = '\0';

      while (*content != '\0' && isspace((unsigned char)*content))
        content++;

      if (*content == '\0') {
        FlushSectionParagraph(state, &row_count, line_index, paragraph);
      } else if (content[0] == '*' && isspace((unsigned char)content[1])) {
        FlushSectionParagraph(state, &row_count, line_index, paragraph);
        while (*content != '\0' && !isspace((unsigned char)*content))
          content++;
        while (*content != '\0' && isspace((unsigned char)*content))
          content++;
        StripHelpMarkdown(content, stripped, sizeof(stripped));
        AppendHelpTextFragment(paragraph, sizeof(paragraph), stripped);
      } else if (content[0] == '-' && isspace((unsigned char)content[1])) {
        FlushSectionParagraph(state, &row_count, line_index, paragraph);
        while (*content != '\0' && !isspace((unsigned char)*content))
          content++;
        while (*content != '\0' && isspace((unsigned char)*content))
          content++;
        StripHelpMarkdown(content, stripped, sizeof(stripped));
        AppendHelpTextFragment(paragraph, sizeof(paragraph), stripped);
      } else {
        StripHelpMarkdown(content, stripped, sizeof(stripped));
        if (!continuation)
          FlushSectionParagraph(state, &row_count, line_index, paragraph);
        if (paragraph[0] != '\0')
          AppendHelpTextFragment(paragraph, sizeof(paragraph), " ");
        AppendHelpTextFragment(paragraph, sizeof(paragraph), stripped);
      }

      if (line_break == NULL)
        break;
      cursor = line_break + 1;
    }

    FlushSectionParagraph(state, &row_count, line_index, paragraph);
  }

  return row_count;
}

static size_t BuildTextRows(RuntimeHelpPopupState *state,
                            const UIHelpPopupRow *prefix_rows,
                            size_t prefix_row_count) {
  size_t row_count = 0;
  size_t line_index = 0;
  const char *cursor;

  if (state == NULL || state->topic == NULL)
    return 0;

  for (row_count = 0; row_count < prefix_row_count &&
                      row_count < GENERATED_HELP_MAX_ROWS;
       ++row_count) {
    state->rows[row_count] = prefix_rows[row_count];
  }

  if (state->topic->topic_id != NULL &&
      strcmp(state->topic->topic_id, "dir") == 0 &&
      state->topic->long_form_section_count > 0)
    return BuildLongFormRows(state, row_count, &line_index);

  cursor = state->topic->contextual_f1;
  while (cursor != NULL && *cursor != '\0' &&
         row_count < GENERATED_HELP_MAX_ROWS &&
         line_index < GENERATED_HELP_MAX_TEXT_LINES) {
    const char *line_break = strchr(cursor, '\n');
    size_t len =
        line_break != NULL ? (size_t)(line_break - cursor) : strlen(cursor);

    if (len > 0) {
      char line[GENERATED_HELP_MAX_TEXT_WIDTH];

      if (len >= sizeof(line))
        len = sizeof(line) - 1;
      memcpy(line, cursor, len);
      line[len] = '\0';
      AppendHelpText(state, &row_count, &line_index, line);
    } else {
      AppendHelpText(state, &row_count, &line_index, "");
    }

    if (line_break == NULL)
      break;
    cursor = line_break + 1;
  }

  return row_count;
}

static int HandleGeneratedHelpFooterKey(ViewContext *ctx, int ch,
                                        void *user_data) {
  RuntimeHelpPopupState *state = (RuntimeHelpPopupState *)user_data;
  size_t i;
  int key;

  (void)ctx;
  if (state == NULL || state->topic == NULL)
    return 0;

  if (ch == KEY_LEFT) {
    if (state->history_count > 0) {
      state->back_requested = TRUE;
      return 1;
    }
    return 0;
  }

  if (ch == KEY_RIGHT || ch == CR || ch == LF) {
    if (state->link_command_count == 0)
      return 0;
    if (state->history_count >= GENERATED_HELP_MAX_HISTORY)
      return -1;

    state->next_topic_id =
        state->topic->explainer_links[state->active_link_index]
            .target_topic_id;
    return 1;
  }

  key = islower(ch) ? toupper(ch) : ch;
  for (i = 0; i < state->link_command_count; ++i) {
    if (state->footer_commands[i].primary_key != NULL &&
        state->footer_commands[i].primary_key[0] == key) {
      if (state->history_count >= GENERATED_HELP_MAX_HISTORY)
        return -1;
      state->active_link_index = i;
      state->next_topic_id = state->topic->explainer_links[i].target_topic_id;
      return 1;
    }
  }

  return 0;
}

int UI_ShowGeneratedContextHelp(ViewContext *ctx, const char *context_id,
                                const UIHelpPopupRow *prefix_rows,
                                size_t prefix_row_count) {
  const GeneratedHelpTopic *history[GENERATED_HELP_MAX_HISTORY];
  size_t history_count = 0;
  const GeneratedHelpTopic *topic;

  if (ctx == NULL || context_id == NULL || context_id[0] == '\0')
    return -1;

  topic = FindGeneratedTopicByContext(context_id);
  if (topic == NULL)
    return -1;

  while (topic != NULL) {
    RuntimeHelpPopupState state;
    UIHelpPopupFooterSpec footer_spec;
    const GeneratedHelpTopic *next_topic;

    memset(&state, 0, sizeof(state));
    state.topic = topic;
    state.history_count = history_count;
    state.footer_command_count = BuildFooterCommands(&state);
    state.row_count = BuildTextRows(&state, prefix_rows, prefix_row_count);
    if (state.row_count == 0)
      return -1;

    memset(&footer_spec, 0, sizeof(footer_spec));
    footer_spec.commands = state.footer_commands;
    footer_spec.command_count = state.footer_command_count;
    footer_spec.link_command_count = state.link_command_count;
    footer_spec.active_command_index = state.active_link_index;
    footer_spec.key_handler = HandleGeneratedHelpFooterKey;
    footer_spec.key_data = &state;

    (void)UI_ShowHelpPopupWithFooter(ctx, topic->title, state.rows,
                                     state.row_count, &footer_spec);

    if (state.back_requested) {
      if (history_count == 0)
        break;
      topic = history[history_count - 1];
      history_count--;
      continue;
    }

    if (state.next_topic_id == NULL)
      break;
    next_topic = FindGeneratedTopicById(state.next_topic_id);
    if (next_topic == NULL)
      break;
    if (history_count < GENERATED_HELP_MAX_HISTORY)
      history[history_count++] = topic;
    topic = next_topic;
  }

  return 0;
}

int UI_ShowGeneratedContextHelpCallback(ViewContext *ctx, void *help_data) {
  const char *context_id = (const char *)help_data;

  return UI_ShowGeneratedContextHelp(ctx, context_id, NULL, 0);
}
