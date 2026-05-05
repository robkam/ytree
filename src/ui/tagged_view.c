/***************************************************************************
 *
 * src/ui/tagged_view.c
 * Tagged-file viewing and extraction flow extracted from interactions.c.
 *
 ***************************************************************************/

#include "ytree_cmd.h"
#include "ytree_fs.h"
#include "ytree_ui.h"
#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static BOOL CopyBoundedStringChecked(char *dst, size_t dst_size,
                                     const char *src) {
  int written;

  if (!dst || dst_size == 0)
    return FALSE;

  if (!src)
    src = "";

  written = snprintf(dst, dst_size, "%s", src);
  if (written < 0) {
    dst[0] = '\0';
    return FALSE;
  }
  if ((size_t)written >= dst_size) {
    dst[dst_size - 1] = '\0';
    return FALSE;
  }
  return TRUE;
}

int recursive_mkdir(char *path) {
  char *p = path;
  if (*p == '/')
    p++;
  while (*p) {
    if (*p == '/') {
      *p = '\0';
      if (mkdir(path, 0700) != 0 && errno != EEXIST)
        return -1;
      *p = '/';
    }
    p++;
  }
  if (mkdir(path, 0700) != 0 && errno != EEXIST)
    return -1;
  return 0;
}

int recursive_rmdir(const char *path) {
  DIR *d = opendir(path);
  const struct dirent *entry;
  char fullpath[PATH_LENGTH];
  int written;

  if (!d)
    return -1;

  while ((entry = readdir(d))) {
    if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
      continue;
    written = snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
    if (written < 0 || (size_t)written >= sizeof(fullpath)) {
      closedir(d);
      errno = ENAMETOOLONG;
      return -1;
    }

    if (recursive_rmdir(fullpath) != 0) {
      if (errno == ENOTDIR || errno == EINVAL) {
        if (unlink(fullpath) != 0) {
          closedir(d);
          return -1;
        }
      } else {
        closedir(d);
        return -1;
      }
    }
  }
  closedir(d);
  return rmdir(path);
}

static void SetupTaggedViewWindow(ViewContext *ctx) {
  int start_y;
  int start_x;
  int available_height;
  int width;

  start_y = ctx->layout.dir_win_y;
  start_x = ctx->layout.dir_win_x;
  available_height = ctx->layout.message_y - start_y;
  width = ctx->layout.main_win_width;

  if (available_height < 3)
    available_height = 3;
  if (width < 20)
    width = 20;

  ctx->viewer.wlines = available_height - 2;
  ctx->viewer.wcols = width - 2;

  if (ctx->viewer.border) {
    delwin(ctx->viewer.border);
    ctx->viewer.border = NULL;
  }
  if (ctx->viewer.view) {
    delwin(ctx->viewer.view);
    ctx->viewer.view = NULL;
  }

  ctx->viewer.border = newwin(available_height, width, start_y, start_x);
  ctx->viewer.view =
      newwin(ctx->viewer.wlines, ctx->viewer.wcols, start_y + 1, start_x + 1);

  keypad(ctx->viewer.view, TRUE);
  scrollok(ctx->viewer.view, FALSE);
  clearok(ctx->viewer.view, TRUE);
  leaveok(ctx->viewer.view, FALSE);

#ifdef COLOR_SUPPORT
  WbkgdSet(ctx, ctx->viewer.view, COLOR_PAIR(CPAIR_WINDIR));
  WbkgdSet(ctx, ctx->viewer.border, COLOR_PAIR(CPAIR_WINDIR) | A_BOLD);
#endif
}

static void DestroyTaggedViewWindow(ViewContext *ctx) {
  if (ctx->viewer.view) {
    delwin(ctx->viewer.view);
    ctx->viewer.view = NULL;
  }
  if (ctx->viewer.border) {
    delwin(ctx->viewer.border);
    ctx->viewer.border = NULL;
  }
}

static long TaggedViewPageStep(ViewContext *ctx) {
  long step = (long)ctx->viewer.wlines - 1;
  if (step < 1)
    step = 1;
  return step;
}

static void DrawTaggedViewHeader(ViewContext *ctx, const char *display_path,
                                 int current_idx, int total_count) {
  int i;
  int available;
  char header_buf[PATH_LENGTH + 64];
  char clipped_header[PATH_LENGTH + 64];

  for (i = ctx->layout.header_y; i <= ctx->layout.status_y; i++) {
    wmove(stdscr, i, 0);
    wclrtoeol(stdscr);
  }

  snprintf(header_buf, sizeof(header_buf), "[%d/%d] %s", current_idx + 1,
           total_count, display_path ? display_path : "");
  available = (COLS > 7) ? (COLS - 7) : 1;

  Print(stdscr, ctx->layout.header_y, 0, "File: ", CPAIR_MENU);
  Print(stdscr, ctx->layout.header_y, 6,
        CutPathname(clipped_header, header_buf, available), CPAIR_HIMENUS);

  PrintOptions(stdscr, ctx->layout.message_y, 0,
               "(Q)uit  (Space/PgDn) next page/file  (PgUp) prev page");
  PrintOptions(stdscr, ctx->layout.prompt_y, 0,
               "(N)ext file  (P)rev file (wrap)  (Up/Down) line  (Home/End)");
  PrintOptions(stdscr, ctx->layout.status_y, 0, "View tagged files");

  box(ctx->viewer.border, 0, 0);
  wnoutrefresh(stdscr);
  wnoutrefresh(ctx->viewer.border);
}

static int RunTaggedViewLoop(ViewContext *ctx, char **view_paths,
                             char **display_paths, int path_count) {
  int current_idx = 0;
  int ch;
  BOOL quit = FALSE;
  long line_offset = 0;

  if (!ctx || !view_paths || !display_paths || path_count <= 0)
    return -1;

  SetupTaggedViewWindow(ctx);

  while (!quit) {
    long page_step;

    DrawTaggedViewHeader(ctx, display_paths[current_idx], current_idx,
                         path_count);
    RenderFilePreview(ctx, ctx->viewer.view, view_paths[current_idx],
                      &line_offset, 0);
    RefreshWindow(ctx->viewer.view);
    doupdate();

    ch = (ctx->resize_request) ? -1 : WGetch(ctx, ctx->viewer.view);

    ch = NormalizeViKey(ctx, ch);

    if (ctx->resize_request) {
      ctx->resize_request = FALSE;
      SetupTaggedViewWindow(ctx);
      continue;
    }

    page_step = TaggedViewPageStep(ctx);

    switch (ch) {
#ifdef KEY_RESIZE
    case KEY_RESIZE:
      ctx->resize_request = TRUE;
      break;
#endif
    case ESC:
    case 'q':
    case 'Q':
      quit = TRUE;
      break;
    case 'n':
    case 'N':
      current_idx = (current_idx + 1) % path_count;
      line_offset = 0;
      break;
    case 'p':
    case 'P':
      current_idx = (current_idx + path_count - 1) % path_count;
      line_offset = 0;
      break;
    case KEY_UP:
      if (line_offset > 0)
        line_offset--;
      break;
    case KEY_DOWN:
      if (line_offset < 2000000000L)
        line_offset++;
      break;
    case KEY_HOME:
      line_offset = 0;
      break;
    case KEY_END:
      line_offset = 2000000000L;
      break;
    case KEY_LEFT:
    case KEY_PPAGE:
      if (line_offset > page_step)
        line_offset -= page_step;
      else
        line_offset = 0;
      break;
    case KEY_RIGHT:
    case KEY_NPAGE:
      if (line_offset < 2000000000L - page_step)
        line_offset += page_step;
      else
        line_offset = 2000000000L;
      break;
    case ' ': {
      long old_offset = line_offset;
      long probe_offset = line_offset;

      if (probe_offset < 2000000000L - page_step)
        probe_offset += page_step;
      else
        probe_offset = 2000000000L;

      RenderFilePreview(ctx, ctx->viewer.view, view_paths[current_idx],
                        &probe_offset, 0);
      if (probe_offset == old_offset) {
        if (current_idx < path_count - 1) {
          current_idx++;
          line_offset = 0;
        }
      } else {
        line_offset = probe_offset;
      }
      break;
    }
    case 'L' & 0x1f:
      clearok(stdscr, TRUE);
      break;
    default:
      break;
    }
  }

  DestroyTaggedViewWindow(ctx);
  touchwin(stdscr);
  wnoutrefresh(stdscr);
  doupdate();
  return 0;
}

static int RunExternalTaggedViewer(ViewContext *ctx, char **view_paths,
                                   int path_count, Statistic *s) {
  const char *viewer;
  char *command_line;
  int i;
  size_t current_len;

  if (!ctx || !view_paths || path_count <= 0 || !s)
    return -1;

  viewer = (GetProfileValue)(ctx, "PAGER");
  if (!viewer || !*viewer)
    viewer = "less";

  command_line = (char *)xmalloc(COMMAND_LINE_LENGTH + 1);
  if (!Path_CommandInit(command_line, COMMAND_LINE_LENGTH + 1, &current_len,
                        viewer)) {
    free(command_line);
    return -1;
  }

  if (ctx->global_search_term[0] != '\0') {
    if (!Path_CommandAppendLiteral(command_line, COMMAND_LINE_LENGTH + 1,
                                   &current_len, " -p ") ||
        !Path_CommandAppendQuotedArg(command_line, COMMAND_LINE_LENGTH + 1,
                                     &current_len, ctx->global_search_term)) {
      UI_Warning(ctx, "Search pattern too long.");
      free(command_line);
      return -1;
    }
  }

  for (i = 0; i < path_count; i++) {
    if (!Path_CommandAppendLiteral(command_line, COMMAND_LINE_LENGTH + 1,
                                   &current_len, " ") ||
        !Path_CommandAppendQuotedArg(command_line, COMMAND_LINE_LENGTH + 1,
                                     &current_len, view_paths[i])) {
      UI_Warning(ctx, "Tagged viewer command too long.");
      free(command_line);
      return -1;
    }
  }

  (void)SystemCall(ctx, command_line, s);
  free(command_line);
  return 0;
}

int UI_ViewTaggedFiles(ViewContext *ctx, DirEntry *dir_entry) {
  FileEntry *fe;
  const char *temp_dir = NULL;
  char **view_paths = NULL;
  char **display_paths = NULL;
  char path[PATH_LENGTH + 1];
  int tagged_count = 0;
  int path_count = 0;
  BOOL use_internal_view = FALSE;
  int i;
  char temp_dir_template[PATH_LENGTH];
  Statistic *s;
  const char *tagged_viewer;
  int result = 0;

  (void)dir_entry;
  if (!ctx || !ctx->active || !ctx->active->vol ||
      !ctx->active->file_entry_list)
    return 0;

  s = &ctx->active->vol->vol_stats;
  tagged_viewer = GetProfileValue(ctx, "TAGGEDVIEWER");
  if (tagged_viewer &&
      (!strcasecmp(tagged_viewer, "internal") ||
       !strcasecmp(tagged_viewer, "builtin") ||
       !strcasecmp(tagged_viewer, "ytree") || !strcmp(tagged_viewer, "1"))) {
    use_internal_view = TRUE;
  }

  if (s->log_mode == ARCHIVE_MODE) {
    if (!Path_BuildTempTemplate(temp_dir_template, sizeof(temp_dir_template),
                                "ytree_view_")) {
      UI_Error(ctx, __FILE__, __LINE__,
               "Could not prepare temp dir template for viewing");
      return -1;
    }
    temp_dir = mkdtemp(temp_dir_template);
    if (!temp_dir) {
      UI_Error(ctx, __FILE__, __LINE__,
               "Could not create temp dir for viewing");
      return -1;
    }
  }

  for (i = 0; i < (int)ctx->active->file_count; i++) {
    fe = ctx->active->file_entry_list[i].file;
    if (fe->tagged && fe->matching)
      tagged_count++;
  }

  if (tagged_count <= 0) {
    goto cleanup;
  }

  view_paths = (char **)xcalloc((size_t)tagged_count, sizeof(char *));
  display_paths = (char **)xcalloc((size_t)tagged_count, sizeof(char *));
  if (!view_paths || !display_paths) {
    UI_Error(ctx, __FILE__, __LINE__,
             "Out of memory while preparing tagged view");
    result = -1;
    goto cleanup;
  }

  for (i = 0; i < (int)ctx->active->file_count; i++) {
    fe = ctx->active->file_entry_list[i].file;

    if (!(fe->tagged && fe->matching))
      continue;

    if (s->log_mode == DISK_MODE || s->log_mode == USER_MODE) {
      GetFileNamePath(fe, path);
      view_paths[path_count] = xstrdup(path);
      display_paths[path_count] = xstrdup(path);
      path_count++;
    } else if (s->log_mode == ARCHIVE_MODE) {
#ifdef HAVE_LIBARCHIVE
      if (path_count >= 100) {
        UI_Warning(ctx, "Archive view limit (100) reached.");
        break;
      }

      {
        char t_filename[PATH_LENGTH];
        char t_dirname[PATH_LENGTH];
        char root_path[PATH_LENGTH + 1];
        char relative_path[PATH_LENGTH + 1];
        char internal_path[PATH_LENGTH + 1];
        char canonical_internal_path[PATH_LENGTH + 1];
        char full_path[PATH_LENGTH + 1];
        size_t root_len;

        GetPath(ctx->active->vol->vol_stats.tree, root_path);
        GetFileNamePath(fe, full_path);

        root_len = strlen(root_path);
        if (strncmp(full_path, root_path, root_len) == 0) {
          char *ptr = full_path + root_len;
          if (*ptr == FILE_SEPARATOR_CHAR) {
            ptr++;
            if (!CopyBoundedStringChecked(relative_path, sizeof(relative_path),
                                          ptr)) {
              UI_Warning(ctx, "Skipped long archive path*\"%s\"", fe->name);
              continue;
            }
          } else if (*ptr == '\0') {
            relative_path[0] = '\0';
          } else {
            if (!CopyBoundedStringChecked(relative_path, sizeof(relative_path),
                                          full_path)) {
              UI_Warning(ctx, "Skipped long archive path*\"%s\"", fe->name);
              continue;
            }
          }
        } else {
          if (!CopyBoundedStringChecked(relative_path, sizeof(relative_path),
                                        full_path)) {
            UI_Warning(ctx, "Skipped long archive path*\"%s\"", fe->name);
            continue;
          }
        }

        if (strlen(relative_path) == 0) {
          if (!CopyBoundedStringChecked(relative_path, sizeof(relative_path),
                                        fe->name)) {
            UI_Warning(ctx, "Skipped long archive path*\"%s\"", fe->name);
            continue;
          }
        }

        if (!CopyBoundedStringChecked(internal_path, sizeof(internal_path),
                                      relative_path)) {
          UI_Warning(ctx, "Skipped long archive path*\"%s\"", fe->name);
          continue;
        }
        if (Archive_ValidateInternalPath(internal_path, canonical_internal_path,
                                         sizeof(canonical_internal_path)) != 0) {
          UI_Warning(ctx, "Skipped unsafe archive member path*\"%s\"",
                     internal_path);
          continue;
        }

        if (strlen(canonical_internal_path) > 0) {
          char *dir_only;

          snprintf(t_dirname, sizeof(t_dirname), "%s/%s", temp_dir,
                   canonical_internal_path);
          dir_only = xstrdup(t_dirname);
          dirname(dir_only);
          if (recursive_mkdir(dir_only) != 0) {
            UI_Warning(ctx, "Failed to prepare temp path*\"%s\"",
                       canonical_internal_path);
            free(dir_only);
            continue;
          }
          free(dir_only);
          snprintf(t_filename, sizeof(t_filename), "%s/%s", temp_dir,
                   canonical_internal_path);
        } else {
          snprintf(t_filename, sizeof(t_filename), "%s/%s", temp_dir, fe->name);
        }

        if (ExtractArchiveNode(s->log_path, canonical_internal_path, t_filename,
                               UI_ArchiveCallback, ctx) == 0) {
          view_paths[path_count] = xstrdup(t_filename);
          display_paths[path_count] = xstrdup(canonical_internal_path);
          path_count++;
        } else {
          UI_Warning(ctx, "Failed to extract*\"%s\"", canonical_internal_path);
        }
      }
#else
      if (path_count == 0) {
        UI_Message(ctx, "Archive view requires libarchive support.");
      }
#endif
    }
  }

  if (path_count > 0) {
    if (use_internal_view) {
      RunTaggedViewLoop(ctx, view_paths, display_paths, path_count);
    } else {
      RunExternalTaggedViewer(ctx, view_paths, path_count, s);
    }
  } else if (s->log_mode == ARCHIVE_MODE) {
    UI_Message(ctx, "No files extracted.");
  }

cleanup:
  for (i = 0; i < tagged_count; i++) {
    if (view_paths && view_paths[i])
      free(view_paths[i]);
    if (display_paths && display_paths[i])
      free(display_paths[i]);
  }
  free(view_paths);
  free(display_paths);

  if (s->log_mode == ARCHIVE_MODE && temp_dir) {
    recursive_rmdir(temp_dir);
  }

  return result;
}
