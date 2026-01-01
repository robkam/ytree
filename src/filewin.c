/***************************************************************************
 *
 * filewin.c
 * Functions for handling the file window
 *
 ***************************************************************************/

#include "ytree.h"

#define MAX( a, b ) ( ( (a) > (b) ) ? (a) : (b) )

#if !defined(__NeXT__) && !defined(ultrix)
extern void qsort(void *, size_t, size_t, int (*) (const void *, const void *));
#endif /* __NeXT__ ultrix */

static BOOL reverse_sort;
static BOOL order;
static BOOL do_case = FALSE;
static int  file_mode;
static int  max_column;

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

/* --- Forward Declarations --- */
static void ReadFileList(BOOL tagged_only, DirEntry *dir_entry);
static void SortFileEntryList(Statistic *s);
static int  SortByName(FileEntryList *e1, FileEntryList *e2);
static int  SortByChgTime(FileEntryList *e1, FileEntryList *e2);
static int  SortByAccTime(FileEntryList *e1, FileEntryList *e2);
static int  SortByModTime(FileEntryList *e1, FileEntryList *e2);
static int  SortBySize(FileEntryList *e1, FileEntryList *e2);
static int  SortByOwner(FileEntryList *e1, FileEntryList *e2);
static int  SortByGroup(FileEntryList *e1, FileEntryList *e2);
static int  SortByExtension(FileEntryList *e1, FileEntryList *e2);
static void DisplayFiles(DirEntry *de_ptr, int start_file_no, int hilight_no, int start_x);
static void PrintFileEntry(int entry_no, int y, int x, unsigned char hilight, int start_x);
static void ReadGlobalFileList(BOOL tagged_only, DirEntry *dir_entry);
static void WalkTaggedFiles(int start_file, int cursor_pos, int (*fkt) (FileEntry *, WalkingPackage *), WalkingPackage *walking_package);
static BOOL IsMatchingTaggedFiles(void);
static void RemoveFileEntry(int entry_no);
static void ChangeFileEntry(void);
static int  DeleteTaggedFiles(int max_dispfiles, Statistic *s);
static void SilentWalkTaggedFiles( int (*fkt) (FileEntry *, WalkingPackage *), WalkingPackage *walking_package);
static void SilentTagWalkTaggedFiles( int (*fkt) (FileEntry *, WalkingPackage *), WalkingPackage *walking_package);
static void RereadWindowSize(DirEntry *dir_entry);
static void ListJump( DirEntry * dir_entry, char *str );
static char GetTypeOfFile(struct stat fst);
static int  GetVisualFileEntryLength(int p_mode, int max_visual_filename_len, int max_visual_linkname_len);
static void HandleInvertTags(DirEntry *dir_entry, Statistic *s);
static void RefreshFileView(DirEntry *dir_entry);
static int  SilentSearchWalk(FileEntry *fe_ptr, WalkingPackage *wp);
static void fmovedown(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry);
static void fmoveup(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry);
static void fmoveright(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry);
static void fmoveleft(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry);
static void fmovenpage(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry);
static void fmoveppage(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry);

void SetFileMode(int new_file_mode)
{
  int height, width;

  getmaxyx( file_window, height, width );
  file_mode = new_file_mode;

  max_column = width /
	       (GetVisualFileEntryLength( file_mode, max_visual_filename_len, max_visual_linkname_len) + 1);

  if( max_column == 0 )
    max_column = 1;
}


static int GetVisualFileEntryLength(int p_mode, int max_visual_filename_len, int max_visual_linkname_len)
{
  int len = 0;

  switch (p_mode)
  {
    case MODE_1: len =  (max_visual_linkname_len) ? max_visual_linkname_len + 4 : 0; /* linkname + " -> " */
		 len += max_visual_filename_len + 42; /* filename + format (increased by 4 for 16-char date) */
#ifdef HAS_LONGLONG
                 len += 4;  /* %11lld instead of %7d */
#endif
                 break;

    case MODE_2: len =  (max_visual_linkname_len) ? max_visual_linkname_len + 4 : 0; /* linkname + " -> " */
		 len += max_visual_filename_len + 40; /* filename + format */
#ifdef HAS_LONGLONG
                 len += 4;  /* %11lld instead of %7d */
#endif
                 break;

    case MODE_3: len = max_visual_filename_len + 2; /* filename + format */
                 break;

    case MODE_4: len =  (max_visual_linkname_len) ? max_visual_linkname_len + 4 : 0; /* linkname + " -> " */
		 len += max_visual_filename_len + 47; /* filename + format (increased by 8 for two 16-char dates) */
                 break;

    case MODE_5: len = GetVisualUserFileEntryLength(max_visual_filename_len, max_visual_linkname_len, USERVIEW);
		 max_visual_userview_len = len;
		 break;
  }

  return len;
}


void RotateFileMode(void)
{
  switch( file_mode )
  {
    case MODE_1: SetFileMode( MODE_3 ); break;
    case MODE_2: SetFileMode( MODE_5 ); break;
    case MODE_3: SetFileMode( MODE_4 ); break;
    case MODE_4: SetFileMode( MODE_2 ); break;
    case MODE_5: SetFileMode( MODE_1 ); break;
  }
  if( (mode != DISK_MODE && mode != USER_MODE) && file_mode == MODE_4 ) {
    RotateFileMode();
  } else if(file_mode == MODE_5 && !strcmp(USERVIEW, "")) {
    RotateFileMode();
  }
}


static void BuildFileEntryList(DirEntry *dir_entry, Statistic *s){
  size_t alloc_count;
  long t_files = 0;
  LONGLONG t_bytes = 0;
  size_t i; /* Loop counter for recalculating tagged stats */

  if (!CurrentVolume) return; /* Safety check */

  if( CurrentVolume->file_entry_list ) {
    free( CurrentVolume->file_entry_list );
    CurrentVolume->file_entry_list = NULL;
    CurrentVolume->file_entry_list_capacity = 0;
  }

  if( !dir_entry->global_flag )  {
    /* Normal Directory View */

    alloc_count = dir_entry->matching_files; /* This is the potentially stale value */
    if (alloc_count < 16) alloc_count = 16;

    if( ( CurrentVolume->file_entry_list = (FileEntryList *)
                  calloc( alloc_count,
                      sizeof( FileEntryList )
                    )
                              ) == NULL ) {
        ERROR_MSG( "Calloc Failed*ABORT" );
        exit( 1 );
    }
    CurrentVolume->file_entry_list_capacity = alloc_count;

    CurrentVolume->file_count = 0;
    ReadFileList( dir_entry->tagged_flag, dir_entry );
    SortFileEntryList(s);
    SetFileMode( file_mode ); /* recalc */

    /* Recalculate and update statistics based on the actual loaded list */
    dir_entry->matching_files = CurrentVolume->file_count;
    t_files = 0;
    t_bytes = 0;
    for (i = 0; i < CurrentVolume->file_count; i++) {
        if (CurrentVolume->file_entry_list[i].file->tagged) {
            t_files++;
            t_bytes += CurrentVolume->file_entry_list[i].file->stat_struct.st_size;
        }
    }
    dir_entry->tagged_files = t_files;
    dir_entry->tagged_bytes = t_bytes;

    } else {
    /* Global / ShowAll View */

    size_t count_source = (dir_entry->tagged_flag) ? s->disk_tagged_files : s->disk_matching_files;
    alloc_count = count_source; /* This is also potentially stale */
    if (alloc_count < 16) alloc_count = 16;

    if( ( CurrentVolume->file_entry_list = (FileEntryList *)
           calloc( alloc_count,
                      sizeof( FileEntryList )
              ) ) == NULL )  {
           ERROR_MSG( "Calloc Failed*ABORT" );
           exit( 1 );
    }
    CurrentVolume->file_entry_list_capacity = alloc_count;

    CurrentVolume->file_count = 0;
    global_max_visual_filename_len = 0;
    global_max_visual_linkname_len = 0;
    ReadGlobalFileList(  dir_entry->tagged_flag, s->tree );
    SortFileEntryList(s);
    SetFileMode( file_mode ); /* recalc */

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
      if (CurrentVolume->file_count >= CurrentVolume->file_entry_list_capacity) {
          size_t new_capacity = CurrentVolume->file_entry_list_capacity * 2;
          if (new_capacity == 0) new_capacity = 128;

          FileEntryList *new_list = (FileEntryList *) realloc(CurrentVolume->file_entry_list, new_capacity * sizeof(FileEntryList));
          if (!new_list) {
              ERROR_MSG("Realloc failed in ReadFileList*ABORT");
              exit(1);
          }
          memset(new_list + CurrentVolume->file_entry_list_capacity, 0, (new_capacity - CurrentVolume->file_entry_list_capacity) * sizeof(FileEntryList));
          CurrentVolume->file_entry_list = new_list;
          CurrentVolume->file_entry_list_capacity = new_capacity;
      }

      CurrentVolume->file_entry_list[CurrentVolume->file_count++].file = fe_ptr;
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




static void SortFileEntryList(Statistic *s)
{
  int aux;
  int (*compare)(FileEntryList *, FileEntryList *);

  reverse_sort = FALSE;
  if ((aux = s->kind_of_sort) > SORT_DSC)
  {
     order = FALSE;
     aux -= SORT_DSC;
  }
  else
  {
     order = TRUE;
     aux -= SORT_ASC;
  }
  switch( aux )
  {
    case SORT_BY_NAME :      compare = SortByName; break;
    case SORT_BY_MOD_TIME :  compare = SortByModTime; break;
    case SORT_BY_CHG_TIME :  compare = SortByChgTime; break;
    case SORT_BY_ACC_TIME :  compare = SortByAccTime; break;
    case SORT_BY_OWNER :     compare = SortByOwner; break;
    case SORT_BY_GROUP :     compare = SortByGroup; break;
    case SORT_BY_SIZE :      compare = SortBySize; break;
    case SORT_BY_EXTENSION : compare = SortByExtension; break;
    default:                 compare = SortByName;
  }

  qsort( (char *) CurrentVolume->file_entry_list,
	 CurrentVolume->file_count,
	 sizeof( CurrentVolume->file_entry_list[0] ),
	 (int (*)(const void *, const void *)) compare
	);
}




static int SortByName(FileEntryList *e1, FileEntryList *e2)
{
  if (do_case)
     if (order)
        return( strcmp( e1->file->name, e2->file->name ) );
     else
        return( - (strcmp( e1->file->name, e2->file->name ) ) );
  else
     if (order)
        return( strcasecmp( e1->file->name, e2->file->name ) );
     else
        return( - (strcasecmp( e1->file->name, e2->file->name ) ) );
}


static int SortByExtension(FileEntryList *e1, FileEntryList *e2)
{
  char *ext1, *ext2;
  int cmp, casecmp;

  /* Ok, this isn't optimized */

  ext1 = GetExtension(e1->file->name);
  ext2 = GetExtension(e2->file->name);
  cmp=strcmp( ext1, ext2 );
  casecmp=strcasecmp( ext1, ext2 );

  if (do_case && !cmp)
      return SortByName( e1, e2 );
  if (!do_case && !casecmp)
      return SortByName( e1, e2 );


  if (do_case)
     if (order)
        return( strcmp( ext1, ext2 ) );
     else
        return( - (strcmp( ext1, ext2 ) ) );
  else
     if (order)
        return( strcasecmp( ext1, ext2 ) );
     else
        return( - (strcasecmp( ext1, ext2 ) ) );
}


static int SortByModTime(FileEntryList *e1, FileEntryList *e2)
{
  if (order)
     return( e1->file->stat_struct.st_mtime - e2->file->stat_struct.st_mtime );
  else
     return( - (e1->file->stat_struct.st_mtime - e2->file->stat_struct.st_mtime ) );
}

static int SortByChgTime(FileEntryList *e1, FileEntryList *e2)
{
  if (order)
     return( e1->file->stat_struct.st_ctime - e2->file->stat_struct.st_ctime );
  else
     return( - (e1->file->stat_struct.st_ctime - e2->file->stat_struct.st_ctime ) );
}

static int SortByAccTime(FileEntryList *e1, FileEntryList *e2)
{
  if (order)
     return( e1->file->stat_struct.st_atime - e2->file->stat_struct.st_atime );
  else
     return( - (e1->file->stat_struct.st_atime - e2->file->stat_struct.st_atime ) );
}

static int SortBySize(FileEntryList *e1, FileEntryList *e2)
{
  int result = 0;

  if(e1->file->stat_struct.st_size < e2->file->stat_struct.st_size)
    result = 1;
  else if (e1->file->stat_struct.st_size > e2->file->stat_struct.st_size)
    result = -1;

  if(order)
    result *= -1;

  return result;
}


static int SortByOwner(FileEntryList *e1, FileEntryList *e2)
{
  char *o1, *o2;
  char n1[10], n2[10];

  o1 = GetPasswdName( e1->file->stat_struct.st_uid );
  o2 = GetPasswdName( e2->file->stat_struct.st_uid );

  if( o1 == NULL )
  {
    (void) snprintf( n1, sizeof(n1), "%d", (int) e1->file->stat_struct.st_uid );
    o1 = n1;
  }

  if( o2 == NULL )
  {
    (void) snprintf( n2, sizeof(n2), "%d", (int) e2->file->stat_struct.st_uid );
    o2 = n2;
  }
  if (do_case)
     if (order)
        return( strcmp( o1, o2 ) );
     else
        return( - (strcmp( o1, o2 ) ) );
  else
     if (order)
        return( strcasecmp( o1, o2 ) );
     else
        return( - (strcasecmp( o1, o2 ) ) );
}



static int SortByGroup(FileEntryList *e1, FileEntryList *e2)
{
  char *g1, *g2;
  char n1[10], n2[10];

  g1 = GetGroupName( e1->file->stat_struct.st_gid );
  g2 = GetGroupName( e2->file->stat_struct.st_gid );

  if( g1 == NULL )
  {
    (void) snprintf( n1, sizeof(n1), "%d", (int) e1->file->stat_struct.st_uid );
    g1 = n1;
  }

  if( g2 == NULL )
  {
    (void) snprintf( n2, sizeof(n2), "%d", (int) e2->file->stat_struct.st_uid );
    g2 = n2;
  }
  if (do_case)
     if (order)
        return( strcmp( g1, g2 ) );
     else
        return( - (strcmp( g1, g2 ) ) );
  else
     if (order)
        return( strcasecmp( g1, g2 ) );
     else
        return( - (strcasecmp( g1, g2 ) ) );
}



/* Removed SetKindOfSort definition - implemented in sort.c */



static void RemoveFileEntry(int entry_no)
{
  int i, n;
  FileEntry *fe_ptr;
  int visual_name_len, name_len;

  max_visual_filename_len = 0;
  max_visual_linkname_len = 0;
  n = CurrentVolume->file_count - 1;

  for( i=0; i < n; i++ )
  {
    if( i >= entry_no ) CurrentVolume->file_entry_list[i] = CurrentVolume->file_entry_list[i+1];
    fe_ptr = CurrentVolume->file_entry_list[i].file;
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

  SetFileMode( file_mode ); /* recalc */

  CurrentVolume->file_count--; /* no realloc */
}



static void ChangeFileEntry(void)
{
  int i, n;
  FileEntry *fe_ptr;
  int visual_name_len, name_len;

  max_visual_filename_len = 0;
  max_visual_linkname_len = 0;
  n = CurrentVolume->file_count - 1;

  for( i=0; i < n; i++ )
  {
    fe_ptr = CurrentVolume->file_entry_list[i].file;
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

  SetFileMode( file_mode ); /* recalc */
}


static char GetTypeOfFile(struct stat fst)
{
	if ( S_ISLNK(fst.st_mode) )
		return '@';
	else if ( S_ISSOCK(fst.st_mode) )
		return '=';
	else if ( S_ISCHR(fst.st_mode) )
		return '-';
	else if ( S_ISBLK(fst.st_mode) )
		return '+';
	else if ( S_ISFIFO(fst.st_mode) )
		return '|';
	else if ( S_ISREG(fst.st_mode) )
		return ' ';
	else
		return '?';
}


static void PrintFileEntry(int entry_no, int y, int x, unsigned char hilight, int start_x)
{
  char attributes[11];
  char modify_time[20]; /* Increased to 20 */
  char change_time[20]; /* Increased to 20 */
  char access_time[20]; /* Increased to 20 */
  char format[60];
  char justify;
  char *line_ptr;
  int  n, pos_x;
  FileEntry *fe_ptr;
  static char *line_buffer = NULL;
  static int  old_cols = -1;
  static size_t line_buffer_size = 0;
  char owner[OWNER_NAME_MAX + 1];
  char group[GROUP_NAME_MAX + 1];
  char *owner_name_ptr;
  char *group_name_ptr;
  int  ef_window_width;
  char *sym_link_name = NULL;
  char type_of_file = ' ';
  int  filename_width = 0;
  int  linkname_width = 0;
  int base_color_pair;
  int height, width;

  getmaxyx(file_window, height, width);

  ef_window_width = width - 2; /* Effective Window Width */

  (reverse_sort) ? (justify='+') : (justify='-');

  if( old_cols != COLS )
  {
    old_cols = COLS;
    if( line_buffer ) free( line_buffer );

    line_buffer_size = COLS + PATH_LENGTH;
    if( ( line_buffer = (char *) malloc( line_buffer_size ) ) == NULL )
    {
      ERROR_MSG( "Malloc failed*ABORT" );
      exit( 1 );
    }
  }

  fe_ptr = CurrentVolume->file_entry_list[entry_no].file;

  if( fe_ptr && S_ISLNK( fe_ptr->stat_struct.st_mode ) )
    sym_link_name = &fe_ptr->name[strlen(fe_ptr->name)+1];
  else
    sym_link_name = "";


#ifdef WITH_UTF8
            #if defined(__GNUC__) && __GNUC__ >= 7
            #pragma GCC diagnostic push
            #pragma GCC diagnostic ignored "-Wstringop-overread"
            #endif
            filename_width = max_visual_filename_len + ( strlen(fe_ptr->name) - StrVisualLength(fe_ptr->name) );
            #if defined(__GNUC__) && __GNUC__ >= 7
            #pragma GCC diagnostic pop
            #endif
  if( fe_ptr && S_ISLNK( fe_ptr->stat_struct.st_mode ) )
    linkname_width = max_visual_linkname_len + ( strlen(sym_link_name) - StrVisualLength(sym_link_name) );
#else
  filename_width = max_visual_filename_len;
  linkname_width = max_visual_linkname_len;
#endif


  type_of_file = GetTypeOfFile(fe_ptr->stat_struct);

  /* Calculate starting column position (pos_x) based on column index `x` */
  switch(file_mode)
  {
      case MODE_1:
          if (max_visual_linkname_len)
              pos_x = x * (max_visual_filename_len + max_visual_linkname_len + 51); /* +47 + 4 = 51 */
          else
              pos_x = x * (max_visual_filename_len + 47); /* +43 + 4 = 47 */
          break;
      case MODE_2:
          if( max_visual_linkname_len )
              pos_x = x * (max_visual_filename_len + max_visual_linkname_len + 43);
          else
              pos_x = x * (max_visual_filename_len + 39);
          break;
      case MODE_3:
          pos_x = x * (max_visual_filename_len + 3);
          break;
      case MODE_4:
          if( max_visual_linkname_len )
              pos_x = x * (max_visual_filename_len + max_visual_linkname_len + 52); /* +44 + 8 = 52 */
          else
              pos_x = x * (max_visual_filename_len + 48); /* +40 + 8 = 48 */
          break;
      case MODE_5:
          pos_x = x * (max_visual_userview_len + 1);
          break;
      default:
          pos_x = x;
          break;
  }

  wmove(file_window, y, pos_x);
  base_color_pair = GetFileTypeColor(fe_ptr);

  if (highlight_full_line) {
      /* --- RENDER METHOD 1: FULL LINE HIGHLIGHT --- */
      wattron(file_window, COLOR_PAIR(base_color_pair));
      if(fe_ptr && fe_ptr->tagged) wattron(file_window, A_BOLD);
      if (hilight) wattron(file_window, A_REVERSE);

      /* Build the full line string */
      switch( file_mode ) {
          case MODE_1:
            (void)GetAttributes(fe_ptr->stat_struct.st_mode, attributes);
            (void)CTime(fe_ptr->stat_struct.st_mtime, modify_time);
            if(S_ISLNK(fe_ptr->stat_struct.st_mode)) {
#ifdef HAS_LONGLONG
              /* Updated %12s to %16s */
              (void)snprintf(format, sizeof(format), "%%c%%c%%-%ds %%10s %%3d %%11lld %%16s -> %%-%ds", filename_width, linkname_width);
              (void)snprintf(line_buffer, line_buffer_size, format, (fe_ptr->tagged)?TAGGED_SYMBOL:' ', type_of_file, fe_ptr->name, attributes, fe_ptr->stat_struct.st_nlink, (LONGLONG)fe_ptr->stat_struct.st_size, modify_time, sym_link_name);
#else
              (void)snprintf(format, sizeof(format), "%%c%%c%%-%ds %%10s %%3d %%7d %%16s -> %%-%ds", filename_width, linkname_width);
              (void)snprintf(line_buffer, line_buffer_size, format, (fe_ptr->tagged)?TAGGED_SYMBOL:' ', type_of_file, fe_ptr->name, attributes, fe_ptr->stat_struct.st_nlink, fe_ptr->stat_struct.st_size, modify_time, sym_link_name);
#endif
            } else {
#ifdef HAS_LONGLONG
              (void)snprintf(format, sizeof(format), "%%c%%c%%%c%ds %%10s %%3d %%11lld %%16s", justify, filename_width);
              (void)snprintf(line_buffer, line_buffer_size, format, (fe_ptr->tagged)?TAGGED_SYMBOL:' ', type_of_file, fe_ptr->name, attributes, fe_ptr->stat_struct.st_nlink, (LONGLONG)fe_ptr->stat_struct.st_size, modify_time);
#else
              (void)snprintf(format, sizeof(format), "%%c%%c%%%c%ds %%10s %%3d %%7d %%16s", justify, filename_width);
              (void)snprintf(line_buffer, line_buffer_size, format, (fe_ptr->tagged)?TAGGED_SYMBOL:' ', type_of_file, fe_ptr->name, attributes, fe_ptr->stat_struct.st_nlink, fe_ptr->stat_struct.st_size, modify_time);
#endif
            }
            break;
          case MODE_2:
            owner_name_ptr = GetDisplayPasswdName(fe_ptr->stat_struct.st_uid);
            group_name_ptr = GetDisplayGroupName(fe_ptr->stat_struct.st_gid);
            if(!owner_name_ptr) { snprintf(owner, sizeof(owner), "%d", (int)fe_ptr->stat_struct.st_uid); owner_name_ptr = owner; }
            if(!group_name_ptr) { snprintf(group, sizeof(group), "%d", (int)fe_ptr->stat_struct.st_gid); group_name_ptr = group; }
            if(S_ISLNK(fe_ptr->stat_struct.st_mode)) {
#ifdef HAS_LONGLONG
              (void)snprintf(format, sizeof(format), "%%c%%c%%%c%ds %%10lld %%-12s %%-12s -> %%-%ds", justify, filename_width, linkname_width);
              (void)snprintf(line_buffer, line_buffer_size, format, (fe_ptr->tagged)?TAGGED_SYMBOL:' ', type_of_file, fe_ptr->name, (LONGLONG)fe_ptr->stat_struct.st_ino, owner_name_ptr, group_name_ptr, sym_link_name);
#else
              (void)snprintf(format, sizeof(format), "%%c%%c%%%c%ds  %%8u  %%-12s %%-12s -> %%-%ds", justify, filename_width, linkname_width);
              (void)snprintf(line_buffer, line_buffer_size, format, (fe_ptr->tagged)?TAGGED_SYMBOL:' ', type_of_file, fe_ptr->name, (unsigned int)fe_ptr->stat_struct.st_ino, owner_name_ptr, group_name_ptr, sym_link_name);
#endif
            } else {
#ifdef HAS_LONGLONG
              (void)snprintf(format, sizeof(format), "%%c%%c%%%c%ds %%10lld %%-12s %%-12s", justify, filename_width);
              (void)snprintf(line_buffer, line_buffer_size, format, (fe_ptr->tagged)?TAGGED_SYMBOL:' ', type_of_file, fe_ptr->name, (LONGLONG)fe_ptr->stat_struct.st_ino, owner_name_ptr, group_name_ptr);
#else
              (void)snprintf(format, sizeof(format), "%%c%%c%%%c%ds  %%8u  %%-12s %%-12s", justify, filename_width);
              (void)snprintf(line_buffer, line_buffer_size, format, (fe_ptr->tagged)?TAGGED_SYMBOL:' ', type_of_file, fe_ptr->name, (unsigned int)fe_ptr->stat_struct.st_ino, owner_name_ptr, group_name_ptr);
#endif
            }
            break;
          case MODE_3:
            (void)snprintf(format, sizeof(format), "%%c%%c%%%c%ds", justify, filename_width);
            (void)snprintf(line_buffer, line_buffer_size, format, (fe_ptr->tagged)?TAGGED_SYMBOL:' ', type_of_file, fe_ptr->name);
            break;
          case MODE_4:
            (void)CTime(fe_ptr->stat_struct.st_ctime, change_time);
            (void)CTime(fe_ptr->stat_struct.st_atime, access_time);
            if(S_ISLNK(fe_ptr->stat_struct.st_mode)) {
              /* Updated %12s to %16s */
              (void)snprintf(format, sizeof(format), "%%c%%c%%%c%ds Chg: %%16s  Acc: %%16s -> %%-%ds", justify, filename_width, linkname_width);
              (void)snprintf(line_buffer, line_buffer_size, format, (fe_ptr->tagged)?TAGGED_SYMBOL:' ', type_of_file, fe_ptr->name, change_time, access_time, sym_link_name);
            } else {
              (void)snprintf(format, sizeof(format), "%%c%%c%%%c%ds Chg: %%16s  Acc: %%16s", justify, filename_width);
              (void)snprintf(line_buffer, line_buffer_size, format, (fe_ptr->tagged)?TAGGED_SYMBOL:' ', type_of_file, fe_ptr->name, change_time, access_time);
            }
            break;
          case MODE_5:
            BuildUserFileEntry(fe_ptr, filename_width, linkname_width, USERVIEW, 200, line_buffer);
            break;
      }

      /* Horizontal scrolling logic */
      n = StrVisualLength( line_buffer );
      if( n <= ef_window_width ) {
        line_ptr = line_buffer;
      } else {
        int line_end_pos;
        if( n > ( start_x + ef_window_width ) )
          line_ptr = &line_buffer[VisualPositionToBytePosition(line_buffer, start_x)];
        else
          line_ptr = &line_buffer[VisualPositionToBytePosition(line_buffer, n - ef_window_width)];

        line_end_pos = VisualPositionToBytePosition(line_ptr, ef_window_width);
        line_ptr[line_end_pos] = '\0';
      }
      waddstr(file_window, line_ptr);

      if (hilight) wattroff(file_window, A_REVERSE);
      if(fe_ptr && fe_ptr->tagged) wattroff(file_window, A_BOLD);

  } else {
      /* --- RENDER METHOD 2: NAME-ONLY HIGHLIGHT --- */
      if (start_x > 0) start_x = 0; /* No horizontal scrolling in this mode. */

      wattron(file_window, COLOR_PAIR(base_color_pair));
      if (fe_ptr && fe_ptr->tagged) wattron(file_window, A_BOLD);

      /* Print tag and type */
      wprintw(file_window, "%c%c", (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ', type_of_file);

      /* Calculate available width for name and truncate if necessary */
      int overhead = 0;
      switch(file_mode) {
          case MODE_1: overhead = 44; break;
          case MODE_2: overhead = 40; break;
          case MODE_4: overhead = 48; break;
          default: overhead = 0; break;
      }
      if (sym_link_name && *sym_link_name) overhead += 4 + linkname_width;

      int max_w = width - pos_x - 3 - overhead;
      if (max_w < 5) max_w = 5;

      char display_name[PATH_LENGTH + 1];
      if ((int)strlen(fe_ptr->name) > max_w) {
          CutFilename(display_name, fe_ptr->name, max_w);
      } else {
          strcpy(display_name, fe_ptr->name);
      }

      /* Highlight only the name */
      if (hilight) wattron(file_window, A_REVERSE);
      wprintw(file_window, "%s", display_name);
      if (hilight) wattroff(file_window, A_REVERSE);

      /* Print attributes for modes other than MODE_3 */
      if (file_mode != MODE_3) {
          /* int current_x, current_y; // Removed: Unused */
          int current_x;
          int dummy_y;
          getyx(file_window, dummy_y, current_x);
          (void)dummy_y;

          /* Adjusted target_x calculation to stay within bounds */
          int target_x = MINIMUM(pos_x + 2 + filename_width, width - overhead);

          /* Fill space between name and attributes */
          for (int i = current_x; i < target_x; i++) waddch(file_window, ' ');

          switch (file_mode) {
              case MODE_1:
                  (void)GetAttributes(fe_ptr->stat_struct.st_mode, attributes);
                  (void)CTime(fe_ptr->stat_struct.st_mtime, modify_time);
  #ifdef HAS_LONGLONG
                  /* Updated %12s to %16s */
                  wprintw(file_window, " %10s %3d %11lld %16s", attributes, (int)fe_ptr->stat_struct.st_nlink, (LONGLONG)fe_ptr->stat_struct.st_size, modify_time);
  #else
                  wprintw(file_window, " %10s %3d %7d %16s", attributes, (int)fe_ptr->stat_struct.st_nlink, (int)fe_ptr->stat_struct.st_size, modify_time);
  #endif
                  if (sym_link_name && *sym_link_name) wprintw(file_window, " -> %s", sym_link_name);
                  break;
              case MODE_2:
                  owner_name_ptr = GetDisplayPasswdName(fe_ptr->stat_struct.st_uid);
                  group_name_ptr = GetDisplayGroupName(fe_ptr->stat_struct.st_gid);
                  if (!owner_name_ptr) { snprintf(owner, sizeof(owner), "%d", (int)fe_ptr->stat_struct.st_uid); owner_name_ptr = owner; }
                  if (!group_name_ptr) { snprintf(group, sizeof(group), "%d", (int)fe_ptr->stat_struct.st_gid); group_name_ptr = group; }
  #ifdef HAS_LONGLONG
                  wprintw(file_window, " %10lld %-12s %-12s", (LONGLONG)fe_ptr->stat_struct.st_ino, owner_name_ptr, group_name_ptr);
  #else
                  wprintw(file_window, "  %8u  %-12s %-12s", (unsigned int)fe_ptr->stat_struct.st_ino, owner_name_ptr, group_name_ptr);
  #endif
                  if (sym_link_name && *sym_link_name) wprintw(file_window, " -> %s", sym_link_name);
                  break;
              case MODE_4:
                  (void)CTime(fe_ptr->stat_struct.st_ctime, change_time);
                  (void)CTime(fe_ptr->stat_struct.st_atime, access_time);
                  /* Updated %12s to %16s */
                  wprintw(file_window, " Chg: %16s  Acc: %16s", change_time, access_time);
                  if (sym_link_name && *sym_link_name) wprintw(file_window, " -> %s", sym_link_name);
                  break;
              case MODE_5:
                  break;
          }
      }

      if (fe_ptr && fe_ptr->tagged) wattroff(file_window, A_BOLD);
  }
  wattroff(file_window, COLOR_PAIR(base_color_pair));
}


void DisplayFileWindow(DirEntry *dir_entry)
{
  int height, width;
  GetMaxYX( file_window, &height, &width );
  BuildFileEntryList( dir_entry, &CurrentVolume->vol_stats );
  DisplayFiles( dir_entry,
		dir_entry->start_file,
                dir_entry->start_file + dir_entry->cursor_pos, 0);
}


static void DisplayFiles(DirEntry *de_ptr, int start_file_no, int hilight_no, int start_x)
{
  int  x, y, p_x, p_y, j;
  int height, width;

  getmaxyx(file_window, height, width);

#ifdef COLOR_SUPPORT
  WbkgdSet(file_window, COLOR_PAIR(CPAIR_WINFILE));
#endif
  werase( file_window );

  if( CurrentVolume->file_count == 0 )
  {
    mvwaddstr( file_window,
	       0,
	       3,
	       (de_ptr->access_denied) ? "Permission Denied!" : "No Files!"
	      );
  }

  j = start_file_no; p_x = -1; p_y = 0;
  for( x=0; x < max_column; x++)
  {
    for( y=0; y < height; y++ )
    {
      if( (unsigned)j < CurrentVolume->file_count )
      {
	if( j == hilight_no )
	{
	  p_x = x;
	  p_y = y;
	}
	else
	{
	  PrintFileEntry( j, y, x, FALSE, start_x);
	}
      }
      j++;
    }
  }

  if( p_x >= 0 )
    PrintFileEntry( hilight_no, p_y, p_x, TRUE, start_x);

  wnoutrefresh(file_window); /* ADDED: Stage update for the screen */
}


static void fmovedown(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry)
{
   if( (unsigned int)(*start_file + *cursor_pos + 1) >= CurrentVolume->file_count )
   {
      /* File not present */
      /*----------------------*/
      return;
   }

    if (*cursor_pos < max_disp_files - 1) {
        (*cursor_pos)++;
    } else {
        (*start_file)++;
    }
    DisplayFiles(dir_entry, *start_file, *start_file + *cursor_pos, *start_x);
    /* Update dynamic header path */
    if (CurrentVolume->file_count > 0) {
        char path[PATH_LENGTH];
        int idx = *start_file + *cursor_pos;
        if ((unsigned)idx < CurrentVolume->file_count) {
            GetFileNamePath(CurrentVolume->file_entry_list[idx].file, path);
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
   if( *start_file + *cursor_pos < 1 )
   {
      /* File not present */
      /*----------------------*/
      return;
   }

    if (*cursor_pos > 0) {
        (*cursor_pos)--;
    } else {
        (*start_file)--;
    }
    DisplayFiles(dir_entry, *start_file, *start_file + *cursor_pos, *start_x);
    /* Update dynamic header path */
    if (CurrentVolume->file_count > 0) {
        char path[PATH_LENGTH];
        int idx = *start_file + *cursor_pos;
        if ((unsigned)idx < CurrentVolume->file_count) {
            GetFileNamePath(CurrentVolume->file_entry_list[idx].file, path);
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
      DisplayFiles( dir_entry,
                    *start_file,
                    *start_file + *cursor_pos,
                    *start_x
                  );
      if( hide_right <= 0 ) (*start_x)--;
   }
   else if( (unsigned int)(*start_file + *cursor_pos) >= CurrentVolume->file_count - 1 )
   {
      /* last position reached */
      /*-------------------------*/
   }
   else
   {
      if( (unsigned int)(*start_file + *cursor_pos + x_step) >= CurrentVolume->file_count )
      {
          /* full step not possible;
           * position on last entry
           */
           my_x_step = (int)CurrentVolume->file_count - *start_file - *cursor_pos - 1;
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
      DisplayFiles( dir_entry,
                    *start_file,
                    *start_file + *cursor_pos,
                    *start_x
                  );
   }
   /* Update dynamic header path */
   if (CurrentVolume->file_count > 0) {
       char path[PATH_LENGTH];
       int idx = *start_file + *cursor_pos;
       if ((unsigned)idx < CurrentVolume->file_count) {
           GetFileNamePath(CurrentVolume->file_entry_list[idx].file, path);
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
         DisplayFiles( dir_entry,
                       *start_file,
                       *start_file + *cursor_pos,
                       *start_x
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
         DisplayFiles( dir_entry,
                       *start_file,
                       *start_file + *cursor_pos,
                       *start_x
                       );
     }
     /* Update dynamic header path */
     if (CurrentVolume->file_count > 0) {
         char path[PATH_LENGTH];
         int idx = *start_file + *cursor_pos;
         if ((unsigned)idx < CurrentVolume->file_count) {
             GetFileNamePath(CurrentVolume->file_entry_list[idx].file, path);
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
   if( (unsigned int)(*start_file + *cursor_pos) >= CurrentVolume->file_count - 1 )
   {
      /* last position reached */
      /*-------------------------*/
      return;
   }

    if( *cursor_pos < max_disp_files - 1 )
    {
        if( (unsigned int)(*start_file + max_disp_files) <= CurrentVolume->file_count - 1 )
            *cursor_pos = max_disp_files - 1;
        else
            *cursor_pos = (int)CurrentVolume->file_count - *start_file - 1;
    }
    else
    {
        if( (unsigned int)(*start_file + *cursor_pos + max_disp_files) < CurrentVolume->file_count )
            *start_file += max_disp_files;
        else
            *start_file = (int)CurrentVolume->file_count - max_disp_files;
        if( (unsigned int)(*start_file + max_disp_files) <= CurrentVolume->file_count - 1 )
            *cursor_pos = max_disp_files - 1;
        else
            *cursor_pos = (int)CurrentVolume->file_count - *start_file - 1;
    }
    DisplayFiles( dir_entry,
                  *start_file,
                  *start_file + *cursor_pos,
                  *start_x
                );
    /* Update dynamic header path */
    if (CurrentVolume->file_count > 0) {
        char path[PATH_LENGTH];
        int idx = *start_file + *cursor_pos;
        if ((unsigned)idx < CurrentVolume->file_count) {
            GetFileNamePath(CurrentVolume->file_entry_list[idx].file, path);
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
     if( *start_file + *cursor_pos <= 0 )
     {
        /* first position reached */
        /*-------------------------*/
        return;
     }

    if( *cursor_pos > 0 )
    {
        *cursor_pos = 0;
    }
    else
    {
        if( *start_file > max_disp_files )
            *start_file -= max_disp_files;
        else
            *start_file = 0;
    }
    DisplayFiles( dir_entry,
                  *start_file,
                  *start_file + *cursor_pos,
                  *start_x
                );
    /* Update dynamic header path */
    if (CurrentVolume->file_count > 0) {
        char path[PATH_LENGTH];
        int idx = *start_file + *cursor_pos;
        if ((unsigned)idx < CurrentVolume->file_count) {
            GetFileNamePath(CurrentVolume->file_entry_list[idx].file, path);
            DisplayHeaderPath(path);
        }
    } else {
        char path[PATH_LENGTH];
        GetPath(dir_entry, path);
        DisplayHeaderPath(path);
    }
}

/* Local helper to refresh file window, maintaining file cursor */
static void RefreshFileView(DirEntry *dir_entry) {
    char *saved_name = NULL;
    Statistic *s = &CurrentVolume->vol_stats;
    int found_idx = -1;
    int start_x = 0;

    /* 1. Save current filename */
    if (CurrentVolume->file_count > 0) {
        FileEntry *curr = CurrentVolume->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
        if (curr) saved_name = strdup(curr->name);
    }

    /* 2. Perform Safe Tree Refresh (Save/Rescan/Restore) */
    RefreshTreeSafe(dir_entry);

    /* 3. Rebuild File List from the refreshed tree */
    BuildFileEntryList(dir_entry, s);

    /* 4. Restore Cursor Position */
    if (saved_name) {
        int k;
        for (k = 0; k < (int)CurrentVolume->file_count; k++) {
            if (strcmp(CurrentVolume->file_entry_list[k].file->name, saved_name) == 0) {
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
            if (dir_entry->start_file + max_disp_files > (int)CurrentVolume->file_count) {
                 dir_entry->start_file = MAXIMUM(0, (int)CurrentVolume->file_count - max_disp_files);
                 dir_entry->cursor_pos = (int)CurrentVolume->file_count - 1 - dir_entry->start_file;
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

    DisplayFiles(dir_entry, dir_entry->start_file, dir_entry->start_file + dir_entry->cursor_pos, start_x);
}

static void HandleInvertTags(DirEntry *dir_entry, Statistic *s)
{
    int i;
    FileEntry *fe_ptr;
    DirEntry *de_ptr;

    /* Iterate through the currently visible list of files */
    for (i = 0; i < (int)CurrentVolume->file_count; i++)
    {
        fe_ptr = CurrentVolume->file_entry_list[i].file;
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
    DisplayFiles(dir_entry,
                 dir_entry->start_file,
                 dir_entry->start_file + dir_entry->cursor_pos,
                 0 /* start_x */
                );
    DisplayDiskStatistic(s);
    if( dir_entry->global_flag )
        DisplayDiskStatistic(s); /* Global view */
    else
        DisplayDirStatistic(dir_entry, NULL, s);
}

/* Helper for Smart Search: Tags matches (Result == 0) */
/* Kept for completeness although not used in Strict Filter mode */
static int SilentSearchWalk(FileEntry *fe_ptr, WalkingPackage *wp)
{
    char command_line[COMMAND_LINE_LENGTH + 1];
    char raw_path[PATH_LENGTH + 1];
    char quoted_path[PATH_LENGTH * 2 + 1];
    const char *template_ptr;
    int i, dest_idx;
    size_t path_len;
    int ret;

    /* Build command like ExecuteCommand, but minimal */
    (void) GetFileNamePath( fe_ptr, raw_path );
    StrCp( quoted_path, raw_path );
    path_len = strlen( quoted_path );

    template_ptr = wp->function_data.execute.command;
    dest_idx = 0;

    for( i = 0; template_ptr[i] != '\0' && dest_idx < COMMAND_LINE_LENGTH; )
    {
        if( template_ptr[i] == '%' && template_ptr[i+1] == 's' ) {
            if( dest_idx + path_len < COMMAND_LINE_LENGTH ) {
                strcpy( &command_line[dest_idx], quoted_path );
                dest_idx += path_len;
            }
            i += 2;
        } else if( template_ptr[i] == '{' && template_ptr[i+1] == '}' ) {
            if( dest_idx + path_len < COMMAND_LINE_LENGTH ) {
                strcpy( &command_line[dest_idx], quoted_path );
                dest_idx += path_len;
            }
            i += 2;
        } else {
            command_line[dest_idx++] = template_ptr[i++];
        }
    }
    command_line[dest_idx] = '\0';

    ret = SilentSystemCallEx(command_line, FALSE, &CurrentVolume->vol_stats);

    /* If command returns 0 (Success), TAG the file */
    if (ret == 0) {
        if (!fe_ptr->tagged) {
            fe_ptr->tagged = TRUE;
            fe_ptr->dir_entry->tagged_files++;
            fe_ptr->dir_entry->tagged_bytes += fe_ptr->stat_struct.st_size;
            CurrentVolume->vol_stats.disk_tagged_files++;
            CurrentVolume->vol_stats.disk_tagged_bytes += fe_ptr->stat_struct.st_size;
        }
    }
    return 0; /* Continue walking */
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
  int  dir_window_width, dir_window_height;
  int  get_dir_ret;
  YtreeAction action = ACTION_NONE; /* Initialize action */
  DirEntry *last_stats_dir = NULL; /* Track context changes */
  struct Volume *start_vol = CurrentVolume; /* Safety Check Variable */
  Statistic *s = &CurrentVolume->vol_stats;
  int pclose_ret;
  int height, width;

  unput_char = '\0';
  fe_ptr = NULL;


  /* Reset cursor position flags */
  /*--------------------------------------*/

  need_dsp_help = TRUE;
  maybe_change_x_step = TRUE;

  BuildFileEntryList( dir_entry, s );

  /* Sanitize cursor position immediately after BuildFileEntryList */
  if (CurrentVolume->file_count > 0) {
      if (dir_entry->cursor_pos < 0) dir_entry->cursor_pos = 0;
      if (dir_entry->start_file < 0) dir_entry->start_file = 0;

      /* Bounds Check: ensure we aren't past the end */
      if ((unsigned int)(dir_entry->start_file + dir_entry->cursor_pos) >= CurrentVolume->file_count) {
          dir_entry->start_file = MAXIMUM(0, (int)CurrentVolume->file_count - max_disp_files);
          dir_entry->cursor_pos = (int)CurrentVolume->file_count - 1 - dir_entry->start_file;
      }
  } else {
      dir_entry->cursor_pos = 0;
      dir_entry->start_file = 0;
  }

  if( dir_entry->global_flag || dir_entry->big_window || dir_entry->tagged_flag)
  {
    SwitchToBigFileWindow();
    getmaxyx( file_window, height, width );
    DisplayDiskStatistic(s);
    /* Force initial display of directory statistics with appropriate title */
    DisplayDirStatistic(dir_entry, (dir_entry->global_flag) ? "SHOW ALL" : NULL, s);
  }
  else
  {
    getmaxyx( file_window, height, width );
    DisplayDirStatistic( dir_entry, NULL, s ); /* Updated call */
  }

  DisplayFiles( dir_entry,
		dir_entry->start_file,
		dir_entry->start_file + dir_entry->cursor_pos,
		start_x
	      );

  do
  {
    /* Critical Safety: If volume was deleted (e.g. via K menu), abort immediately */
    if (CurrentVolume != start_vol) return ESC;

    if( maybe_change_x_step )
    {
      maybe_change_x_step = FALSE;

      getmaxyx( file_window, height, width );
      x_step =  (max_column > 1) ? height : 1;
      max_disp_files = height * max_column;
    }

    if( need_dsp_help )
    {
      need_dsp_help = FALSE;
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
      if (CurrentVolume->file_count > 0) {
           fe_ptr = CurrentVolume->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;

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

      RefreshWindow( dir_window ); /* needed: ncurses-bug ? */
      RefreshWindow( file_window );
      doupdate();
      ch = (resize_request) ? -1 : Getch();
      if( ch == LF ) ch = CR;
    }

    /* Re-check safety after blocking Getch */
    if (CurrentVolume != start_vol) return ESC;

    if (IsUserActionDefined()) { /* User commands take precedence */
       ch = FileUserMode(&(CurrentVolume->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]), ch);
       if (CurrentVolume != start_vol) return ESC;
    }

   if(resize_request) {
     ReCreateWindows();
     RereadWindowSize(dir_entry);
     DisplayMenu();

     getmaxyx(dir_window, dir_window_height, dir_window_width);
     while(s->cursor_pos >= dir_window_height) {
       s->cursor_pos--;
       s->disp_begin_pos++;
     }
     if(dir_entry->global_flag || dir_entry->big_window || dir_entry->tagged_flag) {

       /* big window active */

       SwitchToBigFileWindow();
       DisplayFileWindow(dir_entry);

       if(dir_entry->global_flag) {
	 DisplayDiskStatistic(s);
	 DisplayGlobalFileParameter(fe_ptr);
       } else {
	 DisplayFileWindow(dir_entry);
	 DisplayDirStatistic(dir_entry, NULL, s);
	 DisplayFileParameter(fe_ptr);
       }
     } else {

       /* small window active */

       SwitchToSmallFileWindow();
       DisplayTree( CurrentVolume, dir_window, s->disp_begin_pos,
		  s->disp_begin_pos + s->cursor_pos
		);
       DisplayFileWindow(dir_entry);
       DisplayDirStatistic(dir_entry, NULL, s);
       DisplayFileParameter(fe_ptr);
     }
     need_dsp_help = TRUE;
     DisplayAvailBytes(s);
     DisplayFilter(s);
     DisplayDiskName(s);
     resize_request = FALSE;
   }

   action = GetKeyAction(ch);

   /* Special remapping for MODE_1: TAB/BTAB act as UP/DOWN */
   if( file_mode == MODE_1 )
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

	DisplayFiles( dir_entry,
		      dir_entry->start_file,
		      dir_entry->start_file + dir_entry->cursor_pos,
		      start_x
		    );
     }
   }


   switch( action )
   {

      case ACTION_NONE:         break;

      case ACTION_MOVE_DOWN :  fmovedown(&dir_entry->start_file, &dir_entry->cursor_pos, &start_x, dir_entry);
		      break;

      case ACTION_MOVE_UP   : fmoveup(&dir_entry->start_file, &dir_entry->cursor_pos, &start_x, dir_entry);
		      break;

      case ACTION_MOVE_RIGHT:
          if (!highlight_full_line && x_step == 1) break; /* No horizontal scroll in name-only mode */
          fmoveright(&dir_entry->start_file, &dir_entry->cursor_pos, &start_x, dir_entry);
          break;

      case ACTION_MOVE_LEFT :
          if (!highlight_full_line && x_step == 1) break; /* No horizontal scroll in name-only mode */
          fmoveleft(&dir_entry->start_file, &dir_entry->cursor_pos, &start_x, dir_entry);
          break;

      case ACTION_PAGE_DOWN: fmovenpage(&dir_entry->start_file, &dir_entry->cursor_pos, &start_x, dir_entry);
		      break;

      case ACTION_PAGE_UP: fmoveppage(&dir_entry->start_file, &dir_entry->cursor_pos, &start_x, dir_entry);
		      break;

      case ACTION_END  : if( (unsigned int)(dir_entry->start_file + dir_entry->cursor_pos + 1) >= CurrentVolume->file_count )
		      {
			/* last position reached */
			/*--------------------------*/
		      }
		      else
		      {
			if( (int)CurrentVolume->file_count < max_disp_files )
		        {
			  dir_entry->start_file = 0;
			  dir_entry->cursor_pos = (int)CurrentVolume->file_count - 1;
		        }
		        else
	                {
                          dir_entry->start_file = (int)CurrentVolume->file_count - max_disp_files;
			  dir_entry->cursor_pos = (int)CurrentVolume->file_count - dir_entry->start_file - 1;
		        }

			DisplayFiles( dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
				    );
		      }
		      break;

      case ACTION_HOME : if( dir_entry->start_file + dir_entry->cursor_pos <= 0 )
		      {
			/* first position reached */
			/*-------------------------*/
		      }
		      else
		      {
                        dir_entry->start_file = 0;
			dir_entry->cursor_pos = 0;

			DisplayFiles( dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
				    );

		      }
		      break;

      case ACTION_TOGGLE_HIDDEN:      {
                        ToggleDotFiles();

                        /* Update current dir pointer using the new accessor function */
                        dir_entry = GetSelectedDirEntry(CurrentVolume);

                        /* Explicitly update the file window (preview) */
                        DisplayFileWindow(dir_entry);
                        RefreshWindow(file_window);

                        need_dsp_help = TRUE;
                     }
		     break;

      case ACTION_CMD_A :      fe_ptr = CurrentVolume->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;

	              need_dsp_help = TRUE;

		      if( !ChangeFileModus( fe_ptr ) )
		      {
			DisplayFiles( dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
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

			  DisplayFiles( dir_entry,
					dir_entry->start_file,
					dir_entry->start_file + dir_entry->cursor_pos,
					start_x
				      );
			}
            move( LINES - 2, 1 ); clrtoeol(); /* Cleanup prompt line */
		      }
		      break;

      case ACTION_CMD_O :      fe_ptr = CurrentVolume->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;

		      need_dsp_help = TRUE;

		      if( !ChangeFileOwner( fe_ptr ) )
		      {
			DisplayFiles( dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
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

			  DisplayFiles( dir_entry,
					dir_entry->start_file,
					dir_entry->start_file + dir_entry->cursor_pos,
					start_x
				      );
			}
		      }
		      break;

      case ACTION_CMD_G :      fe_ptr = CurrentVolume->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;

		      need_dsp_help = TRUE;

		      if( !ChangeFileGroup( fe_ptr ) )
		      {
			DisplayFiles( dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
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

			  DisplayFiles( dir_entry,
					dir_entry->start_file,
					dir_entry->start_file + dir_entry->cursor_pos,
					start_x
				      );
			}
		      }
		      break;

      case ACTION_TAG :      fe_ptr = CurrentVolume->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;

		      if( !fe_ptr->tagged )
		      {
                        fe_ptr->tagged = TRUE;
	       	        de_ptr->tagged_files++;
		        de_ptr->tagged_bytes += fe_ptr->stat_struct.st_size;
	       	        s->disk_tagged_files++;
		        s->disk_tagged_bytes += fe_ptr->stat_struct.st_size;
		      }
              DisplayFiles(dir_entry, dir_entry->start_file, dir_entry->start_file + dir_entry->cursor_pos, start_x);
              DisplayDiskStatistic(s); /* Always update global disk stats */
              DisplayDirStatistic(dir_entry, NULL, s); /* Always update current list stats (even in Showall) */
		      unput_char = KEY_DOWN;

                      break;
      case ACTION_UNTAG :      fe_ptr = CurrentVolume->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;
                      if( fe_ptr->tagged )
		      {
			fe_ptr->tagged = FALSE;

			de_ptr->tagged_files--;
			de_ptr->tagged_bytes -= fe_ptr->stat_struct.st_size;
			s->disk_tagged_files--;
			s->disk_tagged_bytes -= fe_ptr->stat_struct.st_size;
		      }
              DisplayFiles(dir_entry, dir_entry->start_file, dir_entry->start_file + dir_entry->cursor_pos, start_x);
              DisplayDiskStatistic(s); /* Always update global disk stats */
              DisplayDirStatistic(dir_entry, NULL, s); /* Always update current list stats (even in Showall) */
		      unput_char = KEY_DOWN;

		      break;

      case ACTION_TOGGLE_MODE :
		      list_pos = dir_entry->start_file + dir_entry->cursor_pos;

		      RotateFileMode();
              getmaxyx( file_window, height, width );
              x_step =  (max_column > 1) ? height : 1;
              max_disp_files = height * max_column;

		      if( dir_entry->cursor_pos >= max_disp_files )
		      {
			/* Cursor must be repositioned */
			/*-------------------------------------*/

                        dir_entry->cursor_pos = max_disp_files - 1;
		      }

		      dir_entry->start_file = list_pos - dir_entry->cursor_pos;
		      DisplayFiles( dir_entry,
				    dir_entry->start_file,
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x
			          );
		      break;

      case ACTION_TAG_ALL :
                      for(i=0; i < (int)CurrentVolume->file_count; i++)
                      {
			fe_ptr = CurrentVolume->file_entry_list[i].file;
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

		      DisplayFiles( dir_entry,
				    dir_entry->start_file,
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x
			          );
              DisplayDiskStatistic(s); /* Always update global disk stats */
              DisplayDirStatistic(dir_entry, NULL, s); /* Always update current list stats (even in Showall) */
		      break;


      case ACTION_UNTAG_ALL :
                      for(i=0; i < (int)CurrentVolume->file_count; i++)
                      {
			fe_ptr = CurrentVolume->file_entry_list[i].file;
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

		      DisplayFiles( dir_entry,
				    dir_entry->start_file,
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x
			          );
              DisplayDiskStatistic(s); /* Always update global disk stats */
              DisplayDirStatistic(dir_entry, NULL, s); /* Always update current list stats (even in Showall) */
		      break;



      case ACTION_TAG_REST :
                      for(i=dir_entry->start_file + dir_entry->cursor_pos; i < (int)CurrentVolume->file_count; i++)
                      {
			fe_ptr = CurrentVolume->file_entry_list[i].file;
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

		      DisplayFiles( dir_entry,
				    dir_entry->start_file,
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x
			          );
              DisplayDiskStatistic(s); /* Always update global disk stats */
              DisplayDirStatistic(dir_entry, NULL, s); /* Always update current list stats (even in Showall) */
		      break;


      case ACTION_UNTAG_REST :
                      for(i=dir_entry->start_file + dir_entry->cursor_pos; i < (int)CurrentVolume->file_count; i++)
                      {
			fe_ptr = CurrentVolume->file_entry_list[i].file;
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

		      DisplayFiles( dir_entry,
				    dir_entry->start_file,
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x
			          );
              DisplayDiskStatistic(s); /* Always update global disk stats */
              DisplayDirStatistic(dir_entry, NULL, s); /* Always update current list stats (even in Showall) */
		      break;

      case ACTION_CMD_V :      fe_ptr = CurrentVolume->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;
		      (void) GetRealFileNamePath( fe_ptr, filepath );
		      (void) View( dir_entry, filepath );
		      need_dsp_help = TRUE;
		      break;

      case ACTION_CMD_H :      fe_ptr = CurrentVolume->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;
		      (void) GetRealFileNamePath( fe_ptr, filepath );
		      (void) ViewHex( filepath );
		      need_dsp_help = TRUE;
		      break;

      case ACTION_CMD_E :      fe_ptr = CurrentVolume->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;
		      (void) GetFileNamePath( fe_ptr, filepath );
		      (void) Edit( de_ptr, filepath );
		      break;

      case ACTION_CMD_Y :
      case ACTION_CMD_C :
		      fe_ptr = CurrentVolume->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
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
                          (void)snprintf(message, MESSAGE_LENGTH, "Invalid destination path*\"%s\"*%s", to_dir, strerror(errno));
                          MESSAGE(message);
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
                  CopyFile( s, fe_ptr, TRUE, expanded_to_file, dest_dir_entry, to_path, path_copy, &dir_create_mode, &overwrite_mode );
              }

              /* Force a full refresh of the file window state after copy attempt */
              DisplayAvailBytes(s);
              DisplayFileWindow(dir_entry);
              keypad(file_window, TRUE);
              touchwin(file_window);
              wrefresh(file_window);
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
                             (void)snprintf(message, MESSAGE_LENGTH, "Invalid destination path*\"%s\"*%s", to_dir, strerror(errno));
                             MESSAGE(message);
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
			  walking_package.function_data.copy.confirm = (term == 'Y') ? TRUE : FALSE; /* Fixed struct access */
              walking_package.function_data.copy.dir_create_mode = 0; /* Reset auto-create mode */
              walking_package.function_data.copy.overwrite_mode = (term == 'N') ? 1 : 0; /* Reset overwrite mode */

			  WalkTaggedFiles( dir_entry->start_file,
					   dir_entry->cursor_pos,
					   CopyTaggedFiles,
					   &walking_package
				         );

                          DisplayAvailBytes(s);

                          /* Force a full refresh of the file window state after copy attempt */
                          DisplayFileWindow(dir_entry);
                          keypad(file_window, TRUE);
                          touchwin(file_window);
                          wrefresh(file_window);
		      }
		      break;

      case ACTION_CMD_M :      if( mode != DISK_MODE && mode != USER_MODE )
                      {
			break;
		      }

		      fe_ptr = CurrentVolume->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
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
				     TRUE,
				     expanded_to_file,
				     dest_dir_entry,
				     to_path,
				     &new_fe_ptr,
                     &dir_create_mode,
                     &overwrite_mode
				   ) )
		      {
			/* File was moved */
			/*-------------------*/

                        DisplayAvailBytes(s);

			if( dir_entry->global_flag )
			  DisplayDiskStatistic(s);
			else
			  DisplayDirStatistic( de_ptr, NULL, s ); /* Updated call */

			BuildFileEntryList( dir_entry, s );

			if( CurrentVolume->file_count == 0 ) unput_char = ESC;

			if( dir_entry->start_file + dir_entry->cursor_pos >= (int)CurrentVolume->file_count )
			{
			  if( --dir_entry->cursor_pos < 0 )
			  {
			    if( dir_entry->start_file > 0 )
			    {
			      dir_entry->start_file--;
			    }
			    dir_entry->cursor_pos = 0;
			  }
			}

			DisplayFiles( dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
			            );
			maybe_change_x_step = TRUE;
            /* REMOVED: RefreshDirWindow(); Fixed UI Glitch */
		      }
              }
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
			walking_package.function_data.mv.confirm = (term == 'Y') ? TRUE : FALSE;
            walking_package.function_data.mv.dir_create_mode = 0; /* Reset auto-create mode */
            walking_package.function_data.mv.overwrite_mode = (term == 'N') ? 1 : 0; /* Reset overwrite mode */

			WalkTaggedFiles( dir_entry->start_file,
					 dir_entry->cursor_pos,
					 MoveTaggedFiles,
					 &walking_package
				       );

			BuildFileEntryList( dir_entry, s );

			if( CurrentVolume->file_count == 0 ) unput_char = ESC;

			dir_entry->start_file = 0;
			dir_entry->cursor_pos = 0;

			DisplayFiles( dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
			            );
			maybe_change_x_step = TRUE;
            /* REMOVED: RefreshDirWindow(); Fixed UI Glitch */
		      }
		      break;

      case ACTION_CMD_D :      if( mode != DISK_MODE && mode != USER_MODE )
		      {
			break;
		      }

		      term = InputChoice( "Delete this file (Y/N) ? ",
					  "YN\033"
					);

		      need_dsp_help = TRUE;

		      if( term != 'Y' ) break;

		      fe_ptr = CurrentVolume->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;

              {
                  int override_mode = 0;
		      if( !DeleteFile( fe_ptr, &override_mode, s ) )
		      {
		        /* File was deleted */
			/*----------------------*/

			if( dir_entry->global_flag )
			  DisplayDiskStatistic(s);
			else
			  DisplayDirStatistic( de_ptr, NULL, s ); /* Updated call */

			DisplayAvailBytes(s);

                        RemoveFileEntry( dir_entry->start_file + dir_entry->cursor_pos );

			if( CurrentVolume->file_count == 0 ) unput_char = ESC;

			if( dir_entry->start_file + dir_entry->cursor_pos >= (int)CurrentVolume->file_count )
			{
			  if( --dir_entry->cursor_pos < 0 )
			  {
			    if( dir_entry->start_file > 0 )
			    {
			      dir_entry->start_file--;
			    }
			    dir_entry->cursor_pos = 0;
			  }
			}

			DisplayFiles( dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
			            );
			maybe_change_x_step = TRUE;
		      }
              }
                      break;

      case ACTION_CMD_TAGGED_D :
		      if(( mode != DISK_MODE && mode != USER_MODE) || !IsMatchingTaggedFiles() )
		      {
		      }
		      else
		      {
		        need_dsp_help = TRUE;
			(void) DeleteTaggedFiles( max_disp_files, s );
			if( CurrentVolume->file_count == 0 ) unput_char = ESC;
			dir_entry->start_file = 0;
			dir_entry->cursor_pos = 0;
                        DisplayAvailBytes(s);
			DisplayFiles( dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
			            );
			maybe_change_x_step = TRUE;
		      }
		      break;

      case ACTION_CMD_R :      if( mode != DISK_MODE && mode != USER_MODE )
		      {
			break;
		      }

		      fe_ptr = CurrentVolume->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;

		      if( !GetRenameParameter( fe_ptr->name, new_name ) )
		      {
            /* EXPAND WILDCARDS FOR SINGLE FILE RENAME */
            BuildFilename(fe_ptr->name, new_name, expanded_new_name);

			if( !RenameFile( fe_ptr, expanded_new_name, &new_fe_ptr ) )
		        {
			  /* Rename OK */
			  /*-----------*/

			  /* Maybe structure has changed... */
			  /*--------------------------------*/

			  BuildFileEntryList( de_ptr, s );

			  DisplayFiles( de_ptr,
				        dir_entry->start_file,
				        dir_entry->start_file + dir_entry->cursor_pos,
				        start_x
			              );
			  maybe_change_x_step = TRUE;
                        }
		      }
		      need_dsp_help = TRUE;
		      break;

      case ACTION_CMD_TAGGED_R :
		      if(( mode != DISK_MODE && mode != USER_MODE) || !IsMatchingTaggedFiles() )
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

			BuildFileEntryList( dir_entry, s );

			if( CurrentVolume->file_count == 0 ) unput_char = ESC;

			DisplayFiles( dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
			            );

			maybe_change_x_step = TRUE;
		      }
		      break;

      case ACTION_CMD_S :       GetKindOfSort();

		      dir_entry->start_file = 0;
		      dir_entry->cursor_pos = 0;

		      SortFileEntryList(s);

		      DisplayFiles( dir_entry,
				    dir_entry->start_file,
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x
			          );
		      need_dsp_help = TRUE;
		      break;

      case ACTION_FILTER :       if(ReadFilter() == 0) {

		        dir_entry->start_file = 0;
		        dir_entry->cursor_pos = 0;

		        BuildFileEntryList( dir_entry, s );

		        DisplayFilter(s);
		        DisplayFiles( dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
			            );

		        if( dir_entry->global_flag )
		          DisplayDiskStatistic(s);
		        else
		          DisplayDirStatistic( dir_entry, NULL, s ); /* Updated call */

                        if( CurrentVolume->file_count == 0 ) unput_char = ESC;
		        maybe_change_x_step = TRUE;
	              }
		      need_dsp_help = TRUE;
		      break;

      case ACTION_LOGIN :      fe_ptr = CurrentVolume->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		     if( mode == DISK_MODE || mode == USER_MODE )
		     {
		       (void) GetFileNamePath( fe_ptr, new_login_path );
                       if( !GetNewLoginPath( new_login_path ) )
		       {
			 dir_entry->login_flag  = TRUE;

		         (void) LoginDisk( new_login_path );
		         unput_char = ESC;
			}
		        need_dsp_help = TRUE;
		     }
		     break;

      case ACTION_ENTER :        if( dir_entry->big_window ) break;
		      dir_entry->big_window = TRUE;
		      ch = '\0'; /* Reset ch to avoid re-triggering action */
		      SwitchToBigFileWindow();
                      GetMaxYX( file_window, &height, &width );

		      x_step =  (max_column > 1) ? height : 1;
                      max_disp_files = height * max_column;

		      DisplayFiles( dir_entry,
				    dir_entry->start_file,
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x
			          );
                      action = ACTION_NONE; /* Prevent loop termination to stay in file window */
		      break;

      case ACTION_CMD_P :      fe_ptr = CurrentVolume->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;
		      (void) Pipe( de_ptr, fe_ptr );
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
			  (void) snprintf( message, MESSAGE_LENGTH, "execution of command*%s*failed", filepath );
			  MESSAGE( message );
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

                        (void) GetAvailBytes( &s->disk_space, s );
                        DisplayAvailBytes(s);

			DisplayFiles( dir_entry,
				      dir_entry->start_file,
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
			            );
		      }
		      break;

      case ACTION_CMD_X :      fe_ptr = CurrentVolume->file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;
		      (void) Execute( de_ptr, fe_ptr );
              RefreshFileView(dir_entry); /* Auto-Refresh after command */
		      need_dsp_help = TRUE;
		      break;

      case ACTION_CMD_TAGGED_S :
                      /* STRICT FILTER MODE: Only allow if tags exist */
                      if( !IsMatchingTaggedFiles() )
                      {
                        /* If no tags, this command does nothing (or shows error) */
                        MESSAGE( "No tagged files" );
                      }
		      else if( mode != DISK_MODE && mode != USER_MODE )
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

			need_dsp_help = TRUE;
			*command_line = '\0';

            /* Filter Mode */
		        if( !GetSearchCommandLine( command_line, "SEARCH TAGGED: " ) )
			    {
			      refresh();
			      endwin();
			      SuspendClock();

			      walking_package.function_data.execute.command = command_line;
                              /* Use modified SilentTagWalk (Untag on Fail) */
                              SilentTagWalkTaggedFiles( ExecuteCommand,
					            &walking_package
					          );
			      RefreshWindow( file_window );
			      HitReturnToContinue();
			      InitClock();
			    }

            /* Refresh Display */
            DisplayFiles( dir_entry,
                  dir_entry->start_file,
                  dir_entry->start_file + dir_entry->cursor_pos,
                  start_x
                );
            DisplayDiskStatistic(s);
            if( dir_entry->global_flag )
                DisplayDiskStatistic(s);
            else
                DisplayDirStatistic(dir_entry, NULL, s);

			free( command_line );
		      }
		      break;

      case ACTION_CMD_TAGGED_X:
		      if( !IsMatchingTaggedFiles() )
		      {
		      }
		      else if( mode != DISK_MODE && mode != USER_MODE )
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

              RefreshFileView(dir_entry); /* Auto-Refresh after tagged command */
			}
			free( command_line );
		      }
		      break;

      case ACTION_QUIT_DIR:
                      need_dsp_help = TRUE;
                      fe_ptr = CurrentVolume->file_entry_list[dir_entry->start_file
                                + dir_entry->cursor_pos].file;
                      de_ptr = fe_ptr->dir_entry;
                      QuitTo(de_ptr);
                      break;

      case ACTION_QUIT:       need_dsp_help = TRUE;
                      Quit();
                      action = ACTION_NONE;
		      break;

      case ACTION_VOL_MENU:
          if (SelectLoadedVolume() == 0) {
              unput_char = ESC;
          } else {
              ch = 0;
          }
          if (CurrentVolume != start_vol) return ESC;
          break;
      case ACTION_VOL_PREV:
          if (CycleLoadedVolume(-1) == 0) {
              unput_char = ESC;
          } else {
              ch = 0;
          }
          if (CurrentVolume != start_vol) return ESC;
          break;
      case ACTION_VOL_NEXT:
          if (CycleLoadedVolume(1) == 0) {
              unput_char = ESC;
          } else {
              ch = 0;
          }
          if (CurrentVolume != start_vol) return ESC;
          break;

      case ACTION_REFRESH:
                {
                    RefreshFileView(dir_entry);
                    need_dsp_help = TRUE;
                }
                break;

      case ACTION_RESIZE: resize_request = TRUE; break;

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

        BuildFileEntryList ( dir_entry, s );

        dir_entry->start_file = 0;
        dir_entry->cursor_pos = 0;
        DisplayFiles
        (
            dir_entry,
            dir_entry->start_file,
            dir_entry->start_file + dir_entry->cursor_pos,
            start_x
        );
        maybe_change_x_step = TRUE;
        break;

    case ACTION_TREE_EXPAND: /* Mapped to '*' for Invert Tags in File Window */
        HandleInvertTags(dir_entry, s);
        need_dsp_help = TRUE;
        break;

     default:
                      break;
    } /* switch */
  } while( action != ACTION_QUIT && action != ACTION_ENTER && action != ACTION_ESCAPE && action != ACTION_QUIT );

  if( dir_entry->big_window )
    SwitchToSmallFileWindow();

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

  max_disp_files = height * max_column;

  for( i=0; i < (int)CurrentVolume->file_count && result == 0; i++ )
  {
    fe_ptr = CurrentVolume->file_entry_list[i].file;

    if( fe_ptr->tagged && fe_ptr->matching )
    {
      if( maybe_change_x == FALSE &&
	  i >= start_file && i < start_file + max_disp_files )
      {
	/* Walk possible without scrolling */
	/*---------------------------*/
    DisplayFiles( fe_ptr->dir_entry,
            start_file,
            i,
            start_x
            );
        cursor_pos = i - start_file;
      }
      else
      {
	/* Scrolling necessary */
	/*---------------*/

	start_file = MAX( 0, i - max_disp_files + 1 );
	cursor_pos = i - start_file;

        DisplayFiles( fe_ptr->dir_entry,
		      start_file,
		      start_file + cursor_pos,
		      start_x
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
        CurrentVolume->file_entry_list[i].file = walking_package->new_fe_ptr;
	ChangeFileEntry();
        max_disp_files = height * max_column;
	maybe_change_x = TRUE;
      }
    }
  }

  if( baudrate() >= QUICK_BAUD_RATE ) typeahead( -1 );
}

/*
 ExecuteCommand (*fkt) had its retval zeroed as found.
 ^S needs that value, so it was unzeroed. forloop below
 was modified to not care about retval instead?

 global flag for stop-on-error? does anybody want it?

 --crb3 12mar04
*/

static void SilentWalkTaggedFiles( int (*fkt) (FileEntry *, WalkingPackage *),
			           WalkingPackage *walking_package
			          )
{
  FileEntry *fe_ptr;
  int       i;


  for( i=0; i < (int)CurrentVolume->file_count; i++ )
  {
    fe_ptr = CurrentVolume->file_entry_list[i].file;

    if( fe_ptr->tagged && fe_ptr->matching )
    {
      (void)fkt( fe_ptr, walking_package );
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

static void SilentTagWalkTaggedFiles( int (*fkt) (FileEntry *, WalkingPackage *),
			           WalkingPackage *walking_package
			          )
{
  FileEntry *fe_ptr;
  int       i;
  int       result = 0;


  for( i=0; i < (int)CurrentVolume->file_count; i++ )
  {
    fe_ptr = CurrentVolume->file_entry_list[i].file;

    if( fe_ptr->tagged && fe_ptr->matching )
    {
      result = fkt( fe_ptr, walking_package );

      if( result != 0 ) {
      	fe_ptr->tagged = FALSE;
        /* Update Stats */
        fe_ptr->dir_entry->tagged_files--;
        fe_ptr->dir_entry->tagged_bytes -= fe_ptr->stat_struct.st_size;
        CurrentVolume->vol_stats.disk_tagged_files--;
        CurrentVolume->vol_stats.disk_tagged_bytes -= fe_ptr->stat_struct.st_size;
      }
    }
  }
}




static BOOL IsMatchingTaggedFiles(void)
{
  FileEntry *fe_ptr;
  int i;

  for( i=0; i < (int)CurrentVolume->file_count; i++)
  {
    fe_ptr = CurrentVolume->file_entry_list[i].file;

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

  /* 1. Count tagged files */
  for(i=0; i < (int)CurrentVolume->file_count; i++) {
      if(CurrentVolume->file_entry_list[i].file->tagged && CurrentVolume->file_entry_list[i].file->matching) {
          tagged_count++;
      }
  }

  if (tagged_count == 0) return 0;

  /* 2. Prompt for Intent */
  (void) snprintf(message, MESSAGE_LENGTH, "Delete %d tagged files (Y/N) ?", tagged_count);
  term = InputChoice(message, "YN\033");
  if (term != 'Y') return -1;

  /* 3. Prompt for Mode (Confirm Each?) */
  term = InputChoice( "Ask for confirmation for each file (Y/N) ? ", "YN\033" );
  if (term == ESC) return -1;
  confirm_each = (term == 'Y') ? TRUE : FALSE;
  if (!confirm_each) override_mode = 1;

  if( baudrate() >= QUICK_BAUD_RATE ) typeahead( 0 );

  for( i=0; i < (int)CurrentVolume->file_count && result == 0; )
  {
    deleted = FALSE;

    fe_ptr = CurrentVolume->file_entry_list[i].file;
    de_ptr = fe_ptr->dir_entry;

    if( fe_ptr->tagged && fe_ptr->matching )
    {
      /* Spinner to indicate progress during bulk deletion */
      DrawSpinner();
      doupdate();
      start_file = MAX( 0, i - max_disp_files + 1 );
      cursor_pos = i - start_file;

      DisplayFiles( de_ptr,
		    start_file,
		    start_file + cursor_pos,
		    start_x
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
  SetFileMode(file_mode);

  GetMaxYX(file_window, &height, &width);
  x_step =  (max_column > 1) ? height : 1;
  max_disp_files = height * max_column;


  if( dir_entry->start_file + dir_entry->cursor_pos < (int)CurrentVolume->file_count )
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
   int incremental = (!strcmp(LISTJUMPSEARCH, "1")) ? 1 : 0; /* from ~/.ytree */

    /*  in file_window press initial char of file to jump to it */

    char *newStr = NULL;
    FileEntry * fe_ptr = NULL;
    int i=0, j=0, n=0, start_x=0, ic=0, tmp2=0;
    char * jumpmsg = "Press initial of file to jump to... ";

    ClearHelp();
    MvAddStr( Y_PROMPT, 1, jumpmsg );
    PrintOptions
    (
        stdscr,
        Y_PROMPT,
        COLS - 14,
        "(Escape) cancel"
    );

    ic = tolower(Getch());

    if( !isprint(ic) )
    {
        return;
    }

    n = strlen(str);
    if((newStr = (char *)malloc(n+2)) == NULL) {
      ERROR_MSG( "Malloc failed*ABORT" );
      exit( 1 );
    }
    strcpy(newStr, str);
    newStr[n] = ic;
    newStr[n+1] = '\0';

    /* index of current entry in list */
    tmp2 = (incremental && n == 0) ? 0 : dir_entry->start_file + dir_entry->cursor_pos;

    if( tmp2 == (int)CurrentVolume->file_count - 1 )
    {
        ClearHelp();
        MvAddStr( Y_PROMPT, 1, "Last entry!");
        RefreshWindow( stdscr );
        RefreshWindow( file_window );
        doupdate();
        sleep(1);
        free(newStr);
        return;
    }

    for( i=tmp2; i < (int)CurrentVolume->file_count; i++ )
    {
        fe_ptr = CurrentVolume->file_entry_list[i].file;
	if(!strncasecmp(newStr, fe_ptr->name, n+1))
          break;
    }

    if ( i == (int)CurrentVolume->file_count )
    {
        ClearHelp();
        MvAddStr( Y_PROMPT, 1, "No match!");
        RefreshWindow( stdscr );
        RefreshWindow( file_window );
        doupdate();
        sleep(1);
        free(newStr);
        return;
    }

    /* position cursor on entry wanted and found */
    if( incremental && n == 0 ) {
      	/* first search start on top */
      	dir_entry->start_file = 0;
      	dir_entry->cursor_pos = 0;
      	DisplayFiles( dir_entry,
            	dir_entry->start_file,
            	dir_entry->start_file + dir_entry->cursor_pos,
            	start_x
          	);
    }
    for ( j=tmp2; j < i; j++ )
        fmovedown
        (
            &dir_entry->start_file,
            &dir_entry->cursor_pos,
            &start_x,
            dir_entry
        );
    RefreshWindow( stdscr );
    RefreshWindow( file_window );
    doupdate();
    ListJump( dir_entry, (incremental) ? newStr : "" );
    free(newStr);
}