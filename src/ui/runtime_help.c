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
#define GENERATED_HELP_MAX_HISTORY 4
#define GENERATED_HELP_NO_SELECTION ((size_t)-1)
#define GENERATED_HELP_MAX_ROWS 128
#define GENERATED_HELP_MAX_TEXT_LINES 128
#define GENERATED_HELP_MAX_TEXT_WIDTH 256
#define GENERATED_HELP_MAX_ITEMS 64
#define GENERATED_HELP_MAX_ITEM_LABEL 64
#define GENERATED_HELP_MAX_ITEM_DETAIL 1024
#define GENERATED_HELP_DEFAULT_WRAP_WIDTH 72
#define GENERATED_HELP_MIN_MAIN_WIDTH 8
#define GENERATED_HELP_WRAP_PADDING 4
#define GENERATED_HELP_INTRO_RESERVED_FOOTER_COMMANDS 1
#define GENERATED_HELP_STANDARD_RESERVED_FOOTER_COMMANDS 3

typedef struct {
  char label[GENERATED_HELP_MAX_ITEM_LABEL];
  char summary[GENERATED_HELP_MAX_TEXT_WIDTH];
  char detail[GENERATED_HELP_MAX_ITEM_DETAIL];
  const char *linked_topic_id;
  BOOL selectable;
} RuntimeHelpItem;

typedef struct {
  const GeneratedHelpTopic *topic;
  size_t selected_item_index;
  size_t current_detail_index;
  BOOL contextual_origin;
} RuntimeHelpView;

typedef struct {
  const GeneratedHelpTopic *topic;
  const char *next_topic_id;
  const UIHelpLabelOverride *label_overrides;
  size_t link_command_count;
  size_t active_link_index;
  size_t label_override_count;
  size_t selected_item_index;
  size_t current_detail_index;
  size_t next_detail_index;
  BOOL back_requested;
  BOOL detail_back_requested;
  BOOL has_history;
  BOOL contextual_list_mode;
  BOOL contextual_origin;
  RuntimeHelpItem items[GENERATED_HELP_MAX_ITEMS];
  UICommandStripCommand footer_commands[GENERATED_HELP_MAX_FOOTER_COMMANDS];
  char footer_keys[GENERATED_HELP_MAX_FOOTER_COMMANDS][2];
  UIHelpPopupRow rows[GENERATED_HELP_MAX_ROWS];
  char text_lines[GENERATED_HELP_MAX_TEXT_LINES][GENERATED_HELP_MAX_TEXT_WIDTH];
  size_t footer_command_count;
  size_t prefix_row_count;
  size_t row_count;
  size_t item_count;
  size_t reselection_anchor_index;
  size_t previous_visible_start;
  size_t previous_visible_end;
  BOOL viewport_valid;
  int visible_row_count;
  int visible_row_offset;
  int reselection_direction;
  int wrap_width;
} RuntimeHelpPopupState;

static const GeneratedHelpCatalog *ActiveGeneratedHelpCatalog(void) {
  const char *language = I18n_GetLanguage();

  if (generated_help_catalog_count == 0)
    return NULL;
  if (language != NULL && *language != '\0') {
    size_t i;

    for (i = 0; i < generated_help_catalog_count; ++i) {
      if (generated_help_catalogs[i].locale_id != NULL &&
          strcmp(generated_help_catalogs[i].locale_id, language) == 0) {
        return &generated_help_catalogs[i];
      }
    }
  }
  return &generated_help_catalogs[0];
}

static const GeneratedHelpTopic *ActiveGeneratedHelpTopics(size_t *topic_count) {
  const GeneratedHelpCatalog *catalog = ActiveGeneratedHelpCatalog();

  if (topic_count != NULL)
    *topic_count = catalog != NULL ? catalog->topic_count : 0;
  return catalog != NULL ? catalog->topics : NULL;
}

static size_t FindNextSelectableItem(const RuntimeHelpPopupState *state,
                                     size_t start_index,
                                     size_t end_index_exclusive) {
  size_t i;

  if (state == NULL || start_index >= end_index_exclusive)
    return GENERATED_HELP_NO_SELECTION;

  if (end_index_exclusive > state->item_count)
    end_index_exclusive = state->item_count;
  for (i = start_index; i < end_index_exclusive; ++i) {
    if (state->items[i].selectable)
      return i;
  }

  return GENERATED_HELP_NO_SELECTION;
}

static size_t FindPreviousSelectableItem(const RuntimeHelpPopupState *state,
                                         size_t start_index,
                                         size_t start_limit_inclusive) {
  size_t i;

  if (state == NULL || state->item_count == 0 || start_limit_inclusive >= state->item_count ||
      start_index >= state->item_count || start_index < start_limit_inclusive)
    return GENERATED_HELP_NO_SELECTION;

  i = start_index;
  while (1) {
    if (state->items[i].selectable)
      return i;
    if (i == start_limit_inclusive)
      break;
    i--;
  }

  return GENERATED_HELP_NO_SELECTION;
}

static BOOL GetVisibleContextItemRange(const RuntimeHelpPopupState *state,
                                       size_t *visible_start_out,
                                       size_t *visible_end_out) {
  size_t visible_start_row;
  size_t visible_end_row;
  size_t item_start_row;
  size_t item_end_row;

  if (state == NULL || visible_start_out == NULL || visible_end_out == NULL ||
      state->visible_row_count <= 0 || state->item_count == 0)
    return FALSE;

  visible_start_row = (size_t)MAXIMUM(state->visible_row_offset, 0);
  visible_end_row = visible_start_row + (size_t)state->visible_row_count;
  item_start_row = state->prefix_row_count;
  item_end_row = item_start_row + state->item_count;
  if (visible_end_row <= item_start_row || visible_start_row >= item_end_row)
    return FALSE;

  if (visible_start_row < item_start_row)
    visible_start_row = item_start_row;
  if (visible_end_row > item_end_row)
    visible_end_row = item_end_row;

  *visible_start_out = visible_start_row - item_start_row;
  *visible_end_out = visible_end_row - item_start_row;
  return (*visible_start_out < *visible_end_out);
}

static void UpdateGeneratedHelpViewport(void *user_data, int scroll_offset,
                                        int visible_rows, int row_count) {
  RuntimeHelpPopupState *state = (RuntimeHelpPopupState *)user_data;
  size_t anchor_index;
  size_t visible_start;
  size_t visible_end;
  size_t previous_visible_start;
  size_t previous_visible_end;
  size_t next_index;

  (void)row_count;
  if (state == NULL)
    return;

  state->visible_row_offset = scroll_offset;
  state->visible_row_count = visible_rows;
  if (state->reselection_direction == 0 || state->visible_row_count <= 0 ||
      state->item_count == 0)
    return;

  if (!GetVisibleContextItemRange(state, &visible_start, &visible_end)) {
    state->viewport_valid = FALSE;
    return;
  }

  previous_visible_start = state->previous_visible_start;
  previous_visible_end = state->previous_visible_end;

  anchor_index = state->reselection_anchor_index;
  if (anchor_index == GENERATED_HELP_NO_SELECTION &&
      state->selected_item_index != GENERATED_HELP_NO_SELECTION &&
      state->selected_item_index < state->item_count) {
    anchor_index = state->selected_item_index;
  }

  next_index = GENERATED_HELP_NO_SELECTION;
  if (state->reselection_direction < 0) {
    size_t search_limit = visible_end;

    if (state->viewport_valid && visible_start >= previous_visible_start)
      search_limit = visible_start;
    else if (state->viewport_valid && previous_visible_start < search_limit)
      search_limit = previous_visible_start;
    if (anchor_index != GENERATED_HELP_NO_SELECTION && anchor_index < search_limit)
      search_limit = anchor_index;
    if (search_limit > visible_start)
      next_index =
          FindPreviousSelectableItem(state, search_limit - 1, visible_start);
  } else {
    size_t search_start = visible_start;

    if (state->viewport_valid && previous_visible_end > search_start)
      search_start = previous_visible_end;
    if (anchor_index != GENERATED_HELP_NO_SELECTION &&
        anchor_index + 1 > search_start)
      search_start = anchor_index + 1;
    if (search_start < visible_end)
      next_index = FindNextSelectableItem(state, search_start, visible_end);
  }

  state->previous_visible_start = visible_start;
  state->previous_visible_end = visible_end;
  state->viewport_valid = TRUE;

  if (next_index == GENERATED_HELP_NO_SELECTION)
    return;

  state->selected_item_index = next_index;
  state->reselection_direction = 0;
  state->reselection_anchor_index = GENERATED_HELP_NO_SELECTION;
}

static const GeneratedHelpTopic *FindGeneratedTopicById(const char *topic_id) {
  const GeneratedHelpTopic *topics;
  size_t topic_count;
  size_t i;

  if (topic_id == NULL || topic_id[0] == '\0')
    return NULL;

  topics = ActiveGeneratedHelpTopics(&topic_count);
  for (i = 0; i < topic_count; ++i) {
    if (topics[i].topic_id != NULL && strcmp(topics[i].topic_id, topic_id) == 0) {
      return &topics[i];
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
  const GeneratedHelpTopic *topics;
  size_t topic_count;
  size_t i;

  if (context_id == NULL || context_id[0] == '\0')
    return NULL;

  topics = ActiveGeneratedHelpTopics(&topic_count);
  for (i = 0; i < topic_count; ++i) {
    if (ContextListContains(topics[i].contexts_csv, context_id))
      return &topics[i];
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

static BOOL TopicIdEquals(const GeneratedHelpTopic *topic, const char *topic_id) {
  return topic != NULL && topic->topic_id != NULL && topic_id != NULL &&
         strcmp(topic->topic_id, topic_id) == 0;
}

static const char *FindExplainerTopicForLabel(const GeneratedHelpTopic *topic,
                                              const char *label) {
  size_t i;

  if (topic == NULL || label == NULL || label[0] == '\0')
    return NULL;

  for (i = 0; i < topic->explainer_link_count; ++i) {
    if (topic->explainer_links[i].label != NULL &&
        strcmp(topic->explainer_links[i].label, label) == 0) {
      return topic->explainer_links[i].target_topic_id;
    }
  }

  return NULL;
}

static void TrimWhitespaceInPlace(char *text) {
  char *start;
  char *end;

  if (text == NULL || text[0] == '\0')
    return;

  start = text;
  while (*start != '\0' && isspace((unsigned char)*start))
    start++;
  if (start != text)
    memmove(text, start, strlen(start) + 1);

  end = text + strlen(text);
  while (end > text && isspace((unsigned char)end[-1]))
    --end;
  *end = '\0';
}

static void AppendHelpTextFragment(char *dest, size_t dest_size,
                                   const char *fragment) {
  size_t used;
  size_t fragment_len;

  if (dest == NULL || dest_size == 0 || fragment == NULL)
    return;

  used = strlen(dest);
  if (used >= dest_size - 1)
    return;
  fragment_len = strlen(fragment);
  if (fragment_len > dest_size - used - 1)
    fragment_len = dest_size - used - 1;
  memcpy(dest + used, fragment, fragment_len);
  dest[used + fragment_len] = '\0';
}

static void StripHelpMarkdown(const char *source, char *dest, size_t dest_size,
                              BOOL preserve_attention) {
  size_t out = 0;
  BOOL in_code = FALSE;

  if (dest == NULL || dest_size == 0)
    return;
  dest[0] = '\0';
  if (source == NULL)
    return;

  while (*source != '\0' && out + 1 < dest_size) {
    if (*source == '\\' && source[1] != '\0') {
      source++;
      dest[out++] = *source++;
      continue;
    }
    if (*source == '`') {
      in_code = !in_code;
      source++;
      continue;
    }
    if (!in_code && source[0] == '*' && source[1] == '*') {
      if (preserve_attention && out + 2 < dest_size) {
        dest[out++] = *source++;
        dest[out++] = *source++;
      } else {
        source += 2;
      }
      continue;
    }
    if (!in_code && *source == '*') {
      source++;
      continue;
    }
    dest[out++] = *source++;
  }

  while (out > 0 && isspace((unsigned char)dest[out - 1]))
    out--;
  dest[out] = '\0';
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
  state->rows[*row_count].selected = FALSE;
  (*row_count)++;
  (*line_index)++;
}

static void AppendWrappedHelpText(RuntimeHelpPopupState *state, size_t *row_count,
                                  size_t *line_index, const char *text) {
  const char *cursor = text;
  int wrap_width;

  if (state == NULL || row_count == NULL || line_index == NULL || text == NULL)
    return;

  wrap_width = state->wrap_width > 0 ? state->wrap_width
                                     : GENERATED_HELP_DEFAULT_WRAP_WIDTH;
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
    if ((int)len <= wrap_width) {
      AppendHelpText(state, row_count, line_index, segment_start);
      break;
    }

    split = (size_t)wrap_width;
    while (split > 0 && !isspace((unsigned char)segment_start[split]))
      split--;
    if (split == 0)
      split = (size_t)wrap_width;

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

static void ExtractItemLabel(const char *heading, char *label,
                             size_t label_size) {
  char stripped[GENERATED_HELP_MAX_TEXT_WIDTH];
  char *open;
  char *close;
  size_t len;

  if (label == NULL || label_size == 0) {
    return;
  }
  label[0] = '\0';
  if (heading == NULL)
    return;

  StripHelpMarkdown(heading, stripped, sizeof(stripped), FALSE);
  TrimWhitespaceInPlace(stripped);
  if (stripped[0] == '\0')
    return;

  open = strchr(stripped, '(');
  close = open != NULL ? strchr(open + 1, ')') : NULL;
  if (open != NULL && close != NULL &&
      !isdigit((unsigned char)stripped[0])) {
    char *comma;

    *close = '\0';
    open++;
    comma = strchr(open, ',');
    if (comma != NULL)
      *comma = '\0';
    TrimWhitespaceInPlace(open);
    snprintf(label, label_size, "%s", open);
    return;
  }

  len = strlen(stripped);
  if (len >= label_size)
    len = label_size - 1;
  memcpy(label, stripped, len);
  label[len] = '\0';
}

static void ExtractSummary(const char *detail, char *summary,
                           size_t summary_size) {
  const char *cursor;
  size_t len;

  if (summary == NULL || summary_size == 0) {
    return;
  }
  summary[0] = '\0';
  if (detail == NULL)
    return;

  cursor = strchr(detail, '.');
  len = cursor != NULL ? (size_t)(cursor - detail + 1) : strlen(detail);
  if (len >= summary_size)
    len = summary_size - 1;
  memcpy(summary, detail, len);
  summary[len] = '\0';
  TrimWhitespaceInPlace(summary);
}

static void FinalizeHelpItem(RuntimeHelpPopupState *state, const char *heading,
                             const char *body) {
  RuntimeHelpItem *item;
  char detail[GENERATED_HELP_MAX_ITEM_DETAIL];
  size_t detail_len;
  const char *linked_topic_id;

  if (state == NULL || heading == NULL || body == NULL ||
      state->item_count >= GENERATED_HELP_MAX_ITEMS)
    return;

  StripHelpMarkdown(body, detail, sizeof(detail), TRUE);
  TrimWhitespaceInPlace(detail);
  if (detail[0] == '\0')
    return;

  item = &state->items[state->item_count];
  memset(item, 0, sizeof(*item));
  ExtractItemLabel(heading, item->label, sizeof(item->label));
  if (item->label[0] == '\0')
    return;
  linked_topic_id = FindExplainerTopicForLabel(state->topic, item->label);
  detail_len = strlen(detail);
  if (detail_len >= sizeof(item->detail))
    detail_len = sizeof(item->detail) - 1;
  memcpy(item->detail, detail, detail_len);
  item->detail[detail_len] = '\0';
  ExtractSummary(detail, item->summary, sizeof(item->summary));
  if (item->summary[0] == '\0')
    snprintf(item->summary, sizeof(item->summary), "%s", detail);
  item->linked_topic_id = linked_topic_id;
  item->selectable =
      (item->linked_topic_id != NULL || strcmp(item->summary, item->detail) != 0);
  state->item_count++;
}

static size_t BuildExplainerLinkItems(RuntimeHelpPopupState *state) {
  size_t link_index;

  if (state == NULL || state->topic == NULL)
    return 0;

  state->item_count = 0;
  for (link_index = 0;
       link_index < state->topic->explainer_link_count &&
       state->item_count < GENERATED_HELP_MAX_ITEMS;
       ++link_index) {
    const GeneratedHelpLink *link = &state->topic->explainer_links[link_index];
    const GeneratedHelpTopic *target = NULL;
    const char *detail = NULL;

    if (link->label == NULL || link->label[0] == '\0')
      continue;
    if (link->target_topic_id != NULL && link->target_topic_id[0] != '\0')
      target = FindGeneratedTopicById(link->target_topic_id);
    if (target != NULL && target->contextual_f1 != NULL &&
        target->contextual_f1[0] != '\0') {
      detail = target->contextual_f1;
    } else if (target != NULL && target->title != NULL && target->title[0] != '\0') {
      detail = target->title;
    } else {
      detail = link->label;
    }
    FinalizeHelpItem(state, link->label, detail);
  }

  return state->item_count;
}

static size_t BuildContextItems(RuntimeHelpPopupState *state) {
  size_t section_index;

  if (state == NULL || state->topic == NULL)
    return 0;

  if (TopicIdEquals(state->topic, "intro"))
    return BuildExplainerLinkItems(state);

  state->item_count = 0;
  for (section_index = 0;
       section_index < state->topic->long_form_section_count &&
       state->item_count < GENERATED_HELP_MAX_ITEMS;
       ++section_index) {
    const GeneratedHelpLongFormSection *section =
        &state->topic->long_form_sections[section_index];
    const char *cursor = section->body;
    char heading[GENERATED_HELP_MAX_TEXT_WIDTH];
    char paragraph[GENERATED_HELP_MAX_ITEM_DETAIL];

    heading[0] = '\0';
    paragraph[0] = '\0';
    while (cursor != NULL && *cursor != '\0' &&
           state->item_count < GENERATED_HELP_MAX_ITEMS) {
      const char *line_break = strchr(cursor, '\n');
      size_t len =
          line_break != NULL ? (size_t)(line_break - cursor) : strlen(cursor);
      char line[GENERATED_HELP_MAX_ITEM_DETAIL];
      char *content = line;
      BOOL is_bullet;

      if (len >= sizeof(line))
        len = sizeof(line) - 1;
      memcpy(line, cursor, len);
      line[len] = '\0';

      while (*content != '\0' && isspace((unsigned char)*content))
        content++;

      is_bullet = ((content[0] == '*' || content[0] == '-') &&
                   isspace((unsigned char)content[1]));
      if (is_bullet) {
        if (heading[0] != '\0' && paragraph[0] != '\0')
          FinalizeHelpItem(state, heading, paragraph);
        heading[0] = '\0';
        paragraph[0] = '\0';
        content += 2;
        while (*content != '\0' && isspace((unsigned char)*content))
          content++;

        {
          char *colon = strchr(content, ':');

          if (colon != NULL) {
            size_t heading_len = (size_t)(colon - content);

            if (heading_len >= sizeof(heading))
              heading_len = sizeof(heading) - 1;
            memcpy(heading, content, heading_len);
            heading[heading_len] = '\0';
            TrimWhitespaceInPlace(heading);
            content = colon + 1;
          } else {
            snprintf(heading, sizeof(heading), "%s", content);
            content += strlen(content);
          }
        }

        while (*content != '\0' && isspace((unsigned char)*content))
          content++;
        AppendHelpTextFragment(paragraph, sizeof(paragraph), content);
      } else if (heading[0] != '\0' && content[0] != '\0') {
        AppendHelpTextFragment(paragraph, sizeof(paragraph), " ");
        AppendHelpTextFragment(paragraph, sizeof(paragraph), content);
      }

      if (line_break == NULL)
        break;
      cursor = line_break + 1;
    }

    if (heading[0] != '\0' && paragraph[0] != '\0')
      FinalizeHelpItem(state, heading, paragraph);
  }

  return state->item_count;
}

static void ApplyLabelOverrides(RuntimeHelpPopupState *state) {
  size_t item_index;
  size_t override_index;

  if (state == NULL || state->label_overrides == NULL)
    return;

  for (item_index = 0; item_index < state->item_count; ++item_index) {
    for (override_index = 0; override_index < state->label_override_count;
         ++override_index) {
      const UIHelpLabelOverride *override = &state->label_overrides[override_index];

      if (override->canonical_label == NULL || override->display_label == NULL)
        continue;
      if (strcmp(state->items[item_index].label, override->canonical_label) == 0) {
        snprintf(state->items[item_index].label,
                 sizeof(state->items[item_index].label), "%s",
                 override->display_label);
        break;
      }
    }
  }
}

static BOOL TopicUsesContextualItemList(const GeneratedHelpTopic *topic,
                                        size_t prefix_row_count) {
  if (topic == NULL || prefix_row_count != 0)
    return FALSE;

  return topic->topic_id != NULL &&
         (strcmp(topic->topic_id, "intro") == 0 ||
         (strcmp(topic->topic_id, "dir") == 0 ||
          strcmp(topic->topic_id, "file") == 0 ||
          strcmp(topic->topic_id, "archive-dir") == 0 ||
          strcmp(topic->topic_id, "archive-file") == 0 ||
          strcmp(topic->topic_id, "showall") == 0 ||
          strcmp(topic->topic_id, "global") == 0 ||
          strcmp(topic->topic_id, "f7") == 0 ||
          strcmp(topic->topic_id, "f8-dir") == 0 ||
          strcmp(topic->topic_id, "f8-file") == 0));
}

static size_t BuildFooterCommands(RuntimeHelpPopupState *state) {
  char used_keys[GENERATED_HELP_MAX_FOOTER_COMMANDS];
  size_t reserved_tail;
  size_t command_count = 0;
  size_t i;

  if (state == NULL || state->topic == NULL)
    return 0;

  if (state->contextual_list_mode) {
    state->footer_commands[command_count].layout =
        state->current_detail_index == GENERATED_HELP_NO_SELECTION
            ? UI_COMMAND_LAYOUT_ALT_MNEMONIC
            : UI_COMMAND_LAYOUT_KEY_PREFIX;
    state->footer_commands[command_count].label =
        state->current_detail_index == GENERATED_HELP_NO_SELECTION
            ? NP_("runtime-help.footer", "open")
            : NP_("runtime-help.footer", "back");
    state->footer_commands[command_count].primary_key =
        state->current_detail_index == GENERATED_HELP_NO_SELECTION ? "Enter"
                                                                   : "Left";
    state->footer_commands[command_count].secondary_key =
        state->current_detail_index == GENERATED_HELP_NO_SELECTION ? "Right"
                                                                   : NULL;
    state->footer_commands[command_count].translation_context =
        "runtime-help.footer";
    command_count++;

    if (state->current_detail_index == GENERATED_HELP_NO_SELECTION &&
        state->has_history) {
      state->footer_commands[command_count].layout = UI_COMMAND_LAYOUT_KEY_PREFIX;
      state->footer_commands[command_count].label =
          NP_("runtime-help.footer", "back");
      state->footer_commands[command_count].primary_key = "Left";
      state->footer_commands[command_count].secondary_key = NULL;
      state->footer_commands[command_count].translation_context =
          "runtime-help.footer";
      command_count++;
    }

    if (!TopicIdEquals(state->topic, "intro") ||
        state->current_detail_index != GENERATED_HELP_NO_SELECTION) {
      state->footer_commands[command_count].layout = UI_COMMAND_LAYOUT_KEY_PREFIX;
      state->footer_commands[command_count].label =
          NP_("runtime-help.footer", "Contents");
      state->footer_commands[command_count].primary_key = "C";
      state->footer_commands[command_count].secondary_key = NULL;
      state->footer_commands[command_count].translation_context =
          "runtime-help.footer";
      command_count++;
    }

    state->footer_commands[command_count].layout = UI_COMMAND_LAYOUT_KEY_PREFIX;
    state->footer_commands[command_count].label =
        NP_("runtime-help.footer", "Navigation");
    state->footer_commands[command_count].primary_key = "N";
    state->footer_commands[command_count].secondary_key = NULL;
    state->footer_commands[command_count].translation_context =
        "runtime-help.footer";
    command_count++;

    state->footer_commands[command_count].layout = UI_COMMAND_LAYOUT_ALT_MNEMONIC;
    state->footer_commands[command_count].label =
        NP_("runtime-help.footer", "Quit");
    state->footer_commands[command_count].primary_key = "Esc";
    state->footer_commands[command_count].secondary_key = "Q";
    state->footer_commands[command_count].translation_context =
        "runtime-help.footer";
    command_count++;
    state->link_command_count = 0;
    state->active_link_index = GENERATED_HELP_NO_SELECTION;
    return command_count;
  }

  if (state->contextual_origin) {
    if (state->has_history) {
      state->footer_commands[command_count].layout = UI_COMMAND_LAYOUT_KEY_PREFIX;
      state->footer_commands[command_count].label =
          NP_("runtime-help.footer", "back");
      state->footer_commands[command_count].primary_key = "Left";
      state->footer_commands[command_count].secondary_key = NULL;
      state->footer_commands[command_count].translation_context =
          "runtime-help.footer";
      command_count++;
    }

    state->footer_commands[command_count].layout = UI_COMMAND_LAYOUT_KEY_PREFIX;
    state->footer_commands[command_count].label =
        NP_("runtime-help.footer", "Contents");
    state->footer_commands[command_count].primary_key = "C";
    state->footer_commands[command_count].secondary_key = NULL;
    state->footer_commands[command_count].translation_context =
        "runtime-help.footer";
    command_count++;

    state->footer_commands[command_count].layout = UI_COMMAND_LAYOUT_KEY_PREFIX;
    state->footer_commands[command_count].label =
        NP_("runtime-help.footer", "Navigation");
    state->footer_commands[command_count].primary_key = "N";
    state->footer_commands[command_count].secondary_key = NULL;
    state->footer_commands[command_count].translation_context =
        "runtime-help.footer";
    command_count++;

    state->footer_commands[command_count].layout = UI_COMMAND_LAYOUT_ALT_MNEMONIC;
    state->footer_commands[command_count].label =
        NP_("runtime-help.footer", "Quit");
    state->footer_commands[command_count].primary_key = "Esc";
    state->footer_commands[command_count].secondary_key = "Q";
    state->footer_commands[command_count].translation_context =
        "runtime-help.footer";
    command_count++;
    state->link_command_count = 0;
    state->active_link_index = GENERATED_HELP_NO_SELECTION;
    return command_count;
  }

  reserved_tail = TopicIdEquals(state->topic, "intro")
                      ? GENERATED_HELP_INTRO_RESERVED_FOOTER_COMMANDS
                      : GENERATED_HELP_STANDARD_RESERVED_FOOTER_COMMANDS;
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
    state->footer_commands[command_count].translation_context = NULL;
    command_count++;
  }

  state->link_command_count = command_count;
  state->active_link_index =
      command_count > 0 ? 0 : GENERATED_HELP_NO_SELECTION;
  if (!TopicIdEquals(state->topic, "intro")) {
    state->footer_commands[command_count].layout = UI_COMMAND_LAYOUT_KEY_PREFIX;
    state->footer_commands[command_count].label =
        NP_("runtime-help.footer", "Back");
    state->footer_commands[command_count].primary_key = "Left";
    state->footer_commands[command_count].secondary_key = NULL;
    state->footer_commands[command_count].translation_context =
        "runtime-help.footer";
    command_count++;

    state->footer_commands[command_count].layout = UI_COMMAND_LAYOUT_KEY_PREFIX;
    state->footer_commands[command_count].label =
        NP_("runtime-help.footer", "Contents");
    state->footer_commands[command_count].primary_key = "C";
    state->footer_commands[command_count].secondary_key = NULL;
    state->footer_commands[command_count].translation_context =
        "runtime-help.footer";
    command_count++;
  }
  state->footer_commands[command_count].layout = UI_COMMAND_LAYOUT_ALT_MNEMONIC;
  state->footer_commands[command_count].label =
      NP_("runtime-help.footer", "Quit");
  state->footer_commands[command_count].primary_key = "Esc";
  state->footer_commands[command_count].secondary_key = "Q";
  state->footer_commands[command_count].translation_context =
      "runtime-help.footer";
  command_count++;

  return command_count;
}

static size_t BuildContextListRows(RuntimeHelpPopupState *state,
                                   size_t selected_item_index,
                                   const UIHelpPopupRow *prefix_rows,
                                   size_t prefix_row_count) {
  size_t row_count = 0;
  size_t index;

  for (row_count = 0; row_count < prefix_row_count &&
                      row_count < GENERATED_HELP_MAX_ROWS;
       ++row_count) {
    state->rows[row_count] = prefix_rows[row_count];
    state->rows[row_count].selected = FALSE;
  }

  for (index = 0;
       index < state->item_count && row_count < GENERATED_HELP_MAX_ROWS;
       ++index) {
    state->rows[row_count].kind = state->items[index].selectable
                                      ? UI_HELP_POPUP_LINK_TEXT
                                      : UI_HELP_POPUP_TEXT;
    state->rows[row_count].prefix = state->items[index].label;
    state->rows[row_count].text = state->items[index].summary;
    state->rows[row_count].commands = NULL;
    state->rows[row_count].command_count = 0;
    state->rows[row_count].selected = (index == selected_item_index);
    row_count++;
  }

  return row_count;
}

static size_t BuildDetailRows(RuntimeHelpPopupState *state,
                              size_t current_detail_index,
                              const UIHelpPopupRow *prefix_rows,
                              size_t prefix_row_count) {
  size_t row_count = 0;
  size_t line_index = 0;

  if (state == NULL || current_detail_index >= state->item_count)
    return 0;

  for (row_count = 0; row_count < prefix_row_count &&
                      row_count < GENERATED_HELP_MAX_ROWS;
       ++row_count) {
    state->rows[row_count] = prefix_rows[row_count];
    state->rows[row_count].selected = FALSE;
  }

  AppendWrappedHelpText(state, &row_count, &line_index,
                        state->items[current_detail_index].detail);
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

  if (state->contextual_list_mode) {
    if (state->current_detail_index != GENERATED_HELP_NO_SELECTION)
      return BuildDetailRows(state, state->current_detail_index, prefix_rows,
                             prefix_row_count);
    return BuildContextListRows(state, state->selected_item_index, prefix_rows,
                                prefix_row_count);
  }

  for (row_count = 0; row_count < prefix_row_count &&
                      row_count < GENERATED_HELP_MAX_ROWS;
       ++row_count) {
    state->rows[row_count] = prefix_rows[row_count];
    state->rows[row_count].selected = FALSE;
  }

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

static void ResetContextualHelpSelection(RuntimeHelpPopupState *state) {
  if (state == NULL)
    return;

  state->selected_item_index = GENERATED_HELP_NO_SELECTION;
  state->reselection_direction = 0;
  state->reselection_anchor_index = GENERATED_HELP_NO_SELECTION;
}

static int HandleContextualOriginFooterKey(RuntimeHelpPopupState *state,
                                           int ch) {
  int key;

  if (state == NULL)
    return 0;

  key = islower(ch) ? toupper(ch) : ch;
  if (ch == KEY_LEFT && state->has_history) {
    state->back_requested = TRUE;
    return 1;
  }
  if (key == 'C') {
    if (!TopicIdEquals(state->topic, "intro")) {
      state->next_topic_id = "intro";
      return 1;
    }
    return -1;
  }
  if (key == 'N') {
    if (!TopicIdEquals(state->topic, "navigation")) {
      state->next_topic_id = "navigation";
      return 1;
    }
    return -1;
  }
  return 0;
}

static int HandleContextualListFooterKey(RuntimeHelpPopupState *state, int ch) {
  size_t visible_start;
  size_t visible_end;
  int key;

  if (state == NULL)
    return 0;

  key = islower(ch) ? toupper(ch) : ch;

  if (state->current_detail_index != GENERATED_HELP_NO_SELECTION) {
    if (ch == KEY_LEFT) {
      state->detail_back_requested = TRUE;
      return 1;
    }
    if (key == 'C') {
      state->next_topic_id = "intro";
      return 1;
    }
    if (key == 'N') {
      state->next_topic_id = "navigation";
      return 1;
    }
    return 0;
  }

  if (ch == KEY_LEFT && state->has_history) {
    state->back_requested = TRUE;
    return 1;
  }

  if (ch == KEY_UP || ch == KEY_DOWN) {
    size_t next_index;

    if (state->item_count == 0 || state->visible_row_count <= 0)
      return 0;

    if (!GetVisibleContextItemRange(state, &visible_start, &visible_end))
      return 0;

    if (state->selected_item_index == GENERATED_HELP_NO_SELECTION) {
      if (state->reselection_direction != 0 &&
          state->reselection_anchor_index != GENERATED_HELP_NO_SELECTION) {
        if (ch == KEY_UP && state->reselection_direction > 0)
          state->reselection_direction = -1;
        else if (ch == KEY_DOWN && state->reselection_direction < 0)
          state->reselection_direction = 1;
        return 0;
      }

      if (ch == KEY_UP) {
        next_index = FindPreviousSelectableItem(
            state, visible_end > 0 ? visible_end - 1 : visible_start,
            visible_start);
      } else {
        next_index = FindNextSelectableItem(state, visible_start, visible_end);
      }

      if (next_index == GENERATED_HELP_NO_SELECTION)
        return 0;

      state->selected_item_index = next_index;
      state->reselection_direction = 0;
      state->reselection_anchor_index = GENERATED_HELP_NO_SELECTION;
      return -1;
    }

    if (state->selected_item_index < visible_start ||
        state->selected_item_index >= visible_end) {
      state->reselection_direction = (ch == KEY_UP) ? -1 : 1;
      state->reselection_anchor_index = state->selected_item_index;
      return 0;
    }

    next_index = GENERATED_HELP_NO_SELECTION;
    if (ch == KEY_UP) {
      if (state->selected_item_index > visible_start) {
        next_index = FindPreviousSelectableItem(
            state, state->selected_item_index - 1, visible_start);
      }
      if (next_index == GENERATED_HELP_NO_SELECTION &&
          state->selected_item_index > 0) {
        state->reselection_direction = -1;
        state->reselection_anchor_index = state->selected_item_index;
        return 0;
      }
    } else {
      if (state->selected_item_index + 1 < visible_end) {
        next_index = FindNextSelectableItem(state,
                                            state->selected_item_index + 1,
                                            visible_end);
      }
      if (next_index == GENERATED_HELP_NO_SELECTION &&
          state->selected_item_index + 1 < state->item_count) {
        state->reselection_direction = 1;
        state->reselection_anchor_index = state->selected_item_index;
        return 0;
      }
    }

    if (next_index == GENERATED_HELP_NO_SELECTION)
      return 0;

    state->selected_item_index = next_index;
    state->reselection_direction = 0;
    state->reselection_anchor_index = GENERATED_HELP_NO_SELECTION;
    return -1;
  }

  if (ch == KEY_HOME || ch == KEY_END || ch == KEY_PPAGE || ch == KEY_NPAGE) {
    ResetContextualHelpSelection(state);
    return 0;
  }

  if (ch == KEY_RIGHT || ch == CR || ch == LF) {
    if (state->item_count == 0 ||
        state->selected_item_index == GENERATED_HELP_NO_SELECTION ||
        state->selected_item_index >= state->item_count)
      return -1;
    if (!state->items[state->selected_item_index].selectable)
      return -1;
    if (state->items[state->selected_item_index].linked_topic_id != NULL) {
      state->next_topic_id =
          state->items[state->selected_item_index].linked_topic_id;
      state->reselection_direction = 0;
      state->reselection_anchor_index = GENERATED_HELP_NO_SELECTION;
      return 1;
    }
    state->next_detail_index = state->selected_item_index;
    state->reselection_direction = 0;
    state->reselection_anchor_index = GENERATED_HELP_NO_SELECTION;
    return 1;
  }

  if (key == 'C') {
    if (state->topic->topic_id != NULL &&
        strcmp(state->topic->topic_id, "intro") != 0) {
      state->next_topic_id = "intro";
      return 1;
    }
    return -1;
  }
  if (key == 'N') {
    if (state->topic->topic_id != NULL &&
        strcmp(state->topic->topic_id, "navigation") != 0) {
      state->next_topic_id = "navigation";
      return 1;
    }
    return -1;
  }

  return 0;
}

static int HandleGeneratedHelpFooterKey(ViewContext *ctx, int ch,
                                        void *user_data) {
  RuntimeHelpPopupState *state = (RuntimeHelpPopupState *)user_data;
  size_t i;
  int key;

  (void)ctx;
  if (state == NULL || state->topic == NULL)
    return 0;

  if (state->contextual_list_mode)
    return HandleContextualListFooterKey(state, ch);

  if (state->contextual_origin)
    return HandleContextualOriginFooterKey(state, ch);

  key = islower(ch) ? toupper(ch) : ch;
  if (ch == KEY_LEFT) {
    state->back_requested = TRUE;
    return 1;
  }
  if (key == 'C') {
    if (!TopicIdEquals(state->topic, "intro")) {
      state->next_topic_id = "intro";
      return 1;
    }
    return -1;
  }

  if (ch == KEY_RIGHT || ch == CR || ch == LF) {
    if (state->link_command_count == 0)
      return 0;

    state->next_topic_id =
        state->topic->explainer_links[state->active_link_index]
            .target_topic_id;
    return 1;
  }

  for (i = 0; i < state->link_command_count; ++i) {
    if (state->footer_commands[i].primary_key != NULL &&
        state->footer_commands[i].primary_key[0] == key) {
      state->active_link_index = i;
      state->next_topic_id = state->topic->explainer_links[i].target_topic_id;
      return 1;
    }
  }

  return 0;
}

static int GetGeneratedHelpActiveRow(const void *user_data) {
  const RuntimeHelpPopupState *state = (const RuntimeHelpPopupState *)user_data;
  size_t visible_start;
  size_t visible_end;

  if (state == NULL || !state->contextual_list_mode ||
      state->current_detail_index != GENERATED_HELP_NO_SELECTION)
    return -1;

  if (state->selected_item_index == GENERATED_HELP_NO_SELECTION ||
      state->selected_item_index >= state->item_count)
    return -1;

  if (state->visible_row_count > 0) {
    if (!GetVisibleContextItemRange(state, &visible_start, &visible_end))
      return -1;

    if (state->selected_item_index < visible_start ||
        state->selected_item_index >= visible_end)
      return -1;
  }

  return (int)(state->prefix_row_count + state->selected_item_index);
}

int UI_ShowGeneratedContextHelpWithOverrides(
    ViewContext *ctx, const char *context_id, const UIHelpPopupRow *prefix_rows,
    size_t prefix_row_count, const UIHelpLabelOverride *label_overrides,
    size_t label_override_count) {
  RuntimeHelpView history[GENERATED_HELP_MAX_HISTORY];
  size_t history_count = 0;
  RuntimeHelpView current_view;
  const GeneratedHelpTopic *topic;

  if (ctx == NULL || context_id == NULL || context_id[0] == '\0')
    return -1;

  topic = FindGeneratedTopicByContext(context_id);
  if (topic == NULL)
    return -1;

  current_view.topic = topic;
  current_view.selected_item_index = GENERATED_HELP_NO_SELECTION;
  current_view.current_detail_index = GENERATED_HELP_NO_SELECTION;
  current_view.contextual_origin =
      TopicUsesContextualItemList(topic, prefix_row_count);

  while (current_view.topic != NULL) {
    RuntimeHelpPopupState state;
    UIHelpPopupFooterSpec footer_spec;
    const GeneratedHelpTopic *next_topic;
    const char *title;

    memset(&state, 0, sizeof(state));
    state.topic = current_view.topic;
    state.label_overrides = label_overrides;
    state.label_override_count = label_override_count;
    state.selected_item_index = current_view.selected_item_index;
    state.current_detail_index = current_view.current_detail_index;
    state.contextual_origin = current_view.contextual_origin;
    state.next_detail_index = GENERATED_HELP_NO_SELECTION;
    state.has_history = history_count > 0;
    state.prefix_row_count = prefix_row_count;
    state.reselection_anchor_index = GENERATED_HELP_NO_SELECTION;
    state.previous_visible_start = 0;
    state.previous_visible_end = 0;
    state.viewport_valid = FALSE;
    state.wrap_width =
        ctx->layout.main_win_width > GENERATED_HELP_MIN_MAIN_WIDTH
            ? ctx->layout.main_win_width - GENERATED_HELP_WRAP_PADDING
            : GENERATED_HELP_DEFAULT_WRAP_WIDTH;
    state.contextual_list_mode =
        TopicUsesContextualItemList(current_view.topic, prefix_row_count);
    if (state.contextual_list_mode) {
      (void)BuildContextItems(&state);
      ApplyLabelOverrides(&state);
      if (state.current_detail_index == GENERATED_HELP_NO_SELECTION &&
          state.selected_item_index == GENERATED_HELP_NO_SELECTION &&
          state.item_count > 0) {
        state.selected_item_index =
            FindNextSelectableItem(&state, 0, state.item_count);
      }
    }
    if (state.selected_item_index >= state.item_count)
      state.selected_item_index = GENERATED_HELP_NO_SELECTION;
    if (state.current_detail_index >= state.item_count)
      state.current_detail_index = GENERATED_HELP_NO_SELECTION;

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
    footer_spec.active_row_handler = GetGeneratedHelpActiveRow;
    footer_spec.viewport_handler = UpdateGeneratedHelpViewport;
    footer_spec.key_data = &state;

    title = (state.contextual_list_mode &&
             state.current_detail_index != GENERATED_HELP_NO_SELECTION)
                ? state.items[state.current_detail_index].label
                : current_view.topic->title;
    (void)UI_ShowHelpPopupWithFooter(ctx, title, state.rows, state.row_count,
                                     &footer_spec);

    current_view.selected_item_index = state.selected_item_index;

    if (state.detail_back_requested) {
      current_view.current_detail_index = GENERATED_HELP_NO_SELECTION;
      continue;
    }

    if (state.next_detail_index != GENERATED_HELP_NO_SELECTION) {
      current_view.current_detail_index = state.next_detail_index;
      continue;
    }

    if (state.back_requested) {
      if (history_count == 0)
        break;
      current_view = history[history_count - 1];
      history_count--;
      continue;
    }

    if (state.next_topic_id == NULL)
      break;
    next_topic = FindGeneratedTopicById(state.next_topic_id);
    if (next_topic == NULL)
      break;
    if (history_count < GENERATED_HELP_MAX_HISTORY)
      history[history_count++] = current_view;
    current_view.topic = next_topic;
    current_view.selected_item_index = 0;
    current_view.current_detail_index = GENERATED_HELP_NO_SELECTION;
    current_view.contextual_origin =
        current_view.contextual_origin || state.contextual_list_mode;
  }

  return 0;
}

int UI_ShowGeneratedContextHelp(ViewContext *ctx, const char *context_id,
                                const UIHelpPopupRow *prefix_rows,
                                size_t prefix_row_count) {
  return UI_ShowGeneratedContextHelpWithOverrides(ctx, context_id, prefix_rows,
                                                  prefix_row_count, NULL, 0);
}

int UI_ShowGeneratedContextHelpCallback(ViewContext *ctx, void *help_data) {
  const char *context_id = (const char *)help_data;

  return UI_ShowGeneratedContextHelp(ctx, context_id, NULL, 0);
}
