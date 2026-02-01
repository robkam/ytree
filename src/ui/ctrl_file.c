/***************************************************************************
 *
 * src/ui/ctrl_file.c
 * File Window Controller (Input & Event Handling)
 *
 ***************************************************************************/

#include "ytree.h"
#include "watcher.h"
#include "sort.h"
#include "ytree_ui.h"

#define MAX( a, b ) ( ( (a) > (b) ) ? (a) : (b) )

#if !defined(__NeXT__) && !defined(ultrix)
extern void qsort(void *, size_t, size_t, int (*) (const void *, const void *));
#endif /* __NeXT__ ultrix */

/* Local Statics for metrics calculation - Pushed to Renderer */
static int  max_disp_files;
static int  x_step;
static int  my_x_step;
/* static int  hide_left; */ /* Removed: Unused */
static int  hide_right;

static unsigned      max_visual_userview_len;
static unsigned      max_visual_filename_len;
static unsigned      max_visual_linkname_len;
static unsigned      global_max_visual_filename_len;
static unsigned      global_max_visual_linkname_len;

static long preview_line_offset = 0;
static int saved_fixed_width = 0;

/* --- Forward Declarations --- */
static void ReadFileList(BOOL tagged_only, DirEntry *dir_entry);
static void ReadGlobalFileList(BOOL tagged_only, DirEntry *dir_entry);
static void WalkTaggedFiles(int start_file, int cursor_pos, int (*fkt) (FileEntry *, WalkingPackage *), WalkingPackage *walking_package);
static BOOL IsMatchingTaggedFiles(void);
static void RemoveFileEntry(int entry_no);
static void ChangeFileEntry(void);
static int  DeleteTaggedFiles(int max_dispfiles, Statistic *s);
static void SilentWalkTaggedFiles( int (*fkt) (FileEntry *, WalkingPackage *, Statistic *), WalkingPackage *walking_package);
static void SilentTagWalkTaggedFiles( int (*fkt) (FileEntry *, WalkingPackage *, Statistic *), WalkingPackage *walking_package);
static void RereadWindowSize(DirEntry *dir_entry);
static void ListJump( DirEntry * dir_entry, char *str );
static void HandleInvertTags(DirEntry *dir_entry, Statistic *s);
static DirEntry *RefreshFileView(DirEntry *dir_entry);
static void fmovedown(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry);
static void fmoveup(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry);
static void fmoveright(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry);
static void fmoveleft(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry);
static void fmovenpage(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry);
static void fmoveppage(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry);
static void UpdatePreview(DirEntry *dir_entry);


void BuildFileEntryList(YtreePanel *panel) {
  size_t alloc_count;
  long t_files = 0;
  LONGLONG t_bytes = 0;
  size_t i; /* Loop counter for recalculating tagged stats */
  DirEntry *dir_entry;
  Statistic *s;
  int idx;

  if (!panel || !panel->vol) return; /* Safety check */

  s = &panel->vol->vol_stats;

  /* Derive current directory from panel state */
  if (panel->vol->dir_entry_list && panel->vol->total_dirs > 0) {
      idx = panel->disp_begin_pos + panel->cursor_pos;
      /* Clamp */
      if (idx < 0) idx = 0;
      if (idx >= panel->vol->total_dirs) idx = panel->vol->total_dirs - 1;

      dir_entry = panel->vol->dir_entry_list[idx].dir_entry;
  } else {
      dir_entry = s->tree;
  }

  if (!dir_entry) return;

  if( panel->file_entry_list ) {
    free( panel->file_entry_list );
    panel->file_entry_list = NULL;
    panel->file_entry_list_capacity = 0;
  }

  if( !dir_entry->global_flag )  {
    /* Normal Directory View */

    alloc_count = dir_entry->matching_files; /* This is the potentially stale value */
    if (alloc_count < 16) alloc_count = 16;

    if( ( panel->file_entry_list = (FileEntryList *)
                  calloc( alloc_count,
                      sizeof( FileEntryList )
                    )
                              ) == NULL ) {
        ERROR_MSG( "Calloc Failed*ABORT" );
        exit( 1 );
    }
    panel->file_entry_list_capacity = alloc_count;

    panel->file_count = 0;
    ReadFileList( dir_entry->tagged_flag, dir_entry );
    Panel_Sort(panel, s->kind_of_sort);

    /* Push metrics to renderer and recalc layout */
    SetFileRenderingMetrics(max_visual_filename_len, max_visual_linkname_len, max_visual_userview_len);
    SetFileMode( GetFileMode() );

    /* Recalculate and update statistics based on the actual loaded list */
    dir_entry->matching_files = panel->file_count;
    t_files = 0;
    t_bytes = 0;
    for (i = 0; i < panel->file_count; i++) {
        if (panel->file_entry_list[i].file->tagged) {
            t_files++;
            t_bytes += panel->file_entry_list[i].file->stat_struct.st_size;
        }
    }
    dir_entry->tagged_files = t_files;
    dir_entry->tagged_bytes = t_bytes;

    } else {
    /* Global / ShowAll View */

    size_t count_source = (dir_entry->tagged_flag) ? s->disk_tagged_files : s->disk_matching_files;
    alloc_count = count_source; /* This is also potentially stale */
    if (alloc_count < 16) alloc_count = 16;

    if( ( panel->file_entry_list = (FileEntryList *)
           calloc( alloc_count,
                      sizeof( FileEntryList )
              ) ) == NULL )  {
           ERROR_MSG( "Calloc Failed*ABORT" );
           exit( 1 );
    }
    panel->file_entry_list_capacity = alloc_count;

    panel->file_count = 0;
    global_max_visual_filename_len = 0;
    global_max_visual_linkname_len = 0;
    ReadGlobalFileList(  dir_entry->tagged_flag, s->tree );
    Panel_Sort(panel, s->kind_of_sort);

    /* Push metrics to renderer and recalc layout */
    SetFileRenderingMetrics(max_visual_filename_len, max_visual_linkname_len, max_visual_userview_len);
    SetFileMode( GetFileMode() );

    /* Recalculate and update statistics based on the actual loaded list */
    /* Note: Previously, this block overwrote dir_entry->matching_files/tagged_files.
       This corruption has been removed. Global stats are now handled by stats.c directly. */
    }
}


static void ReadFileList(BOOL tagged_only, DirEntry *dir_entry)
{
  FileEntry *fe_ptr;
  unsigned int name_len;
  unsigned int visual_name_len;
  unsigned int visual_linkname_len;

  max_visual_filename_len = 0;
  max_visual_linkname_len = 0;

  for( fe_ptr = dir_entry->file; fe_ptr; fe_ptr = fe_ptr->next )
  {
    if( fe_ptr->matching && (!tagged_only || fe_ptr->tagged) )
    {
      /* Ensure hidden dot files are skipped unless option is disabled.
         This applies to both FS mode and Archive mode. */
      if (hide_dot_files && fe_ptr->name[0] == '.')
          continue;

      /* Bounds check */
      if (ActivePanel->file_count >= ActivePanel->file_entry_list_capacity) {
          size_t new_capacity = ActivePanel->file_entry_list_capacity * 2;
          if (new_capacity == 0) new_capacity = 128;

          FileEntryList *new_list = (FileEntryList *) realloc(ActivePanel->file_entry_list, new_capacity * sizeof(FileEntryList));
          if (!new_list) {
              ERROR_MSG("Realloc failed in ReadFileList*ABORT");
              exit(1);
          }
          memset(new_list + ActivePanel->file_entry_list_capacity, 0, (new_capacity - ActivePanel->file_entry_list_capacity) * sizeof(FileEntryList));
          ActivePanel->file_entry_list = new_list;
          ActivePanel->file_entry_list_capacity = new_capacity;
      }

      ActivePanel->file_entry_list[ActivePanel->file_count++].file = fe_ptr;
      visual_name_len = StrVisualLength( fe_ptr->name );
      name_len = strlen( fe_ptr->name );
      if( S_ISLNK( fe_ptr->stat_struct.st_mode ) )
      {
	visual_linkname_len = StrVisualLength( &fe_ptr->name[name_len+1] );
	max_visual_linkname_len = MAX( (int)max_visual_linkname_len, (int)visual_linkname_len );
      }
      max_visual_filename_len = MAX( (int)max_visual_filename_len, (int)visual_name_len );
    }
  }
}



static void ReadGlobalFileList(BOOL tagged_only, DirEntry *dir_entry)
{
  DirEntry  *de_ptr;

  for( de_ptr=dir_entry; de_ptr; de_ptr=de_ptr->next )
  {
    if (hide_dot_files && de_ptr->name[0] == '.')
        continue;
    if( de_ptr->sub_tree ) ReadGlobalFileList( tagged_only, de_ptr->sub_tree );
    ReadFileList( tagged_only, de_ptr );
    global_max_visual_filename_len = MAX( (int)global_max_visual_filename_len, (int)max_visual_filename_len );
    global_max_visual_linkname_len = MAX( (int)global_max_visual_linkname_len, (int)max_visual_linkname_len );
  }
  max_visual_filename_len = global_max_visual_filename_len;
  max_visual_linkname_len = global_max_visual_linkname_len;
}


/* Removed SetKindOfSort definition - implemented in sort.c */



static void RemoveFileEntry(int entry_no)
{
  int i, n;
  FileEntry *fe_ptr;
  int visual_name_len, name_len;

  max_visual_filename_len = 0;
  max_visual_linkname_len = 0;
  n = ActivePanel->file_count - 1;

  for( i=0; i < n; i++ )
  {
    if( i >= entry_no ) ActivePanel->file_entry_list[i] = ActivePanel->file_entry_list[i+1];
    fe_ptr = ActivePanel->file_entry_list[i].file;
    visual_name_len = StrVisualLength( fe_ptr->name );
    name_len = strlen( fe_ptr->name );
    /* FIX: Cast StrVisualLength to int for MAX macro */
    max_visual_filename_len = MAX( (int)max_visual_filename_len, (int)visual_name_len );
    if( S_ISLNK( fe_ptr->stat_struct.st_mode ) )
    {
      /* FIX: Cast StrVisualLength to int for MAX macro */
      max_visual_linkname_len = MAX( (int)max_visual_filename_len, (int)StrVisualLength( &fe_ptr->name[name_len+1] ) );
    }
  }

  SetFileRenderingMetrics(max_visual_filename_len, max_visual_linkname_len, max_visual_userview_len);
  SetFileMode( GetFileMode() );

  ActivePanel->file_count--; /* no realloc */
}



static void ChangeFileEntry(void)
{
  int i, n;
  FileEntry *fe_ptr;
  int visual_name_len, name_len;

  max_visual_filename_len = 0;
  max_visual_linkname_len = 0;
  n = ActivePanel->file_count - 1;

  for( i=0; i < n; i++ )
  {
    fe_ptr = ActivePanel->file_entry_list[i].file;
    if( fe_ptr )
    {
      visual_name_len = StrVisualLength( fe_ptr->name );
      name_len = strlen( fe_ptr->name );
      /* FIX: Cast StrVisualLength to int for MAX macro */
      max_visual_filename_len = MAX( (int)max_visual_filename_len, (int)visual_name_len );
      if( S_ISLNK( fe_ptr->stat_struct.st_mode ) )
      {
        /* FIX: Cast StrVisualLength to int for MAX macro */
        max_visual_linkname_len = MAX( (int)max_visual_linkname_len, (int)StrVisualLength( &fe_ptr->name[name_len+1] ) );
      }
    }
  }

  SetFileRenderingMetrics(max_visual_filename_len, max_visual_linkname_len, max_visual_userview_len);
  SetFileMode( GetFileMode() );
}

void DisplayFileWindow(YtreePanel *panel, DirEntry *dir_entry)
{
  if (!panel || !panel->pan_file_window) return;

  BuildFileEntryList(panel);
  DisplayFiles(panel, dir_entry,
		dir_entry->start_file,
                dir_entry->start_file + dir_entry->cursor_pos, 0, panel->pan_file_window);
}


static void fmovedown(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry)
{
   Nav_MoveDown(cursor_pos, start_file, (int)ActivePanel->file_count, max_disp_files, 1);

    DisplayFiles(ActivePanel, dir_entry, *start_file, *start_file + *cursor_pos, *start_x, file_window);
    /* Update dynamic header path */
    if (ActivePanel->file_count > 0) {
        char path[PATH_LENGTH];
        int idx = *start_file + *cursor_pos;
        if ((unsigned)idx < ActivePanel->file_count) {
            GetFileNamePath(ActivePanel->file_entry_list[idx].file, path);
            DisplayHeaderPath(path);
        }
    } else {
        char path[PATH_LENGTH];
        GetPath(dir_entry, path);
        DisplayHeaderPath(path);
    }
}

static void fmoveup(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry)
{
   Nav_MoveUp(cursor_pos, start_file);

    DisplayFiles(ActivePanel, dir_entry, *start_file, *start_file + *cursor_pos, *start_x, file_window);
    /* Update dynamic header path */
    if (ActivePanel->file_count > 0) {
        char path[PATH_LENGTH];
        int idx = *start_file + *cursor_pos;
        if ((unsigned)idx < ActivePanel->file_count) {
            GetFileNamePath(ActivePanel->file_entry_list[idx].file, path);
            DisplayHeaderPath(path);
        }
    } else {
        char path[PATH_LENGTH];
        GetPath(dir_entry, path);
        DisplayHeaderPath(path);
    }
}

static void fmoveright(int *start_file, int *cursor_pos, int *start_x,DirEntry *dir_entry)
{
   if( x_step == 1 )
   {
      /* Special case: scroll entire file window */
      /*------------------------*/
      (*start_x)++;
      DisplayFiles(ActivePanel, dir_entry,
                    *start_file,
                    *start_file + *cursor_pos,
                    *start_x,
                    file_window
                  );
      if( hide_right <= 0 ) (*start_x)--;
   }
   else if( (unsigned int)(*start_file + *cursor_pos) >= ActivePanel->file_count - 1 )
   {
      /* last position reached */
      /*-------------------------*/
   }
   else
   {
      if( (unsigned int)(*start_file + *cursor_pos + x_step) >= ActivePanel->file_count )
      {
          /* full step not possible;
           * position on last entry
           */
           my_x_step = (int)ActivePanel->file_count - *start_file - *cursor_pos - 1;
      }
      else
      {
          my_x_step = x_step;
      }
      if( *cursor_pos + my_x_step < max_disp_files )
      {
          /* RIGHT possible without scrolling */
          /*----------------------------*/
          *cursor_pos += my_x_step;
      }
      else
      {
          /* Scrolling */
          /*----------*/
          *start_file += x_step;
          *cursor_pos -= x_step - my_x_step;
      }
      DisplayFiles(ActivePanel, dir_entry,
                    *start_file,
                    *start_file + *cursor_pos,
                    *start_x,
                    file_window
                  );
   }
   /* Update dynamic header path */
   if (ActivePanel->file_count > 0) {
       char path[PATH_LENGTH];
       int idx = *start_file + *cursor_pos;
       if ((unsigned)idx < ActivePanel->file_count) {
           GetFileNamePath(ActivePanel->file_entry_list[idx].file, path);
           DisplayHeaderPath(path);
       }
   } else {
       char path[PATH_LENGTH];
       GetPath(dir_entry, path);
       DisplayHeaderPath(path);
   }
   return;
}


static void fmoveleft(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry)
{
     if( x_step == 1 )
     {
         /* Special case: scroll entire file window */
         /*----------------------------------------*/
         if( *start_x > 0 ) (*start_x)--;
         DisplayFiles(ActivePanel, dir_entry,
                       *start_file,
                       *start_file + *cursor_pos,
                       *start_x,
                       file_window
                     );
     }
     else if( *start_file + *cursor_pos <= 0 )
     {
         /* first position reached */
         /*-------------------------*/
     }
     else
     {
         if( *start_file + *cursor_pos - x_step < 0 )
         {
             /* full step not possible;
              * position on first entry
              */
              my_x_step = *start_file + *cursor_pos;
         }
         else
         {
             my_x_step = x_step;
         }
         if( *cursor_pos - my_x_step >= 0 )
         {
             /* LEFT possible without scrolling */
             /*---------------------------*/
             *cursor_pos -= my_x_step;
         }
         else
         {
             /* Scrolling */
             /*----------*/
             if( ( *start_file -= x_step ) < 0 )
                *start_file = 0;
         }
         DisplayFiles(ActivePanel, dir_entry,
                       *start_file,
                       *start_file + *cursor_pos,
                       *start_x,
                       file_window
                       );
     }
     /* Update dynamic header path */
     if (ActivePanel->file_count > 0) {
         char path[PATH_LENGTH];
         int idx = *start_file + *cursor_pos;
         if ((unsigned)idx < ActivePanel->file_count) {
             GetFileNamePath(ActivePanel->file_entry_list[idx].file, path);
             DisplayHeaderPath(path);
         }
     } else {
         char path[PATH_LENGTH];
         GetPath(dir_entry, path);
         DisplayHeaderPath(path);
     }
     return;
}


static void fmovenpage(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry)
{
   Nav_PageDown(cursor_pos, start_file, (int)ActivePanel->file_count, max_disp_files);

    DisplayFiles(ActivePanel, dir_entry,
                  *start_file,
                  *start_file + *cursor_pos,
                  *start_x,
                  file_window
                );
    /* Update dynamic header path */
    if (ActivePanel->file_count > 0) {
        char path[PATH_LENGTH];
        int idx = *start_file + *cursor_pos;
        if ((unsigned)idx < ActivePanel->file_count) {
            GetFileNamePath(ActivePanel->file_entry_list[idx].file, path);
            DisplayHeaderPath(path);
        }
    } else {
        char path[PATH_LENGTH];
        GetPath(dir_entry, path);
        DisplayHeaderPath(path);
    }
}



static void fmoveppage(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry)
{
     Nav_PageUp(cursor_pos, start_file, max_disp_files);

    DisplayFiles(ActivePanel, dir_entry,
                  *start_file,
                  *start_file + *cursor_pos,
                  *start_x,
                  file_window
                );
    /* Update dynamic header path */
    if (ActivePanel->file_count > 0) {
        char path[PATH_LENGTH];
        int idx = *start_file + *cursor_pos;
        if ((unsigned)idx < ActivePanel->file_count) {
            GetFileNamePath(ActivePanel->file_entry_list[idx].file, path);
            DisplayHeaderPath(path);
        }
    } else {
        char path[PATH_LENGTH];
        GetPath(dir_entry, path);
        DisplayHeaderPath(path);
    }
}

/* Local helper to refresh file window, maintaining file cursor */
static DirEntry *RefreshFileView(DirEntry *dir_entry) {
    char *saved_name = NULL;
    Statistic *s = &ActivePanel->vol->vol_stats;
    int found_idx = -1;
    int start_x = 0;

    /* 1. Save current filename */
    if (ActivePanel->file_count > 0) {
        /* Check bounds before access */
        if (dir_entry->start_file + dir_entry->cursor_pos < (int)ActivePanel->file_count) {
            FileEntry *curr = ActivePanel->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
            if (curr) saved_name = strdup(curr->name);
        }
    }

    /* 2. Perform Safe Tree Refresh (Save/Rescan/Restore) */
    /* Update the local pointer with the valid address returned by RefreshTreeSafe */
    dir_entry = RefreshTreeSafe(ActivePanel, dir_entry);

    /* 3. Rebuild File List from the refreshed tree */
    BuildFileEntryList(ActivePanel);

    /* 4. Restore Cursor Position */
    if (saved_name) {
        int k;
        for (k = 0; k < (int)ActivePanel->file_count; k++) {
            if (strcmp(ActivePanel->file_entry_list[k].file->name, saved_name) == 0) {
                found_idx = k;
                break;
            }
        }
        free(saved_name);
    }

    if (found_idx != -1) {
        /* Calculate new start_file and cursor_pos */
        if (found_idx >= dir_entry->start_file &&
            found_idx < dir_entry->start_file + max_disp_files) {
            /* Still on screen, just move cursor */
            dir_entry->cursor_pos = found_idx - dir_entry->start_file;
        } else {
            /* Off screen, recenter or scroll */
            dir_entry->start_file = found_idx;
            dir_entry->cursor_pos = 0;
            /* Bounds check */
            if (dir_entry->start_file + max_disp_files > (int)ActivePanel->file_count) {
                 dir_entry->start_file = MAXIMUM(0, (int)ActivePanel->file_count - max_disp_files);
                 dir_entry->cursor_pos = (int)ActivePanel->file_count - 1 - dir_entry->start_file;
            }
        }
    } else {
        /* File gone, reset to top */
        dir_entry->start_file = 0;
        dir_entry->cursor_pos = 0;
    }

    /* 5. Update Display */
    if( dir_entry->global_flag )
        DisplayDiskStatistic(s);
    else
        DisplayDirStatistic( dir_entry, NULL, s );

    DisplayFiles(ActivePanel, dir_entry, dir_entry->start_file, dir_entry->start_file + dir_entry->cursor_pos, start_x, file_window);

    return dir_entry;
}

static void HandleInvertTags(DirEntry *dir_entry, Statistic *s)
{
    int i;
    FileEntry *fe_ptr;
    DirEntry *de_ptr;

    /* Iterate through the currently visible list of files */
    for (i = 0; i < (int)ActivePanel->file_count; i++)
    {
        fe_ptr = ActivePanel->file_entry_list[i].file;
        de_ptr = fe_ptr->dir_entry;

        /* Only invert matching files */
        if (fe_ptr->matching)
        {
            if (fe_ptr->tagged)
            {
                fe_ptr->tagged = FALSE;
                de_ptr->tagged_files--;
                de_ptr->tagged_bytes -= fe_ptr->stat_struct.st_size;
                s->disk_tagged_files--;
                s->disk_tagged_bytes -= fe_ptr->stat_struct.st_size;
            }
            else
            {
                fe_ptr->tagged = TRUE;
                de_ptr->tagged_files++;
                de_ptr->tagged_bytes += fe_ptr->stat_struct.st_size;
                s->disk_tagged_files++;
                s->disk_tagged_bytes += fe_ptr->stat_struct.st_size;
            }
        }
    }

    /* Refresh UI */
    DisplayFiles(ActivePanel, dir_entry,
                 dir_entry->start_file,
                 dir_entry->start_file + dir_entry->cursor_pos,
                 0 /* start_x */,
                 file_window
                );
    DisplayDiskStatistic(s);
    if( dir_entry->global_flag )
        DisplayDiskStatistic(s); /* Global view */
    else
        DisplayDirStatistic(dir_entry, NULL, s);
}

static void UpdatePreview(DirEntry *dir_entry)
{
    FileEntry *fe_ptr;
    char path[PATH_LENGTH + 1];

    if (!GlobalView->preview_mode || !preview_window) return;

    if (ActivePanel->file_count == 0) {
        wclear(preview_window);
        wnoutrefresh(preview_window);
        return;
    }

    /* Check bounds */
    if (dir_entry->start_file + dir_entry->cursor_pos >= (int)ActivePanel->file_count) return;

    fe_ptr = ActivePanel->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
    if (!fe_ptr) return;

    if (mode == ARCHIVE_MODE) {
        GetFileNamePath(fe_ptr, path);
        RenderArchivePreview(preview_window, ActivePanel->vol->vol_stats.login_path, path, &preview_line_offset);
    } else {
        GetRealFileNamePath(fe_ptr, path);
        RenderFilePreview(preview_window, path, &preview_line_offset, 0);
    }
}

int HandleFileWindow(DirEntry *dir_entry)
{
  FileEntry *fe_ptr;
  FileEntry *new_fe_ptr;
  DirEntry  *de_ptr = NULL;
  DirEntry  *dest_dir_entry;
  WalkingPackage walking_package; /* Local variable, not a pointer */
  int ch;
  int unput_char;
  int list_pos;
  LONGLONG file_size;
  int i;
  int owner_id;
  int group_id;
  int start_x = 0;
  char filepath[PATH_LENGTH +1];
  char modus[11];
  BOOL path_copy;
  int  term;
  int  mask;
  static char to_dir[PATH_LENGTH+1];
  static char to_path[PATH_LENGTH+1];
  static char to_file[PATH_LENGTH+1];
  BOOL need_dsp_help;
  BOOL maybe_change_x_step;
  char new_name[PATH_LENGTH+1];
  char expanded_new_name[PATH_LENGTH+1]; /* Added for rename expansion */
  char expanded_to_file[PATH_LENGTH+1];  /* Added for copy/move expansion */
  char new_login_path[PATH_LENGTH + 1];
  int  get_dir_ret;
  YtreeAction action = ACTION_NONE; /* Initialize action */
  DirEntry *last_stats_dir = NULL; /* Track context changes */
  struct Volume *start_vol = CurrentVolume; /* Safety Check Variable */
  Statistic *s = &ActivePanel->vol->vol_stats;
  int pclose_ret;
  char watcher_path[PATH_LENGTH + 1];

  unput_char = '\0';
  fe_ptr = NULL;


  /* Reset cursor position flags */
  /*--------------------------------------*/

  need_dsp_help = TRUE;
  maybe_change_x_step = TRUE;

  BuildFileEntryList( ActivePanel );

  /* Sanitize cursor position immediately after BuildFileEntryList */
  if (ActivePanel->file_count > 0) {
      if (dir_entry->cursor_pos < 0) dir_entry->cursor_pos = 0;
      if (dir_entry->start_file < 0) dir_entry->start_file = 0;

      /* Bounds Check: ensure we aren't past the end */
      if ((unsigned int)(dir_entry->start_file + dir_entry->cursor_pos) >= ActivePanel->file_count) {
          dir_entry->start_file = MAXIMUM(0, (int)ActivePanel->file_count - max_disp_files);
          dir_entry->cursor_pos = (int)ActivePanel->file_count - 1 - dir_entry->start_file;
      }
  } else {
      dir_entry->cursor_pos = 0;
      dir_entry->start_file = 0;
  }

  /* Initial Display using Centralized Function if applicable */
  if (GlobalView->preview_mode) {
      RefreshGlobalView(dir_entry);
      UpdatePreview(dir_entry);
  } else {
      /* Standard Logic for Big/Small Window */
      if( dir_entry->global_flag || dir_entry->big_window || dir_entry->tagged_flag)
      {
        SwitchToBigFileWindow();
        DisplayDiskStatistic(s);
        DisplayDirStatistic(dir_entry, (dir_entry->global_flag) ? "SHOW ALL" : NULL, s);
      }
      else
      {
        DisplayDirStatistic( dir_entry, NULL, s );
      }

      DisplayFiles(ActivePanel, dir_entry,
            dir_entry->start_file,
            dir_entry->start_file + dir_entry->cursor_pos,
            start_x,
            file_window
          );
  }

  if (s->login_mode == DISK_MODE || s->login_mode == USER_MODE) {
      GetPath(dir_entry, watcher_path);
      Watcher_SetDir(watcher_path);
  }

  do
  {
    /* Critical Safety: If volume was deleted (e.g. via K menu), abort immediately */
    if (CurrentVolume != start_vol) return ESC;

    if( maybe_change_x_step )
    {
      maybe_change_x_step = FALSE;

      x_step =  (GetMaxColumn() > 1) ? getmaxy(file_window) : 1;
      max_disp_files = getmaxy(file_window) * GetMaxColumn();
    }

    if (need_dsp_help) {
        need_dsp_help = FALSE;
        if (GlobalView->preview_mode)
            DisplayPreviewHelp();
        else
            DisplayFileHelp();
    }

    if( unput_char )
    {
      ch = unput_char;
      unput_char = '\0';
    }
    else
    {
      /* Guard fe_ptr access against empty file list */
      if (ActivePanel->file_count > 0) {
           fe_ptr = ActivePanel->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;

           /* Dynamic Stats Update: Check if context changed */
           if (fe_ptr && fe_ptr->dir_entry != last_stats_dir) {
               last_stats_dir = fe_ptr->dir_entry;
               /* Pass "SHOW ALL" if global flag is set, otherwise NULL for default */
               DisplayDirStatistic(last_stats_dir, (dir_entry->global_flag) ? "SHOW ALL" : NULL, s);
           }
      } else {
           fe_ptr = NULL;
      }

      if( dir_entry->global_flag )
        DisplayGlobalFileParameter( fe_ptr );
      else
        DisplayFileParameter( fe_ptr );

      if (!GlobalView->preview_mode) RefreshWindow( dir_window ); /* needed: ncurses-bug ? */
      RefreshWindow( file_window );

      if (IsSplitScreen) {
          YtreePanel *inactive = (ActivePanel == LeftPanel) ? RightPanel : LeftPanel;
          RenderInactivePanel(inactive);
      }

      doupdate();
      ch = (resize_request) ? -1 : GetEventOrKey();
      if( ch == LF ) ch = CR;
    }

    /* Re-check safety after blocking Getch */
    if (CurrentVolume != start_vol) return ESC;

    if (IsUserActionDefined()) { /* User commands take precedence */
       ch = FileUserMode(&(ActivePanel->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]), ch, &ActivePanel->vol->vol_stats);
       if (CurrentVolume != start_vol) return ESC;
    }

   if(resize_request) {
     /* Simplified Resize using Centralized Function */
     RereadWindowSize(dir_entry);
     RefreshGlobalView(dir_entry);
     if (GlobalView->preview_mode) UpdatePreview(dir_entry);
     need_dsp_help = TRUE;
     resize_request = FALSE;
   }

   action = GetKeyAction(ch);

   /* Special remapping for MODE_1: TAB/BTAB act as UP/DOWN */
   if( GetFileMode() == MODE_1 )
   {
      if( action == ACTION_TREE_EXPAND ) action = ACTION_MOVE_DOWN;
      else if( action == ACTION_TREE_COLLAPSE ) action = ACTION_MOVE_UP;
   }

   if( x_step == 1 && ( action == ACTION_MOVE_RIGHT || action == ACTION_MOVE_LEFT ) )
   {
      /* do not reset start_x */
      /*-----------------------------*/

      ; /* do nothing */
   }
   else
   {
      /* start at 0 */
      /*----------------*/

      if( start_x )
      {
	start_x = 0;

	DisplayFiles(ActivePanel, dir_entry,
		      dir_entry->start_file,
		      dir_entry->start_file + dir_entry->cursor_pos,
		      start_x,
                      file_window
		    );
     }
   }


   switch( action )
   {

      case ACTION_NONE:         break;

      case ACTION_VIEW_PREVIEW:
            GlobalView->preview_mode = !GlobalView->preview_mode;

            if (GlobalView->preview_mode) {
                /* Turning ON */
                /* 1. Save current width setting */
                saved_fixed_width = GlobalView->fixed_col_width;

                /* 2. Update Window Layout immediately to get new dims */
                ReCreateWindows();

                /* 3. Force Compact Mode based on new width (width - 2 for borders/padding) */
                /* Accessing layout.big_file_win_width from init.c logic */
                GlobalView->fixed_col_width = layout.big_file_win_width - 2;

                /* 4. Update scrolling metrics (max_column, etc) based on new width/mode */
                RereadWindowSize(dir_entry);
            } else {
                /* Turning OFF */
                /* 1. Restore width setting */
                GlobalView->fixed_col_width = saved_fixed_width;

                /* 2. Update Layout */
                ReCreateWindows();

                /* 3. Update metrics */
                RereadWindowSize(dir_entry);
            }

            /* 5. Draw Everything (Borders, Tree, Stats, List, Preview) */
            RefreshGlobalView(dir_entry);

            if (GlobalView->preview_mode) {
                UpdatePreview(dir_entry);
            }

            need_dsp_help = TRUE;
             break;

      case ACTION_PREVIEW_SCROLL_DOWN:
             if (GlobalView->preview_mode) {
                 preview_line_offset++;
                 UpdatePreview(dir_entry);
             }
             break;

      case ACTION_PREVIEW_SCROLL_UP:
             if (GlobalView->preview_mode) {
                 if (preview_line_offset > 0) preview_line_offset--;
                 UpdatePreview(dir_entry);
             }
             break;

      case ACTION_PREVIEW_HOME:
             if (GlobalView->preview_mode) {
                 preview_line_offset = 0;
                 UpdatePreview(dir_entry);
             }
             break;
      case ACTION_PREVIEW_END:
             if (GlobalView->preview_mode) {
                 /* Set to a large value, renderer handles EOF */
                 preview_line_offset = 2000000000L;
                 UpdatePreview(dir_entry);
             }
             break;
      case ACTION_PREVIEW_PAGE_UP:
             if (GlobalView->preview_mode) {
                 preview_line_offset -= (getmaxy(preview_window) - 1);
                 if (preview_line_offset < 0) preview_line_offset = 0;
                 UpdatePreview(dir_entry);
             }
             break;
      case ACTION_PREVIEW_PAGE_DOWN:
             if (GlobalView->preview_mode) {
                 preview_line_offset += (getmaxy(preview_window) - 1);
                 UpdatePreview(dir_entry);
             }
             break;

      case ACTION_SPLIT_SCREEN:
             IsSplitScreen = !IsSplitScreen;
             ReCreateWindows(); /* Force layout update immediately */

             if (IsSplitScreen) {
                 /* Explicitly copy state here because we won't hit the dirwin logic */
                 YtreePanel *source = ActivePanel;
                 YtreePanel *target = (ActivePanel == LeftPanel) ? RightPanel : LeftPanel;

                 target->vol = source->vol;
                 target->cursor_pos = source->cursor_pos;
                 target->disp_begin_pos = source->disp_begin_pos;
                 target->pan_file_window = target->pan_small_file_window;
             }

             return ESC; /* Return to DirWindow to redraw */

      case ACTION_SWITCH_PANEL:
             if (!IsSplitScreen) break;
             /* Switch Panel */
             if (ActivePanel == LeftPanel) {
                 ActivePanel = RightPanel;
             } else {
                 ActivePanel = LeftPanel;
             }
             /* Update Volume Context */
             CurrentVolume = ActivePanel->vol;

             return ESC; /* Exit loop to re-enter with new context */

      case ACTION_MOVE_DOWN :  fmovedown(&dir_entry->start_file, &dir_entry->cursor_pos, &start_x, dir_entry);
              if (GlobalView->preview_mode) { preview_line_offset = 0; UpdatePreview(dir_entry); }
		      break;

      case ACTION_MOVE_UP   : fmoveup(&dir_entry->start_file, &dir_entry->cursor_pos, &start_x, dir_entry);
              if (GlobalView->preview_mode) { preview_line_offset = 0; UpdatePreview(dir_entry); }
		      break;

      case ACTION_MOVE_RIGHT:
          if (!highlight_full_line && x_step == 1) break; /* No horizontal scroll in name-only mode */
          fmoveright(&dir_entry->start_file, &dir_entry->cursor_pos, &start_x, dir_entry);
          if (GlobalView->preview_mode) { preview_line_offset = 0; UpdatePreview(dir_entry); }
          break;

      case ACTION_MOVE_LEFT :
          if (!highlight_full_line && x_step == 1) break; /* No horizontal scroll in name-only mode */
          fmoveleft(&dir_entry->start_file, &dir_entry->cursor_pos, &start_x, dir_entry);
          if (GlobalView->preview_mode) { preview_line_offset = 0; UpdatePreview(dir_entry); }
          break;

      case ACTION_PAGE_DOWN: fmovenpage(&dir_entry->start_file, &dir_entry->cursor_pos, &start_x, dir_entry);
              if (GlobalView->preview_mode) { preview_line_offset = 0; UpdatePreview(dir_entry); }
		      break;

      case ACTION_PAGE_UP: fmoveppage(&dir_entry->start_file, &dir_entry->cursor_pos, &start_x, dir_entry);
              if (GlobalView->preview_mode) { preview_line_offset = 0; UpdatePreview(dir_entry); }
		      break;

      case ACTION_END  : Nav_End(&dir_entry->cursor_pos, &dir_entry->start_file, (int)ActivePanel->file_count, max_disp_files);

			DisplayFiles(ActivePanel, dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x,
                                      file_window
				    );
            if (GlobalView->preview_mode) { preview_line_offset = 0; UpdatePreview(dir_entry); }
		      break;

      case ACTION_HOME : Nav_Home(&dir_entry->cursor_pos, &dir_entry->start_file);

			DisplayFiles(ActivePanel, dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x,
                                      file_window
				    );
            if (GlobalView->preview_mode) { preview_line_offset = 0; UpdatePreview(dir_entry); }
		      break;

      case ACTION_TOGGLE_HIDDEN:      {
                        ToggleDotFiles(ActivePanel);

                        /* Update current dir pointer using the new accessor function */
                        dir_entry = GetSelectedDirEntry(CurrentVolume);

                        /* Explicitly update the file window (preview) */
                        DisplayFileWindow(ActivePanel, dir_entry);
                        RefreshWindow(file_window);
                        if (GlobalView->preview_mode) { preview_line_offset = 0; UpdatePreview(dir_entry); }

                        need_dsp_help = TRUE;
                     }
		     break;

      case ACTION_CMD_A :      fe_ptr = ActivePanel->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;

	              need_dsp_help = TRUE;

		      if( !ChangeFileModus( fe_ptr ) )
		      {
			DisplayFiles(ActivePanel, dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x,
                                      file_window
			            );
		      }
		      break;

      case ACTION_CMD_TAGGED_A :
		      if( (mode != DISK_MODE && mode != USER_MODE) || !IsMatchingTaggedFiles() )
		      {
		      }
		      else
		      {
			need_dsp_help = TRUE;

		   	mask = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

			(void) GetAttributes( mask, modus );

            /* Updated to use InputString instead of GetNewFileModus */
            ClearHelp();
            MvAddStr( Y_PROMPT, 1, "ATTRIBUTES:" );

            if( InputString( modus, Y_PROMPT, 12, 0, 10, "\r\033", HST_CHANGE_MODUS ) == CR )
			{
			  (void) strcpy( walking_package.function_data.change_modus.new_modus,
					 modus
				       );
                          WalkTaggedFiles( dir_entry->start_file,
					   dir_entry->cursor_pos,
					   SetFileModus,
					   &walking_package
					 );

			  DisplayFiles(ActivePanel, dir_entry,
					dir_entry->start_file,
					dir_entry->start_file + dir_entry->cursor_pos,
					start_x,
                                        file_window
				      );
			}
            move( LINES - 2, 1 ); clrtoeol(); /* Cleanup prompt line */
		      }
		      break;

      case ACTION_CMD_O :      fe_ptr = ActivePanel->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;

		      need_dsp_help = TRUE;

		      if( !ChangeFileOwner( fe_ptr ) )
		      {
			DisplayFiles(ActivePanel, dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x,
                                      file_window
			            );
		      }
		      break;

      case ACTION_CMD_TAGGED_O :
		      if(( mode != DISK_MODE && mode != USER_MODE) || !IsMatchingTaggedFiles() )
		      {
		      }
		      else
		      {
			need_dsp_help = TRUE;
		        if( ( owner_id = GetNewOwner( -1 ) ) >= 0 )
			{
			  walking_package.function_data.change_owner.new_owner_id = owner_id;
                          WalkTaggedFiles( dir_entry->start_file,
					   dir_entry->cursor_pos,
					   SetFileOwner,
					   &walking_package
					 );

			  DisplayFiles(ActivePanel, dir_entry,
					dir_entry->start_file,
					dir_entry->start_file + dir_entry->cursor_pos,
					start_x,
                                        file_window
				      );
			}
		      }
		      break;

      case ACTION_CMD_G :      fe_ptr = ActivePanel->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;

		      need_dsp_help = TRUE;

		      if( !ChangeFileGroup( fe_ptr ) )
		      {
			DisplayFiles(ActivePanel, dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x,
                                      file_window
			            );
		      }
		      break;

      case ACTION_CMD_TAGGED_G :
		      if(( mode != DISK_MODE && mode != USER_MODE) || !IsMatchingTaggedFiles() )
		      {
		      }
		      else
		      {
			need_dsp_help = TRUE;

		        if( ( group_id = GetNewGroup( -1 ) ) >= 0 )
			{
			  walking_package.function_data.change_group.new_group_id = group_id;
                          WalkTaggedFiles( dir_entry->start_file,
					   dir_entry->cursor_pos,
					   SetFileGroup,
					   &walking_package
					 );

			  DisplayFiles(ActivePanel, dir_entry,
					dir_entry->start_file,
					dir_entry->start_file + dir_entry->cursor_pos,
					start_x,
                                        file_window
				      );
			}
		      }
		      break;

      case ACTION_TAG :      fe_ptr = ActivePanel->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;

		      if( !fe_ptr->tagged )
		      {
                        fe_ptr->tagged = TRUE;
	       	        de_ptr->tagged_files++;
		        de_ptr->tagged_bytes += fe_ptr->stat_struct.st_size;
	       	        s->disk_tagged_files++;
		        s->disk_tagged_bytes += fe_ptr->stat_struct.st_size;
		      }
              DisplayFiles(ActivePanel, dir_entry, dir_entry->start_file, dir_entry->start_file + dir_entry->cursor_pos, start_x, file_window);
              DisplayDiskStatistic(s); /* Always update global disk stats */
              DisplayDirStatistic(dir_entry, NULL, s); /* Always update current list stats (even in Showall) */
		      unput_char = KEY_DOWN;

                      break;
      case ACTION_UNTAG :      fe_ptr = ActivePanel->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;
                      if( fe_ptr->tagged )
		      {
			fe_ptr->tagged = FALSE;

			de_ptr->tagged_files--;
			de_ptr->tagged_bytes -= fe_ptr->stat_struct.st_size;
			s->disk_tagged_files--;
			s->disk_tagged_bytes -= fe_ptr->stat_struct.st_size;
		      }
              DisplayFiles(ActivePanel, dir_entry, dir_entry->start_file, dir_entry->start_file + dir_entry->cursor_pos, start_x, file_window);
              DisplayDiskStatistic(s); /* Always update global disk stats */
              DisplayDirStatistic(dir_entry, NULL, s); /* Always update current list stats (even in Showall) */
		      unput_char = KEY_DOWN;

		      break;

      case ACTION_TOGGLE_MODE :
              if (GlobalView->preview_mode) { beep(); break; }
		      list_pos = dir_entry->start_file + dir_entry->cursor_pos;

		      RotateFileMode();
              x_step =  (GetMaxColumn() > 1) ? getmaxy(file_window) : 1;
              max_disp_files = getmaxy(file_window) * GetMaxColumn();

		      if( dir_entry->cursor_pos >= max_disp_files )
		      {
			/* Cursor must be repositioned */
			/*-------------------------------------*/

                        dir_entry->cursor_pos = max_disp_files - 1;
		      }

		      dir_entry->start_file = list_pos - dir_entry->cursor_pos;
		      DisplayFiles(ActivePanel, dir_entry,
				    dir_entry->start_file,
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x,
                                    file_window
			          );
		      break;

      case ACTION_TAG_ALL :
                      for(i=0; i < (int)ActivePanel->file_count; i++)
                      {
			fe_ptr = ActivePanel->file_entry_list[i].file;
			de_ptr = fe_ptr->dir_entry;

			if( !fe_ptr->tagged )
			{
			  file_size = fe_ptr->stat_struct.st_size;

			  fe_ptr->tagged = TRUE;
			  de_ptr->tagged_files++;
			  de_ptr->tagged_bytes += file_size;
			  s->disk_tagged_files++;
			  s->disk_tagged_bytes += file_size;
		        }
		      }

		      DisplayFiles(ActivePanel, dir_entry,
				    dir_entry->start_file,
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x,
                                    file_window
			          );
              DisplayDiskStatistic(s); /* Always update global disk stats */
              DisplayDirStatistic(dir_entry, NULL, s); /* Always update current list stats (even in Showall) */
		      break;


      case ACTION_UNTAG_ALL :
                      for(i=0; i < (int)ActivePanel->file_count; i++)
                      {
			fe_ptr = ActivePanel->file_entry_list[i].file;
			de_ptr = fe_ptr->dir_entry;

			if( fe_ptr->tagged )
			{
			  file_size = fe_ptr->stat_struct.st_size;

			  fe_ptr->tagged = FALSE;
			  de_ptr->tagged_files--;
			  de_ptr->tagged_bytes -= file_size;
			  s->disk_tagged_files--;
			  s->disk_tagged_bytes -= file_size;
		        }
		      }

		      DisplayFiles(ActivePanel, dir_entry,
				    dir_entry->start_file,
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x,
                                    file_window
			          );
              DisplayDiskStatistic(s); /* Always update global disk stats */
              DisplayDirStatistic(dir_entry, NULL, s); /* Always update current list stats (even in Showall) */
		      break;



      case ACTION_TAG_REST :
                      for(i=dir_entry->start_file + dir_entry->cursor_pos; i < (int)ActivePanel->file_count; i++)
                      {
			fe_ptr = ActivePanel->file_entry_list[i].file;
			de_ptr = fe_ptr->dir_entry;

			if( !fe_ptr->tagged )
			{
			  file_size = fe_ptr->stat_struct.st_size;

			  fe_ptr->tagged = TRUE;
			  de_ptr->tagged_files++;
			  de_ptr->tagged_bytes += file_size;
			  s->disk_tagged_files++;
			  s->disk_tagged_bytes += file_size;
		        }
		      }

		      DisplayFiles(ActivePanel, dir_entry,
				    dir_entry->start_file,
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x,
                                    file_window
			          );
              DisplayDiskStatistic(s); /* Always update global disk stats */
              DisplayDirStatistic(dir_entry, NULL, s); /* Always update current list stats (even in Showall) */
		      break;


      case ACTION_UNTAG_REST :
                      for(i=dir_entry->start_file + dir_entry->cursor_pos; i < (int)ActivePanel->file_count; i++)
                      {
			fe_ptr = ActivePanel->file_entry_list[i].file;
			de_ptr = fe_ptr->dir_entry;

			if( fe_ptr->tagged )
			{
			  file_size = fe_ptr->stat_struct.st_size;

			  fe_ptr->tagged = FALSE;
			  de_ptr->tagged_files--;
			  de_ptr->tagged_bytes -= file_size;
			  s->disk_tagged_files--;
			  s->disk_tagged_bytes -= file_size;
		        }
		      }

		      DisplayFiles(ActivePanel, dir_entry,
				    dir_entry->start_file,
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x,
                                    file_window
			          );
              DisplayDiskStatistic(s); /* Always update global disk stats */
              DisplayDirStatistic(dir_entry, NULL, s); /* Always update current list stats (even in Showall) */
		      break;

      case ACTION_CMD_V :      fe_ptr = ActivePanel->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;
		      (void) GetRealFileNamePath( fe_ptr, filepath );
		      (void) View( dir_entry, filepath );
		      need_dsp_help = TRUE;
		      break;

      case ACTION_CMD_TAGGED_V:
              if (!IsMatchingTaggedFiles())
              {
                  /* STRICT FILTER MODE: No tags = no action */
              }
              else
              {
                  ViewTaggedFiles(dir_entry);
                  need_dsp_help = TRUE;
              }
              break;

      case ACTION_CMD_H :      fe_ptr = ActivePanel->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;
		      (void) GetRealFileNamePath( fe_ptr, filepath );
		      (void) ViewHex( filepath );
		      need_dsp_help = TRUE;
		      break;

      case ACTION_CMD_E :      fe_ptr = ActivePanel->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;
		      (void) GetFileNamePath( fe_ptr, filepath );
		      (void) Edit( de_ptr, filepath );
		      break;

      case ACTION_CMD_Y :
      case ACTION_CMD_C :
		      fe_ptr = ActivePanel->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;

		      path_copy = FALSE;
		      if( action == ACTION_CMD_Y ) path_copy = TRUE;

		      need_dsp_help = TRUE;

		      if( GetCopyParameter( fe_ptr->name, path_copy, to_file, to_dir ) )
                      {
			break;
		      }

              if (mode != DISK_MODE && mode != USER_MODE) {
                  if (realpath(to_dir, to_path) == NULL) {
                      if (errno == ENOENT) {
                          strcpy(to_path, to_dir);
                      } else {
                          MESSAGE("Invalid destination path*\"%s\"*%s", to_dir, strerror(errno));
                          break;
                      }
                  }
                  dest_dir_entry = NULL;
              }
              else
              {
                  get_dir_ret = GetDirEntry( s->tree, de_ptr, to_dir, &dest_dir_entry, to_path );
                  if (get_dir_ret == -1) { /* System error */
                      break;
                  }
                  if (get_dir_ret == -3) { /* Directory not found, proceed */
                      dest_dir_entry = NULL;
                  }
              }

              /* EXPAND WILDCARDS FOR SINGLE FILE COPY */
              BuildFilename(fe_ptr->name, to_file, expanded_to_file);

              {
                  int dir_create_mode = 0; /* Local mode for single file op */
                  int overwrite_mode = 0; /* Local mode for single file op */
                  CopyFile( s, fe_ptr, expanded_to_file, dest_dir_entry, to_path, path_copy, &dir_create_mode, &overwrite_mode, UI_AskConflict );
              }

              RefreshGlobalView(dir_entry);

		      need_dsp_help = TRUE;
		      break;

      case ACTION_CMD_TAGGED_Y :
      case ACTION_CMD_TAGGED_C :
		      de_ptr = dir_entry;

                      path_copy = FALSE;
                      if( action == ACTION_CMD_TAGGED_Y ) path_copy = TRUE;

		      if( !IsMatchingTaggedFiles() )
		      {
		      }
		      else
		      {
		        need_dsp_help = TRUE;

			if( GetCopyParameter( NULL, path_copy, to_file, to_dir ) )
                        {
			  break;
		        }


                if (mode != DISK_MODE && mode != USER_MODE) {
                    if (realpath(to_dir, to_path) == NULL) {
                         if (errno == ENOENT) {
                             strcpy(to_path, to_dir);
                         } else {
                             MESSAGE("Invalid destination path*\"%s\"*%s", to_dir, strerror(errno));
                             break;
                         }
                    }
                    dest_dir_entry = NULL;
                } else {
                    get_dir_ret = GetDirEntry( s->tree, de_ptr, to_dir, &dest_dir_entry, to_path );
                    if (get_dir_ret == -1) { /* System error */
                        break;
                    }
                    if (get_dir_ret == -3) { /* Directory not found, proceed */
                        dest_dir_entry = NULL;
                    }
                }

			  term = InputChoice( "Ask for confirmation for each overwrite (Y/N) ? ", "YN\033" );
                          if( term == ESC )
		          {
			    break;
			  }

			  walking_package.function_data.copy.statistic_ptr  = s;
			  walking_package.function_data.copy.dest_dir_entry = dest_dir_entry;
			  walking_package.function_data.copy.to_file        = to_file;
			  walking_package.function_data.copy.to_path        = to_path; /* Fixed struct access */
			  walking_package.function_data.copy.path_copy      = path_copy; /* Fixed struct access */
              walking_package.function_data.copy.conflict_cb    = (void*)UI_AskConflict;
              walking_package.function_data.copy.dir_create_mode = 0; /* Reset auto-create mode */
              walking_package.function_data.copy.overwrite_mode = (term == 'N') ? 1 : 0; /* Reset overwrite mode */

			  WalkTaggedFiles( dir_entry->start_file,
					   dir_entry->cursor_pos,
					   CopyTaggedFiles,
					   &walking_package
				         );

                          RefreshGlobalView(dir_entry);
		      }
		      need_dsp_help = TRUE;
		      break;

      case ACTION_CMD_M :      if( mode != DISK_MODE && mode != USER_MODE )
                      {
			break;
		      }

		      fe_ptr = ActivePanel->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;

		      need_dsp_help = TRUE;

		      if( GetMoveParameter( fe_ptr->name, to_file, to_dir ) )
                      {
			break;
		      }

                      get_dir_ret = GetDirEntry( s->tree, de_ptr, to_dir, &dest_dir_entry, to_path );
                      if (get_dir_ret == -1) {
                          break;
                      }
                      if (get_dir_ret == -3) {
                          dest_dir_entry = NULL;
                      }

                      /* Construct absolute path for checking */
                      {
                          char abs_check_path[PATH_LENGTH * 2 + 2];
                          char current_dir[PATH_LENGTH + 1];
                          BOOL created = FALSE;
                          int dir_create_mode = 0;

                          if (*to_dir == FILE_SEPARATOR_CHAR) {
                              strcpy(abs_check_path, to_dir);
                          } else {
                              GetPath(de_ptr, current_dir);
                              snprintf(abs_check_path, sizeof(abs_check_path), "%s%c%s", current_dir, FILE_SEPARATOR_CHAR, to_dir);
                          }
                          /* FIX: Pass &dest_dir_entry */
                          if (EnsureDirectoryExists(abs_check_path, s->tree, &created, &dest_dir_entry, &dir_create_mode) == -1) break;
                      }

                      /* EXPAND WILDCARDS FOR SINGLE FILE MOVE */
                      BuildFilename(fe_ptr->name, to_file, expanded_to_file);

                      {
                          int dir_create_mode = 0;
                          int overwrite_mode = 0;
		      if( !MoveFile( fe_ptr,
				     expanded_to_file,
				     dest_dir_entry,
				     to_path,
				     &new_fe_ptr,
                     &dir_create_mode,
                     &overwrite_mode,
                     UI_AskConflict
				   ) )
		      {
			/* File was moved */
			/*-------------------*/

			/* ... Stats updates ... */
			/* ... BuildFileEntryList ... */

            RefreshGlobalView(dir_entry);
            maybe_change_x_step = TRUE;
		      }
              }
		      need_dsp_help = TRUE;
		      break;

      case ACTION_CMD_TAGGED_M :
		      if(( mode != DISK_MODE && mode != USER_MODE) || !IsMatchingTaggedFiles() )
		      {
		      }
		      else
		      {
		        need_dsp_help = TRUE;

			if( GetMoveParameter( NULL, to_file, to_dir ) )
                        {
			  break;
		        }


                        get_dir_ret = GetDirEntry( s->tree, de_ptr, to_dir, &dest_dir_entry, to_path );
                        if (get_dir_ret == -1) {
                            break;
                        }
                        if (get_dir_ret == -3) {
                            dest_dir_entry = NULL;
                        }

                        /* Construct absolute path for checking */
                        {
                            char abs_check_path[PATH_LENGTH * 2 + 2];
                            char current_dir[PATH_LENGTH + 1];
                            BOOL created = FALSE;
                            int dir_create_mode = 0;

                            if (*to_dir == FILE_SEPARATOR_CHAR) {
                                strcpy(abs_check_path, to_dir);
                            } else {
                                GetPath(dir_entry, current_dir);
                                snprintf(abs_check_path, sizeof(abs_check_path), "%s%c%s", current_dir, FILE_SEPARATOR_CHAR, to_dir);
                            }
                            /* FIX: Pass &dest_dir_entry */
                            if (EnsureDirectoryExists(abs_check_path, s->tree, &created, &dest_dir_entry, &dir_create_mode) == -1) break;
                        }

			term = InputChoice( "Ask for confirmation for each overwrite (Y/N) ? ", "YN\033" );
                        if( term == ESC )
		        {
			  break;
			}

			walking_package.function_data.mv.dest_dir_entry = dest_dir_entry;
			walking_package.function_data.mv.to_file = to_file;
			walking_package.function_data.mv.to_path = to_path;
            walking_package.function_data.mv.conflict_cb = (void*)UI_AskConflict;
            walking_package.function_data.mv.dir_create_mode = 0; /* Reset auto-create mode */
            walking_package.function_data.mv.overwrite_mode = (term == 'N') ? 1 : 0; /* Reset overwrite mode */

			WalkTaggedFiles( dir_entry->start_file,
					 dir_entry->cursor_pos,
					 MoveTaggedFiles,
					 &walking_package
				       );

			BuildFileEntryList( ActivePanel );

			if( ActivePanel->file_count == 0 ) unput_char = ESC;

			dir_entry->start_file = 0;
			dir_entry->cursor_pos = 0;

            RefreshGlobalView(dir_entry);
			maybe_change_x_step = TRUE;
		      }
		      break;

      case ACTION_CMD_D :      if( mode != DISK_MODE && mode != USER_MODE && mode != ARCHIVE_MODE )
		      {
			break;
		      }

		      term = InputChoice( "Delete this file (Y/N) ? ",
					  "YN\033"
					);

		      need_dsp_help = TRUE;

		      if( term != 'Y' ) break;

		      fe_ptr = ActivePanel->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;

              {
                  int override_mode = 0;
		      if( !DeleteFile( fe_ptr, &override_mode, s ) )
		      {
		        /* File was deleted */
			/*----------------------*/

            RefreshGlobalView(dir_entry);
			maybe_change_x_step = TRUE;
		      }
              }
                      break;

      case ACTION_CMD_TAGGED_D :
		      if(( mode != DISK_MODE && mode != USER_MODE && mode != ARCHIVE_MODE) || !IsMatchingTaggedFiles() )
		      {
		      }
		      else
		      {
		        need_dsp_help = TRUE;
			(void) DeleteTaggedFiles( max_disp_files, s );
			/* ... */

            RefreshGlobalView(dir_entry);
			maybe_change_x_step = TRUE;
		      }
		      break;

      case ACTION_CMD_R :      if( mode != DISK_MODE && mode != USER_MODE && mode != ARCHIVE_MODE )
		      {
			break;
		      }

		      fe_ptr = ActivePanel->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;

		      if( !GetRenameParameter( fe_ptr->name, new_name ) )
		      {
            /* EXPAND WILDCARDS FOR SINGLE FILE RENAME */
            BuildFilename(fe_ptr->name, new_name, expanded_new_name);

			if( !RenameFile( fe_ptr, expanded_new_name, &new_fe_ptr ) )
		        {
			  /* Rename OK */
			  /*-----------*/

              RefreshGlobalView(dir_entry);
			  maybe_change_x_step = TRUE;
                        }
		      }
		      need_dsp_help = TRUE;
		      break;

      case ACTION_CMD_TAGGED_R :
		      if(( mode != DISK_MODE && mode != USER_MODE && mode != ARCHIVE_MODE) || !IsMatchingTaggedFiles() )
		      {
		      }
		      else
		      {
			need_dsp_help = TRUE;

			if( GetRenameParameter( NULL, new_name ) )
                        {
			  break;
		        }

			walking_package.function_data.rename.new_name = new_name;
			walking_package.function_data.rename.confirm  = FALSE;

			WalkTaggedFiles( dir_entry->start_file,
					 dir_entry->cursor_pos,
					 RenameTaggedFiles,
					 &walking_package
				       );

			BuildFileEntryList( ActivePanel );

            RefreshGlobalView(dir_entry);
			maybe_change_x_step = TRUE;
		      }
		      break;

      case ACTION_CMD_S :       GetKindOfSort();

		      dir_entry->start_file = 0;
		      dir_entry->cursor_pos = 0;

		      Panel_Sort(ActivePanel, s->kind_of_sort);

		      DisplayFiles(ActivePanel, dir_entry,
				    dir_entry->start_file,
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x,
                                    file_window
			          );
		      need_dsp_help = TRUE;
		      break;

      case ACTION_FILTER :       if(ReadFilter() == 0) {

		        dir_entry->start_file = 0;
		        dir_entry->cursor_pos = 0;

		        BuildFileEntryList( ActivePanel );

		        DisplayFilter(s);
		        DisplayFiles(ActivePanel, dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x,
                                      file_window
			            );

		        if( dir_entry->global_flag )
		          DisplayDiskStatistic(s);
		        else
		          DisplayDirStatistic( dir_entry, NULL, s ); /* Updated call */

                        if( ActivePanel->file_count == 0 ) unput_char = ESC;
		        maybe_change_x_step = TRUE;
	              }
		      need_dsp_help = TRUE;
		      break;

      case ACTION_LOGIN :      fe_ptr = ActivePanel->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		     if( mode == DISK_MODE || mode == USER_MODE )
		     {
		       (void) GetFileNamePath( fe_ptr, new_login_path );
                       if( !GetNewLoginPath( new_login_path ) )
		       {
			 dir_entry->login_flag  = TRUE;

		         (void)  LogDisk( new_login_path );
		         unput_char = ESC;
			}
		        need_dsp_help = TRUE;
		     }
		     break;

      case ACTION_ENTER :
             if (GlobalView->preview_mode) {
                 action = ACTION_NONE;
                 break;
             }
             /* Toggle Big Window */
             if( dir_entry->big_window ) break; /* Exit loop */

             dir_entry->big_window = TRUE;
             ch = '\0';

             /* Use Global Refresh for clean transition */
             RefreshGlobalView(dir_entry);

             /* Update scrolling metrics for new size */
             RereadWindowSize(dir_entry);

             action = ACTION_NONE; /* Prevent loop termination to stay in file window */
		      break;

      case ACTION_CMD_P :      fe_ptr = ActivePanel->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;
		      (void) Pipe( de_ptr, fe_ptr );
              RefreshGlobalView(dir_entry);
		      need_dsp_help = TRUE;
		      break;

      case ACTION_CMD_TAGGED_P :
		      de_ptr = dir_entry;

		      if( !IsMatchingTaggedFiles() )
		      {
		      }
		      else if( mode != DISK_MODE && mode != USER_MODE )
		      {
			MESSAGE( "^P is not available in archive mode" );
		      }
		      else
		      {
		        need_dsp_help = TRUE;

                        filepath[0] = '\0'; /* Initialize buffer to prevent garbage prompt */
			if( GetPipeCommand( filepath ) )
                        {
			  break;
			}

                        /* Exit ncurses mode */
                        endwin();
                        SuspendClock();

			if( ( walking_package.function_data.pipe_cmd.pipe_file =
			      popen( filepath, "w" ) ) == NULL )
			{
                          /* Restore ncurses mode if popen fails */
                          InitClock();
                          clearok( stdscr, TRUE );
                          refresh();
			  MESSAGE( "execution of command*%s*failed", filepath );
			  break;
			}


			SilentWalkTaggedFiles( PipeTaggedFiles,
					 &walking_package
				       );

                        /* Close pipe and capture return value */
			pclose_ret = pclose( walking_package.function_data.pipe_cmd.pipe_file ); /* Fixed struct access */

                        /* Wait for user input */
                        HitReturnToContinue();

                        /* Restore ncurses mode */
                        InitClock();
		        clearok( stdscr, TRUE );
                        refresh(); /* Restore screen */

			if( pclose_ret )
			{
			  WARNING( "pclose failed" );
			}

            RefreshGlobalView(dir_entry);
		      }
		      break;

      case ACTION_CMD_X :      fe_ptr = ActivePanel->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;
		      (void) Execute( de_ptr, fe_ptr, &ActivePanel->vol->vol_stats );
              dir_entry = RefreshFileView(dir_entry);

              /* Insert: Explicit Global Refresh to be safe */
              RefreshGlobalView(dir_entry);
		      need_dsp_help = TRUE;
		      break;

      case ACTION_CMD_TAGGED_S :
                      /* STRICT FILTER MODE: Only allow if tags exist */
                      if( !IsMatchingTaggedFiles() )
                      {
                        /* If no tags, this command does nothing (or shows error) */
                        MESSAGE( "No tagged files" );
                      }
		      else if( mode != DISK_MODE && mode != USER_MODE && mode != ARCHIVE_MODE )
		      {
			MESSAGE( "^S is not available in archive mode" );
		      }
		      else
		      {
			char *command_line;

			if( ( command_line = (char *)malloc( COLS + 1 ) ) == NULL )
			{
			  ERROR_MSG( "Malloc failed*ABORT" );
			  exit( 1 );
			}

            /* Allocate new buffer for silent command */
            char *silent_cmd = (char *)malloc(COLS + 20);
            if (!silent_cmd) {
                ERROR_MSG( "Malloc failed*ABORT" );
                exit( 1 );
            }

			need_dsp_help = TRUE;
			*command_line = '\0';

            /* Filter Mode */
		        if( !GetSearchCommandLine( command_line, GlobalSearchTerm ) )
			    {
                  /* Construct Silent Command */
                  sprintf(silent_cmd, "%s > /dev/null 2>&1", command_line);

			      walking_package.function_data.execute.command = silent_cmd;
                              /* Use modified SilentTagWalk (Untag on Fail) */
                              SilentTagWalkTaggedFiles( ExecuteCommand,
					            &walking_package
					          );
			      /* No HitReturnToContinue - purely silent */
			    }

            /* Refresh Display */
            DisplayFiles(ActivePanel, dir_entry,
                  dir_entry->start_file,
                  dir_entry->start_file + dir_entry->cursor_pos,
                  start_x,
                  file_window
                );
            DisplayDiskStatistic(s);
            if( dir_entry->global_flag )
                DisplayDiskStatistic(s);
            else
                DisplayDirStatistic(dir_entry, NULL, s);

			free( command_line );
            free( silent_cmd );
		      }
		      break;

      case ACTION_CMD_TAGGED_X:
		      if( !IsMatchingTaggedFiles() )
		      {
		      }
		      else if( mode != DISK_MODE && mode != USER_MODE && mode != ARCHIVE_MODE )
		      {
			MESSAGE( "^X is not available in archive mode" );
		      }
		      else
		      {
			char *command_line;

			if( ( command_line = (char *)malloc( COLS + 1 ) ) == NULL )
			{
			  ERROR_MSG( "Malloc failed*ABORT" );
			  exit( 1 );
			}

			need_dsp_help = TRUE;
			*command_line = '\0';
		        if( !GetCommandLine( command_line ) )
			{
			  refresh();
			  endwin();
			  walking_package.function_data.execute.command = command_line;
                          SilentWalkTaggedFiles( ExecuteCommand,
					         &walking_package
					       );
			  HitReturnToContinue();

              dir_entry = RefreshFileView(dir_entry);

              /* Insert: Explicit Global Refresh */
              RefreshGlobalView(dir_entry);
			}
			free( command_line );
		      }
		      break;

      case ACTION_QUIT_DIR:
                      need_dsp_help = TRUE;
                      fe_ptr = ActivePanel->file_entry_list[dir_entry->start_file
                                + dir_entry->cursor_pos].file;
                      de_ptr = fe_ptr->dir_entry;
                      QuitTo(de_ptr);
                      break;

      case ACTION_QUIT:       need_dsp_help = TRUE;
                      Quit();
                      action = ACTION_NONE;
		      break;

      case ACTION_VOL_MENU:
          /* Save Directory Tree State before switching */
          ActivePanel->vol->vol_stats.cursor_pos = ActivePanel->cursor_pos;
          ActivePanel->vol->vol_stats.disp_begin_pos = ActivePanel->disp_begin_pos;

          if (SelectLoadedVolume() == 0) {
              unput_char = '\0'; /* Ensure we return clean ESC, no pending input */
              return ESC;
          } else {
              ch = 0;
          }
          if (CurrentVolume != start_vol) return ESC;
          break;
      case ACTION_VOL_PREV:
          /* Save Directory Tree State before switching */
          ActivePanel->vol->vol_stats.cursor_pos = ActivePanel->cursor_pos;
          ActivePanel->vol->vol_stats.disp_begin_pos = ActivePanel->disp_begin_pos;

          if (CycleLoadedVolume(-1) == 0) {
              unput_char = '\0'; /* Ensure we return clean ESC, no pending input */
              return ESC;
          } else {
              ch = 0;
          }
          if (CurrentVolume != start_vol) return ESC;
          break;
      case ACTION_VOL_NEXT:
          /* Save Directory Tree State before switching */
          ActivePanel->vol->vol_stats.cursor_pos = ActivePanel->cursor_pos;
          ActivePanel->vol->vol_stats.disp_begin_pos = ActivePanel->disp_begin_pos;

          if (CycleLoadedVolume(1) == 0) {
              unput_char = '\0'; /* Ensure we return clean ESC, no pending input */
              return ESC;
          } else {
              ch = 0;
          }
          if (CurrentVolume != start_vol) return ESC;
          break;

      case ACTION_REFRESH:
                {
                    dir_entry = RefreshFileView(dir_entry);
                    need_dsp_help = TRUE;
                }
                break;

      case ACTION_RESIZE: resize_request = TRUE; break;

      case ACTION_TOGGLE_STATS: /* ADDED */
               GlobalView->show_stats = !GlobalView->show_stats;
               resize_request = TRUE;
               break;

      case ACTION_TOGGLE_COMPACT: /* ADDED */
               GlobalView->fixed_col_width = (GlobalView->fixed_col_width == 0) ? 32 : 0;
               resize_request = TRUE;
               break;

      case ACTION_ESCAPE:    break; /* Handled by loop condition */

      case ACTION_LIST_JUMP:
      		      ListJump(dir_entry, "");
		      need_dsp_help = TRUE;
		      break;

    case ACTION_TOGGLE_TAGGED_MODE:

	/* Toggle mode (if possible) */
        if(dir_entry->tagged_files)
          dir_entry->tagged_flag = !dir_entry->tagged_flag;
        else
          dir_entry->tagged_flag = FALSE;

        BuildFileEntryList ( ActivePanel );

        dir_entry->start_file = 0;
        dir_entry->cursor_pos = 0;
        DisplayFiles(ActivePanel,
            dir_entry,
            dir_entry->start_file,
            dir_entry->start_file + dir_entry->cursor_pos,
            start_x,
            file_window
        );
        maybe_change_x_step = TRUE;
        break;

    case ACTION_ASTERISK: /* Mapped to '*' for Invert Tags in File Window */
        HandleInvertTags(dir_entry, s);
        need_dsp_help = TRUE;
        break;

     default:
                      break;
    } /* switch */
  } while( action != ACTION_QUIT && action != ACTION_ENTER && action != ACTION_ESCAPE && action != ACTION_QUIT );

  if( dir_entry->big_window ) {
    SwitchToSmallFileWindow();
    /* We don't need full refresh here because HandleDirWindow will catch the return */
  }

  if(action != ACTION_ESCAPE) {
    dir_entry->global_flag = FALSE;
    dir_entry->tagged_flag = FALSE;
    dir_entry->big_window  = FALSE;
  }

  return( (action == ACTION_ENTER) ? CR : ESC ); /* Return CR or ESC based on action */
}




static void WalkTaggedFiles(int start_file,
			    int cursor_pos,
			    int (*fkt) (FileEntry *, WalkingPackage *),
			    WalkingPackage *walking_package
			   )
{
  FileEntry *fe_ptr;
  int       i;
  int       start_x = 0;
  int       result = 0;
  BOOL      maybe_change_x = FALSE;
  int       height, width;

  if (baudrate() >= QUICK_BAUD_RATE) typeahead(0);

  GetMaxYX(file_window, &height, &width);

  max_disp_files = height * GetMaxColumn();

  for( i=0; i < (int)ActivePanel->file_count && result == 0; i++ )
  {
    fe_ptr = ActivePanel->file_entry_list[i].file;

    if( fe_ptr->tagged && fe_ptr->matching )
    {
      if( maybe_change_x == FALSE &&
	  i >= start_file && i < start_file + max_disp_files )
      {
	/* Walk possible without scrolling */
	/*---------------------------*/
    DisplayFiles(ActivePanel, fe_ptr->dir_entry,
            start_file,
            i,
            start_x,
            file_window
            );
        cursor_pos = i - start_file;
      }
      else
      {
	/* Scrolling necessary */
	/*---------------*/

	start_file = MAX( 0, i - max_disp_files + 1 );
	cursor_pos = i - start_file;

        DisplayFiles(ActivePanel, fe_ptr->dir_entry,
		      start_file,
		      start_file + cursor_pos,
		      start_x,
                      file_window
	            );
	maybe_change_x = FALSE;
      }

      if( fe_ptr->dir_entry->global_flag )
        DisplayGlobalFileParameter( fe_ptr );
      else
        DisplayFileParameter( fe_ptr );

      RefreshWindow( file_window );
      doupdate();
      result = fkt( fe_ptr, walking_package );

      /* Handle case where file was removed/moved away */
      if( walking_package->new_fe_ptr == NULL ) {
          RemoveFileEntry(i);
          i--; /* Adjust index since we removed current element */
          maybe_change_x = TRUE;
      }
      else if( walking_package->new_fe_ptr != fe_ptr )
      {
        ActivePanel->file_entry_list[i].file = walking_package->new_fe_ptr;
	ChangeFileEntry();
        max_disp_files = height * GetMaxColumn();
	maybe_change_x = TRUE;
      }
    }
  }

  if (baudrate() >= QUICK_BAUD_RATE ) typeahead( -1 );
}

/*
 ExecuteCommand (*fkt) had its retval zeroed as found.
 ^S needs that value, so it was unzeroed. forloop below
 was modified to not care about retval instead?

 global flag for stop-on-error? does anybody want it?

 --crb3 12mar04
*/

static void SilentWalkTaggedFiles( int (*fkt) (FileEntry *, WalkingPackage *, Statistic *),
			           WalkingPackage *walking_package
			          )
{
  FileEntry *fe_ptr;
  int       i;


  for( i=0; i < (int)ActivePanel->file_count; i++ )
  {
    fe_ptr = ActivePanel->file_entry_list[i].file;

    if( fe_ptr->tagged && fe_ptr->matching )
    {
      (void)fkt( fe_ptr, walking_package, &ActivePanel->vol->vol_stats );
    }
  }
}

/*

SilentTagWalkTaggedFiles.
revision of above function to provide something like
XTG's <search> facility, using external grep.
- loops for entire filescount.
- if called program returns 1 (grep's "no-match" retcode), untags the file.
repeated calls can be used to pare down tags, each with a different
string, until only the intended target files are tagged.

ExecuteCommand must have its retval unzeroed.

--crb3 31dec03

*/

static void SilentTagWalkTaggedFiles( int (*fkt) (FileEntry *, WalkingPackage *, Statistic *),
			           WalkingPackage *walking_package
			          )
{
  FileEntry *fe_ptr;
  int       i;
  int       result = 0;


  for( i=0; i < (int)ActivePanel->file_count; i++ )
  {
    fe_ptr = ActivePanel->file_entry_list[i].file;

    if( fe_ptr->tagged && fe_ptr->matching )
    {
      result = fkt( fe_ptr, walking_package, &ActivePanel->vol->vol_stats );

      if( result != 0 ) {
      	fe_ptr->tagged = FALSE;
        /* Update Stats */
        fe_ptr->dir_entry->tagged_files--;
        fe_ptr->dir_entry->tagged_bytes -= fe_ptr->stat_struct.st_size;
        ActivePanel->vol->vol_stats.disk_tagged_files--;
        ActivePanel->vol->vol_stats.disk_tagged_bytes -= fe_ptr->stat_struct.st_size;
      }
    }
  }
}




static BOOL IsMatchingTaggedFiles(void)
{
  FileEntry *fe_ptr;
  int i;

  for( i=0; i < (int)ActivePanel->file_count; i++)
  {
    fe_ptr = ActivePanel->file_entry_list[i].file;

    if( fe_ptr->matching && fe_ptr->tagged )
      return( TRUE );
  }

  return( FALSE );
}


static int DeleteTaggedFiles(int max_disp_files, Statistic *s)
{
  FileEntry *fe_ptr;
  DirEntry  *de_ptr;
  int       i;
  int       start_file;
  int       cursor_pos;
  BOOL      deleted;
  BOOL      confirm_each = FALSE;
  int       term;
  int       start_x = 0;
  int       result = 0;
  int       tagged_count = 0;
  int       override_mode = 0; /* Auto-override mode for read-only files */
  char      prompt_buffer[MESSAGE_LENGTH];

  /* 1. Count tagged files */
  for(i=0; i < (int)ActivePanel->file_count; i++) {
      if(ActivePanel->file_entry_list[i].file->tagged && ActivePanel->file_entry_list[i].file->matching) {
          tagged_count++;
      }
  }

  if (tagged_count == 0) return 0;

  /* 2. Prompt for Intent */
  (void) snprintf(prompt_buffer, sizeof(prompt_buffer), "Delete %d tagged files (Y/N) ?", tagged_count);
  term = InputChoice(prompt_buffer, "YN\033");
  if (term != 'Y') return -1;

  /* 3. Prompt for Mode (Confirm Each?) */
  term = InputChoice( "Ask for confirmation for each file (Y/N) ? ", "YN\033" );
  if (term == ESC) return -1;
  confirm_each = (term == 'Y') ? TRUE : FALSE;
  if (!confirm_each) override_mode = 1;

  if( baudrate() >= QUICK_BAUD_RATE ) typeahead( 0 );

  for( i=0; i < (int)ActivePanel->file_count && result == 0; )
  {
    deleted = FALSE;

    fe_ptr = ActivePanel->file_entry_list[i].file;
    de_ptr = fe_ptr->dir_entry;

    if( fe_ptr->tagged && fe_ptr->matching )
    {
      /* Spinner to indicate progress during bulk deletion */
      DrawSpinner();
      doupdate();
      start_file = MAX( 0, i - max_disp_files + 1 );
      cursor_pos = i - start_file;

      DisplayFiles(ActivePanel, de_ptr,
		    start_file,
		    start_file + cursor_pos,
		    start_x,
                    file_window
	          );

      if( fe_ptr->dir_entry->global_flag )
        DisplayGlobalFileParameter( fe_ptr );
      else
        DisplayFileParameter( fe_ptr );

      RefreshWindow( file_window );
      doupdate();

      term = 'Y';
      if( confirm_each ) {
          term = InputChoice( "Delete this file (Y/N) ? ", "YN\033" );
      }

      if( term == ESC )
      {
        if( baudrate() >= QUICK_BAUD_RATE ) typeahead( -1 );
	result = -1;
	break;
      }

      if( term == 'Y' )
      {
        /* Pass the override mode pointer to suppress read-only prompts if 'A' was selected previously */
        if( ( result = DeleteFile( fe_ptr, &override_mode, s ) ) == 0 )
        {
	  /* File was deleted */
	  /*----------------------*/

	  deleted = TRUE;

  	  if( de_ptr->global_flag )
	    DisplayDiskStatistic(s);
	  else
	    DisplayDirStatistic( de_ptr, NULL, s ); /* Updated call */

	  DisplayAvailBytes(s);

          RemoveFileEntry( start_file + cursor_pos );
        }
      }
    }
    if( !deleted ) i++;
  }
  if( baudrate() >= QUICK_BAUD_RATE ) typeahead( -1 );

  return( result );
}




static void RereadWindowSize(DirEntry *dir_entry)
{
  int height, width;
  SetFileMode(GetFileMode());

  GetMaxYX(file_window, &height, &width);
  x_step =  (GetMaxColumn() > 1) ? height : 1;
  max_disp_files = height * GetMaxColumn();


  if( dir_entry->start_file + dir_entry->cursor_pos < (int)ActivePanel->file_count )
  {
     while( dir_entry->cursor_pos >= max_disp_files )
     {
         dir_entry->start_file += x_step;
         dir_entry->cursor_pos -= x_step;
     }
   }
   return;
}




static void ListJump( DirEntry * dir_entry, char *str )
{
    char search_buf[256];
    int buf_len = 0;
    int ch;
    int original_start_file = dir_entry->start_file;
    int original_cursor_pos = dir_entry->cursor_pos;
    int i;
    int found_idx;
    int start_x = 0;

    (void) str; /* Unused */

    memset(search_buf, 0, sizeof(search_buf));

    ClearHelp();
    MvAddStr(Y_PROMPT, 1, "Search: ");
    RefreshWindow(stdscr);

    while(1)
    {
        /* Update prompt */
        move( Y_PROMPT, 9 );
        addstr( search_buf );
        clrtoeol();
        refresh();

        ch = Getch();

        /* Handle Resize or Error */
        if (ch == -1) break;

        if (ch == ESC)
        {
            /* Cancel: Restore */
            dir_entry->start_file = original_start_file;
            dir_entry->cursor_pos = original_cursor_pos;
            DisplayFiles(ActivePanel, dir_entry, dir_entry->start_file, dir_entry->start_file + dir_entry->cursor_pos, start_x, file_window);
            break;
        }

        if (ch == CR || ch == LF)
        {
            /* Confirm: Keep new position */
            break;
        }

        if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b' || ch == KEY_DC)
        {
            if (buf_len > 0)
            {
                buf_len--;
                search_buf[buf_len] = '\0';

                /* Re-search or Restore */
                if (buf_len == 0)
                {
                    dir_entry->start_file = original_start_file;
                    dir_entry->cursor_pos = original_cursor_pos;
                }
                else
                {
                    /* Search again from top for the shorter string */
                     found_idx = -1;
                    for( i=0; i < (int)ActivePanel->file_count; i++ )
                    {
                        FileEntry *fe = ActivePanel->file_entry_list[i].file;
                        if( strncasecmp( fe->name, search_buf, buf_len ) == 0 )
                        {
                            found_idx = i;
                            break;
                        }
                    }

                    if (found_idx != -1)
                    {
                        if( found_idx >= dir_entry->start_file && found_idx < dir_entry->start_file + max_disp_files )
                        {
                            dir_entry->cursor_pos = found_idx - dir_entry->start_file;
                        }
                        else
                        {
                            dir_entry->start_file = found_idx;
                            dir_entry->cursor_pos = 0;
                        }
                    }
                }
                DisplayFiles(ActivePanel, dir_entry, dir_entry->start_file, dir_entry->start_file + dir_entry->cursor_pos, start_x, file_window );
                RefreshWindow( file_window );
            }
        }
        else if (isprint(ch))
        {
            if (buf_len < (int)sizeof(search_buf) - 1)
            {
                search_buf[buf_len] = ch;
                search_buf[buf_len+1] = '\0';

                found_idx = -1;
                for( i=0; i < (int)ActivePanel->file_count; i++ )
                {
                    FileEntry *fe = ActivePanel->file_entry_list[i].file;
                    if( strncasecmp( fe->name, search_buf, buf_len+1 ) == 0 )
                    {
                        found_idx = i;
                        break;
                    }
                }

                if (found_idx != -1)
                {
                    buf_len++; /* Accept char */
                    if( found_idx >= dir_entry->start_file && found_idx < dir_entry->start_file + max_disp_files )
                    {
                        dir_entry->cursor_pos = found_idx - dir_entry->start_file;
                    }
                    else
                    {
                        dir_entry->start_file = found_idx;
                        dir_entry->cursor_pos = 0;
                    }
                    DisplayFiles(ActivePanel, dir_entry, dir_entry->start_file, dir_entry->start_file + dir_entry->cursor_pos, start_x, file_window );
                    RefreshWindow( file_window );
                }
                else
                {
                    /* No match: Accept char to show user what they typed, but do not move cursor */
                    buf_len++;
                }
            }
        }
    }
}