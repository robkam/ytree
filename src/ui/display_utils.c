/***************************************************************************
 *
 * display_utils.c
 * Functions for formatting and displaying data in the terminal UI
 *
 ***************************************************************************/


#include <stdio.h> /* Added for snprintf, etc. */
#include <string.h>
#include <time.h>
#include <stdlib.h> /* Added for free, exit */
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

  *buffer = '\0'; /* This ensures buffer[10] is null-terminated, safe for char[11] */

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
      /* Format: 2025-11-19 14:30 (16 characters + null terminator) */
      /* The caller (BuildUserFileEntry) provides a buffer of size 20. */
      strftime(buffer, 17, "%Y-%m-%d %H:%M", tm_ptr);
  } else {
      /* Fallback for invalid time, 15 characters + null terminator */
      snprintf(buffer, 17, "    ?     ?   ");
  }

  return( buffer );
}

/*****************************************************************************
 *                              FormFilename                                 *
 * Safely formats a filename, truncating with "..." if it exceeds max_len.   *
 * Prioritizes showing the end of the path (like CutPathname) for clarity.  *
 *****************************************************************************/
char *FormFilename(char *dest, char *src, unsigned int max_len)
{
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

  l = strlen(working_src);

  if( l <= max_len ) {
    /* Safe copy if pointers differ or if they are the same (snprintf handles it) */
    snprintf(dest, max_len + 1, "%s", working_src);
  }
  else
  {
    /* Truncate path: "...<suffix>" */
    /* We need the last (max_len - 3) chars from src, plus "..." prefix */
    if (max_len < 4) {
        /* If max_len is too small for "...", just truncate */
        snprintf(dest, max_len + 1, "%.*s", max_len, "...");
    } else {
        const char *suffix_start = working_src + (l - (max_len - 3));
        snprintf(dest, max_len + 1, "...%s", suffix_start);
    }
  }

  if (src_copy) free(src_copy);
  return dest;
}

/*****************************************************************************
 *                              CutFilename                                  *
 * Truncates a filename by keeping the prefix and appending "..." if too long.*
 *****************************************************************************/
char *CutFilename(char *dest, const char *src, unsigned int max_len)
{
  unsigned int l;

  l = StrVisualLength(src); /* Using visual length as per existing logic */

  if( l <= max_len ) {
    snprintf(dest, max_len + 1, "%s", src);
  }
  else
  {
    /* Truncate string: keep first (max_len - 3) chars, append "..." */
    if (max_len < 4) {
        snprintf(dest, max_len + 1, "%.*s", max_len, "...");
    } else {
        snprintf(dest, max_len + 1, "%.*s...", max_len - 3, src);
    }
  }
  return dest;
}

/*****************************************************************************
 *                              CutPathname                                  *
 * Truncates a pathname by keeping the suffix and prepending "..." if too long.*
 *****************************************************************************/
char *CutPathname(char *dest, const char *src, unsigned int max_len)
{
  unsigned int l;

  l = strlen(src);

  if( l <= max_len ) {
    snprintf(dest, max_len + 1, "%s", src);
  }
  else
  {
    /* Format: "...<suffix>" */
    /* We need the last (max_len - 3) chars from src */
    if (max_len < 4) {
        snprintf(dest, max_len + 1, "%.*s", max_len, "...");
    } else {
        const char *suffix_start = src + (l - (max_len - 3));
        snprintf(dest, max_len + 1, "...%s", suffix_start);
    }
  }
  return( dest );
}

/*****************************************************************************
 *                              CutName                                      *
 * Truncates a name by keeping the prefix and appending "..." if too long.   *
 * (Identical to CutFilename in behavior, but uses strlen for length)        *
 *****************************************************************************/
char *CutName(char *dest, const char *src, unsigned int max_len)
{
  unsigned int l;

  l = strlen(src);

  if( l <= max_len ) {
    snprintf(dest, max_len + 1, "%s", src);
  }
  else
  {
    /* Truncate string: keep first (max_len - 3) chars, append "..." */
    if (max_len < 4) {
        snprintf(dest, max_len + 1, "%.*s", max_len, "...");
    } else {
        snprintf(dest, max_len + 1, "%.*s...", max_len - 3, src);
    }
  }
  return dest;
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
  char format1[60]; /* Increased size for safety with snprintf */
  char format2[60]; /* Increased size for safety with snprintf */
  int  written;
  char owner[OWNER_NAME_MAX + 1];
  char group[GROUP_NAME_MAX + 1];
  char *owner_name_ptr;
  char *group_name_ptr;
  char *sym_link_name = NULL;
  char *sptr;
  char tag;

  /* Setup for safe string handling */
  size_t remaining = linelen;
  char *dptr = line;

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
    snprintf( owner, sizeof(owner), "%d", (int) fe_ptr->stat_struct.st_uid );
    owner_name_ptr = owner;
  }
  if( group_name_ptr == NULL )
  {
    snprintf( group, sizeof(group), "%d", (int) fe_ptr->stat_struct.st_gid );
    group_name_ptr = group;
  }

  /* Safely create format strings */
  snprintf(format1, sizeof(format1), "%%-%ds", filename_width);
  snprintf(format2, sizeof(format2), "%%-%ds", linkname_width);

  for(sptr=template; *sptr && remaining > 0; ) { /* Added remaining check */

    if(*sptr == '%') {
      sptr++;
      if(!strncmp(sptr, TAGSYMBOL_VIEWNAME, 3)) {
        written = snprintf(dptr, remaining, "%c", tag);
      } else if(!strncmp(sptr, FILENAME_VIEWNAME, 3)) {
        written = snprintf(dptr, remaining, format1, fe_ptr->name);
      } else if(!strncmp(sptr, ATTRIBUTE_VIEWNAME, 3)) {
        written = snprintf(dptr, remaining, "%10s", attributes);
      } else if(!strncmp(sptr, LINKCOUNT_VIEWNAME, 3)) {
        written = snprintf(dptr, remaining, "%3d", (int)fe_ptr->stat_struct.st_nlink);
      } else if(!strncmp(sptr, FILESIZE_VIEWNAME, 3)) {
#ifdef HAS_LONGLONG
        written = snprintf(dptr, remaining, "%11lld", (LONGLONG) fe_ptr->stat_struct.st_size);
#else
        written = snprintf(dptr, remaining, "%7d", fe_ptr->stat_struct.st_size);
#endif
      } else if(!strncmp(sptr, MODTIME_VIEWNAME, 3)) {
        written = snprintf(dptr, remaining, "%16s", modify_time);
      } else if(!strncmp(sptr, SYMLINK_VIEWNAME, 3)) {
        written = snprintf(dptr, remaining, format2, sym_link_name);
      } else if(!strncmp(sptr, UID_VIEWNAME, 3)) {
        written = snprintf(dptr, remaining, "%-8s", owner_name_ptr);
      } else if(!strncmp(sptr, GID_VIEWNAME, 3)) {
        written = snprintf(dptr, remaining, "%-8s", group_name_ptr);
      } else if(!strncmp(sptr, INODE_VIEWNAME, 3)) {
#ifdef HAS_LONGLONG
        written = snprintf(dptr, remaining, "%11lld", (LONGLONG)fe_ptr->stat_struct.st_ino);
#else
        written = snprintf(dptr, remaining, "%7ld", (long)fe_ptr->stat_struct.st_ino); /* Changed to long for %ld */
#endif
      } else if(!strncmp(sptr, ACCTIME_VIEWNAME, 3)) {
        written = snprintf(dptr, remaining, "%16s", access_time);
      } else if(!strncmp(sptr, SCTIME_VIEWNAME, 3)) {
        written = snprintf(dptr, remaining, "%16s", change_time);
      } else {
	    written = -1; /* Indicate no match, will print '%' */
      }

      if(written < 0) { /* Error or no match */
        written = snprintf(dptr, remaining, "%%");
        if (written > 0) {
            if ((size_t)written >= remaining) { dptr += remaining - 1; remaining = 1; }
            else { dptr += written; remaining -= written; }
        }
	    sptr++; /* Advance past the single '%' */
      } else { /* Successfully wrote something */
        if ((size_t)written >= remaining) {
            dptr += remaining - 1; remaining = 1; /* Keep space for NULL */
        } else {
            dptr += written; remaining -= written;
        }
        /* Advance sptr past the 3-char viewname */
        if(*sptr) sptr++;
        if(*sptr) sptr++;
        if(*sptr) sptr++;
      }
    } else { /* Not a '%' placeholder, copy character directly */
      if (remaining > 1) { /* Ensure space for char + null */
        *dptr++ = *sptr++;
        remaining--;
      } else {
        sptr++; /* Consume char but don't write if no space */
      }
    }
  }
  *dptr = '\0'; /* Ensure null termination */
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
  if (win == NULL) {
      ERROR_MSG("GetMaxYX called with NULL*ABORT");
      exit(1);
  }

  getmaxyx(win, *height, *width);

  *height = MAXIMUM(*height, 1);
  *width  = MAXIMUM(*width, 1);
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