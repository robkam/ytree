/***************************************************************************
 *
 * dirwin.c
 * Functions for handling the directory window
 *
 ***************************************************************************/


#include "ytree.h"
#include "watcher.h"


static int current_dir_entry;

static void ReadDirList(DirEntry *dir_entry, struct Volume *vol);
static void PrintDirEntry(struct Volume *vol, WINDOW *win, int entry_no, int y, unsigned char hilight);
void BuildDirEntryList(struct Volume *vol);
void FreeDirEntryList(void);
void FreeVolumeCache(struct Volume *vol);
static void HandleReadSubTree(DirEntry *dir_entry, BOOL *need_dsp_help, Statistic *s);
static void HandleUnreadSubTree(DirEntry *dir_entry, DirEntry *de_ptr, BOOL *need_dsp_help, Statistic *s);
static void MoveEnd(DirEntry **dir_entry, Statistic *s);
static void MoveHome(DirEntry **dir_entry, Statistic *s);
static void HandlePlus(DirEntry *dir_entry, DirEntry *de_ptr, char *new_login_path, BOOL *need_dsp_help, Statistic *s);
static void HandleTagDir(DirEntry *dir_entry, BOOL value, Statistic *s);
static void HandleTagAllDirs(struct Volume *vol, DirEntry *dir_entry, BOOL value, Statistic *s);
static void HandleShowAll(BOOL tagged_only, DirEntry *dir_entry, BOOL *need_dsp_help, int *ch, Statistic *s);
static void HandleSwitchWindow(DirEntry *dir_entry, BOOL *need_dsp_help, int *ch, Statistic *s);
static void Movedown(int *disp_begin_pos, int *cursor_pos, DirEntry **dir_entry, Statistic *s);
static void Moveup(int *disp_begin_pos, int *cursor_pos, DirEntry **dir_entry, Statistic *s);
static void Movenpage(int *disp_begin_pos, int *cursor_pos, DirEntry **dir_entry, Statistic *s);
static void Moveppage(int *disp_begin_pos, int *cursor_pos, DirEntry **dir_entry, Statistic *s);

static int dir_mode;


void BuildDirEntryList(struct Volume *vol)
{
  if( vol->dir_entry_list )
  {
    free( vol->dir_entry_list );
    vol->dir_entry_list = NULL;
    vol->dir_entry_list_capacity = 0;
  }

  /* Initialize with the estimated count, but enforce a minimum safety size */
  size_t alloc_count = vol->vol_stats.disk_total_directories;
  if (alloc_count < 16) alloc_count = 16;

  if( ( vol->dir_entry_list = ( DirEntryList *)
                   calloc( alloc_count,
                   sizeof( DirEntryList )
                 ) ) == NULL )
  {
    ERROR_MSG( "Calloc Failed*ABORT" );
    exit( 1 );
  }

  vol->dir_entry_list_capacity = alloc_count;
  current_dir_entry = 0;

  /* Only read if we have a valid tree structure */
  if (vol->vol_stats.tree) {
      ReadDirList( vol->vol_stats.tree, vol );
  }

  vol->total_dirs = current_dir_entry;

#ifdef DEBUG
  if(vol->vol_stats.disk_total_directories != vol->total_dirs)
  {
      /* mismatch detected, but safely handled by realloc in ReadDirList */
  }
#endif
}

/*
 * Frees the memory allocated for the dir_entry_list array of a volume.
 */
void FreeVolumeCache(struct Volume *vol)
{
    if (vol && vol->dir_entry_list != NULL)
    {
        free(vol->dir_entry_list);
        vol->dir_entry_list = NULL;
        vol->dir_entry_list_capacity = 0;
        vol->total_dirs = 0;
    }
}

/*
 * Frees the memory allocated for the current volume's dir_entry_list.
 * Retained for compatibility.
 */
void FreeDirEntryList(void)
{
    if (CurrentVolume)
    {
        FreeVolumeCache(CurrentVolume);
    }
}

static void RotateDirMode(void)
{
  switch( dir_mode )
  {
    case MODE_1 : dir_mode = MODE_2 ; break;
    case MODE_2 : dir_mode = MODE_4 ; break;
    case MODE_3 : dir_mode = MODE_1 ; break;
    case MODE_4 : dir_mode = MODE_3 ; break;
  }
  if( (mode != DISK_MODE && mode != USER_MODE ) &&
      dir_mode == MODE_4 ) RotateDirMode();
}



static void ReadDirList(DirEntry *dir_entry, struct Volume *vol)
{
  DirEntry    *de_ptr;
  static int  level = 0;
  static unsigned long indent = 0L;

  for( de_ptr = dir_entry; de_ptr; de_ptr = de_ptr->next )
  {
    /* Check visibility.
       Hide files starting with '.' if option is set.
       EXCEPTION: Never hide the top-level root directory of the current session.
    */
    if (hide_dot_files && de_ptr->name[0] == '.') {
        if (de_ptr != vol->vol_stats.tree)
            continue;
    }

    /* Bounds Checking & Dynamic Reallocation */
    if (current_dir_entry >= (int)vol->dir_entry_list_capacity) {
        size_t new_capacity = vol->dir_entry_list_capacity * 2;
        if (new_capacity == 0) new_capacity = 128; /* Fallback if 0 start */

        DirEntryList *new_list = (DirEntryList *) realloc(vol->dir_entry_list, new_capacity * sizeof(DirEntryList));

        if (new_list == NULL) {
             ERROR_MSG("Realloc failed in ReadDirList*ABORT");
             exit(1);
        }

        /* Zero out the newly allocated portion to maintain calloc-like safety */
        memset(new_list + vol->dir_entry_list_capacity, 0, (new_capacity - vol->dir_entry_list_capacity) * sizeof(DirEntryList));

        vol->dir_entry_list = new_list;
        vol->dir_entry_list_capacity = new_capacity;
    }

    indent &= ~( 1L << level );
    if( de_ptr->next ) indent |= ( 1L << level );

    vol->dir_entry_list[current_dir_entry].dir_entry = de_ptr;
    vol->dir_entry_list[current_dir_entry].level = (unsigned short) level;
    vol->dir_entry_list[current_dir_entry].indent = indent;

    current_dir_entry++;

    if( !de_ptr->not_scanned && de_ptr->sub_tree )
    {
      level++;
      ReadDirList( de_ptr->sub_tree, vol );
      level--;
    }
  }
}


/*
 * Helper function to return the currently selected directory entry.
 * Now takes a Volume context.
 */
DirEntry *GetSelectedDirEntry(struct Volume *vol)
{
  if (vol->dir_entry_list != NULL && vol->total_dirs > 0) {
    int idx = vol->vol_stats.disp_begin_pos + vol->vol_stats.cursor_pos;

    /* Safety bounds check */
    if (idx < 0) idx = 0;
    if (idx >= vol->total_dirs) idx = vol->total_dirs - 1;

    return vol->dir_entry_list[idx].dir_entry;
  }
  /* Fallback to root if list is empty/invalid */
  return vol->vol_stats.tree;
}


static void PrintDirEntry(struct Volume *vol,
                          WINDOW *win,
                          int entry_no,
                          int y,
                          unsigned char hilight)
{
  unsigned int j;
  int  color;
  char graph_buffer[PATH_LENGTH+1];
  char format[60];
  char *line_buffer = NULL;
  char *dir_name;
  char attributes[11];
  char modify_time[20]; /* Increased from 13 to 20 for "YYYY-MM-DD HH:MM" */
  char change_time[20]; /* Increased from 13 to 20 */
  char access_time[20]; /* Increased to 20 */
  char owner[OWNER_NAME_MAX + 1];
  char group[GROUP_NAME_MAX + 1];
  char *owner_name_ptr;
  char *group_name_ptr;
  DirEntry *de_ptr;


  if(win == f2_window) {
    color     = CPAIR_HST;
  } else {
    color     = CPAIR_DIR;
  }

  /* Build the tree graph string (e.g., "| 6- ") */
  graph_buffer[0] = '\0';
  for(j=0; j < vol->dir_entry_list[entry_no].level; j++)
  {
    if( vol->dir_entry_list[entry_no].indent & ( 1L << j ) )
      (void) strcat( graph_buffer, "| " );
    else
      (void) strcat( graph_buffer, "  " );
  }
  de_ptr = vol->dir_entry_list[entry_no].dir_entry;
  if( de_ptr->next )
    (void) strcat( graph_buffer, "6-" );
  else
    (void) strcat( graph_buffer, "3-" );

  /* Build the attribute string based on the current directory mode */
  switch( dir_mode )
  {
    case MODE_1 :
                 (void)GetAttributes(de_ptr->stat_struct.st_mode, attributes);
                 (void)CTime( de_ptr->stat_struct.st_mtime, modify_time );
                 /* Increased buffer size from 38 to 42 to accommodate 16-char date */
                 if ((line_buffer = (char *) malloc(42)) == NULL)
                 {
                    ERROR_MSG("malloc() Failed*Abort");
                    exit(1);
                 }
#ifdef HAS_LONGLONG
                 /* Updated %12s to %16s for date */
                 (void) strcpy( format, "%10s %3d %8lld %16s");

		 (void) snprintf( line_buffer, 42, format, attributes,
                                 (int)de_ptr->stat_struct.st_nlink,
                                 (LONGLONG) de_ptr->stat_struct.st_size,
                                 modify_time
                                 );
#else
                 (void) strcpy( format, "%10s %3d %8d %16s");
                 (void) snprintf( line_buffer, 42, format, attributes,
                                 (int)de_ptr->stat_struct.st_nlink,
                                 (int)de_ptr->stat_struct.st_size,
                                 modify_time
                                 );
#endif
		 break;
    case MODE_2 :
                 (void)GetAttributes(de_ptr->stat_struct.st_mode, attributes);
                 owner_name_ptr = GetDisplayPasswdName(de_ptr->stat_struct.st_uid);
                 group_name_ptr = GetDisplayGroupName(de_ptr->stat_struct.st_gid);
                 if( owner_name_ptr == NULL )
                 {
                    (void)snprintf(owner, sizeof(owner), "%d",(int)de_ptr->stat_struct.st_uid);
                    owner_name_ptr = owner;
                 }
                 if( group_name_ptr == NULL )
                 {
                     (void) snprintf( group, sizeof(group), "%d", (int) de_ptr->stat_struct.st_gid );
                     group_name_ptr = group;
                 }
                 if ((line_buffer = (char *) malloc(40)) == NULL)
                 {
                    ERROR_MSG("malloc() Failed*Abort");
                    exit(1);
                 }
                 (void) strcpy( format, "%12u  %-12s %-12s");
                 (void) snprintf( line_buffer, 40, format,
                                 (unsigned int)de_ptr->stat_struct.st_ino,
                                 owner_name_ptr,
                                 group_name_ptr);
                 break;
    case MODE_3 : /* No attributes, line_buffer remains NULL */
                 break;
    case MODE_4 :
                 (void) CTime( de_ptr->stat_struct.st_ctime, change_time );
                 (void) CTime( de_ptr->stat_struct.st_atime, access_time );
                 /* Increased buffer size from 40 to 50 to accommodate two 16-char dates */
                 /* Format: "Chg.: " (6) + 16 + "  Acc.: " (8) + 16 = 46 chars. 50 is safe. */
                 (void) strcpy( format, "Chg.: %16s  Acc.: %16s");
                 if ((line_buffer = (char *) malloc(50)) == NULL)
                 {
                    ERROR_MSG("malloc() Failed*Abort");
                    exit(1);
                 }
                 (void)snprintf(line_buffer, 50, format, change_time, access_time);
                 break;
  }

  /* --- Redesigned Drawing Logic --- */

  int attr_start_col = 38; /* Column where attributes begin */
  int graph_len = strlen(graph_buffer);
  chtype line_attr;

  wmove(win, y, 0);
  wclrtoeol(win);

  /* Set the base attribute for the line */
#ifdef COLOR_SUPPORT
  line_attr = COLOR_PAIR(color);
#else
  line_attr = A_NORMAL;
#endif
  wattron(win, line_attr);

  /* If full line highlight is enabled, turn on reverse now. */
  if (hilight && highlight_full_line) {
      wattron(win, A_REVERSE);
  }

  /* Part 1: Draw the tree graph characters manually */
  wmove(win, y, 0);
  for (j = 0; j < (unsigned int)graph_len; ++j) {
      int ch;
      switch (graph_buffer[j]) {
          case '6': ch = ACS_LTEE; break;
          case '3': ch = ACS_LLCORNER; break;
          case '|': ch = ACS_VLINE; break;
          case '-': ch = ACS_HLINE; break;
          default:  ch = graph_buffer[j]; break;
      }
      waddch(win, ch | A_BOLD); /* Keep graph characters bold */
  }

  /* Part 2: Prepare and draw the directory name */
  char name_buffer[PATH_LENGTH + 2];
  dir_name = de_ptr->name;
  (void) strcpy( name_buffer, ( *dir_name ) ? dir_name : "." );
  if( de_ptr->not_scanned ) {
    (void) strcat( name_buffer, "/" );
  }

  /* Calculate maximum allowed name length based on mode and window width */
  int max_name_len;
  if (dir_mode == MODE_3) {
      /* In MODE_3 (name-only), truncate based on full window width */
      max_name_len = layout.dir_win_width - graph_len - 1;
  } else {
      /* In other modes, truncate to prevent overlap with attributes */
      max_name_len = attr_start_col - graph_len - 1;
  }
  /* Safety: Ensure max_name_len is at least 1 to avoid issues with CutName */
  if (max_name_len < 1) {
      max_name_len = 1;
  }

  /* Apply truncation if the name is too long */
  if ((int)strlen(name_buffer) > max_name_len) {
      char temp_name[PATH_LENGTH + 2];
      CutName(temp_name, name_buffer, max_name_len);
      strcpy(name_buffer, temp_name);
  }

  /* If name-only highlight, toggle reverse just for the name. */
  if (hilight && !highlight_full_line) {
      wattron(win, A_REVERSE);
  }
  mvwaddstr(win, y, graph_len, name_buffer);
  if (hilight && !highlight_full_line) {
      wattroff(win, A_REVERSE);
  }


  /* Part 3: Draw attributes and fill the gap in between */
  if (line_buffer) {
      int current_x;
      getyx(win, y, current_x); /* Use y (parameter) as dummy output */
      for (int i = current_x; i < attr_start_col; ++i) {
          waddch(win, ' ');
      }
      mvwaddstr(win, y, attr_start_col, line_buffer);
  }

  /* Turn off attributes */
  if (hilight && highlight_full_line) {
      wattroff(win, A_REVERSE);
  }
  wattroff(win, line_attr);

  if (line_buffer)
     free(line_buffer);
}




void DisplayTree(struct Volume *vol, WINDOW *win, int start_entry_no, int hilight_no)
{
  int i, y;
  int width, height;

  y = -1;
  getmaxyx(win, height, width);

#ifdef COLOR_SUPPORT
  if (win == f2_window) {
    WbkgdSet(win, COLOR_PAIR(CPAIR_WINHST));
  } else {
    WbkgdSet(win, COLOR_PAIR(CPAIR_WINDIR));
  }
#endif
  werase(win);

  if (win == f2_window) {
      box(win, 0, 0);
  }

  for(i=0; i < height; i++)
  {
    if ( start_entry_no + i >= vol->total_dirs ) break;

    if( start_entry_no + i != hilight_no )
      PrintDirEntry( vol, win, start_entry_no + i, i, FALSE );
    else
      y = i;
  }

  if( y >= 0 ) PrintDirEntry( vol, win, start_entry_no + y, y, TRUE );

}

static void Movedown(int *disp_begin_pos, int *cursor_pos, DirEntry **dir_entry, Statistic *s)
{
   if( *disp_begin_pos + *cursor_pos + 1 >= CurrentVolume->total_dirs )
   {
      /* Element not present */
      /*-------------------------*/
      return;
   }

    if (*cursor_pos + 1 < layout.dir_win_height) {
        (*cursor_pos)++;
    } else {
        (*disp_begin_pos)++;
    }
    *dir_entry = CurrentVolume->dir_entry_list[*disp_begin_pos + *cursor_pos].dir_entry;

    if (GlobalView->refresh_mode & REFRESH_ON_NAV) {
        *dir_entry = RefreshTreeSafe(*dir_entry);
        /* Re-sync *dir_entry to global stats which RefreshTreeSafe might have adjusted */
        *dir_entry = CurrentVolume->dir_entry_list[*disp_begin_pos + *cursor_pos].dir_entry;
    }

    (*dir_entry)->start_file = 0;
    (*dir_entry)->cursor_pos = -1;
    DisplayTree(CurrentVolume, dir_window, *disp_begin_pos, *disp_begin_pos + *cursor_pos);
    DisplayFileWindow( *dir_entry, file_window );
    RefreshWindow( file_window );
    DisplayDirStatistic(*dir_entry, NULL, s);
    /* Update header path */
    {
        char path[PATH_LENGTH];
        GetPath(*dir_entry, path);
        DisplayHeaderPath(path);
    }
}


static void Moveup(int *disp_begin_pos, int *cursor_pos, DirEntry **dir_entry, Statistic *s)
{
   if( *disp_begin_pos + *cursor_pos - 1 < 0 )
   {
      /* Element not present */
      /*-------------------------*/
      return;
   }

    if (*cursor_pos - 1 >= 0) {
        (*cursor_pos)--;
    } else {
        (*disp_begin_pos)--;
    }
    *dir_entry = CurrentVolume->dir_entry_list[*disp_begin_pos + *cursor_pos].dir_entry;

    if (GlobalView->refresh_mode & REFRESH_ON_NAV) {
        *dir_entry = RefreshTreeSafe(*dir_entry);
        /* Re-sync *dir_entry to global stats which RefreshTreeSafe might have adjusted */
        *dir_entry = CurrentVolume->dir_entry_list[*disp_begin_pos + *cursor_pos].dir_entry;
    }

    (*dir_entry)->start_file = 0;
    (*dir_entry)->cursor_pos = -1;
    DisplayTree(CurrentVolume, dir_window, *disp_begin_pos, *disp_begin_pos + *cursor_pos);
    DisplayFileWindow( *dir_entry, file_window );
    RefreshWindow( file_window );
    DisplayDirStatistic(*dir_entry, NULL, s);
    /* Update header path */
    {
        char path[PATH_LENGTH];
        GetPath(*dir_entry, path);
        DisplayHeaderPath(path);
    }
}


static void Movenpage(int *disp_begin_pos, int *cursor_pos, DirEntry **dir_entry, Statistic *s)
{
   if( *disp_begin_pos + *cursor_pos >= CurrentVolume->total_dirs - 1 )
   {
      /* Last position */
      /*-----------------*/
   }
   else
   {
      if( *cursor_pos < layout.dir_win_height - 1 )
      {
          /* Cursor is not on last entry of the page */
           if( *disp_begin_pos + layout.dir_win_height > CurrentVolume->total_dirs  - 1 )
              *cursor_pos = CurrentVolume->total_dirs - *disp_begin_pos - 1;
           else
              *cursor_pos = layout.dir_win_height - 1;
      }
      else
      {
          /* Scrolling */
          /*----------*/
          if( *disp_begin_pos + *cursor_pos + layout.dir_win_height < CurrentVolume->total_dirs )
          {
              *disp_begin_pos += layout.dir_win_height;
              *cursor_pos = layout.dir_win_height - 1;
          }
          else
          {
              *disp_begin_pos = CurrentVolume->total_dirs - layout.dir_win_height;
              if( *disp_begin_pos < 0 ) *disp_begin_pos = 0;
              *cursor_pos = CurrentVolume->total_dirs - *disp_begin_pos - 1;
          }
      }
      *dir_entry = CurrentVolume->dir_entry_list[*disp_begin_pos + *cursor_pos].dir_entry;

      if (GlobalView->refresh_mode & REFRESH_ON_NAV) {
          *dir_entry = RefreshTreeSafe(*dir_entry);
          /* Re-sync *dir_entry to global stats which RefreshTreeSafe might have adjusted */
          *dir_entry = CurrentVolume->dir_entry_list[*disp_begin_pos + *cursor_pos].dir_entry;
      }

      (*dir_entry)->start_file = 0;
      (*dir_entry)->cursor_pos = -1;
      DisplayTree(CurrentVolume, dir_window,*disp_begin_pos,*disp_begin_pos+*cursor_pos);
      DisplayFileWindow( *dir_entry, file_window );
      RefreshWindow( file_window );
      DisplayDirStatistic(*dir_entry, NULL, s);
      /* Update header path */
      {
          char path[PATH_LENGTH];
          GetPath(*dir_entry, path);
          DisplayHeaderPath(path);
      }
   }
   return;
}

static void Moveppage(int *disp_begin_pos, int *cursor_pos, DirEntry **dir_entry, Statistic *s)
{
   if( *disp_begin_pos + *cursor_pos <= 0 )
   {
      /* First position */
      /*----------------*/
   }
   else
   {
      if( *cursor_pos > 0 )
      {
          /* Cursor is not on first entry of the page */
           *cursor_pos = 0;
      }
      else
      {
         /* Scrolling */
         /*----------*/
         if( (*disp_begin_pos -= layout.dir_win_height) < 0 )
         {
             *disp_begin_pos = 0;
         }
         *cursor_pos = 0;
      }
      *dir_entry = CurrentVolume->dir_entry_list[*disp_begin_pos + *cursor_pos].dir_entry;

      if (GlobalView->refresh_mode & REFRESH_ON_NAV) {
          *dir_entry = RefreshTreeSafe(*dir_entry);
          /* Re-sync *dir_entry to global stats which RefreshTreeSafe might have adjusted */
          *dir_entry = CurrentVolume->dir_entry_list[*disp_begin_pos + *cursor_pos].dir_entry;
      }

      (*dir_entry)->start_file = 0;
      (*dir_entry)->cursor_pos = -1;
      DisplayTree(CurrentVolume, dir_window,*disp_begin_pos,*disp_begin_pos+*cursor_pos);
      DisplayFileWindow( *dir_entry, file_window );
      RefreshWindow( file_window );
      DisplayDirStatistic(*dir_entry, NULL, s);
      /* Update header path */
      {
          char path[PATH_LENGTH];
          GetPath(*dir_entry, path);
          DisplayHeaderPath(path);
      }
   }
   return;
}

static void MoveEnd(DirEntry **dir_entry, Statistic *s)
{
    s->disp_begin_pos = MAXIMUM(0, CurrentVolume->total_dirs - layout.dir_win_height);
    s->cursor_pos     = CurrentVolume->total_dirs - s->disp_begin_pos - 1;
    *dir_entry = CurrentVolume->dir_entry_list[s->disp_begin_pos + s->cursor_pos].dir_entry;

    if (GlobalView->refresh_mode & REFRESH_ON_NAV) {
        *dir_entry = RefreshTreeSafe(*dir_entry);
        /* Re-sync *dir_entry to global stats which RefreshTreeSafe might have adjusted */
        *dir_entry = CurrentVolume->dir_entry_list[s->disp_begin_pos + s->cursor_pos].dir_entry;
    }

    (*dir_entry)->start_file = 0;
    (*dir_entry)->cursor_pos = -1;
    DisplayFileWindow( *dir_entry, file_window );
    RefreshWindow( file_window );
    DisplayTree( CurrentVolume, dir_window, s->disp_begin_pos,
		s->disp_begin_pos + s->cursor_pos);
    DisplayDirStatistic(*dir_entry, NULL, s);
    /* Update header path */
    {
        char path[PATH_LENGTH];
        GetPath(*dir_entry, path);
        DisplayHeaderPath(path);
    }
    return;
}

static void MoveHome(DirEntry **dir_entry, Statistic *s)
{
    if( s->disp_begin_pos == 0 && s->cursor_pos == 0 )
    {  /* Position 1 already reached */
       /*----------------------------*/
    }
    else
    {
       s->disp_begin_pos = 0;
       s->cursor_pos     = 0;
       *dir_entry = CurrentVolume->dir_entry_list[s->disp_begin_pos + s->cursor_pos].dir_entry;

       if (GlobalView->refresh_mode & REFRESH_ON_NAV) {
           *dir_entry = RefreshTreeSafe(*dir_entry);
           /* Re-sync *dir_entry to global stats which RefreshTreeSafe might have adjusted */
           *dir_entry = CurrentVolume->dir_entry_list[s->disp_begin_pos + s->cursor_pos].dir_entry;
       }

       (*dir_entry)->start_file = 0;
       (*dir_entry)->cursor_pos = -1;
       DisplayFileWindow( *dir_entry, file_window );
       RefreshWindow( file_window );
       DisplayTree( CurrentVolume, dir_window, s->disp_begin_pos,
		s->disp_begin_pos + s->cursor_pos);
       DisplayDirStatistic(*dir_entry, NULL, s);
       /* Update header path */
       {
           char path[PATH_LENGTH];
           GetPath(*dir_entry, path);
           DisplayHeaderPath(path);
       }
    }
    return;
}

static void HandlePlus(DirEntry *dir_entry, DirEntry *de_ptr, char *new_login_path, BOOL *need_dsp_help, Statistic *s)
{
    /* Renamed usage: s->mode -> s->login_mode */
    if (s->login_mode != DISK_MODE && s->login_mode != USER_MODE) {
        return;
    }
    if( !dir_entry->not_scanned ) {
    } else {
        SuspendClock(); /* Suspend clock before scanning */
	for( de_ptr=dir_entry->sub_tree; de_ptr; de_ptr=de_ptr->next) {
	    GetPath( de_ptr, new_login_path );
	    ReadTree( de_ptr, new_login_path, 0, s );
	    ApplyFilter( de_ptr, s );
	}
        InitClock();    /* Resume clock after scanning */
	dir_entry->not_scanned = FALSE;
	BuildDirEntryList( CurrentVolume );
	DisplayTree( CurrentVolume, dir_window, s->disp_begin_pos,
                 s->disp_begin_pos + s->cursor_pos );
	DisplayDiskStatistic(s);
    DisplayDirStatistic(dir_entry, NULL, s);
	DisplayAvailBytes(s);
	*need_dsp_help = TRUE;
    }
}

static void HandleReadSubTree(DirEntry *dir_entry, BOOL *need_dsp_help, Statistic *s)
{
    SuspendClock(); /* Suspend clock before scanning */
    if (ScanSubTree(dir_entry, s) == -1) {
        /* Aborted. Fall through to refresh what we have. */
    }
    InitClock();    /* Resume clock after scanning */
    BuildDirEntryList( CurrentVolume );
    DisplayTree( CurrentVolume, dir_window, s->disp_begin_pos,
		 s->disp_begin_pos + s->cursor_pos );
    RecalculateSysStats(s); /* Fix for Bug 10: Force full recalculation */
    DisplayDiskStatistic(s);
    DisplayDirStatistic(dir_entry, NULL, s);
    DisplayAvailBytes(s);
    *need_dsp_help = TRUE;
}

static void HandleUnreadSubTree(DirEntry *dir_entry, DirEntry *de_ptr, BOOL *need_dsp_help, Statistic *s)
{
    /* Renamed usage: s->mode -> s->login_mode */
    if (s->login_mode != DISK_MODE && s->login_mode != USER_MODE) {
        return;
    }
    if( dir_entry->not_scanned || (dir_entry->sub_tree == NULL) ) {
    } else {
	for( de_ptr=dir_entry->sub_tree; de_ptr; de_ptr=de_ptr->next) {
	    UnReadTree( de_ptr, s );
	}
	dir_entry->not_scanned = TRUE;
	BuildDirEntryList( CurrentVolume );
        DisplayTree( CurrentVolume, dir_window, s->disp_begin_pos,
		    s->disp_begin_pos + s->cursor_pos );
        DisplayAvailBytes(s);
        RecalculateSysStats(s); /* Fix for Bug 10: Force full recalculation */
        DisplayDiskStatistic(s);
        DisplayDirStatistic(dir_entry, NULL, s);
        *need_dsp_help = TRUE;
    }
    return;
}

static void HandleTagDir(DirEntry *dir_entry, BOOL value, Statistic *s)
{
    FileEntry *fe_ptr;
    for(fe_ptr=dir_entry->file; fe_ptr; fe_ptr=fe_ptr->next)
    {
        /* Skip hidden dotfiles if the option is enabled */
        if (hide_dot_files && fe_ptr->name[0] == '.') continue;

	if( (fe_ptr->matching) && (fe_ptr->tagged != value ))
	{
	    fe_ptr->tagged = value;
	    if (value)
	    {
		dir_entry->tagged_files++;
		dir_entry->tagged_bytes += fe_ptr->stat_struct.st_size;
	       	s->disk_tagged_files++;
		s->disk_tagged_bytes += fe_ptr->stat_struct.st_size;
	    }else{
		dir_entry->tagged_files--;
		dir_entry->tagged_bytes -= fe_ptr->stat_struct.st_size;
	       	s->disk_tagged_files--;
		s->disk_tagged_bytes -= fe_ptr->stat_struct.st_size;
	    }
	}
    }
    dir_entry->start_file = 0;
    dir_entry->cursor_pos = -1;
    DisplayFileWindow( dir_entry, file_window );
    RefreshWindow( file_window );
    DisplayDiskStatistic(s);
    DisplayDirStatistic(dir_entry, NULL, s);
    return;
}

static void HandleTagAllDirs(struct Volume *vol, DirEntry *dir_entry, BOOL value, Statistic *s)
{
    FileEntry *fe_ptr;
    long i;
    for(i=0; i < vol->total_dirs; i++)
    {
	for(fe_ptr=vol->dir_entry_list[i].dir_entry->file; fe_ptr; fe_ptr=fe_ptr->next)
	{
        /* Skip hidden dotfiles if the option is enabled */
        if (hide_dot_files && fe_ptr->name[0] == '.') continue;

	    if( (fe_ptr->matching) && (fe_ptr->tagged != value) )
	    {
	        if (value)
	        {
		    fe_ptr->tagged = value;
	       	    dir_entry->tagged_files++;
		    dir_entry->tagged_bytes += fe_ptr->stat_struct.st_size;
	       	    s->disk_tagged_files++;
		    s->disk_tagged_bytes += fe_ptr->stat_struct.st_size;
	        }else{
		    fe_ptr->tagged = value;
	       	    dir_entry->tagged_files--;
		    dir_entry->tagged_bytes -= fe_ptr->stat_struct.st_size;
	       	    s->disk_tagged_files--;
		    s->disk_tagged_bytes -= fe_ptr->stat_struct.st_size;
	        }
	    }
	}
    }
    dir_entry->start_file = 0;
    dir_entry->cursor_pos = -1;
    DisplayFileWindow( dir_entry, file_window );
    RefreshWindow( file_window );
    DisplayDiskStatistic(s);
    DisplayDirStatistic(dir_entry, NULL, s);
    return;
}



static void HandleShowAll(BOOL tagged_only, DirEntry *dir_entry, BOOL *need_dsp_help, int *ch, Statistic *s)
{
    if( (tagged_only) ? s->disk_tagged_files : s->disk_matching_files )
    {
	if(dir_entry->login_flag)
	{
	    dir_entry->login_flag = FALSE;
	} else {
	    dir_entry->big_window  = TRUE;
	    dir_entry->global_flag = TRUE;
	    dir_entry->tagged_flag = tagged_only;
	    dir_entry->start_file  = 0;
	    dir_entry->cursor_pos  = 0;
	}
	if( HandleFileWindow(dir_entry) != LOGIN_ESC )
	{
	    DisplayDiskStatistic(s);
	    DisplayDirStatistic(dir_entry, NULL, s);
	    dir_entry->start_file = 0;
	    dir_entry->cursor_pos = -1;
            DisplayFileWindow( dir_entry, file_window );
            RefreshWindow( small_file_window );
            RefreshWindow( big_file_window );
	    BuildDirEntryList( CurrentVolume );
            DisplayTree( CurrentVolume, dir_window, s->disp_begin_pos,
			 s->disp_begin_pos + s->cursor_pos);
	} else {
	    BuildDirEntryList( CurrentVolume );
            DisplayTree( CurrentVolume, dir_window, s->disp_begin_pos,
			s->disp_begin_pos + s->cursor_pos);
	    *ch = 'L';
	}
    } else {
	dir_entry->login_flag = FALSE;
    }
    *need_dsp_help = TRUE;
    return;
}

static void HandleSwitchWindow(DirEntry *dir_entry, BOOL *need_dsp_help, int *ch, Statistic *s)
{
    /* Critical Safety: Check for volume changes upon return from File Window */
    struct Volume *start_vol = CurrentVolume;

    if( dir_entry->matching_files )
    {
	if(dir_entry->login_flag)
	{
	    dir_entry->login_flag = FALSE;
	} else {
	    dir_entry->global_flag = FALSE;
            dir_entry->tagged_flag = FALSE;
	    dir_entry->big_window  = bypass_small_window;
	    dir_entry->start_file  = 0;
	    dir_entry->cursor_pos  = 0;
	}
	if( HandleFileWindow( dir_entry ) != LOGIN_ESC )
    {
        /* Safety Check: If volume was deleted in File Window (via SelectLoadedVolume), abort */
        if (CurrentVolume != start_vol) return;

	    dir_entry->start_file = 0;
	    dir_entry->cursor_pos = -1;
	    DisplayFileWindow( dir_entry, file_window );
            RefreshWindow( small_file_window );
            RefreshWindow( big_file_window );
	    BuildDirEntryList( CurrentVolume );

            touchwin(dir_window);
            DisplayTree( CurrentVolume, dir_window, s->disp_begin_pos,
			 s->disp_begin_pos + s->cursor_pos);
            RefreshWindow(dir_window);

	    DisplayDiskStatistic(s);
        DisplayDirStatistic(dir_entry, NULL, s);
	} else {
	    BuildDirEntryList( CurrentVolume );
            DisplayTree( CurrentVolume, dir_window, s->disp_begin_pos,
			s->disp_begin_pos + s->cursor_pos);
	    *ch = 'L';
	}
	DisplayAvailBytes(s);
	*need_dsp_help = TRUE;
    } else {
	dir_entry->login_flag = FALSE;
    }
    return;
}

void ToggleDotFiles(void)
{
    DirEntry *target;
    int i, found_idx = -1;
    int win_height, win_width;
    Statistic *s = &CurrentVolume->vol_stats;

    /* Suspend clock to prevent signal handler interrupt corrupting UI during rebuild */
    SuspendClock();

    /* 1. Identify the directory currently under the cursor */
    if (CurrentVolume->total_dirs > 0 && (s->disp_begin_pos + s->cursor_pos < CurrentVolume->total_dirs)) {
        target = CurrentVolume->dir_entry_list[s->disp_begin_pos + s->cursor_pos].dir_entry;
    } else {
        target = s->tree;
    }

    /* 2. Toggle State and Recalculate Stats */
    hide_dot_files = !hide_dot_files;
    RecalculateSysStats(s);

    /* 3. Rebuild the linear list of visible directories */
    BuildDirEntryList(CurrentVolume);

    /* 4. Search for the 'target' directory in the new list */
    DirEntry *search = target;
    while (search != NULL && found_idx == -1) {
        for (i = 0; i < CurrentVolume->total_dirs; i++) {
            if (CurrentVolume->dir_entry_list[i].dir_entry == search) {
                found_idx = i;
                break;
            }
        }
        /* If the target directory is now hidden, walk up to its parent */
        if (found_idx == -1) search = search->up_tree;
    }

    /* 5. Smart Restore of Cursor Position */
    getmaxyx(dir_window, win_height, win_width);

    if (found_idx != -1) {
        /* Check if the found directory is within the current visible page */
        if (found_idx >= s->disp_begin_pos &&
            found_idx < s->disp_begin_pos + layout.dir_win_height) {
            /* It's still on screen. Just update the cursor, don't scroll/jump. */
            s->cursor_pos = found_idx - s->disp_begin_pos;
        } else {
            /* It moved off page. Re-center or adjust slightly. */
            if (found_idx < layout.dir_win_height) {
                s->disp_begin_pos = 0;
                s->cursor_pos = found_idx;
            } else {
                /* Center the item */
                s->disp_begin_pos = found_idx - (layout.dir_win_height / 2);

                /* Bounds check for display position */
                if (s->disp_begin_pos > CurrentVolume->total_dirs - layout.dir_win_height) {
                    s->disp_begin_pos = CurrentVolume->total_dirs - layout.dir_win_height;
                }
                if (s->disp_begin_pos < 0) s->disp_begin_pos = 0;

                s->cursor_pos = found_idx - s->disp_begin_pos;
            }
        }
    } else {
        /* Fallback to root if everything went wrong */
        s->disp_begin_pos = 0;
        s->cursor_pos = 0;
    }

    /* Sanity check cursor limits */
    if (s->cursor_pos >= layout.dir_win_height) s->cursor_pos = layout.dir_win_height - 1;
    if (s->disp_begin_pos + s->cursor_pos >= CurrentVolume->total_dirs) {
        /* Move cursor to last valid item */
        s->cursor_pos = (CurrentVolume->total_dirs > 0) ? (CurrentVolume->total_dirs - 1 - s->disp_begin_pos) : 0;
    }

    /* Refresh Directory Tree */
    DisplayTree(CurrentVolume, dir_window, s->disp_begin_pos, s->disp_begin_pos + s->cursor_pos);
    DisplayDiskStatistic(s);

    /* Update current dir pointer using the new accessor function
       because ToggleDotFiles might have changed the list layout */
    if (CurrentVolume->total_dirs > 0) {
        target = CurrentVolume->dir_entry_list[s->disp_begin_pos + s->cursor_pos].dir_entry;
    } else {
        target = s->tree;
    }

    /* Explicitly update the file window (preview) to match new visibility */
    DisplayFileWindow(target, file_window);
    RefreshWindow(file_window);
    DisplayDirStatistic(target, NULL, s);
    /* Update header path */
    {
        char path[PATH_LENGTH];
        GetPath(target, path);
        DisplayHeaderPath(path);
    }

    InitClock(); /* Resume clock and restore signal handling */
}

/*
 * RefreshTreeSafe
 * Performs a non-destructive refresh of the directory tree.
 * Saves expansion state and tags, rescans from disk, restores state, and refreshes the UI.
 * Can be called from both Directory Window and File Window.
 */
DirEntry *RefreshTreeSafe(DirEntry *entry)
{
    Statistic *s = &CurrentVolume->vol_stats;
    /* Determine root for refresh logic: if user is in a sub-dir, refresh context matters */
    /* However, RestoreTreeState and RescanDir logic works best when anchored. */
    /* The legacy ACTION_REFRESH logic worked on 'dir_entry' which is the current cursor position. */
    /* RescanDir takes 'entry'. */

    werase(dir_window);
    werase(file_window);
    refresh(); /* Force clear */

    /* Capture flags here to preserve state across destructive rescan */
    BOOL saved_big_window = entry->big_window;
    BOOL saved_login_flag = entry->login_flag;
    BOOL saved_global_flag = entry->global_flag;
    BOOL saved_tagged_flag = entry->tagged_flag;

    if (mode != ARCHIVE_MODE) {
        PathList *expanded = NULL;
        PathList *tagged = NULL;
        char saved_path[PATH_LENGTH + 1];
        int win_height;
        int dummy_width;

        GetMaxYX(dir_window, &win_height, &dummy_width);

        /* 1. Save State */
        /* Save path of the *currently selected* directory to restore cursor later */
        /* 'entry' passed in is usually the currently selected directory */
        GetPath(entry, saved_path);
        /* Save state relative to the current cursor position or root? */
        /* Ideally save state from root to preserve everything. */
        SaveTreeState(s->tree, &expanded, &tagged);

        /* 2. Destructive Rescan */
        /* Scan from the current entry downwards? Or Root? */
        /* If we only rescan 'entry', we might miss changes above if we moved? */
        /* Legacy logic rescanned 'dir_entry' (current cursor). */
        /* Let's stick to rescanning 'entry' to update local view, or root if needed? */
        /* Actually, if we rescan 'entry', we only risk collapsing *its* children. */
        /* If we want a full refresh, we should perhaps rescan root? */
        /* But 'entry' allows targeted refresh. Let's keep 'entry'. */
        /* BUT, SaveTreeState saved paths relative to root. RestoreTreeState starts at 'root'. */
        /* This suggests we should use 'entry' as root for Restore if we rescan 'entry'. */
        /* HOWEVER, `SaveTreeState(s->tree)` saves GLOBAL state. */
        /* If we only rescan `entry`, we preserve other branches automatically (not touched). */
        /* So `RestoreTreeState(s->tree)` is safe because it will just re-traverse unchanged nodes. */
        RescanDir(entry, strtol(TREEDEPTH, NULL, 0), s);

        /* 2a. Restore critical flags destroyed by ReadTree */
        entry->big_window = saved_big_window;
        entry->login_flag = saved_login_flag;
        entry->global_flag = saved_global_flag;
        entry->tagged_flag = saved_tagged_flag;

        /* 3. Restore State */
        RestoreTreeState(s->tree, expanded, tagged, s);
        FreePathList(expanded);
        FreePathList(tagged);

        /* 4. Restore Selection */
        BuildDirEntryList(CurrentVolume);

        /* Try to find the directory we were on */
        int found_idx = -1;
        int i;
        char temp_path[PATH_LENGTH + 1];
        for (i = 0; i < CurrentVolume->total_dirs; i++) {
            GetPath(CurrentVolume->dir_entry_list[i].dir_entry, temp_path);
            if (strcmp(temp_path, saved_path) == 0) {
                found_idx = i;
                break;
            }
        }

        if (found_idx != -1) {
            /* Restore cursor */
            if (found_idx >= s->disp_begin_pos &&
                found_idx < s->disp_begin_pos + win_height) {
                s->cursor_pos = found_idx - s->disp_begin_pos;
            } else {
                /* Move to ensure visibility */
                s->disp_begin_pos = found_idx;
                s->cursor_pos = 0;
                if (s->disp_begin_pos + win_height > CurrentVolume->total_dirs) {
                     s->disp_begin_pos = MAXIMUM(0, CurrentVolume->total_dirs - win_height);
                     s->cursor_pos = found_idx - s->disp_begin_pos;
                }
            }
            /* Update entry pointer if it changed address (it shouldn't if found, but good practice) */
            entry = CurrentVolume->dir_entry_list[s->disp_begin_pos + s->cursor_pos].dir_entry;
        } else {
            /* Fallback to start if dir moved/deleted */
            if (CurrentVolume->total_dirs > 0 && (s->disp_begin_pos + s->cursor_pos >= CurrentVolume->total_dirs)) {
                s->disp_begin_pos = 0;
                s->cursor_pos = 0;
                entry = CurrentVolume->dir_entry_list[0].dir_entry;
            }
        }
    } else {
        /* Archive Mode - Standard Rescan */
        RescanDir(entry, strtol(TREEDEPTH, NULL, 0), s);
        /* Restore flags for Archive mode too, as RescanDir/ReadTree clears them */
        entry->big_window = saved_big_window;
        entry->login_flag = saved_login_flag;
        entry->global_flag = saved_global_flag;
        entry->tagged_flag = saved_tagged_flag;

        BuildDirEntryList(CurrentVolume);
        /* Basic bounds check */
        if (CurrentVolume->total_dirs > 0 && (s->disp_begin_pos + s->cursor_pos >= CurrentVolume->total_dirs)) {
            s->disp_begin_pos = 0;
            s->cursor_pos = 0;
            entry = CurrentVolume->dir_entry_list[0].dir_entry;
        }
    }

    /* Force update of free bytes info during refresh */
    (void) GetAvailBytes( &s->disk_space, s );

    DisplayTree(CurrentVolume, dir_window, s->disp_begin_pos, s->disp_begin_pos + s->cursor_pos);
    DisplayFileWindow(entry, file_window);
    DisplayDiskStatistic(s);
    DisplayDirStatistic(entry, NULL, s);

    return entry;
}

int HandleDirWindow(DirEntry *start_dir_entry)
{
  DirEntry  *dir_entry, *de_ptr;
  int i, ch, unput_char;
  BOOL need_dsp_help;
  char new_name[PATH_LENGTH + 1];
  char new_login_path[PATH_LENGTH + 1];
  char *home;
  YtreeAction action; /* Declare YtreeAction variable */
  struct Volume *start_vol = CurrentVolume; /* Track global volume changes */
  Statistic *s = &CurrentVolume->vol_stats;
  int height, width;
  char watcher_path[PATH_LENGTH + 1];

  unput_char = 0;
  de_ptr = NULL;

  getmaxyx(dir_window, height, width);

  /* Clear flags */
  /*-----------------*/

  dir_mode = MODE_3;

  need_dsp_help = TRUE;

  BuildDirEntryList( CurrentVolume );
  if ( initial_directory != NULL )
  {
    if ( !strcmp( initial_directory, "." ) )   /* Entry just a single "." */
    {
      s->disp_begin_pos = 0;
      s->cursor_pos = 0;
      unput_char = CR;
    }
    else
    {
      if ( *initial_directory == '.' ) {   /* Entry of form "./alpha/beta" */
        strcpy( new_login_path, start_dir_entry->name );
        strcat( new_login_path, initial_directory+1 );
      }
      else if ( *initial_directory == '~' && ( home = getenv("HOME") ) ) {
                                           /* Entry of form "~/alpha/beta" */
        strcpy( new_login_path, home );
        strcat( new_login_path, initial_directory+1 );
      }
      else {            /* Entry of form "beta" or "/full/path/alpha/beta" */
        strcpy(new_login_path, initial_directory);
      }
      for ( i = 0; i < CurrentVolume->total_dirs; i++ )
      {
        if ( *new_login_path == FILE_SEPARATOR_CHAR )
          GetPath( CurrentVolume->dir_entry_list[i].dir_entry, new_name );
        else
          strcpy( new_name, CurrentVolume->dir_entry_list[i].dir_entry->name );
        if ( !strcmp( new_login_path, new_name ) )
        {
          s->disp_begin_pos = i;
          s->cursor_pos = 0;
          unput_char = CR;
          break;
        }
      }
    }
    initial_directory = NULL;
  }
  dir_entry = CurrentVolume->dir_entry_list[s->disp_begin_pos + s->cursor_pos].dir_entry;

  DisplayDiskStatistic(s);
  DisplayDirStatistic(dir_entry, NULL, s);
  /* Update header path on initial load */
  {
      char path[PATH_LENGTH];
      GetPath(dir_entry, path);
      DisplayHeaderPath(path);
  }

  if(!dir_entry->login_flag) {
    dir_entry->start_file = 0;
    dir_entry->cursor_pos = -1;
  }
  DisplayFileWindow( dir_entry, file_window );
  RefreshWindow( file_window );
  DisplayTree( CurrentVolume, dir_window, s->disp_begin_pos, s->disp_begin_pos + s->cursor_pos );
  touchwin(dir_window);

  if ( dir_entry->login_flag )
  {
    if( (dir_entry->global_flag ) || (dir_entry->tagged_flag) )
    {
      unput_char = 'S';
    }
    else
    {
      unput_char = CR;
    }
  }
  do
  {
    /* Detect Global Volume Change (Split Brain Fix) */
    if (CurrentVolume != start_vol) return ESC;

    if ( need_dsp_help )
    {
      need_dsp_help = FALSE;
      DisplayDirHelp();
    }
    DisplayDirParameter( dir_entry );
    RefreshWindow( dir_window );

    if (IsSplitScreen) {
        YtreePanel *inactive = (ActivePanel == LeftPanel) ? RightPanel : LeftPanel;
        RenderInactivePanel(inactive);
    }

    if (s->login_mode == DISK_MODE || s->login_mode == USER_MODE) {
        if (GlobalView->refresh_mode & REFRESH_WATCHER) {
            GetPath(dir_entry, watcher_path);
            Watcher_SetDir(watcher_path);
        }
    }

    if( unput_char )
    {
      ch = unput_char;
      unput_char = '\0';
    }
    else
    {
      doupdate();
      ch = (resize_request) ? -1 : GetEventOrKey();
      /* LF to CR normalization is now handled by GetKeyAction */
    }

    if (IsUserActionDefined()) { /* User commands take precedence */
        ch = DirUserMode(dir_entry, ch);
    }

    /* ViKey processing is now handled inside GetKeyAction */

    if(resize_request) {
       ReCreateWindows();
       DisplayMenu();
       getmaxyx(dir_window, height, width);
       while(s->cursor_pos >= height) {
         s->cursor_pos--;
	 s->disp_begin_pos++;
       }
       DisplayTree( CurrentVolume, dir_window, s->disp_begin_pos,
		    s->disp_begin_pos + s->cursor_pos
		  );
       DisplayFileWindow(dir_entry, file_window);
       DisplayDiskStatistic(s);
       DisplayDirStatistic(dir_entry, NULL, s);
       DisplayDirParameter( dir_entry );
       need_dsp_help = TRUE;
       /* Removed redundant calls to fix missing T-junctions */
       /* Update header path after resize */
       {
           char path[PATH_LENGTH];
           GetPath(dir_entry, path);
           DisplayHeaderPath(path);
       }
       doupdate(); /* FORCE UPDATE */
       resize_request = FALSE;
    }

    action = GetKeyAction(ch); /* Translate raw input to YtreeAction */

    switch( action )
    {
      case ACTION_RESIZE: resize_request = TRUE;
      		       break;

      case ACTION_TOGGLE_STATS:
               GlobalView->show_stats = !GlobalView->show_stats;
               resize_request = TRUE;
               break;

      case ACTION_SPLIT_SCREEN:
             IsSplitScreen = !IsSplitScreen;
             if( IsSplitScreen ) {
                 if( !LeftPanel ) {
                     if( ( LeftPanel = (YtreePanel *)calloc( 1, sizeof(YtreePanel) ) ) == NULL ) {
                         ERROR_MSG( "Calloc Failed*ABORT" );
                         exit( 1 );
                     }
                 }
                 if( !RightPanel ) {
                     if( ( RightPanel = (YtreePanel *)calloc( 1, sizeof(YtreePanel) ) ) == NULL ) {
                         ERROR_MSG( "Calloc Failed*ABORT" );
                         exit( 1 );
                     }
                 }
                 RightPanel->vol = CurrentVolume;
                 LeftPanel->vol  = CurrentVolume;
                 if( !ActivePanel ) ActivePanel = LeftPanel;
             }
             resize_request = TRUE;
             break;

      case ACTION_NONE:  /* -1 or unhandled keys */
                         if (ch == -1) break; /* Ignore -1 (resize_request handled above) */
                         /* Fall through for other unhandled keys to beep */
                         beep();
                         break;

      case ACTION_MOVE_DOWN: Movedown(&s->disp_begin_pos, &s->cursor_pos, &dir_entry, s);
                     break;
      case ACTION_MOVE_UP: Moveup(&s->disp_begin_pos, &s->cursor_pos, &dir_entry, s);
                     break;
      case ACTION_MOVE_SIBLING_NEXT:
            {
                DirEntry *target = dir_entry->next;
                if (target == NULL) {
                     /* Wrap to first sibling */
                     if (dir_entry->up_tree != NULL) {
                         target = dir_entry->up_tree->sub_tree;
                     }
                }

                if (target != NULL && target != dir_entry) {
                    /* Find the sibling in the linear list to update cursor index */
                    int k;
                    int found_idx = -1;

                    for (k = 0; k < CurrentVolume->total_dirs; k++) {
                        if (CurrentVolume->dir_entry_list[k].dir_entry == target) {
                            found_idx = k;
                            break;
                        }
                    }

                    if (found_idx != -1) {
                        /* Move cursor to sibling */
                        if (found_idx >= s->disp_begin_pos &&
                            found_idx < s->disp_begin_pos + height) {
                            s->cursor_pos = found_idx - s->disp_begin_pos;
                        } else {
                            /* Off screen, center it or move to top */
                            s->disp_begin_pos = found_idx;
                            s->cursor_pos = 0;
                            /* Bounds check */
                            if (s->disp_begin_pos + height > CurrentVolume->total_dirs) {
                                 s->disp_begin_pos = MAXIMUM(0, CurrentVolume->total_dirs - height);
                                 s->cursor_pos = found_idx - s->disp_begin_pos;
                            }
                        }
                        /* Sync */
                        dir_entry = CurrentVolume->dir_entry_list[s->disp_begin_pos + s->cursor_pos].dir_entry;

                        if (GlobalView->refresh_mode & REFRESH_ON_NAV) {
                            dir_entry = RefreshTreeSafe(dir_entry);
                            break; /* Skip manual refresh logic below */
                        }

                        /* Refresh */
                        DisplayTree(CurrentVolume, dir_window, s->disp_begin_pos, s->disp_begin_pos + s->cursor_pos);
                        DisplayFileWindow(dir_entry, file_window);
                        DisplayDiskStatistic(s);
                        DisplayDirStatistic(dir_entry, NULL, s);
                        DisplayAvailBytes(s);

                        char path[PATH_LENGTH];
                        GetPath(dir_entry, path);
                        DisplayHeaderPath(path);
                    }
                }
            }
            need_dsp_help = TRUE;
            break;
      case ACTION_MOVE_SIBLING_PREV:
            {
                DirEntry *target = dir_entry->prev;
                if (target == NULL) {
                     /* Wrap to last sibling */
                     if (dir_entry->up_tree != NULL) {
                         target = dir_entry->up_tree->sub_tree;
                         while (target && target->next != NULL) {
                             target = target->next;
                         }
                     }
                }

                if (target != NULL && target != dir_entry) {
                    /* Find the sibling in the linear list to update cursor index */
                    int k;
                    int found_idx = -1;

                    for (k = 0; k < CurrentVolume->total_dirs; k++) {
                        if (CurrentVolume->dir_entry_list[k].dir_entry == target) {
                            found_idx = k;
                            break;
                        }
                    }

                    if (found_idx != -1) {
                        /* Move cursor to sibling */
                        if (found_idx >= s->disp_begin_pos &&
                            found_idx < s->disp_begin_pos + height) {
                            s->cursor_pos = found_idx - s->disp_begin_pos;
                        } else {
                            /* Off screen, center it or move to top */
                            s->disp_begin_pos = found_idx;
                            s->cursor_pos = 0;
                            /* Bounds check */
                            if (s->disp_begin_pos + height > CurrentVolume->total_dirs) {
                                 s->disp_begin_pos = MAXIMUM(0, CurrentVolume->total_dirs - height);
                                 s->cursor_pos = found_idx - s->disp_begin_pos;
                            }
                        }
                        /* Sync */
                        dir_entry = CurrentVolume->dir_entry_list[s->disp_begin_pos + s->cursor_pos].dir_entry;

                        if (GlobalView->refresh_mode & REFRESH_ON_NAV) {
                            dir_entry = RefreshTreeSafe(dir_entry);
                            break; /* Skip manual refresh logic below */
                        }

                        /* Refresh */
                        DisplayTree(CurrentVolume, dir_window, s->disp_begin_pos, s->disp_begin_pos + s->cursor_pos);
                        DisplayFileWindow(dir_entry, file_window);
                        DisplayDiskStatistic(s);
                        DisplayDirStatistic(dir_entry, NULL, s);
                        DisplayAvailBytes(s);

                        char path[PATH_LENGTH];
                        GetPath(dir_entry, path);
                        DisplayHeaderPath(path);
                    }
                }
            }
            need_dsp_help = TRUE;
            break;
      case ACTION_PAGE_DOWN: Movenpage(&s->disp_begin_pos, &s->cursor_pos, &dir_entry, s);
                     break;
      case ACTION_PAGE_UP: Moveppage(&s->disp_begin_pos, &s->cursor_pos, &dir_entry, s);
                     break;
      case ACTION_HOME: MoveHome(&dir_entry, s);
                     break;
      case ACTION_END: MoveEnd(&dir_entry, s);
    		     break;
      case ACTION_MOVE_RIGHT:
      case ACTION_TREE_EXPAND_ALL: HandlePlus(dir_entry, de_ptr, new_login_path,
    				&need_dsp_help, s);
	             break;
      case ACTION_ASTERISK:
      	             HandleReadSubTree(dir_entry, &need_dsp_help, s);
                     break;
      case ACTION_TREE_EXPAND: HandleReadSubTree(dir_entry,
    					&need_dsp_help, s);
    		     break;
      case ACTION_MOVE_LEFT:
          /* 1. Transparent Archive Exit Logic (At Root) */
          if (dir_entry->up_tree == NULL && mode == ARCHIVE_MODE) {
              struct Volume *old_vol = CurrentVolume;
              char archive_path[PATH_LENGTH + 1];
              char parent_dir[PATH_LENGTH + 1];
              char dummy_name[PATH_LENGTH + 1];

              /* Calculate Parent Directory of the Archive File */
              strcpy(archive_path, s->login_path);
              Fnsplit(archive_path, parent_dir, dummy_name);

              /* Force Login/Switch to the Parent Directory */
              /* This handles both "New Volume" and "Switch to Existing" logic automatically */
              if (LoginDisk(parent_dir) == 0) {
                  /* Successfully switched context. */

                  /* Delete the archive wrapper we just left to clean up memory/list */
                  Volume_Delete(old_vol);

                  /* Update pointers for the new context */
                  s = &CurrentVolume->vol_stats;
                  start_vol = CurrentVolume; /* Update loop safety variable */

                  /* Attempt to highlight the archive file we just left */
                  if (CurrentVolume->total_dirs > 0) {
                       /* Usually root, or restored position */
                       dir_entry = CurrentVolume->dir_entry_list[s->disp_begin_pos + s->cursor_pos].dir_entry;

                       /* Find the archive file in the file list */
                       FileEntry *fe;
                       int f_idx = 0;
                       for (fe = dir_entry->file; fe; fe = fe->next) {
                           if (strcmp(fe->name, dummy_name) == 0) {
                               dir_entry->start_file = f_idx;
                               dir_entry->cursor_pos = 0;
                               break;
                           }
                           f_idx++;
                       }
                  } else {
                       dir_entry = s->tree;
                  }

                  /* Refresh Full UI */
                  DisplayMenu();
                  DisplayTree(CurrentVolume, dir_window, s->disp_begin_pos, s->disp_begin_pos + s->cursor_pos);
                  DisplayFileWindow(dir_entry, file_window);
                  DisplayDiskStatistic(s);
                  DisplayDirStatistic(dir_entry, NULL, s);
                  DisplayAvailBytes(s);

                  {
                      char path[PATH_LENGTH];
                      GetPath(dir_entry, path);
                      DisplayHeaderPath(path);
                  }
                  need_dsp_help = TRUE;
                  break; /* Exit case done */
              }
          }

          /* 2. Standard Tree Navigation Logic */
          if (!dir_entry->not_scanned && dir_entry->sub_tree != NULL) {
              /* It is expanded */
              if (mode == ARCHIVE_MODE) {
                   /* In Archive Mode, we cannot collapse (UnRead) without data loss.
                      So we skip collapse and fall through to Jump to Parent. */
                   goto JUMP_TO_PARENT;
              } else {
                   /* In FS Mode, collapse it. */
                   HandleUnreadSubTree(dir_entry, de_ptr, &need_dsp_help, s);
              }
          } else {
              /* It is collapsed (or leaf) -> Jump to Parent */
              JUMP_TO_PARENT:
              if (dir_entry->up_tree != NULL) {
                   /* Find parent in the list */
                   int p_idx = -1;
                   int k;
                   for(k=0; k < CurrentVolume->total_dirs; k++) {
                       if(CurrentVolume->dir_entry_list[k].dir_entry == dir_entry->up_tree) {
                           p_idx = k;
                           break;
                       }
                   }
                   if (p_idx != -1) {
                       /* Move cursor to parent */
                       if (p_idx >= s->disp_begin_pos &&
                           p_idx < s->disp_begin_pos + height) {
                           /* Parent is on screen */
                           s->cursor_pos = p_idx - s->disp_begin_pos;
                       } else {
                           /* Parent is off screen - center it or put at top */
                           s->disp_begin_pos = p_idx;
                           s->cursor_pos = 0;
                           /* Adjust if near end */
                           if (s->disp_begin_pos + height > CurrentVolume->total_dirs) {
                               s->disp_begin_pos = MAXIMUM(0, CurrentVolume->total_dirs - height);
                               s->cursor_pos = p_idx - s->disp_begin_pos;
                           }
                       }
                       /* Sync pointers */
                       dir_entry = CurrentVolume->dir_entry_list[s->disp_begin_pos + s->cursor_pos].dir_entry;

                       /* Refresh */
                       DisplayTree(CurrentVolume, dir_window, s->disp_begin_pos, s->disp_begin_pos + s->cursor_pos);
                       DisplayFileWindow(dir_entry, file_window);
                       DisplayDiskStatistic(s);
                       DisplayDirStatistic(dir_entry, NULL, s);
                       DisplayAvailBytes(s);
                       /* Update Header Path */
                       char path[PATH_LENGTH];
                       GetPath(dir_entry, path);
                       DisplayHeaderPath(path);
                   }
              }
          }
          break;
      case ACTION_TREE_COLLAPSE: HandleUnreadSubTree(dir_entry, de_ptr,
    					 &need_dsp_help, s);
	             break;
      case ACTION_TOGGLE_HIDDEN: {
                        ToggleDotFiles();

                        /* Update current dir pointer using the new accessor function
                           because ToggleDotFiles might have changed the list layout */
                        if (CurrentVolume->total_dirs > 0) {
                            dir_entry = CurrentVolume->dir_entry_list[s->disp_begin_pos + s->cursor_pos].dir_entry;
                        } else {
                            dir_entry = s->tree;
                        }

                        need_dsp_help = TRUE;
                     }
		     break;
      case ACTION_FILTER: if(ReadFilter() == 0) {
		       dir_entry->start_file = 0;
		       dir_entry->cursor_pos = -1;
                       DisplayFileWindow( dir_entry, file_window );
                       RefreshWindow( file_window );
		       DisplayDiskStatistic(s);
		       DisplayDirStatistic(dir_entry, NULL, s);
		     }
		     need_dsp_help = TRUE;
                     break;
      case ACTION_TAG: HandleTagDir(dir_entry, TRUE, s);
             Movedown(&s->disp_begin_pos, &s->cursor_pos, &dir_entry, s); /* ADDED */
    		     break;

      case ACTION_UNTAG: HandleTagDir(dir_entry, FALSE, s);
             Movedown(&s->disp_begin_pos, &s->cursor_pos, &dir_entry, s); /* ADDED */
    		     break;
      case ACTION_TAG_ALL:
                     HandleTagAllDirs(CurrentVolume, dir_entry, TRUE, s);
		     break;
      case ACTION_UNTAG_ALL:
    		     HandleTagAllDirs(CurrentVolume, dir_entry, FALSE, s);
		     break;
      case ACTION_TOGGLE_MODE:
		     RotateDirMode();
                     /*DisplayFileWindow( dir_entry, 0, -1 );*/
                     DisplayTree( CurrentVolume, dir_window, s->disp_begin_pos,
                                  s->disp_begin_pos + s->cursor_pos
                                  );
                     /*RefreshWindow( file_window );*/
		     DisplayDiskStatistic(s);
		     DisplayDirStatistic(dir_entry, NULL, s);
		     need_dsp_help = TRUE;
		     break;
      case ACTION_CMD_TAGGED_S:
		     HandleShowAll(TRUE, dir_entry, &need_dsp_help, &ch, s);
		     break;

      case ACTION_CMD_S:
		     HandleShowAll(FALSE, dir_entry, &need_dsp_help, &ch, s);
		     break;
      case ACTION_ENTER:
             if (GlobalView->refresh_mode & REFRESH_ON_ENTER) {
                 dir_entry = RefreshTreeSafe(dir_entry);
                 /* Sync pointer from list in case address changed */
                 dir_entry = CurrentVolume->dir_entry_list[s->disp_begin_pos + s->cursor_pos].dir_entry;
             }
             HandleSwitchWindow(dir_entry, &need_dsp_help, &ch, s);
		     break;
      case ACTION_CMD_X: if (mode != DISK_MODE && mode != USER_MODE) {
		     } else {
			 (void) Execute( dir_entry, NULL );
             dir_entry = RefreshTreeSafe(dir_entry); /* Auto-Refresh after command */
		     }
		     need_dsp_help = TRUE;
		     DisplayAvailBytes(s);
                     DisplayDiskStatistic(s);
                     DisplayDirStatistic(dir_entry, NULL, s);
		     break;
      case ACTION_CMD_M: if( !MakeDirectory( dir_entry ) )
		     {
		       BuildDirEntryList( CurrentVolume );
                       DisplayTree( CurrentVolume, dir_window, s->disp_begin_pos,
				    s->disp_begin_pos + s->cursor_pos
				  );
		       DisplayAvailBytes(s);
                       DisplayDiskStatistic(s);
                       DisplayDirStatistic(dir_entry, NULL, s);
		     }
		     need_dsp_help = TRUE;
		     break;
      case ACTION_CMD_D: if( !DeleteDirectory( dir_entry ) ) {
		       if( s->disp_begin_pos + s->cursor_pos > 0 )
		       {
		         if( s->cursor_pos > 0 ) s->cursor_pos--;
		         else s->disp_begin_pos--;
		       }
      		     }
		     /* Update regardless of success */
		     BuildDirEntryList( CurrentVolume );
		     dir_entry = CurrentVolume->dir_entry_list[s->disp_begin_pos + s->cursor_pos].dir_entry;
		     dir_entry->start_file = 0;
		     dir_entry->cursor_pos = -1;
                     DisplayFileWindow( dir_entry, file_window );
                     RefreshWindow( file_window );
		     DisplayTree( CurrentVolume, dir_window, s->disp_begin_pos,
				  s->disp_begin_pos + s->cursor_pos
				);
		     DisplayAvailBytes(s);
                     DisplayDiskStatistic(s);
                     DisplayDirStatistic(dir_entry, NULL, s);
		     need_dsp_help = TRUE;
		     break;
      case ACTION_CMD_R: if( !GetRenameParameter( dir_entry->name, new_name ) )
                     {
		       if( !RenameDirectory( dir_entry, new_name ) )
		       {
		         /* Rename OK */
		         /*-----------*/
		         BuildDirEntryList( CurrentVolume );
                         DisplayTree( CurrentVolume, dir_window, s->disp_begin_pos,
		                      s->disp_begin_pos + s->cursor_pos
			            );
		         DisplayAvailBytes(s);
		         dir_entry = CurrentVolume->dir_entry_list[s->disp_begin_pos + s->cursor_pos].dir_entry;
                         DisplayDiskStatistic(s);
                         DisplayDirStatistic(dir_entry, NULL, s);
		       }
		     }
		     need_dsp_help = TRUE;
		     break;
      case ACTION_REFRESH: /* Rescan */
             dir_entry = RefreshTreeSafe(dir_entry);
             need_dsp_help = TRUE;
             break;
      case ACTION_CMD_G: (void) ChangeDirGroup( dir_entry );
                     DisplayTree( CurrentVolume, dir_window, s->disp_begin_pos, s->disp_begin_pos + s->cursor_pos );
                     DisplayDiskStatistic(s);
                     DisplayDirStatistic(dir_entry, NULL, s);
		     need_dsp_help = TRUE;
		     break;
      case ACTION_CMD_O: (void) ChangeDirOwner( dir_entry );
                     DisplayTree( CurrentVolume, dir_window, s->disp_begin_pos, s->disp_begin_pos + s->cursor_pos );
                     DisplayDiskStatistic(s);
                     DisplayDirStatistic(dir_entry, NULL, s);
		     need_dsp_help = TRUE;
		     break;
      case ACTION_CMD_A: (void) ChangeDirModus( dir_entry );
                     DisplayTree( CurrentVolume, dir_window, s->disp_begin_pos, s->disp_begin_pos + s->cursor_pos );
                     DisplayDiskStatistic(s);
                     DisplayDirStatistic(dir_entry, NULL, s);
		     need_dsp_help = TRUE;
		     break;

      case ACTION_TOGGLE_COMPACT:
             GlobalView->fixed_col_width = (GlobalView->fixed_col_width == 0) ? 32 : 0;
             resize_request = TRUE;
             break;

      case ACTION_CMD_P: /* Pipe Directory */
                     PipeDirectory(dir_entry);
                     need_dsp_help = TRUE;
                     break;

      /* Volume Cycling and Selection */
      case ACTION_VOL_MENU: /* Shift-K: Select Loaded Volume */
          {
              int res = SelectLoadedVolume();
              if (res == 0) { /* If volume switch was successful */
                  s = &CurrentVolume->vol_stats; /* Update stats pointer for new volume */

                  /* Safety check / Clamping */
                  if (CurrentVolume->total_dirs > 0) {
                      /* If saved position is beyond current end, clamp to last item */
                      if (s->disp_begin_pos + s->cursor_pos >= CurrentVolume->total_dirs) {
                           /* Clamp to last valid index */
                           int last_idx = CurrentVolume->total_dirs - 1;
                           /* Determine new disp_begin_pos and cursor_pos */
                           /* Try to keep cursor at bottom of screen if possible */
                           if (last_idx >= layout.dir_win_height) {
                               s->disp_begin_pos = last_idx - (layout.dir_win_height - 1);
                               s->cursor_pos = layout.dir_win_height - 1;
                           } else {
                               s->disp_begin_pos = 0;
                               s->cursor_pos = last_idx;
                           }
                      }
                      /* Now safe to assign dir_entry */
                      dir_entry = CurrentVolume->dir_entry_list[s->disp_begin_pos + s->cursor_pos].dir_entry;
                  } else {
                      dir_entry = s->tree;
                  }

                  DisplayMenu(); /* Force redraw of frame/separator */
                  DisplayTree(CurrentVolume, dir_window, s->disp_begin_pos, s->disp_begin_pos + s->cursor_pos);
                  DisplayFileWindow(dir_entry, file_window); /* Refresh file window for the new directory */
                  RefreshWindow(file_window);
                  DisplayDiskStatistic(s);
                  DisplayDirStatistic(dir_entry, NULL, s);
                  DisplayAvailBytes(s);
                  /* Update header path after volume switch */
                  {
                      char path[PATH_LENGTH];
                      GetPath(dir_entry, path);
                      DisplayHeaderPath(path);
                  }
                  need_dsp_help = TRUE;
              }
          }
          break;

      case ACTION_VOL_PREV: /* Previous Volume */
          {
              int res = CycleLoadedVolume(-1);
              if (res == 0) { /* If volume switch was successful */
                  s = &CurrentVolume->vol_stats; /* Update stats pointer for new volume */

                  /* Safety check / Clamping */
                  if (CurrentVolume->total_dirs > 0) {
                      /* If saved position is beyond current end, clamp to last item */
                      if (s->disp_begin_pos + s->cursor_pos >= CurrentVolume->total_dirs) {
                           /* Clamp to last valid index */
                           int last_idx = CurrentVolume->total_dirs - 1;
                           /* Determine new disp_begin_pos and cursor_pos */
                           /* Try to keep cursor at bottom of screen if possible */
                           if (last_idx >= layout.dir_win_height) {
                               s->disp_begin_pos = last_idx - (layout.dir_win_height - 1);
                               s->cursor_pos = layout.dir_win_height - 1;
                           } else {
                               s->disp_begin_pos = 0;
                               s->cursor_pos = last_idx;
                           }
                      }
                      /* Now safe to assign dir_entry */
                      dir_entry = CurrentVolume->dir_entry_list[s->disp_begin_pos + s->cursor_pos].dir_entry;
                  } else {
                      dir_entry = s->tree;
                  }

                  DisplayMenu(); /* Force redraw of frame/separator */
                  DisplayTree(CurrentVolume, dir_window, s->disp_begin_pos, s->disp_begin_pos + s->cursor_pos);
                  DisplayFileWindow(dir_entry, file_window); /* Refresh file window for the new directory */
                  RefreshWindow(file_window);
                  DisplayDiskStatistic(s);
                  DisplayDirStatistic(dir_entry, NULL, s);
                  DisplayAvailBytes(s);
                  /* Update header path after volume switch */
                  {
                      char path[PATH_LENGTH];
                      GetPath(dir_entry, path);
                      DisplayHeaderPath(path);
                  }
                  need_dsp_help = TRUE;
              }
          }
          break;

      case ACTION_VOL_NEXT: /* Next Volume */
          {
              int res = CycleLoadedVolume(1);
              if (res == 0) { /* If volume switch was successful */
                  s = &CurrentVolume->vol_stats; /* Update stats pointer for new volume */

                  /* Safety check / Clamping */
                  if (CurrentVolume->total_dirs > 0) {
                      /* If saved position is beyond current end, clamp to last item */
                      if (s->disp_begin_pos + s->cursor_pos >= CurrentVolume->total_dirs) {
                           /* Clamp to last valid index */
                           int last_idx = CurrentVolume->total_dirs - 1;
                           /* Determine new disp_begin_pos and cursor_pos */
                           /* Try to keep cursor at bottom of screen if possible */
                           if (last_idx >= layout.dir_win_height) {
                               s->disp_begin_pos = last_idx - (layout.dir_win_height - 1);
                               s->cursor_pos = layout.dir_win_height - 1;
                           } else {
                               s->disp_begin_pos = 0;
                               s->cursor_pos = last_idx;
                           }
                      }
                      /* Now safe to assign dir_entry */
                      dir_entry = CurrentVolume->dir_entry_list[s->disp_begin_pos + s->cursor_pos].dir_entry;
                  } else {
                      dir_entry = s->tree;
                  }

                  DisplayMenu(); /* Force redraw of frame/separator */
                  DisplayTree(CurrentVolume, dir_window, s->disp_begin_pos, s->disp_begin_pos + s->cursor_pos);
                  DisplayFileWindow(dir_entry, file_window); /* Refresh file window for the new directory */
                  RefreshWindow(file_window);
                  DisplayDiskStatistic(s);
                  DisplayDirStatistic(dir_entry, NULL, s);
                  DisplayAvailBytes(s);
                  /* Update header path after volume switch */
                  {
                      char path[PATH_LENGTH];
                      GetPath(dir_entry, path);
                      DisplayHeaderPath(path);
                  }
                  need_dsp_help = TRUE;
              }
          }
          break;

      case ACTION_QUIT_DIR:
                     need_dsp_help = TRUE;
                     QuitTo(dir_entry);
                     break;

      case ACTION_QUIT:
                     need_dsp_help = TRUE;
                     Quit();
                     action = ACTION_NONE;
                     break;

      case ACTION_LOGIN:
          if( mode != DISK_MODE && mode != USER_MODE ) {
              if (getcwd(new_login_path, sizeof(new_login_path)) == NULL) {
                  strcpy(new_login_path, ".");
              }
          } else {
              (void) GetPath( dir_entry, new_login_path );
          }
          if( !GetNewLoginPath( new_login_path ) )
          {
              int ret; /* DEBUG variable */
              DisplayMenu();
              doupdate();

              ret = LoginDisk(new_login_path);

              /* Check return value. Only update state if login succeeded (0). */
              if (ret == 0) {
                  s = &CurrentVolume->vol_stats; /* Update stats pointer for new volume */

                  /* For a NEW login, we typically want to start at the top unless logic suggests otherwise.
                   * LoginDisk logic in log.c attempts to restore position if volume existed.
                   * Here we respect what LoginDisk/CurrentVolume has set, but clamp for safety. */

                  /* Safety check / Clamping */
                  if (CurrentVolume->total_dirs > 0) {
                      /* If saved position is beyond current end, clamp to last item */
                      if (s->disp_begin_pos + s->cursor_pos >= CurrentVolume->total_dirs) {
                           int last_idx = CurrentVolume->total_dirs - 1;
                           if (last_idx >= layout.dir_win_height) {
                               s->disp_begin_pos = last_idx - (layout.dir_win_height - 1);
                               s->cursor_pos = layout.dir_win_height - 1;
                           } else {
                               s->disp_begin_pos = 0;
                               s->cursor_pos = last_idx;
                           }
                      }
                      /* Now safe to assign dir_entry */
                      dir_entry = CurrentVolume->dir_entry_list[s->disp_begin_pos + s->cursor_pos].dir_entry;
                  } else {
                      dir_entry = s->tree;
                  }

                  DisplayMenu(); /* Redraw menu/frame */

                  /* Force Full Display Refresh */
                  DisplayTree(CurrentVolume, dir_window, s->disp_begin_pos, s->disp_begin_pos + s->cursor_pos);
                  DisplayFileWindow(dir_entry, file_window);
                  RefreshWindow(file_window);
                  DisplayDiskStatistic(s);
                  DisplayDirStatistic(dir_entry, NULL, s);
                  DisplayAvailBytes(s);
                  /* Update header path */
                  {
                      char path[PATH_LENGTH];
                      GetPath(dir_entry, path);
                      DisplayHeaderPath(path);
                  }
              }
              need_dsp_help = TRUE;
          }
          break;
      /* Ctrl-L is now ACTION_REFRESH, handled above */
      default :      /* Unhandled action, beep */
                     beep();
                     break;
    } /* switch */
  } while( action != ACTION_QUIT && action != ACTION_ENTER && action != ACTION_ESCAPE && action != ACTION_LOGIN ); /* Loop until explicit quit, escape or login */
  return( ch ); /* Return the last raw character that caused exit */
}



int ScanSubTree( DirEntry *dir_entry, Statistic *s )
{
  DirEntry *de_ptr;
  char new_login_path[PATH_LENGTH + 1];

  if( dir_entry->not_scanned ) {
    for( de_ptr=dir_entry->sub_tree; de_ptr; de_ptr=de_ptr->next) {
      GetPath( de_ptr, new_login_path );
      if (ReadTree( de_ptr, new_login_path, 999, s ) == -1) {
          /* Abort signal received from ReadTree */
          return -1;
      }
      ApplyFilter( de_ptr, s );
    }
    dir_entry->not_scanned = FALSE;
  } else {
    for( de_ptr=dir_entry->sub_tree; de_ptr; de_ptr=de_ptr->next) {
      if (ScanSubTree( de_ptr, s ) == -1) {
          /* Abort signal received from recursive ScanSubTree */
          return -1;
      }
    }
  }
  return( 0 );
}




int KeyF2Get(DirEntry *start_dir_entry,
             int disp_begin_pos,
	     int cursor_pos,
             char *path)
{
  int ch;
  int result = -1;
  int win_width, win_height;
  struct Volume *target_vol;
  int local_disp_begin_pos;
  int local_cursor_pos;
  YtreeAction action; /* Declare YtreeAction variable */

  if (mode != DISK_MODE && mode != USER_MODE) {
      /* Search for a volume that is in DISK_MODE */
      struct Volume *v, *tmp;
      struct Volume *disk_vol = NULL;

      HASH_ITER(hh, VolumeList, v, tmp) {
          /* Renamed usage: v->vol_stats.mode -> v->vol_stats.login_mode */
          if (v->vol_stats.login_mode == DISK_MODE) {
              disk_vol = v;
              break;
          }
      }

      if (!disk_vol) {
          MESSAGE("No filesystem volume active");
          return -1;
      }

      target_vol = disk_vol;
      local_disp_begin_pos = disk_vol->vol_stats.disp_begin_pos;
      local_cursor_pos = disk_vol->vol_stats.cursor_pos;
  } else {
      target_vol = CurrentVolume;
      local_disp_begin_pos = disp_begin_pos;
      local_cursor_pos = cursor_pos;
  }

  BuildDirEntryList(target_vol);

  /* Safety bounds check */
  if (local_disp_begin_pos < 0) local_disp_begin_pos = 0;
  if (local_cursor_pos < 0) local_cursor_pos = 0;
  if (target_vol->total_dirs > 0 && (local_disp_begin_pos + local_cursor_pos >= target_vol->total_dirs)) {
      local_disp_begin_pos = 0;
      local_cursor_pos = 0;
  }

  GetMaxYX(f2_window, &win_height, &win_width);
  MapF2Window();
  DisplayTree(target_vol, f2_window, local_disp_begin_pos, local_disp_begin_pos + local_cursor_pos );
  do
  {
    RefreshWindow( f2_window );
    doupdate();
    ch = Getch();
    GetMaxYX(f2_window, &win_height, &win_width);  /* Maybe changed... */
    /* LF to CR normalization is now handled by GetKeyAction */

    action = GetKeyAction(ch); /* Translate raw input to YtreeAction */

    switch( action )
    {
      case ACTION_NONE:       break; /* -1 or unhandled keys, no beep in F2Get */
      case ACTION_MOVE_DOWN: if( local_disp_begin_pos + local_cursor_pos + 1 >= target_vol->total_dirs )
		     {
		     }
		     else
		     {
		       if( local_cursor_pos + 1 < win_height )
		       {
			 PrintDirEntry( target_vol, f2_window,
			                local_disp_begin_pos + local_cursor_pos,
					local_cursor_pos, FALSE);
			 local_cursor_pos++;

			 PrintDirEntry( target_vol, f2_window, local_disp_begin_pos + local_cursor_pos,
					local_cursor_pos, TRUE);
                       }
		       else
		       {
			 local_disp_begin_pos++;
                         DisplayTree( target_vol, f2_window, local_disp_begin_pos,
				      local_disp_begin_pos + local_cursor_pos);
                       }
		     }
                     break;

      case ACTION_MOVE_UP: if( local_disp_begin_pos + local_cursor_pos - 1 < 0 )
		     {
		     }
		     else
		     {
		       if( local_cursor_pos - 1 >= 0 )
		       {
			 PrintDirEntry( target_vol, f2_window,
			                local_disp_begin_pos + local_cursor_pos,
					local_cursor_pos, FALSE );
			 local_cursor_pos--;
			 PrintDirEntry( target_vol, f2_window,
			                local_disp_begin_pos + local_cursor_pos,
					local_cursor_pos, TRUE );
                       }
		       else
		       {
			 local_disp_begin_pos--;
                         DisplayTree( target_vol, f2_window, local_disp_begin_pos,
				      local_disp_begin_pos + local_cursor_pos);
                       }
		     }
                     break;

      case ACTION_PAGE_DOWN:
		     if( local_disp_begin_pos + local_cursor_pos >= target_vol->total_dirs - 1 )
		     {
		     }
		     else
		     {
		       if( local_cursor_pos < win_height - 1 )
		       {
			 PrintDirEntry( target_vol, f2_window,
			                local_disp_begin_pos + local_cursor_pos,
					local_cursor_pos, FALSE );
		         if( local_disp_begin_pos + win_height > target_vol->total_dirs  - 1 )
			   local_cursor_pos = target_vol->total_dirs - local_disp_begin_pos - 1;
			 else
			   local_cursor_pos = win_height - 1;
			 PrintDirEntry( target_vol, f2_window,
			                local_disp_begin_pos + local_cursor_pos,
					local_cursor_pos, TRUE );
		       }
		       else
		       {
			 if( local_disp_begin_pos + local_cursor_pos + win_height < target_vol->total_dirs )
			 {
			   local_disp_begin_pos += win_height;
			   local_cursor_pos = win_height - 1;
			 }
			 else
			 {
			   local_disp_begin_pos = target_vol->total_dirs - win_height;
			   if( local_disp_begin_pos < 0 ) local_disp_begin_pos = 0;
			   local_cursor_pos = target_vol->total_dirs - local_disp_begin_pos - 1;
			 }
                         DisplayTree( target_vol, f2_window, local_disp_begin_pos,
				      local_disp_begin_pos + local_cursor_pos);
		       }
		     }
                     break;

      case ACTION_PAGE_UP:
		     if( local_disp_begin_pos + local_cursor_pos <= 0 )
		     {
		     }
		     else
		     {
		       if( local_cursor_pos > 0 )
		       {
			 PrintDirEntry( target_vol, f2_window,
			                local_disp_begin_pos + local_cursor_pos,
					local_cursor_pos, FALSE );
			 local_cursor_pos = 0;
			 PrintDirEntry( target_vol, f2_window,
			                local_disp_begin_pos + local_cursor_pos,
					local_cursor_pos, TRUE );
		       }
		       else
		       {
			 if( (local_disp_begin_pos -= win_height) < 0 )
			 {
			   local_disp_begin_pos = 0;
			 }
                         local_cursor_pos = 0;
                         DisplayTree( target_vol, f2_window, local_disp_begin_pos,
				      local_disp_begin_pos + local_cursor_pos);
		       }
		     }
                     break;

      case ACTION_HOME: if( local_disp_begin_pos == 0 && local_cursor_pos == 0 )
		     { }
		     else
		     {
		       local_disp_begin_pos = 0;
		       local_cursor_pos     = 0;
                       DisplayTree( target_vol, f2_window, local_disp_begin_pos,
				    local_disp_begin_pos + local_cursor_pos);
		     }
                     break;

      case ACTION_END: local_disp_begin_pos = MAXIMUM(0, target_vol->total_dirs - win_height);
		     local_cursor_pos     = target_vol->total_dirs - local_disp_begin_pos - 1;
                     DisplayTree( target_vol, f2_window, local_disp_begin_pos,
				  local_disp_begin_pos + local_cursor_pos);
                     break;

      case ACTION_ENTER:
                     GetPath(target_vol->dir_entry_list[local_cursor_pos + local_disp_begin_pos].dir_entry, path);
		     result = 0;
		     break;
      case ACTION_QUIT:      break;
      case ACTION_ESCAPE:    break;

      default :      beep(); break;
    } /* switch */
  } while( action != ACTION_QUIT && action != ACTION_ENTER && action != ACTION_ESCAPE );

  UnmapF2Window();

  /* Restore the original directory list for the main window if we switched volume context */
  /* Actually, BuildDirEntryList now writes to vol structure, so other volume caches are preserved.
     We only need to ensure the CurrentVolume is consistent for the main window logic, which is untouched here. */
  if (target_vol != CurrentVolume) {
     /* If we messed with another volume's list, that's fine, it's encapsulated. */
  } else {
     /* We modified current volume's list. Ensure it's valid. */
     BuildDirEntryList(CurrentVolume);
  }

  if (action == ACTION_ESCAPE || action == ACTION_QUIT) return -1;

  return( result );
}



int RefreshDirWindow()
{
	DirEntry *de_ptr;
	int i, n;
	int result = -1;
	int window_width, window_height;
    Statistic *s = &CurrentVolume->vol_stats;

	de_ptr = CurrentVolume->dir_entry_list[s->disp_begin_pos + s->cursor_pos].dir_entry;
	BuildDirEntryList( CurrentVolume );

	/* Search old entry */
	for(n=-1, i=0;i < CurrentVolume->total_dirs; i++) {
		if(CurrentVolume->dir_entry_list[i].dir_entry == de_ptr) {
			n = i;
			break;
		}
	}

	if(n == -1) {
		/* Directory disapeared */
      		ERROR_MSG( "Current directory disappeared" );
		result = -1;
	} else {

		if( n != (s->disp_begin_pos + s->cursor_pos)) {
			/* Position changed */
			if((n - s->disp_begin_pos) >= 0) {
				s->cursor_pos = n - s->disp_begin_pos;
			} else {
				s->disp_begin_pos = n;
				s->cursor_pos = 0;
			}
		}

       		getmaxyx(dir_window, window_height, window_width);
	        while(s->cursor_pos >= window_height) {
		  s->cursor_pos--;
		  s->disp_begin_pos++;
	        }
		DisplayTree( CurrentVolume, dir_window, s->disp_begin_pos,
		     	s->disp_begin_pos + s->cursor_pos
		   	);

		DisplayAvailBytes(s);
		DisplayDiskStatistic(s);
		DisplayDirStatistic(CurrentVolume->dir_entry_list[s->disp_begin_pos + s->cursor_pos].dir_entry, NULL, s);
        /* Update header path after refresh */
        {
            char path[PATH_LENGTH];
            GetPath(CurrentVolume->dir_entry_list[s->disp_begin_pos + s->cursor_pos].dir_entry, path);
            DisplayHeaderPath(path);
        }
		result = 0;
	}

	return(result);
}
