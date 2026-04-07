/***************************************************************************
 *
 * src/ui/file_compare.c
 * File Compare Execution
 *
 ***************************************************************************/

#include "ytree_cmd.h"
#include "ytree_fs.h"
#include "ytree_ui.h"
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static int ResolveFileCompareTargetPath(FileEntry *source_file,
                                        const char *target_input,
                                        char *resolved_target_path) {
  char source_dir[PATH_LENGTH + 1];
  char raw_target_path[(PATH_LENGTH * 2) + 2];
  const char *home = NULL;
  int written;

  if (!source_file || !source_file->dir_entry || !target_input ||
      !resolved_target_path) {
    return -1;
  }

  if (!String_HasNonWhitespace(target_input))
    return -1;

  GetPath(source_file->dir_entry, source_dir);
  source_dir[PATH_LENGTH] = '\0';

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
                       source_dir, FILE_SEPARATOR_CHAR, target_input);
  }

  if (written < 0 || (size_t)written >= sizeof(raw_target_path))
    return -1;

  NormPath(raw_target_path, resolved_target_path);
  resolved_target_path[PATH_LENGTH] = '\0';
  return 0;
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

void FileCompare_LaunchExternal(ViewContext *ctx, FileEntry *source_file) {
  CompareRequest request;
  struct stat source_stat;
  struct stat target_stat;
  char source_path[PATH_LENGTH + 1];
  char target_path[PATH_LENGTH + 1];
  char source_dir[PATH_LENGTH + 1];
  char command_line[COMMAND_LINE_LENGTH + 1];
  const char *filediff = NULL;
  int start_dir_fd = -1;
  int result = -1;

  if (!ctx || !source_file)
    return;

  if (ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) {
    UI_Message(ctx, "File compare is not supported in this view mode.*"
                    "Use disk/user mode.");
    return;
  }
  if (!source_file->dir_entry || source_file->dir_entry->global_flag) {
    UI_Message(ctx, "File compare is not supported in this file context.*"
                    "Use a normal file list entry.");
    return;
  }

  if (UI_BuildFileCompareRequest(ctx, source_file, &request) != 0)
    return;

  filediff = UI_GetCompareHelperCommand(ctx, COMPARE_FLOW_FILE);
  if (!String_HasNonWhitespace(filediff)) {
    UI_Message(ctx,
               "FILEDIFF helper is not configured.*Set FILEDIFF in ~/.ytree "
               "(or .ytree).");
    return;
  }

  strncpy(source_path, request.source_path, PATH_LENGTH);
  source_path[PATH_LENGTH] = '\0';
  if (ResolveFileCompareTargetPath(source_file, request.target_path,
                                   target_path) != 0) {
    UI_Message(ctx,
               "Compare target is empty or invalid.*Choose a file target.");
    return;
  }

  if (stat(source_path, &source_stat) != 0) {
    UI_Message(ctx, "Compare source is not accessible:*\"%s\"*%s", source_path,
               strerror(errno));
    return;
  }
  if (stat(target_path, &target_stat) != 0) {
    UI_Message(ctx,
               "Compare target does not exist:*\"%s\"*Select a valid file.",
               target_path);
    return;
  }
  if (S_ISDIR(source_stat.st_mode) || S_ISDIR(target_stat.st_mode)) {
    UI_Message(
        ctx,
        "File compare requires two files.*Directory targets are unsupported.");
    return;
  }
  if ((source_stat.st_dev == target_stat.st_dev &&
       source_stat.st_ino == target_stat.st_ino) ||
      !strcmp(source_path, target_path)) {
    UI_Message(ctx, "Can't compare: source and target are the same file.");
    return;
  }

  if (Path_BuildCompareCommandLine(filediff, source_path, target_path,
                                   command_line, sizeof(command_line)) != 0) {
    UI_Message(
        ctx,
        "FILEDIFF command is invalid or too long.*Check FILEDIFF in ~/.ytree.");
    return;
  }

  GetPath(source_file->dir_entry, source_dir);
  source_dir[PATH_LENGTH] = '\0';

  start_dir_fd = open(".", O_RDONLY);
  if (start_dir_fd == -1) {
    UI_Message(ctx, "Error preparing compare launch:*%s", strerror(errno));
    return;
  }
  if (chdir(source_dir) != 0) {
    UI_Message(ctx, "Can't change directory to*\"%s\"*%s", source_dir,
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
    UI_Message(
        ctx, "Failed to launch FILEDIFF helper.*Install/configure FILEDIFF in "
             "~/.ytree.");
    return;
  }

  if (WIFEXITED(result)) {
    int exit_status = WEXITSTATUS(result);
    if (exit_status == 126 || exit_status == 127) {
      char command_name[PATH_LENGTH + 1];
      String_GetCommandDisplayName(filediff, command_name,
                                   sizeof(command_name));
      UI_Message(ctx,
                 "FILEDIFF helper not available:*\"%s\"*"
                 "Install it or update FILEDIFF in ~/.ytree.",
                 command_name[0] ? command_name : filediff);
    }
  }
}
