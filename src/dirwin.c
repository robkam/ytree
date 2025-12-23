/***************************************************************************
 *
 * dirwin.c
 * Functions for handling the directory window
 *
 ***************************************************************************/


#include "ytree.h"


static DirEntryList *dir_entry_list = NULL;
static size_t dir_entry_list_capacity = 0;
static int current_dir_entry;
static int total_dirs;
static int window_height, window_width;

static void ReadDirList(DirEntry *dir_entry);
static void PrintDirEntry(WINDOW *win, int entry_no, int y, unsigned char hilight);
void BuildDirEntryList(DirEntry *dir_entry, Statistic *stat_source); /* Removed static */
void FreeDirEntryList(void); /* ADDED: Prototype for FreeDirEntryList */
static void HandleReadSubTree(DirEntry *dir_entry, DirEntry *start_dir_entry, BOOL *need_dsp_help);
static void HandleUnreadSubTree(DirEntry *dir_entry, DirEntry *de_ptr, DirEntry *start_dir_entry, BOOL *need_dsp_help);
static void MoveEnd(DirEntry **dir_entry);
static void MoveHome(DirEntry **dir_entry);
static void HandlePlus(DirEntry *dir_entry, DirEntry *de_ptr, char *new_login_path, DirEntry *start_dir_entry, BOOL *need_dsp_help);
static void HandleTagDir(DirEntry *dir_entry, BOOL value);
static void HandleTagAllDirs(DirEntry *dir_entry, BOOL value );
static void HandleShowAll(BOOL tagged_only, DirEntry *dir_entry, DirEntry *start_dir_entry, BOOL *need_dsp_help, int *ch);
static void HandleSwitchWindow(DirEntry *dir_entry, DirEntry *start_dir_entry, BOOL *need_dsp_help, int *ch);
static void Movedown(int *disp_begin_pos, int *cursor_pos, DirEntry **dir_entry); /* Forward declaration */

static int dir_mode;


void BuildDirEntryList(DirEntry *dir_entry, Statistic *stat_source) /* Removed static */
{
  if( dir_entry_list )
  {
    free( dir_entry_list );
    dir_entry_list = NULL;
    dir_entry_list_capacity = 0;
  }

  /* Initialize with the estimated count, but enforce a minimum safety size */
  size_t alloc_count = stat_source->disk_total_directories;
  if (alloc_count < 16) alloc_count = 16;

  if( ( dir_entry_list = ( DirEntryList *)
                   calloc( alloc_count,
                   sizeof( DirEntryList )
                 ) ) == NULL )
  {
    ERROR_MSG( "Calloc Failed*ABORT" );
    exit( 1 );
  }

  dir_entry_list_capacity = alloc_count;
  current_dir_entry = 0;

  /* Only read if we have a valid tree structure */
  if (dir_entry) {
      ReadDirList( dir_entry );
  }

  total_dirs = current_dir_entry;

#ifdef DEBUG
  if(stat_source->disk_total_directories != total_dirs)
  {
      /* mismatch detected, but safely handled by realloc in ReadDirList */
  }
#endif
}

/*
 * Frees the memory allocated for the dir_entry_list array.
 * This function should be called on program exit to prevent memory leaks.
 */
void FreeDirEntryList(void)
{
    if (dir_entry_list != NULL)
    {
        free(dir_entry_list);
        dir_entry_list = NULL;
        dir_entry_list_capacity = 0;
        current_dir_entry = 0;
        total_dirs = 0; /* Also reset total_dirs as the list is now empty */
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



static void ReadDirList(DirEntry *dir_entry)
{
  DirEntry    *de_ptr;
  static int  level = 0;
  static unsigned long indent = 0L;

  for( de_ptr = dir_entry; de_ptr; de_ptr = de_ptr->next )
  {
    /* Check visibility.
       Hide files starting with '.' if option is set.
       EXCEPTION: Never hide the top-level root directory of the current session.
       Use statistic.tree comparison instead of up_tree check to handle archive
       structures where root-level items might have NULL parents.
    */
    if (hide_dot_files && de_ptr->name[0] == '.') {
        if (de_ptr != statistic.tree)
            continue;
    }

    /* Bounds Checking & Dynamic Reallocation */
    if (current_dir_entry >= (int)dir_entry_list_capacity) {
        size_t new_capacity = dir_entry_list_capacity * 2;
        if (new_capacity == 0) new_capacity = 128; /* Fallback if 0 start */

        DirEntryList *new_list = (DirEntryList *) realloc(dir_entry_list, new_capacity * sizeof(DirEntryList));

        if (new_list == NULL) {
             ERROR_MSG("Realloc failed in ReadDirList*ABORT");
             exit(1);
        }

        /* Zero out the newly allocated portion to maintain calloc-like safety */
        memset(new_list + dir_entry_list_capacity, 0, (new_capacity - dir_entry_list_capacity) * sizeof(DirEntryList));

        dir_entry_list = new_list;
        dir_entry_list_capacity = new_capacity;
    }

    indent &= ~( 1L << level );
    if( de_ptr->next ) indent |= ( 1L << level );

    dir_entry_list[current_dir_entry].dir_entry = de_ptr;
    dir_entry_list[current_dir_entry].level = (unsigned short) level;
    dir_entry_list[current_dir_entry].indent = indent;

    current_dir_entry++;

    if( !de_ptr->not_scanned && de_ptr->sub_tree )
    {
      level++;
      ReadDirList( de_ptr->sub_tree );
      level--;
    }
  }
}


/*
 * Helper function to return the currently selected directory entry.
 * Used by filewin.c and other modules that can't access dir_entry_list directly.
 */
DirEntry *GetSelectedDirEntry(void)
{
  if (dir_entry_list != NULL && total_dirs > 0) {
    int idx = statistic.disp_begin_pos + statistic.cursor_pos;

    /* Safety bounds check */
    if (idx < 0) idx = 0;
    if (idx >= total_dirs) idx = total_dirs - 1;

    return dir_entry_list[idx].dir_entry;
  }
  /* Fallback to root if list is empty/invalid */
  return statistic.tree;
}


static void PrintDirEntry(WINDOW *win,
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
  for(j=0; j < dir_entry_list[entry_no].level; j++)
  {
    if( dir_entry_list[entry_no].indent & ( 1L << j ) )
      (void) strcat( graph_buffer, "| " );
    else
      (void) strcat( graph_buffer, "  " );
  }
  de_ptr = dir_entry_list[entry_no].dir_entry;
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
      max_name_len = window_width - graph_len - 1;
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




void DisplayTree(WINDOW *win, int start_entry_no, int hilight_no)
{
  int i, y;
  int width, height;

  y = -1;
  GetMaxYX(win, &height, &width);
  if(win == dir_window) {
    window_width  = width;
    window_height = height;
  }
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
    if ( start_entry_no + i >= total_dirs ) break;

    if( start_entry_no + i != hilight_no )
      PrintDirEntry( win, start_entry_no + i, i, FALSE );
    else
      y = i;
  }

  if( y >= 0 ) PrintDirEntry( win, start_entry_no + y, y, TRUE );

}

static void Movedown(int *disp_begin_pos, int *cursor_pos, DirEntry **dir_entry)
{
   if( *disp_begin_pos + *cursor_pos + 1 >= total_dirs )
   {
      /* Element not present */
      /*-------------------------*/
      return;
   }

    if (*cursor_pos + 1 < window_height) {
        (*cursor_pos)++;
    } else {
        (*disp_begin_pos)++;
    }
    *dir_entry = dir_entry_list[*disp_begin_pos + *cursor_pos].dir_entry;
    (*dir_entry)->start_file = 0;
    (*dir_entry)->cursor_pos = -1;
    DisplayTree(dir_window, *disp_begin_pos, *disp_begin_pos + *cursor_pos);
    DisplayFileWindow( *dir_entry );
    RefreshWindow( file_window );
    DisplayDirStatistic(*dir_entry, NULL); /* Updated call */
    /* Update header path */
    {
        char path[PATH_LENGTH];
        GetPath(*dir_entry, path);
        DisplayHeaderPath(path);
    }
}


static void Moveup(int *disp_begin_pos, int *cursor_pos, DirEntry **dir_entry)
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
    *dir_entry = dir_entry_list[*disp_begin_pos + *cursor_pos].dir_entry;
    (*dir_entry)->start_file = 0;
    (*dir_entry)->cursor_pos = -1;
    DisplayTree(dir_window, *disp_begin_pos, *disp_begin_pos + *cursor_pos);
    DisplayFileWindow( *dir_entry );
    RefreshWindow( file_window );
    DisplayDirStatistic(*dir_entry, NULL); /* Updated call */
    /* Update header path */
    {
        char path[PATH_LENGTH];
        GetPath(*dir_entry, path);
        DisplayHeaderPath(path);
    }
}


static void Movenpage(int *disp_begin_pos, int *cursor_pos, DirEntry **dir_entry)
{
   if( *disp_begin_pos + *cursor_pos >= total_dirs - 1 )
   {
      /* Last position */
      /*-----------------*/
   }
   else
   {
      if( *cursor_pos < window_height - 1 )
      {
          /* Cursor is not on last entry of the page */
           if( *disp_begin_pos + window_height > total_dirs  - 1 )
              *cursor_pos = total_dirs - *disp_begin_pos - 1;
           else
              *cursor_pos = window_height - 1;
      }
      else
      {
          /* Scrolling */
          /*----------*/
          if( *disp_begin_pos + *cursor_pos + window_height < total_dirs )
          {
              *disp_begin_pos += window_height;
              *cursor_pos = window_height - 1;
          }
          else
          {
              *disp_begin_pos = total_dirs - window_height;
              if( *disp_begin_pos < 0 ) *disp_begin_pos = 0;
              *cursor_pos = total_dirs - *disp_begin_pos - 1;
          }
      }
      *dir_entry = dir_entry_list[*disp_begin_pos + *cursor_pos].dir_entry;
      (*dir_entry)->start_file = 0;
      (*dir_entry)->cursor_pos = -1;
      DisplayTree(dir_window,*disp_begin_pos,*disp_begin_pos+*cursor_pos);
      DisplayFileWindow( *dir_entry );
      RefreshWindow( file_window );
      DisplayDirStatistic(*dir_entry, NULL); /* Updated call */
      /* Update header path */
      {
          char path[PATH_LENGTH];
          GetPath(*dir_entry, path);
          DisplayHeaderPath(path);
      }
   }
   return;
}

static void Moveppage(int *disp_begin_pos, int *cursor_pos, DirEntry **dir_entry)
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
         if( (*disp_begin_pos -= window_height) < 0 )
         {
             *disp_begin_pos = 0;
         }
         *cursor_pos = 0;
      }
      *dir_entry = dir_entry_list[*disp_begin_pos + *cursor_pos].dir_entry;
      (*dir_entry)->start_file = 0;
      (*dir_entry)->cursor_pos = -1;
      DisplayTree(dir_window,*disp_begin_pos,*disp_begin_pos+*cursor_pos);
      DisplayFileWindow( *dir_entry );
      RefreshWindow( file_window );
      DisplayDirStatistic(*dir_entry, NULL); /* Updated call */
      /* Update header path */
      {
          char path[PATH_LENGTH];
          GetPath(*dir_entry, path);
          DisplayHeaderPath(path);
      }
   }
   return;
}

static void MoveEnd(DirEntry **dir_entry)
{
    statistic.disp_begin_pos = MAXIMUM(0, total_dirs - window_height);
    statistic.cursor_pos     = total_dirs - statistic.disp_begin_pos - 1;
    *dir_entry = dir_entry_list[statistic.disp_begin_pos + statistic.cursor_pos].dir_entry;
    (*dir_entry)->start_file = 0;
    (*dir_entry)->cursor_pos = -1;
    DisplayFileWindow( *dir_entry );
    RefreshWindow( file_window );
    DisplayTree( dir_window, statistic.disp_begin_pos,
		statistic.disp_begin_pos + statistic.cursor_pos);
    DisplayDirStatistic(*dir_entry, NULL); /* Updated call */
    /* Update header path */
    {
        char path[PATH_LENGTH];
        GetPath(*dir_entry, path);
        DisplayHeaderPath(path);
    }
    return;
}

static void MoveHome(DirEntry **dir_entry)
{
    if( statistic.disp_begin_pos == 0 && statistic.cursor_pos == 0 )
    {  /* Position 1 already reached */
       /*----------------------------*/
    }
    else
    {
       statistic.disp_begin_pos = 0;
       statistic.cursor_pos     = 0;
       *dir_entry = dir_entry_list[statistic.disp_begin_pos + statistic.cursor_pos].dir_entry;
       (*dir_entry)->start_file = 0;
       (*dir_entry)->cursor_pos = -1;
       DisplayFileWindow( *dir_entry );
       RefreshWindow( file_window );
       DisplayTree( dir_window, statistic.disp_begin_pos,
		statistic.disp_begin_pos + statistic.cursor_pos);
       DisplayDirStatistic(*dir_entry, NULL); /* Updated call */
       /* Update header path */
       {
           char path[PATH_LENGTH];
           GetPath(*dir_entry, path);
           DisplayHeaderPath(path);
       }
    }
    return;
}

static void HandlePlus(DirEntry *dir_entry, DirEntry *de_ptr, char *new_login_path,
		       DirEntry *start_dir_entry, BOOL *need_dsp_help)
{
    if (mode != DISK_MODE && mode != USER_MODE) {
        return;
    }
    if( !dir_entry->not_scanned ) {
    } else {
        SuspendClock(); /* Suspend clock before scanning */
	for( de_ptr=dir_entry->sub_tree; de_ptr; de_ptr=de_ptr->next) {
	    GetPath( de_ptr, new_login_path );
	    ReadTree( de_ptr, new_login_path, 0 );
	    ApplyFilter( de_ptr );
	}
        InitClock();    /* Resume clock after scanning */
	dir_entry->not_scanned = FALSE;
	BuildDirEntryList( start_dir_entry, &statistic );
	DisplayTree( dir_window, statistic.disp_begin_pos,
                 statistic.disp_begin_pos + statistic.cursor_pos );
	DisplayDiskStatistic();
    DisplayDirStatistic(dir_entry, NULL); /* Updated call */
	DisplayAvailBytes();
	*need_dsp_help = TRUE;
    }
}

static void HandleReadSubTree(DirEntry *dir_entry, DirEntry *start_dir_entry,
		       BOOL *need_dsp_help)
{
    SuspendClock(); /* Suspend clock before scanning */
    if (ScanSubTree(dir_entry) == -1) {
        /* Aborted. Fall through to refresh what we have. */
    }
    InitClock();    /* Resume clock after scanning */
    BuildDirEntryList( start_dir_entry, &statistic );
    DisplayTree( dir_window, statistic.disp_begin_pos,
		 statistic.disp_begin_pos + statistic.cursor_pos );
    RecalculateSysStats(); /* Fix for Bug 10: Force full recalculation */
    DisplayDiskStatistic();
    DisplayDirStatistic(dir_entry, NULL); /* Updated call */
    DisplayAvailBytes();
    *need_dsp_help = TRUE;
}

static void HandleUnreadSubTree(DirEntry *dir_entry, DirEntry *de_ptr,
			 DirEntry *start_dir_entry, BOOL *need_dsp_help)
{
    if (mode != DISK_MODE && mode != USER_MODE) {
        return;
    }
    if( dir_entry->not_scanned || (dir_entry->sub_tree == NULL) ) {
    } else {
	for( de_ptr=dir_entry->sub_tree; de_ptr; de_ptr=de_ptr->next) {
	    UnReadTree( de_ptr );
	}
	dir_entry->not_scanned = TRUE;
	BuildDirEntryList( start_dir_entry, &statistic );
        DisplayTree( dir_window, statistic.disp_begin_pos,
		    statistic.disp_begin_pos + statistic.cursor_pos );
        DisplayAvailBytes();
        RecalculateSysStats(); /* Fix for Bug 10: Force full recalculation */
        DisplayDiskStatistic();
        DisplayDirStatistic(dir_entry, NULL); /* Updated call */
        *need_dsp_help = TRUE;
    }
    return;
}

static void HandleTagDir(DirEntry *dir_entry, BOOL value)
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
	       	statistic.disk_tagged_files++;
		statistic.disk_tagged_bytes += fe_ptr->stat_struct.st_size;
	    }else{
		dir_entry->tagged_files--;
		dir_entry->tagged_bytes -= fe_ptr->stat_struct.st_size;
	       	statistic.disk_tagged_files--;
		statistic.disk_tagged_bytes -= fe_ptr->stat_struct.st_size;
	    }
	}
    }
    dir_entry->start_file = 0;
    dir_entry->cursor_pos = -1;
    DisplayFileWindow( dir_entry );
    RefreshWindow( file_window );
    DisplayDiskStatistic();
    DisplayDirStatistic(dir_entry, NULL); /* Updated call */
    return;
}

static void HandleTagAllDirs(DirEntry *dir_entry, BOOL value )
{
    FileEntry *fe_ptr;
    long i;
    for(i=0; i < total_dirs; i++)
    {
	for(fe_ptr=dir_entry_list[i].dir_entry->file; fe_ptr; fe_ptr=fe_ptr->next)
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
	       	    statistic.disk_tagged_files++;
		    statistic.disk_tagged_bytes += fe_ptr->stat_struct.st_size;
	        }else{
		    fe_ptr->tagged = value;
	       	    dir_entry->tagged_files--;
		    dir_entry->tagged_bytes -= fe_ptr->stat_struct.st_size;
	       	    statistic.disk_tagged_files--;
		    statistic.disk_tagged_bytes -= fe_ptr->stat_struct.st_size;
	        }
	    }
	}
    }
    dir_entry->start_file = 0;
    dir_entry->cursor_pos = -1;
    DisplayFileWindow( dir_entry );
    RefreshWindow( file_window );
    DisplayDiskStatistic();
    DisplayDirStatistic(dir_entry, NULL); /* Updated call */
    return;
}



static void HandleShowAll(BOOL tagged_only, DirEntry *dir_entry, DirEntry *start_dir_entry, BOOL *need_dsp_help, int *ch)
{
    if( (tagged_only) ? statistic.disk_tagged_files : statistic.disk_matching_files )
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
	    DisplayDiskStatistic();
	    DisplayDirStatistic(dir_entry, NULL); /* Updated call */
	    dir_entry->start_file = 0;
	    dir_entry->cursor_pos = -1;
            DisplayFileWindow( dir_entry );
            RefreshWindow( small_file_window );
            RefreshWindow( big_file_window );
	    BuildDirEntryList( start_dir_entry, &statistic );
            DisplayTree( dir_window, statistic.disp_begin_pos,
			 statistic.disp_begin_pos + statistic.cursor_pos);
	} else {
	    BuildDirEntryList( statistic.tree, &statistic );
            DisplayTree( dir_window, statistic.disp_begin_pos,
			statistic.disp_begin_pos + statistic.cursor_pos);
	    *ch = 'L';
	}
    } else {
	dir_entry->login_flag = FALSE;
    }
    *need_dsp_help = TRUE;
    return;
}

static void HandleSwitchWindow(DirEntry *dir_entry, DirEntry *start_dir_entry, BOOL *need_dsp_help, int *ch)
{
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
	    dir_entry->start_file = 0;
	    dir_entry->cursor_pos = -1;
	    DisplayFileWindow( dir_entry );
            RefreshWindow( small_file_window );
            RefreshWindow( big_file_window );
	    BuildDirEntryList( start_dir_entry, &statistic );
            DisplayTree( dir_window, statistic.disp_begin_pos,
			 statistic.disp_begin_pos + statistic.cursor_pos);
	    DisplayDiskStatistic();
        DisplayDirStatistic(dir_entry, NULL); /* Updated call */
	} else {
	    BuildDirEntryList( statistic.tree, &statistic );
            DisplayTree( dir_window, statistic.disp_begin_pos,
			statistic.disp_begin_pos + statistic.cursor_pos);
	    *ch = 'L';
	}
	DisplayAvailBytes();
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

    /* Suspend clock to prevent signal handler interrupt corrupting UI during rebuild */
    SuspendClock();

    /* 1. Identify the directory currently under the cursor */
    if (total_dirs > 0 && (statistic.disp_begin_pos + statistic.cursor_pos < total_dirs)) {
        target = dir_entry_list[statistic.disp_begin_pos + statistic.cursor_pos].dir_entry;
    } else {
        target = statistic.tree;
    }

    /* 2. Toggle State and Recalculate Stats */
    hide_dot_files = !hide_dot_files;
    RecalculateSysStats();

    /* 3. Rebuild the linear list of visible directories */
    BuildDirEntryList(statistic.tree, &statistic);

    /* 4. Search for the 'target' directory in the new list */
    DirEntry *search = target;
    while (search != NULL && found_idx == -1) {
        for (i = 0; i < total_dirs; i++) {
            if (dir_entry_list[i].dir_entry == search) {
                found_idx = i;
                break;
            }
        }
        /* If the target directory is now hidden, walk up to its parent */
        if (found_idx == -1) search = search->up_tree;
    }

    /* 5. Smart Restore of Cursor Position */
    GetMaxYX(dir_window, &win_height, &win_width);

    if (found_idx != -1) {
        /* Check if the found directory is within the current visible page */
        if (found_idx >= statistic.disp_begin_pos &&
            found_idx < statistic.disp_begin_pos + win_height) {
            /* It's still on screen. Just update the cursor, don't scroll/jump. */
            statistic.cursor_pos = found_idx - statistic.disp_begin_pos;
        } else {
            /* It moved off page. Re-center or adjust slightly. */
            if (found_idx < win_height) {
                statistic.disp_begin_pos = 0;
                statistic.cursor_pos = found_idx;
            } else {
                /* Center the item */
                statistic.disp_begin_pos = found_idx - (win_height / 2);

                /* Bounds check for display position */
                if (statistic.disp_begin_pos > total_dirs - win_height) {
                    statistic.disp_begin_pos = total_dirs - win_height;
                }
                if (statistic.disp_begin_pos < 0) statistic.disp_begin_pos = 0;

                statistic.cursor_pos = found_idx - statistic.disp_begin_pos;
            }
        }
    } else {
        /* Fallback to root if everything went wrong */
        statistic.disp_begin_pos = 0;
        statistic.cursor_pos = 0;
    }

    /* Sanity check cursor limits */
    if (statistic.cursor_pos >= win_height) statistic.cursor_pos = win_height - 1;
    if (statistic.disp_begin_pos + statistic.cursor_pos >= total_dirs) {
        /* Move cursor to last valid item */
        statistic.cursor_pos = (total_dirs > 0) ? (total_dirs - 1 - statistic.disp_begin_pos) : 0;
    }

    /* Refresh Directory Tree */
    DisplayTree(dir_window, statistic.disp_begin_pos, statistic.disp_begin_pos + statistic.cursor_pos);
    DisplayDiskStatistic();

    /* Update current dir pointer using the new accessor function
       because ToggleDotFiles might have changed the list layout */
    if (total_dirs > 0) {
        target = dir_entry_list[statistic.disp_begin_pos + statistic.cursor_pos].dir_entry;
    } else {
        target = statistic.tree;
    }

    /* Explicitly update the file window (preview) to match new visibility */
    DisplayFileWindow(target);
    RefreshWindow(file_window);
    DisplayDirStatistic(target, NULL); /* Updated call */
    /* Update header path */
    {
        char path[PATH_LENGTH];
        GetPath(target, path);
        DisplayHeaderPath(path);
    }

    InitClock(); /* Resume clock and restore signal handling */
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
  struct Volume *last_seen_volume = CurrentVolume; /* Track global volume changes */

  unput_char = 0;
  de_ptr = NULL;

  GetMaxYX(dir_window, &window_height, &window_width);

  /* Clear flags */
  /*-----------------*/

  dir_mode = MODE_3;

  need_dsp_help = TRUE;

  BuildDirEntryList( start_dir_entry, &statistic );
  if ( initial_directory != NULL )
  {
    if ( !strcmp( initial_directory, "." ) )   /* Entry just a single "." */
    {
      statistic.disp_begin_pos = 0;
      statistic.cursor_pos = 0;
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
      for ( i = 0; i < total_dirs; i++ )
      {
        if ( *new_login_path == FILE_SEPARATOR_CHAR )
          GetPath( dir_entry_list[i].dir_entry, new_name );
        else
          strcpy( new_name, dir_entry_list[i].dir_entry->name );
        if ( !strcmp( new_login_path, new_name ) )
        {
          statistic.disp_begin_pos = i;
          statistic.cursor_pos = 0;
          unput_char = CR;
          break;
        }
      }
    }
    initial_directory = NULL;
  }
  dir_entry = dir_entry_list[statistic.disp_begin_pos + statistic.cursor_pos].dir_entry;

  DisplayDiskStatistic();
  DisplayDirStatistic(dir_entry, NULL); /* Updated call */
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
  DisplayFileWindow( dir_entry );
  RefreshWindow( file_window );
  DisplayTree( dir_window, statistic.disp_begin_pos, statistic.disp_begin_pos + statistic.cursor_pos );
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
    if (CurrentVolume != last_seen_volume) {
        last_seen_volume = CurrentVolume;
        start_dir_entry = statistic.tree;

        /* Force Rebuild of Directory List */
        BuildDirEntryList(start_dir_entry, &statistic);

        /* Validate and use restored positions from statistic */
        if (total_dirs > 0) {
            if (statistic.disp_begin_pos >= total_dirs) statistic.disp_begin_pos = 0;
            if (statistic.disp_begin_pos < 0) statistic.disp_begin_pos = 0;
            if (statistic.cursor_pos >= window_height) statistic.cursor_pos = window_height - 1;
            if (statistic.disp_begin_pos + statistic.cursor_pos >= total_dirs) statistic.cursor_pos = 0;

            dir_entry = dir_entry_list[statistic.disp_begin_pos + statistic.cursor_pos].dir_entry;
        } else {
            dir_entry = statistic.tree;
        }

        /* Full Display Refresh */
        DisplayTree(dir_window, statistic.disp_begin_pos, statistic.disp_begin_pos + statistic.cursor_pos);
        DisplayFileWindow(dir_entry);
        RefreshWindow(file_window);
        DisplayDiskStatistic();
        DisplayDirStatistic(dir_entry, NULL); /* Updated call */
        DisplayAvailBytes();

        /* Update Header Path */
        {
            char path[PATH_LENGTH];
            GetPath(dir_entry, path);
            DisplayHeaderPath(path);
        }
    }

    if ( need_dsp_help )
    {
      need_dsp_help = FALSE;
      DisplayDirHelp();
    }
    DisplayDirParameter( dir_entry );
    RefreshWindow( dir_window );
    if( unput_char )
    {
      ch = unput_char;
      unput_char = '\0';
    }
    else
    {
      doupdate();
      ch = (resize_request) ? -1 : Getch();
      /* LF to CR normalization is now handled by GetKeyAction */
    }

    if (IsUserActionDefined()) { /* User commands take precedence */
        ch = DirUserMode(dir_entry, ch);
    }

    /* ViKey processing is now handled inside GetKeyAction */

    if(resize_request) {
       ReCreateWindows();
       DisplayMenu();
       GetMaxYX(dir_window, &window_height, &window_width);
       while(statistic.cursor_pos >= window_height) {
         statistic.cursor_pos--;
	 statistic.disp_begin_pos++;
       }
       DisplayTree( dir_window, statistic.disp_begin_pos,
		    statistic.disp_begin_pos + statistic.cursor_pos
		  );
       DisplayFileWindow(dir_entry);
       DisplayDiskStatistic();
       DisplayDirStatistic(dir_entry, NULL); /* Updated call */
       DisplayDirParameter( dir_entry );
       need_dsp_help = TRUE;
       DisplayAvailBytes();
       DisplayFilter();
       DisplayDiskName();
       /* Update header path after resize */
       {
           char path[PATH_LENGTH];
           GetPath(dir_entry, path);
           DisplayHeaderPath(path);
       }
       resize_request = FALSE;
    }

    action = GetKeyAction(ch); /* Translate raw input to YtreeAction */

    switch( action )
    {
      case ACTION_RESIZE: resize_request = TRUE;
      		       break;

      case ACTION_NONE:  /* -1 or unhandled keys */
                         if (ch == -1) break; /* Ignore -1 (resize_request handled above) */
                         /* Fall through for other unhandled keys to beep */
                         beep();
                         break;

      case ACTION_MOVE_DOWN: Movedown(&statistic.disp_begin_pos, &statistic.cursor_pos, &dir_entry);
                     break;
      case ACTION_MOVE_UP: Moveup(&statistic.disp_begin_pos, &statistic.cursor_pos, &dir_entry);
                     break;
      case ACTION_MOVE_SIBLING_NEXT:
            if (dir_entry->next != NULL) {
                /* Find the sibling in the linear list to update cursor index */
                int k;
                int found_idx = -1;
                /* Optimization: Start searching from current position */
                int start_idx = statistic.disp_begin_pos + statistic.cursor_pos;

                for (k = start_idx + 1; k < total_dirs; k++) {
                    if (dir_entry_list[k].dir_entry == dir_entry->next) {
                        found_idx = k;
                        break;
                    }
                }

                if (found_idx != -1) {
                    /* Move cursor to sibling */
                    if (found_idx >= statistic.disp_begin_pos &&
                        found_idx < statistic.disp_begin_pos + window_height) {
                        statistic.cursor_pos = found_idx - statistic.disp_begin_pos;
                    } else {
                        /* Off screen, center it or move to top */
                        statistic.disp_begin_pos = found_idx;
                        statistic.cursor_pos = 0;
                        /* Bounds check */
                        if (statistic.disp_begin_pos + window_height > total_dirs) {
                             statistic.disp_begin_pos = MAXIMUM(0, total_dirs - window_height);
                             statistic.cursor_pos = found_idx - statistic.disp_begin_pos;
                        }
                    }
                    /* Sync */
                    dir_entry = dir_entry_list[statistic.disp_begin_pos + statistic.cursor_pos].dir_entry;

                    /* Refresh */
                    DisplayTree(dir_window, statistic.disp_begin_pos, statistic.disp_begin_pos + statistic.cursor_pos);
                    DisplayFileWindow(dir_entry);
                    DisplayDiskStatistic();
                    DisplayDirStatistic(dir_entry, NULL); /* Updated call */
                    DisplayAvailBytes();

                    char path[PATH_LENGTH];
                    GetPath(dir_entry, path);
                    DisplayHeaderPath(path);
                }
            }
            need_dsp_help = TRUE;
            break;
      case ACTION_PAGE_DOWN: Movenpage(&statistic.disp_begin_pos, &statistic.cursor_pos, &dir_entry);
                     break;
      case ACTION_PAGE_UP: Moveppage(&statistic.disp_begin_pos, &statistic.cursor_pos, &dir_entry);
                     break;
      case ACTION_HOME: MoveHome(&dir_entry);
                     break;
      case ACTION_END: MoveEnd(&dir_entry);
    		     break;
      case ACTION_MOVE_RIGHT:
      case ACTION_TREE_EXPAND_ALL: HandlePlus(dir_entry, de_ptr, new_login_path,
    				start_dir_entry, &need_dsp_help);
	             break;
      case ACTION_TREE_EXPAND: HandleReadSubTree(dir_entry, start_dir_entry,
    					&need_dsp_help);
    		     break;
      case ACTION_MOVE_LEFT:
          /* Check if directory is expanded (has sub-tree and is scanned) */
          if (!dir_entry->not_scanned && dir_entry->sub_tree != NULL) {
              /* It is expanded -> Collapse it */
              HandleUnreadSubTree(dir_entry, de_ptr, start_dir_entry, &need_dsp_help);
          } else {
              /* It is collapsed (or leaf) -> Jump to Parent */
              if (dir_entry->up_tree != NULL) {
                   /* Find parent in the list */
                   int p_idx = -1;
                   int k;
                   for(k=0; k < total_dirs; k++) {
                       if(dir_entry_list[k].dir_entry == dir_entry->up_tree) {
                           p_idx = k;
                           break;
                       }
                   }
                   if (p_idx != -1) {
                       /* Move cursor to parent */
                       if (p_idx >= statistic.disp_begin_pos &&
                           p_idx < statistic.disp_begin_pos + window_height) {
                           /* Parent is on screen */
                           statistic.cursor_pos = p_idx - statistic.disp_begin_pos;
                       } else {
                           /* Parent is off screen - center it or put at top */
                           statistic.disp_begin_pos = p_idx;
                           statistic.cursor_pos = 0;
                           /* Adjust if near end */
                           if (statistic.disp_begin_pos + window_height > total_dirs) {
                               statistic.disp_begin_pos = MAXIMUM(0, total_dirs - window_height);
                               statistic.cursor_pos = p_idx - statistic.disp_begin_pos;
                           }
                       }
                       /* Sync pointers */
                       dir_entry = dir_entry_list[statistic.disp_begin_pos + statistic.cursor_pos].dir_entry;

                       /* Refresh */
                       DisplayTree(dir_window, statistic.disp_begin_pos, statistic.disp_begin_pos + statistic.cursor_pos);
                       DisplayFileWindow(dir_entry);
                       DisplayDiskStatistic();
                       DisplayDirStatistic(dir_entry, NULL); /* Updated call */
                       DisplayAvailBytes();
                       /* Update Header Path */
                       char path[PATH_LENGTH];
                       GetPath(dir_entry, path);
                       DisplayHeaderPath(path);
                   }
              }
          }
          break;
      case ACTION_TREE_COLLAPSE: HandleUnreadSubTree(dir_entry, de_ptr, start_dir_entry,
    					 &need_dsp_help);
	             break;
      case ACTION_TOGGLE_HIDDEN: {
                        ToggleDotFiles();

                        /* Update current dir pointer using the new accessor function
                           because ToggleDotFiles might have changed the list layout */
                        if (total_dirs > 0) {
                            dir_entry = dir_entry_list[statistic.disp_begin_pos + statistic.cursor_pos].dir_entry;
                        } else {
                            dir_entry = statistic.tree;
                        }

                        need_dsp_help = TRUE;
                     }
		     break;
      case ACTION_FILTER: if(ReadFilter() == 0) {
		       dir_entry->start_file = 0;
		       dir_entry->cursor_pos = -1;
                       DisplayFileWindow( dir_entry );
                       RefreshWindow( file_window );
		       DisplayDiskStatistic();
		       DisplayDirStatistic(dir_entry, NULL); /* Updated call */
		     }
		     need_dsp_help = TRUE;
                     break;
      case ACTION_TAG: HandleTagDir(dir_entry, TRUE);
             Movedown(&statistic.disp_begin_pos, &statistic.cursor_pos, &dir_entry); /* ADDED */
    		     break;

      case ACTION_UNTAG: HandleTagDir(dir_entry, FALSE);
             Movedown(&statistic.disp_begin_pos, &statistic.cursor_pos, &dir_entry); /* ADDED */
    		     break;
      case ACTION_TAG_ALL:
                     HandleTagAllDirs(dir_entry,TRUE);
		     break;
      case ACTION_UNTAG_ALL:
    		     HandleTagAllDirs(dir_entry,FALSE);
		     break;
      case ACTION_TOGGLE_MODE:
		     RotateDirMode();
                     /*DisplayFileWindow( dir_entry, 0, -1 );*/
                     DisplayTree( dir_window, statistic.disp_begin_pos,
                                  statistic.disp_begin_pos + statistic.cursor_pos
                                  );
                     /*RefreshWindow( file_window );*/
		     DisplayDiskStatistic();
		     DisplayDirStatistic(dir_entry, NULL); /* Updated call */
		     need_dsp_help = TRUE;
		     break;
      case ACTION_CMD_TAGGED_S:
		     HandleShowAll(TRUE, dir_entry, start_dir_entry, &need_dsp_help, &ch);
		     break;

      case ACTION_CMD_S:
		     HandleShowAll(FALSE, dir_entry, start_dir_entry, &need_dsp_help, &ch);
		     break;
      case ACTION_ENTER: HandleSwitchWindow(dir_entry, start_dir_entry, &need_dsp_help, &ch);
		     break;
      case ACTION_CMD_X: if (mode != DISK_MODE && mode != USER_MODE) {
		     } else {
			 (void) Execute( dir_entry, NULL );
		     }
		     need_dsp_help = TRUE;
		     DisplayAvailBytes();
                     DisplayDiskStatistic();
                     DisplayDirStatistic(dir_entry, NULL); /* Updated call */
		     break;
      case ACTION_CMD_M: if( !MakeDirectory( dir_entry ) )
		     {
		       BuildDirEntryList( start_dir_entry, &statistic );
                       DisplayTree( dir_window, statistic.disp_begin_pos,
				    statistic.disp_begin_pos + statistic.cursor_pos
				  );
		       DisplayAvailBytes();
                       DisplayDiskStatistic();
                       DisplayDirStatistic(dir_entry, NULL); /* Updated call */
		     }
		     need_dsp_help = TRUE;
		     break;
      case ACTION_CMD_D: if( !DeleteDirectory( dir_entry ) ) {
		       if( statistic.disp_begin_pos + statistic.cursor_pos > 0 )
		       {
		         if( statistic.cursor_pos > 0 ) statistic.cursor_pos--;
		         else statistic.disp_begin_pos--;
		       }
      		     }
		     /* Update regardless of success */
		     BuildDirEntryList( start_dir_entry, &statistic );
		     dir_entry = dir_entry_list[statistic.disp_begin_pos + statistic.cursor_pos].dir_entry;
		     dir_entry->start_file = 0;
		     dir_entry->cursor_pos = -1;
                     DisplayFileWindow( dir_entry );
                     RefreshWindow( file_window );
		     DisplayTree( dir_window, statistic.disp_begin_pos,
				  statistic.disp_begin_pos + statistic.cursor_pos
				);
		     DisplayAvailBytes();
                     DisplayDiskStatistic();
                     DisplayDirStatistic(dir_entry, NULL); /* Updated call */
		     need_dsp_help = TRUE;
		     break;
      case ACTION_CMD_R: if( !GetRenameParameter( dir_entry->name, new_name ) )
                     {
		       if( !RenameDirectory( dir_entry, new_name ) )
		       {
		         /* Rename OK */
		         /*-----------*/
		         BuildDirEntryList( start_dir_entry, &statistic );
                         DisplayTree( dir_window, statistic.disp_begin_pos,
		                      statistic.disp_begin_pos + statistic.cursor_pos
			            );
		         DisplayAvailBytes();
		         dir_entry = dir_entry_list[statistic.disp_begin_pos + statistic.cursor_pos].dir_entry;
                         DisplayDiskStatistic();
                         DisplayDirStatistic(dir_entry, NULL); /* Updated call */
		       }
		     }
		     need_dsp_help = TRUE;
		     break;
      case ACTION_REFRESH: /* Rescan */
             RescanDir(dir_entry, strtol(TREEDEPTH, NULL, 0)); /* Fixed: Use configured TREEDEPTH */
             BuildDirEntryList(start_dir_entry, &statistic);
             if (total_dirs > 0 && (statistic.disp_begin_pos + statistic.cursor_pos >= total_dirs)) {
                 statistic.disp_begin_pos = 0;
                 statistic.cursor_pos = 0;
             }
             if (total_dirs > 0) {
                dir_entry = dir_entry_list[statistic.disp_begin_pos + statistic.cursor_pos].dir_entry;
             }
             DisplayTree(dir_window, statistic.disp_begin_pos, statistic.disp_begin_pos + statistic.cursor_pos);
             DisplayFileWindow(dir_entry);
             DisplayDiskStatistic();
             DisplayDirStatistic(dir_entry, NULL); /* Updated call */
             need_dsp_help = TRUE;
             break;
      case ACTION_CMD_G: (void) ChangeDirGroup( dir_entry );
                     DisplayTree( dir_window, statistic.disp_begin_pos, statistic.disp_begin_pos + statistic.cursor_pos );
                     DisplayDiskStatistic();
                     DisplayDirStatistic(dir_entry, NULL); /* Updated call */
		     need_dsp_help = TRUE;
		     break;
      case ACTION_CMD_O: (void) ChangeDirOwner( dir_entry );
                     DisplayTree( dir_window, statistic.disp_begin_pos, statistic.disp_begin_pos + statistic.cursor_pos );
                     DisplayDiskStatistic();
                     DisplayDirStatistic(dir_entry, NULL); /* Updated call */
		     need_dsp_help = TRUE;
		     break;
      case ACTION_CMD_A: (void) ChangeDirModus( dir_entry );
                     DisplayTree( dir_window, statistic.disp_begin_pos, statistic.disp_begin_pos + statistic.cursor_pos );
                     DisplayDiskStatistic();
                     DisplayDirStatistic(dir_entry, NULL); /* Updated call */
		     need_dsp_help = TRUE;
		     break;

      case ACTION_CMD_B: /* About Box */
                     AboutBox();
                     need_dsp_help = TRUE;
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
                  start_dir_entry = statistic.tree; /* CRITICAL: Update local pointer to new volume's tree root */
                  BuildDirEntryList(start_dir_entry, &statistic); /* Rebuild list for the new tree */
                  /* PRESERVE STATE: Remove lines resetting cursor_pos and disp_begin_pos.
                   * These are now loaded from the Volume struct by LoginDisk. */
                  /* Safety check for cursor validity after list rebuild */
                  if (total_dirs > 0 && (statistic.disp_begin_pos + statistic.cursor_pos >= total_dirs)) {
                      statistic.disp_begin_pos = 0;
                      statistic.cursor_pos = 0;
                  }
                  /* Update local dir_entry pointer to the currently selected entry of the new list */
                  if (total_dirs > 0) {
                      dir_entry = dir_entry_list[statistic.disp_begin_pos + statistic.cursor_pos].dir_entry;
                  } else {
                      dir_entry = statistic.tree; /* Fallback to root if list is empty */
                  }
                  DisplayTree(dir_window, statistic.disp_begin_pos, statistic.disp_begin_pos + statistic.cursor_pos);
                  DisplayFileWindow(dir_entry); /* Refresh file window for the new directory */
                  RefreshWindow(file_window);
                  DisplayDiskStatistic();
                  DisplayDirStatistic(dir_entry, NULL); /* Updated call */
                  DisplayAvailBytes();
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
                  start_dir_entry = statistic.tree; /* CRITICAL: Update local pointer to new volume's tree root */
                  BuildDirEntryList(start_dir_entry, &statistic); /* Rebuild list for the new tree */
                  /* PRESERVE STATE: Remove lines resetting cursor_pos and disp_begin_pos.
                   * These are now loaded from the Volume struct by LoginDisk. */
                  /* Safety check for cursor validity after list rebuild */
                  if (total_dirs > 0 && (statistic.disp_begin_pos + statistic.cursor_pos >= total_dirs)) {
                      statistic.disp_begin_pos = 0;
                      statistic.cursor_pos = 0;
                  }
                  /* Update local dir_entry pointer to the currently selected entry of the new list */
                  if (total_dirs > 0) {
                      dir_entry = dir_entry_list[statistic.disp_begin_pos + statistic.cursor_pos].dir_entry;
                  } else {
                      dir_entry = statistic.tree; /* Fallback to root if list is empty */
                  }
                  DisplayTree(dir_window, statistic.disp_begin_pos, statistic.disp_begin_pos + statistic.cursor_pos);
                  DisplayFileWindow(dir_entry); /* Refresh file window for the new directory */
                  RefreshWindow(file_window);
                  DisplayDiskStatistic();
                  DisplayDirStatistic(dir_entry, NULL); /* Updated call */
                  DisplayAvailBytes();
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
                  start_dir_entry = statistic.tree; /* CRITICAL: Update local pointer to new volume's tree root */
                  BuildDirEntryList(start_dir_entry, &statistic); /* Rebuild list for the new tree */
                  /* PRESERVE STATE: Remove lines resetting cursor_pos and disp_begin_pos.
                   * These are now loaded from the Volume struct by LoginDisk. */
                  /* Safety check for cursor validity after list rebuild */
                  if (total_dirs > 0 && (statistic.disp_begin_pos + statistic.cursor_pos >= total_dirs)) {
                      statistic.disp_begin_pos = 0;
                      statistic.cursor_pos = 0;
                  }
                  /* Update local dir_entry pointer to the currently selected entry of the new list */
                  if (total_dirs > 0) {
                      dir_entry = dir_entry_list[statistic.disp_begin_pos + statistic.cursor_pos].dir_entry;
                  } else {
                      dir_entry = statistic.tree; /* Fallback to root if list is empty */
                  }
                  DisplayTree(dir_window, statistic.disp_begin_pos, statistic.disp_begin_pos + statistic.cursor_pos);
                  DisplayFileWindow(dir_entry); /* Refresh file window for the new directory */
                  RefreshWindow(file_window);
                  DisplayDiskStatistic();
                  DisplayDirStatistic(dir_entry, NULL); /* Updated call */
                  DisplayAvailBytes();
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
                  /* LoginDisk has already built the list internally, but for safety against
                   * state reversion when logging sub-volumes, we force a complete refresh
                   * of local pointers and the directory list here. */
                  start_dir_entry = statistic.tree;

                  /* Rebuild the list based on the new volume's tree */
                  BuildDirEntryList(start_dir_entry, &statistic);

                  /* Ensure cursor is at the top for a new login */
                  statistic.disp_begin_pos = 0;
                  statistic.cursor_pos = 0;

                  /* Safety: Update dir_entry to the first item of the newly built list */
                  if (total_dirs > 0) {
                      dir_entry = dir_entry_list[0].dir_entry;
                  } else {
                      dir_entry = statistic.tree;
                  }

                  /* Force Full Display Refresh */
                  DisplayTree(dir_window, statistic.disp_begin_pos, statistic.disp_begin_pos + statistic.cursor_pos);
                  DisplayFileWindow(dir_entry);
                  RefreshWindow(file_window);
                  DisplayDiskStatistic();
                  DisplayDirStatistic(dir_entry, NULL); /* Updated call */
                  DisplayAvailBytes();
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
  } while( action != ACTION_QUIT && action != ACTION_ESCAPE && action != ACTION_LOGIN ); /* Loop until explicit quit, escape or login */
  return( ch ); /* Return the last raw character that caused exit */
}



int ScanSubTree( DirEntry *dir_entry )
{
  DirEntry *de_ptr;
  char new_login_path[PATH_LENGTH + 1];

  if( dir_entry->not_scanned ) {
    for( de_ptr=dir_entry->sub_tree; de_ptr; de_ptr=de_ptr->next) {
      GetPath( de_ptr, new_login_path );
      if (ReadTree( de_ptr, new_login_path, 999 ) == -1) {
          /* Abort signal received from ReadTree */
          return -1;
      }
      ApplyFilter( de_ptr );
    }
    dir_entry->not_scanned = FALSE;
  } else {
    for( de_ptr=dir_entry->sub_tree; de_ptr; de_ptr=de_ptr->next) {
      if (ScanSubTree( de_ptr ) == -1) {
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
  DirEntry *tree_source;
  Statistic *stat_source;
  int local_disp_begin_pos;
  int local_cursor_pos;
  YtreeAction action; /* Declare YtreeAction variable */

  if (mode != DISK_MODE && mode != USER_MODE) {
      /* Search for a volume that is in DISK_MODE */
      struct Volume *v, *tmp;
      struct Volume *disk_vol = NULL;

      HASH_ITER(hh, VolumeList, v, tmp) {
          if (v->vol_stats.mode == DISK_MODE) {
              disk_vol = v;
              break;
          }
      }

      if (!disk_vol) {
          MESSAGE("No filesystem volume active");
          return -1;
      }

      tree_source = disk_vol->vol_stats.tree;
      stat_source = &disk_vol->vol_stats;
      local_disp_begin_pos = disk_vol->vol_stats.disp_begin_pos;
      local_cursor_pos = disk_vol->vol_stats.cursor_pos;
  } else {
      tree_source = start_dir_entry;
      stat_source = &statistic;
      local_disp_begin_pos = disp_begin_pos;
      local_cursor_pos = cursor_pos;
  }

  BuildDirEntryList(tree_source, stat_source);

  /* Safety bounds check */
  if (local_disp_begin_pos < 0) local_disp_begin_pos = 0;
  if (local_cursor_pos < 0) local_cursor_pos = 0;
  if (total_dirs > 0 && (local_disp_begin_pos + local_cursor_pos >= total_dirs)) {
      local_disp_begin_pos = 0;
      local_cursor_pos = 0;
  }

  GetMaxYX(f2_window, &win_height, &win_width);
  MapF2Window();
  DisplayTree( f2_window, local_disp_begin_pos, local_disp_begin_pos + local_cursor_pos );
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
      case ACTION_MOVE_DOWN: if( local_disp_begin_pos + local_cursor_pos + 1 >= total_dirs )
		     {
		     }
		     else
		     {
		       if( local_cursor_pos + 1 < win_height )
		       {
			 PrintDirEntry( f2_window,
			                local_disp_begin_pos + local_cursor_pos,
					local_cursor_pos, FALSE);
			 local_cursor_pos++;

			 PrintDirEntry( f2_window, local_disp_begin_pos + local_cursor_pos,
					local_cursor_pos, TRUE);
                       }
		       else
		       {
			 local_disp_begin_pos++;
                         DisplayTree( f2_window, local_disp_begin_pos,
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
			 PrintDirEntry( f2_window,
			                local_disp_begin_pos + local_cursor_pos,
					local_cursor_pos, FALSE );
			 local_cursor_pos--;
			 PrintDirEntry( f2_window,
			                local_disp_begin_pos + local_cursor_pos,
					local_cursor_pos, TRUE );
                       }
		       else
		       {
			 local_disp_begin_pos--;
                         DisplayTree( f2_window, local_disp_begin_pos,
				      local_disp_begin_pos + local_cursor_pos);
                       }
		     }
                     break;

      case ACTION_PAGE_DOWN:
		     if( local_disp_begin_pos + local_cursor_pos >= total_dirs - 1 )
		     {
		     }
		     else
		     {
		       if( local_cursor_pos < win_height - 1 )
		       {
			 PrintDirEntry( f2_window,
			                local_disp_begin_pos + local_cursor_pos,
					local_cursor_pos, FALSE );
		         if( local_disp_begin_pos + win_height > total_dirs  - 1 )
			   local_cursor_pos = total_dirs - local_disp_begin_pos - 1;
			 else
			   local_cursor_pos = win_height - 1;
			 PrintDirEntry( f2_window,
			                local_disp_begin_pos + local_cursor_pos,
					local_cursor_pos, TRUE );
		       }
		       else
		       {
			 if( local_disp_begin_pos + local_cursor_pos + win_height < total_dirs )
			 {
			   local_disp_begin_pos += win_height;
			   local_cursor_pos = win_height - 1;
			 }
			 else
			 {
			   local_disp_begin_pos = total_dirs - win_height;
			   if( local_disp_begin_pos < 0 ) local_disp_begin_pos = 0;
			   local_cursor_pos = total_dirs - local_disp_begin_pos - 1;
			 }
                         DisplayTree( f2_window, local_disp_begin_pos,
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
			 PrintDirEntry( f2_window,
			                local_disp_begin_pos + local_cursor_pos,
					local_cursor_pos, FALSE );
			 local_cursor_pos = 0;
			 PrintDirEntry( f2_window,
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
                         DisplayTree( f2_window, local_disp_begin_pos,
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
                       DisplayTree( f2_window, local_disp_begin_pos,
				    local_disp_begin_pos + local_cursor_pos);
		     }
                     break;

      case ACTION_END: local_disp_begin_pos = MAXIMUM(0, total_dirs - win_height);
		     local_cursor_pos     = total_dirs - local_disp_begin_pos - 1;
                     DisplayTree( f2_window, local_disp_begin_pos,
				  local_disp_begin_pos + local_cursor_pos);
                     break;

      case ACTION_ENTER:
                     GetPath(dir_entry_list[local_cursor_pos + local_disp_begin_pos].dir_entry, path);
		     result = 0;
		     break;
      case ACTION_QUIT:      break;
      case ACTION_ESCAPE:    break;

      default :      beep(); break;
    } /* switch */
  } while( action != ACTION_QUIT && action != ACTION_ENTER && action != ACTION_ESCAPE );

  UnmapF2Window();

  /* Restore the original directory list for the main window */
  BuildDirEntryList(start_dir_entry, &statistic);

  if (action == ACTION_ESCAPE || action == ACTION_QUIT) return -1;

  return( result );
}



int RefreshDirWindow()
{
	DirEntry *de_ptr;
	int i, n;
	int result = -1;
	int window_width, window_height;

	de_ptr = dir_entry_list[statistic.disp_begin_pos + statistic.cursor_pos].dir_entry;
	BuildDirEntryList( dir_entry_list[0].dir_entry, &statistic );

	/* Search old entry */
	for(n=-1, i=0;i < total_dirs; i++) {
		if(dir_entry_list[i].dir_entry == de_ptr) {
			n = i;
			break;
		}
	}

	if(n == -1) {
		/* Directory disapeared */
      		ERROR_MSG( "Current directory disappeared" );
		result = -1;
	} else {

		if( n != (statistic.disp_begin_pos + statistic.cursor_pos)) {
			/* Position changed */
			if((n - statistic.disp_begin_pos) >= 0) {
				statistic.cursor_pos = n - statistic.disp_begin_pos;
			} else {
				statistic.disp_begin_pos = n;
				statistic.cursor_pos = 0;
			}
		}

       		GetMaxYX(dir_window, &window_height, &window_width);
	        while(statistic.cursor_pos >= window_height) {
		  statistic.cursor_pos--;
		  statistic.disp_begin_pos++;
	        }
		DisplayTree( dir_window, statistic.disp_begin_pos,
		     	statistic.disp_begin_pos + statistic.cursor_pos
		   	);

		DisplayAvailBytes();
		DisplayDiskStatistic();
		DisplayDirStatistic(dir_entry_list[statistic.disp_begin_pos + statistic.cursor_pos].dir_entry, NULL); /* Updated call */
        /* Update header path after refresh */
        {
            char path[PATH_LENGTH];
            GetPath(dir_entry_list[statistic.disp_begin_pos + statistic.cursor_pos].dir_entry, path);
            DisplayHeaderPath(path);
        }
		result = 0;
	}

	return(result);
}