/***************************************************************************
 *
 * src/ui/stats.c
 * Statistics Module - Modernized Boxed Layout
 * Refactored to share attribute display logic between files and directories.
 * Responsive layout update.
 *
 ***************************************************************************/

#include "ytree.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <curses.h>

/* Geometry Definitions */
#define STAT_W   (layout.stats_width)
#define STAT_X   (COLS - STAT_W)
#define L_BORDER (STAT_X - 1)
#define R_BORDER (COLS - 1)
#define INNER_W  (STAT_W - 2)

/* Y-Coordinates (Dynamic) */
#define Y_TOP        1

static int y_filter_val;
static int y_vol_sep;
static int y_vol_info;
static int y_vstat_sep;
static int y_vstat_val;
static int y_dstat_sep;
static int y_dstat_val;
static int y_attr_sep;
static int y_attr_val;
static int y_bot;

/* Prototypes */
static void RecalcLayout(void);
static void FormatNumber(char *buf, size_t size, LONGLONG val);
static void FormatShortSize(char *buf, size_t size, LONGLONG val);
static void SetColor(void);
static void DrawBoxFrame(void);
static void DrawSeparator(int y, const char *title);
static void PrintStatRow(int y, const char *label, LONGLONG count, LONGLONG bytes);
static void DrawAttributes(const char *name, struct stat *s, FileEntry *fe);
static void RecalcDir(DirEntry *de, Statistic *s);

/* ************************************************************************* */
/*                           LOGIC FUNCTIONS                                 */
/* ************************************************************************* */

static void RecalcLayout(void) {
    y_bot = layout.bottom_border_y;

    if (LINES < 26) {
        /* Compact Mode for small terminals (e.g. 24 lines) */
        y_filter_val = 2;
        y_vol_sep    = 0; /* Hidden */
        y_vol_info   = 3; /* 3, 4, 5 */
        y_vstat_sep  = 0; /* Hidden */
        y_vstat_val  = 6; /* 6, 7, 8 */
        y_dstat_sep  = 0; /* Hidden */
        y_dstat_val  = 9; /* 9, 10, 11, 12 */
        y_attr_sep   = 0; /* Hidden */
        y_attr_val   = 13;/* 13, 14, 15, 16, 17 */
        /* Total used: 2 to 17. 18 is border. Fits in 20 (LINES=24 -> y_bot=20) */
    } else {
        /* Standard Spacious Mode */
        y_filter_val = 2;
        y_vol_sep    = 3;
        y_vol_info   = 4;
        y_vstat_sep  = 7;
        y_vstat_val  = 8;
        y_dstat_sep  = 11;
        y_dstat_val  = 12;
        y_attr_sep   = 16;
        y_attr_val   = 17;
    }
}

static void RecalcDir(DirEntry *d, Statistic *s) {
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
            RecalcDir(sub, s);
            s->disk_total_directories++;
        }
        sub = sub->next;
    }

    s->disk_total_files += d->total_files;
    s->disk_total_bytes += d->total_bytes;
    s->disk_matching_files += d->matching_files;
    s->disk_matching_bytes += d->matching_bytes;
    s->disk_tagged_files += d->tagged_files;
    s->disk_tagged_bytes += d->tagged_bytes;
}

void RecalculateSysStats(Statistic *s) {
    s->disk_total_files = 0;
    s->disk_total_bytes = 0;
    s->disk_matching_files = 0;
    s->disk_matching_bytes = 0;
    s->disk_tagged_files = 0;
    s->disk_tagged_bytes = 0;
    s->disk_total_directories = 0;

    if (s->tree) {
        s->disk_total_directories++;
        RecalcDir(s->tree, s);
    }
}

/* ************************************************************************* */
/*                           DISPLAY HELPERS                                 */
/* ************************************************************************* */

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
    /* int hline_len = R_BORDER - L_BORDER - 1; */ /* Inner width for lines - Unused var removed */
    int sep_y = layout.dir_win_y + layout.dir_win_height;

    attrset(COLOR_PAIR(CPAIR_WINDIR));

    /* --- Top Border with embedded " FILTER " --- */
    /* The ACS_ULCORNER is replaced by ACS_TTEE at the junction with the main window's top border. */
    /* mvaddch(Y_TOP, L_BORDER, ACS_ULCORNER); */ /* Removed as per instruction */

    /* Calculate embedded title position */
    {
        const char *title = " FILTER ";
        int hline_len = R_BORDER - L_BORDER - 1;
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
    mvaddch(y_bot, L_BORDER, ACS_LLCORNER);
    mvwhline(stdscr, y_bot, L_BORDER + 1, ACS_HLINE, R_BORDER - L_BORDER - 1);
    mvaddch(y_bot, R_BORDER, ACS_LRCORNER);

    /* --- Vertical Lines --- */
    for (y = Y_TOP + 1; y < y_bot; y++) {
        mvaddch(y, R_BORDER, ACS_VLINE);
        mvaddch(y, L_BORDER, ACS_VLINE);
    }

    /* --- Junctions --- */
    mvaddch(Y_TOP, L_BORDER, ACS_TTEE); /* Connects to Path bar in main win */

    /* Handle File Window artifact */
    if (file_window == big_file_window) {
        mvaddch(sep_y, L_BORDER, ACS_VLINE | A_BOLD | COLOR_PAIR(CPAIR_WINDIR));
    } else {
        mvaddch(sep_y, L_BORDER, ACS_RTEE | A_BOLD | COLOR_PAIR(CPAIR_WINDIR));
    }
    mvaddch(y_bot, L_BORDER, ACS_BTEE);

    SetColor();
}

static void DrawSeparator(int y, const char *title) {
    int x;
    int text_len = title ? strlen(title) : 0;
    int total_inner_width = R_BORDER - L_BORDER - 1;

    if (y <= 0) return;

    attrset(COLOR_PAIR(CPAIR_WINDIR));

    if (title && text_len > 0) {
        int left_hline_len;
        /* int right_hline_len; // Removed: Unused */
        int title_content_start_x;
        int pad = 2; /* 1 space each side */

        if (total_inner_width >= text_len + pad) {
            int rem = total_inner_width - (text_len + pad);
            left_hline_len = rem / 2;
            /* right_hline_len = rem - left_hline_len; */
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

    if (y >= y_bot) return;

    FormatNumber(count_buf, sizeof(count_buf), count);
    FormatShortSize(size_buf, sizeof(size_buf), bytes);

    SetColor();
    /* Format: "Tot: 1,831,129 12.5M"
     * Math: 4 (Label) + 1 (Space) + 9 (Count) + 1 (Space) + 6 (Size) = 21 chars.
     * INNER_W is 22. This leaves 1 char padding. Perfect.
     */
    mvprintw(y, STAT_X + 1, "%-4s %9s %6s", label, count_buf, size_buf);
}

static void DrawAttributes(const char *name, struct stat *s, FileEntry *fe) {
    char buf[128];
    char num_buf[32];
    char time_buf[20];
    char temp[256]; /* Increased from 128 to fix truncation warning */

    if (!name || !s) return;

    DrawSeparator(y_attr_sep, "ATTRIBUTES");

    /* Name */
    CutPathname(buf, (char*)name, INNER_W); /* Keep CutPathname for stats box */

    /* Explicitly clear the line to avoid background artifacts */
    mvwhline(stdscr, y_attr_val, STAT_X + 1, ' ', INNER_W);

    if (fe) {
#ifdef COLOR_SUPPORT
        int color = GetFileTypeColor(fe);
        attron(COLOR_PAIR(color) | A_BOLD);
        mvprintw(y_attr_val, STAT_X + 1, "%s", buf); /* Print without padding */
        attroff(COLOR_PAIR(color) | A_BOLD);
#else
        attron(A_BOLD);
        mvprintw(y_attr_val, STAT_X + 1, "%s", buf);
        attroff(A_BOLD);
#endif
    } else {
        attron(A_BOLD);
        mvprintw(y_attr_val, STAT_X + 1, "%s", buf); /* Print without padding */
        attroff(A_BOLD);
    }

    /* Size */
    FormatShortSize(num_buf, sizeof(num_buf), s->st_size);
    snprintf(temp, sizeof(temp), "Size: %s", num_buf);
    mvprintw(y_attr_val + 1, STAT_X + 1, "%-22s", temp);

    /* Attr */
    GetAttributes(s->st_mode, buf);
    snprintf(temp, sizeof(temp), "Attr: %s", buf);
    mvprintw(y_attr_val + 2, STAT_X + 1, "%-22s", temp);

    /* Owner */
    {
        char *owner = GetDisplayPasswdName(s->st_uid);
        char *group = GetDisplayGroupName(s->st_gid);
        char owner_buf[32];
        char grp_buf[32];
        if (!owner) { snprintf(owner_buf, sizeof(owner_buf), "%d", s->st_uid); owner = owner_buf; }
        if (!group) { snprintf(grp_buf, sizeof(grp_buf), "%d", s->st_gid); group = grp_buf; }

        char full_own[64];
        snprintf(full_own, sizeof(full_own), "%s:%s", owner, group);
        CutName(buf, full_own, INNER_W - 6); /* "Own : " is 6 chars */
        mvprintw(y_attr_val + 3, STAT_X + 1, "Own : %-16s", buf);
    }

    /* Mod */
    CTime(s->st_mtime, time_buf);
    mvprintw(y_attr_val + 4, STAT_X + 1, "Mod : %-16s", time_buf);

    refresh();
}

/* ************************************************************************* */
/*                           DISPLAY FUNCTIONS                                 */
/* ************************************************************************* */

void DisplayFullStatsPanel(Statistic *s) {
    DisplayDiskStatistic(s);
}

void DisplayDiskName(Statistic *s)
{
    char buf[128];
    char path_buf[PATH_LENGTH + 1];
    char size_buf[32];
    int total_volumes = HASH_COUNT(VolumeList);
    int current_index = 0;
    struct Volume *vol_iter, *tmp;
    int i = 1;

    if (layout.stats_width == 0) return;

    /* Recalculate layout based on current terminal height */
    RecalcLayout();

    /* 1. Determine Volume Index */
    if (VolumeList) {
        HASH_ITER(hh, VolumeList, vol_iter, tmp) {
            if (&vol_iter->vol_stats == s) {
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
    CutName(buf, s->file_spec, INNER_W);
    attron(A_BOLD);
    /* Center filter text using padding format to clear ghosts */
    {
        int pad = (INNER_W - strlen(buf)) / 2;
        mvprintw(y_filter_val, STAT_X + 1, "%*s%-*s", pad, "", INNER_W - pad, buf);
    }
    attroff(A_BOLD);

    /* 4. Volume Section */
    snprintf(buf, sizeof(buf), "VOLUME %d/%d", current_index, total_volumes);
    DrawSeparator(y_vol_sep, buf);

    /* Path */
    if (mode == ARCHIVE_MODE) strncpy(path_buf, s->login_path, PATH_LENGTH);
    else strncpy(path_buf, s->path, PATH_LENGTH);
    path_buf[PATH_LENGTH] = '\0';

    CutPathname(buf, path_buf, INNER_W); /* Changed to CutPathname */
    mvprintw(y_vol_info, STAT_X + 1, "%-*s", INNER_W, buf); /* Pad to clear */

    /* FS */
    char fs_buf[64];
    if (mode == ARCHIVE_MODE) snprintf(fs_buf, sizeof(fs_buf), "FS: ARCHIVE");
    else snprintf(fs_buf, sizeof(fs_buf), "FS: %s", s->disk_name);
    /* Truncate to fit */
    CutName(buf, fs_buf, INNER_W);
    mvprintw(y_vol_info + 1, STAT_X + 1, "%-*s", INNER_W, buf);

    /* Free */
    if (mode == ARCHIVE_MODE) {
        snprintf(fs_buf, sizeof(fs_buf), "Free: -");
    } else {
        FormatShortSize(size_buf, sizeof(size_buf), s->disk_space);
        snprintf(fs_buf, sizeof(fs_buf), "Free: %s", size_buf);
    }
    mvprintw(y_vol_info + 2, STAT_X + 1, "%-*s", INNER_W, fs_buf);
}

void DisplayAvailBytes(Statistic *s) {
    DisplayDiskStatistic(s);
}

void DisplayFilter(Statistic *s) {
    DisplayDiskStatistic(s);
}

void DisplayDiskStatistic(Statistic *s)
{
    if (layout.stats_width == 0) return;

    DisplayDiskName(s);

    DrawSeparator(y_vstat_sep, "VOLUME STATS");

    PrintStatRow(y_vstat_val,     "Tot:", s->disk_total_files, s->disk_total_bytes);
    PrintStatRow(y_vstat_val + 1, "Mat:", s->disk_matching_files, s->disk_matching_bytes);
    PrintStatRow(y_vstat_val + 2, "Tag:", s->disk_tagged_files, s->disk_tagged_bytes);

    refresh();
}

void DisplayDirStatistic(DirEntry *de, const char *title, Statistic *s)
{
    char buf[128];

    if (layout.stats_width == 0) return;

    if (!de) return;

    /* Use provided title, or fallback to default logic */
    if (title) {
        DrawSeparator(y_dstat_sep, title);
    } else if (de->global_flag) {
        DrawSeparator(y_dstat_sep, "SHOW ALL");
    } else {
        if (mode == ARCHIVE_MODE) {
             DrawSeparator(y_dstat_sep, "ARCHIVE");
        } else {
             DrawSeparator(y_dstat_sep, "CURRENT DIR");
        }
    }

    /* Dir Name */
    CutPathname(buf, de->name, INNER_W); /* Changed to CutPathname */
    attron(A_BOLD);
    mvprintw(y_dstat_val, STAT_X + 1, "%-*s", INNER_W, buf); /* Clear ghosting */
    attroff(A_BOLD);

    if (de->global_flag) {
        /* In Show All mode, display global totals */
        PrintStatRow(y_dstat_val + 1, "Tot:", s->disk_total_files, s->disk_total_bytes);
        PrintStatRow(y_dstat_val + 2, "Mat:", s->disk_matching_files, s->disk_matching_bytes);
    } else {
        /* In Normal mode, display current directory totals */
        PrintStatRow(y_dstat_val + 1, "Tot:", de->total_files, de->total_bytes);
        PrintStatRow(y_dstat_val + 2, "Mat:", de->matching_files, de->matching_bytes);
    }

    /* Tag count always shows global disk total in Show All mode, but we use the disk stats directly if global_flag is set. */
    if (de->global_flag) {
        PrintStatRow(y_dstat_val + 3, "Tag:", s->disk_tagged_files, s->disk_tagged_bytes);
    } else {
        PrintStatRow(y_dstat_val + 3, "Tag:", de->tagged_files, de->tagged_bytes);
    }

    refresh();
}

void DisplayFileParameter(FileEntry *fe)
{
    if (layout.stats_width == 0) return;
    if (fe) {
        DrawAttributes(fe->name, &fe->stat_struct, fe);
    }
}

/* ************************************************************************* */
/*                           COMPATIBILITY WRAPPERS                          */
/* ************************************************************************* */

void DisplayDiskTagged(Statistic *s) {
    if (layout.stats_width == 0) return;
    DisplayDiskStatistic(s);
}

void DisplayDirTagged(DirEntry *de, Statistic *s) {
    if (layout.stats_width == 0) return;
    DisplayDirStatistic(de, NULL, s);
}

void DisplayDirParameter(DirEntry *de) {
    if (layout.stats_width == 0) return;
    if (de) {
        DrawAttributes(de->name, &de->stat_struct, NULL);
    }
}

void DisplayGlobalFileParameter(FileEntry *fe) {
    if (layout.stats_width == 0) return;
    DisplayFileParameter(fe);
}