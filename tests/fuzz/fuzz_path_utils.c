#include "fuzz_common.h"

#include "ytree_fs.h"

#include <stdarg.h>
#include <stdio.h>

int UI_Message(ViewContext *ctx, const char *fmt, ...) {
  va_list args;
  (void)ctx;
  (void)fmt;
  va_start(args, fmt);
  va_end(args);
  return 0;
}

int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  FuzzCursor cursor;
  char path_a[PATH_LENGTH + 1];
  char path_b[PATH_LENGTH + 1];
  char append_path[PATH_LENGTH + 1];
  char cmd_template[PATH_LENGTH + 1];
  char compare_template[PATH_LENGTH + 1];
  char command_line[(PATH_LENGTH * 6) + 1];
  char quoted[(PATH_LENGTH * 4) + 3];
  char joined[PATH_LENGTH + 1];
  char literal[64];
  size_t command_len = 0;
  int use_token1;
  int use_token2;

  FuzzCursor_Init(&cursor, data, size);
  FuzzCursor_ReadAsciiString(&cursor, path_a, sizeof(path_a), PATH_LENGTH);
  FuzzCursor_ReadAsciiString(&cursor, path_b, sizeof(path_b), PATH_LENGTH);
  FuzzCursor_ReadAsciiString(&cursor, append_path, sizeof(append_path), PATH_LENGTH);
  FuzzCursor_ReadAsciiString(&cursor, cmd_template, sizeof(cmd_template), PATH_LENGTH);
  FuzzCursor_ReadAsciiString(&cursor, compare_template, sizeof(compare_template),
                             PATH_LENGTH);
  FuzzCursor_ReadAsciiString(&cursor, literal, sizeof(literal), 63);

  if (cmd_template[0] == '\0')
    (void)snprintf(cmd_template, sizeof(cmd_template), "echo %%1 %%2");
  if (compare_template[0] == '\0')
    (void)snprintf(compare_template, sizeof(compare_template), "cmp %%1 %%2");

  use_token1 = FuzzCursor_NextBool(&cursor);
  use_token2 = FuzzCursor_NextBool(&cursor);

  (void)Path_ShellQuote(path_a, quoted, sizeof(quoted));

  (void)Path_BuildCommandLine(cmd_template, append_path,
                              use_token1 ? "%1" : NULL, use_token1 ? path_a : NULL,
                              use_token2 ? "%2" : NULL, use_token2 ? path_b : NULL,
                              command_line, sizeof(command_line));

  (void)Path_BuildCompareCommandLine(compare_template, path_a, path_b, command_line,
                                     sizeof(command_line));

  (void)Path_Join(joined, sizeof(joined), path_a, path_b);

  if (Path_CommandInit(command_line, sizeof(command_line), &command_len, "sh -c ") ==
      TRUE) {
    (void)Path_CommandAppendLiteral(command_line, sizeof(command_line), &command_len,
                                    literal);
    (void)Path_CommandAppendQuotedArg(command_line, sizeof(command_line), &command_len,
                                      append_path);
  }

  return 0;
}
