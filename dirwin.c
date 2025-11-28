/***************************************************************************
 *
 * dirwin.c
 * Funktionen zur Handhabung des DIR-Windows
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
static void HandleReadSubTree(DirEntry *dir_entry, DirEntry *start_dir_entry, BOOL *need_dsp_help);
static void HandleUnreadSubTree(DirEntry *dir_entry, DirEntry *de_ptr, DirEntry *start_dir_entry, BOOL *need_dsp_help);
static void MoveEnd(DirEntry **dir_entry);
static void MoveHome(DirEntry **dir_entry);
static void HandlePlus(DirEntry *dir_entry, DirEntry *de_ptr, char *new_login_path, DirEntry *start_dir_entry, BOOL *need_dsp_help);
static void HandleTagDir(DirEntry *dir_entry, BOOL value);
static void HandleTagAllDirs(DirEntry *dir_entry, BOOL value );
static void HandleShowAll(BOOL tagged_only, DirEntry *dir_entry, DirEntry *start_dir_entry, BOOL *need_dsp_help, int *ch);
static void HandleSwitchWindow(DirEntry *dir_entry, DirEntry *start_dir_entry, BOOL *need_dsp_help, int *ch);

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

static void RotateDirMode(void)
{
  switch( dir_mode )
  {
    case MODE_1: dir_mode = MODE_2 ; break;
    case MODE_2: dir_mode = MODE_4 ; break;
    case MODE_3: dir_mode = MODE_1 ; break;
    case MODE_4: dir_mode = MODE_3 ; break;
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
  char access_time[20]; /* Increased from 13 to 20 */
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

		 (void) sprintf( line_buffer, format, attributes,
                                 (int)de_ptr->stat_struct.st_nlink,
                                 (LONGLONG) de_ptr->stat_struct.st_size,
                                 modify_time
                                 );
#else
                 (void) strcpy( format, "%10s %3d %8d %16s");
                 (void) sprintf( line_buffer, format, attributes,
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
                    (void)sprintf(owner,"%d",(int)de_ptr->stat_struct.st_uid);
                    owner_name_ptr = owner;
                 }
                 if( group_name_ptr == NULL )
                 {
                     (void) sprintf( group, "%d", (int) de_ptr->stat_struct.st_gid );
                     group_name_ptr = group;
                 }
                 if ((line_buffer = (char *) malloc(40)) == NULL)
                 {
                    ERROR_MSG("malloc() Failed*Abort");
                    exit(1);
                 }
                 (void) strcpy( format, "%12u  %-12s %-12s");
                 (void) sprintf( line_buffer, format,
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
                 (void)sprintf(line_buffer, format, change_time, access_time);
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

  /* Truncate name if the graph + name would overlap with the attribute column */
  if (dir_mode != MODE_3 && (graph_len + (int)strlen(name_buffer) >= attr_start_col)) {
      char temp_name[PATH_LENGTH + 2];
      int available_name_width = attr_start_col - graph_len - 1;
      CutName(temp_name, name_buffer, available_name_width);
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
      int current_x, current_y;
      getyx(win, current_y, current_x);
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
    if( start_entry_no + i >= total_dirs ) break;

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
      /* Element nicht vorhanden */
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
}


static void Moveup(int *disp_begin_pos, int *cursor_pos, DirEntry **dir_entry)
{
   if( *disp_begin_pos + *cursor_pos - 1 < 0 )
   {
      /* Element nicht vorhanden */
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
}


static void Movenpage(int *disp_begin_pos, int *cursor_pos, DirEntry **dir_entry)
{
   if( *disp_begin_pos + *cursor_pos >= total_dirs - 1 )
   {
      /* Letzte Position */
      /*-----------------*/
   }
   else
   {
      if( *cursor_pos < window_height - 1 )
      {
          /* Cursor steht nicht auf letztem Eintrag
           * der Seite
           */
           if( *disp_begin_pos + window_height > total_dirs  - 1 )
              *cursor_pos = total_dirs - *disp_begin_pos - 1;
           else
              *cursor_pos = window_height - 1;
      }
      else
      {
          /* Scrollen */
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
   }
   return;
}

static void Moveppage(int *disp_begin_pos, int *cursor_pos, DirEntry **dir_entry)
{
   if( *disp_begin_pos + *cursor_pos <= 0 )
   {
      /* Erste Position */
      /*----------------*/
   }
   else
   {
      if( *cursor_pos > 0 )
      {
          /* Cursor steht nicht auf erstem Eintrag
           * der Seite
           */
           *cursor_pos = 0;
      }
      else
      {
         /* Scrollen */
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
    return;
}

static void MoveHome(DirEntry **dir_entry)
{
    if( statistic.disp_begin_pos == 0 && statistic.cursor_pos == 0 )
    {  /* Position 1 bereits errecht */
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
	for( de_ptr=dir_entry->sub_tree; de_ptr; de_ptr=de_ptr->next) {
	    GetPath( de_ptr, new_login_path );
	    ReadTree( de_ptr, new_login_path, 0 );
	    ApplyFilter( de_ptr );
	}
	dir_entry->not_scanned = FALSE;
	BuildDirEntryList( start_dir_entry, &statistic );
	DisplayTree( dir_window, statistic.disp_begin_pos,
                 statistic.disp_begin_pos + statistic.cursor_pos );
	DisplayDiskStatistic();
	DisplayAvailBytes();
	*need_dsp_help = TRUE;
    }
}

static void HandleReadSubTree(DirEntry *dir_entry, DirEntry *start_dir_entry,
		       BOOL *need_dsp_help)
{
    ScanSubTree( dir_entry );
    BuildDirEntryList( start_dir_entry, &statistic );
    DisplayTree( dir_window, statistic.disp_begin_pos,
		 statistic.disp_begin_pos + statistic.cursor_pos );
    DisplayDiskStatistic();
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
        *need_dsp_help = TRUE;
    }
    return;
}

static void HandleTagDir(DirEntry *dir_entry, BOOL value)
{
    FileEntry *fe_ptr;
    for(fe_ptr=dir_entry->file; fe_ptr; fe_ptr=fe_ptr->next)
    {
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

    InitClock(); /* Resume clock and restore signal handling */
}


int HandleDirWindow(DirEntry *start_dir_entry)
{
  DirEntry  *dir_entry, *de_ptr;
  int i, ch, unput_char;
  BOOL need_dsp_help;
  char new_name[PATH_LENGTH + 1];
  char new_login_path[PATH_LENGTH + 1];
  char *home, *p;

  unput_char = 0;
  de_ptr = NULL;

  GetMaxYX(dir_window, &window_height, &window_width);

  /* Merker loeschen */
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
    if( need_dsp_help )
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
      if( ch == LF ) ch = CR;
    }

    if (mode == USER_MODE) { /* User commands take precedence */
        ch = DirUserMode(dir_entry, ch);
    }

#ifdef VI_KEYS
    ch = ViKey( ch );
#endif /* VI_KEYS */


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
       DisplayDirParameter( dir_entry );
       need_dsp_help = TRUE;
       DisplayAvailBytes();
       DisplayFilter();
       DisplayDiskName();
       resize_request = FALSE;
    }


    switch( ch )
    {

#ifdef KEY_RESIZE
      case KEY_RESIZE: resize_request = TRUE;
      		       break;
#endif

      case -1:        break;

      case ' ':      /*break;   Quick-Key */
      case KEY_DOWN: Movedown(&statistic.disp_begin_pos, &statistic.cursor_pos, &dir_entry);
                     break;
      case KEY_UP  : Moveup(&statistic.disp_begin_pos, &statistic.cursor_pos, &dir_entry);
                     break;
      case KEY_NPAGE:Movenpage(&statistic.disp_begin_pos, &statistic.cursor_pos, &dir_entry);
                     break;
      case KEY_PPAGE:Moveppage(&statistic.disp_begin_pos, &statistic.cursor_pos, &dir_entry);
                     break;
      case KEY_HOME: MoveHome(&dir_entry);
                     break;
      case KEY_END : MoveEnd(&dir_entry);
    		     break;
      case KEY_RIGHT:
      case '+':      HandlePlus(dir_entry, de_ptr, new_login_path,
    				start_dir_entry, &need_dsp_help);
	             break;
      case '\t':
      case '*':      HandleReadSubTree(dir_entry, start_dir_entry,
    					&need_dsp_help);
    		     break;
      case KEY_LEFT:
      case '-':
      case KEY_BTAB: HandleUnreadSubTree(dir_entry, de_ptr, start_dir_entry,
    					 &need_dsp_help);
	             break;
      case '`':      {
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
      case 'F':
      case 'f':      if(ReadFilter() == 0) {
		       dir_entry->start_file = 0;
		       dir_entry->cursor_pos = -1;
                       DisplayFileWindow( dir_entry );
                       RefreshWindow( file_window );
		       DisplayDiskStatistic();
		     }
		     need_dsp_help = TRUE;
                     break;
      case 'T' :
      case 't' :     HandleTagDir(dir_entry, TRUE);
    		     break;

      case 'U' :
      case 'u' :     HandleTagDir(dir_entry, FALSE);
    		     break;
      case 'T' & 0x1F :
                     HandleTagAllDirs(dir_entry,TRUE);
		     break;
      case 'U' & 0x1F :
    		     HandleTagAllDirs(dir_entry,FALSE);
		     break;
      case 'F' & 0x1F :
		     RotateDirMode();
                     /*DisplayFileWindow( dir_entry, 0, -1 );*/
                     DisplayTree( dir_window, statistic.disp_begin_pos,
                                  statistic.disp_begin_pos + statistic.cursor_pos
                                  );
                     /*RefreshWindow( file_window );*/
		     DisplayDiskStatistic();
		     need_dsp_help = TRUE;
		     break;
      case 'S' & 0x1F :
		     HandleShowAll(TRUE, dir_entry, start_dir_entry, &need_dsp_help, &ch);
		     break;

      case 'S':
      case 's':      HandleShowAll(FALSE, dir_entry, start_dir_entry, &need_dsp_help, &ch);
		     break;
      case LF :
      case CR :      HandleSwitchWindow(dir_entry, start_dir_entry, &need_dsp_help, &ch);
		     break;
      case 'X':
      case 'x':      if (mode != DISK_MODE && mode != USER_MODE) {
		     } else {
			 (void) Execute( dir_entry, NULL );
		     }
		     need_dsp_help = TRUE;
		     DisplayAvailBytes();
		     break;
      case 'M':
      case 'm':      if( !MakeDirectory( dir_entry ) )
		     {
		       BuildDirEntryList( start_dir_entry, &statistic );
                       DisplayTree( dir_window, statistic.disp_begin_pos,
				    statistic.disp_begin_pos + statistic.cursor_pos
				  );
		       DisplayAvailBytes();
		     }
		     need_dsp_help = TRUE;
		     break;
      case 'D':
      case 'd':      if( !DeleteDirectory( dir_entry ) ) {
		       if( statistic.disp_begin_pos + statistic.cursor_pos > 0 )
		       {
		         if( statistic.cursor_pos > 0 ) statistic.cursor_pos--;
		         else statistic.disp_begin_pos--;
		       }
      		     }
		     /* Unabhaengig vom Erfolg aktualisieren */
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
		     need_dsp_help = TRUE;
		     break;
      case 'r':
      case 'R':      if( !GetRenameParameter( dir_entry->name, new_name ) )
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
		       }
		     }
		     need_dsp_help = TRUE;
		     break;
      case 'R' & 0x1F: /* Rescan */
             RescanDir(dir_entry, 0);
             BuildDirEntryList(start_dir_entry, &statistic);
             if (statistic.disp_begin_pos + statistic.cursor_pos >= total_dirs) {
                 statistic.disp_begin_pos = 0;
                 statistic.cursor_pos = 0;
             }
             if (total_dirs > 0) {
                dir_entry = dir_entry_list[statistic.disp_begin_pos + statistic.cursor_pos].dir_entry;
             }
             DisplayTree(dir_window, statistic.disp_begin_pos, statistic.disp_begin_pos + statistic.cursor_pos);
             DisplayFileWindow(dir_entry);
             DisplayDiskStatistic();
             need_dsp_help = TRUE;
             break;
      case 'G':
      case 'g':      (void) ChangeDirGroup( dir_entry );
                     DisplayTree( dir_window, statistic.disp_begin_pos, statistic.disp_begin_pos + statistic.cursor_pos );
		     need_dsp_help = TRUE;
		     break;
      case 'O':
      case 'o':      (void) ChangeDirOwner( dir_entry );
                     DisplayTree( dir_window, statistic.disp_begin_pos, statistic.disp_begin_pos + statistic.cursor_pos );
		     need_dsp_help = TRUE;
		     break;
      case 'A':
      case 'a':      (void) ChangeDirModus( dir_entry );
                     DisplayTree( dir_window, statistic.disp_begin_pos, statistic.disp_begin_pos + statistic.cursor_pos );
		     need_dsp_help = TRUE;
		     break;

      case 'B':
      case 'b':      /* About Box */
                     AboutBox();
                     need_dsp_help = TRUE;
                     break;

      /* Volume Cycling and Selection */
      case 'K': /* Shift-K: Select Loaded Volume */
          {
              int res = SelectLoadedVolume();
              if (res == 0) { /* If volume switch was successful */
                  start_dir_entry = statistic.tree; /* CRITICAL: Update local pointer to new volume's tree root */
                  BuildDirEntryList(start_dir_entry, &statistic); /* Rebuild list for the new tree */
                  /* PRESERVE STATE: Remove lines resetting cursor_pos and disp_begin_pos.
                   * These are now loaded from the Volume struct by LoginDisk. */
                  /* Safety check for cursor validity after list rebuild */
                  if (statistic.disp_begin_pos + statistic.cursor_pos >= total_dirs) {
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
                  DisplayAvailBytes();
                  need_dsp_help = TRUE;
              }
          }
          break;

      case ',': /* Previous Volume */
      case '<':
          {
              int res = CycleLoadedVolume(-1);
              if (res == 0) { /* If volume switch was successful */
                  start_dir_entry = statistic.tree; /* CRITICAL: Update local pointer to new volume's tree root */
                  BuildDirEntryList(start_dir_entry, &statistic); /* Rebuild list for the new tree */
                  /* PRESERVE STATE: Remove lines resetting cursor_pos and disp_begin_pos.
                   * These are now loaded from the Volume struct by LoginDisk. */
                  /* Safety check for cursor validity after list rebuild */
                  if (statistic.disp_begin_pos + statistic.cursor_pos >= total_dirs) {
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
                  DisplayAvailBytes();
                  need_dsp_help = TRUE;
              }
          }
          break;

      case '.': /* Next Volume */
      case '>':
          {
              int res = CycleLoadedVolume(1);
              if (res == 0) { /* If volume switch was successful */
                  start_dir_entry = statistic.tree; /* CRITICAL: Update local pointer to new volume's tree root */
                  BuildDirEntryList(start_dir_entry, &statistic); /* Rebuild list for the new tree */
                  /* PRESERVE STATE: Remove lines resetting cursor_pos and disp_begin_pos.
                   * These are now loaded from the Volume struct by LoginDisk. */
                  /* Safety check for cursor validity after list rebuild */
                  if (statistic.disp_begin_pos + statistic.cursor_pos >= total_dirs) {
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
                  DisplayAvailBytes();
                  need_dsp_help = TRUE;
              }
          }
          break;

      case 'Q' & 0x1F:
                     need_dsp_help = TRUE;
                     QuitTo(dir_entry);
                     break;

      case 'Q':
      case 'q':      need_dsp_help = TRUE;
                     break;

#ifndef VI_KEYS
      case 'l':
#endif /* VI_KEYS */
      case 'L':      if( mode != DISK_MODE && mode != USER_MODE ) {
			 if (getcwd(new_login_path, sizeof(new_login_path)) == NULL) {
			     strcpy(new_login_path, ".");
			 }
		     } else {
		       (void) GetPath( dir_entry, new_login_path );
		     }
		     if( !GetNewLoginPath( new_login_path ) )
		     {
		       DisplayMenu();
		       doupdate();
		       (void) LoginDisk( new_login_path );
		     }
		     need_dsp_help = TRUE;
		     break;
      case 'L' & 0x1F:
		     clearok( stdscr, TRUE );
		     break;
      default :      break;
    } /* switch */
  } while( (ch != 'q') && (ch != 'Q') && (ch != 'l') && (ch != 'L') );
  return( ch );
}



int ScanSubTree( DirEntry *dir_entry )
{
  DirEntry *de_ptr;
  char new_login_path[PATH_LENGTH + 1];

  if( dir_entry->not_scanned ) {
    for( de_ptr=dir_entry->sub_tree; de_ptr; de_ptr=de_ptr->next) {
      GetPath( de_ptr, new_login_path );
      ReadTree( de_ptr, new_login_path, 999 );
      ApplyFilter( de_ptr );
    }
    dir_entry->not_scanned = FALSE;
  } else {
    for( de_ptr=dir_entry->sub_tree; de_ptr; de_ptr=de_ptr->next) {
      ScanSubTree( de_ptr );
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

  if (mode != DISK_MODE && mode != USER_MODE) {
      /* When in an archive, F2 should show the filesystem tree */
      if (disk_statistic.login_path[0] == '\0') {
          MESSAGE("No filesystem context available.*Log into a directory first.");
          return -1;
      }
      tree_source = disk_statistic.tree;
      stat_source = &disk_statistic;
      /* Use the saved filesystem cursor positions, not the archive's */
      local_disp_begin_pos = disk_statistic.disp_begin_pos;
      local_cursor_pos = disk_statistic.cursor_pos;
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
    if( ch == LF ) ch = CR;


#ifdef VI_KEYS

    ch = ViKey( ch );

#endif /* VI_KEYS */

    switch( ch )
    {
      case -1:       break;
      case ' ':      break;  /* Quick-Key */
      case KEY_DOWN: if( local_disp_begin_pos + local_cursor_pos + 1 >= total_dirs )
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

      case KEY_UP  : if( local_disp_begin_pos + local_cursor_pos - 1 < 0 )
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

      case KEY_NPAGE:
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

      case KEY_PPAGE:
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

      case KEY_HOME: if( local_disp_begin_pos == 0 && local_cursor_pos == 0 )
		     { }
		     else
		     {
		       local_disp_begin_pos = 0;
		       local_cursor_pos     = 0;
                       DisplayTree( f2_window, local_disp_begin_pos,
				    local_disp_begin_pos + local_cursor_pos);
		     }
                     break;

      case KEY_END : local_disp_begin_pos = MAXIMUM(0, total_dirs - win_height);
		     local_cursor_pos     = total_dirs - local_disp_begin_pos - 1;
                     DisplayTree( f2_window, local_disp_begin_pos,
				  local_disp_begin_pos + local_cursor_pos);
                     break;

      case LF :
      case CR :
                     GetPath(dir_entry_list[local_cursor_pos + local_disp_begin_pos].dir_entry, path);
		     result = 0;
		     break;
      case ESC:
      case 'Q':
      case 'q':      break;

      default :      break;
    } /* switch */
  } while( (ch != 'q') && (ch != ESC) && (ch != 'Q') && (ch != CR) && (ch != -1) );

  UnmapF2Window();

  /* Restore the original directory list for the main window */
  BuildDirEntryList(start_dir_entry, &statistic);

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
		result = 0;
	}

	return(result);
}