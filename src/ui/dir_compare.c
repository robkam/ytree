/***************************************************************************
 *
 * src/ui/dir_compare.c
 * Directory Compare Execution
 *
 ***************************************************************************/

#include "ytree_cmd.h"
#include "ytree_fs.h"
#include "ytree_ui.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

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

static int ResolveDirectoryCompareTargetPath(const char *source_path,
                                             const char *target_input,
                                             char *resolved_target_path) {
  char raw_target_path[(PATH_LENGTH * 2) + 2];
  const char *home = NULL;
  int written;

  if (!source_path || !target_input || !resolved_target_path)
    return -1;
  if (!String_HasNonWhitespace(target_input))
    return -1;

  if (target_input[0] == FILE_SEPARATOR_CHAR) {
    written =
        snprintf(raw_target_path, sizeof(raw_target_path), "%s", target_input);
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

static void TagComparedFile(YtreePanel *panel, FileEntry *source_file,
                            Statistic *stats, unsigned long *tagged_count) {
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
  PanelTags_RecordFileState(panel, source_file, TRUE);
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
    if (source_entry_path[0] != FILE_SEPARATOR_CHAR ||
        source_entry_path[1] == '\0')
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
    written = snprintf(target_entry_path, target_entry_path_size, "/%s",
                       relative_path);
  } else {
    written = snprintf(target_entry_path, target_entry_path_size, "%s%c%s",
                       target_root_path, FILE_SEPARATOR_CHAR, relative_path);
  }

  if (written < 0 || (size_t)written >= target_entry_path_size)
    return -1;

  return 0;
}

void DirCompare_RunInternalDirectory(ViewContext *ctx, DirEntry *source_dir,
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
    UI_Message(ctx,
               "Compare target is empty or invalid.*Choose a target path.");
    return;
  }

  if (STAT_(source_path, &source_dir_stat) != 0) {
    UI_Message(ctx, "Compare source is not accessible:*\"%s\"*%s", source_path,
               strerror(errno));
    return;
  }
  if (STAT_(target_path, &target_dir_stat) != 0) {
    UI_Message(
        ctx, "Compare target does not exist:*\"%s\"*Select a valid directory.",
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

  for (source_file = source_dir->file; source_file;
       source_file = source_file->next) {
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

    written = snprintf(target_entry_path, sizeof(target_entry_path), "%s%c%s",
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
      } else if (EvaluateDirectoryCompareBasis(
                     request->basis, &source_stat, source_entry_path,
                     &target_stat, target_entry_path, &basis_match) == 0) {
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
      TagComparedFile(ctx->active, source_file, stats, &summary.tagged_count);
    }
  }

  UI_Message(
      ctx,
      "Directory compare complete.*SOURCE: %s*TARGET: %s*BASIS: %s"
      "*SOURCE ENTRIES: %lu"
      "*RESULT COUNTS: different=%lu match=%lu newer=%lu older=%lu unique=%lu "
      "type-mismatch=%lu error=%lu"
      "*TAGGED (%s): %lu file(s) in active/source list.",
      Path_LeafName(source_path), Path_LeafName(target_path),
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

  for (source_file = source_dir->file; source_file;
       source_file = source_file->next) {
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
      } else if (EvaluateDirectoryCompareBasis(
                     request->basis, &source_stat, source_entry_path,
                     &target_stat, target_entry_path, &basis_match) == 0) {
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
      TagComparedFile(NULL, source_file, stats, &summary->tagged_count);
    }
  }

  for (child_dir = source_dir->sub_tree; child_dir;
       child_dir = child_dir->next) {
    RunInternalLoggedTreeCompareRecursive(
        child_dir, source_root_path, target_root_path, request, summary, stats);
  }
}

void DirCompare_RunInternalLoggedTree(ViewContext *ctx,
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
    UI_Message(ctx,
               "Compare target is empty or invalid.*Choose a target path.");
    return;
  }

  if (STAT_(source_path, &source_dir_stat) != 0) {
    UI_Message(ctx, "Compare source is not accessible:*\"%s\"*%s", source_path,
               strerror(errno));
    return;
  }
  if (STAT_(target_path, &target_dir_stat) != 0) {
    UI_Message(
        ctx, "Compare target does not exist:*\"%s\"*Select a valid directory.",
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
      Path_LeafName(source_path), Path_LeafName(target_path),
      UI_CompareBasisName(request->basis), summary.source_entries,
      summary.different_count, summary.match_count, summary.newer_count,
      summary.older_count, summary.unique_count, summary.type_mismatch_count,
      summary.error_count, summary.skipped_unlogged_source_dirs,
      UI_CompareTagResultName(request->tag_result), summary.tagged_count);
}

void DirCompare_LaunchExternal(ViewContext *ctx, DirEntry *source_dir,
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
  helper_key =
      (flow_type == COMPARE_FLOW_LOGGED_TREE) ? "TREEDIFF/DIRDIFF" : "DIRDIFF";
  if (!String_HasNonWhitespace(helper)) {
    UI_Message(ctx, "%s helper is not configured.*Set %s in ~/.ytree.",
               UI_CompareFlowTypeName(flow_type), helper_key);
    return;
  }

  strncpy(source_path, request.source_path, PATH_LENGTH);
  source_path[PATH_LENGTH] = '\0';
  if (ResolveDirectoryCompareTargetPath(source_path, request.target_path,
                                        target_path) != 0) {
    UI_Message(ctx,
               "Compare target is empty or invalid.*Choose a target path.");
    return;
  }

  if (stat(source_path, &source_stat) != 0) {
    UI_Message(ctx, "Compare source is not accessible:*\"%s\"*%s", source_path,
               strerror(errno));
    return;
  }
  if (stat(target_path, &target_stat) != 0) {
    UI_Message(
        ctx, "Compare target does not exist:*\"%s\"*Select a valid directory.",
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

  if (Path_BuildCompareCommandLine(helper, source_path, target_path,
                                   command_line, sizeof(command_line)) != 0) {
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
      String_GetCommandDisplayName(helper, command_name, sizeof(command_name));
      UI_Message(ctx,
                 "Compare helper not available:*\"%s\"*"
                 "Install it or update %s in ~/.ytree.",
                 command_name[0] ? command_name : helper, helper_key);
    }
  }
}
