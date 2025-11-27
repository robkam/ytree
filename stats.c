/***************************************************************************
 *
 * stats.c
 * Statistik-Modul
 *
 ***************************************************************************/


#include "ytree.h"



static void PrettyPrintNumber(int y, int x, LONGLONG number);
static void RecalcDir(DirEntry *d);


void DisplayDiskStatistic(void)
{
  /* Fix buffer overflow and screen wrap:
     1. Use a static small buffer for the truncated spec.
     2. Ensure total length [ + spec + ] fits in side panel (18 chars max).
     3. COLS - 18 is the starting X. Width available is 18 chars.
     4. [ + 16 chars + ] = 18 chars.
  */
  char buff[40];
  char trunc_spec[20];

  *buff = '\0';

  /* Truncate the file spec to 16 chars to ensure it fits */
  CutName(trunc_spec, statistic.file_spec, 16);

  snprintf( buff, sizeof(buff), "[%-16s]", trunc_spec);

  PrintMenuOptions( stdscr, 2, COLS - 18, buff, CPAIR_MENU, CPAIR_HIMENUS);
  PrettyPrintNumber( 5,  COLS - 17, statistic.disk_space / (LONGLONG)1024 );
  PrintOptions( stdscr, 7,  COLS - 24, "[DISK Statistics   ]" );
  PrettyPrintNumber( 9, COLS - 17, statistic.disk_total_files );
  PrettyPrintNumber( 10, COLS - 17, statistic.disk_total_bytes );
  PrettyPrintNumber( 12, COLS - 17, statistic.disk_matching_files );
  PrettyPrintNumber( 13, COLS - 17, statistic.disk_matching_bytes );
  PrettyPrintNumber( 15, COLS - 17, statistic.disk_tagged_files );
  PrettyPrintNumber( 16, COLS - 17, statistic.disk_tagged_bytes );
  PrintOptions( stdscr, 17, COLS - 24, "[Current Directory    ]");
  DisplayDiskName();

  /* Path display logic is consolidated in DisplayDirStatistic.
     DisplayDirStatistic will handle filling the line up to the clock window. */
  DisplayDirStatistic(statistic.tree);

  return;
}



void DisplayAvailBytes(void)
{
  PrettyPrintNumber( 5,  COLS - 17, statistic.disk_space / (LONGLONG)1024 );
  RefreshWindow( stdscr );
}




void DisplayFilter(void)
{
  /* Updated to match DisplayDiskStatistic format and safety */
  char buff[40];
  char trunc_spec[20];

  CutName(trunc_spec, statistic.file_spec, 16);
  snprintf( buff, sizeof(buff), "[%-16s]", trunc_spec);

  /* Use PrintMenuOptions to maintain color consistency */
  PrintMenuOptions( stdscr, 2,  COLS - 18, buff, CPAIR_MENU, CPAIR_HIMENUS );
  RefreshWindow( stdscr );
}


void DisplayDiskName(void)
{
  char line_buf[PATH_LENGTH + 64]; /* Safe size for path + labels */
  char display_name_buf[PATH_LENGTH + 1]; /* Buffer for truncated name */
  char vol_part_buf[30]; /* Buffer for "[Vol: X/Y] " */
  const char *source_name = NULL;
  int total_volumes = 0;
  int current_index = 0;
  struct Volume *s, *tmp;
  int offset_x = 1; /* Start printing from column 1 */
  int available_width_for_path;
  int vol_part_len = 0;

  /* 1. Determine the source name for display */
  if (mode == ARCHIVE_MODE) {
    /* In archive mode, display the archive file's path (login_path) */
    if (statistic.login_path[0] != '\0') {
        source_name = statistic.login_path;
    } else if (statistic.tree && statistic.tree->name[0] != '\0') {
        source_name = statistic.tree->name; /* Fallback to archive name if login_path is empty */
    } else {
        source_name = "ARCHIVE"; /* Generic fallback */
    }
  } else { /* DISK_MODE or USER_MODE */
    /* In disk/user mode, display the current directory path */
    if (statistic.path[0] != '\0') {
        source_name = statistic.path;
    } else {
        source_name = "DISK"; /* Generic fallback */
    }
  }

  /* 2. Get volume statistics and format volume part */
  total_volumes = HASH_COUNT(VolumeList);
  if (total_volumes > 0) { /* Even for 1 volume, display "1/1" for consistency */
      int i = 1;
      HASH_ITER(hh, VolumeList, s, tmp) {
          if (s == CurrentVolume) {
              current_index = i;
              break;
          }
          i++;
      }
      if (current_index == 0) current_index = 1; /* Fallback if not found (shouldn't happen) */
      snprintf(vol_part_buf, sizeof(vol_part_buf), "[Vol: %d/%d] ", current_index, total_volumes);
      vol_part_len = strlen(vol_part_buf);
  } else {
      /* Should not happen if CurrentVolume is always set, but for safety */
      strcpy(vol_part_buf, "");
      vol_part_len = 0;
  }

  /* 3. Calculate available width for the path and truncate */
  /* DIR_WINDOW_WIDTH is COLS - 26. We print at offset_x (1).
     So, total usable width for the entire line_buf is DIR_WINDOW_WIDTH - offset_x.
     We need space for vol_part_buf, then " [" and "]".
  */
  available_width_for_path = (DIR_WINDOW_WIDTH - offset_x) - vol_part_len - 2; /* -2 for the path's own [] */
  if (available_width_for_path < 0) available_width_for_path = 0;

  CutPathname(display_name_buf, (char*)source_name, available_width_for_path);

  /* 4. Format the final line_buf */
  snprintf(line_buf, sizeof(line_buf), "%s[%s]", vol_part_buf, display_name_buf);
  line_buf[sizeof(line_buf) - 1] = '\0'; /* Ensure null termination */

  /* 5. Print to dir_window */
  wattrset(dir_window, COLOR_PAIR(CPAIR_WINDIR));
  mvwhline(dir_window, 0, 0, ' ', COLS); /* Clear the top line of dir_window */
  mvwaddstr(dir_window, 0, offset_x, line_buf);
  wattroff(dir_window, COLOR_PAIR(CPAIR_WINDIR)); /* Restore attributes */
}




void DisplayDirStatistic(DirEntry *dir_entry)
{
  char format[20];
  char buffer[PATH_LENGTH + 1];
  char auxbuff[PATH_LENGTH + 1];
  int available_width;

  *auxbuff = *buffer = '\0';

  /* Calculate available width: Start at col 6, end before TIME_WINDOW_X.
     TIME_WINDOW_X is (COLS - 20). */
  available_width = TIME_WINDOW_X - 6;
  if (available_width < 0) available_width = 0;

  (void) sprintf( format, "%%-%ds", available_width );

  if (mode == DISK_MODE || mode == USER_MODE) {
    (void) GetPath( dir_entry, statistic.path );
    if (dir_entry -> not_scanned)
       strcat(statistic.path,"*");
  } else {
    /* In archive mode, top path is the archive file itself */
    (void) strncpy(statistic.path, statistic.login_path, sizeof(statistic.path) - 1);
    statistic.path[sizeof(statistic.path) - 1] = '\0';
  }

  sprintf(auxbuff, format, FormFilename( buffer, statistic.path, available_width));

  Print( stdscr, 0, 6, auxbuff, CPAIR_HIMENUS);

  PrintOptions( stdscr, 7,  COLS - 24, "[DIR Statistics    ]" );
  PrettyPrintNumber( 9, COLS - 17, dir_entry->total_files );
  PrettyPrintNumber( 10, COLS - 17, dir_entry->total_bytes );
  PrettyPrintNumber( 12, COLS - 17, dir_entry->matching_files );
  PrettyPrintNumber( 13, COLS - 17, dir_entry->matching_bytes );
  PrettyPrintNumber( 15, COLS - 17, dir_entry->tagged_files );
  PrettyPrintNumber( 16, COLS - 17, dir_entry->tagged_bytes );
  PrintOptions( stdscr, 17, COLS - 24, "[Current File        ]" );
  RefreshWindow( stdscr );
  return;
}





void DisplayDirTagged(DirEntry *dir_entry)
{
  PrettyPrintNumber( 15, COLS - 17, dir_entry->tagged_files );
  PrettyPrintNumber( 16, COLS - 17, dir_entry->tagged_bytes );

  RefreshWindow( stdscr );
}



void DisplayDiskTagged(void)
{
  PrettyPrintNumber( 15, COLS - 17, statistic.disk_tagged_files );
  PrettyPrintNumber( 16, COLS - 17, statistic.disk_tagged_bytes );

  RefreshWindow( stdscr );
}



void DisplayDirParameter(DirEntry *dir_entry)
{
  char *p, *f;
  char format[20];
  char buffer[PATH_LENGTH + 1];
  char auxbuff[PATH_LENGTH + 1];
  char display_path[PATH_LENGTH + 1];
  int available_width;

  p = strrchr( dir_entry->name, FILE_SEPARATOR_CHAR );

  if( p == NULL ) f = dir_entry->name;
  else            f = p + 1;

  /* Also fix blinking here by respecting clock window boundaries */
  available_width = TIME_WINDOW_X - 6;
  if (available_width < 0) available_width = 0;

  (void) sprintf( format, "%%-%ds", available_width );

  if (mode == DISK_MODE || mode == USER_MODE) {
    (void) GetPath( dir_entry, display_path );
    if (dir_entry -> not_scanned)
       strcat(display_path,"*");
  } else {
    /* In archive mode, top path is the real path to the archive file */
    (void) strncpy(display_path, statistic.login_path, sizeof(display_path) - 1);
    display_path[sizeof(display_path) - 1] = '\0';
  }

  sprintf(auxbuff, format, FormFilename(buffer,display_path, available_width));

  Print( stdscr, 0, 6, auxbuff, CPAIR_HIMENUS);
  *auxbuff = '\0';

  /* The current dir name (bottom right) shows the virtual path inside the archive */
  GetPath(dir_entry, buffer);
  sprintf(auxbuff, "[%-20s]", CutFilename(buffer, buffer, 20));

  PrintMenuOptions( stdscr, 18, COLS - 22, auxbuff, CPAIR_MENU, CPAIR_HIMENUS);
  PrettyPrintNumber( 19, COLS - 17, (LONGLONG) dir_entry->total_bytes );
  RefreshWindow( stdscr );
}







void DisplayGlobalFileParameter(FileEntry *file_entry)
{
  char buffer1[PATH_LENGTH+1];
  char buffer2[PATH_LENGTH+3];
  char format[20];
  char display_path[PATH_LENGTH + 1];
  int available_width;

  /* Fix blinking here as well */
  available_width = TIME_WINDOW_X - 6;
  if (available_width < 0) available_width = 0;

  (void) sprintf( format, "[%%-%ds]", available_width );

  if (mode == DISK_MODE || mode == USER_MODE) {
    (void) GetPath( file_entry->dir_entry, display_path );
  } else {
      (void) strncpy(display_path, statistic.login_path, sizeof(display_path) - 1);
      display_path[sizeof(display_path) - 1] = '\0';
  }

  FormFilename( buffer2, display_path, available_width );
  sprintf(buffer1, format, buffer2);

  PrintMenuOptions( stdscr, 0, 6, buffer1, CPAIR_GLOBAL, CPAIR_HIGLOBAL);

  /* Current file name still shows just the filename */
  CutFilename( buffer1, file_entry->name, 20 );
  sprintf( buffer2, "[%-20s]", buffer1 );
  PrintMenuOptions( stdscr, 18, COLS - 22, buffer2, CPAIR_GLOBAL, CPAIR_HIGLOBAL);
  PrettyPrintNumber( 19, COLS - 17, (LONGLONG) file_entry->stat_struct.st_size );
  RefreshWindow( stdscr );
}




void DisplayFileParameter(FileEntry *file_entry)
{
  char buffer[21*6];
  char auxbuff[23*6];
  sprintf( auxbuff, "[%-20s]", CutFilename( buffer, file_entry->name, 20 ) );
  PrintMenuOptions( stdscr, 18, COLS - 22, auxbuff, CPAIR_MENU, CPAIR_HIMENUS);
  PrettyPrintNumber( 19, COLS - 17, (LONGLONG)file_entry->stat_struct.st_size );
  RefreshWindow( stdscr );
}



static void PrettyPrintNumber(int y, int x, LONGLONG number)
{
  char buffer[40];
  long terra, giga, mega, kilo, one;

  *buffer = 0;
  terra    = (long)   ( number / (LONGLONG) 1000000000000 );
  giga     = (long) ( ( number % (LONGLONG) 1000000000000 ) / (LONGLONG) 1000000000 );
  mega     = (long) ( ( number % (LONGLONG) 1000000000 ) / (LONGLONG) 1000000 );
  kilo     = (long) ( ( number % (LONGLONG) 1000000 ) / (LONGLONG) 1000 );
  one      = (long)   ( number % (LONGLONG) 1000 );

  if( terra ){
     /* "123123123123123" */
     sprintf( buffer, "[%3ld%3ld%03ld%03ld%03ld]", terra, giga, mega, kilo, one );
     PrintMenuOptions( stdscr, y, x, buffer, CPAIR_MENU, CPAIR_HIMENUS);
     }
  if( giga ){
     /* "123,123,123,123" */
     sprintf( buffer, "[%3ld%c%03ld%c%03ld%c%03ld]", giga, number_seperator, mega, number_seperator, kilo, number_seperator, one );
     PrintMenuOptions( stdscr, y, x, buffer, CPAIR_MENU, CPAIR_HIMENUS);
     }
  else if( mega ) {
     /* "    123,123,123" */
     sprintf( buffer, "[    %3ld%c%03ld%c%03ld]", mega, number_seperator, kilo,  number_seperator, one );
     PrintMenuOptions( stdscr, y, x, buffer, CPAIR_MENU, CPAIR_HIMENUS);
     }
  else if( kilo ) {
     /* "        123,123" */
     sprintf( buffer, "[        %3ld%c%03ld]", kilo, number_seperator, one);
     PrintMenuOptions( stdscr, y, x, buffer, CPAIR_MENU, CPAIR_HIMENUS);
     }
  else {
     /* "            123" */
     sprintf( buffer, "[            %3ld]", one);
     PrintMenuOptions( stdscr, y, x, buffer, CPAIR_MENU, CPAIR_HIMENUS);
     }
 return;
}


static void RecalcDir(DirEntry *d) {
    FileEntry *f;
    DirEntry *sub;

    /* Reset directory specific stats */
    d->total_files = 0;
    d->total_bytes = 0;
    d->matching_files = 0;
    d->matching_bytes = 0;
    d->tagged_files = 0;
    d->tagged_bytes = 0;

    /* Sum files in this directory */
    for (f = d->file; f; f = f->next) {
        /* Skip dot files if hidden */
        if (hide_dot_files && f->name[0] == '.') continue;

        d->total_files++;
        d->total_bytes += f->stat_struct.st_size;

        if (f->matching) {
            d->matching_files++;
            d->matching_bytes += f->stat_struct.st_size;
        }
        if (f->tagged) {
             d->tagged_files++;
             d->tagged_bytes += f->stat_struct.st_size;
        }
    }

    /* Recursively handle sub-trees if this directory is visible */
    /* Process children */
    sub = d->sub_tree;
    while (sub) {
        /* Check visibility of sub: skip dot directories if hidden */
        if ( !(hide_dot_files && sub->name[0] == '.') ) {
            /* Recurse first to calculate child stats */
            RecalcDir(sub);

            /* Add this visible child directory to global stats */
            /* Note: RecalcDir(sub) has already added sub's children to globals
               but here we ensure 'sub' itself is counted in directory count?
               ReadTree increments disk_total_directories as it creates them.
               We are recalculating global totals from scratch. */
            statistic.disk_total_directories++;
        }
        sub = sub->next;
    }

    /* Add this directory's file stats to global stats (if d itself is visible,
       checked by caller, except root which is always visible) */
    statistic.disk_total_files += d->total_files;
    statistic.disk_total_bytes += d->total_bytes;
    statistic.disk_matching_files += d->matching_files;
    statistic.disk_matching_bytes += d->matching_bytes;
    statistic.disk_tagged_files += d->tagged_files;
    statistic.disk_tagged_bytes += d->tagged_bytes;
}


void RecalculateSysStats(void) {
    statistic.disk_total_files = 0;
    statistic.disk_total_bytes = 0;
    statistic.disk_matching_files = 0;
    statistic.disk_matching_bytes = 0;
    statistic.disk_tagged_files = 0;
    statistic.disk_tagged_bytes = 0;
    statistic.disk_total_directories = 0;

    if (statistic.tree) {
        /* statistic.tree (root) is always visible */
        statistic.disk_total_directories++; /* Count root */
        RecalcDir(statistic.tree);
    }
}