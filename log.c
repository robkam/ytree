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
  if (strlen(statistic.file_spec) > 0) {
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
      /* Case A: Volume Found */
      CurrentVolume = found_vol;

      /* Synchronize the global 'mode' variable with the found volume's stored mode. */
      mode = CurrentVolume->vol_stats.mode;

      /* Restore the filter that was active when LoginDisk was called.
       * This ensures the user's filter preference persists across volume switches. */
      (void) strcpy(statistic.file_spec, saved_filter);
      (void) SetFilter(statistic.file_spec);

      /* Refresh display for the switched volume, using its stored display state. */
      DisplayMenu(); /* Updates path at top */
      DisplayTree(dir_window, statistic.disp_begin_pos, statistic.disp_begin_pos + statistic.cursor_pos);
      DisplayDiskStatistic();
      DisplayAvailBytes();
      return 0;
  } else {
      /* Case B: New Volume Needed */

      /* Save current volume pointer before switching to a new one.
       * This allows restoration if the new volume fails to load. */
      old_vol = CurrentVolume;

      /* Create new volume. Volume_Create exits on OOM, so CurrentVolume should not be NULL here. */
      CurrentVolume = Volume_Create();
      /* Defensive check, though Volume_Create should exit on failure. */
      if (CurrentVolume == NULL) {
          ERROR_MSG("Failed to create new volume (out of memory)*ABORT");
          if (old_vol != NULL) {
              CurrentVolume = old_vol; /* Restore old volume */
              mode = CurrentVolume->vol_stats.mode; /* Restore global mode */
              DisplayMenu();
              DisplayTree(dir_window, statistic.disp_begin_pos, statistic.disp_begin_pos + statistic.cursor_pos);
              DisplayDiskStatistic();
              DisplayAvailBytes();
          }
          return -1;
      }

      /* CRITICAL: Allocate the root tree node for the new volume.
       * Allocate extra space for the flexible array member 'name' to hold the full path. */
      statistic.tree = (DirEntry *)calloc(1, sizeof(DirEntry) + PATH_LENGTH);
      if (statistic.tree == NULL) {
          ERROR_MSG("Malloc failed for root DirEntry*ABORT");
          Volume_Delete(CurrentVolume); /* Delete the newly created but failed volume */
          if (old_vol != NULL) {
              CurrentVolume = old_vol; /* Restore old volume */
              mode = CurrentVolume->vol_stats.mode; /* Restore global mode */
              DisplayMenu();
              DisplayTree(dir_window, statistic.disp_begin_pos, statistic.disp_begin_pos + statistic.cursor_pos);
              DisplayDiskStatistic();
              DisplayAvailBytes();
          }
          return -1;
      }

      /* Set the path for the new volume */
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
        if( ReadTree( statistic.tree, resolved_path, depth ) ) /* Use resolved_path for ReadTree */
        {
          ERROR_MSG( "ReadTree Failed" );
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

      /* Error handling for new volume creation/scan */
      if (result != 0 || statistic.tree->file == NULL) { /* Check for empty tree as well */
          Volume_Delete(CurrentVolume);
          if (old_vol != NULL) {
              CurrentVolume = old_vol;
              mode = CurrentVolume->vol_stats.mode; /* Restore global mode */
              /* Restore display for the old volume */
              DisplayMenu();
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
          return -1;
      }

      if(animation_method == 1) {
          StopAnimation();
          SwitchToSmallFileWindow();
      }

      (void) SetFilter( statistic.file_spec );
      /*  SetKindOfSort( statistic.kind_of_sort ); */

      /* Refresh display for the new volume, resetting cursor position. */
      DisplayTree(dir_window, 0, 0); /* New volume, reset display position */
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
    char prompt[] = "Use UP/DOWN to select, ENTER to switch, ESC/q to cancel.";

    ClearHelp();

    /* 1. Snapshot Volumes */
    num_volumes = HASH_COUNT(VolumeList);

    if (num_volumes == 0) {
        MESSAGE("No volumes currently loaded.");
        return -1;
    }

    vol_array = (struct Volume **)malloc(num_volumes * sizeof(struct Volume *));
    if (vol_array == NULL) {
        ERROR_MSG("Failed to allocate memory for volume list.");
        return -1;
    }

    i = 0;
    HASH_ITER(hh, VolumeList, s, tmp) {
        vol_array[i] = s;
        int len = StrVisualLength(s->vol_stats.login_path);
        if (len > max_path_len) {
            max_path_len = len;
        }
        if (s == CurrentVolume) {
            current_volume_index = i;
            selected_index = i; /* Start selection on current volume */
        }
        i++;
    }

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

            mvwprintw(win, y_pos, 2, "[%c] %s",
                      (i == selected_index ? '*' : ' '),
                      vol_array[i]->vol_stats.login_path);

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
            default:
                /* Ignore other keys */
                break;
        }
    }

END_LOOP:
    /* 5. Cleanup */
    curs_set(1); /* Restore cursor */
    delwin(win);
    free(vol_array);
    touchwin(stdscr);
    refresh();

    /* 4. Execution (if not cancelled) */
    if (result == 0) {
        struct Volume *target_vol = vol_array[selected_index];
        if (target_vol != CurrentVolume) {
            /* LoginDisk will handle setting CurrentVolume and refreshing display */
            return LoginDisk(target_vol->vol_stats.login_path);
        } else {
            MESSAGE("Already on selected volume.");
            return 0; /* No actual switch, but not an error */
        }
    }

    return result;
}