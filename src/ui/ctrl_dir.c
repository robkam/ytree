/***************************************************************************
 *
 * src/ui/ctrl_dir.c
 * Directory Window Controller (Input & Event Handling)
 *
 ***************************************************************************/

#define NO_YTREE_MACROS
#include "watcher.h"
#include "ytree.h"
#include <stdio.h>

/* TREEDEPTH uses GetProfileValue which is 2-arg in NO_YTREE_MACROS context */
#undef TREEDEPTH
#define TREEDEPTH (GetProfileValue)(ctx, "TREEDEPTH")

/* dir_list.c: BuildDirEntryList, FreeDirEntryList, FreeVolumeCache,
 *             GetPanelDirEntry, GetSelectedDirEntry */

/* dir_ops.c */
void HandlePlus(ViewContext *ctx, DirEntry *dir_entry, DirEntry *de_ptr,
                char *new_log_path, BOOL *need_dsp_help, YtreePanel *p);
void HandleReadSubTree(ViewContext *ctx, DirEntry *dir_entry,
                       BOOL *need_dsp_help, YtreePanel *p);
void HandleUnreadSubTree(ViewContext *ctx, DirEntry *dir_entry,
                         DirEntry *de_ptr, BOOL *need_dsp_help, YtreePanel *p);

void HandleSwitchWindow(ViewContext *ctx, DirEntry *dir_entry,
                        BOOL *need_dsp_help, int *ch, YtreePanel *p);
static void DirListJump(ViewContext *ctx, DirEntry **dir_entry_ptr,
                        const Statistic *s);
static void DrawDirListJumpPrompt(ViewContext *ctx, WINDOW *win,
                                  const char *search_buf);
static void HandleDirectoryCompare(ViewContext *ctx, DirEntry *source_dir);

static const char *PathLeafName(const char *path) {
  const char *sep;

  if (!path || !*path)
    return "";

  sep = strrchr(path, FILE_SEPARATOR_CHAR);
  if (!sep || !sep[1])
    return path;
  return sep + 1;
}

static void DrainPendingInput(ViewContext *ctx) {
  int ch;

  if (!ctx)
    return;

  nodelay(stdscr, TRUE);
  do {
    ch = WGetch(ctx, stdscr);
  } while (ch != ERR);
  nodelay(stdscr, FALSE);
}

static BOOL HasNonWhitespace(const char *text) {
  if (!text)
    return FALSE;
  while (*text) {
    if (!isspace((unsigned char)*text))
      return TRUE;
    text++;
  }
  return FALSE;
}

static int BuildCompareCommand(const char *command_template,
                               const char *source_path,
                               const char *target_path, char *command_line,
                               size_t command_line_size) {
  char escaped_source[PATH_LENGTH * 2 + 1];
  char escaped_target[PATH_LENGTH * 2 + 1];
  int written;
  BOOL has_placeholders;

  if (!command_template || !source_path || !target_path || !command_line ||
      command_line_size == 0) {
    return -1;
  }

  StrCp(escaped_source, source_path);
  StrCp(escaped_target, target_path);
  has_placeholders = (strstr(command_template, "%1") != NULL ||
                      strstr(command_template, "%2") != NULL);

  if (has_placeholders) {
    char expanded_command[COMMAND_LINE_LENGTH + 1];

    if (String_Replace(expanded_command, sizeof(expanded_command),
                       command_template, "%1", escaped_source) != 0) {
      return -1;
    }
    if (String_Replace(command_line, command_line_size, expanded_command, "%2",
                       escaped_target) != 0) {
      return -1;
    }
    return 0;
  }

  written = snprintf(command_line, command_line_size, "%s %s %s",
                     command_template, escaped_source, escaped_target);
  if (written < 0 || (size_t)written >= command_line_size)
    return -1;

  return 0;
}

static int ResolveDirectoryCompareTargetPath(const char *source_path,
                                             const char *target_input,
                                             char *resolved_target_path) {
  char raw_target_path[(PATH_LENGTH * 2) + 2];
  const char *home = NULL;
  int written;

  if (!source_path || !target_input || !resolved_target_path)
    return -1;
  if (!HasNonWhitespace(target_input))
    return -1;

  if (target_input[0] == FILE_SEPARATOR_CHAR) {
    written = snprintf(raw_target_path, sizeof(raw_target_path), "%s",
                       target_input);
  } else if (target_input[0] == '~' &&
             (target_input[1] == '\0' ||
              target_input[1] == FILE_SEPARATOR_CHAR) &&
             (home = getenv("HOME")) != NULL) {
    written = snprintf(raw_target_path, sizeof(raw_target_path), "%s%s", home,
                       target_input + 1);
  } else {
    written = snprintf(raw_target_path, sizeof(raw_target_path), "%s%c%s",
                       source_path, FILE_SEPARATOR_CHAR, target_input);
  }

  if (written < 0 || (size_t)written >= sizeof(raw_target_path))
    return -1;

  NormPath(raw_target_path, resolved_target_path);
  resolved_target_path[PATH_LENGTH] = '\0';
  return 0;
}

static void GetCommandDisplayName(const char *command_template,
                                  char *command_name, size_t command_name_size) {
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

typedef struct {
  unsigned long source_entries;
  unsigned long different_count;
  unsigned long match_count;
  unsigned long newer_count;
  unsigned long older_count;
  unsigned long unique_count;
  unsigned long type_mismatch_count;
  unsigned long error_count;
  unsigned long tagged_count;
  unsigned long skipped_unlogged_source_dirs;
} DirectoryCompareSummary;

static int CompareStatMtime(const struct stat *source_stat,
                            const struct stat *target_stat) {
  if (!source_stat || !target_stat)
    return 0;

  if (source_stat->st_mtime > target_stat->st_mtime)
    return 1;
  if (source_stat->st_mtime < target_stat->st_mtime)
    return -1;
  return 0;
}

static BOOL IsSameEntryType(const struct stat *source_stat,
                            const struct stat *target_stat) {
  if (!source_stat || !target_stat)
    return FALSE;
  return (source_stat->st_mode & S_IFMT) == (target_stat->st_mode & S_IFMT);
}

static int CompareFileContentExact(const char *source_path,
                                   const char *target_path) {
  char source_buf[8192];
  char target_buf[8192];
  int source_fd = -1;
  int target_fd = -1;
  int result = -1;

  if (!source_path || !target_path)
    return -1;

  source_fd = open(source_path, O_RDONLY);
  if (source_fd == -1)
    return -1;

  target_fd = open(target_path, O_RDONLY);
  if (target_fd == -1) {
    close(source_fd);
    return -1;
  }

  while (1) {
    ssize_t source_read;
    ssize_t target_read;

    do {
      source_read = read(source_fd, source_buf, sizeof(source_buf));
    } while (source_read < 0 && errno == EINTR);
    if (source_read < 0) {
      result = -1;
      break;
    }

    do {
      target_read = read(target_fd, target_buf, sizeof(target_buf));
    } while (target_read < 0 && errno == EINTR);
    if (target_read < 0) {
      result = -1;
      break;
    }

    if (source_read != target_read) {
      result = 0;
      break;
    }
    if (source_read == 0) {
      result = 1;
      break;
    }
    if (memcmp(source_buf, target_buf, (size_t)source_read) != 0) {
      result = 0;
      break;
    }
  }

  close(source_fd);
  close(target_fd);
  return result;
}

static int EvaluateDirectoryCompareBasis(CompareBasis basis,
                                         const struct stat *source_stat,
                                         const char *source_path,
                                         const struct stat *target_stat,
                                         const char *target_path,
                                         BOOL *basis_match) {
  int hash_compare_result;

  if (!source_stat || !source_path || !target_stat || !target_path ||
      !basis_match) {
    return -1;
  }

  switch (basis) {
  case COMPARE_BASIS_SIZE:
    *basis_match = (source_stat->st_size == target_stat->st_size);
    return 0;
  case COMPARE_BASIS_DATE:
    *basis_match = (CompareStatMtime(source_stat, target_stat) == 0);
    return 0;
  case COMPARE_BASIS_SIZE_AND_DATE:
    *basis_match = (source_stat->st_size == target_stat->st_size &&
                    CompareStatMtime(source_stat, target_stat) == 0);
    return 0;
  case COMPARE_BASIS_HASH:
    if (!S_ISREG(source_stat->st_mode) || !S_ISREG(target_stat->st_mode)) {
      errno = EINVAL;
      return -1;
    }
    if (source_stat->st_size != target_stat->st_size) {
      *basis_match = FALSE;
      return 0;
    }
    hash_compare_result = CompareFileContentExact(source_path, target_path);
    if (hash_compare_result < 0)
      return -1;
    *basis_match = (hash_compare_result == 1);
    return 0;
  default:
    return -1;
  }
}

static BOOL ShouldTagComparedFile(CompareTagResult tag_result, BOOL is_error,
                                  BOOL is_unique, BOOL is_type_mismatch,
                                  BOOL has_target, BOOL basis_known,
                                  BOOL basis_match, int mtime_cmp) {
  switch (tag_result) {
  case COMPARE_TAG_DIFFERENT:
    return (!is_error && !is_unique && !is_type_mismatch && basis_known &&
            !basis_match);
  case COMPARE_TAG_MATCH:
    return (!is_error && !is_unique && !is_type_mismatch && basis_known &&
            basis_match);
  case COMPARE_TAG_NEWER:
    return (!is_error && !is_type_mismatch && has_target && mtime_cmp > 0);
  case COMPARE_TAG_OLDER:
    return (!is_error && !is_type_mismatch && has_target && mtime_cmp < 0);
  case COMPARE_TAG_UNIQUE:
    return is_unique;
  case COMPARE_TAG_TYPE_MISMATCH:
    return is_type_mismatch;
  case COMPARE_TAG_ERROR:
    return is_error;
  default:
    return FALSE;
  }
}

static void TagComparedFile(FileEntry *source_file, Statistic *stats,
                            unsigned long *tagged_count) {
  DirEntry *owner_dir;
  long long file_size;

  if (!source_file || !stats || !source_file->dir_entry)
    return;

  if (source_file->tagged)
    return;

  owner_dir = source_file->dir_entry;
  file_size = source_file->stat_struct.st_size;
  source_file->tagged = TRUE;
  owner_dir->tagged_files++;
  owner_dir->tagged_bytes += file_size;
  stats->disk_tagged_files++;
  stats->disk_tagged_bytes += file_size;
  if (tagged_count)
    (*tagged_count)++;
}

static int BuildRelativeComparePath(const char *source_root_path,
                                    const char *source_entry_path,
                                    char *relative_path,
                                    size_t relative_path_size) {
  size_t root_len;
  const char *relative_start;

  if (!source_root_path || !source_entry_path || !relative_path ||
      relative_path_size == 0) {
    return -1;
  }

  root_len = strlen(source_root_path);
  if (!strcmp(source_root_path, FILE_SEPARATOR_STRING)) {
    if (source_entry_path[0] != FILE_SEPARATOR_CHAR || source_entry_path[1] == '\0')
      return -1;
    relative_start = source_entry_path + 1;
  } else {
    if (strncmp(source_entry_path, source_root_path, root_len) != 0)
      return -1;
    if (source_entry_path[root_len] != FILE_SEPARATOR_CHAR)
      return -1;
    if (source_entry_path[root_len + 1] == '\0')
      return -1;
    relative_start = source_entry_path + root_len + 1;
  }

  if (snprintf(relative_path, relative_path_size, "%s", relative_start) < 0 ||
      strlen(relative_start) >= relative_path_size) {
    return -1;
  }

  return 0;
}

static int BuildTargetComparePathFromRelative(const char *target_root_path,
                                              const char *relative_path,
                                              char *target_entry_path,
                                              size_t target_entry_path_size) {
  int written;

  if (!target_root_path || !relative_path || !target_entry_path ||
      target_entry_path_size == 0) {
    return -1;
  }

  if (!strcmp(target_root_path, FILE_SEPARATOR_STRING)) {
    written =
        snprintf(target_entry_path, target_entry_path_size, "/%s", relative_path);
  } else {
    written = snprintf(target_entry_path, target_entry_path_size, "%s%c%s",
                       target_root_path, FILE_SEPARATOR_CHAR, relative_path);
  }

  if (written < 0 || (size_t)written >= target_entry_path_size)
    return -1;

  return 0;
}

static void RunInternalDirectoryCompare(ViewContext *ctx, DirEntry *source_dir,
                                        const CompareRequest *request) {
  struct stat source_dir_stat;
  struct stat target_dir_stat;
  char source_path[PATH_LENGTH + 1];
  char target_path[PATH_LENGTH + 1];
  char source_entry_path[PATH_LENGTH + 1];
  char target_entry_path[PATH_LENGTH + 1];
  FileEntry *source_file;
  DirectoryCompareSummary summary;
  Statistic *stats = NULL;

  if (!ctx || !source_dir || !request)
    return;

  strncpy(source_path, request->source_path, PATH_LENGTH);
  source_path[PATH_LENGTH] = '\0';

  if (ResolveDirectoryCompareTargetPath(source_path, request->target_path,
                                        target_path) != 0) {
    UI_Message(ctx, "Compare target is empty or invalid.*Choose a target path.");
    return;
  }

  if (STAT_(source_path, &source_dir_stat) != 0) {
    UI_Message(ctx, "Compare source is not accessible:*\"%s\"*%s", source_path,
               strerror(errno));
    return;
  }
  if (STAT_(target_path, &target_dir_stat) != 0) {
    UI_Message(ctx,
               "Compare target does not exist:*\"%s\"*Select a valid directory.",
               target_path);
    return;
  }
  if (!S_ISDIR(source_dir_stat.st_mode) || !S_ISDIR(target_dir_stat.st_mode)) {
    UI_Message(ctx, "Directory compare requires two directories.");
    return;
  }
  if ((source_dir_stat.st_dev == target_dir_stat.st_dev &&
       source_dir_stat.st_ino == target_dir_stat.st_ino) ||
      !strcmp(source_path, target_path)) {
    UI_Message(ctx, "Can't compare: source and target are the same directory.");
    return;
  }

  memset(&summary, 0, sizeof(summary));
  if (ctx->active && ctx->active->vol)
    stats = &ctx->active->vol->vol_stats;

  for (source_file = source_dir->file; source_file; source_file = source_file->next) {
    struct stat source_stat;
    struct stat target_stat;
    BOOL has_target = FALSE;
    BOOL is_unique = FALSE;
    BOOL is_type_mismatch = FALSE;
    BOOL is_error = FALSE;
    BOOL basis_known = FALSE;
    BOOL basis_match = FALSE;
    int mtime_cmp = 0;
    int written;

    if (!source_file->matching)
      continue;

    summary.source_entries++;
    GetFileNamePath(source_file, source_entry_path);
    source_entry_path[PATH_LENGTH] = '\0';

    written =
        snprintf(target_entry_path, sizeof(target_entry_path), "%s%c%s",
                 target_path, FILE_SEPARATOR_CHAR, source_file->name);
    if (written < 0 || (size_t)written >= sizeof(target_entry_path)) {
      is_error = TRUE;
    } else if (STAT_(source_entry_path, &source_stat) != 0) {
      is_error = TRUE;
    } else if (STAT_(target_entry_path, &target_stat) != 0) {
      if (errno == ENOENT)
        is_unique = TRUE;
      else
        is_error = TRUE;
    } else {
      has_target = TRUE;
      mtime_cmp = CompareStatMtime(&source_stat, &target_stat);
      if (!IsSameEntryType(&source_stat, &target_stat)) {
        is_type_mismatch = TRUE;
      } else if (EvaluateDirectoryCompareBasis(request->basis, &source_stat,
                                               source_entry_path, &target_stat,
                                               target_entry_path,
                                               &basis_match) == 0) {
        basis_known = TRUE;
        if (basis_match)
          summary.match_count++;
        else
          summary.different_count++;
      } else {
        is_error = TRUE;
      }
    }

    if (has_target && !is_error && !is_type_mismatch) {
      if (mtime_cmp > 0)
        summary.newer_count++;
      else if (mtime_cmp < 0)
        summary.older_count++;
    }

    if (is_error)
      summary.error_count++;
    else if (is_unique)
      summary.unique_count++;
    else if (is_type_mismatch)
      summary.type_mismatch_count++;

    if (ShouldTagComparedFile(request->tag_result, is_error, is_unique,
                              is_type_mismatch, has_target, basis_known,
                              basis_match, mtime_cmp)) {
      TagComparedFile(source_file, stats, &summary.tagged_count);
    }
  }

  UI_Message(
      ctx,
      "Directory compare complete.*SOURCE: %s*TARGET: %s*BASIS: %s"
      "*SOURCE ENTRIES: %lu"
      "*RESULT COUNTS: different=%lu match=%lu newer=%lu older=%lu unique=%lu "
      "type-mismatch=%lu error=%lu"
      "*TAGGED (%s): %lu file(s) in active/source list.",
      PathLeafName(source_path), PathLeafName(target_path),
      UI_CompareBasisName(request->basis), summary.source_entries,
      summary.different_count, summary.match_count, summary.newer_count,
      summary.older_count, summary.unique_count, summary.type_mismatch_count,
      summary.error_count, UI_CompareTagResultName(request->tag_result),
      summary.tagged_count);
}

static void RunInternalLoggedTreeCompareRecursive(
    DirEntry *source_dir, const char *source_root_path,
    const char *target_root_path, const CompareRequest *request,
    DirectoryCompareSummary *summary, Statistic *stats) {
  DirEntry *child_dir;
  FileEntry *source_file;

  if (!source_dir || !source_root_path || !target_root_path || !request ||
      !summary) {
    return;
  }

  if (source_dir->not_scanned)
    summary->skipped_unlogged_source_dirs++;

  for (source_file = source_dir->file; source_file; source_file = source_file->next) {
    struct stat source_stat;
    struct stat target_stat;
    char source_entry_path[PATH_LENGTH + 1];
    char relative_path[PATH_LENGTH + 1];
    char target_entry_path[PATH_LENGTH + 1];
    BOOL has_target = FALSE;
    BOOL is_unique = FALSE;
    BOOL is_type_mismatch = FALSE;
    BOOL is_error = FALSE;
    BOOL basis_known = FALSE;
    BOOL basis_match = FALSE;
    int mtime_cmp = 0;

    if (!source_file->matching)
      continue;

    summary->source_entries++;
    GetFileNamePath(source_file, source_entry_path);
    source_entry_path[PATH_LENGTH] = '\0';

    if (BuildRelativeComparePath(source_root_path, source_entry_path,
                                 relative_path, sizeof(relative_path)) != 0) {
      is_error = TRUE;
    } else if (BuildTargetComparePathFromRelative(
                   target_root_path, relative_path, target_entry_path,
                   sizeof(target_entry_path)) != 0) {
      is_error = TRUE;
    } else if (STAT_(source_entry_path, &source_stat) != 0) {
      is_error = TRUE;
    } else if (STAT_(target_entry_path, &target_stat) != 0) {
      if (errno == ENOENT)
        is_unique = TRUE;
      else
        is_error = TRUE;
    } else {
      has_target = TRUE;
      mtime_cmp = CompareStatMtime(&source_stat, &target_stat);
      if (!IsSameEntryType(&source_stat, &target_stat)) {
        is_type_mismatch = TRUE;
      } else if (EvaluateDirectoryCompareBasis(request->basis, &source_stat,
                                               source_entry_path, &target_stat,
                                               target_entry_path,
                                               &basis_match) == 0) {
        basis_known = TRUE;
        if (basis_match)
          summary->match_count++;
        else
          summary->different_count++;
      } else {
        is_error = TRUE;
      }
    }

    if (has_target && !is_error && !is_type_mismatch) {
      if (mtime_cmp > 0)
        summary->newer_count++;
      else if (mtime_cmp < 0)
        summary->older_count++;
    }

    if (is_error)
      summary->error_count++;
    else if (is_unique)
      summary->unique_count++;
    else if (is_type_mismatch)
      summary->type_mismatch_count++;

    if (ShouldTagComparedFile(request->tag_result, is_error, is_unique,
                              is_type_mismatch, has_target, basis_known,
                              basis_match, mtime_cmp)) {
      TagComparedFile(source_file, stats, &summary->tagged_count);
    }
  }

  for (child_dir = source_dir->sub_tree; child_dir; child_dir = child_dir->next) {
    RunInternalLoggedTreeCompareRecursive(child_dir, source_root_path,
                                          target_root_path, request, summary,
                                          stats);
  }
}

static void RunInternalLoggedTreeCompare(ViewContext *ctx,
                                         const CompareRequest *request) {
  struct stat source_dir_stat;
  struct stat target_dir_stat;
  char source_path[PATH_LENGTH + 1];
  char target_path[PATH_LENGTH + 1];
  DirectoryCompareSummary summary;
  Statistic *stats = NULL;
  DirEntry *source_root = NULL;

  if (!ctx || !request || !ctx->active || !ctx->active->vol ||
      !ctx->active->vol->vol_stats.tree) {
    return;
  }

  source_root = ctx->active->vol->vol_stats.tree;
  strncpy(source_path, request->source_path, PATH_LENGTH);
  source_path[PATH_LENGTH] = '\0';

  if (ResolveDirectoryCompareTargetPath(source_path, request->target_path,
                                        target_path) != 0) {
    UI_Message(ctx, "Compare target is empty or invalid.*Choose a target path.");
    return;
  }

  if (STAT_(source_path, &source_dir_stat) != 0) {
    UI_Message(ctx, "Compare source is not accessible:*\"%s\"*%s", source_path,
               strerror(errno));
    return;
  }
  if (STAT_(target_path, &target_dir_stat) != 0) {
    UI_Message(ctx,
               "Compare target does not exist:*\"%s\"*Select a valid directory.",
               target_path);
    return;
  }
  if (!S_ISDIR(source_dir_stat.st_mode) || !S_ISDIR(target_dir_stat.st_mode)) {
    UI_Message(ctx, "Directory compare requires two directories.");
    return;
  }
  if ((source_dir_stat.st_dev == target_dir_stat.st_dev &&
       source_dir_stat.st_ino == target_dir_stat.st_ino) ||
      !strcmp(source_path, target_path)) {
    UI_Message(ctx, "Can't compare: source and target are the same directory.");
    return;
  }

  memset(&summary, 0, sizeof(summary));
  stats = &ctx->active->vol->vol_stats;

  RunInternalLoggedTreeCompareRecursive(source_root, source_path, target_path,
                                        request, &summary, stats);

  UI_Message(
      ctx,
      "Logged-tree compare complete.*SOURCE: %s*TARGET: %s*BASIS: %s"
      "*SOURCE ENTRIES: %lu"
      "*RESULT COUNTS: different=%lu match=%lu newer=%lu older=%lu unique=%lu "
      "type-mismatch=%lu error=%lu"
      "*SKIPPED UNLOGGED: source=%lu"
      "*TAGGED (%s): %lu file(s) in active/source list.",
      PathLeafName(source_path), PathLeafName(target_path),
      UI_CompareBasisName(request->basis), summary.source_entries,
      summary.different_count, summary.match_count, summary.newer_count,
      summary.older_count, summary.unique_count, summary.type_mismatch_count,
      summary.error_count, summary.skipped_unlogged_source_dirs,
      UI_CompareTagResultName(request->tag_result), summary.tagged_count);
}

static void LaunchExternalDirectoryCompare(ViewContext *ctx, DirEntry *source_dir,
                                           CompareFlowType flow_type) {
  CompareRequest request;
  struct stat source_stat;
  struct stat target_stat;
  char source_path[PATH_LENGTH + 1];
  char target_path[PATH_LENGTH + 1];
  char command_line[COMMAND_LINE_LENGTH + 1];
  const char *helper = NULL;
  const char *helper_key = NULL;
  int start_dir_fd = -1;
  int result = -1;

  if (!ctx)
    return;

  if (UI_BuildDirectoryCompareLaunchRequest(ctx, source_dir, flow_type,
                                            &request) != 0) {
    return;
  }

  helper = UI_GetCompareHelperCommand(ctx, flow_type);
  helper_key = (flow_type == COMPARE_FLOW_LOGGED_TREE) ? "TREEDIFF/DIRDIFF"
                                                        : "DIRDIFF";
  if (!HasNonWhitespace(helper)) {
    UI_Message(ctx, "%s helper is not configured.*Set %s in ~/.ytree.",
               UI_CompareFlowTypeName(flow_type), helper_key);
    return;
  }

  strncpy(source_path, request.source_path, PATH_LENGTH);
  source_path[PATH_LENGTH] = '\0';
  if (ResolveDirectoryCompareTargetPath(source_path, request.target_path,
                                        target_path) != 0) {
    UI_Message(ctx, "Compare target is empty or invalid.*Choose a target path.");
    return;
  }

  if (stat(source_path, &source_stat) != 0) {
    UI_Message(ctx, "Compare source is not accessible:*\"%s\"*%s", source_path,
               strerror(errno));
    return;
  }
  if (stat(target_path, &target_stat) != 0) {
    UI_Message(ctx,
               "Compare target does not exist:*\"%s\"*Select a valid directory.",
               target_path);
    return;
  }
  if (!S_ISDIR(source_stat.st_mode) || !S_ISDIR(target_stat.st_mode)) {
    UI_Message(ctx, "Directory compare helper requires two directories.");
    return;
  }
  if ((source_stat.st_dev == target_stat.st_dev &&
       source_stat.st_ino == target_stat.st_ino) ||
      !strcmp(source_path, target_path)) {
    UI_Message(ctx, "Can't compare: source and target are the same directory.");
    return;
  }

  if (BuildCompareCommand(helper, source_path, target_path, command_line,
                          sizeof(command_line)) != 0) {
    UI_Message(ctx, "%s command is invalid or too long.*Check %s in ~/.ytree.",
               helper_key, helper_key);
    return;
  }

  start_dir_fd = open(".", O_RDONLY);
  if (start_dir_fd == -1) {
    UI_Message(ctx, "Error preparing compare launch:*%s", strerror(errno));
    return;
  }
  if (chdir(source_path) != 0) {
    UI_Message(ctx, "Can't change directory to*\"%s\"*%s", source_path,
               strerror(errno));
    close(start_dir_fd);
    return;
  }

  DrainPendingInput(ctx);
  result = QuerySystemCall(ctx, command_line, &ctx->active->vol->vol_stats);

  if (fchdir(start_dir_fd) == -1) {
    UI_Message(ctx, "Error restoring directory*%s", strerror(errno));
  }
  close(start_dir_fd);

  if (result == -1) {
    UI_Message(ctx, "Failed to launch compare helper.*Check %s in ~/.ytree.",
               helper_key);
    return;
  }

  if (WIFEXITED(result)) {
    int exit_status = WEXITSTATUS(result);
    if (exit_status == 126 || exit_status == 127) {
      char command_name[PATH_LENGTH + 1];
      GetCommandDisplayName(helper, command_name, sizeof(command_name));
      UI_Message(ctx,
                 "Compare helper not available:*\"%s\"*"
                 "Install it or update %s in ~/.ytree.",
                 command_name[0] ? command_name : helper, helper_key);
    }
  }
}

static void RestorePanelFileSelection(DirEntry *dir_entry, YtreePanel *panel) {
  if (!dir_entry || !panel)
    return;

  if (panel->saved_focus != FOCUS_FILE)
    return;

  if (panel->file_dir_entry != dir_entry)
    return;

  dir_entry->start_file = panel->start_file;
  dir_entry->cursor_pos = panel->file_cursor_pos;
  if (dir_entry->start_file < 0)
    dir_entry->start_file = 0;
  if (dir_entry->cursor_pos < 0 && dir_entry->total_files > 0)
    dir_entry->cursor_pos = 0;
}

static void HandleDirectoryCompare(ViewContext *ctx, DirEntry *source_dir) {
  CompareMenuChoice menu_choice = COMPARE_MENU_CANCEL;
  CompareFlowType flow_type = COMPARE_FLOW_DIRECTORY;
  CompareRequest request;
  BOOL external_launch = FALSE;

  if (!ctx || !source_dir)
    return;

  if (UI_SelectCompareMenuChoice(ctx, &menu_choice) != 0)
    return;

  if (menu_choice == COMPARE_MENU_DIRECTORY_PLUS_TREE) {
    flow_type = COMPARE_FLOW_LOGGED_TREE;
  } else if (menu_choice == COMPARE_MENU_EXTERNAL_DIRECTORY) {
    flow_type = COMPARE_FLOW_DIRECTORY;
    external_launch = TRUE;
  } else if (menu_choice == COMPARE_MENU_EXTERNAL_TREE) {
    flow_type = COMPARE_FLOW_LOGGED_TREE;
    external_launch = TRUE;
  } else if (menu_choice != COMPARE_MENU_DIRECTORY_ONLY) {
    return;
  }

  if (external_launch) {
    LaunchExternalDirectoryCompare(ctx, source_dir, flow_type);
    RefreshView(ctx, source_dir);
    return;
  }

  if (UI_BuildDirectoryCompareRequest(ctx, source_dir, flow_type, &request) !=
      0) {
    return;
  }

  if (flow_type == COMPARE_FLOW_DIRECTORY) {
    RunInternalDirectoryCompare(ctx, source_dir, &request);
    return;
  }

  RunInternalLoggedTreeCompare(ctx, &request);
}

int HandleDirWindow(ViewContext *ctx, const DirEntry *start_dir_entry) {
  DirEntry *dir_entry, *de_ptr;
  int i, ch, unput_char;
  BOOL need_dsp_help;
  char new_name[PATH_LENGTH + 1];
  char new_log_path[PATH_LENGTH + 1];
  const char *home;
  YtreeAction action; /* Declare YtreeAction variable */
  const struct Volume *start_vol = NULL;
  Statistic *s = NULL;
  int height;
  char watcher_path[PATH_LENGTH + 1];

  DEBUG_LOG("HandleDirWindow: Recalculating layout");
  Layout_Recalculate(ctx);
  DEBUG_LOG("HandleDirWindow: Calling DisplayMenu");
  DisplayMenu(ctx);

  unput_char = 0;
  de_ptr = NULL;

  /* Safety fallback if Init() has not set up panels */
  if (ctx->active == NULL)
    ctx->active = ctx->left;
  if (ctx->active == NULL || ctx->active->vol == NULL)
    return ESC;

  start_vol = ctx->active->vol;
  s = &ctx->active->vol->vol_stats;

  if (ctx->active) {
    DEBUG_LOG("HandleDirWindow: Syncing panel state");
    ctx->focused_window = ctx->active->saved_focus;

    /* Update Global View Context to match Active Panel */
    /* This ensures macros like ctx->ctx_dir_window resolve to the
     * correct ncurses window
     */
    ctx->ctx_dir_window = ctx->active->pan_dir_window;
    ctx->ctx_small_file_window = ctx->active->pan_small_file_window;
    ctx->ctx_big_file_window = ctx->active->pan_big_file_window;
    ctx->ctx_file_window = ctx->active->pan_file_window;
  }

  /* Safety Reset for Preview Mode */
  if (ctx->preview_mode) {
    DEBUG_LOG("HandleDirWindow: Resetting preview mode");
    ctx->preview_mode = FALSE;
    ReCreateWindows(ctx);
    DisplayMenu(ctx);
    /* Update context again after ReCreateWindows */
    if (ctx->active) {
      ctx->ctx_dir_window = ctx->active->pan_dir_window;
      ctx->ctx_small_file_window = ctx->active->pan_small_file_window;
      ctx->ctx_big_file_window = ctx->active->pan_big_file_window;
      ctx->ctx_file_window = ctx->active->pan_file_window;
    }
  }

  height = getmaxy(ctx->ctx_dir_window);
  DEBUG_LOG("HandleDirWindow: Window height=%d", height);

  /* Clear flags */
  /*-----------------*/

  SetDirMode(ctx, MODE_3);

  need_dsp_help = TRUE;

  DEBUG_LOG("HandleDirWindow: Building DirEntryList for vol=%p",
            (void *)ctx->active->vol);
  BuildDirEntryList(ctx, ctx->active->vol, &ctx->active->current_dir_entry);
  if (ctx->initial_directory != NULL) {
    if (!strcmp(ctx->initial_directory, ".")) /* Entry just a single "." */
    {
      ctx->active->disp_begin_pos = 0;
      ctx->active->cursor_pos = 0;
      unput_char = CR;
    } else {
      int log_path_len = -1;
      if (*ctx->initial_directory == '.') { /* Entry of form "./alpha/beta" */
        log_path_len = snprintf(new_log_path, sizeof(new_log_path),
                                  "%s%s", start_dir_entry->name,
                                  ctx->initial_directory + 1);
      } else if (*ctx->initial_directory == '~' && (home = getenv("HOME"))) {
        /* Entry of form "~/alpha/beta" */
        log_path_len = snprintf(new_log_path, sizeof(new_log_path),
                                  "%s%s", home,
                                  ctx->initial_directory + 1);
      } else { /* Entry of form "beta" or "/full/path/alpha/beta" */
        log_path_len = snprintf(new_log_path, sizeof(new_log_path), "%s",
                                  ctx->initial_directory);
      }
      if (log_path_len >= 0 &&
          (size_t)log_path_len < sizeof(new_log_path)) {
        for (i = 0; i < ctx->active->vol->total_dirs; i++) {
          if (*new_log_path == FILE_SEPARATOR_CHAR) {
            GetPath(ctx->active->vol->dir_entry_list[i].dir_entry, new_name);
          } else {
            (void)snprintf(
                new_name, sizeof(new_name), "%s",
                ctx->active->vol->dir_entry_list[i].dir_entry->name);
          }
          if (!strcmp(new_log_path, new_name)) {
            ctx->active->disp_begin_pos = i;
            ctx->active->cursor_pos = 0;
            unput_char = CR;
            break;
          }
        }
      }
    }
    ctx->initial_directory = NULL;
  }

  {
    int safe_idx = ctx->active->disp_begin_pos + ctx->active->cursor_pos;
    if (ctx->active->vol->total_dirs <= 0) {
      ctx->active->disp_begin_pos = 0;
      ctx->active->cursor_pos = 0;
      safe_idx = 0;
    } else if (safe_idx < 0 || safe_idx >= ctx->active->vol->total_dirs) {
      ctx->active->disp_begin_pos = 0;
      ctx->active->cursor_pos = 0;
      safe_idx = 0;
    }

    if (ctx->active->vol->dir_entry_list) {
      dir_entry = ctx->active->vol->dir_entry_list[safe_idx].dir_entry;
    } else {
      dir_entry = ctx->active->vol->vol_stats.tree;
    }
  }

  /* REPLACEMENT START: Unified Rendering Call */
  if (!dir_entry->log_flag) {
    if (ctx->active && ctx->active->saved_focus == FOCUS_FILE) {
      if (ctx->active->file_dir_entry == dir_entry) {
        dir_entry->start_file = ctx->active->start_file;
        dir_entry->cursor_pos = ctx->active->file_cursor_pos;
      }
      if (dir_entry->start_file < 0)
        dir_entry->start_file = 0;
      if (dir_entry->cursor_pos < 0 && dir_entry->total_files > 0)
        dir_entry->cursor_pos = 0;
    } else {
      dir_entry->start_file = 0;
      dir_entry->cursor_pos = -1;
    }
  }

  RefreshView(ctx, dir_entry);
  /* REPLACEMENT END */

  if (dir_entry->log_flag) {
    if ((dir_entry->global_flag) || (dir_entry->tagged_flag)) {
      unput_char = 'S';
    } else {
      unput_char = CR;
    }
  } else if (ctx->active && ctx->active->saved_focus == FOCUS_FILE &&
             dir_entry->total_files > 0) {
    unput_char = CR;
  }
  do {
    /* Detect Global Volume Change (Split Brain Fix) */
    if (ctx->active == NULL || ctx->active->vol == NULL ||
        ctx->active->vol != start_vol)
      return ESC;

    if (need_dsp_help) {
      need_dsp_help = FALSE;
      DisplayDirHelp(ctx);
    }
    DisplayDirParameter(ctx, dir_entry);
    RefreshWindow(ctx->ctx_dir_window);

    if (ctx->is_split_screen) {
      YtreePanel *inactive =
          (ctx->active == ctx->left) ? ctx->right : ctx->left;
      RenderInactivePanel(ctx, inactive);
    }

    if (s->log_mode == DISK_MODE || s->log_mode == USER_MODE) {
      if (ctx->refresh_mode & REFRESH_WATCHER) {
        GetPath(dir_entry, watcher_path);
        Watcher_SetDir(ctx, watcher_path);
      }
    }

    if (unput_char) {
      ch = unput_char;
      unput_char = '\0';
    } else {
      doupdate();
      ch = (ctx->resize_request) ? -1 : GetEventOrKey(ctx);
      /* LF to CR normalization is now handled by GetKeyAction */
    }

    if (IsUserActionDefined(ctx)) { /* User commands take precedence */
      ch = DirUserMode(ctx, dir_entry, ch, &ctx->active->vol->vol_stats);
    }

    /* ViKey processing is now handled inside GetKeyAction */

    if (ctx->resize_request) {
      /* SIMPLIFIED RESIZE: Just call Global Refresh */
      RefreshView(ctx, dir_entry);
      need_dsp_help = TRUE;
      ctx->resize_request = FALSE;
    }

    action = GetKeyAction(ctx, ch); /* Translate raw input to YtreeAction */

    switch (action) {
    case ACTION_RESIZE:
      ctx->resize_request = TRUE;
      break;

    case ACTION_TOGGLE_STATS:
      ctx->show_stats = !ctx->show_stats;
      ctx->resize_request = TRUE;
      break;

    case ACTION_VIEW_PREVIEW: {
      const YtreePanel *saved_panel = ctx->active;

      /* Save Preview State - BEFORE potential panel switching */
      ctx->preview_return_panel = ctx->active;
      ctx->preview_return_focus = ctx->focused_window;

      ctx->preview_mode = TRUE;
      /* ADDED INSTRUCTION: Track Entry Point */
      ctx->preview_entry_focus = FOCUS_TREE;

      /* Enter File Window loop via HandleSwitchWindow logic */
      /* We use HandleSwitchWindow to encapsulate the setup for HandleFileWindow
       */
      HandleSwitchWindow(ctx, dir_entry, &need_dsp_help, &ch, ctx->active);

      /* Post-Return Cleanup */
      ctx->preview_mode = FALSE; /* Restore cleaning of preview mode flag */
      ctx->focused_window = FOCUS_TREE; /* Restore focus to tree on return */

      if (ctx->active != saved_panel) {
        /* If panel was switched during preview, we REFRESH local state
         * BEFORE calling RefreshView to ensure the correct entry is
         * drawn. */
        if (ctx->active->vol == NULL)
          return ESC;
        start_vol = ctx->active->vol;
        s = &ctx->active->vol->vol_stats;

        if (ctx->active->vol && ctx->active->vol->total_dirs > 0) {
          dir_entry = ctx->active->vol
                          ->dir_entry_list[ctx->active->disp_begin_pos +
                                           ctx->active->cursor_pos]
                          .dir_entry;
        } else {
          dir_entry = s->tree;
        }

        /* Sync Global View Context */
        ctx->ctx_dir_window = ctx->active->pan_dir_window;
        ctx->ctx_small_file_window = ctx->active->pan_small_file_window;
        ctx->ctx_big_file_window = ctx->active->pan_big_file_window;
        ctx->ctx_file_window = ctx->active->pan_file_window;

        /* EXPLICIT REFRESH with the CORRECT dir_entry */
        RefreshView(ctx, dir_entry);

        need_dsp_help = TRUE;
        continue;
      }

      /* Standard refresh if panel didn't change */
      RefreshView(ctx, dir_entry);

      need_dsp_help = TRUE;

      /* Refresh local pointer */
      if (ctx->active->vol->total_dirs > 0) {
        dir_entry = ctx->active->vol
                        ->dir_entry_list[ctx->active->disp_begin_pos +
                                         ctx->active->cursor_pos]
                        .dir_entry;
      } else {
        dir_entry = s->tree;
      }
    } break;

    case ACTION_SPLIT_SCREEN:
      if (ctx->is_split_screen && ctx->active == ctx->right) {
        /* Preserve Right Panel state into Left Panel before unsplitting */
        ctx->left->vol = ctx->right->vol;
        ctx->left->cursor_pos = ctx->right->cursor_pos;
        ctx->left->disp_begin_pos = ctx->right->disp_begin_pos;
        ctx->left->start_file = ctx->right->start_file;
        ctx->left->file_cursor_pos = ctx->right->file_cursor_pos;
        ctx->left->file_dir_entry = ctx->right->file_dir_entry;
        ctx->left->saved_big_file_view = ctx->right->saved_big_file_view;
        ctx->left->saved_focus = ctx->right->saved_focus;
        /* Left panel now follows right panel state; force rebuild to avoid
         * stale file list from an older volume. */
        FreeFileEntryList(ctx->left);
        /* Sync Global Volume */
        ctx->active->vol = ctx->left->vol;
      }

      ctx->is_split_screen = !ctx->is_split_screen;
      ReCreateWindows(ctx); /* Force layout update immediately */

      if (ctx->is_split_screen) {
        if (ctx->right && ctx->left) {
          ctx->right->vol = ctx->left->vol;
          ctx->right->cursor_pos = ctx->left->cursor_pos;
          ctx->right->disp_begin_pos = ctx->left->disp_begin_pos;
          ctx->right->start_file = ctx->left->start_file;
          ctx->right->file_cursor_pos = ctx->left->file_cursor_pos;
          ctx->right->file_dir_entry = ctx->left->file_dir_entry;
          ctx->right->saved_big_file_view = ctx->left->saved_big_file_view;
          /* Start a newly split peer panel in tree focus by default. */
          ctx->right->saved_focus = FOCUS_TREE;
          /* Right panel inherited a new volume/state; drop stale cache so
           * inactive render shows the correct mirror immediately. */
          FreeFileEntryList(ctx->right);
        }
      } else {
        /* Prevent hidden peer cache from leaking across later volume splits. */
        FreeFileEntryList(ctx->right);
        ctx->active = ctx->left;
      }
      ctx->focused_window = ctx->active->saved_focus;
      RefreshView(ctx, dir_entry);
      need_dsp_help = TRUE;
      break;

    case ACTION_SWITCH_PANEL:
      if (!ctx->is_split_screen)
        break;

      /* Save local state to Active Panel for persistence before switching */
      /* Note: Navigation actions already update ctx->active->cursor_pos
       * directly. Do NOT overwrite with stale local_cursor_pos! */

      /* Switch Panel */
      if (ctx->active == ctx->left) {
        ctx->active = ctx->right;
      } else {
        ctx->active = ctx->left;
      }

      /* Restore Volume context */
      ctx->focused_window = ctx->active->saved_focus;
      s = &ctx->active->vol->vol_stats; /* UPDATE S */
      /* Do NOT overwrite ctx->active->cursor_pos here; preserve its
       * independent state */

      /* Recalculate local dir_entry for the new ctx->active */
      if (ctx->active->vol->total_dirs > 0) {
        /* Clamp coordinates just in case */
        if (ctx->active->disp_begin_pos + ctx->active->cursor_pos >=
            ctx->active->vol->total_dirs) {
          ctx->active->cursor_pos =
              ctx->active->vol->total_dirs - 1 - ctx->active->disp_begin_pos;
        }
        dir_entry = ctx->active->vol
                        ->dir_entry_list[ctx->active->disp_begin_pos +
                                         ctx->active->cursor_pos]
                        .dir_entry;
      } else {
        dir_entry = s->tree;
      }
      DEBUG_LOG("ACTION_SWITCH_PANEL: active panel is now %s with "
                "cursor_pos=%d, dir_entry=%s",
                ctx->active == ctx->left ? "LEFT" : "RIGHT",
                ctx->active->cursor_pos, dir_entry ? dir_entry->name : "NULL");

      /* Sync Global View Context */
      ctx->ctx_dir_window = ctx->active->pan_dir_window;
      ctx->ctx_small_file_window = ctx->active->pan_small_file_window;
      ctx->ctx_big_file_window = ctx->active->pan_big_file_window;
      ctx->ctx_file_window = ctx->active->pan_file_window;

      /* REFRESH View for new active panel */
      RestorePanelFileSelection(dir_entry, ctx->active);
      RefreshView(ctx, dir_entry);
      need_dsp_help = TRUE;
      if (ctx->focused_window == FOCUS_FILE && dir_entry->total_files > 0) {
        unput_char = CR;
      }
      continue; /* Stay in loop with new context */

    case ACTION_NONE: /* -1 or unhandled keys */
      if (ch == -1)
        break; /* Ignore -1 (ctx->resize_request handled above) */
      /* Fall through for other unhandled keys to beep */
      UI_Beep(ctx, FALSE);
      break;

    case ACTION_MOVE_DOWN:
      DirNav_Movedown(ctx, &dir_entry, ctx->active);
      break;
    case ACTION_MOVE_UP:
      DirNav_Moveup(ctx, &dir_entry, ctx->active);
      break;
    case ACTION_MOVE_SIBLING_NEXT: {
      const DirEntry *target = dir_entry->next;
      if (target == NULL) {
        /* Wrap to first sibling */
        if (dir_entry->up_tree != NULL) {
          target = dir_entry->up_tree->sub_tree;
        }
      }

      if (target != NULL && target != dir_entry) {
        /* Find the sibling in the linear list to update cursor index */
        int k;
        int found_idx = -1;

        for (k = 0; k < ctx->active->vol->total_dirs; k++) {
          if (ctx->active->vol->dir_entry_list[k].dir_entry == target) {
            found_idx = k;
            break;
          }
        }

        if (found_idx != -1) {
          /* Move cursor to sibling */
          if (found_idx >= ctx->active->disp_begin_pos &&
              found_idx < ctx->active->disp_begin_pos + height) {
            ctx->active->cursor_pos = found_idx - ctx->active->disp_begin_pos;
          } else {
            /* Off screen, center it or move to top */
            ctx->active->disp_begin_pos = found_idx;
            ctx->active->cursor_pos = 0;
            /* Bounds check */
            if (ctx->active->disp_begin_pos + height >
                ctx->active->vol->total_dirs) {
              ctx->active->disp_begin_pos =
                  MAXIMUM(0, ctx->active->vol->total_dirs - height);
              ctx->active->cursor_pos = found_idx - ctx->active->disp_begin_pos;
            }
          }
          /* Sync */
          dir_entry = ctx->active->vol
                          ->dir_entry_list[ctx->active->disp_begin_pos +
                                           ctx->active->cursor_pos]
                          .dir_entry;

          if (0) {
            dir_entry = RefreshTreeSafe(ctx, ctx->active, dir_entry);
            break; /* Skip manual refresh logic below */
          }

          /* Refresh */
          DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                      ctx->active->disp_begin_pos,
                      ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                      TRUE);
          DisplayFileWindow(ctx, ctx->active, dir_entry);
          DisplayDiskStatistic(ctx, s);
          UpdateStatsPanel(ctx, dir_entry, s);
          DisplayAvailBytes(ctx, s);

          char path[PATH_LENGTH];
          GetPath(dir_entry, path);
          DisplayHeaderPath(ctx, path);
        }
      }
    }
      need_dsp_help = TRUE;
      break;
    case ACTION_MOVE_SIBLING_PREV: {
      DirEntry *target = dir_entry->prev;
      if (target == NULL) {
        /* Wrap to last sibling */
        if (dir_entry->up_tree != NULL) {
          target = dir_entry->up_tree->sub_tree;
          while (target && target->next != NULL) {
            target = target->next;
          }
        }
      }

      if (target != NULL && target != dir_entry) {
        /* Find the sibling in the linear list to update cursor index */
        int k;
        int found_idx = -1;

        for (k = 0; k < ctx->active->vol->total_dirs; k++) {
          if (ctx->active->vol->dir_entry_list[k].dir_entry == target) {
            found_idx = k;
            break;
          }
        }

        if (found_idx != -1) {
          /* Move cursor to sibling */
          if (found_idx >= ctx->active->disp_begin_pos &&
              found_idx < ctx->active->disp_begin_pos + height) {
            ctx->active->cursor_pos = found_idx - ctx->active->disp_begin_pos;
          } else {
            /* Off screen, center it or move to top */
            ctx->active->disp_begin_pos = found_idx;
            ctx->active->cursor_pos = 0;
            /* Bounds check */
            if (ctx->active->disp_begin_pos + height >
                ctx->active->vol->total_dirs) {
              ctx->active->disp_begin_pos =
                  MAXIMUM(0, ctx->active->vol->total_dirs - height);
              ctx->active->cursor_pos = found_idx - ctx->active->disp_begin_pos;
            }
          }
          /* Sync */
          dir_entry = ctx->active->vol
                          ->dir_entry_list[ctx->active->disp_begin_pos +
                                           ctx->active->cursor_pos]
                          .dir_entry;

          if (0) {
            dir_entry = RefreshTreeSafe(ctx, ctx->active, dir_entry);
            break; /* Skip manual refresh logic below */
          }

          /* Refresh */
          DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                      ctx->active->disp_begin_pos,
                      ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                      TRUE);
          DisplayFileWindow(ctx, ctx->active, dir_entry);
          DisplayDiskStatistic(ctx, s);
          UpdateStatsPanel(ctx, dir_entry, s);
          DisplayAvailBytes(ctx, s);

          char path[PATH_LENGTH];
          GetPath(dir_entry, path);
          DisplayHeaderPath(ctx, path);
        }
      }
    }
      need_dsp_help = TRUE;
      break;
    case ACTION_PAGE_DOWN:
      DirNav_Movenpage(ctx, &dir_entry, ctx->active);
      break;
    case ACTION_PAGE_UP:
      DirNav_Moveppage(ctx, &dir_entry, ctx->active);
      break;
    case ACTION_HOME:
      DirNav_MoveHome(ctx, &dir_entry, ctx->active);
      break;
    case ACTION_END:
      DirNav_MoveEnd(ctx, &dir_entry, ctx->active);
      break;
    case ACTION_MOVE_RIGHT:
    case ACTION_TREE_EXPAND_ALL:
      HandlePlus(ctx, dir_entry, de_ptr, new_log_path, &need_dsp_help,
                 ctx->active);
      break;
    case ACTION_ASTERISK:
      HandleReadSubTree(ctx, dir_entry, &need_dsp_help, ctx->active);
      break;
    case ACTION_TREE_EXPAND:
      HandleReadSubTree(ctx, dir_entry, &need_dsp_help, ctx->active);
      break;
    case ACTION_MOVE_LEFT:
      /* 1. Transparent Archive Exit Logic (At Root) */
      if (dir_entry->up_tree == NULL && ctx->view_mode == ARCHIVE_MODE) {
        struct Volume *old_vol = ctx->active->vol;
        char archive_path[PATH_LENGTH + 1];
        char parent_dir[PATH_LENGTH + 1];
        char dummy_name[PATH_LENGTH + 1];

        /* Calculate Parent Directory of the Archive File */
        (void)snprintf(archive_path, sizeof(archive_path), "%s",
                       s->log_path);
        Fnsplit(archive_path, parent_dir, dummy_name);

        /* Force Log/Switch to the Parent Directory */
        /* This handles both "New Volume" and "Switch to Existing" logic
         * automatically */
        if (LogDisk(ctx, ctx->active, parent_dir) == 0) {
          /* Successfully switched context. */

          /* Delete the archive wrapper we just left to clean up memory/list */
          /* Fix Archive Double-Free Check */
          BOOL vol_in_use = FALSE;
          if (ctx->is_split_screen) {
            const YtreePanel *other =
                (ctx->active == ctx->left) ? ctx->right : ctx->left;
            if (other->vol == old_vol)
              vol_in_use = TRUE;
          }
          if (!vol_in_use)
            Volume_Delete(ctx, old_vol);

          /* Update pointers for the new context */
          s = &ctx->active->vol->vol_stats; /* UPDATE S */
          start_vol = ctx->active->vol;     /* Update loop safety variable */

          /* Reset Search Term to prevent bleeding */
          ctx->global_search_term[0] = '\0';

          /* Re-sync selected directory in the new context. */
          if (ctx->active->vol->total_dirs > 0) {
            dir_entry = ctx->active->vol
                            ->dir_entry_list[ctx->active->disp_begin_pos +
                                             ctx->active->cursor_pos]
                            .dir_entry;
          } else {
            dir_entry = s->tree;
          }

          /* Tree focus: keep file list in small-window mode and no file cursor.
           */
          dir_entry->start_file = 0;
          dir_entry->cursor_pos = -1;

          /* Keep the just-exited archive file visible.
           * Use filtered file_entry_list indices to avoid blank panes when
           * hidden/filter-mismatched files exist.
           */
          if (ctx->active->vol->total_dirs > 0) {
            int target_file_idx = -1;
            int file_idx;
            int visible_rows;

            BuildFileEntryList(ctx, ctx->active);
            visible_rows = getmaxy(ctx->ctx_small_file_window);
            if (visible_rows < 1)
              visible_rows = 1;

            for (file_idx = 0; file_idx < (int)ctx->active->file_count;
                 file_idx++) {
              const FileEntry *fe = ctx->active->file_entry_list[file_idx].file;
              if (fe && strcmp(fe->name, dummy_name) == 0) {
                target_file_idx = file_idx;
                break;
              }
            }

            if (target_file_idx >= visible_rows) {
              dir_entry->start_file = target_file_idx - (visible_rows - 1);
            }
            if (dir_entry->start_file >= (int)ctx->active->file_count) {
              dir_entry->start_file =
                  MAXIMUM(0, (int)ctx->active->file_count - 1);
            }
            if (dir_entry->start_file < 0) {
              dir_entry->start_file = 0;
            }
          }

          /* Keep mode in sync with selected volume and do a full redraw. */
          ctx->view_mode = ctx->active->vol->vol_stats.log_mode;
          RefreshView(ctx, dir_entry);
          need_dsp_help = TRUE;
          break; /* Exit case done */
        }
      }

      /* 2. Standard Tree Navigation Logic */
      if (!dir_entry->not_scanned && dir_entry->sub_tree != NULL) {
        /* It is expanded */
        if (ctx->view_mode == ARCHIVE_MODE) {
          /* In Archive Mode, we cannot collapse (UnRead) without data loss.
          So we skip collapse and fall through to Jump to Parent. */
          goto JUMP_TO_PARENT;
        } else {
          /* In FS Mode, collapse it. */
          HandleUnreadSubTree(ctx, dir_entry, de_ptr, &need_dsp_help,
                              ctx->active);
        }
      } else {
        /* It is collapsed (or leaf) -> Jump to Parent */
      JUMP_TO_PARENT:
        if (dir_entry->up_tree != NULL) {
          /* Find parent in the list */
          int p_idx = -1;
          int k;
          for (k = 0; k < ctx->active->vol->total_dirs; k++) {
            if (ctx->active->vol->dir_entry_list[k].dir_entry ==
                dir_entry->up_tree) {
              p_idx = k;
              break;
            }
          }
          if (p_idx != -1) {
            /* Move cursor to parent */
            if (p_idx >= ctx->active->disp_begin_pos &&
                p_idx < ctx->active->disp_begin_pos + height) {
              /* Parent is on screen */
              ctx->active->cursor_pos = p_idx - ctx->active->disp_begin_pos;
            } else {
              /* Parent is off screen - center it or move to top */
              ctx->active->disp_begin_pos = p_idx;
              ctx->active->cursor_pos = 0;
              /* Adjust if near end */
              if (ctx->active->disp_begin_pos + height >
                  ctx->active->vol->total_dirs) {
                ctx->active->disp_begin_pos =
                    MAXIMUM(0, ctx->active->vol->total_dirs - height);
                ctx->active->cursor_pos = p_idx - ctx->active->disp_begin_pos;
              }
            }
            /* Sync pointers */
            dir_entry = ctx->active->vol
                            ->dir_entry_list[ctx->active->disp_begin_pos +
                                             ctx->active->cursor_pos]
                            .dir_entry;

            /* Refresh */
            DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                        ctx->active->disp_begin_pos,
                        ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                        TRUE);
            DisplayFileWindow(ctx, ctx->active, dir_entry);
            DisplayDiskStatistic(ctx, s);
            UpdateStatsPanel(ctx, dir_entry, s);
            DisplayAvailBytes(ctx, s);
            /* Update Header Path */
            char path[PATH_LENGTH];
            GetPath(dir_entry, path);
            DisplayHeaderPath(ctx, path);
          }
        }
      }
      break;
    case ACTION_TREE_COLLAPSE:
      HandleUnreadSubTree(ctx, dir_entry, de_ptr, &need_dsp_help, ctx->active);
      break;
    case ACTION_TOGGLE_HIDDEN: {
      ToggleDotFiles(ctx, ctx->active);

      /* Update current dir pointer using the new accessor function
      because ToggleDotFiles might have changed the list layout */
      if (ctx->active->vol->total_dirs > 0) {
        dir_entry = ctx->active->vol
                        ->dir_entry_list[ctx->active->disp_begin_pos +
                                         ctx->active->cursor_pos]
                        .dir_entry;
      } else {
        dir_entry = s->tree;
      }

      need_dsp_help = TRUE;
    } break;
    case ACTION_FILTER:
      if (UI_ReadFilter(ctx) == 0) {
        RecalculateSysStats(ctx, s);
        dir_entry->start_file = 0;
        dir_entry->cursor_pos = -1;
        RefreshView(ctx, dir_entry);
      }
      need_dsp_help = TRUE;
      break;
    case ACTION_LIST_JUMP:
      DirListJump(ctx, &dir_entry, s);
      need_dsp_help = TRUE;
      break;
    case ACTION_TAG:
    case ACTION_UNTAG:
    case ACTION_TAG_ALL:
    case ACTION_UNTAG_ALL:
    case ACTION_CMD_TAGGED_S:
      if (HandleDirTagActions(ctx, action, &dir_entry, &need_dsp_help, &ch)) {
        break;
      }
      break;

    case ACTION_TOGGLE_MODE:
      RotateDirMode(ctx);
      /*DisplayFileWindow(ctx,  dir_entry, 0, -1 );*/
      DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                  ctx->active->disp_begin_pos,
                  ctx->active->disp_begin_pos + ctx->active->cursor_pos, TRUE);
      /*RefreshWindow( ctx->ctx_file_window );*/
      DisplayDiskStatistic(ctx, s);
      UpdateStatsPanel(ctx, dir_entry, s);
      need_dsp_help = TRUE;
      break;

    case ACTION_CMD_S:
      HandleShowAll(ctx, FALSE, FALSE, dir_entry, &need_dsp_help, &ch,
                    ctx->active);
      break;
    case ACTION_COMPARE_DIR:
      HandleDirectoryCompare(ctx, dir_entry);
      need_dsp_help = TRUE;
      break;
    case ACTION_COMPARE_TREE:
      HandleDirectoryCompare(ctx, dir_entry);
      need_dsp_help = TRUE;
      break;
    case ACTION_ENTER:
      if (dir_entry == NULL) {
        UI_Beep(ctx, FALSE);
        break;
      }

      if (ctx->refresh_mode & REFRESH_ON_ENTER) {
        dir_entry = RefreshTreeSafe(ctx, ctx->active, dir_entry);
        /* Sync pointer from list in case address changed */
        dir_entry = ctx->active->vol
                        ->dir_entry_list[ctx->active->disp_begin_pos +
                                         ctx->active->cursor_pos]
                        .dir_entry;
      }

      DEBUG_LOG("ACTION_ENTER: dir_entry=%p name=%s matching=%u",
                (void *)dir_entry, dir_entry ? dir_entry->name : "NULL",
                dir_entry ? (unsigned int)dir_entry->matching_files : 0U);

      if (dir_entry == NULL || dir_entry->total_files == 0) {
        UI_Beep(ctx, FALSE);
        break;
      }

      /* Fix Context Safety on Return */
      {
        const YtreePanel *saved_panel = ctx->active;

        /* Note: Navigation actions already update active->cursor_pos. */

        HandleSwitchWindow(ctx, dir_entry, &need_dsp_help, &ch, ctx->active);

        /* Restore whichever focus the active panel had before/after the
         * switch. */
        ctx->focused_window = ctx->active->saved_focus;

        if (ctx->active != saved_panel) {
          /* If panel was switched, refresh state locally and continue */
          if (ctx->active->vol == NULL)
            return ESC;
          start_vol = ctx->active->vol;
          s = &ctx->active->vol->vol_stats;

          if (ctx->active->vol && ctx->active->vol->total_dirs > 0) {
            dir_entry = ctx->active->vol
                            ->dir_entry_list[ctx->active->disp_begin_pos +
                                             ctx->active->cursor_pos]
                            .dir_entry;
          } else {
            dir_entry = s->tree;
          }

          ctx->ctx_dir_window = ctx->active->pan_dir_window;
          ctx->ctx_small_file_window = ctx->active->pan_small_file_window;
          ctx->ctx_big_file_window = ctx->active->pan_big_file_window;
          ctx->ctx_file_window = ctx->active->pan_file_window;

          RestorePanelFileSelection(dir_entry, ctx->active);
          RefreshView(ctx, dir_entry);
          need_dsp_help = TRUE;
          if (ctx->focused_window == FOCUS_FILE && dir_entry->total_files > 0) {
            unput_char = CR;
          }
          action = ACTION_NONE; /* Prevent while(...) condition from exiting
                                   HandleDirWindow! */
          continue;
        }

        /* Important: Check for volume changes after returning from File Window
         */
        if (ctx->active->vol != start_vol)
          return ESC;

        /* CENTRALIZED REFRESH: Restore clean layout after return */
        RefreshView(ctx, dir_entry);
      }
      /* Refresh local pointer */
      dir_entry = ctx->active->vol
                      ->dir_entry_list[ctx->active->disp_begin_pos +
                                       ctx->active->cursor_pos]
                      .dir_entry;
      break;
    case ACTION_CMD_X:
      if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
      } else {
        char command_template[COMMAND_LINE_LENGTH + 1];
        command_template[0] = '\0';
        if (GetCommandLine(ctx, command_template) == 0) {
          (void)Execute(ctx, dir_entry, NULL, command_template,
                        &ctx->active->vol->vol_stats, UI_ArchiveCallback);
          dir_entry = RefreshTreeSafe(
              ctx, ctx->active, dir_entry); /* Auto-Refresh after command */
        }
      }
      need_dsp_help = TRUE;
      DisplayAvailBytes(ctx, s);
      DisplayDiskStatistic(ctx, s);
      UpdateStatsPanel(ctx, dir_entry, s);
      break;
    case ACTION_CMD_MKFILE:
      DEBUG_LOG("ACTION_CMD_MKFILE reached in ctrl_dir.c. mode=%d",
                ctx->view_mode);
      if (ctx->view_mode != DISK_MODE)
        break;
      {
        char file_name[PATH_LENGTH * 2 + 1];

        ClearHelp(ctx);
        *file_name = '\0';
        if (UI_ReadString(ctx, ctx->active, "MAKE FILE:", file_name,
                          PATH_LENGTH, HST_FILE) == CR) {
          int mk_result =
              MakeFile(ctx, dir_entry, file_name, s, NULL, UI_ChoiceResolver);
          if (mk_result == 0) {
            /* Determine where the new file should be. */
            if (ctx->active && ctx->active->pan_file_window) {
              /* Force file window refresh */
              DisplayFileWindow(ctx, ctx->active, dir_entry);
            }
            RefreshView(ctx, dir_entry);
          } else if (mk_result == 1) {
            MESSAGE(ctx, "File already exists!");
          } else {
            MESSAGE(ctx, "Can't create File*\"%s\"", file_name);
          }
        }
      }
      need_dsp_help = TRUE;
      break;

    case ACTION_CMD_M: {
      char dir_name[PATH_LENGTH * 2 + 1];
      ClearHelp(ctx);
      *dir_name = '\0';
      if (UI_ReadString(ctx, ctx->active, "MAKE DIRECTORY:", dir_name,
                        PATH_LENGTH, HST_FILE) == CR) {
        if (!MakeDirectory(ctx, ctx->active, dir_entry, dir_name, s)) {
          BuildDirEntryList(ctx, ctx->active->vol,
                            &ctx->active->current_dir_entry);
          RefreshView(ctx, dir_entry);
        }
      }
      wmove(ctx->ctx_border_window, ctx->layout.prompt_y, 0);
      wclrtoeol(ctx->ctx_border_window);
      wnoutrefresh(ctx->ctx_border_window);
    }
      need_dsp_help = TRUE;
      break;
    case ACTION_CMD_D:
      if (!DeleteDirectory(ctx, dir_entry, UI_ChoiceResolver)) {
        if (ctx->active->disp_begin_pos + ctx->active->cursor_pos > 0) {
          if (ctx->active->cursor_pos > 0)
            ctx->active->cursor_pos--;
          else
            ctx->active->disp_begin_pos--;
        }
      }
      /* Update regardless of success */
      BuildDirEntryList(ctx, ctx->active->vol, &ctx->active->current_dir_entry);
      dir_entry = ctx->active->vol
                      ->dir_entry_list[ctx->active->disp_begin_pos +
                                       ctx->active->cursor_pos]
                      .dir_entry;
      dir_entry->start_file = 0;
      dir_entry->cursor_pos = -1;

      RefreshView(ctx, dir_entry);
      need_dsp_help = TRUE;
      break;

    case ACTION_CMD_R:
      if (!GetRenameParameter(ctx, dir_entry->name, new_name)) {
        int rename_result = RenameDirectory(ctx, dir_entry, new_name);
        if (!rename_result) {
          /* Rename OK */
          BuildDirEntryList(ctx, ctx->active->vol,
                            &ctx->active->current_dir_entry);
          dir_entry = ctx->active->vol
                          ->dir_entry_list[ctx->active->disp_begin_pos +
                                           ctx->active->cursor_pos]
                          .dir_entry;
        }
        RefreshView(ctx, dir_entry);
      }
      need_dsp_help = TRUE;
      break;
    case ACTION_REFRESH: /* Rescan */
      dir_entry = RefreshTreeSafe(ctx, ctx->active, dir_entry);
      need_dsp_help = TRUE;
      break;
    case ACTION_CMD_G:
      HandleShowAll(ctx, FALSE, TRUE, dir_entry, &need_dsp_help, &ch,
                    ctx->active);
      break;
    case ACTION_CMD_O:
      UI_Beep(ctx, FALSE);
      break;
    case ACTION_CMD_A:
      if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
        UI_Beep(ctx, FALSE);
        break;
      }
      need_dsp_help = TRUE;
      switch (UI_PromptAttributeAction(ctx, FALSE, TRUE)) {
      case 'M':
        if (ChangeDirModus(ctx, dir_entry))
          break;
        DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                    ctx->active->disp_begin_pos,
                    ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                    TRUE);
        DisplayDiskStatistic(ctx, s);
        UpdateStatsPanel(ctx, dir_entry, s);
        break;
      case 'O':
        if (HandleDirOwnership(ctx, dir_entry, TRUE, FALSE))
          break;
        DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                    ctx->active->disp_begin_pos,
                    ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                    TRUE);
        DisplayDiskStatistic(ctx, s);
        UpdateStatsPanel(ctx, dir_entry, s);
        break;
      case 'G':
        if (HandleDirOwnership(ctx, dir_entry, FALSE, TRUE))
          break;
        DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                    ctx->active->disp_begin_pos,
                    ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                    TRUE);
        DisplayDiskStatistic(ctx, s);
        UpdateStatsPanel(ctx, dir_entry, s);
        break;
      case 'D':
        if (ChangeDirDate(ctx, dir_entry))
          break;
        DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                    ctx->active->disp_begin_pos,
                    ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                    TRUE);
        DisplayDiskStatistic(ctx, s);
        UpdateStatsPanel(ctx, dir_entry, s);
        break;
      default:
        break;
      }
      break;

    case ACTION_TOGGLE_COMPACT:
      ctx->fixed_col_width = (ctx->fixed_col_width == 0) ? 32 : 0;
      ctx->resize_request = TRUE;
      break;

    case ACTION_CMD_P: /* Pipe Directory */
    {
      char pipe_cmd[PATH_LENGTH + 1];
      pipe_cmd[0] = '\0';
      if (GetPipeCommand(ctx, pipe_cmd) == 0) {
        PipeDirectory(ctx, dir_entry, pipe_cmd);
      }
    }
      need_dsp_help = TRUE;
      break;

    /* Volume Cycling and Selection */
    case ACTION_VOL_MENU: /* Shift-K: Select Loaded Volume */
    {
      /* Save current panel state to volume before switching away */
      /* Panel state isolation: No vol_stats sync */
      ctx->active->vol->saved_tree_index =
          ctx->active->disp_begin_pos + ctx->active->cursor_pos;

      int res = SelectLoadedVolume(ctx, NULL);
      if (res == 0) { /* If volume switch was successful */
        if (ctx->active->vol != start_vol)
          return ESC; /* Abort to main loop to handle clean re-entry */

        /* Update loop variables for new volume */
        s = &ctx->active->vol->vol_stats; /* UPDATE S */
        /* Panel isolation: No vol_stats sync */
        ctx->active->disp_begin_pos =
            ctx->active->vol->saved_tree_index /* legacy removed */;

        /* Safety check / Clamping */
        if (ctx->active->vol->total_dirs > 0) {
          /* If saved position is beyond current end, clamp to last item */
          if (ctx->active->disp_begin_pos + ctx->active->cursor_pos >=
              ctx->active->vol->total_dirs) {
            /* Clamp to last valid index */
            int last_idx = ctx->active->vol->total_dirs - 1;
            /* Determine new disp_begin_pos and cursor_pos */
            if (last_idx >= ctx->layout.dir_win_height) {
              ctx->active->disp_begin_pos =
                  last_idx - (ctx->layout.dir_win_height - 1);
              ctx->active->cursor_pos = ctx->layout.dir_win_height - 1;
            } else {
              ctx->active->disp_begin_pos = 0;
              ctx->active->cursor_pos = last_idx;
            }
          }
          /* Now safe to assign dir_entry */
          dir_entry = ctx->active->vol
                          ->dir_entry_list[ctx->active->disp_begin_pos +
                                           ctx->active->cursor_pos]
                          .dir_entry;
        } else {
          dir_entry = s->tree;
        }

        DisplayMenu(ctx); /* Force redraw of frame/separator */
        DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                    ctx->active->disp_begin_pos,
                    ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                    TRUE);
        DisplayFileWindow(
            ctx, ctx->active,
            dir_entry); /* Refresh file window for the new directory */
        RefreshWindow(ctx->ctx_file_window);
        DisplayDiskStatistic(ctx, s);
        UpdateStatsPanel(ctx, dir_entry, s);
        DisplayAvailBytes(ctx, s);
        /* Update header path after volume switch */
        {
          char path[PATH_LENGTH];
          GetPath(dir_entry, path);
          DisplayHeaderPath(ctx, path);
        }
        need_dsp_help = TRUE;
      } else {
        /* Cancelled volume menu clears footer/help; redraw immediately. */
        DisplayMenu(ctx);
        need_dsp_help = TRUE;
      }
    } break;

    case ACTION_VOL_PREV: /* Previous Volume */
    {
      /* Save current panel state to volume before switching away */
      /* Panel isolation: No vol_stats sync */
      ctx->active->vol->saved_tree_index =
          ctx->active->disp_begin_pos + ctx->active->cursor_pos;

      int res = CycleLoadedVolume(ctx, ctx->active, -1);
      if (res == 0) { /* If volume switch was successful */
        if (ctx->active->vol != start_vol)
          return ESC;

        s = &ctx->active->vol->vol_stats; /* UPDATE S */
        /* Panel isolation: No vol_stats sync */
        ctx->active->disp_begin_pos =
            ctx->active->vol->saved_tree_index /* legacy removed */;

        /* Safety check / Clamping */
        if (ctx->active->vol->total_dirs > 0) {
          if (ctx->active->disp_begin_pos + ctx->active->cursor_pos >=
              ctx->active->vol->total_dirs) {
            int last_idx = ctx->active->vol->total_dirs - 1;
            if (last_idx >= ctx->layout.dir_win_height) {
              ctx->active->disp_begin_pos =
                  last_idx - (ctx->layout.dir_win_height - 1);
              ctx->active->cursor_pos = ctx->layout.dir_win_height - 1;
            } else {
              ctx->active->disp_begin_pos = 0;
              ctx->active->cursor_pos = last_idx;
            }
          }
          dir_entry = ctx->active->vol
                          ->dir_entry_list[ctx->active->disp_begin_pos +
                                           ctx->active->cursor_pos]
                          .dir_entry;
        } else {
          dir_entry = s->tree;
        }

        DisplayMenu(ctx);
        DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                    ctx->active->disp_begin_pos,
                    ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                    TRUE);
        DisplayFileWindow(ctx, ctx->active, dir_entry);
        RefreshWindow(ctx->ctx_file_window);
        DisplayDiskStatistic(ctx, s);
        UpdateStatsPanel(ctx, dir_entry, s);
        DisplayAvailBytes(ctx, s);
        {
          char path[PATH_LENGTH];
          GetPath(dir_entry, path);
          DisplayHeaderPath(ctx, path);
        }
        need_dsp_help = TRUE;
      }
    } break;

    case ACTION_VOL_NEXT: /* Next Volume */
    {
      /* Save current panel state to volume before switching away */
      /* Panel isolation: No vol_stats sync */
      ctx->active->vol->saved_tree_index =
          ctx->active->disp_begin_pos + ctx->active->cursor_pos;

      int res = CycleLoadedVolume(ctx, ctx->active, 1);
      if (res == 0) { /* If volume switch was successful */
        if (ctx->active->vol != start_vol)
          return ESC;

        s = &ctx->active->vol->vol_stats; /* UPDATE S */
        /* Panel isolation: No vol_stats sync */
        ctx->active->disp_begin_pos =
            ctx->active->vol->saved_tree_index /* legacy removed */;

        if (ctx->active->vol->total_dirs > 0) {
          if (ctx->active->disp_begin_pos + ctx->active->cursor_pos >=
              ctx->active->vol->total_dirs) {
            int last_idx = ctx->active->vol->total_dirs - 1;
            if (last_idx >= ctx->layout.dir_win_height) {
              ctx->active->disp_begin_pos =
                  last_idx - (ctx->layout.dir_win_height - 1);
              ctx->active->cursor_pos = ctx->layout.dir_win_height - 1;
            } else {
              ctx->active->disp_begin_pos = 0;
              ctx->active->cursor_pos = last_idx;
            }
          }
          dir_entry = ctx->active->vol
                          ->dir_entry_list[ctx->active->disp_begin_pos +
                                           ctx->active->cursor_pos]
                          .dir_entry;
        } else {
          dir_entry = s->tree;
        }

        DisplayMenu(ctx);
        DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                    ctx->active->disp_begin_pos,
                    ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                    TRUE);
        DisplayFileWindow(ctx, ctx->active, dir_entry);
        RefreshWindow(ctx->ctx_file_window);
        DisplayDiskStatistic(ctx, s);
        UpdateStatsPanel(ctx, dir_entry, s);
        DisplayAvailBytes(ctx, s);
        {
          char path[PATH_LENGTH];
          GetPath(dir_entry, path);
          DisplayHeaderPath(ctx, path);
        }
        need_dsp_help = TRUE;
      }
    } break;

    case ACTION_QUIT_DIR:
      need_dsp_help = TRUE;
      QuitTo(ctx, dir_entry);
      break;

    case ACTION_QUIT:
      need_dsp_help = TRUE;
      Quit(ctx);
      action = ACTION_NONE;
      break;

    case ACTION_LOG:
      if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
        if (getcwd(new_log_path, sizeof(new_log_path)) == NULL) {
          (void)snprintf(new_log_path, sizeof(new_log_path), "%s", ".");
        }
      } else {
        (void)GetPath(dir_entry, new_log_path);
      }
      if (!GetNewLogPath(ctx, ctx->active, new_log_path)) {
        int ret; /* DEBUG variable */
        DisplayMenu(ctx);
        doupdate();

        ret = LogDisk(ctx, ctx->active, new_log_path);

        /* Check return value. Only update state if log succeeded (0). */
        if (ret == 0) {
          /* Safety Check: If volume was changed, return to main loop to handle
           * new context */
          if (ctx->active->vol != start_vol)
            return ESC;

          s = &ctx->active->vol
                   ->vol_stats; /* Update stats pointer for new volume */

          /* Safety check / Clamping */
          if (ctx->active->vol->total_dirs > 0) {
            if (ctx->active->disp_begin_pos + ctx->active->cursor_pos >=
                ctx->active->vol->total_dirs) {
              int last_idx = ctx->active->vol->total_dirs - 1;
              if (last_idx >= ctx->layout.dir_win_height) {
                ctx->active->disp_begin_pos =
                    last_idx - (ctx->layout.dir_win_height - 1);
                ctx->active->cursor_pos = ctx->layout.dir_win_height - 1;
              } else {
                ctx->active->disp_begin_pos = 0;
                ctx->active->cursor_pos = last_idx;
              }
            }
            /* Now safe to assign dir_entry */
            dir_entry = ctx->active->vol
                            ->dir_entry_list[ctx->active->disp_begin_pos +
                                             ctx->active->cursor_pos]
                            .dir_entry;
          } else {
            dir_entry = s->tree;
          }

          DisplayMenu(ctx); /* Redraw menu/frame */

          /* Force Full Display Refresh */
          DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window,
                      ctx->active->disp_begin_pos,
                      ctx->active->disp_begin_pos + ctx->active->cursor_pos,
                      TRUE);
          DisplayFileWindow(ctx, ctx->active, dir_entry);
          RefreshWindow(ctx->ctx_file_window);
          DisplayDiskStatistic(ctx, s);
          DisplayDirStatistic(ctx, dir_entry, NULL, s);
          DisplayAvailBytes(ctx, s);
          /* Update header path */
          {
            char path[PATH_LENGTH];
            GetPath(dir_entry, path);
            DisplayHeaderPath(ctx, path);
          }
        }
        need_dsp_help = TRUE;
      }
      break;
    /* Ctrl-L is now ACTION_REFRESH, handled above */
    default: /* Unhandled action, beep */
      UI_Beep(ctx, FALSE);
      break;
    } /* switch */

    /* Refresh dir_entry pointer in case tree was rescanned/reallocated during
     * action */
    if (ctx->active && ctx->active->vol && ctx->active->vol->dir_entry_list) {
      int safe_idx = ctx->active->disp_begin_pos + ctx->active->cursor_pos;
      if (safe_idx < 0)
        safe_idx = 0;
      if (safe_idx >= ctx->active->vol->total_dirs)
        safe_idx = ctx->active->vol->total_dirs - 1;
      if (safe_idx >= 0) {
        dir_entry = ctx->active->vol->dir_entry_list[safe_idx].dir_entry;
      } else {
        dir_entry = ctx->active->vol->vol_stats.tree;
      }
    }

  } while (action != ACTION_QUIT && action != ACTION_ENTER &&
           action != ACTION_ESCAPE &&
           action !=
               ACTION_LOG); /* Loop until explicit quit, escape or log */

  /* Sync state back to Volume on exit */
  /* Removed shared state sync on exit */

  return (ch); /* Return the last raw character that caused exit */
}

static void DirListJump(ViewContext *ctx, DirEntry **dir_entry_ptr,
                        const Statistic *s) {
  char search_buf[256];
  int buf_len = 0;
  int i;
  int found_idx;
  int height;
  int original_disp_begin_pos;
  int original_cursor_pos;
  WINDOW *jump_win = (ctx && ctx->ctx_menu_window) ? ctx->ctx_menu_window : stdscr;

  if (!ctx || !ctx->active || !ctx->active->vol || !ctx->active->vol->dir_entry_list ||
      ctx->active->vol->total_dirs <= 0 || !dir_entry_ptr || !*dir_entry_ptr) {
    return;
  }

  height = getmaxy(ctx->ctx_dir_window);
  if (height <= 0)
    return;

  original_disp_begin_pos = ctx->active->disp_begin_pos;
  original_cursor_pos = ctx->active->cursor_pos;

  memset(search_buf, 0, sizeof(search_buf));

  ClearHelp(ctx);
  DrawDirListJumpPrompt(ctx, jump_win, search_buf);

  while (1) {
    int ch;

    DrawDirListJumpPrompt(ctx, jump_win, search_buf);

    ch = Getch(ctx);
    if (ch == -1)
      break;

    if (ch == ESC) {
      ctx->active->disp_begin_pos = original_disp_begin_pos;
      ctx->active->cursor_pos = original_cursor_pos;
    } else if (ch == CR || ch == LF) {
      break;
    } else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b' || ch == KEY_DC) {
      if (buf_len > 0) {
        buf_len--;
        search_buf[buf_len] = '\0';

        if (buf_len == 0) {
          ctx->active->disp_begin_pos = original_disp_begin_pos;
          ctx->active->cursor_pos = original_cursor_pos;
        } else {
          found_idx = -1;
          for (i = 0; i < ctx->active->vol->total_dirs; i++) {
            DirEntry *candidate = ctx->active->vol->dir_entry_list[i].dir_entry;
            if (candidate && strncasecmp(candidate->name, search_buf, buf_len) == 0) {
              found_idx = i;
              break;
            }
          }

          if (found_idx != -1) {
            if (found_idx >= ctx->active->disp_begin_pos &&
                found_idx < ctx->active->disp_begin_pos + height) {
              ctx->active->cursor_pos = found_idx - ctx->active->disp_begin_pos;
            } else {
              ctx->active->disp_begin_pos = found_idx;
              ctx->active->cursor_pos = 0;
              if (ctx->active->disp_begin_pos + height > ctx->active->vol->total_dirs) {
                ctx->active->disp_begin_pos =
                    MAXIMUM(0, ctx->active->vol->total_dirs - height);
                ctx->active->cursor_pos = found_idx - ctx->active->disp_begin_pos;
              }
            }
          }
        }
      }
    } else if (isprint(ch)) {
      if (buf_len < (int)sizeof(search_buf) - 1) {
        search_buf[buf_len] = ch;
        search_buf[buf_len + 1] = '\0';

        found_idx = -1;
        for (i = 0; i < ctx->active->vol->total_dirs; i++) {
          DirEntry *candidate = ctx->active->vol->dir_entry_list[i].dir_entry;
          if (candidate &&
              strncasecmp(candidate->name, search_buf, (size_t)buf_len + 1) == 0) {
            found_idx = i;
            break;
          }
        }

        if (found_idx != -1) {
          buf_len++;
          if (found_idx >= ctx->active->disp_begin_pos &&
              found_idx < ctx->active->disp_begin_pos + height) {
            ctx->active->cursor_pos = found_idx - ctx->active->disp_begin_pos;
          } else {
            ctx->active->disp_begin_pos = found_idx;
            ctx->active->cursor_pos = 0;
            if (ctx->active->disp_begin_pos + height > ctx->active->vol->total_dirs) {
              ctx->active->disp_begin_pos =
                  MAXIMUM(0, ctx->active->vol->total_dirs - height);
              ctx->active->cursor_pos = found_idx - ctx->active->disp_begin_pos;
            }
          }
        } else {
          /* Sticky cursor: keep current selection when input has no match. */
          buf_len++;
        }
      }
    }

    if (ctx->active->cursor_pos < 0)
      ctx->active->cursor_pos = 0;

    if (ctx->active->disp_begin_pos + ctx->active->cursor_pos >=
        ctx->active->vol->total_dirs) {
      int last_idx = ctx->active->vol->total_dirs - 1;
      if (last_idx < 0)
        last_idx = 0;
      if (last_idx >= height) {
        ctx->active->disp_begin_pos = last_idx - (height - 1);
        ctx->active->cursor_pos = height - 1;
      } else {
        ctx->active->disp_begin_pos = 0;
        ctx->active->cursor_pos = last_idx;
      }
    }

    *dir_entry_ptr = ctx->active
                         ->vol->dir_entry_list[ctx->active->disp_begin_pos +
                                               ctx->active->cursor_pos]
                         .dir_entry;

    DisplayTree(ctx, ctx->active->vol, ctx->ctx_dir_window, ctx->active->disp_begin_pos,
                ctx->active->disp_begin_pos + ctx->active->cursor_pos, TRUE);
    DisplayFileWindow(ctx, ctx->active, *dir_entry_ptr);
    DisplayDiskStatistic(ctx, s);
    UpdateStatsPanel(ctx, *dir_entry_ptr, s);
    DisplayAvailBytes(ctx, s);
    {
      char path[PATH_LENGTH];
      GetPath(*dir_entry_ptr, path);
      DisplayHeaderPath(ctx, path);
    }
    RefreshWindow(ctx->ctx_dir_window);
    RefreshWindow(ctx->ctx_file_window);
    doupdate();

    if (ch == ESC)
      break;
  }
}

static void DrawDirListJumpPrompt(ViewContext *ctx, WINDOW *win,
                                  const char *search_buf) {
  int y = 0;

  if (!ctx || !win || !search_buf)
    return;

  if (win == stdscr) {
    if (ctx->layout.prompt_y > 0) {
      wmove(win, ctx->layout.prompt_y - 1, 0);
      wclrtoeol(win);
    }
    wmove(win, ctx->layout.prompt_y, 0);
    wclrtoeol(win);
    wmove(win, ctx->layout.status_y, 0);
    wclrtoeol(win);
    y = ctx->layout.prompt_y;
  } else {
    werase(win);
    y = 1; /* Center line in 3-line footer window */
  }

  mvwaddstr(win, y, 1, "Jump to: ");
  mvwaddstr(win, y, 10, search_buf);
  wclrtoeol(win);
  wnoutrefresh(win);
  doupdate();
}
