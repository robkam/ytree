#include "fuzz_common.h"

#include "ytnova_defs.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  FuzzCursor cursor;
  char input_name[PATH_LENGTH + 1];
  char pattern[PATH_LENGTH + 1];
  char built_name[PATH_LENGTH + 1];
  char replace_src[PATH_LENGTH + 1];
  char replace_token[64];
  char replace_value[64];
  char replace_out[(PATH_LENGTH * 2) + 1];
  char command_template[PATH_LENGTH + 1];
  char command_name[128];
  char duration[32];
  long long raw_seconds;
  int seconds;

  FuzzCursor_Init(&cursor, data, size);

  FuzzCursor_ReadAsciiString(&cursor, input_name, sizeof(input_name), PATH_LENGTH);
  FuzzCursor_ReadAsciiString(&cursor, pattern, sizeof(pattern), PATH_LENGTH);
  FuzzCursor_ReadAsciiString(&cursor, replace_src, sizeof(replace_src), PATH_LENGTH);
  FuzzCursor_ReadAsciiString(&cursor, replace_token, sizeof(replace_token), 63);
  FuzzCursor_ReadAsciiString(&cursor, replace_value, sizeof(replace_value), 63);
  FuzzCursor_ReadAsciiString(&cursor, command_template, sizeof(command_template),
                             PATH_LENGTH);
  raw_seconds = FuzzCursor_ReadSigned64(&cursor);
  if (raw_seconds > INT_MAX)
    seconds = INT_MAX;
  else if (raw_seconds < INT_MIN)
    seconds = INT_MIN;
  else
    seconds = (int)raw_seconds;

  if (replace_token[0] == '\0')
    (void)snprintf(replace_token, sizeof(replace_token), "x");

  (void)BuildFilename(input_name, pattern, built_name);
  (void)String_Replace(replace_out, sizeof(replace_out), replace_src, replace_token,
                       replace_value);
  (void)String_HasNonWhitespace(replace_src);
  String_GetCommandDisplayName(command_template, command_name,
                               sizeof(command_name));
  String_FormatCompactDuration(seconds, duration, sizeof(duration));

  if (built_name[0] == '\0' && command_name[0] == '\0' && duration[0] == '\0')
    return 0;

  return 0;
}
