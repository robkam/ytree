/***************************************************************************
 *
 * src/ui/prompt.c
 * Prompt & Input Manager
 *
 * Implements a managed window for user input, replacing the legacy
 * direct-to-screen InputStringEx.
 *
 ***************************************************************************/

#include "ytree.h"
#include <ctype.h> /* For isalnum */

#define PROMPT_WIN_WIDTH 60
#define PROMPT_WIN_HEIGHT 5

/* Helper to get visible length of string */
/* Using extern from input.c logic (now moved here or duplicated) */
extern int StrVisualLength(const char *str);
extern char *StrLeft(const char *str, size_t count);
extern int VisualPositionToBytePosition(const char *str, int visual_pos);
extern int WGetch(WINDOW *win);
extern int ViKey( int ch );


/*
 * UI_ReadString
 * Creates a centered window, displays a prompt, and reads user input into buffer.
 *
 * prompt: The message to display.
 * buffer: The buffer to store input (must be initialized with default value).
 * max_len: Maximum length of the string in the buffer.
 * history_type: ID for history management (HST_...).
 *
 * Returns: The terminating key (CR or ESC).
 */
int UI_ReadString(const char *prompt, char *buffer, int max_len, int history_type)
{
    WINDOW *win;
    int win_y, win_x;
    int input_y = 2; /* Line inside window for input */
    int input_x = 2; /* Column inside window for input */
    int input_width;
    int ch;
    int p = 0; /* Visual cursor position */
    int scroll_offset = 0;
    static BOOL insert_flag = TRUE;

    /* Calculate window position */
    win_y = (LINES - PROMPT_WIN_HEIGHT) / 2;
    win_x = (COLS - PROMPT_WIN_WIDTH) / 2;
    input_width = PROMPT_WIN_WIDTH - 4; /* Padding */

    /* Ensure buffer is valid */
    if (buffer == NULL) return ESC;

    /* Initial cursor position at end of string */
    p = StrVisualLength(buffer);

    /* Setup Window */
    win = newwin(PROMPT_WIN_HEIGHT, PROMPT_WIN_WIDTH, win_y, win_x);
    if (win == NULL) return ESC;

    keypad(win, TRUE);
    WbkgdSet(win, COLOR_PAIR(CPAIR_MENU));

    while (1) {
        werase(win);
        box(win, 0, 0);

        /* Draw Prompt */
        mvwprintw(win, 1, 2, "%s", prompt);

        /* Draw Input Field */
        /* Handle Scrolling */
        if (p < scroll_offset) {
            scroll_offset = p;
        } else if (p >= scroll_offset + input_width) {
            scroll_offset = p - input_width + 1;
        }

        /* Extract visible portion */
        int start_byte = VisualPositionToBytePosition(buffer, scroll_offset);
        char *display_str = StrLeft(&buffer[start_byte], input_width);

        mvwaddstr(win, input_y, input_x, display_str);

        /* Fill remaining space with underlines to indicate field */
        int len_drawn = StrVisualLength(display_str);
        int i;
        for (i = len_drawn; i < input_width; i++) {
            waddch(win, '_');
        }

        free(display_str);

        /* Position Cursor */
        wmove(win, input_y, input_x + (p - scroll_offset));

        wrefresh(win);

        curs_set(1); /* Ensure cursor is visible */
        ch = WGetch(win);

#ifdef VI_KEYS
        ch = ViKey(ch);
#endif

        if (ch == ESC) {
            break;
        }
        if (ch == '\n' || ch == '\r') {
            ch = CR; /* Standardize return */
            break;
        }

        if (resize_request) {
            resize_request = FALSE;
            /* Recalculate center */
            win_y = (LINES - PROMPT_WIN_HEIGHT) / 2;
            win_x = (COLS - PROMPT_WIN_WIDTH) / 2;
            mvwin(win, win_y, win_x);
            /* Refresh background */
            RefreshGlobalView(GetSelectedDirEntry(CurrentVolume));
            touchwin(win);
            continue;
        }

        switch (ch) {
            case KEY_LEFT:
            case KEY_BTAB:
                if (p > 0) p--;
                break;

            case KEY_RIGHT:
                if (p < StrVisualLength(buffer)) p++;
                break;

            case 'A' & 0x1F: /* Ctrl-A */
            case KEY_HOME:
                p = 0;
                break;

            case 'E' & 0x1F: /* Ctrl-E */
            case KEY_END:
                p = StrVisualLength(buffer);
                break;

            case 'K' & 0x1F: /* Ctrl-K: Delete to end */
                {
                    int byte_pos = VisualPositionToBytePosition(buffer, p);
                    buffer[byte_pos] = '\0';
                }
                break;

            case 'U' & 0x1F: /* Ctrl-U: Delete to start */
                if (p > 0) {
                    int byte_pos = VisualPositionToBytePosition(buffer, p);
                    memmove(buffer, buffer + byte_pos, strlen(buffer) - byte_pos + 1);
                    p = 0;
                }
                break;

            case 'W' & 0x1F: /* Ctrl-W: Delete word left */
                if (p > 0) {
                    int byte_p = VisualPositionToBytePosition(buffer, p);
                    int end_of_word = byte_p;
                    int start_of_word;

                    /* Skip backwards over any non-alphanumeric characters */
                    while (end_of_word > 0 && !isalnum((unsigned char)buffer[end_of_word - 1])) {
                        end_of_word--;
                    }
                    /* Find the start of the word */
                    start_of_word = end_of_word;
                    while (start_of_word > 0 && isalnum((unsigned char)buffer[start_of_word - 1])) {
                        start_of_word--;
                    }

                    if (start_of_word < byte_p) {
                        memmove(buffer + start_of_word, buffer + byte_p, strlen(buffer + byte_p) + 1);
                        /* Recalculate p based on new string prefix */
                        char c = buffer[start_of_word];
                        buffer[start_of_word] = '\0';
                        p = StrVisualLength(buffer);
                        buffer[start_of_word] = c;
                    }
                }
                break;

            case 'H' & 0x1F: /* Ctrl-H, Backspace, 0x08 */
            case KEY_BACKSPACE:
            case 127:        /* ASCII DEL sometimes maps to Backspace */
                if (p > 0) {
                    int curr_byte = VisualPositionToBytePosition(buffer, p);
                    int prev_byte = VisualPositionToBytePosition(buffer, p - 1);
                    memmove(buffer + prev_byte, buffer + curr_byte, strlen(buffer) - curr_byte + 1);
                    p--;
                }
                break;

            case 'D' & 0x1F: /* Ctrl-D */
            case KEY_DC:     /* Delete Key */
                if (p < StrVisualLength(buffer)) {
                    int curr_byte = VisualPositionToBytePosition(buffer, p);
                    int next_byte = VisualPositionToBytePosition(buffer, p + 1);
                    memmove(buffer + curr_byte, buffer + next_byte, strlen(buffer) - next_byte + 1);
                }
                break;

            case KEY_IC:
                insert_flag = !insert_flag;
                break;

            case KEY_UP:
                {
                    /* GetHistory returns a pointer to internal history data.
                       Do NOT free it. */
                    char *h = GetHistory(history_type);
                    if (h) {
                        strncpy(buffer, h, max_len - 1);
                        buffer[max_len - 1] = '\0';
                        p = StrVisualLength(buffer);
                    }
                }
                break;

            case '\t':
                {
                    /* GetMatches allocates a new string. MUST free it. */
                    char *match = GetMatches(buffer);
                    if (match) {
                        strncpy(buffer, match, max_len - 1);
                        buffer[max_len - 1] = '\0';
                        p = StrVisualLength(buffer);
                        free(match);
                    }
                }
                break;

#ifdef KEY_F
            case KEY_F(2):
#endif
            case 'F' & 0x1f:
                if (history_type == HST_LOGIN || history_type == HST_PATH) {
                    char path[PATH_LENGTH + 1];
                    /* F2 Directory Selection */
                    /* Note: KeyF2Get handles saving/restoring the screen context internally */
                    if (KeyF2Get(CurrentVolume->vol_stats.tree,
                                 CurrentVolume->vol_stats.disp_begin_pos,
                                 CurrentVolume->vol_stats.cursor_pos,
                                 path) == 0)
                    {
                        if (*path) {
                            strncpy(buffer, path, max_len - 1);
                            buffer[max_len - 1] = '\0';
                            p = StrVisualLength(buffer);
                            /* Force refresh of global view before we redraw our window,
                               just in case KeyF2Get left things dirty */
                            RefreshGlobalView(GetSelectedDirEntry(CurrentVolume));
                            touchwin(win);
                        }
                    } else {
                         /* Cancelled or error, just ensure background is clean */
                         RefreshGlobalView(GetSelectedDirEntry(CurrentVolume));
                         touchwin(win);
                    }
                }
                break;

            default:
                if (ch >= ' ' && ch <= '~') {
                    if (strlen(buffer) < (size_t)(max_len - 1)) {
                        int byte_pos = VisualPositionToBytePosition(buffer, p);
                        if (insert_flag) {
                            memmove(buffer + byte_pos + 1, buffer + byte_pos, strlen(buffer) - byte_pos + 1);
                        }
                        buffer[byte_pos] = ch;
                        if (!insert_flag && buffer[byte_pos+1] == '\0') {
                             /* Overwrite mode at end extends string */
                             buffer[byte_pos+1] = '\0';
                        }
                        p++;
                    } else {
                        beep();
                    }
                }
                break;
        }
    }

    /* Cleanup */
    curs_set(0);
    delwin(win);

    /* Update History on success */
    if (ch == CR && buffer[0] != '\0') {
        InsHistory(buffer, history_type);
    }

    /* Global Refresh to restore what was behind the window */
    RefreshGlobalView(GetSelectedDirEntry(CurrentVolume));

    return ch;
}