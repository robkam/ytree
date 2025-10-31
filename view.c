/***************************************************************************
 *
 * View-Kommando-Bearbeitung
 *
 ***************************************************************************/


#include "ytree.h"
#include <errno.h>
#include "xmalloc.h"

typedef struct MODIF {
    long pos;
    unsigned char old_char;
    unsigned char new_char;
    struct MODIF *next;
} CHANGES;

static CHANGES *changes;
static int cursor_pos_x;
static int cursor_pos_y;

static long current_line;
static int fd, fd2;
static struct stat fdstat;
static int WLINES, WCOLS, BYTES;
static WINDOW *VIEW, *BORDER;
static BOOL resize_done = FALSE;

static BOOL inhex=TRUE;
static BOOL inedit=FALSE;
static BOOL hexoffset=TRUE;


#define CURSOR_CALC_X (10+(((cursor_pos_x/2)<(BYTES/2)) ? 2:3)+(cursor_pos_x)+(cursor_pos_x/2))
#define CURSOR_POS_X ((inhex)? CURSOR_CALC_X:(WCOLS-BYTES+cursor_pos_x))
#define C_POSX (((cursor_pos_x%2)!=1)? cursor_pos_x:(cursor_pos_x-1))
#define CURSOR_POSX ((inhex)? (C_POSX/2):cursor_pos_x)
#define CANTX(x) ((inhex)? (x*2):x)
#define THECOLOR ((inedit)? COLOR_PAIR(STATS_COLOR):COLOR_PAIR(DIR_COLOR))

static int ViewFile(DirEntry * dir_entry, char *file_path);
static int ViewArchiveFile(char *file_path);
static void hex_edit(char *file_path);
static void printhexline(WINDOW *win, char *line, char *buf, int r, long offset);
static void update_line(WINDOW *win, long line);
static void scroll_down(WINDOW *win);
static void scroll_up(WINDOW *win);
static void update_all_lines(WINDOW *win, char l);
static void Change2Edit(char *file_path);
static void Change2View(char *file_path);
static void SetupViewWindow(char *file_path);
static unsigned char hexval(unsigned char v);
static void change_char(int ch);
static void move_right(WINDOW *win);



int View(DirEntry * dir_entry, char *file_path)
{
  switch( mode )
  {
    case DISK_MODE :
    case USER_MODE :     return( ViewFile(dir_entry, file_path ) );
    case TAPE_MODE:
    case RAR_FILE_MODE:
    case RPM_FILE_MODE:
    case TAR_FILE_MODE : 
    case ZOO_FILE_MODE :
    case ZIP_FILE_MODE :
    case SEVENZIP_FILE_MODE :
    case ISO_FILE_MODE :
    case LHA_FILE_MODE :
    case ARC_FILE_MODE : return( ViewArchiveFile( file_path ) );
    default:             beep(); return( -1 );
  }
}



static int ViewFile(DirEntry * dir_entry, char *file_path)
{
  char *command_line, *aux;
  int  compress_method;
  int  result = -1;
  char *file_p_aux;
  BOOL notice_mapped = FALSE;
  char cwd[PATH_LENGTH+1];
  char path[PATH_LENGTH+1];
      
  command_line = file_p_aux = NULL;

  if( ( file_p_aux = (char *) malloc( COMMAND_LINE_LENGTH + 1 ) ) == NULL )
  {
    ERROR_MSG( "Malloc failed*ABORT" );
    exit( 1 );
  }
  StrCp(file_p_aux, file_path);

  if( access( file_path, R_OK ) )
  {
    (void) sprintf( message, 
            "View not possible!*\"%s\"*%s", 
            file_path, 
            strerror(errno) 
          );
    MESSAGE( message );
    ESCAPE;
  }

  if( ( command_line = malloc( COMMAND_LINE_LENGTH + 1 ) ) == NULL )
  {
    ERROR_MSG( "Malloc failed*ABORT" );
    exit( 1 );
  }
  
  if (( aux = GetExtViewer(file_path))!= NULL) 
  {
     if (strstr(aux,"%s") != NULL) 
     {
        (void) sprintf(command_line, aux, file_p_aux);
     } 
     else {
        (void) sprintf(command_line, "%s %s", aux, file_p_aux);
     }
  } 
  else 
  {
    compress_method = GetFileMethod( file_path );
    if( compress_method == FREEZE_COMPRESS )
    {
      (void) sprintf( command_line, 
              "%s < %s %s | %s", 
              MELT, 
              file_p_aux, 
              ERR_TO_STDOUT,
              PAGER 
            );
    } else if( compress_method == COMPRESS_COMPRESS ) {
        (void) sprintf( command_line, 
            "%s < %s %s | %s", 
            UNCOMPRESS, 
            file_p_aux, 
            ERR_TO_STDOUT,
            PAGER 
          );
    } else if( compress_method == GZIP_COMPRESS ) {
        (void) sprintf( command_line, 
                    "%s < %s %s | %s", 
            GNUUNZIP, 
            file_p_aux, 
            ERR_TO_STDOUT,
            PAGER 
          );
    } else if( compress_method == BZIP_COMPRESS ) {
        (void) sprintf( command_line, 
                    "%s < %s %s | %s", 
            BUNZIP, 
            file_p_aux, 
            ERR_TO_STDOUT,
            PAGER 
          );
    } else if( compress_method == LZIP_COMPRESS ) {
        (void) sprintf( command_line, 
                    "%s -dc < %s %s | %s", 
            LUNZIP, 
            file_p_aux, 
            ERR_TO_STDOUT,
            PAGER 
          );
    } else {
        (void) sprintf( command_line, 
            "%s %s", 
            PAGER,
            file_p_aux
          );
    }
  }

/* --crb3 01oct02: replicating what I did to <e>dit, eliminate
the problem with jstar-chained-thru-less writing new files to
the ytree starting cwd. new code grabbed from execute.c.
*/                                                                                


  if (mode == DISK_MODE)
  {
    if (Getcwd(cwd, PATH_LENGTH) == NULL)
    {
        WARNING("Getcwd failed*\".\"assumed");
        (void) strcpy(cwd, ".");    
    }
    if (chdir(GetPath(dir_entry, path)))
    {
        (void) sprintf(message, "Can't change directory to*\"%s\"", path);
        MESSAGE(message);
    } else {
        result = SystemCall(command_line);
    }
    if( chdir(cwd) )
    {
        (void) sprintf(message, "Can't change directory to*\"%s\"", cwd);
        MESSAGE(message);
    }
  } else {
    result = SystemCall(command_line);  
  }
  
  if(result)                                                                      
  {
    (void) sprintf( message, "can't execute*%s", command_line );
    MESSAGE( message );
  }

  if( notice_mapped ) 
  {
    UnmapNoticeWindow();
  }

FNC_XIT:

  if(file_p_aux)
    free(file_p_aux);
  if(command_line)
    free(command_line);

  return( result );
}



static int ViewArchiveFile(char *file_path)
{
  char *command_line, *aux;
  char buffer[100];
  char *archive;
  int  result = -1;

  if( ( command_line = malloc( COMMAND_LINE_LENGTH + 1 ) ) == NULL )
  {
    ERROR_MSG( "Malloc failed*ABORT" );
    exit( 1 );
  }

  if (( aux = GetExtViewer(file_path)) != NULL) {
     if (strstr(aux,"%s") != NULL) {
    (void) sprintf( buffer, "| %s", PAGER );
     } else {
    (void) sprintf( buffer, "| %s", aux ); /* maybe pipe-able */
     }
  } else {
    (void) sprintf( buffer, "| %s", PAGER );
  }

  archive = (mode == TAPE_MODE) ? statistic.tape_name : statistic.login_path;

  MakeExtractCommandLine( command_line, 
              archive,
              file_path, 
              buffer
            );
  if((result = SystemCall( command_line )))
  {
    (void) sprintf( message, "can't execute*%s", command_line );
    MESSAGE( message );
  }

  free( command_line );

  return( result );
}



/*
static char *strn2print(char *dest, char *src, int c)
{
    dest[c]='\0';
    for( ;c >= 0;c--)
    dest[c] = (isprint(src[c]) ? src[c] : '.');
    return dest;
}
*/


static void printhexline(WINDOW *win, char *line, char *buf, int r, long offset)
{
    char *aux;
    int i;
    aux = (char *) xmalloc(WCOLS );
    if (r==0)
    {
        wclrtoeol(win);
        return;
    }
    if(hexoffset) {
      sprintf(line, "%010X  ", (int)offset);
    } else {
      sprintf(line, "%010d  ", (int)offset);
    }
    for (i = 1; i <= r; i++ ) 
    {
        if ((i == (BYTES / 2) ) || (i == BYTES ))
            sprintf(aux, "%02hhX  ", buf[i-1]);
        else
            sprintf(aux, "%02hhX ", buf[i-1]);
        strcat(line, aux);
    }
    for (i = r+1; i <= BYTES; i++)
    {
        buf[i-1]= ' ';
        if ((i == (BYTES / 2) ) || (i == BYTES ))
            sprintf(aux, "    ");
        else
            sprintf(aux, "   ");
        strcat(line, aux);
    }
/*    strcat(line, " ");*/
    line[strlen(line)] = ' ';
    for (i=0; i< WCOLS-BYTES; i++)
        waddch(win, line[i]| THECOLOR);
    for( i=0; i< BYTES; i++)
        isprint(buf[i]) ? waddch(win, buf[i] | THECOLOR) : waddch(win, ACS_BLOCK | COLOR_PAIR(HIDIR_COLOR));
    free(aux);
    return;
}



static void update_line(WINDOW *win, long line)
{
    int r;
    char *buf;
    char *line_string;
    char msg[50];

    line_string = (char *) xmalloc(WCOLS);
    memset(line_string, ' ', WCOLS);
    line_string[0] = '\0';
    buf = (char *) xmalloc(BYTES);
    memset(buf, ' ', BYTES);
    if (lseek(fd, (line - 1) * BYTES, SEEK_SET)== -1 )
    {
        sprintf(msg, "File seek failed for line: %ld: %s ", line, strerror(errno));
        ERROR_MSG(msg);
        fflush(stdout);
        return;
    }
    r = read(fd, buf, BYTES);
    printhexline(win, line_string, buf, r, (line - 1) * (BYTES));
    xfree(line_string);
    xfree(buf);
}

static void scroll_down(WINDOW *win)
{
    scrollok(win,TRUE);
    wscrl(win,1);
    scrollok(win,FALSE);
    wmove(win, WLINES - 1 , 0);
    update_line(win, current_line + WLINES - 1);
    wnoutrefresh(win);
    doupdate();    
}

static void scroll_up(WINDOW *win)
{
    scrollok(win,TRUE);
    wscrl(win,-1);
    scrollok(win,FALSE);
    wmove(win, 0, 0);
    update_line(win, current_line );
    wnoutrefresh(win);
    doupdate();    
}

static void update_all_lines(WINDOW *win, char l)
{
    long i;

    for (i = current_line; i <= current_line + l; i++)
    {
        wmove(win, i - current_line, 0);
        update_line(win, i);
    }
    wnoutrefresh(win);
    doupdate();    
}


static void Change2Edit(char *file_path)
{
    int i;
    char *str;

    str = (char *)xmalloc(WCOLS);
    
    for(i = WLINES + 4; i < LINES; i++)
    {
        wmove(stdscr,i , 0);
        wclrtoeol(stdscr);
    }

    doupdate();
    
    Print( stdscr, 0, 0, "File: ", MENU_COLOR );
    Print( stdscr, 0, 6, CutPathname(str,file_path,WCOLS-5), HIMENUS_COLOR );
    PrintOptions( stdscr, LINES - 3, 0, "(Edit file in hexadecimal mode)");
    wclrtoeol(stdscr);
    PrintOptions( stdscr, LINES - 2, 0, "(Q)uit   (^L) redraw  (<TAB>) change edit mode");
    wclrtoeol(stdscr);
    PrintOptions( stdscr, LINES - 1, 0, "(NEXT)-(RIGHT)/(PREV)-(LEFT) page   (HOME)-(END) of line   (DOWN)-(UP) line");
    wclrtoeol(stdscr);
    free(str);
    wnoutrefresh(stdscr);
    doupdate();
    return;
}

static void Change2View(char *file_path)
{
    int i;
    char *str;
    
    str = (char *)xmalloc(WCOLS);
    for(i = WLINES + 4; i < LINES; i++)
    {
        wmove(stdscr,i , 0);
        wclrtoeol(stdscr);
    }
    doupdate();
    
    Print( stdscr, 0, 0, "File: ", MENU_COLOR );
    Print( stdscr, 0, 6, CutPathname(str,file_path,WCOLS-5), HIMENUS_COLOR );
    PrintOptions( stdscr, LINES - 3, 0, "View file in hexadecimal mode"); 
    wclrtoeol(stdscr);
    PrintOptions( stdscr, LINES - 2, 0, "(Q)uit   (^L) redraw  (E)dit hex");
    wclrtoeol(stdscr);
    PrintOptions( stdscr, LINES - 1, 0, "(NEXT)-(RIGHT)/(PREV)-(LEFT) page   (HOME)-(END) of line   (DOWN)-(UP) line");
    wclrtoeol(stdscr);
    free(str);
    wnoutrefresh(stdscr);
    return;
}

static void SetupViewWindow(char *file_path)
{
    int i;

    int myCOLS  = (COLS > 18) ? COLS : 19; /* to display (at least) address, frame and one byte */
    int myLINES = (LINES > 6) ? LINES : 7; /* to display (at least) footer, border and one line */
    
    WLINES=myLINES - 6;
    WCOLS=myCOLS - 2;
    if (BORDER)
        delwin(BORDER);
    BORDER=newwin(WLINES + 2, WCOLS + 2, 1, 0);
    if (VIEW)
        delwin(VIEW);
    VIEW=newwin(WLINES, WCOLS, 2, 1);
    keypad(VIEW,TRUE);
    scrollok(VIEW,FALSE);
    clearok(VIEW,TRUE);
    leaveok(VIEW,FALSE);
/*    werase(VIEW);*/
    WbkgdSet(VIEW,COLOR_PAIR(WINDIR_COLOR));
    wclear(VIEW);
    for( i = 0; i < WLINES - 1; i++)
    {
        wmove(VIEW,i,0);
        wclrtoeol(VIEW);
    }
    WbkgdSet(BORDER,COLOR_PAIR(WINDIR_COLOR)|A_BOLD);
    Change2View(file_path);
    box(BORDER,0,0);
    RefreshWindow(BORDER);
    RefreshWindow(VIEW);  
    BYTES = (WCOLS - 13) / 4;
    return;
}



static unsigned char hexval(unsigned char v) {
    if (v >= 'a' && v <= 'f')
        v = v - 'a' + 10;
    else if (v >= '0' && v <= '9')
        v = v - '0';
    return v;
}


static void change_char(int ch)
{
    CHANGES *cambio=NULL;
    char pp=0;
    char msg[50];
    
    cambio = malloc(sizeof(struct MODIF));
    cambio -> pos = ( (cursor_pos_y + current_line - 1) * BYTES) + CURSOR_POSX;
    if (lseek(fd, cambio -> pos, SEEK_SET)== -1 )
    {
        sprintf(msg,"File seek failed: %s ", strerror(errno));
        ERROR_MSG(msg);
        fflush(stdout);
        free(cambio);
        return;
    }

    if ((read(fd, &cambio -> old_char,1)==1))
    { 
        if (lseek(fd, cambio -> pos, SEEK_SET)!= -1 )
        {
            if (inhex) {

                switch( ch) {

                    case '0': case '1': case '2': case '3': case '4':
                    case '5': case '6': case '7': case '8': case '9':
                    case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':

                    if ((cursor_pos_x%2)==1)
                        pp = (cambio -> old_char & 0xF0) | (hexval(ch));
                    else
                        pp = (cambio -> old_char & 0x0F) | (hexval(ch) << 4);
                    touchwin(VIEW);
                    break;

                    default:
                    beep();
                    touchwin(VIEW);
                    free(cambio);
                    return;
                    break;
                }
            } else {
                pp = ch;
            }

            if (write(fd, &pp, 1) != 1)
            {
                sprintf(msg,"Write to file failed: %s ", strerror(errno));
                ERROR_MSG(msg);
                fflush(stdout);
                free(cambio);
                return;
            }
            cambio -> new_char = pp;
            cambio -> next = changes;
            changes = cambio;
        } else {
            sprintf(msg,"File seek failed: %s ", strerror(errno));
            ERROR_MSG(msg);
            fflush(stdout);
            free(cambio);
            return;
        }
    } else {
        sprintf(msg,"Read from file failed: %s ", strerror(errno));
        ERROR_MSG(msg);
        fflush(stdout);
        free(cambio);
        return;
    }
    return;
}


static void move_right(WINDOW *win)
{
    fstat(fd,&fdstat);
    cursor_pos_x++;
    if (fdstat.st_size > ((cursor_pos_y+current_line-1) * BYTES + CURSOR_POSX )) {
        cursor_pos_x--;
        if ( cursor_pos_x < CANTX(BYTES) - 1 ) {
            cursor_pos_x += 1;
            wmove( win, cursor_pos_y, CURSOR_POS_X);
        } else {
            if (fdstat.st_size >= ((current_line+cursor_pos_y) * BYTES) ) {
                if (cursor_pos_y < WLINES-1 ) {
                    cursor_pos_y++;
                    cursor_pos_x = 0;
                    wmove( win, cursor_pos_y, CURSOR_POS_X);
                } else {
                    current_line++;
                    scroll_down(win);
                    cursor_pos_x = 0;
                    wmove( win, cursor_pos_y, CURSOR_POS_X);
                }
            } else {
                beep();
            }
        }
    } else {
        cursor_pos_x--;
        beep();
    }
    return;
}


static void DoResize(char *file_path)
{
    int old_cursor_pos_x = cursor_pos_x;
    int old_cursor_pos_y = cursor_pos_y;
    int old_WLINES = WLINES;
    int old_WCOLS = WCOLS;
    int offset = (cursor_pos_y + current_line - 1) * BYTES + CURSOR_POSX;
    int new_cols;
    int new_lines;

    SetupViewWindow(file_path);

    new_cols   = offset % BYTES;
    new_lines  = offset / BYTES;

    if(WLINES < old_WLINES)
    {
        if(old_cursor_pos_y >= WLINES)  /* view is getting smaller (heigh) */
        {
            /* current position is (now) in invisible line; "scoll down" */
            int n = old_WLINES - WLINES;
            current_line += n;
            cursor_pos_y -= n;
            if(cursor_pos_y < 0)
            {
                current_line += (cursor_pos_y * -1);
                cursor_pos_y = 0;
            }
        }
    }

    if(WCOLS != old_WCOLS) 
    {
        /* reposition x */
        cursor_pos_x = (inhex) ? (new_cols * 2 + (old_cursor_pos_x % 2)) : new_cols;

        /* reposition y */
        while((cursor_pos_y + current_line - 1) < new_lines)  /* maybe view is smaller (less columns) */
        {
            if(cursor_pos_y < (WLINES-1))
                cursor_pos_y++;
            else
                current_line++;
        }

        while((cursor_pos_y + current_line - 1) > new_lines)  /* maybe view is larger (more columns) */
        {
            if(cursor_pos_y > 0)
                cursor_pos_y--;
            else
                current_line--;
        }
    }


    Change2Edit(file_path);
    update_all_lines(VIEW,WLINES-1);
    wmove( VIEW, cursor_pos_y, CURSOR_POS_X);
    wnoutrefresh(VIEW);
    doupdate();
    resize_done = TRUE;
}



static void hex_edit(char *file_path)
{
    int ch;
    char msg[50];

    BOOL QUIT=FALSE;

    cursor_pos_x = cursor_pos_y = 0;
    fd2 = fd;
    fd=open(file_path,O_RDWR);
    if (fd == -1) {
        sprintf(msg,"File open failed: %s ", strerror(errno));
        ERROR_MSG(msg);
        touchwin(VIEW);
        fd = fd2;
        return;
    }
    inedit=TRUE;
    update_all_lines(VIEW,WLINES-1);
    leaveok( VIEW, FALSE);
    curs_set( 1);
    wmove( VIEW, cursor_pos_y, CURSOR_POS_X);
    wnoutrefresh(VIEW);

    while (!QUIT) {

       doupdate();
       ch = (resize_request) ? -1 : Getch();

#ifdef VI_KEYS
        ch = ViKey(ch);
#endif
        if (resize_request)
        {
            DoResize(file_path);
            resize_request = FALSE;
            continue;
        }

        switch(ch) {

#ifdef KEY_RESIZE

            case KEY_RESIZE: resize_request = TRUE;
                    break;
#endif
            case ESC: QUIT=TRUE;
                    break;

            case KEY_DOWN: /*ScrollDown();*/
                fstat(fd,&fdstat);
                if (fdstat.st_size > ((cursor_pos_y + current_line - 1) * BYTES + CURSOR_POSX)) {

                    if (fdstat.st_size > ((cursor_pos_y + current_line - 1 + 1) * BYTES + CURSOR_POSX)) {
                    
                        if (cursor_pos_y < WLINES-1) {
                            wmove( VIEW, ++cursor_pos_y, CURSOR_POS_X);
                            wnoutrefresh(VIEW);
                        } else {
                            ++current_line;
                            scroll_down(VIEW);
                            wmove( VIEW, cursor_pos_y, CURSOR_POS_X);
                            wnoutrefresh(VIEW);
                        }
                    } else {
                        /* special case: last line */

                        if (fdstat.st_size > ((cursor_pos_y + current_line - 1 + 1) * BYTES)) {
                        
                            for(cursor_pos_x = 0; (CURSOR_POSX + 1) < (fdstat.st_size % BYTES); cursor_pos_x++);

                            if (cursor_pos_y < WLINES-1) {
                                wmove( VIEW, ++cursor_pos_y, CURSOR_POS_X);
                                wnoutrefresh(VIEW);
                            } else {
                                ++current_line;
                                scroll_down(VIEW);
                                wmove( VIEW, cursor_pos_y, CURSOR_POS_X);
                                wnoutrefresh(VIEW);
                            }
                        }
                    }
                } else {
                    beep();
                }
                break;
        case KEY_UP: /*ScroollUp();*/
                if (cursor_pos_y > 0)
                {
                    wmove( VIEW, --cursor_pos_y, CURSOR_POS_X);
                    wnoutrefresh(VIEW);
                } else if (current_line > 1) {
                    current_line--;
                    scroll_up(VIEW);
                    wmove( VIEW, cursor_pos_y, CURSOR_POS_X);
                    wnoutrefresh(VIEW);
                } else {        
                    beep();
                }
                break;
        case KEY_LEFT: /* move 1 char left */
                if ( cursor_pos_x > 0 ) {
                    cursor_pos_x-=1;
                    wmove( VIEW, cursor_pos_y, CURSOR_POS_X);
                } else if (cursor_pos_y > 0 ) {
                    /*cursor_pos_x=ultimo_caracter;*/
                    cursor_pos_x=CANTX(BYTES) - 1;
                    wmove( VIEW, --cursor_pos_y,CURSOR_POS_X);
                } else if (current_line > 1) {
                    current_line--;
                    scroll_up(VIEW);
                    cursor_pos_x=CANTX(BYTES) - 1;
                    wmove( VIEW, cursor_pos_y, CURSOR_POS_X);
                } else {
                    beep();
                }
                wnoutrefresh(VIEW);
                break;
        case KEY_PPAGE: /*ScrollPageDown();*/
                if (current_line > WLINES)
                    current_line -= WLINES;
                else if (current_line > 1)
                   current_line = 1;
                else
                    beep();
                update_all_lines(VIEW,WLINES);
                wmove( VIEW, cursor_pos_y, CURSOR_POS_X);
                wnoutrefresh(VIEW);
                break;
        case KEY_RIGHT: move_right(VIEW);
                wnoutrefresh(VIEW);
                break;
        case KEY_NPAGE: /*ScroollPageUp();*/
                fstat(fd,&fdstat);
                if (fdstat.st_size > ((current_line - 1 + WLINES + cursor_pos_y) * BYTES) + CURSOR_POSX ) {
                    current_line += WLINES;
                } else {
                    int n;
                    n = fdstat.st_size / BYTES; /* numer of full lines */
                    if(fdstat.st_size % BYTES) {
                    n++; /* plus 1 not fully used line */
                    }
                    if(current_line != n) {
                        current_line = n;
                        cursor_pos_y = 0;
                        for(cursor_pos_x = 0; (CURSOR_POSX + 1) < (fdstat.st_size % BYTES); cursor_pos_x++);
                    } else {
                        beep();
                    }
                }
                update_all_lines(VIEW,WLINES);
                wmove( VIEW, cursor_pos_y, CURSOR_POS_X);
                wnoutrefresh(VIEW);
                break;
        case KEY_HOME: 
                if (CURSOR_POSX > 0) {
                    cursor_pos_x = 0;
                    wmove( VIEW, cursor_pos_y, CURSOR_POS_X);
                } else {
                    beep();
                }
                wnoutrefresh(VIEW);
                break;
        case KEY_END: 
                fstat(fd,&fdstat);
                if ( ((cursor_pos_y + current_line) * BYTES) > fdstat.st_size ) {
                    cursor_pos_x = CANTX(fdstat.st_size % BYTES) - 1;
                } else {
                    cursor_pos_x = CANTX(BYTES)-1;
                }
                wmove( VIEW, cursor_pos_y, CURSOR_POS_X);
                wnoutrefresh(VIEW);
                break;
        case '\t' :
                /* move cursor to the the other part of the window*/
                if (inhex){
                    inhex=FALSE;
                    cursor_pos_x=cursor_pos_x/2;
                } else {
                    inhex=TRUE;
                    cursor_pos_x=cursor_pos_x*2;
                }
                wmove( VIEW, cursor_pos_y, CURSOR_POS_X);
                wnoutrefresh(VIEW);
                break;
        case 'L' & 0x1f: 
                clearok(stdscr,TRUE);
                RefreshWindow(stdscr);
                break;

        case 'q': 
        case 'Q': 
                if (inhex) {
                    QUIT=TRUE;
                    break;
                }
        default:
                change_char(ch); 
                wmove(VIEW, cursor_pos_y, 0);
                update_line(VIEW, current_line+cursor_pos_y);
                move_right(VIEW);
                wmove(VIEW, cursor_pos_y, CURSOR_POS_X);
                wnoutrefresh(VIEW);
                break;
        }
    }
    curs_set( 0);
    close(fd);
    fd=fd2;
    inedit=FALSE;
    return;
}


int InternalView(char *file_path)
{
    int ch;
    BOOL QUIT=FALSE;
    
    hexoffset = (!strcmp(HEXEDITOFFSET, "HEX")) ? TRUE : FALSE;

    if (stat(file_path, &fdstat)!=0)
        return -1;
    if (!(S_ISREG(fdstat.st_mode)) || S_ISBLK(fdstat.st_mode))
        return -1;
    fd=open(file_path,O_RDONLY);
    if (fd == -1)
        return -1;
    SetupViewWindow(file_path);
    current_line = 1;
    update_all_lines(VIEW,WLINES-1);

    while (!QUIT) {

       ch = (resize_request) ? -1 : WGetch(VIEW);

#ifdef VI_KEYS
    ch = ViKey(ch);
#endif

       if (resize_request)
       {
            DoResize(file_path);
            resize_request = FALSE;
            continue;
        }

        switch(ch) {

#ifdef KEY_RESIZE
            case KEY_RESIZE: resize_request = TRUE;
                     break;
#endif

            case ESC:
            case 'q':
            case 'Q': QUIT=TRUE;
                break;
            case 'e':
            case 'E': Change2Edit(file_path);
                    hex_edit(file_path);
                    update_all_lines(VIEW,WLINES-1);
                    Change2View(file_path);
                    break;
            case KEY_DOWN: /*ScrollDown();*/
                    fstat(fd,&fdstat);
                    if (fdstat.st_size > (current_line * BYTES) )
                    {
                        current_line++;
                        scroll_down(VIEW);
                    } else {
                        beep();
                    }
                    break;
            case KEY_UP: /*ScroollUp();*/
                    if (current_line > 1)
                    {
                        current_line--;
                        scroll_up(VIEW);
                    } else {
                        beep();
                    }
                    break;
            case KEY_LEFT:
            case KEY_PPAGE: /*ScrollPageDown();*/
                    if (current_line > WLINES)
                        current_line -= WLINES;
                    else
                    if (current_line > 1)
                        current_line = 1;
                    else
                        beep();
                    update_all_lines(VIEW,WLINES);
                    break;
            case KEY_RIGHT:
            case KEY_NPAGE: /*ScroollPageUp();*/
                    fstat(fd,&fdstat);
                    if (fdstat.st_size > ((current_line - 1 + WLINES) * BYTES) ) {
                        current_line += WLINES;
                    } else {
                        int n;
                        n = fdstat.st_size / BYTES; /* numer of full lines */
                        if(fdstat.st_size % BYTES) {
                            n++; /* plus 1 not fully used line */
                        }
                        if(current_line != n) {
                            current_line = n;
                        } else {
                            beep();
                        }
                    }
                    update_all_lines(VIEW,WLINES);
                    break;
            case KEY_HOME: /*ScrollHome();*/
                    if (current_line > 1)
                    {
                        current_line = 1;
                        update_all_lines(VIEW,WLINES-1);
                    } else {
                        beep(); 
                    }            
                    break;
            case KEY_END: /*ScrollEnd();*/
                fstat(fd,&fdstat);
                if (fdstat.st_size >= BYTES * 2) {
                    current_line = (fdstat.st_size - BYTES) / BYTES;
                }
                else
                    beep();
                update_all_lines(VIEW,WLINES);
                break;
            case 'L' & 0x1f: 
                clearok(stdscr,TRUE);
                RefreshWindow(stdscr);
                break;
            default: break;
        }
    }
    Print( stdscr, 0, 0, "Path: ", MENU_COLOR );
    delwin(VIEW);
    VIEW = NULL;
    delwin(BORDER);
    BORDER = NULL;
    touchwin(stdscr);
    wnoutrefresh(stdscr);
    close(fd);
    resize_request = resize_done;
    resize_done = FALSE;
    return 0;
}

