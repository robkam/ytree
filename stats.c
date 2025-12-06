/***************************************************************************
 *
 * stats.c
 * Statistics Module - Modernized Boxed Layout
 * Refactored to share attribute display logic between files and directories.
 *
 ***************************************************************************/

#include "ytree.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <curses.h>

/* Geometry Definitions */
#define STAT_W   STATS_WIDTH       /* 24 */
#define STAT_X   (COLS - STAT_W)
#define L_BORDER (STAT_X - 1)
#define R_BORDER (COLS - 1)
#define INNER_W  (STAT_W - 2)      /* 22 */

/* Y-Coordinates */
#define Y_TOP        1
#define Y_FILTER_VAL 2
#define Y_VOL_SEP    3
#define Y_VOL_INFO   4  /* Path=4, FS=5, Free=6 */
#define Y_VSTAT_SEP  7
#define Y_VSTAT_VAL  8  /* Tot=8, Mat=9, Tag=10 */
#define Y_DSTAT_SEP  11
#define Y_DSTAT_VAL  12 /* Name=12, Tot=13, Mat=14, Tag=15 */
#define Y_ATTR_SEP   16
#define Y_ATTR_VAL   17 /* Name=17, Size=18, Attr=19, Own=20, Mod=21 */
#define Y_BOT        (LINES - 4)

/* Prototypes */
static void FormatNumber(char *buf, size_t size, LONGLONG val);
static void FormatShortSize(char *buf, size_t size, LONGLONG val);
static void SetColor(void);
static void DrawBoxFrame(void);
static void DrawSeparator(int y, const char *title);
static void PrintStatRow(int y, const char *label, LONGLONG count, LONGLONG bytes);
static void DrawAttributes(const char *name, struct stat *s);
static void RecalcDir(DirEntry *de);

/* ========================================================================= */
/*                           LOGIC FUNCTIONS                                 */
/* ========================================================================= */

static void RecalcDir(DirEntry *d) {
    FileEntry *f;
    DirEntry *sub;

    d->total_files = 0;
    d->total_bytes = 0;
    d->matching_files = 0;
    d->matching_bytes = 0;
    d->tagged_files = 0;
    d->tagged_bytes = 0;

    for (f = d->file; f; f = f->next) {
        if (hide_dot_files && f->name[0] == '.') continue;

        d->total_files++;
        d->total_bytes += f->stat_struct.st_size;

        if (f->matching) {
            d->matching_files++;
            d->matching_bytes += f->stat_struct.st_size;
        }
        if (f->tagged) {
             d->tagged_files++;
             d->tagged_bytes += f->stat_struct.st_size;
        }
    }

    sub = d->sub_tree;
    while (sub) {
        if ( !(hide_dot_files && sub->name[0] == '.') ) {
            RecalcDir(sub);
            statistic.disk_total_directories++;
        }
        sub = sub->next;
    }

    statistic.disk_total_files += d->total_files;
    statistic.disk_total_bytes += d->total_bytes;
    statistic.disk_matching_files += d->matching_files;
    statistic.disk_matching_bytes += d->matching_bytes;
    statistic.disk_tagged_files += d->tagged_files;
    statistic.disk_tagged_bytes += d->tagged_bytes;
}

void RecalculateSysStats(void) {
    statistic.disk_total_files = 0;
    statistic.disk_total_bytes = 0;
    statistic.disk_matching_files = 0;
    statistic.disk_matching_bytes = 0;
    statistic.disk_tagged_files = 0;
    statistic.disk_tagged_bytes = 0;
    statistic.disk_total_directories = 0;

    if (statistic.tree) {
        statistic.disk_total_directories++;
        RecalcDir(statistic.tree);
    }
}

/* ========================================================================= */
/*                           DISPLAY HELPERS                                 */
/* ========================================================================= */

static void FormatNumber(char *buf, size_t size, LONGLONG val) {
    char temp[64];
    int len, i, j, commacount;

    snprintf(temp, sizeof(temp), "%lld", val);
    len = strlen(temp);
    commacount = (len - 1) / 3;

    if (len + commacount >= (int)size) {
        snprintf(buf, size, "%lld", val);
        return;
    }

    j = len + commacount;
    buf[j] = '\0';

    for (i = len - 1; i >= 0; i--) {
        buf[--j] = temp[i];
        if (i > 0 && (len - i) % 3 == 0) {
            buf[--j] = number_seperator;
        }
    }
}

static void FormatShortSize(char *buf, size_t size, LONGLONG val) {
    double d = (double)val;
    const char *units[] = {"B", "K", "M", "G", "T", "P"};
    int i = 0;

    /* Handle negative values gracefully (though they shouldn't happen) */
    if (val < 0) {
        snprintf(buf, size, "Err");
        return;
    }

    while (d >= 999.5 && i < 5) { /* threshold slightly < 1000 to avoid "1000K" -> "1.0M" */
        d /= 1024.0;
        i++;
    }

    if (i == 0) {
        /* Bytes: max "999B" (4 chars) */
        snprintf(buf, size, "%lld%s", val, units[i]);
    } else {
        /* Units: "1.2M", "100G" */
        /* Use %.1f for < 10, %.0f for >= 10 to save space?
           Standard: just ensure it fits.
           "999.9G" is 6 chars. "1000T" is 5 chars. Safe. */
        snprintf(buf, size, "%.1f%s", d, units[i]);
    }
}

static void SetColor(void) {
    attrset(COLOR_PAIR(CPAIR_WINDIR));
}

static void DrawBoxFrame(void) {
    int y;
    int hline_len = R_BORDER - L_BORDER - 1; /* Inner width for lines */

    attrset(COLOR_PAIR(CPAIR_WINDIR));

    /* --- Top Border with embedded " FILTER " --- */
    /* The ACS_ULCORNER is replaced by ACS_TTEE at the junction with the main window's top border. */
    /* mvaddch(Y_TOP, L_BORDER, ACS_ULCORNER); */ /* Removed as per instruction */

    /* Calculate embedded title position */
    {
        const char *title = " FILTER ";
        int t_len = strlen(title);
        int left_len = (hline_len - t_len) / 2;
        int right_len = hline_len - t_len - left_len;
        int x = L_BORDER + 1;

        /* Left HLINE */
        mvwhline(stdscr, Y_TOP, x, ACS_HLINE, left_len);
        x += left_len;

        /* Title */
        attron(A_BOLD);
        mvprintw(Y_TOP, x, "%s", title);
        attroff(A_BOLD);
        x += t_len;

        /* Right HLINE */
        mvwhline(stdscr, Y_TOP, x, ACS_HLINE, right_len);
    }

    mvaddch(Y_TOP, R_BORDER, ACS_URCORNER);


    /* --- Bottom Border --- */
    mvaddch(Y_BOT, L_BORDER, ACS_LLCORNER);
    mvwhline(stdscr, Y_BOT, L_BORDER + 1, ACS_HLINE, hline_len);
    mvaddch(Y_BOT, R_BORDER, ACS_LRCORNER);

    /* --- Vertical Lines --- */
    for (y = Y_TOP + 1; y < Y_BOT; y++) {
        mvaddch(y, R_BORDER, ACS_VLINE);
        mvaddch(y, L_BORDER, ACS_VLINE);
    }

    /* --- Junctions --- */
    mvaddch(Y_TOP, L_BORDER, ACS_TTEE); /* Connects to Path bar in main win */

    /* Handle File Window artifact */
    if (file_window == big_file_window) {
        mvaddch(STATS_MIDDLE_SEPARATOR_Y, L_BORDER, ACS_VLINE | A_BOLD | COLOR_PAIR(CPAIR_WINDIR));
    } else {
        mvaddch(STATS_MIDDLE_SEPARATOR_Y, L_BORDER, ACS_RTEE | A_BOLD | COLOR_PAIR(CPAIR_WINDIR));
    }
    mvaddch(Y_BOT, L_BORDER, ACS_BTEE);

    SetColor();
}

static void DrawSeparator(int y, const char *title) {
    int x;
    int text_len = title ? strlen(title) : 0;
    int total_inner_width = R_BORDER - L_BORDER - 1;

    attrset(COLOR_PAIR(CPAIR_WINDIR));

    if (title && text_len > 0) {
        int left_hline_len;
        int right_hline_len;
        int title_content_start_x;
        int pad = 2; /* 1 space each side */

        if (total_inner_width >= text_len + pad) {
            int rem = total_inner_width - (text_len + pad);
            left_hline_len = rem / 2;
            right_hline_len = rem - left_hline_len;
            title_content_start_x = L_BORDER + 1 + left_hline_len;

            /* Left Line */
            for (x = L_BORDER + 1; x < title_content_start_x; x++) mvaddch(y, x, ACS_HLINE);

            /* Title */
            attron(A_BOLD);
            mvprintw(y, title_content_start_x, " %s ", title);
            attroff(A_BOLD);

            /* Right Line */
            for (x = title_content_start_x + text_len + pad; x < R_BORDER; x++) mvaddch(y, x, ACS_HLINE);
        } else {
            /* Truncate if too long (rare) */
            int eff_len = MINIMUM(text_len, total_inner_width);
            title_content_start_x = L_BORDER + 1;
            char fmt[16];
            snprintf(fmt, sizeof(fmt), "%%.%ds", eff_len);
            attron(A_BOLD);
            mvprintw(y, title_content_start_x, fmt, title);
            attroff(A_BOLD);
            /* Fill rest with line */
            for (x = L_BORDER + 1 + eff_len; x < R_BORDER; x++) mvaddch(y, x, ACS_HLINE);
        }
    } else {
        /* Full line */
        for (x = L_BORDER + 1; x < R_BORDER; x++) mvaddch(y, x, ACS_HLINE);
    }

    /* Junctions - drawn last to ensure visibility */
    mvaddch(y, L_BORDER, ACS_LTEE | A_BOLD | COLOR_PAIR(CPAIR_WINDIR));
    mvaddch(y, R_BORDER, ACS_RTEE | A_BOLD | COLOR_PAIR(CPAIR_WINDIR));

    SetColor();
}

static void PrintStatRow(int y, const char *label, LONGLONG count, LONGLONG bytes) {
    char count_buf[32];
    char size_buf[32];

    if (y >= Y_BOT) return;

    FormatNumber(count_buf, sizeof(count_buf), count);
    FormatShortSize(size_buf, sizeof(size_buf), bytes);

    SetColor();
    /* Format: "Tot: 1,831,129 12.5M"
     * Math: 4 (Label) + 1 (Space) + 9 (Count) + 1 (Space) + 6 (Size) = 21 chars.
     * INNER_W is 22. This leaves 1 char padding. Perfect.
     */
    mvprintw(y, STAT_X + 1, "%-4s %9s %6s", label, count_buf, size_buf);
}

static void DrawAttributes(const char *name, struct stat *s) {
    char buf[128];
    char num_buf[32];
    char time_buf[20];
    char temp[128];

    if (!name || !s) return;

    DrawSeparator(Y_ATTR_SEP, "ATTRIBUTES");

    /* Name */
    CutName(buf, (char*)name, INNER_W);
    attron(A_BOLD);
    mvprintw(Y_ATTR_VAL, STAT_X + 1, "%-22s", buf);
    attroff(A_BOLD);

    /* Size */
    FormatShortSize(num_buf, sizeof(num_buf), s->st_size);
    snprintf(temp, sizeof(temp), "Size: %s", num_buf);
    mvprintw(Y_ATTR_VAL + 1, STAT_X + 1, "%-22s", temp);

    /* Attr */
    GetAttributes(s->st_mode, buf);
    snprintf(temp, sizeof(temp), "Attr: %s", buf);
    mvprintw(Y_ATTR_VAL + 2, STAT_X + 1, "%-22s", temp);

    /* Owner */
    {
        char *owner = GetDisplayPasswdName(s->st_uid);
        char *group = GetDisplayGroupName(s->st_gid);
        char owner_buf[32];
        char grp_buf[32];
        if (!owner) { sprintf(owner_buf, "%d", s->st_uid); owner = owner_buf; }
        if (!group) { sprintf(grp_buf, "%d", s->st_gid); group = grp_buf; }

        char full_own[64];
        snprintf(full_own, sizeof(full_own), "%s:%s", owner, group);
        CutName(buf, full_own, INNER_W - 6); /* "Own : " is 6 chars */
        mvprintw(Y_ATTR_VAL + 3, STAT_X + 1, "Own : %-16s", buf);
    }

    /* Mod */
    CTime(s->st_mtime, time_buf);
    mvprintw(Y_ATTR_VAL + 4, STAT_X + 1, "Mod : %-16s", time_buf);

    refresh();
}

/* ========================================================================= */
/*                           DISPLAY FUNCTIONS                                 */
/* ========================================================================= */

void DisplayFullStatsPanel(void) {
    DisplayDiskStatistic();
}

void DisplayDiskName(void)
{
    char buf[128];
    char path_buf[PATH_LENGTH + 1];
    char size_buf[32];
    int total_volumes = HASH_COUNT(VolumeList);
    int current_index = 0;
    struct Volume *s, *tmp;
    int i = 1;

    /* 1. Determine Volume Index */
    if (VolumeList) {
        HASH_ITER(hh, VolumeList, s, tmp) {
            if (s == CurrentVolume) {
                current_index = i;
                break;
            }
            i++;
        }
    }
    if (current_index == 0 && total_volumes > 0) current_index = 1;

    /* 2. Setup Panel */
    SetColor();
    DrawBoxFrame(); /* Draws Top Border with "FILTER" */

    /* 3. Filter Value */
    CutName(buf, statistic.file_spec, INNER_W);
    attron(A_BOLD);
    /* Center filter text using padding format to clear ghosts */
    {
        int pad = (INNER_W - strlen(buf)) / 2;
        mvprintw(Y_FILTER_VAL, STAT_X + 1, "%*s%-*s", pad, "", INNER_W - pad, buf);
    }
    attroff(A_BOLD);

    /* 4. Volume Section */
    snprintf(buf, sizeof(buf), "VOLUME %d/%d", current_index, total_volumes);
    DrawSeparator(Y_VOL_SEP, buf);

    /* Path */
    if (mode == ARCHIVE_MODE) strncpy(path_buf, statistic.login_path, PATH_LENGTH);
    else strncpy(path_buf, statistic.path, PATH_LENGTH);
    path_buf[PATH_LENGTH] = '\0';

    CutName(buf, path_buf, INNER_W);
    mvprintw(Y_VOL_INFO, STAT_X + 1, "%-22s", buf); /* Pad to clear */

    /* FS */
    char fs_buf[64];
    if (mode == ARCHIVE_MODE) snprintf(fs_buf, sizeof(fs_buf), "FS: ARCHIVE");
    else snprintf(fs_buf, sizeof(fs_buf), "FS: %s", statistic.disk_name);
    /* Truncate to fit */
    CutName(buf, fs_buf, INNER_W);
    mvprintw(Y_VOL_INFO + 1, STAT_X + 1, "%-22s", buf);

    /* Free */
    FormatShortSize(size_buf, sizeof(size_buf), statistic.disk_space);
    snprintf(fs_buf, sizeof(fs_buf), "Free: %s", size_buf);
    mvprintw(Y_VOL_INFO + 2, STAT_X + 1, "%-22s", fs_buf);
}

void DisplayAvailBytes(void) {
    DisplayDiskStatistic();
}

void DisplayFilter(void) {
    DisplayDiskStatistic();
}

void DisplayDiskStatistic(void)
{
    DisplayDiskName();

    DrawSeparator(Y_VSTAT_SEP, "VOLUME STATS");

    PrintStatRow(Y_VSTAT_VAL,     "Tot:", statistic.disk_total_files, statistic.disk_total_bytes);
    PrintStatRow(Y_VSTAT_VAL + 1, "Mat:", statistic.disk_matching_files, statistic.disk_matching_bytes);
    PrintStatRow(Y_VSTAT_VAL + 2, "Tag:",   statistic.disk_tagged_files, statistic.disk_tagged_bytes);

    refresh();
}

void DisplayDirStatistic(DirEntry *de)
{
    char buf[128];

    if (!de) return;

    if (de->global_flag) {
        DrawSeparator(Y_DSTAT_SEP, "SHOW ALL");
    } else {
        DrawSeparator(Y_DSTAT_SEP, "CURRENT DIR");
    }

    /* Dir Name */
    CutName(buf, de->name, INNER_W);
    attron(A_BOLD);
    mvprintw(Y_DSTAT_VAL, STAT_X + 1, "%-22s", buf); /* Clear ghosting */
    attroff(A_BOLD);

    PrintStatRow(Y_DSTAT_VAL + 1, "Tot:", de->total_files, de->total_bytes);
    PrintStatRow(Y_DSTAT_VAL + 2, "Mat:", de->matching_files, de->matching_bytes);
    PrintStatRow(Y_DSTAT_VAL + 3, "Tag:",   de->tagged_files, de->tagged_bytes);

    refresh();
}

void DisplayFileParameter(FileEntry *fe)
{
    if (fe) {
        DrawAttributes(fe->name, &fe->stat_struct);
    }
}

/* ========================================================================= */
/*                           COMPATIBILITY WRAPPERS                          */
/* ========================================================================= */

void DisplayDiskTagged(void) {
    DisplayDiskStatistic();
}

void DisplayDirTagged(DirEntry *de) {
    DisplayDirStatistic(de);
}

void DisplayDirParameter(DirEntry *de) {
    if (de) {
        DrawAttributes(de->name, &de->stat_struct);
    }
}

void DisplayGlobalFileParameter(FileEntry *fe) {
    DisplayFileParameter(fe);
}