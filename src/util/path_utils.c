/***************************************************************************
 *
 * src/util/path_utils.c
 * Path construction and manipulation functions
 *
 ***************************************************************************/

#include "ytree_defs.h"
#include <libgen.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static int AppendBounded(char *dst, size_t dst_size, const char *src) {
  size_t used;
  int written;

  if (!dst || dst_size == 0 || !src) {
    return -1;
  }

  used = strlen(dst);
  if (used >= dst_size) {
    dst[dst_size - 1] = '\0';
    return -1;
  }

  written = snprintf(dst + used, dst_size - used, "%s", src);
  if (written < 0 || (size_t)written >= (dst_size - used)) {
    dst[dst_size - 1] = '\0';
    return -1;
  }
  return 0;
}

static BOOL AppendBoundedSpan(char *dst, size_t dst_size, size_t *len,
                              const char *src, size_t src_len) {
  size_t current_len;

  if (!dst || dst_size == 0 || !len || !src)
    return FALSE;

  current_len = *len;
  if (current_len >= dst_size) {
    dst[dst_size - 1] = '\0';
    return FALSE;
  }
  if (src_len >= (dst_size - current_len)) {
    dst[dst_size - 1] = '\0';
    return FALSE;
  }

  memcpy(dst + current_len, src, src_len);
  current_len += src_len;
  dst[current_len] = '\0';
  *len = current_len;
  return TRUE;
}

BOOL Path_CommandInit(char *command_line, size_t command_size,
                      size_t *command_len, const char *prefix) {
  if (!command_line || command_size == 0 || !command_len)
    return FALSE;

  command_line[0] = '\0';
  *command_len = 0;
  if (!prefix)
    return TRUE;

  return AppendBoundedSpan(command_line, command_size, command_len, prefix,
                           strlen(prefix));
}

BOOL Path_CommandAppendLiteral(char *command_line, size_t command_size,
                               size_t *command_len, const char *literal) {
  if (!command_line || command_size == 0 || !command_len)
    return FALSE;
  if (!literal)
    literal = "";

  return AppendBoundedSpan(command_line, command_size, command_len, literal,
                           strlen(literal));
}

BOOL Path_CommandAppendQuotedArg(char *command_line, size_t command_size,
                                 size_t *command_len, const char *arg) {
  static const char escaped_single_quote[] = "'\\''";
  const char *cptr;

  if (!command_line || command_size == 0 || !command_len)
    return FALSE;
  if (!arg)
    arg = "";

  if (!AppendBoundedSpan(command_line, command_size, command_len, "'", 1))
    return FALSE;

  for (cptr = arg; *cptr; cptr++) {
    if (*cptr == '\'') {
      if (!AppendBoundedSpan(command_line, command_size, command_len,
                             escaped_single_quote,
                             sizeof(escaped_single_quote) - 1)) {
        return FALSE;
      }
    } else {
      if (!AppendBoundedSpan(command_line, command_size, command_len, cptr, 1))
        return FALSE;
    }
  }

  return AppendBoundedSpan(command_line, command_size, command_len, "'", 1);
}

BOOL Path_BuildUserActionCommand(const char *command_template, const char *path,
                                 char *command_line, size_t command_size) {
  const char *cptr;
  size_t command_len;
  BOOL saw_placeholder = FALSE;

  if (!command_template || !path || !command_line || command_size == 0)
    return FALSE;

  if (!Path_CommandInit(command_line, command_size, &command_len, ""))
    return FALSE;

  cptr = command_template;
  while (*cptr) {
    if (cptr[0] == '%' && cptr[1] == 's') {
      if (!Path_CommandAppendQuotedArg(command_line, command_size, &command_len,
                                       path)) {
        return FALSE;
      }
      cptr += 2;
      saw_placeholder = TRUE;
      continue;
    }
    if (cptr[0] == '%' && cptr[1] == '%') {
      if (!Path_CommandAppendLiteral(command_line, command_size, &command_len,
                                     "%")) {
        return FALSE;
      }
      cptr += 2;
      continue;
    }
    if (!AppendBoundedSpan(command_line, command_size, &command_len, cptr, 1))
      return FALSE;
    cptr++;
  }

  if (!saw_placeholder) {
    if (!Path_CommandAppendLiteral(command_line, command_size, &command_len,
                                   " ")) {
      return FALSE;
    }
    if (!Path_CommandAppendQuotedArg(command_line, command_size, &command_len,
                                     path)) {
      return FALSE;
    }
  }

  return TRUE;
}

const char *Path_LeafName(const char *path) {
  const char *sep;

  if (!path || !*path)
    return "";

  sep = strrchr(path, FILE_SEPARATOR_CHAR);
  if (!sep || !sep[1])
    return path;
  return sep + 1;
}

BOOL Path_ShellQuote(const char *src, char *dst, size_t dst_size) {
  size_t out = 0;

  if (!src || !dst || dst_size < 3)
    return FALSE;

  dst[out++] = '\'';
  while (*src) {
    if (*src == '\'') {
      if (out + 4 >= dst_size)
        return FALSE;
      dst[out++] = '\'';
      dst[out++] = '\\';
      dst[out++] = '\'';
      dst[out++] = '\'';
    } else {
      if (out + 1 >= dst_size)
        return FALSE;
      dst[out++] = *src;
    }
    src++;
  }

  if (out + 2 > dst_size)
    return FALSE;
  dst[out++] = '\'';
  dst[out] = '\0';
  return TRUE;
}

int Path_BuildCommandLine(const char *command_template, const char *append_path,
                          const char *token1, const char *value1,
                          const char *token2, const char *value2,
                          char *command_line, size_t command_line_size) {
  char quoted_append[(PATH_LENGTH * 4) + 3];
  char quoted_value1[(PATH_LENGTH * 4) + 3];
  char quoted_value2[(PATH_LENGTH * 4) + 3];
  const char *template_ptr;
  size_t token1_len;
  size_t token2_len;

  if (!command_template || !command_line || command_line_size == 0) {
    return -1;
  }

  if ((token1 && !value1) || (!token1 && value1) || (token2 && !value2) ||
      (!token2 && value2)) {
    return -1;
  }

  if (append_path &&
      !Path_ShellQuote(append_path, quoted_append, sizeof(quoted_append))) {
    return -1;
  }
  if (token1 &&
      !Path_ShellQuote(value1, quoted_value1, sizeof(quoted_value1))) {
    return -1;
  }
  if (token2 &&
      !Path_ShellQuote(value2, quoted_value2, sizeof(quoted_value2))) {
    return -1;
  }

  token1_len = token1 ? strlen(token1) : 0;
  token2_len = token2 ? strlen(token2) : 0;
  command_line[0] = '\0';

  for (template_ptr = command_template; *template_ptr; template_ptr++) {
    if (token1 && token1_len > 0 &&
        strncmp(template_ptr, token1, token1_len) == 0) {
      if (AppendBounded(command_line, command_line_size, quoted_value1) != 0)
        return -1;
      template_ptr += token1_len - 1;
      continue;
    }

    if (token2 && token2_len > 0 &&
        strncmp(template_ptr, token2, token2_len) == 0) {
      if (AppendBounded(command_line, command_line_size, quoted_value2) != 0)
        return -1;
      template_ptr += token2_len - 1;
      continue;
    }

    {
      char single_char[2];
      single_char[0] = *template_ptr;
      single_char[1] = '\0';
      if (AppendBounded(command_line, command_line_size, single_char) != 0)
        return -1;
    }
  }

  if (append_path) {
    if (*command_line &&
        AppendBounded(command_line, command_line_size, " ") != 0) {
      return -1;
    }
    if (AppendBounded(command_line, command_line_size, quoted_append) != 0) {
      return -1;
    }
  }

  return 0;
}

int Path_BuildCompareCommandLine(const char *command_template,
                                 const char *source_path,
                                 const char *target_path, char *command_line,
                                 size_t command_line_size) {
  char quoted_source[(PATH_LENGTH * 4) + 3];
  char quoted_target[(PATH_LENGTH * 4) + 3];
  const char *template_ptr;
  BOOL has_placeholders;

  if (!command_template || !source_path || !target_path || !command_line ||
      command_line_size == 0) {
    return -1;
  }

  if (!Path_ShellQuote(source_path, quoted_source, sizeof(quoted_source)) ||
      !Path_ShellQuote(target_path, quoted_target, sizeof(quoted_target))) {
    return -1;
  }

  has_placeholders = (strstr(command_template, "%1") != NULL ||
                      strstr(command_template, "%2") != NULL);

  command_line[0] = '\0';
  if (!has_placeholders) {
    if (AppendBounded(command_line, command_line_size, command_template) != 0 ||
        AppendBounded(command_line, command_line_size, " ") != 0 ||
        AppendBounded(command_line, command_line_size, quoted_source) != 0 ||
        AppendBounded(command_line, command_line_size, " ") != 0 ||
        AppendBounded(command_line, command_line_size, quoted_target) != 0) {
      return -1;
    }
    return 0;
  }

  for (template_ptr = command_template; *template_ptr; template_ptr++) {
    if (*template_ptr == '%' && template_ptr[1] == '1') {
      if (AppendBounded(command_line, command_line_size, quoted_source) != 0)
        return -1;
      template_ptr++;
      continue;
    }

    if (*template_ptr == '%' && template_ptr[1] == '2') {
      if (AppendBounded(command_line, command_line_size, quoted_target) != 0)
        return -1;
      template_ptr++;
      continue;
    }

    {
      char single_char[2];
      single_char[0] = *template_ptr;
      single_char[1] = '\0';
      if (AppendBounded(command_line, command_line_size, single_char) != 0)
        return -1;
    }
  }

  return 0;
}

int Path_Join(char *dest, size_t size, const char *dir, const char *leaf) {
  size_t dir_len;
  size_t trimmed_dir_len;
  const char *leaf_part;
  int is_root_dir;

  if (!dest || size == 0 || !dir || !leaf) {
    return -1;
  }

  dest[0] = '\0';

  dir_len = strlen(dir);
  trimmed_dir_len = dir_len;
  while (trimmed_dir_len > 1 &&
         dir[trimmed_dir_len - 1] == FILE_SEPARATOR_CHAR) {
    trimmed_dir_len--;
  }

  leaf_part = leaf;
  while (*leaf_part == FILE_SEPARATOR_CHAR) {
    leaf_part++;
  }

  is_root_dir =
      (trimmed_dir_len == 1 && dir[0] == FILE_SEPARATOR_CHAR) ? 1 : 0;

  if (trimmed_dir_len > 0) {
    if (trimmed_dir_len >= size) {
      dest[size - 1] = '\0';
      return -1;
    }
    memcpy(dest, dir, trimmed_dir_len);
    dest[trimmed_dir_len] = '\0';
  }

  if (*leaf_part != '\0') {
    if (trimmed_dir_len > 0 && !is_root_dir &&
        AppendBounded(dest, size, FILE_SEPARATOR_STRING) != 0) {
      return -1;
    }
    if (AppendBounded(dest, size, leaf_part) != 0) {
      return -1;
    }
    return 0;
  }

  if (trimmed_dir_len > 0 && !is_root_dir &&
      AppendBounded(dest, size, FILE_SEPARATOR_STRING) != 0) {
    return -1;
  }

  return 0;
}

BOOL Path_BuildTempTemplate(char *dest, size_t size, const char *name_prefix) {
  const char *tmp_root;
  size_t tmp_root_len;
  const char *separator;
  int written;

  if (!dest || size == 0 || !name_prefix || name_prefix[0] == '\0') {
    return FALSE;
  }

  tmp_root = getenv("TMPDIR");
  if (!tmp_root || tmp_root[0] == '\0') {
    tmp_root = "/tmp";
  }

  tmp_root_len = strlen(tmp_root);
  separator = "";
  if (tmp_root_len > 0 && tmp_root[tmp_root_len - 1] != FILE_SEPARATOR_CHAR) {
    separator = FILE_SEPARATOR_STRING;
  }

  written = snprintf(dest, size, "%s%s%sXXXXXX", tmp_root, separator,
                     name_prefix);
  if (written < 0 || (size_t)written >= size) {
    dest[size - 1] = '\0';
    return FALSE;
  }

  return TRUE;
}

BOOL Path_CreateTempFile(char *dest, size_t size, const char *name_prefix,
                         BOOL unlink_after_open, int *fd_out) {
  int fd;

  if (!dest || size == 0 || !name_prefix || !fd_out) {
    return FALSE;
  }

  if (!Path_BuildTempTemplate(dest, size, name_prefix)) {
    return FALSE;
  }

  fd = mkstemp(dest);
  if (fd == -1) {
    dest[0] = '\0';
    return FALSE;
  }

  if (unlink_after_open) {
    if (unlink(dest) != 0) {
      close(fd);
      dest[0] = '\0';
      return FALSE;
    }
    dest[0] = '\0';
  }

  *fd_out = fd;
  return TRUE;
}

char *GetPath(DirEntry *dir_entry, char *buffer) {
  char *components[256];
  int depth = 0;
  DirEntry *de_ptr;
  const size_t buffer_size = PATH_LENGTH + 1;

  memset(components, 0, sizeof(components));
  *buffer = '\0';
  if (dir_entry == NULL) {
    return buffer;
  }

  /* Collect path components in reverse order (from leaf to root) */
  for (de_ptr = dir_entry; de_ptr != NULL && depth < 256;
       de_ptr = de_ptr->up_tree) {
    components[depth++] = de_ptr->name;
  }

  if (depth > 0 && components[depth - 1] != NULL) {
    /* The last component collected is the root. Start the path with it. */
    if (snprintf(buffer, buffer_size, "%s", components[depth - 1]) < 0) {
      buffer[0] = '\0';
      return buffer;
    }

    /* Append the rest of the components in correct order */
    for (int i = depth - 2; i >= 0; i--) {
      /* Add separator if the current path is not just the root "/" */
      if (strcmp(buffer, FILE_SEPARATOR_STRING) != 0) {
        if (AppendBounded(buffer, buffer_size, FILE_SEPARATOR_STRING) != 0) {
          break;
        }
      }
      if (AppendBounded(buffer, buffer_size, components[i]) != 0) {
        break;
      }
    }
  }
  return buffer;
}

char *GetFileNamePath(FileEntry *file_entry, char *buffer) {
  const size_t buffer_size = PATH_LENGTH + 1;

  (void)GetPath(file_entry->dir_entry, buffer);
  if (*buffer && strcmp(buffer, FILE_SEPARATOR_STRING))
    (void)AppendBounded(buffer, buffer_size, FILE_SEPARATOR_STRING);
  (void)AppendBounded(buffer, buffer_size, file_entry->name);
  return buffer;
}

char *GetRealFileNamePath(FileEntry *file_entry, char *buffer, int view_mode) {
  const size_t buffer_size = PATH_LENGTH + 1;

  if (view_mode == DISK_MODE || view_mode == USER_MODE)
    return (GetFileNamePath(file_entry, buffer));

  if (S_ISLNK(file_entry->stat_struct.st_mode)) {
    const char *sym_name = &file_entry->name[strlen(file_entry->name) + 1];
    if (*sym_name == FILE_SEPARATOR_CHAR)
      (void)snprintf(buffer, buffer_size, "%s", sym_name);
    if (*sym_name == FILE_SEPARATOR_CHAR)
      return buffer;
  }

  (void)GetPath(file_entry->dir_entry, buffer);
  if (*buffer && strcmp(buffer, FILE_SEPARATOR_STRING))
    (void)AppendBounded(buffer, buffer_size, FILE_SEPARATOR_STRING);
  if (S_ISLNK(file_entry->stat_struct.st_mode))
    (void)AppendBounded(buffer, buffer_size,
                        &file_entry->name[strlen(file_entry->name) + 1]);
  else
    (void)AppendBounded(buffer, buffer_size, file_entry->name);
  return buffer;
}

/* Aufsplitten des Dateinamens in die einzelnen Komponenten */
void Fnsplit(char *path, char *dir, char *name) {
  char *path_copy_dir;
  char *path_copy_base;
  const char *dname;
  const char *bname;
  const char *processed_path;
  int has_sep = 0;
  size_t len;

  /* Skip Whitespace */
  while (*path == ' ' || *path == '\t')
    path++;
  processed_path = path;

  /* Check for standard separator to distinguish file in current dir */
  if (strchr(processed_path, FILE_SEPARATOR_CHAR)) {
    has_sep = 1;
  }

  /* Preparation: create copies for dirname/basename */
  path_copy_dir = strdup(processed_path);
  path_copy_base = strdup(processed_path);

  if (!path_copy_dir || !path_copy_base) {
    if (path_copy_dir)
      free(path_copy_dir);
    if (path_copy_base)
      free(path_copy_base);
    fprintf(stderr, "ytree: strdup failed in Fnsplit\n");
    exit(1);
  }

  /* Execution */
  dname = dirname(path_copy_dir);
  bname = basename(path_copy_base);

  /* Directory Output (dir) */
  /* If original path had no separators (and dirname returned .): Write empty
   * string */
  if (!has_sep && strcmp(dname, ".") == 0) {
    *dir = '\0';
  } else {
    strncpy(dir, dname, PATH_LENGTH - 1);
    dir[PATH_LENGTH - 1] = '\0';

    /* Trailing Slash: If result is not root, append separator */
    if (strcmp(dir, FILE_SEPARATOR_STRING) != 0) {
      len = strlen(dir);
      if (len < PATH_LENGTH - 1) {
        dir[len] = FILE_SEPARATOR_CHAR;
        dir[len + 1] = '\0';
      }
    }
  }

  /* Filename Output (name) */
  len = strlen(bname);
  if (len >= PATH_LENGTH) {
    strncpy(name, bname, PATH_LENGTH - 1);
    name[PATH_LENGTH - 1] = '\0';
    fprintf(stderr, "ytree: warning: filename too long, truncating: %s\n",
            processed_path);
  } else {
    (void)snprintf(name, PATH_LENGTH + 1, "%s", bname);
  }

  /* Cleanup */
  free(path_copy_dir);
  free(path_copy_base);
}

const char *GetExtension(const char *filename) {
  const char *cptr;

  cptr = strrchr(filename, '.');

  if (cptr == NULL)
    return "";

  if (cptr == filename)
    return "";
  /* filenames beginning with a dot are not an extension */

  return (cptr + 1);
}

void NormPath(char *in_path, char *out_path) {
  /* Step 1: Try physical resolution using realpath (if file exists) */
  const size_t component_stack_limit = 256;
  char *canonical = realpath(in_path, NULL);
  int overflowed = 0;

  if (out_path == NULL) {
    errno = EINVAL;
    return;
  }

  out_path[0] = '\0';

  if (canonical != NULL) {
    /* Path exists on disk, use canonical absolute path */
    (void)snprintf(out_path, PATH_LENGTH + 1, "%s", canonical);
    free(canonical);
    return;
  }

  /* Step 2: Virtual/Logical Normalization (for archives or new dirs) */
  /* Fallback implementation when realpath fails */

  char stack[256][256]; /* Fixed depth stack for components */
  int stack_top = 0;
  char *path_copy;
  const char *token;
  char *saveptr;
  int is_absolute = 0;

  if (in_path == NULL || *in_path == '\0') {
    (void)snprintf(out_path, PATH_LENGTH + 1, "%s", ".");
    return;
  }

  if (in_path[0] == FILE_SEPARATOR_CHAR) {
    is_absolute = 1;
  }

  /* Create a mutable copy for tokenization */
  path_copy = strdup(in_path);
  if (path_copy == NULL) {
    UI_Message(NULL, "strdup failed*ABORT");
    exit(1);
  }

  token = strtok_r(path_copy, FILE_SEPARATOR_STRING, &saveptr);
  while (token != NULL) {
    if (strcmp(token, ".") == 0) {
      /* Ignore current dir */
    } else if (strcmp(token, "..") == 0) {
      /* Pop parent dir if stack is not empty */
      if (stack_top > 0) {
        stack_top--;
      } else if (!is_absolute) {
        /* If relative path '..' goes above start, keep '..' */
        /* NOTE: simplified logic here usually just stops at top */
        /* Alternatively: strcpy(stack[stack_top++], ".."); */
      }
    } else {
      /* Push directory */
      size_t token_len = strlen(token);
      if ((size_t)stack_top >= component_stack_limit || token_len >= 256) {
        overflowed = 1;
        break;
      }
      strncpy(stack[stack_top], token, 255);
      stack[stack_top][255] = '\0';
      stack_top++;
    }
    token = strtok_r(NULL, FILE_SEPARATOR_STRING, &saveptr);
  }

  free(path_copy);

  if (overflowed) {
    errno = ENAMETOOLONG;
    out_path[0] = '\0';
    return;
  }

  /* Reconstruct Path */
  out_path[0] = '\0';
  if (is_absolute) {
    (void)snprintf(out_path, PATH_LENGTH + 1, "%s", FILE_SEPARATOR_STRING);
  } else if (stack_top == 0) {
    /* If relative and stack empty (e.g. "./."), result is "." */
    (void)snprintf(out_path, PATH_LENGTH + 1, "%s", ".");
    return;
  }

  for (int i = 0; i < stack_top; i++) {
    if (i > 0) {
      if (AppendBounded(out_path, PATH_LENGTH + 1, FILE_SEPARATOR_STRING) !=
          0) {
        errno = ENAMETOOLONG;
        out_path[0] = '\0';
        return;
      }
    }
    if (AppendBounded(out_path, PATH_LENGTH + 1, stack[i]) != 0) {
      errno = ENAMETOOLONG;
      out_path[0] = '\0';
      return;
    }
  }
}
