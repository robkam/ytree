/***************************************************************************
 *
 * log.c
 * Dateibaum lesen
 *
 ***************************************************************************/


#include "ytree.h"
/* #include <sys/wait.h> */  /* maybe wait.h is available */


/* Login Disk liefert
 * -1 bei Fehler
 * 0  bei fehlerfreiem lesen eines neuen Baumes oder bei Benutzung eines Baumes im Speicher
 */


int LoginDisk(char *path)
{
  struct stat stat_struct;
  int    depth;
  int    result = 0;
  char   saved_filter[FILE_SPEC_LENGTH + 1];
  char   resolved_path[PATH_LENGTH + 1];
  struct Volume *s, *tmp;
  struct Volume *found_vol = NULL;
  struct Volume *old_vol = NULL;
  BOOL   is_new_vol_created = FALSE; /* Flag to track if we actually called Volume_Create */

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
    (void) sprintf( message, "Can't access*\"%s\"*%s", resolved_path, strerror(errno) );
    MESSAGE( message );
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
          return -1;
      }
      archive_read_support_filter_all(a_test);
      archive_read_support_format_all(a_test);
      r_test = archive_read_open_filename(a_test, resolved_path, 10240);
      archive_read_free(a_test);

      if (r_test != ARCHIVE_OK) {
          (void) sprintf(message, "Not a recognized archive file*or format not supported*\"%s\"", resolved_path);
          MESSAGE(message);
          return -1;
      }
#else
      /* Fallback for when libarchive is not available */
      (void) sprintf(message, "Cannot open file as archive*ytree not compiled with*libarchive support");
      MESSAGE(message);
      return -1;
#endif
  }


  /* Save current filter to preserve it across transitions.
   * 'statistic' here refers to CurrentVolume->vol_stats via macro. */
  if (CurrentVolume != NULL && strlen(statistic.file_spec) > 0) {
      strncpy(saved_filter, statistic.file_spec, FILE_SPEC_LENGTH);
      saved_filter[FILE_SPEC_LENGTH] = '\0';
  } else {
      strcpy(saved_filter, DEFAULT_FILE_SPEC);
  }


  /* 2. Search Existing Volume */
  HASH_ITER(hh, VolumeList, s, tmp) {
      if (strcmp(s->vol_stats.login_path, resolved_path) == 0) {
          found_vol = s;
          break;
      }
  }

  if (found_vol != NULL) {
      /* Case A: Volume Found - Switch to it */

      /* Verify accessibility of the volume before switching. */
      BOOL access_ok = FALSE;
      struct stat st_check;

      if (found_vol->vol_stats.mode == ARCHIVE_MODE) {
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
                  chdir(parent_dir);
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
          sprintf(error_message_buffer, "Volume \"%s\" not accessible (Error: %s). Removed.",
                found_vol->vol_stats.login_path, strerror(errno));
          MESSAGE(error_message_buffer);
          if (found_vol == CurrentVolume) {
               Volume_Delete(found_vol);
               CurrentVolume = Volume_Create();
               /* ... refresh empty ... */
               DisplayMenu();
               DisplayTree(dir_window, 0, 0);
               DisplayDiskStatistic();
               DisplayAvailBytes();
          } else {
               Volume_Delete(found_vol);
          }
          return -1;
      }

      /* If access_ok, proceed with switching to the found volume. */
      CurrentVolume = found_vol;

      /* Synchronize the global 'mode' variable with the found volume's stored mode. */
      mode = CurrentVolume->vol_stats.mode;

      /* Restore the filter that was active when LoginDisk was called.
       * This ensures the user's filter preference persists across volume switches. */
      (void) strcpy(statistic.file_spec, saved_filter);
      (void) SetFilter(statistic.file_spec);

      /* Refresh display for the switched volume, using its stored display state. */
      DisplayMenu(); /* Updates path at top */
      /* CRITICAL FIX: Rebuild the directory entry list for the new volume's tree
       * before displaying it, to prevent use-after-free issues with stale pointers. */
      BuildDirEntryList(statistic.tree, &statistic);
      DisplayTree(dir_window, statistic.disp_begin_pos, statistic.disp_begin_pos + statistic.cursor_pos);
      DisplayDiskStatistic();
      DisplayAvailBytes();
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
          if (statistic.tree) {
              DeleteTree(statistic.tree);
              statistic.tree = NULL;
          }
          /* Reset other stats for the virgin volume to a clean state */
          memset(&statistic, 0, sizeof(Statistic));
          statistic.kind_of_sort = SORT_BY_NAME + SORT_ASC; /* Re-init default sort */
          strcpy(statistic.file_spec, DEFAULT_FILE_SPEC);   /* Re-init default filter */
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
                  mode = CurrentVolume->vol_stats.mode;
                  DisplayMenu();
                  DisplayTree(dir_window, statistic.disp_begin_pos, statistic.cursor_pos);
                  DisplayDiskStatistic();
                  DisplayAvailBytes();
              }
              return -1;
          }
      }

      /* CRITICAL: Allocate the root tree node for the current volume if it doesn't exist.
       * This handles both newly created volumes and reusing the virgin volume
       * if its tree was never allocated or was freed due to a previous error. */
      if (!statistic.tree) {
          statistic.tree = (DirEntry *)calloc(1, sizeof(DirEntry) + PATH_LENGTH);
          if (statistic.tree == NULL) {
              ERROR_MSG("Malloc failed for root DirEntry*ABORT");
              if (is_new_vol_created) {
                  Volume_Delete(CurrentVolume); /* Delete the newly created but failed volume */
              } else {
                  /* If reusing virgin volume and tree alloc failed, its state is already cleared by memset. */
                  statistic.tree = NULL; /* Ensure tree pointer is NULL */
              }
              /* Restore old_vol if we switched away from it and failed. */
              if (old_vol != NULL && CurrentVolume != old_vol) {
                  CurrentVolume = old_vol;
                  mode = CurrentVolume->vol_stats.mode;
                  DisplayMenu();
                  DisplayTree(dir_window, statistic.disp_begin_pos, statistic.cursor_pos);
                  DisplayDiskStatistic();
                  DisplayAvailBytes();
              }
              return -1;
          }
      }

      /* Set the path for the new/reused volume */
      strncpy(statistic.login_path, resolved_path, PATH_LENGTH);
      statistic.login_path[PATH_LENGTH] = '\0';
      strncpy(statistic.path, resolved_path, PATH_LENGTH);
      statistic.path[PATH_LENGTH] = '\0';

      /* Restore the user's active filter */
      (void) strcpy( statistic.file_spec, saved_filter );

      statistic.kind_of_sort = SORT_BY_NAME + SORT_ASC;
      (void) memcpy( &statistic.tree->stat_struct, &stat_struct, sizeof( stat_struct ) );

      if( !S_ISDIR(stat_struct.st_mode ) )
      {
        /* "root" node is always a directory */
        (void) memset( (char *) &statistic.tree->stat_struct, 0, sizeof( struct stat ) );
        statistic.tree->stat_struct.st_mode = S_IFDIR;
        statistic.disk_total_directories = 1;
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
      statistic.mode = mode; /* Store the determined mode in the new volume's stats */


      (void) GetDiskParameter( resolved_path, /* Use resolved_path for disk parameter */
                               statistic.disk_name,
                               &statistic.disk_space,
                               &statistic.disk_capacity
                             );

      DisplayMenu();

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
        (void) strcpy( statistic.tree->name, resolved_path ); /* Use resolved_path for archive name */

#ifdef HAVE_LIBARCHIVE
        if (animation_method == 0) Notice("Scanning archive...");
        /* Pass address of statistic.tree so it can be updated */
        if (ReadTreeFromArchive(&statistic.tree, statistic.login_path))
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
        (void) strcpy( statistic.tree->name, resolved_path ); /* Use resolved_path for root dir name */
        statistic.tree->next = statistic.tree->prev = NULL;

        depth = strtol(TREEDEPTH, NULL, 0);
        int rt_ret = ReadTree(statistic.tree, resolved_path, depth); /* Use resolved_path for ReadTree */
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

        /* Recalculate stats to respect initial hide_dot_files setting,
           as ReadTree now loads everything. */
        RecalculateSysStats();

        /* Copy current statistic to disk_statistic for the new volume */
        (void) memcpy( (char *) &disk_statistic,
                       (char *) &statistic,
                       sizeof( Statistic )
                     );
      }

      /* Error handling for new/reused volume creation/scan */
      if (result != 0) { /* Check for empty tree as well */
          if (is_new_vol_created) {
              Volume_Delete(CurrentVolume);
              if (old_vol != NULL) {
                  CurrentVolume = old_vol;
                  mode = CurrentVolume->vol_stats.mode; /* Restore global mode */
                  /* Restore display for the old volume */
                  DisplayMenu();
                  BuildDirEntryList(statistic.tree, &statistic); /* Rebuild list for old volume */
                  DisplayTree(dir_window, statistic.disp_begin_pos, statistic.disp_begin_pos + statistic.cursor_pos);
                  DisplayDiskStatistic();
                  DisplayAvailBytes();
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
              if (statistic.tree) {
                  DeleteTree(statistic.tree);
                  statistic.tree = NULL;
              }
              /* Clear other relevant fields for the virgin volume */
              memset(&statistic, 0, sizeof(Statistic)); /* Clear all stats */
              statistic.kind_of_sort = SORT_BY_NAME + SORT_ASC; /* Re-init default sort */
              strcpy(statistic.file_spec, DEFAULT_FILE_SPEC);   /* Re-init default filter */
              /* CurrentVolume remains the empty virgin volume. */
              DisplayMenu(); /* Refresh to show empty state */
              BuildDirEntryList(statistic.tree, &statistic); /* Rebuild list for empty tree */
              DisplayTree(dir_window, 0, 0);
              DisplayDiskStatistic();
              DisplayAvailBytes();
          }
          return -1;
      }

      if(animation_method == 1) {
          StopAnimation();
          SwitchToSmallFileWindow();
      }

      (void) SetFilter( statistic.file_spec );
      /*  SetKindOfSort( statistic.kind_of_sort ); */

      /* Refresh display for the new/reused volume, resetting cursor position. */
      BuildDirEntryList(statistic.tree, &statistic); /* Rebuild list for the newly loaded tree */
      DisplayTree(dir_window, 0, 0); /* New/reused volume, reset display position */
      DisplayDiskStatistic();
      DisplayAvailBytes();

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

    MvAddStr(LINES - 2, 1, "LOG:");

    /* Save the current directory context and set it as default for user input */
    strcpy(current_dir_path, path);
    strcpy(user_input, path);

    if (mode == LL_FILE_MODE && *path == '<') {
        for (cptr = user_input; (*cptr = *(cptr + 1)); cptr++);
        if (user_input[strlen(user_input) - 1] == '>')
            user_input[strlen(user_input) - 1] = '\0';
    }

    if (InputString(user_input, LINES - 2, 6, 0, COLS - 7, "\r\033") == CR) {
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

/*
 * SelectLoadedVolume
 * Displays a list of currently loaded volumes and allows the user to switch between them.
 * Returns 0 on successful switch, -1 on cancel.
 */
int SelectLoadedVolume(void)
{
    struct Volume *s, *tmp;
    struct Volume **vol_array = NULL;
    int num_volumes = 0;
    int max_path_len = 0;
    int i, ch;
    int selected_index = 0;
    int current_volume_index = -1;
    WINDOW *win = NULL;
    int win_height, win_width, win_x, win_y;
    int result = -1; /* Assume cancel by default */
    char title[] = "Select Volume";
    char prompt[] = "Use UP/DOWN to select, ENTER to switch, ESC/q to cancel. D to delete."; // Updated prompt
    BOOL changes_made = FALSE; // ADDED: Flag to track if volumes were added/deleted

    ClearHelp();

START_MENU: // ADDED: Label to restart menu after volume changes
    // Free previous allocation if we are restarting the menu
    if (vol_array != NULL) {
        free(vol_array);
        vol_array = NULL;
    }
    // If window exists, delete it before creating a new one
    if (win != NULL) {
        delwin(win);
        win = NULL;
    }

    // Reset num_volumes and max_path_len for fresh snapshot
    num_volumes = 0;
    max_path_len = 0;
    current_volume_index = -1;
    // selected_index will be re-calculated based on CurrentVolume or default to 0

    num_volumes = HASH_COUNT(VolumeList);

    if (num_volumes == 0) {
        MESSAGE("No volumes currently loaded.");
        // If we deleted the last volume, CurrentVolume should have been reset to a blank one.
        // In this case, we should return 0 to force a refresh of the main screen.
        // If we started with 0 volumes, this is an error.
        return changes_made ? 0 : -1;
    }

    vol_array = (struct Volume **)malloc(num_volumes * sizeof(struct Volume *));
    if (vol_array == NULL) {
        ERROR_MSG("Failed to allocate memory for volume list.");
        return -1;
    }

    i = 0;
    int new_selected_index = 0; // Default to first item
    HASH_ITER(hh, VolumeList, s, tmp) {
        vol_array[i] = s;
        int len = StrVisualLength(s->vol_stats.login_path);
        if (len > max_path_len) {
            max_path_len = len;
        }
        if (s == CurrentVolume) {
            current_volume_index = i;
            new_selected_index = i; // Set selected to current volume
        }
        i++;
    }
    selected_index = new_selected_index; // Update selected_index after array is built
    // Ensure selected_index is within bounds (should be if CurrentVolume is valid)
    if (selected_index >= num_volumes) selected_index = num_volumes - 1;
    if (selected_index < 0) selected_index = 0;


    /* 2. Window Setup */
    /* Minimum height: title (1) + prompt (1) + border (2) + at least 1 item (1) = 5 */
    /* Minimum width: title (strlen) + padding (2) = 15 */
    /* Max path len + " [ ] (current)" + padding */
    win_width = MAXIMUM(strlen(title) + 4, max_path_len + 15); /* 15 for "[*] " + " (current)" + padding */
    win_width = MAXIMUM(win_width, StrVisualLength(prompt) + 4); /* Ensure prompt fits */
    win_width = MINIMUM(win_width, COLS - 4); /* Don't exceed screen width */

    win_height = MINIMUM(LINES - 4, num_volumes + 5); /* 5 for top/bottom border, title, prompt, empty line */
    win_height = MAXIMUM(win_height, 10); /* Minimum 10 lines */

    win_x = (COLS - win_width) / 2;
    win_y = (LINES - win_height) / 2;

    // Create new window (old one was deleted if it existed)
    win = newwin(win_height, win_width, win_y, win_x);
    if (win == NULL) {
        ERROR_MSG("Failed to create window for volume selection.");
        free(vol_array);
        return -1;
    }

    keypad(win, TRUE);
    WbkgdSet(win, COLOR_PAIR(CPAIR_MENU));
    curs_set(0); /* Hide cursor */

    /* 3. Input Loop */
    for (;;) {
        werase(win);
        box(win, 0, 0);
        mvwprintw(win, 1, (win_width - strlen(title)) / 2, "%s", title);
        mvwprintw(win, win_height - 2, (win_width - StrVisualLength(prompt)) / 2, "%s", prompt);

        for (i = 0; i < num_volumes; i++) {
            int y_pos = 3 + i; /* Start listing from y=3 */
            if (y_pos >= win_height - 3) { /* Don't draw over prompt/bottom border */
                break;
            }

            if (i == selected_index) {
                wattron(win, A_REVERSE); /* Highlight selected item */
            } else if (i == current_volume_index) {
                wattron(win, COLOR_PAIR(CPAIR_HIMENUS)); /* Highlight current volume differently */
            }

            const char *path_to_display = vol_array[i]->vol_stats.login_path;
            if (strlen(path_to_display) == 0) {
                path_to_display = "<No Path>";
            }

            mvwprintw(win, y_pos, 2, "[%c] %s",
                      (i == selected_index ? '*' : ' '),
                      path_to_display);

            if (i == current_volume_index && i != selected_index) {
                wattroff(win, COLOR_PAIR(CPAIR_HIMENUS));
            } else if (i == selected_index) {
                wattroff(win, A_REVERSE);
            }
        }
        wrefresh(win);

        ch = WGetch(win);

        switch (ch) {
            case KEY_UP:
                selected_index--;
                if (selected_index < 0) {
                    selected_index = num_volumes - 1;
                }
                break;
            case KEY_DOWN:
                selected_index++;
                if (selected_index >= num_volumes) {
                    selected_index = 0;
                }
                break;
            case LF:
            case CR:
                result = 0; /* Success */
                goto END_LOOP;
            case ESC:
            case 'q':
                result = -1; /* Cancel */
                goto END_LOOP;
            case 'D':
            case 'd':
            case KEY_DC: // Delete key
                if (num_volumes <= 1) {
                    MESSAGE("Cannot release the last volume.");
                    // No need to redraw, loop will do it.
                    break; // break from switch, loop continues to redraw
                }

                if (InputChoise("Release this volume? (Y/N)", "YN\033") == 'Y') {
                    struct Volume *target_vol = vol_array[selected_index];
                    int neighbor_idx = -1;

                    if (target_vol == CurrentVolume) {
                        /* Scenario A: Deleting Current Volume */
                        /* Find a neighbor to switch to */
                        if (num_volumes > 1) {
                            // If selected is 0, try 1. Otherwise, try 0.
                            neighbor_idx = (selected_index == 0) ? 1 : 0;
                            struct Volume *neighbor = vol_array[neighbor_idx];

                            /* Verify neighbor accessibility before switching */
                            // This logic is similar to LoginDisk, but for a neighbor.
                            BOOL neighbor_access_ok = FALSE;
                            struct stat neighbor_st_check;

                            if (neighbor->vol_stats.mode == ARCHIVE_MODE) {
                                if (stat(neighbor->vol_stats.login_path, &neighbor_st_check) == 0 && !S_ISDIR(neighbor_st_check.st_mode)) {
                                    neighbor_access_ok = TRUE;
                                    char neighbor_parent_dir[PATH_LENGTH + 1];
                                    strcpy(neighbor_parent_dir, neighbor->vol_stats.login_path);
                                    char *slash = strrchr(neighbor_parent_dir, FILE_SEPARATOR_CHAR);
                                    if (slash) {
                                        *slash = '\0';
                                        chdir(neighbor_parent_dir);
                                    }
                                }
                            } else {
                                if (chdir(neighbor->vol_stats.login_path) == 0) {
                                    neighbor_access_ok = TRUE;
                                }
                            }

                            if (!neighbor_access_ok) {
                                char error_message_buffer[MESSAGE_LENGTH + 1];
                                sprintf(error_message_buffer, "Neighbor volume \"%s\" not accessible (Error: %s). Removed.",
                                        neighbor->vol_stats.login_path, strerror(errno));
                                MESSAGE(error_message_buffer);
                                Volume_Delete(neighbor); // Delete the inaccessible neighbor
                                changes_made = TRUE;
                                // Need to restart the menu to rebuild the vol_array and window
                                free(vol_array); vol_array = NULL; // Ensure it's freed before goto
                                delwin(win); win = NULL; // Ensure window is deleted
                                touchwin(stdscr); refresh(); // Clear artifacts
                                goto START_MENU; // Restart menu to rebuild list
                            }

                            CurrentVolume = neighbor;
                            mode = CurrentVolume->vol_stats.mode; // Sync global mode
                        } else {
                            // This case should be caught by num_volumes <= 1 check, but defensive.
                            MESSAGE("Cannot release the last volume.");
                            break; // break from switch, loop continues to redraw
                        }
                    }
                    /* Scenario B: Deleting Background Volume (or target_vol is now CurrentVolume's old self) */
                    Volume_Delete(target_vol);
                    changes_made = TRUE;

                    /* Cleanup and restart menu */
                    free(vol_array); vol_array = NULL; // Ensure it's freed before goto
                    delwin(win); win = NULL; // Ensure window is deleted
                    touchwin(stdscr); refresh(); // Clear artifacts
                    goto START_MENU;
                }
                break; // break from switch, loop continues to redraw (if not goto START_MENU)
            default:
                /* Ignore other keys */
                break;
        }
    }

END_LOOP:
    /* 5. Cleanup */
    curs_set(1); /* Restore cursor */
    delwin(win);
    touchwin(stdscr);
    refresh();

    /* 4. Execution (if not cancelled) */
    if (result == 0) {
        struct Volume *target_vol = vol_array[selected_index];
        if (target_vol != CurrentVolume) {
            /* LoginDisk will handle setting CurrentVolume and refreshing display */
            int login_result = LoginDisk(target_vol->vol_stats.login_path);
            free(vol_array); /* Free AFTER LoginDisk returns */
            return login_result;
        } else {
            /* REMOVED: MESSAGE("Already on selected volume."); */
            free(vol_array); /* Free even if no actual switch */
            return 0; /* No actual switch, but not an error */
        }
    }

    free(vol_array); /* Free if cancelled (result != 0) */
    // If changes were made (volumes deleted), return 0 to force main loop to refresh.
    // Otherwise, return original result (0 for switch, -1 for cancel).
    return (result == 0 || changes_made) ? 0 : -1;
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

    // Get initial number of volumes to determine max_retries.
    // This ensures we don't loop indefinitely if volumes are repeatedly deleted.
    num_volumes = HASH_COUNT(VolumeList);
    max_retries = num_volumes + 1; // Allow trying all volumes + one wrap-around attempt

    while (retries++ < max_retries) {
        // Re-evaluate num_volumes and re-snapshot the list in each iteration
        // because LoginDisk might delete volumes if they are inaccessible.
        num_volumes = HASH_COUNT(VolumeList);

        if (num_volumes <= 1) {
            MESSAGE("Only one volume loaded. No cycling possible.");
            return -1;
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
            return -1;
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
            return 0; // Success!
        }
    }

    // If the loop finishes, it means max_retries were exhausted without successfully
    // switching to an accessible volume.
    MESSAGE("Failed to switch to an accessible volume after multiple attempts.");
    return -1;
}