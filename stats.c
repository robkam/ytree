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
  const char *fmt= "[%-17s]";
  char buff[20];

  *buff = '\0';

  sprintf( buff, fmt, statistic.file_spec);
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




void DisplayFileSpec(void)
{
  mvwprintw( stdscr, 2,  COLS - 18, "%-17s", statistic.file_spec );
  RefreshWindow( stdscr );
}


void DisplayDiskName(void)
{
  char line_buf[40];

  if (mode == DISK_MODE || mode == USER_MODE) {
    /* Format for DISK mode: "DISK: [name_padded_to_17]" */
    snprintf(line_buf, sizeof(line_buf), "DISK: [%-17s]", statistic.disk_name);
  } else {
    /* Format for ARCHIVE mode: "ARCHIVE: [name_padded_to_14]" */
    snprintf(line_buf, sizeof(line_buf), "ARCHIVE: [%-14s]", statistic.disk_name);
  }

  /* Print the fully-formatted string, overwriting the old content */
  PrintMenuOptions(stdscr, 4, COLS - 24, line_buf, CPAIR_MENU, CPAIR_HIMENUS);
  RefreshWindow(stdscr);
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