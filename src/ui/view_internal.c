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


#define CURSOR_CALC_X                                                          \
  (10 + (((cursor_pos_x / 2) < (ctx->viewer.bytes / 2)) ? 2 : 3) + (cursor_pos_x) +        \
   (cursor_pos_x / 2))
#define CURSOR_POS_X ((ctx->viewer.inhex) ? CURSOR_CALC_X : (ctx->viewer.wcols - ctx->viewer.bytes + cursor_pos_x))
#define C_POSX (((cursor_pos_x % 2) != 1) ? cursor_pos_x : (cursor_pos_x - 1))
#define CURSOR_POSX ((ctx->viewer.inhex) ? (C_POSX / 2) : cursor_pos_x)
#define CANTX(x) ((ctx->viewer.inhex) ? (x * 2) : x)
#define THECOLOR ((ctx->viewer.inedit) ? COLOR_PAIR(CPAIR_STATS) : COLOR_PAIR(CPAIR_DIR))

static void hex_edit(ViewContext *ctx, char *file_path);
static void printhexline(ViewContext *ctx, WINDOW *win, char *line, char *buf, int r,
                         long offset);
static void update_line(ViewContext *ctx, WINDOW *win, long line);
static void scroll_down(ViewContext *ctx, WINDOW *win);
static void scroll_up(ViewContext *ctx, WINDOW *win);
static void update_all_lines(ViewContext *ctx, WINDOW *win, char l);
static void Change2Edit(const ViewContext *ctx, const char *file_path);
static void Change2View(const ViewContext *ctx, const char *file_path);
static void SetupViewWindow(ViewContext *ctx, const char *file_path);
static unsigned char hexval(unsigned char v);
static void change_char(ViewContext *ctx, int ch);
static void move_right(ViewContext *ctx, WINDOW *win);
static void DoResize(ViewContext *ctx, char *file_path);

static void printhexline(ViewContext *ctx, WINDOW *win, char *line, char *buf, int r,
                         long offset) {
  char *aux;
  aux = (char *)xmalloc(ctx->viewer.wcols);

  if (r == 0) {
    wclrtoeol(win);
    return;
  }
  if (ctx->viewer.hexoffset) {
    snprintf(line, ctx->viewer.wcols, "%010lX  ", offset);
  } else {
    snprintf(line, ctx->viewer.wcols, "%010ld  ", offset);
  }
  for (int i = 1; i <= r; i++) {
    if ((i == (ctx->viewer.bytes / 2)) || (i == ctx->viewer.bytes))
      snprintf(aux, ctx->viewer.wcols, "%02X  ", (unsigned char)buf[i - 1]);
    else
      snprintf(aux, ctx->viewer.wcols, "%02X ", (unsigned char)buf[i - 1]);
    strcat(line, aux);
  }
  for (int i = r + 1; i <= ctx->viewer.bytes; i++) {
    buf[i - 1] = ' ';
    if ((i == (ctx->viewer.bytes / 2)) || (i == ctx->viewer.bytes))
      snprintf(aux, ctx->viewer.wcols, "    ");
    else
      snprintf(aux, ctx->viewer.wcols, "   ");
    strcat(line, aux);
  }
  /*    strcat(line, " ");*/
  line[strlen(line)] = ' ';
  for (int i = 0; i < ctx->viewer.wcols - ctx->viewer.bytes; i++)
    waddch(win, line[i] | THECOLOR);
  for (int i = 0; i < ctx->viewer.bytes; i++)
    isprint(buf[i]) ? waddch(win, buf[i] | THECOLOR)
                    : waddch(win, ACS_BLOCK | COLOR_PAIR(CPAIR_HIDIR));
  free(aux);
  return;
}

static void update_line(ViewContext *ctx, WINDOW *win, long line) {
  int r;
  char *buf;
  char *line_string;

  line_string = (char *)xmalloc(ctx->viewer.wcols);
  memset(line_string, ' ', ctx->viewer.wcols);
  line_string[0] = '\0';
  buf = (char *)xmalloc(ctx->viewer.bytes);
  memset(buf, ' ', ctx->viewer.bytes);
  if (lseek(fd, (line - 1) * ctx->viewer.bytes, SEEK_SET) == -1) {
    char msg[50];
    snprintf(msg, sizeof(msg), "File seek failed for line: %ld: %s ", line,
             strerror(errno));
    UI_Error(ctx, __FILE__, __LINE__, "%s", msg);
    fflush(stdout);
    free(line_string);
    free(buf);
    return;
  }
  r = read(fd, buf, ctx->viewer.bytes);
  printhexline(ctx, win, line_string, buf, r, (line - 1) * (ctx->viewer.bytes));
  free(line_string);
  free(buf);
}

static void scroll_down(ViewContext *ctx, WINDOW *win) {
  scrollok(win, TRUE);
  wscrl(win, 1);
  scrollok(win, FALSE);
  wmove(win, ctx->viewer.wlines - 1, 0);
  update_line(ctx, win, current_line + ctx->viewer.wlines - 1);
  wnoutrefresh(win);
  doupdate();
}

static void scroll_up(ViewContext *ctx, WINDOW *win) {
  scrollok(win, TRUE);
  wscrl(win, -1);
  scrollok(win, FALSE);
  wmove(win, 0, 0);
  update_line(ctx, win, current_line);
  wnoutrefresh(win);
  doupdate();
}

static void update_all_lines(ViewContext *ctx, WINDOW *win, char l) {
  long i;

  for (i = current_line; i <= current_line + l; i++) {
    wmove(win, i - current_line, 0);
    update_line(ctx, win, i);
  }
  wnoutrefresh(win);
  doupdate();
}

static void Change2Edit(const ViewContext *ctx, const char *file_path) {
  int i;
  char *str;

  str = (char *)xmalloc(ctx->viewer.wcols);

  for (i = ctx->layout.message_y; i <= ctx->layout.status_y; i++) {
    wmove(stdscr, i, 0);
    wclrtoeol(stdscr);
  }

  doupdate();

  Print(stdscr, ctx->layout.header_y, 0, "File: ", CPAIR_MENU);
  Print(stdscr, ctx->layout.header_y, 6, CutPathname(str, file_path, ctx->viewer.wcols - 5),
        CPAIR_HIMENUS);
  PrintOptions(stdscr, ctx->layout.message_y, 0,
               "(Edit file in hexadecimal mode)");
  wclrtoeol(stdscr);
  PrintOptions(stdscr, ctx->layout.prompt_y, 0,
               "(Q)uit   (^L) redraw  (<TAB>) change edit mode");
  wclrtoeol(stdscr);
  PrintOptions(stdscr, ctx->layout.status_y, 0,
               "(NEXT)-(RIGHT)/(PREV)-(LEFT) page   (HOME)-(END) of line   "
               "(DOWN)-(UP) line");
  wclrtoeol(stdscr);
  free(str);
  wnoutrefresh(stdscr);
  doupdate();
  return;
}

static void Change2View(const ViewContext *ctx, const char *file_path) {
  int i;
  char *str;

  str = (char *)xmalloc(ctx->viewer.wcols);
  for (i = ctx->layout.message_y; i <= ctx->layout.status_y; i++) {
    wmove(stdscr, i, 0);
    wclrtoeol(stdscr);
  }
  doupdate();

  Print(stdscr, ctx->layout.header_y, 0, "File: ", CPAIR_MENU);
  Print(stdscr, ctx->layout.header_y, 6, CutPathname(str, file_path, ctx->viewer.wcols - 5),
        CPAIR_HIMENUS);
  PrintOptions(stdscr, ctx->layout.message_y, 0,
               "View file in hexadecimal mode");
  wclrtoeol(stdscr);
  PrintOptions(stdscr, ctx->layout.prompt_y, 0,
               "(Q)uit   (^L) redraw  (E)dit hex");
  wclrtoeol(stdscr);
  PrintOptions(stdscr, ctx->layout.status_y, 0,
               "(NEXT)-(RIGHT)/(PREV)-(LEFT) page   (HOME)-(END) of line   "
               "(DOWN)-(UP) line");
  wclrtoeol(stdscr);
  free(str);
  wnoutrefresh(stdscr);
  return;
}

static void SetupViewWindow(ViewContext *ctx, const char *file_path) {
  int i;
  int start_y = ctx->layout.dir_win_y;
  int start_x = ctx->layout.dir_win_x;
  int available_height = ctx->layout.message_y - start_y;
  int width = ctx->layout.main_win_width;

  /* Ensure minimal dimensions */
  if (available_height < 3)
    available_height = 3;
  if (width < 20)
    width = 20;

  ctx->viewer.wlines = available_height - 2;
  ctx->viewer.wcols = width - 2;

  if (ctx->viewer.border)
    delwin(ctx->viewer.border);
  ctx->viewer.border = newwin(available_height, width, start_y, start_x);

  if (ctx->viewer.view)
    delwin(ctx->viewer.view);
  ctx->viewer.view = newwin(ctx->viewer.wlines, ctx->viewer.wcols, start_y + 1, start_x + 1);

  keypad(ctx->viewer.view, TRUE);
  scrollok(ctx->viewer.view, FALSE);
  clearok(ctx->viewer.view, TRUE);
  leaveok(ctx->viewer.view, FALSE);
  /*    werase(ctx->viewer.view);*/
  WbkgdSet(ctx, ctx->viewer.view, COLOR_PAIR(CPAIR_WINDIR));
  wclear(ctx->viewer.view);
  for (i = 0; i < ctx->viewer.wlines - 1; i++) {
    wmove(ctx->viewer.view, i, 0);
    wclrtoeol(ctx->viewer.view);
  }
  WbkgdSet(ctx, ctx->viewer.border, COLOR_PAIR(CPAIR_WINDIR) | A_BOLD);
  Change2View(ctx, file_path);
  box(ctx->viewer.border, 0, 0);
  RefreshWindow(ctx->viewer.border);
  RefreshWindow(ctx->viewer.view);
  ctx->viewer.bytes = (ctx->viewer.wcols - 13) / 4;
  return;
}

static unsigned char hexval(unsigned char v) {
  if (v >= 'a' && v <= 'f')
    v = v - 'a' + 10;
  else if (v >= '0' && v <= '9')
    v = v - '0';
  return v;
}

static void change_char(ViewContext *ctx, int ch) {
  CHANGES *cambio = NULL;
  char pp = 0;
  char msg[50];

  cambio = malloc(sizeof(struct MODIF));
  if (cambio == NULL) {
    UI_Error(ctx, __FILE__, __LINE__, "Malloc failed*ABORT");
    exit(1);
  }
  cambio->pos = ((cursor_pos_y + current_line - 1) * ctx->viewer.bytes) + CURSOR_POSX;
  if (lseek(fd, cambio->pos, SEEK_SET) == -1) {
    snprintf(msg, sizeof(msg), "File seek failed: %s ", strerror(errno));
    UI_Error(ctx, __FILE__, __LINE__, "%s", msg);
    fflush(stdout);
    free(cambio);
    return;
  }

  if ((read(fd, &cambio->old_char, 1) == 1)) {
    if (lseek(fd, cambio->pos, SEEK_SET) != -1) {
      if (ctx->viewer.inhex) {

        switch (ch) {

        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':

          if ((cursor_pos_x % 2) == 1)
            pp = (cambio->old_char & 0xF0) | (hexval(ch));
          else
            pp = (cambio->old_char & 0x0F) | (hexval(ch) << 4);
          touchwin(ctx->viewer.view);
          break;

        default:
          if (strtol((GetProfileValue)(ctx, "AUDIBLEERROR"), NULL, 0) != 0)
            UI_Beep(ctx, FALSE);
          touchwin(ctx->viewer.view);
          free(cambio);
          return;
          break;
        }
      } else {
        pp = ch;
      }

      if (write(fd, &pp, 1) != 1) {
        snprintf(msg, sizeof(msg), "Write to file failed: %s ",
                 strerror(errno));
        UI_Error(ctx, __FILE__, __LINE__, "%s", msg);
        fflush(stdout);
        free(cambio);
        return;
      }
      cambio->new_char = pp;
      cambio->next = changes;
      changes = cambio;
    } else {
      snprintf(msg, sizeof(msg), "File seek failed: %s ", strerror(errno));
      UI_Error(ctx, __FILE__, __LINE__, "%s", msg);
      fflush(stdout);
      free(cambio);
      return;
    }
  } else {
    snprintf(msg, sizeof(msg), "Read from file failed: %s ", strerror(errno));
    UI_Error(ctx, __FILE__, __LINE__, "%s", msg);
    fflush(stdout);
    free(cambio);
    return;
  }
  return;
}

static void move_right(ViewContext *ctx, WINDOW *win) {
  fstat(fd, &fdstat);
  cursor_pos_x++;
  if (fdstat.st_size >
      ((cursor_pos_y + current_line - 1) * ctx->viewer.bytes + CURSOR_POSX)) {
    cursor_pos_x--;
    if (cursor_pos_x < CANTX(ctx->viewer.bytes) - 1) {
      cursor_pos_x += 1;
      wmove(win, cursor_pos_y, CURSOR_POS_X);
    } else {
      if (fdstat.st_size >= ((current_line + cursor_pos_y) * ctx->viewer.bytes)) {
        if (cursor_pos_y < ctx->viewer.wlines - 1) {
          cursor_pos_y++;
          cursor_pos_x = 0;
          wmove(win, cursor_pos_y, CURSOR_POS_X);
        } else {
          current_line++;
          scroll_down(ctx, win);
          cursor_pos_x = 0;
          wmove(win, cursor_pos_y, CURSOR_POS_X);
        }
      } else {
      }
    }
  } else {
    cursor_pos_x--;
  }
  return;
}

static void DoResize(ViewContext *ctx, char *file_path) {
  int old_cursor_pos_x = cursor_pos_x;
  int old_cursor_pos_y = cursor_pos_y;
  int old_WLINES = ctx->viewer.wlines;
  int old_WCOLS = ctx->viewer.wcols;
  int offset = (cursor_pos_y + current_line - 1) * ctx->viewer.bytes + CURSOR_POSX;
  int new_cols;
  int new_lines;

  SetupViewWindow(ctx, file_path);

  new_cols = offset % ctx->viewer.bytes;
  new_lines = offset / ctx->viewer.bytes;

  if (ctx->viewer.wlines < old_WLINES) {
    if (old_cursor_pos_y >= ctx->viewer.wlines) /* view is getting smaller (heigh) */
    {
      /* current position is (now) in invisible line; "scoll down" */
      int n = old_WLINES - ctx->viewer.wlines;
      current_line += n;
      cursor_pos_y -= n;
      if (cursor_pos_y < 0) {
        current_line += (cursor_pos_y * -1);
        cursor_pos_y = 0;
      }
    }
  }

  if (ctx->viewer.wcols != old_WCOLS) {
    /* reposition x */
    cursor_pos_x = (ctx->viewer.inhex) ? (new_cols * 2 + (old_cursor_pos_x % 2)) : new_cols;

    /* reposition y */
    while ((cursor_pos_y + current_line - 1) <
           new_lines) /* maybe view is smaller (less columns) */
    {
      if (cursor_pos_y < (ctx->viewer.wlines - 1))
        cursor_pos_y++;
      else
        current_line++;
    }

    while ((cursor_pos_y + current_line - 1) >
           new_lines) /* maybe view is larger (more columns) */
    {
      if (cursor_pos_y > 0)
        cursor_pos_y--;
      else
        current_line--;
    }
  }

  Change2Edit(ctx, file_path);
  update_all_lines(ctx, ctx->viewer.view, ctx->viewer.wlines - 1);
  wmove(ctx->viewer.view, cursor_pos_y, CURSOR_POS_X);
  wnoutrefresh(ctx->viewer.view);
  doupdate();
  ctx->viewer.resize_done = TRUE;
}

static void hex_edit(ViewContext *ctx, char *file_path) {
  int ch;
  char msg[50];

  BOOL QUIT = FALSE;

  cursor_pos_x = cursor_pos_y = 0;
  fd2 = fd;
  fd = open(file_path, O_RDWR);
  if (fd == -1) {
    snprintf(msg, sizeof(msg), "File open failed: %s ", strerror(errno));
    UI_Error(ctx, __FILE__, __LINE__, "%s", msg);
    touchwin(ctx->viewer.view);
    fd = fd2;
    return;
  }
  ctx->viewer.inedit = TRUE;
  update_all_lines(ctx, ctx->viewer.view, ctx->viewer.wlines - 1);
  leaveok(ctx->viewer.view, FALSE);
  curs_set(1);
  wmove(ctx->viewer.view, cursor_pos_y, CURSOR_POS_X);
  wnoutrefresh(ctx->viewer.view);

  while (!QUIT) {

    doupdate();
    ch = (ctx->resize_request) ? -1 : Getch(ctx);

    ch = NormalizeViKey(ctx, ch);
    if (ctx->resize_request) {
      DoResize(ctx, file_path);
      ctx->resize_request = FALSE;
      continue;
    }

    switch (ch) {

#ifdef KEY_RESIZE

    case KEY_RESIZE:
      ctx->resize_request = TRUE;
      break;
#endif
    case ESC:
      QUIT = TRUE;
      break;

    case KEY_DOWN: /*ScrollDown();*/
      fstat(fd, &fdstat);
      if (fdstat.st_size >
          ((cursor_pos_y + current_line - 1) * ctx->viewer.bytes + CURSOR_POSX)) {

        if (fdstat.st_size >
            ((cursor_pos_y + current_line - 1 + 1) * ctx->viewer.bytes + CURSOR_POSX)) {

          if (cursor_pos_y < ctx->viewer.wlines - 1) {
            wmove(ctx->viewer.view, ++cursor_pos_y, CURSOR_POS_X);
            wnoutrefresh(ctx->viewer.view);
          } else {
            ++current_line;
            scroll_down(ctx, ctx->viewer.view);
            wmove(ctx->viewer.view, cursor_pos_y, CURSOR_POS_X);
            wnoutrefresh(ctx->viewer.view);
          }
        } else {
          /* special case: last line */

          if (fdstat.st_size >
              ((cursor_pos_y + current_line - 1 + 1) * ctx->viewer.bytes)) {

            for (cursor_pos_x = 0; (CURSOR_POSX + 1) < (fdstat.st_size % ctx->viewer.bytes);
                 cursor_pos_x++)
              ;

            if (cursor_pos_y < ctx->viewer.wlines - 1) {
              wmove(ctx->viewer.view, ++cursor_pos_y, CURSOR_POS_X);
              wnoutrefresh(ctx->viewer.view);
            } else {
              ++current_line;
              scroll_down(ctx, ctx->viewer.view);
              wmove(ctx->viewer.view, cursor_pos_y, CURSOR_POS_X);
              wnoutrefresh(ctx->viewer.view);
            }
          }
        }
      }
      break;
    case KEY_UP: /*ScroollUp();*/
      if (cursor_pos_y > 0) {
        wmove(ctx->viewer.view, --cursor_pos_y, CURSOR_POS_X);
        wnoutrefresh(ctx->viewer.view);
      } else if (current_line > 1) {
        current_line--;
        scroll_up(ctx, ctx->viewer.view);
        wmove(ctx->viewer.view, cursor_pos_y, CURSOR_POS_X);
        wnoutrefresh(ctx->viewer.view);
      }
      break;
    case KEY_LEFT: /* move 1 char left */
      if (cursor_pos_x > 0) {
        cursor_pos_x -= 1;
        wmove(ctx->viewer.view, cursor_pos_y, CURSOR_POS_X);
      } else if (cursor_pos_y > 0) {
        /*cursor_pos_x=ultimo_caracter;*/
        cursor_pos_x = CANTX(ctx->viewer.bytes) - 1;
        wmove(ctx->viewer.view, --cursor_pos_y, CURSOR_POS_X);
      } else if (current_line > 1) {
        current_line--;
        scroll_up(ctx, ctx->viewer.view);
        cursor_pos_x = CANTX(ctx->viewer.bytes) - 1;
        wmove(ctx->viewer.view, cursor_pos_y, CURSOR_POS_X);
      }
      wnoutrefresh(ctx->viewer.view);
      break;
    case KEY_PPAGE: /*ScrollPageDown();*/
      if (current_line > ctx->viewer.wlines)
        current_line -= ctx->viewer.wlines;
      else if (current_line > 1)
        current_line = 1;
      update_all_lines(ctx, ctx->viewer.view, ctx->viewer.wlines);
      wmove(ctx->viewer.view, cursor_pos_y, CURSOR_POS_X);
      wnoutrefresh(ctx->viewer.view);
      break;
    case KEY_RIGHT:
      move_right(ctx, ctx->viewer.view);
      wnoutrefresh(ctx->viewer.view);
      break;
    case KEY_NPAGE: /*ScroollPageUp();*/
      fstat(fd, &fdstat);
      if (fdstat.st_size >
          ((current_line - 1 + ctx->viewer.wlines + cursor_pos_y) * ctx->viewer.bytes) + CURSOR_POSX) {
        current_line += ctx->viewer.wlines;
      } else {
        int n;
        n = fdstat.st_size / ctx->viewer.bytes; /* numer of full lines */
        if (fdstat.st_size % ctx->viewer.bytes) {
          n++; /* plus 1 not fully used line */
        }
        if (current_line != n) {
          current_line = n;
          cursor_pos_y = 0;
          for (cursor_pos_x = 0; (CURSOR_POSX + 1) < (fdstat.st_size % ctx->viewer.bytes);
               cursor_pos_x++)
            ;
        }
      }
      update_all_lines(ctx, ctx->viewer.view, ctx->viewer.wlines);
      wmove(ctx->viewer.view, cursor_pos_y, CURSOR_POS_X);
      wnoutrefresh(ctx->viewer.view);
      break;
    case KEY_HOME:
      if (CURSOR_POSX > 0) {
        cursor_pos_x = 0;
        wmove(ctx->viewer.view, cursor_pos_y, CURSOR_POS_X);
      }
      wnoutrefresh(ctx->viewer.view);
      break;
    case KEY_END:
      fstat(fd, &fdstat);
      if (((cursor_pos_y + current_line) * ctx->viewer.bytes) > fdstat.st_size) {
        cursor_pos_x = CANTX(fdstat.st_size % ctx->viewer.bytes) - 1;
      } else {
        cursor_pos_x = CANTX(ctx->viewer.bytes) - 1;
      }
      wmove(ctx->viewer.view, cursor_pos_y, CURSOR_POS_X);
      wnoutrefresh(ctx->viewer.view);
      break;
    case '\t':
      /* move cursor to the the other part of the window*/
      if (ctx->viewer.inhex) {
        ctx->viewer.inhex = FALSE;
        cursor_pos_x = cursor_pos_x / 2;
      } else {
        ctx->viewer.inhex = TRUE;
        cursor_pos_x = cursor_pos_x * 2;
      }
      wmove(ctx->viewer.view, cursor_pos_y, CURSOR_POS_X);
      wnoutrefresh(ctx->viewer.view);
      break;
    case 'L' & 0x1f:
      clearok(stdscr, TRUE);
      RefreshWindow(stdscr);
      break;

    case 'q':
    case 'Q':
      if (ctx->viewer.inhex) {
        QUIT = TRUE;
        break;
      }
      /* fallthrough */
    default:
      if (ch != ERR) {
        change_char(ctx, ch);
        wmove(ctx->viewer.view, cursor_pos_y, 0);
        update_line(ctx, ctx->viewer.view, current_line + cursor_pos_y);
        move_right(ctx, ctx->viewer.view);
      }
      wmove(ctx->viewer.view, cursor_pos_y, CURSOR_POS_X);
      wnoutrefresh(ctx->viewer.view);
      break;
    }
  }
  curs_set(0);
  close(fd);
  fd = fd2;
  ctx->viewer.inedit = FALSE;
  return;
}

int InternalView(ViewContext *ctx, char *file_path) {
  int ch;
  BOOL QUIT = FALSE;
  int result = VIEW_EXIT;

  ctx->viewer.hexoffset =
      (!strcmp((GetProfileValue)(ctx, "HEXEDITOFFSET"), "HEX")) ? TRUE : FALSE;

  if (stat(file_path, &fdstat) != 0)
    return -1;
  if (!(S_ISREG(fdstat.st_mode)) || S_ISBLK(fdstat.st_mode))
    return -1;
  fd = open(file_path, O_RDONLY);
  if (fd == -1)
    return -1;
  SetupViewWindow(ctx, file_path);
  current_line = 1;
  update_all_lines(ctx, ctx->viewer.view, ctx->viewer.wlines - 1);

  while (!QUIT) {

    ch = (ctx->resize_request) ? -1 : WGetch(ctx, ctx->viewer.view);

    ch = NormalizeViKey(ctx, ch);

    if (ctx->resize_request) {
      DoResize(ctx, file_path);
      ctx->resize_request = FALSE;
      continue;
    }

    switch (ch) {

#ifdef KEY_RESIZE
    case KEY_RESIZE:
      ctx->resize_request = TRUE;
      break;
#endif

    case ESC:
    case 'q':
    case 'Q':
      QUIT = TRUE;
      result = VIEW_EXIT;
      break;
    case ' ':
    case 'n':
      QUIT = TRUE;
      result = VIEW_NEXT;
      break;
    case 'p':
      QUIT = TRUE;
      result = VIEW_PREV;
      break;
    case 'e':
    case 'E':
      Change2Edit(ctx, file_path);
      hex_edit(ctx, file_path);
      update_all_lines(ctx, ctx->viewer.view, ctx->viewer.wlines - 1);
      Change2View(ctx, file_path);
      break;
    case KEY_DOWN: /*ScrollDown();*/
      fstat(fd, &fdstat);
      if (fdstat.st_size > (current_line * ctx->viewer.bytes)) {
        current_line++;
        scroll_down(ctx, ctx->viewer.view);
      }
      break;
    case KEY_UP: /*ScroollUp();*/
      if (current_line > 1) {
        current_line--;
        scroll_up(ctx, ctx->viewer.view);
      }
      break;
    case KEY_LEFT:
    case KEY_PPAGE: /*ScrollPageDown();*/
      if (current_line > ctx->viewer.wlines)
        current_line -= ctx->viewer.wlines;
      else if (current_line > 1)
        current_line = 1;
      update_all_lines(ctx, ctx->viewer.view, ctx->viewer.wlines);
      break;
    case KEY_RIGHT:
    case KEY_NPAGE: /*ScroollPageUp();*/
      fstat(fd, &fdstat);
      if (fdstat.st_size > ((current_line - 1 + ctx->viewer.wlines) * ctx->viewer.bytes)) {
        current_line += ctx->viewer.wlines;
      } else {
        int n;
        n = fdstat.st_size / ctx->viewer.bytes; /* numer of full lines */
        if (fdstat.st_size % ctx->viewer.bytes) {
          n++; /* plus 1 not fully used line */
        }
        current_line = n;
      }
      update_all_lines(ctx, ctx->viewer.view, ctx->viewer.wlines);
      break;
    case KEY_HOME: /*ScrollHome();*/
      if (current_line > 1) {
        current_line = 1;
        update_all_lines(ctx, ctx->viewer.view, ctx->viewer.wlines - 1);
      }
      break;
    case KEY_END: /*ScrollEnd();*/
      fstat(fd, &fdstat);
      if (fdstat.st_size >= ctx->viewer.bytes * 2) {
        current_line = (fdstat.st_size - ctx->viewer.bytes) / ctx->viewer.bytes;
      }
      update_all_lines(ctx, ctx->viewer.view, ctx->viewer.wlines);
      break;
    case 'L' & 0x1f:
      clearok(stdscr, TRUE);
      RefreshWindow(stdscr);
      break;
    default:
      break;
    }
  }
  delwin(ctx->viewer.view);
  ctx->viewer.view = NULL;
  delwin(ctx->viewer.border);
  ctx->viewer.border = NULL;
  touchwin(stdscr);
  wnoutrefresh(stdscr);
  close(fd);
  ctx->resize_request = ctx->viewer.resize_done;
  ctx->viewer.resize_done = FALSE;
  return result;
}
