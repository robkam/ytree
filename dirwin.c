/***************************************************************************
 *
 * Funktionen zur Handhabung des DIR-Windows
 *
 ***************************************************************************/


#include "ytree.h"


static DirEntryList *dir_entry_list;
static int current_dir_entry;
static int total_dirs;
static int window_height, window_width;

static void ReadDirList(DirEntry *dir_entry);
static void PrintDirEntry(WINDOW *win, int entry_no, int y, unsigned char hilight);
static void BuildDirEntryList(DirEntry *dir_entry, Statistic *stat_source);
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


static void BuildDirEntryList(DirEntry *dir_entry, Statistic *stat_source)
{
  if( dir_entry_list ) 
  {
    free( dir_entry_list );
    dir_entry_list = NULL;
  }

  /* fuer !ANSI-Systeme.. */
  /*----------------------*/
  
  if( stat_source->disk_total_directories == 0 )
  {
    dir_entry_list = NULL;
    total_dirs = 0;
  }
  else
  {
    if( ( dir_entry_list = ( DirEntryList *) 
		             calloc( stat_source->disk_total_directories,
	        	     sizeof( DirEntryList )
		           ) ) == NULL )
    {
      ERROR_MSG( "Calloc Failed*ABORT" );
      exit( 1 );
    }
    
    current_dir_entry = 0;

    ReadDirList( dir_entry );

    total_dirs = current_dir_entry;

#ifdef DEBUG
    if(stat_source->disk_total_directories != total_dirs)
    {
        /* This can happen if a directory becomes unreadable between scans */
        /* It is not a fatal error, so we adjust our count */
    }
#endif
  }
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





static void PrintDirEntry(WINDOW *win, 
                          int entry_no, 
                          int y, 
                          unsigned char hilight)
{
  unsigned int j, l1, l2;
  int  n; 
  int  aux=0;
  int  color, hi_color;
  char buffer[32*3+PATH_LENGTH+1];
  char format[60];
  char *line_buffer = NULL;
  char *dir_name;
  char attributes[11];
  char modify_time[13];
  char change_time[13];
  char access_time[13];
  char owner[OWNER_NAME_MAX + 1];
  char group[GROUP_NAME_MAX + 1];
  char *owner_name_ptr;
  char *group_name_ptr;
  DirEntry *de_ptr;
  BOOL suppress_output = FALSE;


  if(win == f2_window) {
    color     = HST_COLOR;
    hi_color  = HIHST_COLOR;
  } else {
    color     = DIR_COLOR;
    hi_color  = HIDIR_COLOR;
  }
  buffer[0] = '\0';

  for(j=0; j < dir_entry_list[entry_no].level; j++) 
  {
    if( dir_entry_list[entry_no].indent & ( 1L << j ) )
      (void) strcat( buffer, "| " );
    else
      (void) strcat( buffer, "  " );
  }
  de_ptr = dir_entry_list[entry_no].dir_entry;
  if( de_ptr->next )
    (void) strcat( buffer, "6-" );
  else
    (void) strcat( buffer, "3-" );

  switch( dir_mode )
  {
    case MODE_1 : 
                 (void)GetAttributes(de_ptr->stat_struct.st_mode, attributes);
                 (void)CTime( de_ptr->stat_struct.st_mtime, modify_time );
                 if ((line_buffer = (char *) malloc(38)) == NULL)
                 {
                    ERROR_MSG("malloc() Failed*Abort");
                    exit(1);
                 }
#ifdef HAS_LONGLONG
                 (void) strcpy( format, "%10s %3d %8lld %12s");
                 
		 (void) sprintf( line_buffer, format, attributes,
                                 (int)de_ptr->stat_struct.st_nlink,
                                 (LONGLONG) de_ptr->stat_struct.st_size,
                                 modify_time
                                 );
#else
                 (void) strcpy( format, "%10s %3d %8d %12s");
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
    case MODE_3 : break;
    case MODE_4 : 
                 (void) CTime( de_ptr->stat_struct.st_ctime, change_time );
                 (void) CTime( de_ptr->stat_struct.st_atime, access_time );
                 (void) strcpy( format, "Chg.: %12s  Acc.: %12s");
                 if ((line_buffer = (char *) malloc(40)) == NULL)
                 {
                    ERROR_MSG("malloc() Failed*Abort");
                    exit(1);
                 }
                 (void)sprintf(line_buffer, format, change_time, access_time);
                 break;
  }
  
  if(window_width == 1)
    suppress_output = TRUE;

  aux = 0;
  /* Output optional Attributes */
  if (!suppress_output) {
     WbkgdSet(win, COLOR_PAIR(color)| A_BOLD );
     if(line_buffer) {
       aux = strlen(line_buffer);
       if(window_width <= aux) {
         line_buffer[window_width - 1] = '\0';
         suppress_output = TRUE;
       }
       mvwaddstr(win, y, 0, line_buffer );
    }
  }

  if(!suppress_output) {
    /* Output Graph */
    l1 = strlen(buffer);
    n = window_width - aux;
    if((int)l1 > n) {
       buffer[MAXIMUM(n-1, 0)] = '\0';
       suppress_output = TRUE;
    }
    PrintSpecialString( win, y, aux, buffer, color );
  }

  if(!suppress_output) {

    /* Output Dirname */

    dir_name = de_ptr->name;
    (void) strcpy( buffer, ( *dir_name ) ? dir_name : "." );
    if( de_ptr->not_scanned ) {
      (void) strcat( buffer, "/" );
    }
    
#ifdef NO_HIGHLIGHT
    (void) strcat( buffer, (hilight) ? " <" : "  " ); 
#else /* NO_HIGHLIGHT */
#ifdef COLOR_SUPPORT
    if( hilight ) 
      WbkgdSet(win, COLOR_PAIR(hi_color)|A_BOLD);
    else
      WbkgdSet(win, COLOR_PAIR(color));

    n = window_width - aux - l1;
    l2 = strlen(buffer);
    if((int)l2 > n)
      buffer[MAXIMUM(n-1, 0)] = '\0';

/*    waddstr( win, buffer );*/
    mvwaddstr( win, y, aux + l1, buffer);
    WbkgdSet(win, COLOR_PAIR(color)|A_BOLD);

#else
    if( hilight ) wattrset( win, A_REVERSE );
/*    waddstr( win, buffer );*/
    mvwaddstr( win, y, aux + l1, buffer);

    if( hilight ) wattrset( win, 0 );
#endif /* COLOR_SUPPORT */
#endif /* NO_HIGHLIGHT */
  }

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
  for(i=0; i < height; i++)
  {
    wmove(win, i, 0);
    wclrtoeol(win); 
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
      beep();
   }
   else
   {
      /* Element vorhanden */
      /*-------------------*/
      if( *cursor_pos + 1 < window_height )
      {
          /* Element ist ohne scrollen erreichbar */
          /*--------------------------------------*/
          PrintDirEntry( dir_window,
                         *disp_begin_pos + *cursor_pos, 
                         *cursor_pos,
                         FALSE
                         );
          (*cursor_pos)++;
          PrintDirEntry( dir_window,
                         *disp_begin_pos + *cursor_pos,
                         *cursor_pos,
                         TRUE
                         );
      }
      else
      {
          /* Es muss gescrollt werden */
          /*--------------------------*/
          PrintDirEntry( dir_window,
                         *disp_begin_pos + *cursor_pos, 
                         *cursor_pos,
                         FALSE
                         );
          scroll( dir_window );
          (*disp_begin_pos)++;
          PrintDirEntry( dir_window,
                         *disp_begin_pos + *cursor_pos, 
                         *cursor_pos,
                         TRUE
                         );
      }
      *dir_entry = dir_entry_list[*disp_begin_pos + *cursor_pos].dir_entry;
      (*dir_entry)->start_file = 0;
      (*dir_entry)->cursor_pos = -1;
      DisplayFileWindow( *dir_entry );
      RefreshWindow( file_window );
   }
   return;
}


static void Moveup(int *disp_begin_pos, int *cursor_pos, DirEntry **dir_entry)
{
   if( *disp_begin_pos + *cursor_pos - 1 < 0 )
   {
      /* Element nicht vorhanden */
      /*-------------------------*/
      beep();
   }
   else
   {
      /* Element vorhanden */
      /*-------------------*/
      if( *cursor_pos - 1 >= 0 )
      {
         /* Element ist ohne scrollen erreichbar */
         /*--------------------------------------*/
         PrintDirEntry( dir_window,
                        *disp_begin_pos + *cursor_pos, 
                        *cursor_pos,
                        FALSE
                        );
         (*cursor_pos)--;
         PrintDirEntry( dir_window,
                        *disp_begin_pos + *cursor_pos,
                        *cursor_pos,
                        TRUE
                        );
      }
      else
      {
          /* Es muss gescrollt werden */
          /*--------------------------*/
          PrintDirEntry( dir_window,
                         *disp_begin_pos + *cursor_pos, 
                         *cursor_pos,
                         FALSE
                         );
          wmove( dir_window, 0, 0 );
          winsertln( dir_window );
          (*disp_begin_pos)--;
          PrintDirEntry( dir_window,
                         *disp_begin_pos + *cursor_pos, 
                         *cursor_pos,
                         TRUE
                         );
      }
      *dir_entry = dir_entry_list[*disp_begin_pos + *cursor_pos].dir_entry;
      (*dir_entry)->start_file = 0;
      (*dir_entry)->cursor_pos = -1;
      DisplayFileWindow( *dir_entry );
      RefreshWindow( file_window );
   }
   return;
}


static void Movenpage(int *disp_begin_pos, int *cursor_pos, DirEntry **dir_entry)
{
   if( *disp_begin_pos + *cursor_pos >= total_dirs - 1 )
   {
      /* Letzte Position */
      /*-----------------*/
      beep();
   }
   else
   {
      if( *cursor_pos < window_height - 1 )
      {
          /* Cursor steht nicht auf letztem Eintrag
           * der Seite
           */
           PrintDirEntry( dir_window,
                          *disp_begin_pos + *cursor_pos, 
                          *cursor_pos,
                          FALSE
                          );
           if( *disp_begin_pos + window_height > total_dirs  - 1 )
              *cursor_pos = total_dirs - *disp_begin_pos - 1;
           else
              *cursor_pos = window_height - 1;
           *dir_entry = dir_entry_list[*disp_begin_pos + *cursor_pos].dir_entry;
      	   (*dir_entry)->start_file = 0;
      	   (*dir_entry)->cursor_pos = -1;
           DisplayFileWindow( *dir_entry );
           RefreshWindow( file_window );
           PrintDirEntry( dir_window,
                          *disp_begin_pos + *cursor_pos, 
                          *cursor_pos,
                          TRUE
                          );
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
          *dir_entry = dir_entry_list[*disp_begin_pos + *cursor_pos].dir_entry;
      	  (*dir_entry)->start_file = 0;
      	  (*dir_entry)->cursor_pos = -1;
          DisplayFileWindow( *dir_entry );
          RefreshWindow( file_window );
          DisplayTree(dir_window,*disp_begin_pos,*disp_begin_pos+*cursor_pos);
      }
   }
   return;
}

static void Moveppage(int *disp_begin_pos, int *cursor_pos, DirEntry **dir_entry)
{
   if( *disp_begin_pos + *cursor_pos <= 0 )
   {
      /* Erste Position */
      /*----------------*/
      beep();
   }
   else
   {
      if( *cursor_pos > 0 )
      {
          /* Cursor steht nicht auf erstem Eintrag
           * der Seite
           */
           PrintDirEntry( dir_window,
                          *disp_begin_pos + *cursor_pos, 
                          *cursor_pos,
                          FALSE
                          );
           *cursor_pos = 0;
           *dir_entry = dir_entry_list[*disp_begin_pos + *cursor_pos].dir_entry;
      	   (*dir_entry)->start_file = 0;
      	   (*dir_entry)->cursor_pos = -1;
           DisplayFileWindow( *dir_entry );
           RefreshWindow( file_window );
           PrintDirEntry( dir_window,
                          *disp_begin_pos + *cursor_pos, 
                          *cursor_pos,
                          TRUE
                          );
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
         *dir_entry = dir_entry_list[*disp_begin_pos + *cursor_pos].dir_entry;
      	 (*dir_entry)->start_file = 0;
      	 (*dir_entry)->cursor_pos = -1;
         DisplayFileWindow( *dir_entry );
         RefreshWindow( file_window );
         DisplayTree(dir_window,*disp_begin_pos,*disp_begin_pos+*cursor_pos);
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
    return;
}

static void MoveHome(DirEntry **dir_entry)
{
    if( statistic.disp_begin_pos == 0 && statistic.cursor_pos == 0 )
    {  /* Position 1 bereits errecht */
       /*----------------------------*/
       beep();
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
        beep();
        return;
    }
    if( !dir_entry->not_scanned ) {
        beep();
    } else {
	for( de_ptr=dir_entry->sub_tree; de_ptr; de_ptr=de_ptr->next) {
	    GetPath( de_ptr, new_login_path );
	    ReadTree( de_ptr, new_login_path, 0 );
	    SetMatchingParam( de_ptr );
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
        beep();
        return;
    }
    if( dir_entry->not_scanned || (dir_entry->sub_tree == NULL) ) {
	beep();
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
			statistic.disp_begin_pos + statistic.cursor_pos );
	    *ch = 'L';
	}
    } else {
	dir_entry->login_flag = FALSE;
	beep();
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
	beep();
    }
    return;
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

  if( dir_entry->login_flag )
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
       DisplayFileSpec();
       DisplayDiskName();
       resize_request = FALSE;
    }

   if (mode == USER_MODE) { /* DirUserMode returns (possibly remapped) ch, or -1 if it handles ch */
      ch = DirUserMode(dir_entry, ch);
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
      case 'F':
      case 'f':      if(ReadFileSpec() == 0) {
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
			 beep();
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
      default :      beep();
		     break;
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
      SetMatchingParam( de_ptr );
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
		       beep(); 
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
		       beep(); 
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
		       beep(); 
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
		       beep(); 
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
		     { beep(); }
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

      default :      beep();
		     break;
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