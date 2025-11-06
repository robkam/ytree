/***************************************************************************
 *
 * Funktionen zur Handhabung des FILE-Windows
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

static int  window_height;
static int  window_width;
static int  max_disp_files;
static int  x_step;
static int  my_x_step;
static int  hide_left;
static int  hide_right;

static FileEntryList *file_entry_list;
static unsigned      file_count;
static unsigned      max_visual_userview_len;
static unsigned      max_visual_filename_len;
static unsigned      max_visual_linkname_len;
static unsigned      global_max_visual_filename_len;
static unsigned      global_max_visual_linkname_len;

static void ReadFileList(BOOL tagged_only, DirEntry *dir_entry);
static void SortFileEntryList(void);
static int  SortByName(FileEntryList *e1, FileEntryList *e2);
static int  SortByChgTime(FileEntryList *e1, FileEntryList *e2);
static int  SortByAccTime(FileEntryList *e1, FileEntryList *e2);
static int  SortByModTime(FileEntryList *e1, FileEntryList *e2);
static int  SortBySize(FileEntryList *e1, FileEntryList *e2);
static int  SortByOwner(FileEntryList *e1, FileEntryList *e2);
static int  SortByGroup(FileEntryList *e1, FileEntryList *e2);
static int  SortByExtension(FileEntryList *e1, FileEntryList *e2);
static void DisplayFiles(DirEntry *de_ptr, int start_file_no, int hilight_no, int start_x);
static void ReadGlobalFileList(BOOL tagged_only, DirEntry *dir_entry);
static void WalkTaggedFiles(int start_file, int cursor_pos, int (*fkt) (FileEntry *, WalkingPackage *), WalkingPackage *walking_package);
static BOOL IsMatchingTaggedFiles(void);
static void RemoveFileEntry(int entry_no);
static void ChangeFileEntry(void);
static int  DeleteTaggedFiles(int max_dispfiles);
static void SilentWalkTaggedFiles( int (*fkt) (FileEntry *, WalkingPackage *),
			           WalkingPackage *walking_package
			          );
static void SilentTagWalkTaggedFiles( int (*fkt) (FileEntry *, WalkingPackage *),
			           WalkingPackage *walking_package
			          );
static void RereadWindowSize(DirEntry *dir_entry);
static void ListJump( DirEntry * dir_entry, char *str );
static char GetTypeOfFile(struct stat fst);
static int  GetVisualFileEntryLength(int mode, int max_visual_filename_len, int max_visual_linkname_len);



void SetFileMode(int new_file_mode)
{

  GetMaxYX( file_window, &window_height, &window_width );
  file_mode = new_file_mode;

  max_column = window_width / 
	       (GetVisualFileEntryLength( file_mode, max_visual_filename_len, max_visual_linkname_len) + 1);

  if( max_column == 0 ) 
    max_column = 1;
}


static int GetVisualFileEntryLength(int mode, int max_visual_filename_len, int max_visual_linkname_len)
{
  int len = 0;

  switch (file_mode)
  {
    case MODE_1: len =  (max_visual_linkname_len) ? max_visual_linkname_len + 4 : 0; /* linkname + " -> " */
		 len += max_visual_filename_len + 38; /* filename + format */
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
		 len += max_visual_filename_len + 39; /* filename + format */
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



static void BuildFileEntryList(DirEntry *dir_entry){

  if( file_entry_list ) {
    free( file_entry_list );
    file_entry_list = NULL;
  }

  if( !dir_entry->global_flag )  {
    /* ... for !ANSI-Systeme ... */
    /*---------------------------*/
    if( dir_entry->matching_files > 0 ) {

        if( ( file_entry_list = (FileEntryList *) 
	  		      calloc( dir_entry->matching_files,
				      sizeof( FileEntryList )
				    ) 
                              ) == NULL ) {
        ERROR_MSG( "Calloc Failed*ABORT" );
        exit( 1 );
        }
    }
    file_count = 0;
    ReadFileList( dir_entry->tagged_flag, dir_entry );
    SortFileEntryList();
    SetFileMode( file_mode ); /* recalc */

    } else {
  
    if( statistic.disk_matching_files > 0 ) {

        if( ( file_entry_list = (FileEntryList *) 
           calloc( (dir_entry->tagged_flag) ? statistic.disk_tagged_files : statistic.disk_matching_files,
				      sizeof( FileEntryList )
              ) ) == NULL )  {
           ERROR_MSG( "Calloc Failed*ABORT" );
           exit( 1 );
           }
        }
        file_count = 0;
        global_max_visual_filename_len = 0;
        global_max_visual_linkname_len = 0;
        ReadGlobalFileList(  dir_entry->tagged_flag, statistic.tree );
        SortFileEntryList();
        SetFileMode( file_mode ); /* recalc */
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
      file_entry_list[file_count++].file = fe_ptr;
      visual_name_len = StrVisualLength( fe_ptr->name );
      name_len = strlen( fe_ptr->name );
      if( S_ISLNK( fe_ptr->stat_struct.st_mode ) )
      {
	visual_linkname_len = StrVisualLength( &fe_ptr->name[name_len+1] );
	max_visual_linkname_len = MAX( max_visual_linkname_len, visual_linkname_len );
      }
      max_visual_filename_len = MAX( max_visual_filename_len, visual_name_len );
    }
  }
}



static void ReadGlobalFileList(BOOL tagged_only, DirEntry *dir_entry)
{
  DirEntry  *de_ptr;

  for( de_ptr=dir_entry; de_ptr; de_ptr=de_ptr->next )
  {
    if( de_ptr->sub_tree ) ReadGlobalFileList( tagged_only, de_ptr->sub_tree );
    ReadFileList( tagged_only, de_ptr );
    global_max_visual_filename_len = MAX( global_max_visual_filename_len, max_visual_filename_len );
    global_max_visual_linkname_len = MAX( global_max_visual_linkname_len, max_visual_linkname_len );
  }
  max_visual_filename_len = global_max_visual_filename_len;
  max_visual_linkname_len = global_max_visual_linkname_len;
}




static void SortFileEntryList(void)
{
  int aux;
  int (*compare)(FileEntryList *, FileEntryList *);

  reverse_sort = FALSE;
  if ((aux = statistic.kind_of_sort) > SORT_DSC)
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
    default:                 compare = SortByName; beep();
  }

  qsort( (char *) file_entry_list, 
	 file_count,
	 sizeof( file_entry_list[0] ),
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
    (void) sprintf( n1, "%d", (int) e1->file->stat_struct.st_uid );
    o1 = n1;
  }

  if( o2 == NULL ) 
  {
    (void) sprintf( n2, "%d", (int) e2->file->stat_struct.st_uid );
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
    (void) sprintf( n1, "%d", (int) e1->file->stat_struct.st_uid );
    g1 = n1;
  }

  if( g2 == NULL ) 
  {
    (void) sprintf( n2, "%d", (int) e2->file->stat_struct.st_uid );
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



void SetKindOfSort(int new_kind_of_sort)
{
  statistic.kind_of_sort = new_kind_of_sort;
}



static void RemoveFileEntry(int entry_no)
{
  int i, n;
  FileEntry *fe_ptr;
  int visual_name_len, name_len;

  max_visual_filename_len = 0;
  max_visual_linkname_len = 0;
  n = file_count - 1;

  for( i=0; i < n; i++ )
  {
    if( i >= entry_no ) file_entry_list[i] = file_entry_list[i+1];    
    fe_ptr = file_entry_list[i].file;
    visual_name_len = StrVisualLength( fe_ptr->name );
    name_len = strlen( fe_ptr->name );
    max_visual_filename_len = MAX( (int)max_visual_filename_len, visual_name_len );
    if( S_ISLNK( fe_ptr->stat_struct.st_mode ) )
    {
      max_visual_linkname_len = MAX( max_visual_filename_len, StrVisualLength( &fe_ptr->name[name_len+1] ) );
    }
  }

  SetFileMode( file_mode ); /* recalc */

  file_count--; /* no realloc */
}



static void ChangeFileEntry(void)
{
  int i, n;
  FileEntry *fe_ptr;
  int visual_name_len, name_len;

  max_visual_filename_len = 0;
  max_visual_linkname_len = 0;
  n = file_count - 1;

  for( i=0; i < n; i++ )
  {
    fe_ptr = file_entry_list[i].file;
    if( fe_ptr )
    {
      visual_name_len = StrVisualLength( fe_ptr->name );
      name_len = strlen( fe_ptr->name );
      max_visual_filename_len = MAX( (int)max_visual_filename_len, visual_name_len );
      if( S_ISLNK( fe_ptr->stat_struct.st_mode ) )
      {
        max_visual_linkname_len = MAX( max_visual_filename_len, StrVisualLength( &fe_ptr->name[name_len+1] ) );
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
  char modify_time[13];
  char change_time[13];
  char access_time[13];
  char format[60];
  char justify;
  char *line_ptr;
  int  n, pos_x = 0;
  FileEntry *fe_ptr;
  static char *line_buffer = NULL;
  static int  old_cols = -1;
  char owner[OWNER_NAME_MAX + 1];
  char group[GROUP_NAME_MAX + 1];
  char *owner_name_ptr;
  char *group_name_ptr;
  int  ef_window_width;
  char *sym_link_name = NULL;
  char type_of_file = ' ';
  int  filename_width = 0;
  int  linkname_width = 0;


  ef_window_width = window_width - 2; /* Effektive Window-Width */

  (reverse_sort) ? (justify='+') : (justify='-');

  if( old_cols != COLS )
  {
    old_cols = COLS;
    if( line_buffer ) free( line_buffer );

    if( ( line_buffer = (char *) malloc( COLS + PATH_LENGTH ) ) == NULL )
    {
      ERROR_MSG( "Malloc failed*ABORT" );
      exit( 1 );
    }
  }
  
  fe_ptr = file_entry_list[entry_no].file;

  if( fe_ptr && S_ISLNK( fe_ptr->stat_struct.st_mode ) )
    sym_link_name = &fe_ptr->name[strlen(fe_ptr->name)+1];
  else
    sym_link_name = "";


#ifdef WITH_UTF8
  filename_width = max_visual_filename_len + ( strlen(fe_ptr->name) - StrVisualLength(fe_ptr->name) );
  if( fe_ptr && S_ISLNK( fe_ptr->stat_struct.st_mode ) )
    linkname_width = max_visual_linkname_len + ( strlen(sym_link_name) - StrVisualLength(sym_link_name) );
#else
  filename_width = max_visual_filename_len;
  linkname_width = max_visual_linkname_len;
#endif


  type_of_file = GetTypeOfFile(fe_ptr->stat_struct);
  
  switch( file_mode )
  {
    case MODE_1 : if( fe_ptr )
		  {
		    (void) GetAttributes( fe_ptr->stat_struct.st_mode,
		                          attributes
				        );
		        
		    (void) CTime( fe_ptr->stat_struct.st_mtime, modify_time );
  
		    
		    
                    if( S_ISLNK( fe_ptr->stat_struct.st_mode ) )
		    {
#ifdef HAS_LONGLONG
		      (void) sprintf( format, "%%c%%c%%-%ds %%10s %%3d %%11lld %%12s -> %%-%ds", 
				      filename_width,
				      linkname_width
				    );
		      
		      (void) sprintf( line_buffer, format,
				      (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
				      type_of_file, 
				      fe_ptr->name,
				      attributes,
				      fe_ptr->stat_struct.st_nlink,
                                      (LONGLONG) fe_ptr->stat_struct.st_size,
				      modify_time,
				      sym_link_name
				    );
#else
		      (void) sprintf( format, "%%c%%c%%-%ds %%10s %%3d %%7d %%12s -> %%-%ds", 
				      filename_width,
				      linkname_width
				    );
    
		      (void) sprintf( line_buffer, format,
				      (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
				      type_of_file,
				      fe_ptr->name,
				      attributes,
				      fe_ptr->stat_struct.st_nlink,
                                      fe_ptr->stat_struct.st_size,
				      modify_time,
				      sym_link_name
				    );
#endif
                    }
		    else
		    {
#ifdef HAS_LONGLONG
		      (void) sprintf( format, "%%c%%c%%%c%ds %%10s %%3d %%11lld %%12s", 
                                      justify,
				      filename_width 
				    );
    
		      (void) sprintf( line_buffer, format,
				      (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
				      type_of_file,
				      fe_ptr->name,
				      attributes,
				      fe_ptr->stat_struct.st_nlink,
                                      (LONGLONG) fe_ptr->stat_struct.st_size,
				      modify_time
				    );
#else
		      (void) sprintf( format, "%%c%%c%%%c%ds %%10s %%3d %%7d %%12s", 
                                      justify,
				      filename_width 
				    );
    
		      (void) sprintf( line_buffer, format,
				      (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
				      type_of_file,
				      fe_ptr->name,
				      attributes,
				      fe_ptr->stat_struct.st_nlink,
                                      fe_ptr->stat_struct.st_size,
				      modify_time
				    );
#endif
                    }				    
		  }
		  else
		  {
		    /* Empty Entry */
		    /*-------------*/

		    (void) sprintf( format, "%%-%ds", max_visual_filename_len + 42 );
		    (void) sprintf( line_buffer, format, "" );
		  }

		  if( max_visual_linkname_len ) 
		    pos_x = x * (max_visual_filename_len + max_visual_linkname_len + 47); 
		  else                   
		    pos_x = x * (max_visual_filename_len + 43); 
		  break;
    
    case MODE_2 : if( fe_ptr )
		  {
		    (void) GetAttributes( fe_ptr->stat_struct.st_mode,
		                          attributes
				        );
		        
                    owner_name_ptr = GetDisplayPasswdName(fe_ptr->stat_struct.st_uid);
                    group_name_ptr = GetDisplayGroupName(fe_ptr->stat_struct.st_gid);
		      
		    if( owner_name_ptr == NULL )
		    {
		      (void) sprintf( owner, "%d", (int) fe_ptr->stat_struct.st_uid );
		      owner_name_ptr = owner;
		    }
		    if( group_name_ptr == NULL )
		    {
		      (void) sprintf( group, "%d", (int) fe_ptr->stat_struct.st_gid );
		      group_name_ptr = group;
		    }
  
                    if( S_ISLNK( fe_ptr->stat_struct.st_mode ) )
		    {
#ifdef HAS_LONGLONG
                      (void) sprintf( format, "%%c%%c%%%c%ds %%10lld %%-12s %%-12s -> %%-%ds",
                                      justify,
			              filename_width,
			              linkname_width
				      );
		      (void) sprintf( line_buffer, format,
				      (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
				      type_of_file,
				      fe_ptr->name,
				      (LONGLONG)fe_ptr->stat_struct.st_ino,
				      owner_name_ptr,
				      group_name_ptr,
				      sym_link_name
				    );
#else
                      (void) sprintf( format, "%%c%%c%%%c%ds  %%8u  %%-12s %%-12s -> %%-%ds",
                                      justify,
			              filename_width,
			              linkname_width
				      );
		      (void) sprintf( line_buffer, format,
				      (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
				      type_of_file,
				      fe_ptr->name,
				      (int)fe_ptr->stat_struct.st_ino,
				      owner_name_ptr,
				      group_name_ptr,
				      sym_link_name
				    );
#endif
                    }
		    else
		    {
#ifdef HAS_LONGLONG
                      (void) sprintf( format, "%%c%%c%%%c%ds %%10lld %%-12s %%-12s",
                                      justify,
			              filename_width
				      );
		      (void) sprintf( line_buffer, format,
				      (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
				      type_of_file,
				      fe_ptr->name,
				      (LONGLONG)fe_ptr->stat_struct.st_ino,
				      owner_name_ptr,
				      group_name_ptr
				    );
#else
                      (void) sprintf( format, "%%c%%c%%%c%ds  %%8u  %%-12s %%-12s",
                                      justify,
			              filename_width
				      );
		      (void) sprintf( line_buffer, format,
				      (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
				      type_of_file,
				      fe_ptr->name,
				      (int)fe_ptr->stat_struct.st_ino,
				      owner_name_ptr,
				      group_name_ptr
				    );
#endif

                    }				    
	          }
		  else
		  {
		    /* Empty-Entry */
		    /*-------------*/

		    (void) sprintf( format, "%%-%ds", max_visual_filename_len + 38 );
		    (void) sprintf( line_buffer, format, "" );
		  }

		  if( max_visual_linkname_len ) 
                    pos_x = x * (max_visual_filename_len + max_visual_linkname_len + 43);
		  else                   
                    pos_x = x * (max_visual_filename_len + 39);
		  break;
    
    case MODE_3 : if( fe_ptr )
		  {
		    (void) sprintf( format, "%%c%%c%%%c%ds", 
                                    justify, 
                                    filename_width );

		    (void) sprintf( line_buffer, format,
				    (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
				    type_of_file,
				    fe_ptr->name
				  );
                  }
		  else
		  {
		    /* Empty-Entry */
		    /*-------------*/

		    (void) sprintf( format, "%%-%ds", max_visual_filename_len + 2 );
		    (void) sprintf( line_buffer, format, "" );
		  }

		  pos_x = x * (max_visual_filename_len + 3); 
		  break;
    
    case MODE_4 : if( fe_ptr )
		  {
		    (void) CTime( fe_ptr->stat_struct.st_ctime, change_time );
		    (void) CTime( fe_ptr->stat_struct.st_atime, access_time );
  
                    if( S_ISLNK( fe_ptr->stat_struct.st_mode ) )
		    {
                      (void) sprintf( format, "%%c%%c%%%c%ds Chg: %%12s  Acc: %%12s -> %%-%ds",
                                      justify,
				      filename_width,
				      linkname_width
				    );
		      (void) sprintf( line_buffer, format,
				      (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
				      type_of_file,
				      fe_ptr->name,
				      change_time,
				      access_time,
				      sym_link_name
				  );
                    }
		    else
		    {
                      (void) sprintf( format, "%%c%%c%%%c%ds Chg: %%12s  Acc: %%12s",
                                      justify,
				      filename_width
				    );
		      (void) sprintf( line_buffer, format,
				      (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ',
				      type_of_file,
				      fe_ptr->name,
				      change_time,
				      access_time
				  );
                    }				    
		  }
		  else
		  {
		    /* Empty-Entry */
		    /*-------------*/

		    (void) sprintf( format, "%%-%ds", max_visual_filename_len + 39 );
		    (void) sprintf( line_buffer, format, "" );
		  }


		  if( max_visual_linkname_len ) 
		    pos_x = x * (max_visual_filename_len + max_visual_linkname_len + 44); 
		  else                   
		    pos_x = x * (max_visual_filename_len + 40); 
		  break;
    
    case MODE_5 : if( fe_ptr )
		  {
 		    BuildUserFileEntry(fe_ptr,  filename_width, linkname_width, 
		        USERVIEW, 
		        200, line_buffer);
		  }
		  else
		  {
		    /* Empty-Entry */
		    /*-------------*/

		    (void) sprintf( format, "%%-%ds", max_visual_userview_len );
		    (void) sprintf( line_buffer, format, "" );
		  }
		  pos_x = x * (max_visual_userview_len + 1);
		  break;

  }

  /* display line */
  /*--------------*/

  n = StrVisualLength( line_buffer );
  
  if( n <= ef_window_width )
  {
    /* line fits */
    /*-----------*/

    hide_left = 0;
    hide_right = 0;
    line_ptr = line_buffer;
  }
  else
  {
    int line_end_pos;

    /* ... does not fit; use start_x */
    /*-------------------------------*/
    
    if( n > ( start_x + ef_window_width ) )
      line_ptr = &line_buffer[VisualPositionToBytePosition(line_buffer, start_x)];
    else
      line_ptr = &line_buffer[VisualPositionToBytePosition(line_buffer, n - ef_window_width)];

    hide_left = start_x;
    line_end_pos = VisualPositionToBytePosition(line_ptr, ef_window_width);
    hide_right = StrVisualLength(&line_ptr[line_end_pos]);
    line_ptr[line_end_pos] = '\0';
  }
 
#ifdef NO_HIGHLIGHT
  line_ptr[1] = (hilight) ? '>' : ' ';
  mvwaddstr( file_window, y, pos_x + 1, line_ptr );
#else
#ifdef COLOR_SUPPORT
  if( hilight )
    WbkgdSet(file_window, COLOR_PAIR(HIFILE_COLOR)|A_BOLD);
  else
    WbkgdSet(file_window, COLOR_PAIR(FILE_COLOR));

  mvwaddstr(file_window, y, pos_x + 1, line_ptr );
  WbkgdSet(file_window, COLOR_PAIR(FILE_COLOR)|A_BOLD);

#else
#endif /* COLOR_SUPPORT */
  if( hilight ) wattrset( file_window, A_REVERSE );
  mvwaddstr( file_window, y, pos_x + 1, line_ptr );
  if( hilight ) wattrset( file_window, 0 );
#endif /* NO_HIGHLIGHT */
 
#ifdef WITH_UTF8
  if(x_step == 1 && start_x > 0)
    wclrtoeol(file_window); /* maybe needed if filename contains wide characters */
#endif
}





void DisplayFileWindow(DirEntry *dir_entry)
{
  GetMaxYX( file_window, &window_height, &window_width );
  BuildFileEntryList( dir_entry );
  DisplayFiles( dir_entry, 
		dir_entry->start_file, 
                dir_entry->start_file + dir_entry->cursor_pos, 0);
}




static void DisplayFiles(DirEntry *de_ptr, int start_file_no, int hilight_no, int start_x)
{
  int  x, y, p_x, p_y, j;

  werase( file_window );

  if( file_count == 0 )
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
    for( y=0; y < window_height; y++ )
    {
      if( j < (int)file_count )
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

}


static void fmovedown(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry)
{
   if( *start_file + *cursor_pos + 1 >= (int)file_count )
   {
      /* File nicht vorhanden */
      /*----------------------*/
      beep();
   }
   else 
   {
      if( *cursor_pos < max_disp_files - 1 )
      {
          /* DOWN ohne scroll moeglich */
          /*---------------------------*/
          PrintFileEntry( *start_file + *cursor_pos,
                          *cursor_pos % window_height,
                          *cursor_pos / window_height,
                          FALSE,
                          *start_x
                          );
          (*cursor_pos)++;
          PrintFileEntry( *start_file + *cursor_pos,
                          *cursor_pos % window_height,
                          *cursor_pos / window_height,
                          TRUE ,
                          *start_x
                          );
      }
      else
      {
          /* Scrollen */
          /*----------*/
          (*start_file)++;
          DisplayFiles( dir_entry, 
                        *start_file, 
                        *start_file + *cursor_pos,
                        *start_x
                        );
      }
   }
   return;
}

static void fmoveup(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry)
{
   if( *start_file + *cursor_pos < 1 )
   {
      /* File nicht vorhanden */
      /*----------------------*/
      beep();
   }
   else
   {
      if( *cursor_pos > 0 )
      {
         /* UP ohne scroll moeglich */
         /*-------------------------*/
         PrintFileEntry( *start_file + *cursor_pos,
                         *cursor_pos % window_height,
                         *cursor_pos / window_height,
                         FALSE,
                         *start_x
                         );
         (*cursor_pos)--;
         PrintFileEntry( *start_file + *cursor_pos,
                         *cursor_pos % window_height,
                         *cursor_pos / window_height,
                         TRUE,
                         *start_x
                         );
      }
      else
      {
         /* Scrollen */
         /*----------*/
         (*start_file)--;
         DisplayFiles( dir_entry, 
                       *start_file, 
                       *start_file + *cursor_pos,
                       *start_x
                       );
      }
   }
   return;
}
      
static void fmoveright(int *start_file, int *cursor_pos, int *start_x,DirEntry *dir_entry)
{
   if( x_step == 1 )
   {
      /* Sonderfall: ganzes Filewindow scrollen */
      /*----------------------------------------*/
      (*start_x)++;
      PrintFileEntry( *start_file + *cursor_pos,
                      *cursor_pos % window_height,
                      *cursor_pos / window_height,
                      TRUE ,
                      *start_x
                      );
      if( hide_right <= 0 ) (*start_x)--;
   }
   else if( *start_file + *cursor_pos >= (int)file_count - 1 )
   {
      /*letzte Position erreicht */
      /*-------------------------*/
      beep();
   }
   else 
   {
      if( *start_file + *cursor_pos + x_step >= (int)file_count )
      {
          /* voller Step nicht moeglich;
           * auf letzten Eintrag positionieren
           */
           my_x_step = file_count - *start_file - *cursor_pos - 1;
      }
      else
      {
          my_x_step = x_step;
      }
      if( *cursor_pos + my_x_step < max_disp_files )
      {
          /* RIGHT ohne scroll moeglich */
          /*----------------------------*/
          PrintFileEntry( *start_file + *cursor_pos,
                          *cursor_pos % window_height,
                          *cursor_pos / window_height,
                          FALSE,
                          *start_x
                          );
          *cursor_pos += my_x_step;
          PrintFileEntry( *start_file + *cursor_pos,
                          *cursor_pos % window_height,
                          *cursor_pos / window_height,
                          TRUE ,
                          *start_x
                          );
      }
      else
      {
          /* Scrollen */
          /*----------*/
          *start_file += x_step;
          *cursor_pos -= x_step - my_x_step;
          DisplayFiles( dir_entry, 
                        *start_file, 
                        *start_file + *cursor_pos,
                        *start_x
                        );
      }
   }
   return;
}


static void fmoveleft(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry)
{    
     if( x_step == 1 )
     {
         /* Sonderfall: ganzes Filewindow scrollen */
         /*----------------------------------------*/
         if( *start_x > 0 ) (*start_x)--;
            PrintFileEntry( *start_file + *cursor_pos,
                            *cursor_pos % window_height,
                            *cursor_pos / window_height,
                            TRUE ,
                            *start_x
                            );
     }
     else if( *start_file + *cursor_pos <= 0 )
     {
         /* erste Position erreicht */
         /*-------------------------*/
         beep();
     }
     else 
     {
         if( *start_file + *cursor_pos - x_step < 0 )
         {
             /* voller Step nicht moeglich;
              * auf ersten Eintrag positionieren
              */
              my_x_step = *start_file + *cursor_pos;
         }
         else
         {
             my_x_step = x_step;
         }
         if( *cursor_pos - my_x_step >= 0 )
         {
             /* LEFT ohne scroll moeglich */
             /*---------------------------*/
             PrintFileEntry( *start_file + *cursor_pos,
                             *cursor_pos % window_height,
                             *cursor_pos / window_height,
                             FALSE,
                             *start_x
                             );
             *cursor_pos -= my_x_step;
             PrintFileEntry( *start_file + *cursor_pos,
                             *cursor_pos % window_height,
                             *cursor_pos / window_height,
                             TRUE,
                             *start_x
                             );
         }
         else
         {
             /* Scrollen */
             /*----------*/
             if( ( *start_file -= x_step ) < 0 )
                *start_file = 0;
             DisplayFiles( dir_entry, 
                           *start_file, 
                           *start_file + *cursor_pos,
                           *start_x
                           );
         }
     }
     return;
}


static void fmovenpage(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry)
{
   if( *start_file + *cursor_pos >= (int)file_count - 1 )
   {
      /*letzte Position erreicht */
      /*-------------------------*/
      beep();
   }
   else 
   {
      if( *cursor_pos < max_disp_files - 1 )
      {
        /* Cursor steht noch nicht auf letztem
         * Eintrag 
         * ==> setzen
         */
         PrintFileEntry( *start_file + *cursor_pos,
                         *cursor_pos % window_height,
                         *cursor_pos / window_height,
                         FALSE,
                         *start_x
                         );
         if( *start_file + max_disp_files <= (int)file_count - 1 )
            *cursor_pos = max_disp_files - 1;
         else
            *cursor_pos = file_count - *start_file - 1;
         PrintFileEntry( *start_file + *cursor_pos,
                         *cursor_pos % window_height,
                         *cursor_pos / window_height,
                         TRUE,
                         *start_x
                         );
      }
      else
      {
        /* Scrollen */
        /*----------*/
        if( *start_file + *cursor_pos + max_disp_files < (int)file_count )
           *start_file += max_disp_files;
        else
           *start_file = file_count - max_disp_files;
        if( *start_file + max_disp_files <= (int)file_count - 1 )
           *cursor_pos = max_disp_files - 1;
        else
           *cursor_pos = file_count - *start_file - 1;
        DisplayFiles( dir_entry, 
                      *start_file, 
                      *start_file + *cursor_pos,
                      *start_x
                      );
      }
   }
   return;
}



static void fmoveppage(int *start_file, int *cursor_pos, int *start_x, DirEntry *dir_entry)
{    
     if( *start_file + *cursor_pos <= 0 )
     {
        /* erste Position erreicht */
        /*-------------------------*/
        beep();
     }
     else 
     {
        if( *cursor_pos > 0 )
        {
            /* Cursor steht noch nicht auf erstem
             * Eintrag 
             * ==> setzen
             */
             PrintFileEntry( *start_file + *cursor_pos,
                             *cursor_pos % window_height,
                             *cursor_pos / window_height,
                             FALSE,
                             *start_x
                             );
             *cursor_pos = 0;
             PrintFileEntry( *start_file + *cursor_pos,
                             *cursor_pos % window_height,
                             *cursor_pos / window_height,
                             TRUE,
                             *start_x
                             );
        }
        else
        {
            /* Scrollen */
            /*----------*/
            if( *start_file > max_disp_files )
               *start_file -= max_disp_files;
            else
               *start_file = 0;
            DisplayFiles( dir_entry, 
                          *start_file, 
                          *start_file + *cursor_pos,
                          *start_x
                          );
        }
     }
     return;
}




int HandleFileWindow(DirEntry *dir_entry)
{
  FileEntry *fe_ptr;
  FileEntry *new_fe_ptr;
  DirEntry  *de_ptr = NULL;
  DirEntry  *dest_dir_entry;
  WalkingPackage walking_package;
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
  char new_login_path[PATH_LENGTH + 1];
  int  dir_window_width, dir_window_height;


  unput_char = '\0';
  fe_ptr = NULL;


  /* Cursor-Positionsmerker zuruecksetzen */
  /*--------------------------------------*/

  need_dsp_help = TRUE;
  maybe_change_x_step = TRUE;

  BuildFileEntryList( dir_entry );
  
  if( dir_entry->global_flag || dir_entry->big_window || dir_entry->tagged_flag)
  {
    SwitchToBigFileWindow();
    GetMaxYX( file_window, &window_height, &window_width );
    DisplayDiskStatistic();
  }
  else
  {
    GetMaxYX( file_window, &window_height, &window_width );
    DisplayDirStatistic( dir_entry );
  }

  DisplayFiles( dir_entry, 
		dir_entry->start_file, 
		dir_entry->start_file + dir_entry->cursor_pos, 
		start_x
	      );

  do
  {
    if( maybe_change_x_step )
    {
      maybe_change_x_step = FALSE;
      
      x_step =  (max_column > 1) ? window_height : 1;
      max_disp_files = window_height * max_column;
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
      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
      
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

#ifdef VI_KEYS

    ch = ViKey( ch );

#endif /* VI_KEYS */

   if(resize_request) {
     ReCreateWindows();
     RereadWindowSize(dir_entry);
     DisplayMenu();

     GetMaxYX(dir_window, &dir_window_height, &dir_window_width);
     while(statistic.cursor_pos >= dir_window_height) {
       statistic.cursor_pos--;
       statistic.disp_begin_pos++;
     }
     if(dir_entry->global_flag || dir_entry->big_window || dir_entry->tagged_flag) {

       /* big window active */

       SwitchToBigFileWindow();
       DisplayFileWindow(dir_entry);

       if(dir_entry->global_flag) {
	 DisplayDiskStatistic();
	 DisplayGlobalFileParameter(fe_ptr);
       } else {
	 DisplayFileWindow(dir_entry);
	 DisplayDirStatistic(dir_entry);
	 DisplayFileParameter(fe_ptr);
       }
     } else {

       /* small window active */
	    
       SwitchToSmallFileWindow();
       DisplayTree( dir_window, statistic.disp_begin_pos,
		  statistic.disp_begin_pos + statistic.cursor_pos
		);
       DisplayFileWindow(dir_entry);
       DisplayDirStatistic(dir_entry);
       DisplayFileParameter(fe_ptr);
     }
     need_dsp_help = TRUE;
     DisplayAvailBytes();
     DisplayFileSpec();
     DisplayDiskName();
     resize_request = FALSE;
   }

   if( file_mode == MODE_1 )
   {
      if( ch == '\t' ) ch = KEY_DOWN;
      else if( ch == KEY_BTAB ) ch = KEY_UP;
   }

   if( x_step == 1 && ( ch == KEY_RIGHT || ch == KEY_LEFT ) )
   {
      /* start_x nicht zuruecksetzen */
      /*-----------------------------*/
      
      ; /* do nothing */
   }
   else
   {
      /* bei 0 beginnen */
      /*----------------*/
      
      if( start_x )
      {
	start_x = 0;
			
	PrintFileEntry( dir_entry->start_file + dir_entry->cursor_pos,
	 	        dir_entry->cursor_pos % window_height,
		        dir_entry->cursor_pos / window_height,
		        TRUE ,
		        start_x
	              );
     }
   }

   if (mode == USER_MODE) { /* FileUserMode returns (possibly remapped) ch, or -1 if it handles ch */
      ch = FileUserMode(&(file_entry_list[dir_entry->start_file + dir_entry->cursor_pos]), ch);
   }

   switch( ch )
   {

#ifdef KEY_RESIZE

      case KEY_RESIZE: resize_request = TRUE;
                       break;
#endif

      case -1:         break;

      case ' ' :   /*   break;  Quick-Key */

      case KEY_DOWN :  fmovedown(&dir_entry->start_file, &dir_entry->cursor_pos, &start_x, dir_entry);
		      break;

      case KEY_UP   : fmoveup(&dir_entry->start_file, &dir_entry->cursor_pos, &start_x, dir_entry);
		      break;
      
      case KEY_RIGHT: fmoveright(&dir_entry->start_file, &dir_entry->cursor_pos, &start_x, dir_entry);
		      break;
    
      case KEY_LEFT : fmoveleft(&dir_entry->start_file, &dir_entry->cursor_pos, &start_x, dir_entry);
		      break;
    
      case KEY_NPAGE: fmovenpage(&dir_entry->start_file, &dir_entry->cursor_pos, &start_x, dir_entry);
		      break;
    
      case KEY_PPAGE: fmoveppage(&dir_entry->start_file, &dir_entry->cursor_pos, &start_x, dir_entry);
		      break;
    
      case KEY_END  : if( dir_entry->start_file + dir_entry->cursor_pos + 1 >= (int)file_count )
		      {
			/* Letzte Position erreicht */
			/*--------------------------*/
			
			beep();
		      }
		      else
		      {
			if( (int)file_count < max_disp_files )
		        {
			  dir_entry->start_file = 0;
			  dir_entry->cursor_pos = file_count - 1;
		        }
		        else
	                {
                          dir_entry->start_file = file_count - max_disp_files;
			  dir_entry->cursor_pos = file_count - dir_entry->start_file - 1;
		        }

			DisplayFiles( dir_entry, 
				      dir_entry->start_file, 
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
				    );
		      }
		      break;
    
      case KEY_HOME : if( dir_entry->start_file + dir_entry->cursor_pos <= 0 )
		      {
			/* erste Position erreicht */
			/*-------------------------*/

			beep();
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
      
      case 'A' :
      case 'a' :      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;

	              need_dsp_help = TRUE;
		      
		      if( !ChangeFileModus( fe_ptr ) )
		      {
			PrintFileEntry( dir_entry->start_file + dir_entry->cursor_pos,
				        dir_entry->cursor_pos % window_height,
				        dir_entry->cursor_pos / window_height,
				        TRUE, 
					start_x
			              );
		      }
		      break;

      case 'A' & 0x1F :
		      if( (mode != DISK_MODE && mode != USER_MODE) || !IsMatchingTaggedFiles() )
		      {
			beep();
		      }
		      else
		      {
			need_dsp_help = TRUE;
		
		   	mask = S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;

			(void) GetAttributes( mask, modus );

		        if( GetNewFileModus( LINES - 2, 1, modus, "\r\033" ) == CR )
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
			else
			{
			  beep();
			}
		      }
		      break;

      case 'O' :
      case 'o' :      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;

		      need_dsp_help = TRUE;
		      
		      if( !ChangeFileOwner( fe_ptr ) )
		      {
			PrintFileEntry( dir_entry->start_file + dir_entry->cursor_pos,
				        dir_entry->cursor_pos % window_height,
				        dir_entry->cursor_pos / window_height,
				        TRUE ,
					start_x
			              );
		      }
		      break;

      case 'O' & 0x1F :
		      if(( mode != DISK_MODE && mode != USER_MODE) || !IsMatchingTaggedFiles() )
		      {
			beep();
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

      case 'G' :
      case 'g' :      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;

		      need_dsp_help = TRUE;
		      
		      if( !ChangeFileGroup( fe_ptr ) )
		      {
			PrintFileEntry( dir_entry->start_file + dir_entry->cursor_pos,
				        dir_entry->cursor_pos % window_height,
				        dir_entry->cursor_pos / window_height,
				        TRUE, 
					start_x
			              );
		      }
		      break;

      case 'G' & 0x1F :
		      if(( mode != DISK_MODE && mode != USER_MODE) || !IsMatchingTaggedFiles() )
		      {
			beep();
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

      case 'T' :
      case 't' :      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;

		      if( !fe_ptr->tagged )
		      {
                        fe_ptr->tagged = TRUE;
		        
			PrintFileEntry( dir_entry->start_file + dir_entry->cursor_pos,
				        dir_entry->cursor_pos % window_height,
				        dir_entry->cursor_pos / window_height,
				        TRUE,
					start_x
			              );
	       	        de_ptr->tagged_files++;
		        de_ptr->tagged_bytes += fe_ptr->stat_struct.st_size;
	       	        statistic.disk_tagged_files++;
		        statistic.disk_tagged_bytes += fe_ptr->stat_struct.st_size;
			if( dir_entry->global_flag ) 
			  DisplayDiskTagged();
			else              
			  DisplayDirTagged( de_ptr );
		      }
		      unput_char = KEY_DOWN;
                        
                      break;
      case 'U' :
      case 'u' :      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;
                      if( fe_ptr->tagged )
		      {
			fe_ptr->tagged = FALSE;
		      
			PrintFileEntry( dir_entry->start_file + dir_entry->cursor_pos,
				        dir_entry->cursor_pos % window_height,
				        dir_entry->cursor_pos / window_height,
				        TRUE, 
					start_x
			              );
			
			de_ptr->tagged_files--;
			de_ptr->tagged_bytes -= fe_ptr->stat_struct.st_size;
			statistic.disk_tagged_files--;
			statistic.disk_tagged_bytes -= fe_ptr->stat_struct.st_size;
			if( dir_entry->global_flag ) 
			  DisplayDiskTagged();
			else              
			  DisplayDirTagged( de_ptr );
		      }
		      
		      unput_char = KEY_DOWN;
                      
		      break;
      
      case 'F' & 0x1F : 
		      list_pos = dir_entry->start_file + dir_entry->cursor_pos;

		      RotateFileMode();

                      x_step =  (max_column > 1) ? window_height : 1;
                      max_disp_files = window_height * max_column;

		      if( dir_entry->cursor_pos >= max_disp_files )
		      {
			/* Cursor muss neu positioniert werden */
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
      
      case 'T' & 0x1F : 
                      for(i=0; i < (int)file_count; i++)
                      {
			fe_ptr = file_entry_list[i].file;
			de_ptr = fe_ptr->dir_entry;

			if( !fe_ptr->tagged )
			{
			  file_size = fe_ptr->stat_struct.st_size;

			  fe_ptr->tagged = TRUE;
			  de_ptr->tagged_files++;
			  de_ptr->tagged_bytes += file_size;
			  statistic.disk_tagged_files++;
			  statistic.disk_tagged_bytes += file_size;
		        }
		      }

		      if( dir_entry->global_flag ) 
		        DisplayDiskTagged();
		      else              
		        DisplayDirTagged( dir_entry );
		      
		      DisplayFiles( dir_entry, 
				    dir_entry->start_file, 
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x
			          );
		      break;


      case 'U' & 0x1F : 
                      for(i=0; i < (int)file_count; i++)
                      {
			fe_ptr = file_entry_list[i].file;
			de_ptr = fe_ptr->dir_entry;

			if( fe_ptr->tagged )
			{
			  file_size = fe_ptr->stat_struct.st_size;

			  fe_ptr->tagged = FALSE;
			  de_ptr->tagged_files--;
			  de_ptr->tagged_bytes -= file_size;
			  statistic.disk_tagged_files--;
			  statistic.disk_tagged_bytes -= file_size;
		        }
		      }

		      if( dir_entry->global_flag ) 
		        DisplayDiskTagged();
		      else              
		        DisplayDirTagged( dir_entry );
		      
		      DisplayFiles( dir_entry, 
				    dir_entry->start_file, 
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x
			          );
		      break;



      case ';': 
      case 't' | 0x80 : 
                      for(i=dir_entry->start_file + dir_entry->cursor_pos; i < (int)file_count; i++)
                      {
			fe_ptr = file_entry_list[i].file;
			de_ptr = fe_ptr->dir_entry;

			if( !fe_ptr->tagged )
			{
			  file_size = fe_ptr->stat_struct.st_size;

			  fe_ptr->tagged = TRUE;
			  de_ptr->tagged_files++;
			  de_ptr->tagged_bytes += file_size;
			  statistic.disk_tagged_files++;
			  statistic.disk_tagged_bytes += file_size;
		        }
		      }

		      if( dir_entry->global_flag ) 
		        DisplayDiskTagged();
		      else              
		        DisplayDirTagged( dir_entry );
		      
		      DisplayFiles( dir_entry, 
				    dir_entry->start_file, 
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x
			          );
		      break;


      case ':': 
      case 'u' | 0x80 : 
                      for(i=dir_entry->start_file + dir_entry->cursor_pos; i < (int)file_count; i++)
                      {
			fe_ptr = file_entry_list[i].file;
			de_ptr = fe_ptr->dir_entry;

			if( fe_ptr->tagged )
			{
			  file_size = fe_ptr->stat_struct.st_size;

			  fe_ptr->tagged = FALSE;
			  de_ptr->tagged_files--;
			  de_ptr->tagged_bytes -= file_size;
			  statistic.disk_tagged_files--;
			  statistic.disk_tagged_bytes -= file_size;
		        }
		      }

		      if( dir_entry->global_flag ) 
		        DisplayDiskTagged();
		      else              
		        DisplayDirTagged( dir_entry );
		      
		      DisplayFiles( dir_entry, 
				    dir_entry->start_file, 
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x
			          );
		      break;

      case 'V' :
      case 'v' :      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;
		      (void) GetRealFileNamePath( fe_ptr, filepath );
		      (void) View( dir_entry, filepath );
		      need_dsp_help = TRUE;
		      break;

      case 'H' :
      case 'h' :      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;
		      (void) GetRealFileNamePath( fe_ptr, filepath );
		      (void) ViewHex( filepath );
		      need_dsp_help = TRUE;
		      break;
      
      case 'E':
      case 'e' :      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;
		      (void) GetFileNamePath( fe_ptr, filepath );
		      (void) Edit( de_ptr, filepath );
		      break;

      case 'Y' :
      case 'y' :
      case 'C' :
      case 'c' :      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;

		      path_copy = FALSE;
		      if( ch == 'y' || ch == 'Y' ) path_copy = TRUE;

		      need_dsp_help = TRUE;

		      if( GetCopyParameter( fe_ptr->name, path_copy, to_file, to_dir ) )
                      {
			beep();
			break;
		      }

              if (mode != DISK_MODE && mode != USER_MODE) {
                  if (realpath(to_dir, to_path) == NULL) {
                      if (errno == ENOENT) {
                          strcpy(to_path, to_dir);
                      } else {
                          (void)sprintf(message, "Invalid destination path*\"%s\"*%s", to_dir, strerror(errno));
                          MESSAGE(message);
                          break;
                      }
                  }
                  dest_dir_entry = NULL;
              }
              else
              {
                  if( GetDirEntry( statistic.tree, de_ptr, to_dir, &dest_dir_entry, to_path ) )
                  {
                      break;
                  }
              }

              if( !CopyFile( &statistic, fe_ptr, TRUE, to_file, dest_dir_entry, to_path, path_copy ) )
              {
                  DisplayAvailBytes();		       
                  if( dest_dir_entry )
                  {
                      if( dir_entry->global_flag ) 
                        DisplayDiskStatistic();
                      else              
                        DisplayDirStatistic( de_ptr );
                    
                      if( dest_dir_entry == de_ptr ) 
                      {
                        BuildFileEntryList( dir_entry );
                        DisplayFiles( dir_entry, dir_entry->start_file, dir_entry->start_file + dir_entry->cursor_pos, start_x );
                      }
                  }
              }
		      break;	

      case 'Y' & 0x1F :
      case 'K' & 0x1F :
      case 'C' & 0x1F :
		      de_ptr = dir_entry;

                      path_copy = FALSE;
                      if( ch == ('Y' & 0x1F) ) path_copy = TRUE;

		      if( !IsMatchingTaggedFiles() )
		      {
			beep();
		      }
		      else
		      {
		        need_dsp_help = TRUE;
		        
			if( GetCopyParameter( NULL, path_copy, to_file, to_dir ) )
                        {
			  beep();
			  break;
		        }


                if (mode != DISK_MODE && mode != USER_MODE) {
                    if (realpath(to_dir, to_path) == NULL) {
                         if (errno == ENOENT) {
                             strcpy(to_path, to_dir);
                         } else {
                             (void)sprintf(message, "Invalid destination path*\"%s\"*%s", to_dir, strerror(errno));
                             MESSAGE(message);
                             break;
                         }
                    }
                    dest_dir_entry = NULL;
                } else {
                    if( GetDirEntry( statistic.tree, de_ptr, to_dir, &dest_dir_entry, to_path ) ) {
                        break;
                    }
                }
	  	
			  term = InputChoise( "Confirm overwrite existing files (Y/N) ? ", "YN\033" );
                          if( term == ESC ) 
		          {
			    beep();
			    break;
			  }
  
			  walking_package.function_data.copy.statistic_ptr  = &statistic;
			  walking_package.function_data.copy.dest_dir_entry = dest_dir_entry;
			  walking_package.function_data.copy.to_file        = to_file;
			  walking_package.function_data.copy.to_path        = to_path;
			  walking_package.function_data.copy.path_copy      = path_copy;
			  walking_package.function_data.copy.confirm = (term == 'Y') ? TRUE : FALSE;
                            
			  WalkTaggedFiles( dir_entry->start_file, 
					   dir_entry->cursor_pos, 
					   CopyTaggedFiles,
					   &walking_package
				         );
			    
                          DisplayAvailBytes();		       
			  
			  
			  DisplayFiles( dir_entry, 
					dir_entry->start_file, 
					dir_entry->start_file + dir_entry->cursor_pos,
					start_x
				      );
		      }
		      break;

      case 'M' :
      case 'm' :      if( mode != DISK_MODE && mode != USER_MODE )
                      {
			beep();
			break;
		      }

		      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;
		      
		      need_dsp_help = TRUE;

		      if( GetMoveParameter( fe_ptr->name, to_file, to_dir ) )
                      {
			beep();
			break;
		      }

                      if( GetDirEntry( statistic.tree,
				       de_ptr, 
				       to_dir, 
				       &dest_dir_entry, 
				       to_path 
				     ) )
		      {
			beep();
			break;
		      }
		      
		      if( !MoveFile( fe_ptr, 
				     TRUE, 
				     to_file, 
				     dest_dir_entry,
				     to_path,
				     &new_fe_ptr
				   ) )
		      {
			/* File wurde bewegt */
			/*-------------------*/
                       
                        DisplayAvailBytes();		       
			
			if( dir_entry->global_flag ) 
			  DisplayDiskStatistic();
			else              
			  DisplayDirStatistic( de_ptr );
			
			BuildFileEntryList( dir_entry );
		        
			if( file_count == 0 ) unput_char = ESC;

			if( dir_entry->start_file + dir_entry->cursor_pos >= (int)file_count )
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
		      break;	

      case 'N' & 0x1F :
		      if(( mode != DISK_MODE && mode != USER_MODE) || !IsMatchingTaggedFiles() )
		      {
			beep();
		      }
		      else
		      {
		        need_dsp_help = TRUE;
		        
			if( GetMoveParameter( NULL, to_file, to_dir ) )
                        {
			  beep();
			  break;
		        }


                        if( GetDirEntry( statistic.tree,
					 de_ptr, 
				         to_dir, 
				         &dest_dir_entry, 
				         to_path 
				       ) )
		        {
			  beep();
			  break;
		        }
		
			term = InputChoise( "Confirm overwrite existing files (Y/N) ? ", "YN\033" );
                        if( term == ESC ) 
		        {
			  beep();
			  break;
			}

			walking_package.function_data.mv.dest_dir_entry = dest_dir_entry;
			walking_package.function_data.mv.to_file = to_file;
			walking_package.function_data.mv.to_path = to_path;
			walking_package.function_data.mv.confirm = (term == 'Y') ? TRUE : FALSE;
                          
			WalkTaggedFiles( dir_entry->start_file, 
					 dir_entry->cursor_pos, 
					 MoveTaggedFiles,
					 &walking_package
				       );
			  
			BuildFileEntryList( dir_entry );
		        
			if( file_count == 0 ) unput_char = ESC;
		
			dir_entry->start_file = 0;
			dir_entry->cursor_pos = 0;

			DisplayFiles( dir_entry, 
				      dir_entry->start_file, 
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
			            );
			maybe_change_x_step = TRUE;
		      }
		      break;

      case 'D' :
      case 'd' :      if( mode != DISK_MODE && mode != USER_MODE )
		      {
			beep();
			break;
		      }
	              
		      term = InputChoise( "Delete this file (Y/N) ? ", 
					  "YN\033" 
					);
		        
		      need_dsp_help = TRUE;
		       
		      if( term != 'Y' ) break;
  
		      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;
			    
		      if( !DeleteFile( fe_ptr ) )
		      {
		        /* File wurde geloescht */
			/*----------------------*/
  
			if( dir_entry->global_flag ) 
			  DisplayDiskStatistic();
			else              
			  DisplayDirStatistic( de_ptr );
		  	
			DisplayAvailBytes();
  
                        RemoveFileEntry( dir_entry->start_file + dir_entry->cursor_pos );
		          
			if( file_count == 0 ) unput_char = ESC;
		          
			if( dir_entry->start_file + dir_entry->cursor_pos >= (int)file_count )
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
                      break;	

      case 'D' & 0x1F :
		      if(( mode != DISK_MODE && mode != USER_MODE) || !IsMatchingTaggedFiles() )
		      {
			beep();
		      }
		      else
		      {
		        need_dsp_help = TRUE;
			(void) DeleteTaggedFiles( max_disp_files );
			if( file_count == 0 ) unput_char = ESC;
			dir_entry->start_file = 0;
			dir_entry->cursor_pos = 0;
                        DisplayAvailBytes();		       
			DisplayFiles( dir_entry, 
				      dir_entry->start_file, 
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
			            );
			maybe_change_x_step = TRUE;
		      }
		      break;	

      case 'R':
      case 'r':       if( mode != DISK_MODE && mode != USER_MODE )
		      {
			beep();
			break;
		      }

		      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;

		      if( !GetRenameParameter( fe_ptr->name, new_name ) )
		      {
			if( !RenameFile( fe_ptr, new_name, &new_fe_ptr ) )
		        {
			  /* Rename OK */
			  /*-----------*/

			  /* Maybe structure has changed... */
			  /*--------------------------------*/

			  BuildFileEntryList( de_ptr );
		          
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
			  
      case 'R' & 0x1F :
		      if(( mode != DISK_MODE && mode != USER_MODE) || !IsMatchingTaggedFiles() )
		      {
			beep();
		      }
		      else
		      {
		        need_dsp_help = TRUE;
		        
			if( GetRenameParameter( NULL, new_name ) )
                        {
			  beep();
			  break;
		        }

			walking_package.function_data.rename.new_name = new_name;
			walking_package.function_data.rename.confirm  = FALSE;
                          
			WalkTaggedFiles( dir_entry->start_file, 
					 dir_entry->cursor_pos, 
					 RenameTaggedFiles,
					 &walking_package
				       );
			  
			BuildFileEntryList( dir_entry );
			
			if( file_count == 0 ) unput_char = ESC;
		        
			DisplayFiles( dir_entry, 
				      dir_entry->start_file, 
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
			            );
			
			maybe_change_x_step = TRUE;
		      }
		      break;

      case 'S':
      case 's':       GetKindOfSort();
		     
		      dir_entry->start_file = 0;
		      dir_entry->cursor_pos = 0;

		      SortFileEntryList();
		      
		      DisplayFiles( dir_entry, 
				    dir_entry->start_file, 
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x
			          );
		      need_dsp_help = TRUE;
		      break;
      
      case 'F':
      case 'f':       if(ReadFileSpec() == 0) {
		     
		        dir_entry->start_file = 0;
		        dir_entry->cursor_pos = 0;

		        BuildFileEntryList( dir_entry );
		     
		        DisplayFileSpec();
		        DisplayFiles( dir_entry, 
				      dir_entry->start_file, 
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
			            );
		       
		        if( dir_entry->global_flag ) 
		          DisplayDiskStatistic();
		        else              
		          DisplayDirStatistic( dir_entry );

                        if( file_count == 0 ) unput_char = ESC;
		        maybe_change_x_step = TRUE;
	              }
		      need_dsp_help = TRUE;
		      break;
    
#ifndef VI_KEYS
      case 'l':
#endif /* VI_KEYS */
      case 'L':      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		     if( mode 
         
         == DISK_MODE || mode == USER_MODE )
		     {
		       (void) GetFileNamePath( fe_ptr, new_login_path );
                       if( !GetNewLoginPath( new_login_path ) )
		       {
			 dir_entry->login_flag  = TRUE;

		         (void) LoginDisk( new_login_path );
		         unput_char = LOGIN_ESC;
			}
		        need_dsp_help = TRUE;
		     }
		     else beep();
		     break;

      case LF:
      case CR:        if( dir_entry->big_window ) break;
		      dir_entry->big_window = TRUE;
		      ch = '\0';
		      SwitchToBigFileWindow();
                      GetMaxYX( file_window, &window_height, &window_width );
                      
		      x_step =  (max_column > 1) ? window_height : 1;
                      max_disp_files = window_height * max_column;
                       
		      DisplayFiles( dir_entry, 
				    dir_entry->start_file, 
				    dir_entry->start_file + dir_entry->cursor_pos,
				    start_x
			          );
		      break;

      case 'P' :
      case 'p' :      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;
		      (void) Pipe( de_ptr, fe_ptr );
		      need_dsp_help = TRUE;
		      break;
	
      case 'P' & 0x1F :
		      de_ptr = dir_entry;

		      if( !IsMatchingTaggedFiles() )
		      {
			beep();
		      }
		      else if( mode != DISK_MODE && mode != USER_MODE )
		      {
			MESSAGE( "i am sorry*^P not supported in Archive-mode" );
		      }
		      else
		      {
		        need_dsp_help = TRUE;
		        
			if( GetPipeCommand( filepath ) )
                        {
			  beep();
			  break;
		        }


			
			if( ( walking_package.function_data.pipe_cmd.pipe_file = 
			      popen( filepath, "w" ) ) == NULL )
			{
			  (void) sprintf( message, "execution of command*%s*failed", filepath );
			  MESSAGE( message );
			  break;
			}

                            
			WalkTaggedFiles( dir_entry->start_file, 
					 dir_entry->cursor_pos, 
					 PipeTaggedFiles,
					 &walking_package
				       );
			
		        clearok( stdscr, TRUE );
 
			if( pclose( walking_package.function_data.pipe_cmd.pipe_file ) )
			{
			  WARNING( "pclose failed" );
			}

                        (void) GetAvailBytes( &statistic.disk_space );
                        DisplayAvailBytes();		       
			  
			DisplayFiles( dir_entry, 
				      dir_entry->start_file, 
				      dir_entry->start_file + dir_entry->cursor_pos,
				      start_x
			            );
		      }
		      break;

      case 'X':
      case 'x' :      fe_ptr = file_entry_list[dir_entry->start_file + dir_entry->cursor_pos].file;
		      de_ptr = fe_ptr->dir_entry;
		      (void) Execute( de_ptr, fe_ptr );
		      need_dsp_help = TRUE;
		      break;
    
      case 'S' & 0x1F :
                      if( !IsMatchingTaggedFiles() )
                      {
                        beep();
                      }
		      else if( mode != DISK_MODE && mode != USER_MODE )
		      {
			MESSAGE( "Feature not available in archives." );
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
		        if( !GetSearchCommandLine( command_line ) ) 
			{
			  refresh();
			  endwin();
			  SuspendClock();
			 
			  walking_package.function_data.execute.command = command_line;
                          SilentTagWalkTaggedFiles( ExecuteCommand,
					            &walking_package
					          );
			  RefreshWindow( file_window ); 

			  HitReturnToContinue();

			  InitClock();

			  DisplayFiles( dir_entry, 
					dir_entry->start_file, 
					dir_entry->start_file + dir_entry->cursor_pos,
					start_x
				      );
			}  
			free( command_line );
		      }
		      break;
                      
      case 'X' & 0x1F:
		      if( !IsMatchingTaggedFiles() )
		      {
			beep();
		      }
		      else if( mode != DISK_MODE && mode != USER_MODE )
		      {
			MESSAGE( "I am sorry*^X not supported in Archive-mode" );
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

			  DisplayFiles( dir_entry, 
					dir_entry->start_file, 
					dir_entry->start_file + dir_entry->cursor_pos,
					start_x
				      );
			}  
			free( command_line );
		      }
		      break;
                      
      case 'Q' & 0x1F:
                      need_dsp_help = TRUE;
                      fe_ptr = file_entry_list[dir_entry->start_file
                                + dir_entry->cursor_pos].file;
                      de_ptr = fe_ptr->dir_entry;
                      QuitTo(de_ptr);
                      break;

      case 'Q':
      case 'q':       need_dsp_help = TRUE;
                      Quit();
		      break;
    
      case 'L' & 0x1F:
		      clearok( stdscr, TRUE );
		      break;

      case '\033':    break;

      case LOGIN_ESC :
		      break;

      case KEY_F(12):
      		      ListJump(dir_entry, "");
		      need_dsp_help = TRUE;
		      break;

    case KEY_F(28):                  /* Ctrl-F4 */
    case KEY_F(4+12):                /* Shift-F4 */
        
	/* Toggle mode (if possible) */
        if(dir_entry->tagged_files)
          dir_entry->tagged_flag = !dir_entry->tagged_flag;
        else
          dir_entry->tagged_flag = FALSE;

        BuildFileEntryList ( dir_entry );

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

     default:
        beep();
		      break;
    }
  } while( ch != CR && ch != ESC && ch != LOGIN_ESC );
 
  if( dir_entry->big_window ) 
    SwitchToSmallFileWindow();

  if(ch != LOGIN_ESC) {
    dir_entry->global_flag = FALSE;
    dir_entry->tagged_flag = FALSE;
    dir_entry->big_window  = FALSE;
  }

  return( ch );
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

  if( baudrate() >= QUICK_BAUD_RATE ) typeahead( 0 );

/*  GetMaxYX( file_window, &window_height, &window_width );*/
 
  max_disp_files = window_height * max_column;

  for( i=0; i < (int)file_count && result == 0; i++ )
  {
    fe_ptr = file_entry_list[i].file;

    if( fe_ptr->tagged && fe_ptr->matching )
    {
      if( maybe_change_x == FALSE &&
	  i >= start_file && i < start_file + max_disp_files )
      {
	/* Walk ohne scroll moeglich */
	/*---------------------------*/

  	PrintFileEntry( start_file + cursor_pos,
			cursor_pos % window_height,
			cursor_pos / window_height,
			FALSE,
		        start_x
	 	      );
			
        cursor_pos = i - start_file;
			
	PrintFileEntry( start_file + cursor_pos,
		 	cursor_pos % window_height,
			cursor_pos / window_height,
			TRUE, 
		        start_x
		      );
      }
      else
      {
	/* Scroll noetig */
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
      if( walking_package->new_fe_ptr != fe_ptr )
      {
        file_entry_list[i].file = walking_package->new_fe_ptr;
	ChangeFileEntry();
        max_disp_files = window_height * max_column;
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

  
  for( i=0; i < (int)file_count; i++ )
  {
    fe_ptr = file_entry_list[i].file;

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

  
  for( i=0; i < (int)file_count; i++ )
  {
    fe_ptr = file_entry_list[i].file;

    if( fe_ptr->tagged && fe_ptr->matching )
    {
      result = fkt( fe_ptr, walking_package );

      if( result == 0 ) {
      	fe_ptr->tagged = FALSE;
      }
    }
  }
}




static BOOL IsMatchingTaggedFiles(void)
{
  FileEntry *fe_ptr;
  int i;

  for( i=0; i < (int)file_count; i++)
  {
    fe_ptr = file_entry_list[i].file;

    if( fe_ptr->matching && fe_ptr->tagged )
      return( TRUE );
  }

  return( FALSE );
}






static int DeleteTaggedFiles(int max_disp_files)
{
  FileEntry *fe_ptr;
  DirEntry  *de_ptr;
  int       i;
  int       start_file;
  int       cursor_pos;
  BOOL      deleted;
  BOOL      confirm;
  int       term;
  int       start_x = 0;
  int       result = 0;

  term = InputChoise( "Confirm delete each file (Y/N) ? ", "YN\033" );

  if( term == ESC ) return( -1 );

  if( term == 'Y' ) confirm = TRUE;
  else confirm = FALSE;

  if( baudrate() >= QUICK_BAUD_RATE ) typeahead( 0 );

  for( i=0; i < (int)file_count && result == 0; )
  {
    deleted = FALSE;

    fe_ptr = file_entry_list[i].file;
    de_ptr = fe_ptr->dir_entry;

    if( fe_ptr->tagged && fe_ptr->matching )
    {
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
   
      if( confirm ) term = InputChoise( "Delete this file (Y/N) ? ", "YN\033" );
      else term = 'Y';

      if( term == ESC ) 
      {
        if( baudrate() >= QUICK_BAUD_RATE ) typeahead( -1 );
	result = -1;
	break;
      }

      if( term == 'Y' )
      {
        if( ( result = DeleteFile( fe_ptr ) ) == 0 )
        {
	  /* File wurde geloescht */
	  /*----------------------*/

	  deleted = TRUE;

  	  if( de_ptr->global_flag ) 
	    DisplayDiskStatistic();
	  else              
	    DisplayDirStatistic( de_ptr );
			
	  DisplayAvailBytes();

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
  SetFileMode(file_mode);
  x_step =  (max_column > 1) ? window_height : 1;
  max_disp_files = window_height * max_column;


  if( dir_entry->start_file + dir_entry->cursor_pos < (int)file_count )
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
    MvAddStr( LINES - 2, 1, jumpmsg );
    PrintOptions
    (
        stdscr,
        LINES - 2,
        COLS - 14,
        "(Escape) cancel"
    );

    ic = tolower(getch());

    if( !isprint(ic) )
    {
        beep();
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

    if( tmp2 == file_count - 1 )
    {
        ClearHelp();
        MvAddStr( LINES - 2, 1, "Last entry!");
        beep();
        RefreshWindow( stdscr );
        RefreshWindow( file_window );
        doupdate();
        sleep(1);
        free(newStr);
        return;
    }

    for( i=tmp2; i < file_count; i++ )
    {
        fe_ptr = file_entry_list[i].file;
	if(!strncasecmp(newStr, fe_ptr->name, n+1))
          break;
    }

    if ( i == file_count )
    {
        ClearHelp();
        MvAddStr( LINES - 2, 1, "No match!");
        beep();
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