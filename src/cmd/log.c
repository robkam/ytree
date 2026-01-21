/***************************************************************************
 *
 * log.c
 * Read file tree
 *
 ***************************************************************************/


#include "ytree.h"
/* #include <sys/wait.h> */  /* maybe wait.h is available */


/* Login Disk returns
 * -1 on error
 * 0  on successfully reading a new tree or when using a tree in memory
 */


int LoginDisk(char *path)
{
  struct stat stat_struct;
  int    depth;
  int    result = 0;
  char   saved_filter[FILE_SPEC_LENGTH + 1];
  char   resolved_path[PATH_LENGTH + 1];
  struct Volume *s_vol, *tmp;
  struct Volume *found_vol = NULL;
  struct Volume *old_vol = NULL;
  BOOL   is_new_vol_created = FALSE; /* Flag to track if we actually called Volume_Create */
  Statistic *s;

  SuspendClock(); /* Suspend clock during critical operations to prevent screen corruption */

  /* 1. Normalize Path */
  /* Use realpath to get the canonical path. If it fails (e.g., path doesn't exist yet,
   * which can happen for a new archive path), fall back to the provided path directly. */
  if (realpath(path, resolved_path) == NULL) {
      strncpy(resolved_path, path, PATH_LENGTH);
      resolved_path[PATH_LENGTH] = '\0';
  }

  /* Pre-flight check for path existence and type */
  if( STAT_( resolved_path, &stat_struct ) )
  {
    /* Stat failed */
    (void) snprintf( message, MESSAGE_LENGTH, "Can't access*\"%s\"*%s", resolved_path, strerror(errno) );
    MESSAGE( message );
    InitClock(); /* Resume clock before returning */
    return( -1 );
  }

  /* Pre-flight check for non-directory files to ensure they are valid archives */
  if( !S_ISDIR(stat_struct.st_mode ) )
  {
#ifdef HAVE_LIBARCHIVE
      struct archive *a_test;
      int r_test;

      a_test = archive_read_new();
      if (a_test == NULL) {
          ERROR_MSG("archive_read_new() failed");
          InitClock(); /* Resume clock before returning */
          return -1;
      }
      archive_read_support_filter_all(a_test);
      archive_read_support_format_all(a_test);
      r_test = archive_read_open_filename(a_test, resolved_path, 10240);
      archive_read_free(a_test);

      if (r_test != ARCHIVE_OK) {
          (void) snprintf(message, MESSAGE_LENGTH, "Not a recognized archive file*or format not supported*\"%s\"", resolved_path);
          MESSAGE(message);
          InitClock(); /* Resume clock before returning */
          return -1;
      }
#else
      /* Fallback for when libarchive is not available */
      (void) snprintf(message, MESSAGE_LENGTH, "Cannot open file as archive*ytree not compiled with*libarchive support");
      MESSAGE(message);
      InitClock(); /* Resume clock before returning */
      return -1;
#endif
  }


  /* Save current filter to preserve it across transitions.
   * 'statistic' here refers to CurrentVolume->vol_stats via macro if not removed, but we should use CurrentVolume directly */
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
      /* Case A: Volume Found - Switch to it */

      /* Verify accessibility of the volume before switching. */
      BOOL access_ok = FALSE;
      struct stat st_check;

      /* Renamed usage: found_vol->vol_stats.mode -> found_vol->vol_stats.login_mode */
      if (found_vol->vol_stats.login_mode == ARCHIVE_MODE) {
          /* For archives, check if the file still exists */
          if (stat(found_vol->vol_stats.login_path, &st_check) == 0 && !S_ISDIR(st_check.st_mode)) {
              access_ok = TRUE;
              /* Optional: Attempt to chdir to the parent directory for context,
                 but don't fail if we can't. */
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

      if (!access_ok) {
          char error_message_buffer[MESSAGE_LENGTH + 1];
          snprintf(error_message_buffer, sizeof(error_message_buffer), "Volume \"%s\" not accessible (Error: %s). Removed.",
                found_vol->vol_stats.login_path, strerror(errno));
          MESSAGE(error_message_buffer);
          if (found_vol == CurrentVolume) {
               Volume_Delete(found_vol);
               CurrentVolume = Volume_Create();
               s = &CurrentVolume->vol_stats;
               /* ... refresh empty ... */
               DisplayMenu();
               DisplayTree(CurrentVolume, dir_window, 0, 0, TRUE);
               DisplayDiskStatistic(s);
               DisplayAvailBytes(s);
          } else {
               Volume_Delete(found_vol);
           }
           InitClock(); /* Resume clock before returning */
           return -1;
       }

       /* If access_ok, proceed with switching to the found volume. */
       CurrentVolume = found_vol;
       s = &CurrentVolume->vol_stats;

       /* Reset global search term to prevent context bleeding */
       GlobalSearchTerm[0] = '\0';

       /* Synchronize the global 'mode' variable with the found volume's stored mode. */
       /* Renamed usage: CurrentVolume->vol_stats.mode -> CurrentVolume->vol_stats.login_mode */
       mode = CurrentVolume->vol_stats.login_mode;

       /* FIX: Do NOT overwrite an existing volume's filter settings.
          This prevents filter leakage between volumes.
          We re-apply the filter stored in the volume's stats to ensure display consistency. */
       (void) SetFilter(s->file_spec, s); /* This calls ApplyFilter internally */
       RecalculateSysStats(s);            /* Sum up stats based on flags */

       /* Refresh display for the switched volume, using its stored display state. */
       DisplayMenu(); /* Updates path at top */
       /* CRITICAL FIX: Rebuild the directory entry list for the new volume's tree
        * before displaying it, to prevent use-after-free issues with stale pointers. */
       BuildDirEntryList(CurrentVolume);
       DisplayTree(CurrentVolume, dir_window, s->disp_begin_pos, s->disp_begin_pos + s->cursor_pos, TRUE);
       DisplayDiskStatistic(s);
       DisplayAvailBytes(s);
       InitClock(); /* Resume clock before returning */

       return 0;
  } else {
      /* Case B: New Volume Needed or Reuse Virgin Volume */

      /* Save current volume pointer before potentially switching to a new one. */
      old_vol = CurrentVolume;

      /* Check if the current volume (old_vol) is the "virgin" empty volume (login_path is empty). */
      if (old_vol != NULL && old_vol->vol_stats.login_path[0] == '\0') {
          /* Reuse the virgin volume. CurrentVolume remains old_vol. */
          /* No need to call Volume_Create, so is_new_vol_created remains FALSE. */
          /* If the virgin volume already had a tree (e.g., from a previous failed load),
           * we should clear it before loading new data. */
          if (old_vol->vol_stats.tree) {
              DeleteTree(old_vol->vol_stats.tree);
              old_vol->vol_stats.tree = NULL;
          }
          /* Reset other stats for the virgin volume to a clean state */
          memset(&old_vol->vol_stats, 0, sizeof(Statistic));
          old_vol->vol_stats.kind_of_sort = SORT_BY_NAME + SORT_ASC; /* Re-init default sort */
          strcpy(old_vol->vol_stats.file_spec, DEFAULT_FILE_SPEC);   /* Re-init default filter */
          FreeVolumeCache(old_vol); /* Clear any old cache */
      } else {
          /* Create a truly new volume. */
          CurrentVolume = Volume_Create();
          is_new_vol_created = TRUE;
          /* Defensive check, though Volume_Create should exit on failure. */
          if (CurrentVolume == NULL) {
              ERROR_MSG("Failed to create new volume (out of memory)*ABORT");
              /* If we failed to create a new volume, restore old_vol if it existed. */
              if (old_vol != NULL) {
                  CurrentVolume = old_vol;
                  /* Renamed usage: CurrentVolume->vol_stats.mode -> CurrentVolume->vol_stats.login_mode */
                  mode = CurrentVolume->vol_stats.login_mode;
                  s = &CurrentVolume->vol_stats;
                  DisplayMenu();
                  DisplayTree(CurrentVolume, dir_window, s->disp_begin_pos, s->cursor_pos, TRUE);
                  DisplayDiskStatistic(s);
                  DisplayAvailBytes(s);
              }
              InitClock(); /* Resume clock before returning */
              return -1;
          }
          /* Initialize default sort for the brand new volume */
          CurrentVolume->vol_stats.kind_of_sort = SORT_BY_NAME + SORT_ASC;
      }

      s = &CurrentVolume->vol_stats;

      /* CRITICAL: Allocate the root tree node for the current volume if it doesn't exist.
       * This handles both newly created volumes and reusing the virgin volume
       * if its tree was never allocated or was freed due to a previous error. */
      if (!s->tree) {
          s->tree = (DirEntry *)calloc(1, sizeof(DirEntry) + PATH_LENGTH);
          if (s->tree == NULL) {
              ERROR_MSG("Malloc failed for root DirEntry*ABORT");
              if (is_new_vol_created) {
                  Volume_Delete(CurrentVolume); /* Delete the newly created but failed volume */
              } else {
                  /* If reusing virgin volume and tree alloc failed, its state is already cleared by memset. */
                  s->tree = NULL; /* Ensure tree pointer is NULL */
              }
              /* Restore old_vol if we switched away from it and failed. */
              if (old_vol != NULL && CurrentVolume != old_vol) {
                  CurrentVolume = old_vol;
                  /* Renamed usage: CurrentVolume->vol_stats.mode -> CurrentVolume->vol_stats.login_mode */
                  mode = CurrentVolume->vol_stats.login_mode;
                  s = &CurrentVolume->vol_stats;
                  DisplayMenu();
                  DisplayTree(CurrentVolume, dir_window, s->disp_begin_pos, s->cursor_pos, TRUE);
                  DisplayDiskStatistic(s);
                  DisplayAvailBytes(s);
              }
              InitClock(); /* Resume clock before returning */
              return -1;
          }
      }

      /* Set the path for the new/reused volume */
      strncpy(s->login_path, resolved_path, PATH_LENGTH);
      s->login_path[PATH_LENGTH] = '\0';
      strncpy(s->path, resolved_path, PATH_LENGTH);
      s->path[PATH_LENGTH] = '\0';

      /* Reset global search term to prevent context bleeding */
      GlobalSearchTerm[0] = '\0';

      /* Restore the user's active filter from the previous volume ONLY for new volumes.
         This acts as a "default" filter inheritance for convenience. */
      (void) strcpy( s->file_spec, saved_filter );

      /* Moved: statistic.kind_of_sort initialization is now inside is_new_vol_created or virgin check block */

      (void) memcpy( &s->tree->stat_struct, &stat_struct, sizeof( stat_struct ) );

      if( !S_ISDIR(stat_struct.st_mode ) )
      {
        /* "root" node is always a directory */
        (void) memset( (char *) &s->tree->stat_struct, 0, sizeof( struct stat ) );
        s->tree->stat_struct.st_mode = S_IFDIR;
        s->disk_total_directories = 1;
        mode = ARCHIVE_MODE; /* Set global mode for the new volume */
      }
      else if (IsUserActionDefined())
      {
        mode = USER_MODE; /* Set global mode for the new volume */
      }
      else
      {
        mode = DISK_MODE; /* Set global mode for the new volume */
      }
      /* Renamed usage: s->mode -> s->login_mode */
      s->login_mode = mode; /* Store the determined mode in the new volume's stats */


      (void) GetDiskParameter( resolved_path, /* Use resolved_path for disk parameter */
                               s->disk_name,
                               &s->disk_space,
                               &s->disk_capacity,
                               s
                              );

      DisplayMenu();
      DisplayDiskStatistic(s); /* User requested: Draw the stats panel frame immediately */

      if(animation_method == 1) {
          SwitchToBigFileWindow();
          InitAnimation();
      } else {
          RefreshWindow( stdscr );
          RefreshWindow( dir_window );
      }
      doupdate();

      if( mode == ARCHIVE_MODE)
      {
        (void) strcpy( s->tree->name, resolved_path ); /* Use resolved_path for archive name */

#ifdef HAVE_LIBARCHIVE
        if (animation_method == 0) Notice("Scanning archive...");
        /* Pass address of statistic.tree so it can be updated */
        if (ReadTreeFromArchive(&s->tree, s->login_path, s))
        {
            /* Error message will have been displayed by the function */
            result = -1;
        }
        /* Recalculate handled inside ReadTreeFromArchive now */

        if (animation_method == 0) UnmapNoticeWindow();
#else
        ERROR_MSG("Archive support not compiled.*Please install libarchive-dev*and recompile ytree.");
        result = -1;
#endif

      }
      else
      {
        /* For a new volume, the old volume's tree is preserved in old_vol.
         * No need to delete any tree here. */
        (void) strcpy( s->tree->name, resolved_path ); /* Use resolved_path for root dir name */
        s->tree->next = s->tree->prev = NULL;

        depth = strtol(TREEDEPTH, NULL, 0);
        int rt_ret = ReadTree(s->tree, resolved_path, depth, s); /* Use resolved_path for ReadTree */
        if (rt_ret != 0)
        {
            if (rt_ret != -1) {
                /* Only show error if NOT an abort (-1) */
                ERROR_MSG("ReadTree Failed");
            }
            if(animation_method == 1) {
                StopAnimation();
                SwitchToSmallFileWindow();
            }
            result = -1;
        }

        /* Removed: RecalculateSysStats() here. It will be called after SetFilter. */

        /* Copy current statistic to disk_statistic for the new volume */
        (void) memcpy( (char *) &disk_statistic,
                       (char *) s,
                       sizeof( Statistic )
                      );
       }

      /* Error handling for new/reused volume creation/scan */
      if (result != 0) { /* Check for empty tree as well */
          if (is_new_vol_created) {
               Volume_Delete(CurrentVolume);
               if (old_vol != NULL) {
                   CurrentVolume = old_vol;
                   /* Renamed usage: CurrentVolume->vol_stats.mode -> CurrentVolume->vol_stats.login_mode */
                   mode = CurrentVolume->vol_stats.login_mode; /* Restore global mode */
                   s = &CurrentVolume->vol_stats;
                   /* Restore display for the old volume */
                   DisplayMenu();
                   BuildDirEntryList(CurrentVolume); /* Rebuild list for old volume */
                   DisplayTree(CurrentVolume, dir_window, s->disp_begin_pos, s->disp_begin_pos + s->cursor_pos, TRUE);
                   DisplayDiskStatistic(s);
                   DisplayAvailBytes(s);
               } else {
                   /* This case should ideally not be reached if ytree starts with a default volume.
                    * If it is, it means the initial volume failed to load. */
                   endwin();
                   fprintf(stderr, "Critical error: Failed to load initial volume and no previous volume to restore.\n");
                   exit(1);
               }
           } else {
               /* If !is_new_vol_created (reused virgin volume) and ReadTree failed:
                * Clear its state but DO NOT delete the volume structure. */
               if (s->tree) {
                   DeleteTree(s->tree);
                   s->tree = NULL;
               }
               /* Clear other relevant fields for the virgin volume */
               memset(s, 0, sizeof(Statistic)); /* Clear all stats */
               s->kind_of_sort = SORT_BY_NAME + SORT_ASC; /* Re-init default sort */
               strcpy(s->file_spec, DEFAULT_FILE_SPEC);   /* Re-init default filter */
               FreeVolumeCache(CurrentVolume); /* Clear cache for reuse */
               /* CurrentVolume remains the empty virgin volume. */
               DisplayMenu(); /* Refresh to show empty state */
               BuildDirEntryList(CurrentVolume); /* Rebuild list for empty tree */
               DisplayTree(CurrentVolume, dir_window, 0, 0, TRUE);
               DisplayDiskStatistic(s);
               DisplayAvailBytes(s);
           }
           InitClock(); /* Resume clock before returning */
           return -1;
       }

      if(animation_method == 1) {
           StopAnimation();
      }
      SwitchToSmallFileWindow(); /* Force split view mode even if animation was unused */

      (void) SetFilter( s->file_spec, s ); /* This calls ApplyFilter internally */
      RecalculateSysStats(s);              /* Sum up stats based on flags */

      /* Refresh display */
      BuildDirEntryList(CurrentVolume); /* Rebuild list for the newly loaded tree */
      DisplayTree(CurrentVolume, dir_window, 0, 0, TRUE); /* New/reused volume, reset display position */
      DisplayDiskStatistic(s);
      DisplayAvailBytes(s);

      InitClock(); /* Resume clock before returning */

      return( result );
  }
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

    if (InputStringEx(user_input, Y_PROMPT, 6, 0, COLS - 7, PATH_LENGTH - 1, "\r\033", HST_LOGIN) == CR) {
        /*
         * NOTE: The size of temp_path has been increased to prevent potential
         * buffer overflows identified by the -Wformat-truncation compiler warning.
         * The new size accounts for the worst-case concatenation of a base path,
         * a path separator, and the user's input string.
         */
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

        /*
         * Use realpath() to resolve '..' and '.' components. This is robust.
         * If it fails, it could be because the path does not exist (e.g.,
         * logging into a non-existent path for an archive). In that case,
         * fall back to the pure string-based NormPath.
         */
        if (realpath(temp_path, resolved_path) != NULL) {
            strcpy(path, resolved_path);
        } else {
            NormPath(temp_path, path);
        }
        result = 0;
    }

    return (result);
}

/* Helper function to clear big_window flag recursively */
static void RecursiveClearBigWindow(DirEntry *d) {
    if (!d) return;
    d->big_window = FALSE;
    if (d->sub_tree) RecursiveClearBigWindow(d->sub_tree);
    if (d->next) RecursiveClearBigWindow(d->next);
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
    BOOL changes_made = FALSE; /* Fix: Declare it here */

    // Get initial number of volumes to determine max_retries.
    // This ensures we don't loop indefinitely if volumes are repeatedly deleted.
    num_volumes = HASH_COUNT(VolumeList);
    max_retries = num_volumes + 1; // Allow trying all volumes + one wrap-around attempt

    while (retries++ < max_retries) {
        // Re-evaluate num_volumes and re-snapshot the list in each iteration
        // because LoginDisk might delete volumes if they are inaccessible.
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

        current_index = -1; // Reset for each snapshot
        i = 0;
        HASH_ITER(hh, VolumeList, s, tmp) {
            vol_array[i] = s;
            if (s == CurrentVolume) {
                current_index = i;
            }
            i++;
        }

        if (current_index == -1) {
            // This indicates CurrentVolume was somehow removed from VolumeList
            // without being replaced, which is an internal inconsistency.
            ERROR_MSG("Current volume not found in list during cycle.");
            free(vol_array);
            return -1;
        }

        // Calculate target index, handling negative modulo correctly for wrap-around
        target_index = (current_index + direction + num_volumes) % num_volumes;

        // If the calculated target is the current volume, and we've already made
        // at least one attempt (retries > 1), it means we've cycled through
        // all other available volumes and failed to switch. Break the loop.
        if (target_index == current_index && retries > 1) {
            free(vol_array);
            MESSAGE("No other accessible volumes found.");
            return (changes_made ? 0 : -1);
        }

        // Capture target path before freeing vol_array, as per instruction.
        struct Volume *target = vol_array[target_index];
        char target_path[PATH_LENGTH + 1];
        strncpy(target_path, target->vol_stats.login_path, PATH_LENGTH);
        target_path[PATH_LENGTH] = '\0';

        free(vol_array); // Free NOW, before calling LoginDisk.

        // Attempt to switch to the target volume.
        // If LoginDisk returns 0, the switch was successful.
        // If LoginDisk returns -1, the target volume was inaccessible/deleted,
        // and the loop will continue to the next retry.
        if (LoginDisk(target_path) == 0) {
            /* Force UI reset to standard split view on cycle */
            RecursiveClearBigWindow(CurrentVolume->vol_stats.tree); /* Reset big window state recursively */
            ReCreateWindows();
            ClockHandler(0); /* Redraw clock immediately */
            DisplayMenu();
            DisplayTree(CurrentVolume, dir_window, CurrentVolume->vol_stats.disp_begin_pos, CurrentVolume->vol_stats.disp_begin_pos + CurrentVolume->vol_stats.cursor_pos, TRUE);
            DisplayDiskStatistic(&CurrentVolume->vol_stats);
            if(animation_method == 0) SwitchToSmallFileWindow();
            return 0; // Success!
        } else {
             struct Volume *check;
             HASH_FIND_INT(VolumeList, &target->id, check);
             if (check) {
                 Volume_Delete(check);
                 changes_made = TRUE;
             }
        }
    }

    MESSAGE("Failed to switch to an accessible volume after multiple attempts.");
    return (changes_made ? 0 : -1);
}