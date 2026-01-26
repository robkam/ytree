/***************************************************************************
 *
 * src/cmd/log.c
 * Read file tree (UI Controller)
 *
 ***************************************************************************/


#include "ytree.h"
/* #include <sys/wait.h> */  /* maybe wait.h is available */

/* Helper function to handle scan progress updates */
static void Login_Progress(void *data)
{
    Statistic *s = (Statistic *)data;

    DrawSpinner();
    ClockHandler(0);

    if (animation_method == 1) {
        /* If animating, redraw the animation step. */
        DrawAnimationStep(file_window);
        doupdate();
    } else {
        /* Update statistics panel periodically */
        if (s) {
            DisplayDiskStatistic(s);
            doupdate();
        }
    }
}

/* Helper function to clear big_window flag recursively */
static void RecursiveClearBigWindow(DirEntry *d) {
    if (!d) return;
    d->big_window = FALSE;
    if (d->sub_tree) RecursiveClearBigWindow(d->sub_tree);
    if (d->next) RecursiveClearBigWindow(d->next);
}

/*
 * LogDisk
 * UI Controller for loading or switching to a disk volume.
 *
 * Returns:
 * -1 on error
 * 0  on successfully reading a new tree or switching to an existing one
 */
int LogDisk(char *path)
{
  char   saved_filter[FILE_SPEC_LENGTH + 1];
  char   resolved_path[PATH_LENGTH + 1];
  struct Volume *s_vol, *tmp;
  struct Volume *found_vol = NULL;
  struct Volume *loaded_vol = NULL;
  struct Volume *old_vol = NULL;
  struct Volume *reuse_vol = NULL;
  Statistic *s;
  BOOL   access_ok = FALSE;
  struct stat st_check;

  SuspendClock(); /* Suspend clock during critical operations */

  /* 1. Resolve Path (for UI searching/display purposes) */
  if (realpath(path, resolved_path) == NULL) {
      strncpy(resolved_path, path, PATH_LENGTH);
      resolved_path[PATH_LENGTH] = '\0';
  }

  /* Save current filter to preserve it across transitions. */
  if (CurrentVolume != NULL && strlen(CurrentVolume->vol_stats.file_spec) > 0) {
      strncpy(saved_filter, CurrentVolume->vol_stats.file_spec, FILE_SPEC_LENGTH);
      saved_filter[FILE_SPEC_LENGTH] = '\0';
  } else {
      strcpy(saved_filter, DEFAULT_FILE_SPEC);
  }

  /* 2. Search Existing Volume */
  HASH_ITER(hh, VolumeList, s_vol, tmp) {
      if (strcmp(s_vol->vol_stats.login_path, resolved_path) == 0) {
          found_vol = s_vol;
          break;
      }
  }

  if (found_vol != NULL) {
      /* Case A: Volume Found - Check Access and Switch */

      if (found_vol->vol_stats.login_mode == ARCHIVE_MODE) {
          /* For archives, check if the file still exists */
          if (stat(found_vol->vol_stats.login_path, &st_check) == 0 && !S_ISDIR(st_check.st_mode)) {
              access_ok = TRUE;
              /* Optional: Attempt to chdir to the parent directory for context */
              char parent_dir[PATH_LENGTH + 1];
              strcpy(parent_dir, found_vol->vol_stats.login_path);
              char *slash = strrchr(parent_dir, FILE_SEPARATOR_CHAR);
              if (slash) {
                  *slash = '\0';
                  if (chdir(parent_dir)) { ; } /* Explicitly ignore result */
              }
          }
      } else {
          /* For normal disks, try to enter the directory */
          if (chdir(found_vol->vol_stats.login_path) == 0) {
              access_ok = TRUE;
          }
      }

      if (access_ok) {
           CurrentVolume = found_vol;
           s = &CurrentVolume->vol_stats;
           GlobalSearchTerm[0] = '\0';
           mode = CurrentVolume->vol_stats.login_mode;

           /* Re-apply the volume's own filter */
           (void) SetFilter(s->file_spec, s);
           RecalculateSysStats(s);

           /* Refresh display */
           DisplayMenu();
           BuildDirEntryList(CurrentVolume);
           DisplayTree(CurrentVolume, dir_window, s->disp_begin_pos, s->disp_begin_pos + s->cursor_pos, TRUE);
           DisplayDiskStatistic(s);
           DisplayAvailBytes(s);
           InitClock();
           return 0;
       } else {
           /* Volume exists but is inaccessible */
           (void) snprintf(message, MESSAGE_LENGTH, "Volume \"%s\" not accessible (Error: %s). Removed.",
                found_vol->vol_stats.login_path, strerror(errno));
           MESSAGE(message);

           if (found_vol == CurrentVolume) {
               /* If the current volume is bad, we must delete it. */
               /* We will fall through to load a new one (or reload this one if user fixes issue,
                  but LogDisk is called with a path).
                  Actually, if we remove it, we should try to reload it using Volume_Load logic below. */
               Volume_Delete(found_vol);
               CurrentVolume = NULL; /* Temporarily NULL */
           } else {
               Volume_Delete(found_vol);
           }
           /* Proceed to try loading it fresh */
           found_vol = NULL;
       }
  }

  /* Case B: Load Volume (New or Reuse) */
  old_vol = CurrentVolume;

  /* Determine if we can reuse the "virgin" volume */
  if (old_vol != NULL && old_vol->vol_stats.login_path[0] == '\0') {
      reuse_vol = old_vol;
  }

  /* Prepare UI for loading */
  DisplayMenu();
  if (CurrentVolume) DisplayDiskStatistic(&CurrentVolume->vol_stats); /* Maintain frame if possible */

  if (animation_method == 1) {
      SwitchToBigFileWindow();
      InitAnimation();
  } else {
      RefreshWindow(stdscr);
      RefreshWindow(dir_window);
  }
  doupdate();

  if (animation_method == 0) Notice("Scanning...");

  /* Call Logic Core */
  loaded_vol = Volume_Load(resolved_path, reuse_vol, Login_Progress);

  if (animation_method == 0) UnmapNoticeWindow();
  if (animation_method == 1) StopAnimation();
  SwitchToSmallFileWindow();

  /* Handle Result */
  if (loaded_vol == NULL) {
      /* Failure */
      MESSAGE(message); /* message is populated by Volume_Load */

      /* Restore state */
      if (reuse_vol) {
          /* We reused the virgin volume and it failed. It's now empty/reset. */
          /* CurrentVolume is still pointing to it (old_vol == reuse_vol). */
          /* Display empty state. */
          DisplayMenu();
          BuildDirEntryList(CurrentVolume);
          DisplayTree(CurrentVolume, dir_window, 0, 0, TRUE);
          DisplayDiskStatistic(&CurrentVolume->vol_stats);
          DisplayAvailBytes(&CurrentVolume->vol_stats);
      } else if (old_vol != NULL) {
          /* We tried to create a NEW volume and failed. */
          /* CurrentVolume is still old_vol (valid). Restore its display. */
          CurrentVolume = old_vol;
          mode = CurrentVolume->vol_stats.login_mode;
          s = &CurrentVolume->vol_stats;

          DisplayMenu();
          BuildDirEntryList(CurrentVolume);
          DisplayTree(CurrentVolume, dir_window, s->disp_begin_pos, s->disp_begin_pos + s->cursor_pos, TRUE);
          DisplayDiskStatistic(s);
          DisplayAvailBytes(s);
      } else {
          /* Critical: No old volume, failed to load new one (e.g., initial startup fail) */
          /* Main will handle exit if CurrentVolume is invalid */
      }
      InitClock();
      return -1;
  }

  /* Success */
  CurrentVolume = loaded_vol;
  s = &CurrentVolume->vol_stats;
  GlobalSearchTerm[0] = '\0';
  mode = s->login_mode;

  /* If this is a new volume (not the startup one), apply saved filter from previous context */
  /* If it's the very first volume (old_vol was NULL or we reused virgin), default filter is already set by Volume_Load */
  if (old_vol != NULL && old_vol->vol_stats.login_path[0] != '\0') {
      strcpy(s->file_spec, saved_filter);
  }

  (void) SetFilter(s->file_spec, s);
  RecalculateSysStats(s);

  /* Final Refresh */
  BuildDirEntryList(CurrentVolume);
  DisplayTree(CurrentVolume, dir_window, 0, 0, TRUE);
  DisplayDiskStatistic(s);
  DisplayAvailBytes(s);

  InitClock();
  return 0;
}


int GetNewLoginPath(char *path)
{
    int result;
    char *cptr;
    char user_input[PATH_LENGTH * 2 + 1] = "";
    char current_dir_path[PATH_LENGTH + 1];

    result = -1;

    ClearHelp();

    MvAddStr(Y_PROMPT, 1, "LOG:");

    /* Save the current directory context and set it as default for user input */
    strcpy(current_dir_path, path);
    strcpy(user_input, path);

    if (mode == LL_FILE_MODE && *path == '<') {
        for (cptr = user_input; (*cptr = *(cptr + 1)); cptr++);
        if (user_input[strlen(user_input) - 1] == '>')
            user_input[strlen(user_input) - 1] = '\0';
    }

    if (UI_ReadString("Log Path:", user_input, PATH_LENGTH - 1, HST_LOGIN) == CR) {
        char temp_path[PATH_LENGTH * 3 + 2];
        char resolved_path[PATH_LENGTH + 1];

        /* InputString expands '~', so check if the result is an absolute path. */
        if (user_input[0] != FILE_SEPARATOR_CHAR) {
            /* It's a relative path. Construct the full path to be resolved. */
            if (strcmp(current_dir_path, FILE_SEPARATOR_STRING) == 0) {
                snprintf(temp_path, sizeof(temp_path), "/%s", user_input);
            } else {
                snprintf(temp_path, sizeof(temp_path), "%s/%s", current_dir_path, user_input);
            }
        } else {
            /* It's an absolute path. */
            strcpy(temp_path, user_input);
        }

        if (realpath(temp_path, resolved_path) != NULL) {
            strcpy(path, resolved_path);
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
int CycleLoadedVolume(int direction)
{
    struct Volume *s, *tmp;
    struct Volume **vol_array = NULL;
    int num_volumes = 0;
    int current_index = -1;
    int target_index = -1;
    int i = 0;

    int retries = 0;
    int max_retries;
    BOOL changes_made = FALSE;

    num_volumes = HASH_COUNT(VolumeList);
    max_retries = num_volumes + 1;

    while (retries++ < max_retries) {
        num_volumes = HASH_COUNT(VolumeList);

        if (num_volumes <= 1) {
            MESSAGE("Only one volume loaded.*No cycling possible.");
            return (changes_made ? 0 : -1);
        }

        vol_array = (struct Volume **)malloc(num_volumes * sizeof(struct Volume *));
        if (vol_array == NULL) {
            ERROR_MSG("Failed to allocate memory for volume list during cycle.");
            return -1;
        }

        current_index = -1;
        i = 0;
        HASH_ITER(hh, VolumeList, s, tmp) {
            vol_array[i] = s;
            if (s == CurrentVolume) {
                current_index = i;
            }
            i++;
        }

        if (current_index == -1) {
            ERROR_MSG("Current volume not found in list during cycle.");
            free(vol_array);
            return -1;
        }

        target_index = (current_index + direction + num_volumes) % num_volumes;

        if (target_index == current_index && retries > 1) {
            free(vol_array);
            MESSAGE("No other accessible volumes found.");
            return (changes_made ? 0 : -1);
        }

        struct Volume *target = vol_array[target_index];
        char target_path[PATH_LENGTH + 1];
        strncpy(target_path, target->vol_stats.login_path, PATH_LENGTH);
        target_path[PATH_LENGTH] = '\0';

        free(vol_array);

        /* Use LogDisk to attempt the switch.
         * LogDisk handles validation, cleanup if inaccessible, and UI updates. */
        if (LogDisk(target_path) == 0) {
            /* Force UI reset to standard split view on cycle */
            RecursiveClearBigWindow(CurrentVolume->vol_stats.tree);
            ReCreateWindows();
            ClockHandler(0);
            return 0;
        } else {
             /* If LogDisk failed, the target volume was likely removed.
                The loop will retry. */
             changes_made = TRUE;
        }
    }

    MESSAGE("Failed to switch to an accessible volume after multiple attempts.");
    return (changes_made ? 0 : -1);
}