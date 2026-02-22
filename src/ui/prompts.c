/***************************************************************************
 *
 * src/ui/prompts.c
 * UI Prompts and Interaction Functions
 *
 ***************************************************************************/

#include "sort.h"
#include "watcher.h"
#include "ytree.h"
#include "ytree_ui.h"
#include <dirent.h>
#include <fcntl.h>
#include <grp.h>
#include <libgen.h>
#include <pwd.h>
#include <sys/stat.h>
#include <unistd.h>

static char move_prompt_header[PATH_LENGTH + 50];
static char move_prompt_as[PATH_LENGTH + 1];

int UI_ConflictResolverWrapper(const char *src_path, const char *dst_path,
                               int *mode_flags) {
  return UI_AskConflict(src_path, dst_path, mode_flags);
}

int UI_ChoiceResolver(const char *prompt, const char *choices) {
  return InputChoice(prompt, choices);
}

int GetMoveParameter(char *from_file, char *to_file, char *to_dir) {
  if (from_file == NULL) {
    from_file = "TAGGED FILES";
    (void)strcpy(to_file, "*");
  } else {
    (void)strcpy(to_file, from_file);
  }

  (void)snprintf(move_prompt_header, sizeof(move_prompt_header), "MOVE: %s",
                 from_file);

  ClearHelp();

  if (UI_ReadString(move_prompt_header, to_file, PATH_LENGTH, HST_FILE) == CR) {

    strncpy(move_prompt_as, to_file, PATH_LENGTH);
    move_prompt_as[PATH_LENGTH] = '\0';

    if (IsSplitScreen && ActivePanel) {
      YtreePanel *target = (ActivePanel == LeftPanel) ? RightPanel : LeftPanel;
      if (target && target->vol && target->vol->total_dirs > 0) {
        int idx = target->disp_begin_pos + target->cursor_pos;
        /* Safety bounds check */
        if (idx < 0)
          idx = 0;
        if (idx >= target->vol->total_dirs)
          idx = target->vol->total_dirs - 1;

        GetPath(target->vol->dir_entry_list[idx].dir_entry, to_dir);
      }
    }

    if (UI_ReadString("To Directory:", to_dir, PATH_LENGTH, HST_PATH) == CR) {
      if (to_dir[0] == '\0') {
        strcpy(to_dir, ".");
      }
      return (0);
    }
  }
  ClearHelp();
  return (-1);
}

int GetCopyParameter(char *from_file, BOOL path_copy, char *to_file,
                     char *to_dir) {
  char prompt_header[PATH_LENGTH + 50];
  char prompt_as[PATH_LENGTH + 1];

  if (from_file == NULL) {
    from_file = "TAGGED FILES";
    (void)strcpy(to_file, "*");
  } else {
    (void)strcpy(to_file, from_file);
  }

  if (path_copy) {
    (void)snprintf(prompt_header, sizeof(prompt_header), "PATHCOPY: %s",
                   from_file);
  } else {
    (void)snprintf(prompt_header, sizeof(prompt_header), "COPY: %s", from_file);
  }

  ClearHelp();

  if (UI_ReadString(prompt_header, to_file, PATH_LENGTH, HST_FILE) == CR) {

    strncpy(prompt_as, to_file, PATH_LENGTH);
    prompt_as[PATH_LENGTH] = '\0';

    if (IsSplitScreen && ActivePanel) {
      YtreePanel *target = (ActivePanel == LeftPanel) ? RightPanel : LeftPanel;
      if (target && target->vol && target->vol->total_dirs > 0) {
        int idx = target->disp_begin_pos + target->cursor_pos;
        /* Safety bounds check */
        if (idx < 0)
          idx = 0;
        if (idx >= target->vol->total_dirs)
          idx = target->vol->total_dirs - 1;

        GetPath(target->vol->dir_entry_list[idx].dir_entry, to_dir);
      }
    }

    if (UI_ReadString("To Directory:", to_dir, PATH_LENGTH, HST_PATH) == CR) {
      if (to_dir[0] == '\0') {
        strcpy(to_dir, ".");
      }
      return (0);
    }
  }
  ClearHelp();
  return (-1);
}

int GetRenameParameter(const char *old_name, char *new_name) {
  const char *prompt;

  if (old_name == NULL) {
    prompt = "RENAME TAGGED FILES TO:";
  } else {
    prompt = "RENAME TO:";
  }

  (void)strcpy(new_name, (old_name) ? old_name : "*");

  if (UI_ReadString(prompt, new_name, PATH_LENGTH, HST_FILE) != CR)
    return (-1);

  if (!strlen(new_name))
    return (-1);

  if (old_name && !strcmp(old_name, new_name)) {
    UI_Message("Can't rename: New name same as old name.");
    return (-1);
  }

  if (strrchr(new_name, FILE_SEPARATOR_CHAR) != NULL) {
    UI_Message("Invalid new name:*No slashes when renaming!");
    return (-1);
  }

  return (0);
}

static int InputModeString(char *modus, int y, int x) {
  int cursor_pos = 1; /* Skip file type char at index 0 */
  int ch;
  char path[PATH_LENGTH];

  curs_set(0); /* Hide hardware cursor, we manage highlighting manually */

  while (1) {
    /* Draw current mode string */
    wmove(stdscr, y, x);
    int i;
    for (i = 0; i < 10; i++) {
      if (i == cursor_pos)
        wattron(stdscr, A_REVERSE);
      waddch(stdscr, modus[i]);
      if (i == cursor_pos)
        wattroff(stdscr, A_REVERSE);
    }
    wrefresh(stdscr);

    ch = Getch();

    /* F2 Handling for Volume Switching */
    if (ch == KEY_F(2)) {
      if (KeyF2Get(CurrentVolume->vol_stats.tree,
                   CurrentVolume->vol_stats.disp_begin_pos,
                   CurrentVolume->vol_stats.cursor_pos, path) == 0) {
        /* Ignore path result as chmod uses specific characters */
      }
      /* Always redraw prompt */
      MvAddStr(LINES - 2, 1, "MODE:");
      continue;
    }

    if (ch == ESC)
      return ESC;
    if (ch == '\n' || ch == '\r')
      return CR;

    switch (ch) {
    case KEY_LEFT:
      if (cursor_pos > 1)
        cursor_pos--;
      break;
    case KEY_RIGHT:
      if (cursor_pos < 9)
        cursor_pos++;
      break;
    case KEY_HOME:
      cursor_pos = 1;
      break;
    case KEY_END:
      cursor_pos = 9;
      break;
    default:
      if (strchr("rwx-sStT", ch)) {
        modus[cursor_pos] = ch;
        if (cursor_pos < 9)
          cursor_pos++;
      }
      break;
    }
  }
}

int ChangeFileModus(FileEntry *fe_ptr) {
  char modus[12];
  WalkingPackage walking_package;
  int result;
  int x;

  result = -1;

  if (mode != DISK_MODE && mode != USER_MODE) {
    return (result);
  }

  (void)GetAttributes(fe_ptr->stat_struct.st_mode, modus);

  ClearHelp();
  MvAddStr(LINES - 2, 1, "MODE:");
  x = 1 + strlen("MODE:") + UI_INPUT_PADDING;

  if (InputModeString(modus, LINES - 2, x) == CR) {
    (void)strcpy(walking_package.function_data.change_modus.new_modus, modus);
    result = SetFileModus(fe_ptr, &walking_package);
  }

  move(LINES - 2, 1);
  clrtoeol();

  return (result);
}

int ChangeDirModus(DirEntry *de_ptr) {
  char modus[12];
  WalkingPackage walking_package;
  int result;
  int x;

  result = -1;

  if (mode != DISK_MODE && mode != USER_MODE) {
    return (result);
  }

  (void)GetAttributes(de_ptr->stat_struct.st_mode, modus);

  ClearHelp();
  MvAddStr(LINES - 2, 1, "MODE:");
  x = 1 + strlen("MODE:") + UI_INPUT_PADDING;

  if (InputModeString(modus, LINES - 2, x) == CR) {
    (void)strcpy(walking_package.function_data.change_modus.new_modus, modus);
    result = SetDirModus(de_ptr, &walking_package);
  }

  move(LINES - 2, 1);
  clrtoeol();

  return (result);
}

int GetNewOwner(int st_uid) {
  char owner[OWNER_NAME_MAX * 2 + 1];
  char *owner_name_ptr;
  int owner_id;
  int id;

  owner_id = -1;

  id = (st_uid == -1) ? (int)getuid() : st_uid;

  owner_name_ptr = GetPasswdName(id);
  if (owner_name_ptr == NULL) {
    (void)snprintf(owner, sizeof(owner), "%d", id);
  } else {
    (void)strcpy(owner, owner_name_ptr);
  }

  ClearHelp();

  MvAddStr(LINES - 2, 1, "OWNER:");

  if (InputStringEx(owner, LINES - 2, 1 + strlen("OWNER:") + UI_INPUT_PADDING,
                    0, OWNER_NAME_MAX, OWNER_NAME_MAX, "\r\033", HST_ID,
                    NULL) == CR) {
    if ((owner_id = GetPasswdUid(owner)) == -1) {
      UI_Message("Can't read Owner-ID:*%s", owner);
    }
  }

  move(LINES - 2, 1);
  clrtoeol();

  return (owner_id);
}

int GetNewGroup(int st_gid) {
  char group[GROUP_NAME_MAX * 2 + 1];
  char *group_name_ptr;
  int id;
  int group_id;

  group_id = -1;

  id = (st_gid == -1) ? (int)getgid() : st_gid;

  group_name_ptr = GetGroupName(id);
  if (group_name_ptr == NULL) {
    (void)snprintf(group, sizeof(group), "%d", id);
  } else {
    (void)strcpy(group, group_name_ptr);
  }

  ClearHelp();

  if (UI_ReadString("GROUP:", group, GROUP_NAME_MAX, HST_ID) == CR) {
    if ((group_id = GetGroupId(group)) == -1) {
      UI_Message("Can't read Group-ID:*\"%s\"", group);
    }
  }

  move(LINES - 2, 1);
  clrtoeol();

  return (group_id);
}

int ChangeFileOrDirOwnership(const char *path, struct stat *stat_buf,
                             BOOL change_owner, BOOL change_group) {
  uid_t new_uid = stat_buf->st_uid;
  gid_t new_gid = stat_buf->st_gid;
  int result = -1;

  if (change_owner) {
    int owner_id = GetNewOwner(stat_buf->st_uid);
    if (owner_id < 0)
      return -1; /* User cancelled or error */
    new_uid = (uid_t)owner_id;
  }

  if (change_group) {
    int group_id = GetNewGroup(stat_buf->st_gid);
    if (group_id < 0)
      return -1; /* User cancelled or error */
    new_gid = (gid_t)group_id;
  }

  /* Use the generic helper to perform the system call and update stat */
  result = ChangeOwnership(path, new_uid, new_gid, stat_buf);

  return result;
}

int HandleDirOwnership(DirEntry *de_ptr, BOOL change_owner, BOOL change_group) {
  char path[PATH_LENGTH + 1];

  if (mode != DISK_MODE && mode != USER_MODE) {
    return -1;
  }

  GetPath(de_ptr, path);
  return ChangeFileOrDirOwnership(path, &de_ptr->stat_struct, change_owner,
                                  change_group);
}

int HandleFileOwnership(FileEntry *fe_ptr, BOOL change_owner,
                        BOOL change_group) {
  char path[PATH_LENGTH + 1];

  if (mode != DISK_MODE && mode != USER_MODE) {
    return -1;
  }

  GetFileNamePath(fe_ptr, path);
  return ChangeFileOrDirOwnership(path, &fe_ptr->stat_struct, change_owner,
                                  change_group);
}

int UI_ArchiveCallback(int status, const char *msg, void *user_data) {
  (void)user_data;
  if (status == ARCHIVE_STATUS_PROGRESS) {
    DrawSpinner();
    if (EscapeKeyPressed()) {
      return ARCHIVE_CB_ABORT;
    }
  } else if (status == ARCHIVE_STATUS_ERROR) {
    if (msg)
      UI_Message("%s", msg);
  } else if (status == ARCHIVE_STATUS_WARNING) {
    if (msg)
      UI_Warning("%s", msg);
  }
  return ARCHIVE_CB_CONTINUE;
}

int GetCommandLine(char *command_line) {
  int result = -1;

  ClearHelp();

  if (UI_ReadString("COMMAND:", command_line, COMMAND_LINE_LENGTH, HST_EXEC) ==
      CR) {
    result = 0;
  }

  move(LINES - 2, 1);
  clrtoeol();

  return (result);
}

int GetSearchCommandLine(char *command_line, char *raw_pattern) {
  int result = -1;
  char input_buf[256];

  ClearHelp();

  input_buf[0] = '\0';

  if (UI_ReadString("SEARCH TAGGED:", input_buf, 256, HST_SEARCH) == CR) {
    if (raw_pattern) {
      strncpy(raw_pattern, input_buf, 255);
      raw_pattern[255] = '\0';
    }

    /* Construct command line: grep -i "pattern" {} */
    snprintf(command_line, COMMAND_LINE_LENGTH, "grep -i \"%s\" {}", input_buf);

    result = 0;
  }

  move(LINES - 2, 1);
  clrtoeol();

  return (result);
}

int GetPipeCommand(char *pipe_command) {
  int result = -1;

  ClearHelp();

  if (UI_ReadString("Pipe-Command:", pipe_command, PATH_LENGTH, HST_PIPE) ==
      CR) {
    result = 0;
  }

  move(LINES - 2, 1);
  clrtoeol();

  return (result);
}

int SystemCall(char *command_line, Statistic *s) {
  int result;

  endwin(); /* Ensure terminal state is reset before external command */
  result = SilentSystemCall(command_line, s);

  (void)GetAvailBytes(&s->disk_space, s);
  /* Full screen redraw to fully restore the curses UI */
  clearok(stdscr, TRUE);
  refresh();
  return (result);
}

int QuerySystemCall(char *command_line, Statistic *s) {
  int result;

  endwin(); /* 1. Save state / Exit curses mode */

  /* 2. Execute command (runs outside curses) */
  result = SilentSystemCallEx(command_line, FALSE, s);

  /* The external command has finished. We are still in the raw terminal. */

  HitReturnToContinue(); /* 3. Print message and wait for key in raw terminal */

  /* 4. Aggressive redraw/refresh to restore the curses UI completely */
  clearok(stdscr, TRUE);
  refresh();

  (void)GetAvailBytes(&s->disk_space, s);

  return (result);
}

int UI_ReadFilter(void) {
  int result = -1;
  char buffer[FILE_SPEC_LENGTH * 2 + 1];

  ClearHelp();
  (void)strcpy(buffer, "*");

  if (UI_ReadString("FILTER:", buffer, FILE_SPEC_LENGTH, HST_FILTER) == CR) {
    if (SetFilter(buffer, &CurrentVolume->vol_stats)) {
      UI_Message("Invalid Filter Spec");
    } else {
      (void)strcpy(CurrentVolume->vol_stats.file_spec, buffer);
      result = 0;
    }
  }
  move(LINES - 3, 1);
  clrtoeol();
  move(LINES - 2, 1);
  clrtoeol();
  return (result);
}

void UI_GetKindOfSort(void) {
  int c;
  int s = 0;
  int order = SORT_ASC;

  ClearHelp();
  PrintOptions(
      stdscr, LINES - 1, 1,
      "Sort by (A)ccTime (C)hgTime (E)xtension (G)roup (M)odTime   (O)rder: "
      "[ascending]");

  do {
    c = Getch();
    if (c == -1 || c == ESC)
      return;
    c = toupper(c);
    if (c == 'Q')
      return;

    switch (c) {
    case 'N':
      s = SORT_BY_NAME;
      break;
    case 'E':
      s = SORT_BY_EXTENSION;
      break;
    case 'M':
      s = SORT_BY_MOD_TIME;
      break;
    case 'A':
      s = SORT_BY_ACC_TIME;
      break;
    case 'C':
      s = SORT_BY_CHG_TIME;
      break;
    case 'G':
      s = SORT_BY_GROUP;
      break;
    case 'W':
      s = SORT_BY_OWNER;
      break;
    case 'S':
      s = SORT_BY_SIZE;
      break;
    case 'O':
      if (order == SORT_ASC) {
        order = SORT_DSC;
        PrintOptions(stdscr, LINES - 1, 1,
                     "Sort by (A)ccTime (C)hgTime (E)xtension (G)roup "
                     "(M)odTime   (O)rder: "
                     "[descending]");
      } else {
        order = SORT_ASC;
        PrintOptions(stdscr, LINES - 1, 1,
                     "Sort by (A)ccTime (C)hgTime (E)xtension (G)roup "
                     "(M)odTime   (O)rder: "
                     "[ascending]");
      }
      break;
    }
  } while (!strchr("ACEGMNWS", c));

  SetKindOfSort(s + order, &CurrentVolume->vol_stats);
}

void shell_quote(char *dest, const char *src) {
  *dest++ = '\'';
  while (*src) {
    if (*src == '\'') {
      *dest++ = '\'';
      *dest++ = '\\';
      *dest++ = '\'';
      *dest++ = '\'';
    } else {
      *dest++ = *src;
    }
    src++;
  }
  *dest++ = '\'';
  *dest = '\0';
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
  struct dirent *entry;
  char fullpath[PATH_LENGTH];

  if (!d)
    return -1;

  while ((entry = readdir(d))) {
    if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
      continue;
    snprintf(fullpath, sizeof(fullpath), "%s/%s", path, entry->d_name);
    struct stat st;
    if (stat(fullpath, &st) == 0) {
      if (S_ISDIR(st.st_mode)) {
        recursive_rmdir(fullpath);
      } else {
        unlink(fullpath);
      }
    }
  }
  closedir(d);
  return rmdir(path);
}

int UI_ViewTaggedFiles(DirEntry *dir_entry) {
  FileEntry *fe;
  char *command_line;
  char path[PATH_LENGTH + 1];
  char quoted_path[PATH_LENGTH * 2 + 1];
  int start_dir_fd = -1;
  int temp_count = 0;
  int i;
  int max_cmd_len = COMMAND_LINE_LENGTH;
  int current_len = 0;
  char temp_dir_template[PATH_LENGTH];
  char *temp_dir = NULL;
  Statistic *s = &CurrentVolume->vol_stats;

  const char *viewer = PAGER;
  if (!viewer || !*viewer)
    viewer = "less";

  command_line = (char *)xmalloc(max_cmd_len + 1);
  strcpy(command_line, viewer);

  if (GlobalSearchTerm[0] != '\0') {
    char search_arg[300];
    snprintf(search_arg, sizeof(search_arg), " -p \"%s\"", GlobalSearchTerm);
    strcat(command_line, search_arg);
  }

  current_len = strlen(command_line);

  if (s->login_mode == ARCHIVE_MODE) {
    strcpy(temp_dir_template, "/tmp/ytree_view_XXXXXX");
    temp_dir = mkdtemp(temp_dir_template);
    if (!temp_dir) {
      UI_Error(__FILE__, __LINE__, "Could not create temp dir for viewing");
      free(command_line);
      return -1;
    }
  }

  if (!ActivePanel || !ActivePanel->file_entry_list) {
    free(command_line);
    return 0;
  }

  for (i = 0; i < (int)ActivePanel->file_count; i++) {
    fe = ActivePanel->file_entry_list[i].file;

    if (fe->tagged && fe->matching) {
      if (s->login_mode == DISK_MODE || s->login_mode == USER_MODE) {
        GetFileNamePath(fe, path);
        shell_quote(quoted_path, path);

        if ((int)(current_len + strlen(quoted_path) + 1) >= max_cmd_len) {
          UI_Warning("Too many tagged files. Truncated list.");
          break;
        }
        strcat(command_line, " ");
        strcat(command_line, quoted_path);
        current_len += strlen(quoted_path) + 1;

      } else if (s->login_mode == ARCHIVE_MODE) {
#ifdef HAVE_LIBARCHIVE
        if (temp_count >= 100) {
          UI_Warning("Archive view limit (100) reached.");
          break;
        }

        char t_filename[PATH_LENGTH];
        char t_dirname[PATH_LENGTH];
        char root_path[PATH_LENGTH + 1];
        char relative_path[PATH_LENGTH + 1];
        char internal_path[PATH_LENGTH + 1];
        char full_path[PATH_LENGTH + 1];

        GetPath(CurrentVolume->vol_stats.tree, root_path);
        GetFileNamePath(fe, full_path);

        size_t root_len = strlen(root_path);
        if (strncmp(full_path, root_path, root_len) == 0) {
          char *ptr = full_path + root_len;
          if (*ptr == FILE_SEPARATOR_CHAR) {
            ptr++;
            strcpy(relative_path, ptr);
          } else if (*ptr == '\0') {
            relative_path[0] = '\0';
          } else {
            strcpy(relative_path, full_path);
          }
        } else {
          strcpy(relative_path, full_path);
        }

        if (strlen(relative_path) == 0) {
          strcpy(relative_path, fe->name);
        }

        if (strlen(relative_path) > 0) {
          snprintf(t_dirname, sizeof(t_dirname), "%s/%s", temp_dir,
                   relative_path);
          char *dir_only = xstrdup(t_dirname);
          dirname(dir_only);
          recursive_mkdir(dir_only);
          free(dir_only);
          snprintf(t_filename, sizeof(t_filename), "%s/%s", temp_dir,
                   relative_path);
        } else {
          snprintf(t_filename, sizeof(t_filename), "%s/%s", temp_dir, fe->name);
        }

        strcpy(internal_path, relative_path);

        if (ExtractArchiveNode(s->login_path, internal_path, t_filename,
                               UI_ArchiveCallback, NULL) == 0) {
          shell_quote(quoted_path, t_filename);

          if ((int)(current_len + strlen(quoted_path) + 1) >= max_cmd_len) {
            UI_Warning("Command line too long.");
            break;
          }

          strcat(command_line, " ");
          strcat(command_line, quoted_path);
          current_len += strlen(quoted_path) + 1;
          temp_count++;
        } else {
          UI_Warning("Failed to extract*\"%s\"", internal_path);
        }
#endif
      }
    }
  }

  if (s->login_mode == DISK_MODE) {
    start_dir_fd = open(".", O_RDONLY);
    if (chdir(GetPath(dir_entry, path)) == 0) {
      SystemCall(command_line, &CurrentVolume->vol_stats);
      if (start_dir_fd != -1) {
        (void)fchdir(start_dir_fd);
      }
    }
    if (start_dir_fd != -1)
      close(start_dir_fd);
  } else {
    if (temp_count > 0) {
      SystemCall(command_line, &CurrentVolume->vol_stats);
    } else if (s->login_mode == ARCHIVE_MODE) {
      UI_Message("No files extracted.");
    }
  }

  if (s->login_mode == ARCHIVE_MODE && temp_dir) {
    recursive_rmdir(temp_dir);
  }

  free(command_line);
  return 0;
}
