/***************************************************************************
 *
 * src/util/string_utils.c
 * Generic string manipulation functions
 *
 ***************************************************************************/

#include "ytree_defs.h"
#include <ctype.h>
#include <string.h>

void StrCp(char *dest, const char *src) {
  static const char esc_chars[] = "#*|&;()<> \t\n\r\"!$?'`~";

  while (*src) {
    if (strchr(esc_chars, *src))
      *dest++ = '\\';
    *dest++ = *src++;
  }
  *dest = '\0';
}

int BuildFilename(char *in_filename, char *pattern, char *out_filename) {
  const char *s = in_filename;
  const char *p = pattern;
  char *d = out_filename;

  while (*p) {
    if (*p == '*') {
      char next_char = *(p + 1);
      if (next_char == '\0') {
        /* Optimization: pattern ends with *, copy rest of source */
        size_t remaining_len = strlen(s);
        memcpy(d, s, remaining_len + 1);
        return 0;
      }

      const char *stop;
      if (next_char == '.') {
        /* Special Case: Extension. Find LAST dot. */
        stop = strrchr(s, '.');
      } else {
        /* General Case: Find FIRST occurrence of next delimiter */
        stop = strchr(s, next_char);
      }

      if (stop) {
        size_t len = stop - s;
        strncpy(d, s, len);
        d += len;
        s = stop; /* Advance source to the delimiter */
      } else {
        /* Delimiter not found in source, consume remainder */
        size_t remaining_len = strlen(s);
        memcpy(d, s, remaining_len + 1);
        d += remaining_len;
        s += remaining_len;
      }
      p++; /* Consume '*' */
    } else if (*p == '?') {
      if (*s) {
        *d++ = *s++;
      } else {
        *d++ = '?'; /* Source exhausted, keep literal placeholder? */
      }
      p++;
    } else if (*p == '.') {
      /* Literal dot: synchronize with the extension separator in source */
      char *last_dot = strrchr(in_filename, '.');
      if (last_dot) {
        s = last_dot;
      }
      *d++ = *p;
      /* If source is now at the dot, consume it to stay in sync */
      if (*s == *p) {
        s++;
      }
      p++;
    } else {
      /* Literal character */
      *d++ = *p;
      /* If source matches the pattern literal, consume source to stay in sync
       */
      if (*s == *p) {
        s++;
      }
      p++;
    }
  }
  *d = '\0';

  return (0);
}

int String_Replace(char *dest, size_t dest_size, const char *src,
                   const char *token, const char *replacement) {
  const char *in_ptr = src;
  char *out_ptr = dest;
  size_t current_len = 0;
  size_t token_len;
  size_t repl_len;
  const char *next_token;

  if (!dest || !src || !token || !replacement || dest_size == 0)
    return -1;

  token_len = strlen(token);
  repl_len = strlen(replacement);

  while ((next_token = strstr(in_ptr, token)) != NULL) {
    size_t segment_len = next_token - in_ptr;

    /* Check bounds: current + segment + replacement + null terminator */
    if (current_len + segment_len + repl_len + 1 > dest_size) {
      /* Prevent overflow, ensure null termination of what fits so far */
      if (current_len < dest_size)
        dest[current_len] = '\0';
      return -1;
    }

    memcpy(out_ptr, in_ptr, segment_len);
    out_ptr += segment_len;
    current_len += segment_len;

    memcpy(out_ptr, replacement, repl_len);
    out_ptr += repl_len;
    current_len += repl_len;

    in_ptr = next_token + token_len;
  }

  /* Copy remainder */
  size_t remainder_len = strlen(in_ptr);
  if (current_len + remainder_len + 1 > dest_size) {
    if (current_len < dest_size)
      dest[current_len] = '\0';
    return -1;
  }

  memcpy(out_ptr, in_ptr, remainder_len + 1);
  return 0;
}

BOOL String_HasNonWhitespace(const char *text) {
  if (!text)
    return FALSE;
  while (*text) {
    if (!isspace((unsigned char)*text))
      return TRUE;
    text++;
  }
  return FALSE;
}

void String_GetCommandDisplayName(const char *command_template, char *command_name,
                                  size_t command_name_size) {
  const char *cursor;
  size_t idx = 0;

  if (!command_name || command_name_size == 0)
    return;

  command_name[0] = '\0';
  if (!command_template)
    return;

  cursor = command_template;
  while (*cursor && isspace((unsigned char)*cursor))
    cursor++;
  if (*cursor == '\0')
    return;

  if (*cursor == '"' || *cursor == '\'') {
    char quote = *cursor++;
    while (*cursor && *cursor != quote && idx + 1 < command_name_size) {
      command_name[idx++] = *cursor++;
    }
  } else {
    while (*cursor && !isspace((unsigned char)*cursor) &&
           idx + 1 < command_name_size) {
      command_name[idx++] = *cursor++;
    }
  }
  command_name[idx] = '\0';
}
