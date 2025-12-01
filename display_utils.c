/***************************************************************************
 *
 * display_utils.c
 * Functions for formatting and displaying data in the terminal UI
 *
 ***************************************************************************/


#include "ytree.h"


/*****************************************************************************
 *                              GetAttributes                                *
 *****************************************************************************/
char *GetAttributes(unsigned short modus, char *buffer)
{
  char *save_buffer = buffer;

       if( S_ISREG( modus ) )  *buffer++ = '-';
  else if( S_ISDIR( modus ) )  *buffer++ = 'd';
  else if( S_ISCHR( modus ) )  *buffer++ = 'c';
  else if( S_ISBLK( modus ) )  *buffer++ = 'b';
  else if( S_ISFIFO( modus ) ) *buffer++ = 'p';
  else if( S_ISLNK( modus ) )  *buffer++ = 'l';
  else if( S_ISSOCK( modus ) ) *buffer++ = 's';  /* ??? */
  else                         *buffer++ = '?';  /* unknown */

  if( modus & S_IRUSR ) *buffer++ = 'r';
  else *buffer++ = '-';

  if( modus & S_IWUSR ) *buffer++ = 'w';
  else *buffer++ = '-';

  if( modus & S_IXUSR ) *buffer++ = 'x';
  else *buffer++ = '-';

  if( modus & S_ISUID ) *(buffer - 1) = 's';


  if( modus & S_IRGRP ) *buffer++ = 'r';
  else *buffer++ = '-';

  if( modus & S_IWGRP ) *buffer++ = 'w';
  else *buffer++ = '-';

  if( modus & S_IXGRP ) *buffer++ = 'x';
  else *buffer++ = '-';

  if( modus & S_ISGID ) *(buffer - 1) = 's';


  if( modus & S_IROTH ) *buffer++ = 'r';
  else *buffer++ = '-';

  if( modus & S_IWOTH ) *buffer++ = 'w';
  else *buffer++ = '-';

  if( modus & S_IXOTH ) *buffer++ = 'x';
  else *buffer++ = '-';

  *buffer = '\0';

  return( save_buffer );
}

/*****************************************************************************
 *                                  CTime                                    *
 * Modernized to use ISO-like format: YYYY-MM-DD HH:MM (16 chars)            *
 *****************************************************************************/
char *CTime(time_t f_time, char *buffer)
{
  struct tm *tm_ptr;

  tm_ptr = localtime(&f_time);

  if (tm_ptr) {
      /* Format: 2025-11-19 14:30 */
      strftime(buffer, 17, "%Y-%m-%d %H:%M", tm_ptr);
  } else {
      strcpy(buffer, "    ?     ?   ");
  }

  return( buffer );
}

/*****************************************************************************
 *                              FormFilename                                 *
 *****************************************************************************/
char *FormFilename(char *dest, char *src, unsigned int max_len)
{
  int i;
  int begin;
  unsigned int l;
  char *src_copy = NULL;
  char *working_src = src;

  /* If dest and src overlap, we must copy src to avoid corruption during write */
  if (dest == src) {
      src_copy = strdup(src);
      if (!src_copy) {
          ERROR_MSG("strdup failed*ABORT");
          exit(1);
      }
      working_src = src_copy;
  }

  l = strlen(working_src); /* TODO: UTF-8 */
  begin = 0;

  if( l <= max_len ) {
    /* Safe copy if pointers differ */
    if (dest != working_src) {
        memmove(dest, working_src, l + 1);
    }
  }
  else
  {
    (void) strcpy( dest, "/..." );

    if(max_len < 4) {
      dest[max_len] = '\0';
    } else {
      for(i=0; i < (int) max_len - 4; i++)
        if( working_src[l - i] == FILE_SEPARATOR_CHAR || working_src[l - i] == '\\' )
          begin = l - i;

      if(begin > 0) {
        strcat(dest, &working_src[begin] );
        if(strlen(dest) > max_len)
          strcpy( &dest[max_len - 3], "..." );
      } else {
        for(i=0; i < l; i++)
	  if( working_src[i] == FILE_SEPARATOR_CHAR || working_src[i] == '\\' )
	    begin = i;  /* find last '/' */
        strcat(dest, &working_src[begin] );
        if(strlen(dest) > max_len)
          strcpy( &dest[max_len - 3], "..." );
      }
    }
  }

  if (src_copy) free(src_copy);
  return dest;
}

/*****************************************************************************
 *                              CutFilename                                  *
 *****************************************************************************/
char *CutFilename(char *dest, char *src, unsigned int max_len)
{
  unsigned int l;
  char *tmp;

  l = StrVisualLength(src);

  if( l <= max_len ) {
    if (dest != src) {
        /* memmove handles overlap safely if src/dest are offset but in same buffer */
        memmove(dest, src, strlen(src) + 1);
    }
    return dest;
  }
  else
  {
    /* StrLeft allocates new memory, so safe even if dest == src */
    if ((tmp = StrLeft(src, max_len - 3)) == NULL) {
      ERROR_MSG("Malloc failed*ABORT");
      exit(1);
    }
    /* dest is overwritten here, which is fine as we have the data in tmp */
    sprintf(dest, "%s...", tmp);
    free(tmp);
    return( dest );
  }
}

/*****************************************************************************
 *                              CutPathname                                  *
 *****************************************************************************/
char *CutPathname(char *dest, char *src, unsigned int max_len)
{
  unsigned int l;

  l = strlen(src);

  if( l <= max_len ) {
    if (dest != src) {
        memmove(dest, src, l + 1);
    }
    return dest;
  }
  else
  {
    /* Format: "...<suffix>" */
    /* We need the last (max_len - 3) chars from src */
    const char *suffix_start = src + (l - (max_len - 3));
    size_t suffix_len = max_len - 3;

    /* Move data to the end of the destination buffer first to prevent overwrite */
    memmove(dest + 3, suffix_start, suffix_len);

    /* Add prefix */
    memcpy(dest, "...", 3);

    /* Null terminate */
    dest[max_len] = '\0';

    return( dest );
  }
}

/*****************************************************************************
 *                              CutName                                      *
 *****************************************************************************/
char *CutName(char *dest, char *src, unsigned int max_len)
{
  unsigned int l;

  l = strlen(src);

  if( l <= max_len ) {
    if (dest != src) {
        memmove(dest, src, l + 1);
    }
    return dest;
  }
  else
  {
    /* Truncate string: keep first (max_len - 3) chars, append "..." */
    if (dest != src) {
        memmove(dest, src, max_len - 3);
    }
    /* If dest == src, the first chars are already there, we just append the dot */
    strcpy( &dest[max_len - 3], "..." );
    return dest; // Actually, strcpy returns destination pointer, but we want `dest`
  }
}


/*****************************************************************************
 *                           BuildUserFileEntry                              *
 *****************************************************************************/
int BuildUserFileEntry(FileEntry *fe_ptr,
			int filename_width, int linkname_width,
			char *template, int linelen, char *line)
{
  char attributes[11];
  char modify_time[20];
  char change_time[20];
  char access_time[20];
  char format1[60];
  char format2[60];
  int  n;
  char owner[OWNER_NAME_MAX + 1];
  char group[GROUP_NAME_MAX + 1];
  char *owner_name_ptr;
  char *group_name_ptr;
  char *sym_link_name = NULL;
  char *sptr, *dptr;
  char tag;
  char buffer[4096]; /* enough??? */


  if( fe_ptr && S_ISLNK( fe_ptr->stat_struct.st_mode ) )
    sym_link_name = &fe_ptr->name[strlen(fe_ptr->name)+1];
  else
    sym_link_name = "";


  tag = (fe_ptr->tagged) ? TAGGED_SYMBOL : ' ';
  (void) GetAttributes( fe_ptr->stat_struct.st_mode, attributes);

  (void) CTime( fe_ptr->stat_struct.st_mtime, modify_time );
  (void) CTime( fe_ptr->stat_struct.st_ctime, change_time );
  (void) CTime( fe_ptr->stat_struct.st_atime, access_time );

  owner_name_ptr = GetPasswdName(fe_ptr->stat_struct.st_uid);
  group_name_ptr = GetGroupName(fe_ptr->stat_struct.st_gid);

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


  sprintf(format1, "%%-%ds", filename_width);
  sprintf(format2, "%%-%ds", linkname_width);

  for(sptr=template, dptr=buffer; *sptr; ) {

    if(*sptr == '%') {
      sptr++;
      if(!strncmp(sptr, TAGSYMBOL_VIEWNAME, 3)) {
        *dptr = tag; n=1;
      } else if(!strncmp(sptr, FILENAME_VIEWNAME, 3)) {
        n = sprintf(dptr, format1, fe_ptr->name);
      } else if(!strncmp(sptr, ATTRIBUTE_VIEWNAME, 3)) {
        n = sprintf(dptr, "%10s", attributes);
      } else if(!strncmp(sptr, LINKCOUNT_VIEWNAME, 3)) {
        n = sprintf(dptr, "%3d", (int)fe_ptr->stat_struct.st_nlink);
      } else if(!strncmp(sptr, FILESIZE_VIEWNAME, 3)) {
#ifdef HAS_LONGLONG
        n = sprintf(dptr, "%11lld", (LONGLONG) fe_ptr->stat_struct.st_size);
#else
        n = sprintf(dptr, "%7d", fe_ptr->stat_struct.st_size);
#endif
      } else if(!strncmp(sptr, MODTIME_VIEWNAME, 3)) {
        n = sprintf(dptr, "%16s", modify_time);
      } else if(!strncmp(sptr, SYMLINK_VIEWNAME, 3)) {
        n = sprintf(dptr, format2, sym_link_name);
      } else if(!strncmp(sptr, UID_VIEWNAME, 3)) {
        n = sprintf(dptr, "%-8s", owner_name_ptr);
      } else if(!strncmp(sptr, GID_VIEWNAME, 3)) {
        n = sprintf(dptr, "%-8s", group_name_ptr);
      } else if(!strncmp(sptr, INODE_VIEWNAME, 3)) {
#ifdef HAS_LONGLONG
        n = sprintf(dptr, "%11lld", (LONGLONG)fe_ptr->stat_struct.st_ino);
#else
        n = sprintf(dptr, "%7ld", (int)fe_ptr->stat_struct.st_ino);
#endif
      } else if(!strncmp(sptr, ACCTIME_VIEWNAME, 3)) {
        n = sprintf(dptr, "%16s", access_time);
      } else if(!strncmp(sptr, SCTIME_VIEWNAME, 3)) {
        n = sprintf(dptr, "%16s", change_time);
      } else {
	n = -1;
      }
      if(n == -1) {
        *dptr++ = '%';
	} else {
        dptr += n;
        if(*sptr) sptr++;
        if(*sptr) sptr++;
        if(*sptr) sptr++;
      }
    } else {
      *dptr++ = *sptr++;
    }
  }
  *dptr = '\0';
  strncpy(line, buffer, linelen);
  line[linelen - 1] = '\0';
  return(0);
}



int GetVisualUserFileEntryLength( int max_visual_filename_len, int max_visual_linkname_len, char *template)
{
  int  len, n;
  char *sptr;


  for(len=0, sptr=template; *sptr; ) {

    if(*sptr == '%') {
      sptr++;
      if(!strncmp(sptr, TAGSYMBOL_VIEWNAME, 3)) {
        n=1;
      } else if(!strncmp(sptr, FILENAME_VIEWNAME, 3)) {
        n = max_visual_filename_len;
      } else if(!strncmp(sptr, ATTRIBUTE_VIEWNAME, 3)) {
        n = 10;
      } else if(!strncmp(sptr, LINKCOUNT_VIEWNAME, 3)) {
        n = 3;
      } else if(!strncmp(sptr, FILESIZE_VIEWNAME, 3)) {
#ifdef HAS_LONGLONG
        n = 11;
#else
        n = 7;
#endif
      } else if(!strncmp(sptr, MODTIME_VIEWNAME, 3)) {
        n = 16; /* Updated to 16 for YYYY-MM-DD HH:MM */
      } else if(!strncmp(sptr, SYMLINK_VIEWNAME, 3)) {
        n = max_visual_linkname_len;
      } else if(!strncmp(sptr, UID_VIEWNAME, 3)) {
        n = 8;
      } else if(!strncmp(sptr, GID_VIEWNAME, 3)) {
        n = 8;
      } else if(!strncmp(sptr, INODE_VIEWNAME, 3)) {
#ifdef HAS_LONGLONG
        n = 11;
#else
        n = 7;
#endif
      } else if(!strncmp(sptr, ACCTIME_VIEWNAME, 3)) {
        n = 16; /* Updated to 16 */
      } else if(!strncmp(sptr, SCTIME_VIEWNAME, 3)) {
        n = 16; /* Updated to 16 */
      } else {
	n = -1;
      }
      if(n == -1) {
        len++;
	sptr++;
	} else {
        len += n;
        if(*sptr) sptr++;
        if(*sptr) sptr++;
        if(*sptr) sptr++;
      }
    } else {
      sptr++;
      len++;
    }
  }
  return(len);
}

/*****************************************************************************
 *                                  GetMaxYX                                 *
 *****************************************************************************/
void GetMaxYX(WINDOW *win, int *height, int *width)
{
  if( win == dir_window )
  {
    *height = MAXIMUM(DIR_WINDOW_HEIGHT, 1);
    *width  = MAXIMUM(DIR_WINDOW_WIDTH, 1);
  }
  else if( win == small_file_window )
  {
    *height = MAXIMUM(FILE_WINDOW_1_HEIGHT, 1);
    *width  = MAXIMUM(FILE_WINDOW_1_WIDTH, 1);
  }
  else if( win == big_file_window )
  {
    *height = MAXIMUM(FILE_WINDOW_2_HEIGHT, 1);
    *width  = MAXIMUM(FILE_WINDOW_2_WIDTH, 1);
  }
  else if( win == f2_window )
  {
    *height = MAXIMUM(F2_WINDOW_HEIGHT - 1, 1); /* fake for separator line */
    *width  = MAXIMUM(F2_WINDOW_WIDTH, 1);
  }
  else if( win == history_window )
  {
    *height = MAXIMUM(HISTORY_WINDOW_HEIGHT, 1);
    *width  = MAXIMUM(HISTORY_WINDOW_WIDTH, 1);
  }
  else
  {
    ERROR_MSG( "Unknown Window-ID*ABORT" );
    exit( 1 );
  }
}

/***************************************************************************
 *
 * Enhanced Curses Print Functions (from former print.c)
 *
 ***************************************************************************/

int MvAddStr(int y, int x, char *str)
{
#ifdef WITH_UTF8
  mvaddstr(y, x, str);
#else
  for(;*str != '\0';str++)
      mvaddch(y, x++, PRINT(*str));
#endif
  return 0;
}

int MvWAddStr(WINDOW *win, int y, int x, char *str)
{
#ifdef WITH_UTF8
  mvwaddstr(win, y, x, str);
#else
  for(;*str != '\0';str++)
      mvwaddch(win, y, x++, PRINT(*str));
#endif
  return 0;
}


int WAddStr(WINDOW *win, char *str)
{
#ifdef WITH_UTF8
  waddstr(win, str);
#else
  for(;*str != '\0';str++)
      waddch(win, PRINT(*str));
#endif
  return 0;
}

int AddStr(char *str)
{
#ifdef WITH_UTF8
  addstr(str);
#else
  for(;*str != '\0';str++)
      addch( PRINT(*str));
#endif
  return 0;
}

int WAttrAddStr(WINDOW *win, int attr, char *str)
{
  int rc;

  wattrset( win, attr );
  rc = WAddStr(win, str);
  wattrset( win, 0 );

  return(rc);
}

void PrintSpecialString(WINDOW *win, int y, int x, char *str, int color)
{
  int ch;

  if(x < 0 || y < 0) {
     /* screen too small */
    return;
  }

  wmove( win, y, x);

  for( ; *str; str++ )
  {
    if ( (!iscntrl(*str)) || (!isspace(*str)) || (*str==' ') )
    switch( *str )
    {
      case '1': ch = ACS_ULCORNER; break;
      case '2': ch = ACS_URCORNER; break;
      case '3': ch = ACS_LLCORNER; break;
      case '4': ch = ACS_LRCORNER; break;
      case '5': ch = ACS_TTEE;     break;
      case '6': ch = ACS_LTEE;     break;
      case '7': ch = ACS_RTEE;     break;
      case '8': ch = ACS_BTEE;     break;
      case '9': ch = ACS_LARROW;   break;
      case '|': ch = ACS_VLINE;    break;
      case '-': ch = ACS_HLINE;    break;
      default:  ch = PRINT(*str);
    }
    else
            ch = ACS_BLOCK;

#ifdef COLOR_SUPPORT
    wattrset( win, COLOR_PAIR(color) | A_BOLD );
#endif /* COLOR_SUPPORT */
    waddch( win, ch );
#ifdef COLOR_SUPPORT
    wattrset( win, 0 );
#endif /* COLOR_SUPPORT */
  }
}

/*****************************************************************************
 *                                  PrintLine                                *
 * Generates a line of 'len' characters based on a 2-char pattern:           *
 * start_char, fill_char. The line will consist of start_char followed by    *
 * (len-1) fill_char characters.                                             *
 *****************************************************************************/
void PrintLine(WINDOW *win, int y, int x, char *line, int len)
{
  char *buffer_content;
  int i;

  if (len < 1) return;

  // Allocate for 'len' characters plus null terminator
  if ((buffer_content = (char *)malloc(len + 1)) == NULL) {
     ERROR_MSG( "Malloc failed*ABORT" );
     exit( 1 );
  }

  // Set the starting character
  buffer_content[0] = line[0];

  // Fill the remaining characters with the fill character (line[1])
  for (i = 1; i < len; i++) {
    buffer_content[i] = line[1];
  }
  buffer_content[len] = '\0'; // Null terminate

  PrintOptions(win, y, x, buffer_content); // Pass the correctly sized buffer
  free(buffer_content);
}


void Print(WINDOW *win, int y, int x, char *str, int color)
{
 int ch;

  if(x < 0 || y < 0) {
     /* screen too small */
    return;
  }
  wmove(win, y, x);
  for( ; *str; str++ )
  {
    ch = PRINT((int) *str);

#ifdef COLOR_SUPPORT
    wattrset( win, COLOR_PAIR(color) | A_BOLD);
#endif /* COLOR_SUPPORT */
    waddch(win, ch );
#ifdef COLOR_SUPPORT
    wattrset( win, 0);
#endif /* COLOR_SUPPORT */
  }
}


void PrintOptions(WINDOW *win, int y, int x, char *str)
{
  int ch;
  int color, hi_color, lo_color;

  if(x < 0 || y < 0) {
     /* screen too small */
    return;
  }

#ifdef COLOR_SUPPORT
     lo_color = CPAIR_MENU;
     hi_color = CPAIR_HIMENUS;
#else
     lo_color = A_NORMAL;
     hi_color = A_BOLD;
#endif

  color = lo_color;

  for( ; *str; str++ )
  {
    ch = (int) *str;

    switch( *str ) {
        case '(': color = hi_color;  continue;
	case ')': color = lo_color;  continue;

#ifdef COLOR_SUPPORT
	case ']': color = lo_color;  continue;
	case '[': color = hi_color;  continue;
#else
	case ']':
	case '[': /* ignore */ continue;
#endif

        case '1': ch = ACS_ULCORNER; break;
        case '2': ch = ACS_URCORNER; break;
        case '3': ch = ACS_LLCORNER; break;
        case '4': ch = ACS_LRCORNER; break;
        case '5': ch = ACS_TTEE;     break;
        case '6': ch = ACS_LTEE;     break;
        case '7': ch = ACS_RTEE;     break;
        case '8': ch = ACS_BTEE;     break;
        case '9': ch = ACS_LARROW;   break;
        case '|': ch = ACS_VLINE;    break;
        case '-': ch = ACS_HLINE;    break;
        default:  ch = PRINT(*str);
     }

#ifdef COLOR_SUPPORT
    wattrset( win, COLOR_PAIR(color) | A_BOLD);
#else
    wattrset( win, color);
#endif
     mvwaddch( win, y, x++, ch );
     wattrset( win, 0 );
   }
}


void PrintMenuOptions(WINDOW *win,int y, int x, char *str, int ncolor, int hcolor)
{
  int ch;
  int color, hi_color, lo_color;
  char *sbuf, buf[2];

  if ((sbuf = (char *)malloc(strlen(str)+1)) == NULL) {
    ERROR_MSG( "Malloc failed*ABORT" );
    exit( 1 );
  }
  sbuf[0] = '\0';
  buf[1] = '\0';

  if(x < 0 || y < 0) {
     /* screen too small */
    free(sbuf);
    return;
  }

#ifdef COLOR_SUPPORT
     lo_color = ncolor;
     hi_color = hcolor;
#else
     lo_color = A_NORMAL;
     hi_color = A_REVERSE;
#endif

  color = lo_color;
  wmove(win, y, x);

  for( ; *str; str++ )
  {
    ch = (int) *str;

    switch( ch ) {
        case '(': color = hi_color;
#ifdef COLOR_SUPPORT
                  WAttrAddStr( win, COLOR_PAIR(color) | A_BOLD, sbuf);
#else
                  WAttrAddStr( win, color, sbuf);
#endif
		  strcpy(sbuf, "");
	          continue;

	case ')': color = lo_color;
#ifdef COLOR_SUPPORT
                  WAttrAddStr( win, COLOR_PAIR(color) | A_BOLD, sbuf);
#else
                  WAttrAddStr( win, color, sbuf);
#endif
		  strcpy(sbuf, "");
	          continue;

#ifdef COLOR_SUPPORT
	case ']': color = lo_color;
                  WAttrAddStr( win, COLOR_PAIR(color) | A_BOLD, sbuf);
		  strcpy(sbuf, "");
	          continue;
	case '[': color = hi_color;
                  WAttrAddStr( win, COLOR_PAIR(color) | A_BOLD, sbuf);
		  strcpy(sbuf, "");
	          continue;
#else
	case ']':
	case '[': /* ignore */ continue;
#endif
        default : buf[0] = PRINT(*str);
		  strcat(sbuf, buf);
    }
  }

#ifdef COLOR_SUPPORT
  WAttrAddStr( win, COLOR_PAIR(color) | A_BOLD, sbuf);
#else
  WAttrAddStr( win, color, sbuf);
#endif
  free(sbuf);
}