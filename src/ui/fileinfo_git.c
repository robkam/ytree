/***************************************************************************
 *
 * src/ui/fileinfo_git.c
 * Cached Git-status helpers for the FileInfo band.
 *
 ***************************************************************************/

#include "ytnova_fs.h"
#include "ytnova_ui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void ClearGitStatusCache(YtreeNovaPanel *panel) {
  if (!panel)
    return;
  free(panel->git_status_entries);
  panel->git_status_entries = NULL;
  panel->git_status_entry_count = 0;
  panel->git_status_first_file = NULL;
  panel->git_status_last_file = NULL;
  panel->git_status_dir_path[0] = '\0';
  panel->git_status_is_worktree = FALSE;
}

void FileInfoGitInvalidate(YtreeNovaPanel *panel) { ClearGitStatusCache(panel); }

static BOOL ReadCommandOutput(const char *command, char **output_ptr) {
  FILE *pipe_fp;
  char *buffer = NULL;
  size_t used = 0;
  size_t capacity = 0;

  if (!command || !output_ptr)
    return FALSE;
  *output_ptr = NULL;

  pipe_fp = popen(command, "r");
  if (!pipe_fp)
    return FALSE;

  for (;;) {
    size_t chunk_size = 4096;
    size_t remaining;
    size_t read_now;
    char *new_buffer;

    if (capacity - used < chunk_size + 1) {
      size_t new_capacity = (capacity == 0) ? 8192 : capacity * 2;
      while (new_capacity - used < chunk_size + 1)
        new_capacity *= 2;
      new_buffer = (char *)realloc(buffer, new_capacity);
      if (!new_buffer) {
        free(buffer);
        (void)pclose(pipe_fp);
        return FALSE;
      }
      buffer = new_buffer;
      capacity = new_capacity;
    }

    remaining = capacity - used - 1;
    read_now = fread(buffer + used, 1, remaining, pipe_fp);
    used += read_now;
    if (read_now < remaining) {
      if (ferror(pipe_fp)) {
        free(buffer);
        (void)pclose(pipe_fp);
        return FALSE;
      }
      break;
    }
  }

  if (buffer == NULL) {
    buffer = (char *)malloc(1);
    if (!buffer) {
      (void)pclose(pipe_fp);
      return FALSE;
    }
  }
  buffer[used] = '\0';

  if (pclose(pipe_fp) != 0) {
    free(buffer);
    return FALSE;
  }

  *output_ptr = buffer;
  return TRUE;
}

static BOOL GitCommandForDir(const char *dir_path, const char *suffix,
                             char *command, size_t command_size) {
  char quoted_dir[(PATH_LENGTH * 4) + 3];

  if (!dir_path || !suffix || !command || command_size == 0)
    return FALSE;
  if (!Path_ShellQuote(dir_path, quoted_dir, sizeof(quoted_dir)))
    return FALSE;
  if (snprintf(command, command_size, "git -C %s %s", quoted_dir, suffix) >=
      (int)command_size)
    return FALSE;
  return TRUE;
}

static void TrimTrailingNewline(char *text) {
  size_t len;

  if (!text)
    return;
  len = strlen(text);
  while (len > 0 && (text[len - 1] == '\n' || text[len - 1] == '\r')) {
    text[len - 1] = '\0';
    len--;
  }
}

static BOOL GitDirShowPrefix(const char *dir_path, char *prefix,
                             size_t prefix_size) {
  char command[(PATH_LENGTH * 4) + 128];
  char *output = NULL;
  BOOL ok = FALSE;

  if (!dir_path || !prefix || prefix_size == 0)
    return FALSE;
  prefix[0] = '\0';

  if (!GitCommandForDir(dir_path, "rev-parse --show-prefix 2>/dev/null", command,
                        sizeof(command)))
    return FALSE;
  if (!ReadCommandOutput(command, &output) || !output)
    return FALSE;

  TrimTrailingNewline(output);
  if (snprintf(prefix, prefix_size, "%s", output) < (int)prefix_size)
    ok = TRUE;
  free(output);
  return ok;
}

static const char *PathWithinGitDirPrefix(const char *git_dir_prefix,
                                          const char *status_path) {
  size_t prefix_len;

  if (!status_path)
    return NULL;
  if (!git_dir_prefix || git_dir_prefix[0] == '\0')
    return status_path;

  prefix_len = strlen(git_dir_prefix);
  if (strncmp(status_path, git_dir_prefix, prefix_len) == 0)
    return status_path + prefix_len;
  return status_path;
}

static int GitStatusRank(const char code[3]) {
  if (!code)
    return 0;
  if (code[0] == '!' && code[1] == '!')
    return 1;
  if (code[0] == '?' && code[1] == '?')
    return 2;
  if (code[0] == 'D' || code[1] == 'D')
    return 5;
  if (code[0] == 'R' || code[1] == 'R' || code[0] == 'C' || code[1] == 'C')
    return 4;
  if (code[0] == 'M' || code[1] == 'M' || code[0] == 'A' || code[1] == 'A' ||
      code[0] == 'U' || code[1] == 'U')
    return 3;
  return 0;
}

static const char *GitStatusLabel(const char code[3]) {
  if (!code)
    return "clean";

  if (code[0] == ' ' && code[1] == ' ')
    return "clean";
  if ((code[0] == '?' && code[1] == '?') ||
      (code[0] == '!' && code[1] == '!'))
    return "untracked";
  if (code[0] == 'U' || code[1] == 'U' ||
      (code[0] == 'A' && code[1] == 'A') ||
      (code[0] == 'D' && code[1] == 'D'))
    return "conflict";
  if (code[0] == 'R' || code[1] == 'R' || code[0] == 'C' || code[1] == 'C')
    return "renamed";
  if (code[0] == 'D' || code[1] == 'D')
    return "deleted";
  if (code[1] != ' ')
    return "modified";
  if (code[0] != ' ')
    return "staged";
  return "clean";
}

static void SetEntryStatus(PanelGitStatusEntry *entry, const char code[3]) {
  if (!entry || !code)
    return;
  if (GitStatusRank(code) >= GitStatusRank(entry->code)) {
    entry->code[0] = code[0];
    entry->code[1] = code[1];
    entry->code[2] = '\0';
  }
}

static void ApplyGitStatusToEntry(YtreeNovaPanel *panel, const char *path,
                                  const char code[3]) {
  unsigned int index;

  if (!panel || !path || !code)
    return;

  for (index = 0; index < panel->git_status_entry_count; index++) {
    const FileEntry *file_entry = panel->git_status_entries[index].file;
    const char *name;
    size_t name_len;

    if (!file_entry)
      continue;
    name = file_entry->name;
    name_len = strlen(name);

    if (strcmp(path, name) == 0) {
      SetEntryStatus(&panel->git_status_entries[index], code);
      continue;
    }
    if (S_ISDIR(file_entry->stat_struct.st_mode) &&
        strncmp(path, name, name_len) == 0 && path[name_len] == '/') {
      SetEntryStatus(&panel->git_status_entries[index], code);
    }
  }
}

static void PrimeGitStatusEntries(YtreeNovaPanel *panel) {
  unsigned int index;

  if (!panel)
    return;

  panel->git_status_entry_count = panel->file_count;
  panel->git_status_first_file =
      (panel->file_count > 0 && panel->file_entry_list != NULL)
          ? panel->file_entry_list[0].file
          : NULL;
  panel->git_status_last_file =
      (panel->file_count > 0 && panel->file_entry_list != NULL)
          ? panel->file_entry_list[panel->file_count - 1].file
          : NULL;

  if (panel->git_status_entry_count == 0)
    return;

  panel->git_status_entries = (PanelGitStatusEntry *)calloc(
      panel->git_status_entry_count, sizeof(PanelGitStatusEntry));
  if (!panel->git_status_entries) {
    panel->git_status_entry_count = 0;
    panel->git_status_first_file = NULL;
    panel->git_status_last_file = NULL;
    return;
  }

  for (index = 0; index < panel->git_status_entry_count; index++) {
    panel->git_status_entries[index].file = panel->file_entry_list[index].file;
    panel->git_status_entries[index].code[0] = ' ';
    panel->git_status_entries[index].code[1] = ' ';
    panel->git_status_entries[index].code[2] = '\0';
  }
}

static BOOL GitDirMatchesCache(const YtreeNovaPanel *panel, const char *dir_path) {
  if (!panel || !dir_path || !panel->git_status_is_worktree)
    return FALSE;
  if (strcmp(panel->git_status_dir_path, dir_path) != 0)
    return FALSE;
  if (panel->git_status_entry_count != panel->file_count)
    return FALSE;
  if (panel->git_status_entry_count == 0)
    return TRUE;
  if (!panel->file_entry_list)
    return FALSE;
  return panel->git_status_first_file == panel->file_entry_list[0].file &&
         panel->git_status_last_file ==
             panel->file_entry_list[panel->file_count - 1].file;
}

static BOOL GitDirIsWorktree(const char *dir_path) {
  char command[(PATH_LENGTH * 4) + 128];
  char *output = NULL;
  BOOL result = FALSE;

  if (!GitCommandForDir(dir_path, "rev-parse --is-inside-work-tree 2>/dev/null",
                        command, sizeof(command)))
    return FALSE;
  if (ReadCommandOutput(command, &output) && output &&
      strncmp(output, "true", 4) == 0) {
    result = TRUE;
  }
  free(output);
  return result;
}

BOOL FileInfoGitRefresh(ViewContext *ctx, YtreeNovaPanel *panel,
                        const DirEntry *dir_entry) {
  char dir_path[PATH_LENGTH + 1];
  char git_dir_prefix[PATH_LENGTH + 1];
  char command[(PATH_LENGTH * 4) + 160];
  char *output = NULL;
  char *record;

  (void)ctx;
  if (!panel || !dir_entry)
    return FALSE;

  GetPath((DirEntry *)dir_entry, dir_path);
  if (!*dir_path)
    return FALSE;
  if (GitDirMatchesCache(panel, dir_path))
    return panel->git_status_is_worktree;

  ClearGitStatusCache(panel);
  if (!GitDirIsWorktree(dir_path))
    return FALSE;
  if (!GitDirShowPrefix(dir_path, git_dir_prefix, sizeof(git_dir_prefix)))
    git_dir_prefix[0] = '\0';

  if (!GitCommandForDir(
          dir_path,
          "status --porcelain=1 -z --ignored=matching --untracked-files=normal -- . 2>/dev/null",
          command, sizeof(command)))
    return FALSE;

  PrimeGitStatusEntries(panel);
  panel->git_status_is_worktree = TRUE;
  (void)snprintf(panel->git_status_dir_path, sizeof(panel->git_status_dir_path),
                 "%s", dir_path);

  if (!ReadCommandOutput(command, &output) || !output)
    return TRUE;

  record = output;
  while (*record) {
    char code[3];
    char *path;
    char *next;

    if (strlen(record) < 3)
      break;
    code[0] = record[0];
    code[1] = record[1];
    code[2] = '\0';
    path = record + 3;
    next = path + strlen(path);
    ApplyGitStatusToEntry(panel, PathWithinGitDirPrefix(git_dir_prefix, path),
                          code);
    record = next + 1;

    if ((code[0] == 'R' || code[1] == 'R' || code[0] == 'C' || code[1] == 'C') &&
        *record) {
      ApplyGitStatusToEntry(panel,
                            PathWithinGitDirPrefix(git_dir_prefix, record),
                            code);
      record += strlen(record) + 1;
    }
  }

  free(output);
  return TRUE;
}

void FileInfoGitDescribe(const YtreeNovaPanel *panel, const FileEntry *file_entry,
                         char *buffer, size_t buffer_size) {
  unsigned int index;

  if (!buffer || buffer_size == 0)
    return;
  buffer[0] = '\0';
  if (!panel || !panel->git_status_is_worktree || !file_entry)
    return;

  for (index = 0; index < panel->git_status_entry_count; index++) {
    const PanelGitStatusEntry *entry = &panel->git_status_entries[index];
    if (entry->file != file_entry)
      continue;
    (void)snprintf(buffer, buffer_size, " [%s]", GitStatusLabel(entry->code));
    return;
  }

  (void)snprintf(buffer, buffer_size, " [clean]");
}
