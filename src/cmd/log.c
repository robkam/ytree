/***************************************************************************
 *
 * src/cmd/log.c
 * Read file tree (UI Controller)
 *
 ***************************************************************************/

#include "ytree.h"

static void SavePanelTreeSelection(YtreePanel *panel) {
  int selected_index;

  if (!panel || !panel->vol)
    return;

  selected_index = panel->disp_begin_pos + panel->cursor_pos;
  if (selected_index < 0)
    selected_index = 0;
  panel->vol->saved_tree_index = selected_index;
}

static void RestorePanelTreeSelection(ViewContext *ctx, YtreePanel *panel) {
  int selected_index;
  int total_dirs;
  int win_height;

  if (!ctx || !panel || !panel->vol)
    return;

  total_dirs = panel->vol->total_dirs;
  if (total_dirs <= 0) {
    panel->disp_begin_pos = 0;
    panel->cursor_pos = 0;
    return;
  }

  selected_index = panel->vol->saved_tree_index;
  if (selected_index < 0)
    selected_index = 0;
  if (selected_index >= total_dirs)
    selected_index = total_dirs - 1;

  win_height = ctx->layout.dir_win_height;
  if (win_height <= 0 && ctx->ctx_dir_window)
    win_height = getmaxy(ctx->ctx_dir_window);
  if (win_height <= 0)
    win_height = 1;

  if (selected_index >= win_height) {
    panel->disp_begin_pos = selected_index - (win_height - 1);
    panel->cursor_pos = win_height - 1;
  } else {
    panel->disp_begin_pos = 0;
    panel->cursor_pos = selected_index;
  }
}

/* Helper function to handle scan progress updates */
static void Login_Progress(ViewContext *ctx, void *data) {
  const Statistic *s = (const Statistic *)data;

  DrawSpinner(ctx);
  ClockHandler(ctx, 0);

  if (ctx->animation_method == 1) {
    /* If animating, redraw the animation step. */
    DrawAnimationStep(ctx, ctx->ctx_file_window);
    doupdate();
  } else {
    /* Update statistics panel periodically */
    if (s) {
      DisplayDiskStatistic(ctx, s);
      doupdate();
    }
  }
}

/* Helper function to clear big_window flag recursively */
static void RecursiveClearBigWindow(DirEntry *d) {
  if (!d)
    return;
  d->big_window = FALSE;
  if (d->sub_tree)
    RecursiveClearBigWindow(d->sub_tree);
  if (d->next)
    RecursiveClearBigWindow(d->next);
}

/*
 * LogDisk
 * UI Controller for loading or switching to a disk volume.
 *
 * Returns:
 * -1 on error
 * 0  on successfully reading a new tree or switching to an existing one
 */
int LogDisk(ViewContext *ctx, YtreePanel *panel, char *path) {
  char saved_filter[FILE_SPEC_LENGTH + 1];
  char resolved_path[PATH_LENGTH + 1];
  struct Volume *s_vol, *tmp;
  struct Volume *found_vol = NULL;
  struct Volume *loaded_vol = NULL;
  struct Volume *old_vol = NULL;
  struct Volume *reuse_vol = NULL;
  Statistic *s;
  struct stat st_check;

  DEBUG_LOG("ENTER LogDisk: path=%s", path);

  SuspendClock(ctx); /* Suspend clock during critical operations */

  /* Keep per-volume tree selection before switching away. */
  SavePanelTreeSelection(panel);

  /* 1. Resolve Path (for UI searching/display purposes) */
  if (realpath(path, resolved_path) == NULL) {
    strncpy(resolved_path, path, PATH_LENGTH);
    resolved_path[PATH_LENGTH] = '\0';
  }

  /* Save current filter to preserve it across transitions. */
  if (panel->vol != NULL && strlen(panel->vol->vol_stats.file_spec) > 0) {
    strncpy(saved_filter, panel->vol->vol_stats.file_spec, FILE_SPEC_LENGTH);
    saved_filter[FILE_SPEC_LENGTH] = '\0';
  } else {
    strncpy(saved_filter, DEFAULT_FILE_SPEC, FILE_SPEC_LENGTH);
    saved_filter[FILE_SPEC_LENGTH] = '\0';
  }

  /* 2. Search Existing Volume */
  HASH_ITER(hh, ctx->volumes_head, s_vol, tmp) {
    if (strcmp(s_vol->vol_stats.login_path, resolved_path) == 0) {
      found_vol = s_vol;
      break;
    }
  }

  if (found_vol != NULL) {
    /* Case A: Volume Found - Check Access and Switch */
    BOOL access_ok = FALSE;

    if (found_vol->vol_stats.login_mode == ARCHIVE_MODE) {
      /* For archives, check if the file still exists */
      if (stat(found_vol->vol_stats.login_path, &st_check) == 0 &&
          !S_ISDIR(st_check.st_mode)) {
        access_ok = TRUE;
        /* Optional: Attempt to chdir to the parent directory for context */
        char parent_dir[PATH_LENGTH + 1];
        strncpy(parent_dir, found_vol->vol_stats.login_path, PATH_LENGTH);
        parent_dir[PATH_LENGTH] = '\0';
        char *slash = strrchr(parent_dir, FILE_SEPARATOR_CHAR);
        if (slash) {
          *slash = '\0';
          if (chdir(parent_dir)) {
            ; /* Explicitly ignore result */
          }
        }
      }
    } else {
      /* For normal disks, try to enter the directory */
      if (chdir(found_vol->vol_stats.login_path) == 0) {
        access_ok = TRUE;
      }
    }

    if (access_ok) {
      panel->vol = found_vol;
      s = &panel->vol->vol_stats;
      ctx->global_search_term[0] = '\0';
      ctx->view_mode = panel->vol->vol_stats.login_mode;

      /* Re-apply the volume's own filter */
      (void)SetFilter(s->file_spec, s);
      RecalculateSysStats(ctx, s);

      /* Refresh display */
      DisplayMenu(ctx);
      BuildDirEntryList(ctx, panel->vol, &(int){0});
      RestorePanelTreeSelection(ctx, panel);

      DisplayTree(ctx, panel->vol, ctx->ctx_dir_window, panel->disp_begin_pos,
                  panel->disp_begin_pos + panel->cursor_pos, TRUE);
      DisplayDiskStatistic(ctx, s);
      DisplayAvailBytes(ctx, s);
      InitClock(ctx);
      return 0;
    } else {
      /* Volume exists but is inaccessible */
      MESSAGE(ctx, "Volume \"%s\" not accessible (Error: %s). Removed.",
              found_vol->vol_stats.login_path, strerror(errno));

      if (found_vol == panel->vol) {
        /* If the current volume is bad, we must delete it. */
        Volume_Delete(ctx, found_vol);
        panel->vol = NULL; /* Temporarily NULL */
      } else {
        Volume_Delete(ctx, found_vol);
      }
      /* Proceed to try loading it fresh */
      found_vol = NULL;
    }
  }

  /* Case B: Load Volume (New or Reuse) */
  old_vol = panel->vol;

  /* Determine if we can reuse the "virgin" volume */
  if (old_vol != NULL && old_vol->vol_stats.login_path[0] == '\0') {
    reuse_vol = old_vol;
  }

  /* Prepare UI for loading */
  DisplayMenu(ctx);
  if (panel->vol)
    DisplayDiskStatistic(
        ctx, &panel->vol->vol_stats); /* Maintain frame if possible */

  if (ctx->animation_method == 1) {
    SwitchToBigFileWindow(ctx);
    InitAnimation(ctx);
  } else {
    RefreshWindow(stdscr);
    RefreshWindow(ctx->ctx_dir_window);
  }
  doupdate();

  if (ctx->animation_method == 0)
    NOTICE(ctx, "Scanning...");

  /* Call Logic Core */
  loaded_vol = Volume_Load(ctx, resolved_path, reuse_vol, Login_Progress, NULL);

  DEBUG_LOG("LogDisk: Volume_Load returned %p", (void *)loaded_vol);

  if (ctx->animation_method == 1)
    StopAnimation(ctx);
  SwitchToSmallFileWindow(ctx);

  /* Handle Result */
  if (loaded_vol == NULL) {
    /* Failure */
    /* Note: Volume_Load displays its own error message now. */

    /* Restore state */
    if (reuse_vol) {
      /* We reused the virgin volume and it failed. It's now empty/reset. */
      /* panel->vol is still pointing to it (old_vol == reuse_vol). */
      /* Display empty state. */
      DisplayMenu(ctx);
      BuildDirEntryList(ctx, panel->vol, &(int){0});
      DisplayTree(ctx, panel->vol, ctx->ctx_dir_window, 0, 0, TRUE);
      DisplayDiskStatistic(ctx, &panel->vol->vol_stats);
      DisplayAvailBytes(ctx, &panel->vol->vol_stats);
    } else if (old_vol != NULL) {
      /* We tried to create a NEW volume and failed. */
      /* panel->vol is still old_vol (valid). Restore its display. */
      panel->vol = old_vol;
      ctx->view_mode = panel->vol->vol_stats.login_mode;
      s = &panel->vol->vol_stats;

      DisplayMenu(ctx);
      BuildDirEntryList(ctx, panel->vol, &(int){0});
      RestorePanelTreeSelection(ctx, panel);

      DisplayTree(ctx, panel->vol, ctx->ctx_dir_window, panel->disp_begin_pos,
                  panel->disp_begin_pos + panel->cursor_pos, TRUE);
      DisplayDiskStatistic(ctx, s);
      DisplayAvailBytes(ctx, s);
    } else {
      /* Critical: No old volume, failed to load new one (e.g., initial startup
       * fail) */
      /* Main will handle exit if panel->vol is invalid */
    }
    InitClock(ctx);
    return -1;
  }

  /* Success */
  panel->vol = loaded_vol;
  s = &panel->vol->vol_stats;
  ctx->global_search_term[0] = '\0';
  ctx->view_mode = s->login_mode;

  /* If this is a new volume (not the startup one), apply saved filter from
   * previous context */
  /* If it's the very first volume (old_vol was NULL or we reused virgin),
   * default filter is already set by Volume_Load */
  if (old_vol != NULL && old_vol->vol_stats.login_path[0] != '\0') {
    strncpy(s->file_spec, saved_filter, FILE_SPEC_LENGTH);
    s->file_spec[FILE_SPEC_LENGTH] = '\0';
  }

  (void)SetFilter(s->file_spec, s);
  RecalculateSysStats(ctx, s);

  /* Final Refresh */
  BuildDirEntryList(ctx, panel->vol, &(int){0});
  RestorePanelTreeSelection(ctx, panel);
  DisplayTree(ctx, panel->vol, ctx->ctx_dir_window, panel->disp_begin_pos,
              panel->disp_begin_pos + panel->cursor_pos, TRUE);
  DisplayDiskStatistic(ctx, s);
  DisplayAvailBytes(ctx, s);

  InitClock(ctx);
  return 0;
}

int GetNewLoginPath(ViewContext *ctx, YtreePanel *panel, char *path) {
  int result;
  int copied_len;
  char user_input[PATH_LENGTH * 2 + 1] = "";
  char current_dir_path[PATH_LENGTH + 1];

  result = -1;

  ClearHelp(ctx);

  MvAddStr(Y_PROMPT(ctx), 1, "LOG:");

  /* Save the current directory context and set it as default for user input */
  copied_len = snprintf(current_dir_path, sizeof(current_dir_path), "%s", path);
  if (copied_len < 0 || (size_t)copied_len >= sizeof(current_dir_path)) {
    return result;
  }

  copied_len = snprintf(user_input, sizeof(user_input), "%s", path);
  if (copied_len < 0 || (size_t)copied_len >= sizeof(user_input)) {
    return result;
  }

  if (ctx->view_mode == LL_FILE_MODE && *path == '<') {
    char *cptr;
    for (cptr = user_input; (*cptr = *(cptr + 1)); cptr++)
      ;
    if (user_input[strlen(user_input) - 1] == '>')
      user_input[strlen(user_input) - 1] = '\0';
  }

  if ((UI_ReadString)(ctx, panel, "Log Path:", user_input, PATH_LENGTH - 1,
                      HST_LOGIN) == CR) {
    char temp_path[PATH_LENGTH * 3 + 2];
    char resolved_path[PATH_LENGTH + 1];
    int composed_len;

    /* InputString expands '~', so check if the result is an absolute path. */
    if (user_input[0] != FILE_SEPARATOR_CHAR) {
      /* It's a relative path. Construct the full path to be resolved. */
      if (strcmp(current_dir_path, FILE_SEPARATOR_STRING) == 0) {
        composed_len = snprintf(temp_path, sizeof(temp_path), "/%s", user_input);
      } else {
        composed_len = snprintf(temp_path, sizeof(temp_path), "%s/%s",
                                current_dir_path, user_input);
      }
    } else {
      /* It's an absolute path. */
      composed_len = snprintf(temp_path, sizeof(temp_path), "%s", user_input);
    }

    if (composed_len < 0 || (size_t)composed_len >= sizeof(temp_path)) {
      return result;
    }

    if (realpath(temp_path, resolved_path) != NULL) {
      copied_len = snprintf(path, PATH_LENGTH + 1, "%s", resolved_path);
      if (copied_len < 0 || copied_len > PATH_LENGTH) {
        return result;
      }
    } else {
      NormPath(temp_path, path);
    }
    result = 0;
  }

  return (result);
}

/*
 * CycleLoadedVolume
 * Cycles through the list of currently loaded volumes.
 * direction: -1 for previous, 1 for next.
 * Returns 0 on successful switch, -1 if no switch occurred or on error.
 */
int CycleLoadedVolume(ViewContext *ctx, YtreePanel *panel, int direction) {
  struct Volume *s, *tmp;
  struct Volume **vol_array = NULL;
  int num_volumes = 0;

  int retries = 0;
  int max_retries;
  BOOL changes_made = FALSE;

  num_volumes = HASH_COUNT(ctx->volumes_head);
  max_retries = num_volumes + 1;

  while (retries++ < max_retries) {
    num_volumes = HASH_COUNT(ctx->volumes_head);

    if (num_volumes <= 1) {
      MESSAGE(ctx, "Only one volume loaded.*No cycling possible.");
      return (changes_made ? 0 : -1);
    }

    vol_array = (struct Volume **)malloc(num_volumes * sizeof(struct Volume *));
    if (vol_array == NULL) {
      MESSAGE(ctx, "Failed to allocate memory for volume list during cycle.");
      return -1;
    }

    int current_index = -1;
    int i = 0;
    HASH_ITER(hh, ctx->volumes_head, s, tmp) {
      vol_array[i] = s;
      if (s == panel->vol) {
        current_index = i;
      }
      i++;
    }

    if (current_index == -1) {
      MESSAGE(ctx, "Current volume not found in list during cycle.");
      free(vol_array);
      return -1;
    }

    int target_index = (current_index + direction + num_volumes) % num_volumes;

    if (target_index == current_index && retries > 1) {
      free(vol_array);
      MESSAGE(ctx, "No other accessible volumes found.");
      return (changes_made ? 0 : -1);
    }

    const struct Volume *target = vol_array[target_index];
    char target_path[PATH_LENGTH + 1];
    strncpy(target_path, target->vol_stats.login_path, PATH_LENGTH);
    target_path[PATH_LENGTH] = '\0';

    free(vol_array);

    /* Use LogDisk to attempt the switch.
     * LogDisk handles validation, cleanup if inaccessible, and UI updates. */
    if (LogDisk(ctx, panel, target_path) == 0) {
      /* Force UI reset to standard split view on cycle */
      RecursiveClearBigWindow(panel->vol->vol_stats.tree);
      ReCreateWindows(ctx);
      ClockHandler(ctx, 0);
      return 0;
    } else {
      /* If LogDisk failed, the target volume was likely removed.
         The loop will retry. */
      changes_made = TRUE;
    }
  }

  MESSAGE(ctx,
          "Failed to switch to an accessible volume after multiple attempts.");
  return (changes_made ? 0 : -1);
}
