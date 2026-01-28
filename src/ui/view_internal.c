/***************************************************************************
 *
 * src/ui/view_internal.c
 * Internal Hex/Text Viewer implementation
 *
 ***************************************************************************/

#include "ytree.h"
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

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
#define THECOLOR ((inedit)? COLOR_PAIR(CPAIR_STATS):COLOR_PAIR(CPAIR_DIR))

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
static void DoResize(char *file_path);


static void printhexline(WINDOW *win, char *line, char *buf, int r, long offset)
{
    char *aux;
    aux = (char *) xmalloc(WCOLS);

    if (r==0)
    {
        wclrtoeol(win);
        return;
    }
    if(hexoffset) {
      snprintf(line, WCOLS, "%010lX  ", offset);
    } else {
      snprintf(line, WCOLS, "%010ld  ", offset);
    }
    for (int i = 1; i <= r; i++ )
    {
        if ((i == (BYTES / 2) ) || (i == BYTES ))
            snprintf(aux, WCOLS, "%02hhX  ", buf[i-1]);
        else
            snprintf(aux, WCOLS, "%02hhX ", buf[i-1]);
        strcat(line, aux);
    }
    for (int i = r+1; i <= BYTES; i++)
    {
        buf[i-1]= ' ';
        if ((i == (BYTES / 2) ) || (i == BYTES ))
            snprintf(aux, WCOLS, "    ");
        else
            snprintf(aux, WCOLS, "   ");
        strcat(line, aux);
    }
/*    strcat(line, " ");*/
    line[strlen(line)] = ' ';
    for (int i=0; i< WCOLS-BYTES; i++)
        waddch(win, line[i]| THECOLOR);
    for( int i=0; i< BYTES; i++)
        isprint(buf[i]) ? waddch(win, buf[i] | THECOLOR) : waddch(win, ACS_BLOCK | COLOR_PAIR(CPAIR_HIDIR));
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
        snprintf(msg, sizeof(msg), "File seek failed for line: %ld: %s ", line, strerror(errno));
        ERROR_MSG(msg);
        fflush(stdout);
        free(line_string);
        free(buf);
        return;
    }
    r = read(fd, buf, BYTES);
    printhexline(win, line_string, buf, r, (line - 1) * (BYTES));
    free(line_string);
    free(buf);
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

    str = (char *) xmalloc(WCOLS);

    for(i = layout.message_y; i <= layout.status_y; i++)
    {
        wmove(stdscr,i , 0);
        wclrtoeol(stdscr);
    }

    doupdate();

    Print( stdscr, layout.header_y, 0, "File: ", CPAIR_MENU );
    Print( stdscr, layout.header_y, 6, CutPathname(str,file_path,WCOLS-5), CPAIR_HIMENUS );
    PrintOptions( stdscr, layout.message_y, 0, "(Edit file in hexadecimal mode)");
    wclrtoeol(stdscr);
    PrintOptions( stdscr, layout.prompt_y, 0, "(Q)uit   (^L) redraw  (<TAB>) change edit mode");
    wclrtoeol(stdscr);
    PrintOptions( stdscr, layout.status_y, 0, "(NEXT)-(RIGHT)/(PREV)-(LEFT) page   (HOME)-(END) of line   (DOWN)-(UP) line");
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

    str = (char *) xmalloc(WCOLS);
    for(i = layout.message_y; i <= layout.status_y; i++)
    {
        wmove(stdscr,i , 0);
        wclrtoeol(stdscr);
    }
    doupdate();

    Print( stdscr, layout.header_y, 0, "File: ", CPAIR_MENU );
    Print( stdscr, layout.header_y, 6, CutPathname(str,file_path,WCOLS-5), CPAIR_HIMENUS );
    PrintOptions( stdscr, layout.message_y, 0, "View file in hexadecimal mode");
    wclrtoeol(stdscr);
    PrintOptions( stdscr, layout.prompt_y, 0, "(Q)uit   (^L) redraw  (E)dit hex");
    wclrtoeol(stdscr);
    PrintOptions( stdscr, layout.status_y, 0, "(NEXT)-(RIGHT)/(PREV)-(LEFT) page   (HOME)-(END) of line   (DOWN)-(UP) line");
    wclrtoeol(stdscr);
    free(str);
    wnoutrefresh(stdscr);
    return;
}

static void SetupViewWindow(char *file_path)
{
    int i;
    int start_y = layout.dir_win_y;
    int start_x = layout.dir_win_x;
    int available_height = layout.message_y - start_y;
    int width = layout.main_win_width;

    /* Ensure minimal dimensions */
    if (available_height < 3) available_height = 3;
    if (width < 20) width = 20;

    WLINES = available_height - 2;
    WCOLS = width - 2;

    if (BORDER)
        delwin(BORDER);
    BORDER=newwin(available_height, width, start_y, start_x);

    if (VIEW)
        delwin(VIEW);
    VIEW=newwin(WLINES, WCOLS, start_y + 1, start_x + 1);

    keypad(VIEW,TRUE);
    scrollok(VIEW,FALSE);
    clearok(VIEW,TRUE);
    leaveok(VIEW,FALSE);
/*    werase(VIEW);*/
    WbkgdSet(VIEW,COLOR_PAIR(CPAIR_WINDIR));
    wclear(VIEW);
    for( i = 0; i < WLINES - 1; i++)
    {
        wmove(VIEW,i,0);
        wclrtoeol(VIEW);
    }
    WbkgdSet(BORDER,COLOR_PAIR(CPAIR_WINDIR)|A_BOLD);
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
    if (cambio == NULL) {
      ERROR_MSG("Malloc failed*ABORT");
      exit(1);
    }
    cambio -> pos = ( (cursor_pos_y + current_line - 1) * BYTES) + CURSOR_POSX;
    if (lseek(fd, cambio -> pos, SEEK_SET)== -1 )
    {
        snprintf(msg, sizeof(msg), "File seek failed: %s ", strerror(errno));
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
                    if (strtol(AUDIBLEERROR, NULL, 0) != 0) beep();
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
                snprintf(msg, sizeof(msg), "Write to file failed: %s ", strerror(errno));
                ERROR_MSG(msg);
                fflush(stdout);
                free(cambio);
                return;
            }
            cambio -> new_char = pp;
            cambio -> next = changes;
            changes = cambio;
        } else {
            snprintf(msg, sizeof(msg), "File seek failed: %s ", strerror(errno));
            ERROR_MSG(msg);
            fflush(stdout);
            free(cambio);
            return;
        }
    } else {
        snprintf(msg, sizeof(msg), "Read from file failed: %s ", strerror(errno));
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
            }
        }
    } else {
        cursor_pos_x--;
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
        snprintf(msg, sizeof(msg), "File open failed: %s ", strerror(errno));
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
                }
                wnoutrefresh(VIEW);
                break;
        case KEY_PPAGE: /*ScrollPageDown();*/
                if (current_line > WLINES)
                    current_line -= WLINES;
                else if (current_line > 1)
                   current_line = 1;
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
                /* fallthrough */
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
    int result = VIEW_EXIT;

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
            case 'Q': QUIT=TRUE; result = VIEW_EXIT;
                break;
            case ' ':
            case 'n': QUIT=TRUE; result = VIEW_NEXT;
                break;
            case 'p': QUIT=TRUE; result = VIEW_PREV;
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
                    }
                    break;
            case KEY_UP: /*ScroollUp();*/
                    if (current_line > 1)
                    {
                        current_line--;
                        scroll_up(VIEW);
                    }
                    break;
            case KEY_LEFT:
            case KEY_PPAGE: /*ScrollPageDown();*/
                    if (current_line > WLINES)
                        current_line -= WLINES;
                    else
                    if (current_line > 1)
                        current_line = 1;
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
                        }
                    }
                    update_all_lines(VIEW,WLINES);
                    break;
            case KEY_HOME: /*ScrollHome();*/
                    if (current_line > 1)
                    {
                        current_line = 1;
                        update_all_lines(VIEW,WLINES-1);
                    }
                    break;
            case KEY_END: /*ScrollEnd();*/
                fstat(fd,&fdstat);
                if (fdstat.st_size >= BYTES * 2) {
                    current_line = (fdstat.st_size - BYTES) / BYTES;
                }
                update_all_lines(VIEW,WLINES);
                break;
            case 'L' & 0x1f:
                clearok(stdscr,TRUE);
                RefreshWindow(stdscr);
                break;
            default: break;
        }
    }
    Print( stdscr, 0, 0, "Path: ", CPAIR_MENU );
    delwin(VIEW);
    VIEW = NULL;
    delwin(BORDER);
    BORDER = NULL;
    touchwin(stdscr);
    wnoutrefresh(stdscr);
    close(fd);
    resize_request = resize_done;
    resize_done = FALSE;
    return result;
}