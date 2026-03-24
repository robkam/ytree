/***************************************************************************
 *
 * src/ui/stats.c
 * Statistics Module - Modernized Boxed Layout
 * Refactored to share attribute display logic between files and directories.
 * Responsive layout update.
 *
 ***************************************************************************/

#include "ytree.h"

#include <curses.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Geometry Definitions */
#define STAT_W (ctx->layout.stats_width)
#define STAT_X (COLS - STAT_W)
#define L_BORDER (STAT_X - 1)
#define R_BORDER (COLS - 1)
#define INNER_W (STAT_W - 2)

/* Y-Coordinates (Dynamic) */
#define Y_TOP 1

/* Prototypes */
static void RecalcLayout(ViewContext *ctx);
static void FormatNumber(const ViewContext *ctx, char *buf, size_t size,
                         long long val);
static void FormatShortSize(char *buf, size_t size, long long val);
static void SetColor(ViewContext *ctx);
static void DrawBoxFrame(ViewContext *ctx);
static void DrawSeparator(ViewContext *ctx, int y, const char *title);
static void PrintStatRow(ViewContext *ctx, int y, const char *label,
                         long long count, long long bytes);
static void DrawAttributes(ViewContext *ctx, const char *name,
                           const struct stat *s, const FileEntry *fe);
static void RecalcDir(ViewContext *ctx, DirEntry *d, Statistic *s);

/* ************************************************************************* */
/*                           LOGIC FUNCTIONS                                 */
/* ************************************************************************* */

static void RecalcLayout(ViewContext *ctx) {
  if (LINES < 26) {
    /* Compact Mode for small terminals (e.g. 24 lines) */
    ctx->layout.stats_y_filter_val = 2;
    ctx->layout.stats_y_vol_sep = 0;   /* Hidden */
    ctx->layout.stats_y_vol_info = 3;  /* 3, 4, 5 */
    ctx->layout.stats_y_vstat_sep = 0; /* Hidden */
    ctx->layout.stats_y_vstat_val = 6; /* 6, 7, 8 */
    ctx->layout.stats_y_dstat_sep = 0; /* Hidden */
    ctx->layout.stats_y_dstat_val = 9; /* 9, 10, 11, 12 */
    ctx->layout.stats_y_attr_sep = 0;  /* Hidden */
    ctx->layout.stats_y_attr_val = 13; /* 13, 14, 15, 16, 17 */
    /* Total used: 2 to 17. 18 is border. Fits in 20 (LINES=24 ->
     * ctx->layout.bottom_border_y=20) */
  } else {
    /* Standard Spacious Mode */
    ctx->layout.stats_y_filter_val = 2;
    ctx->layout.stats_y_vol_sep = 3;
    ctx->layout.stats_y_vol_info = 4;
    ctx->layout.stats_y_vstat_sep = 7;
    ctx->layout.stats_y_vstat_val = 8;
    ctx->layout.stats_y_dstat_sep = 11;
    ctx->layout.stats_y_dstat_val = 12;
    ctx->layout.stats_y_attr_sep = 16;
    ctx->layout.stats_y_attr_val = 17;
  }
}

static void RecalcDir(ViewContext *ctx, DirEntry *d, Statistic *s) {
  FileEntry *f;
  DirEntry *sub;

  /* Apply current filter to this directory */
  ApplyFilter(d, s);

  d->total_files = 0;
  d->total_bytes = 0;
  /* matching_files/bytes already updated by ApplyFilter, but we sum them
   * globally below */

  for (f = d->file; f; f = f->next) {
    if (ctx->hide_dot_files && f->name[0] == '.')
      continue;

    d->total_files++;
    d->total_bytes += f->stat_struct.st_size;

    if (f->tagged) {
      d->tagged_files++;
      d->tagged_bytes += f->stat_struct.st_size;
    }
  }

  sub = d->sub_tree;
  while (sub) {
    RecalcDir(ctx, sub, s);
    s->disk_total_directories++;
    sub = sub->next;
  }

  s->disk_total_files += d->total_files;
  s->disk_total_bytes += d->total_bytes;
  s->disk_matching_files += d->matching_files;
  s->disk_matching_bytes += d->matching_bytes;
  s->disk_tagged_files += d->tagged_files;
  s->disk_tagged_bytes += d->tagged_bytes;
}

void RecalculateSysStats(ViewContext *ctx, Statistic *s) {
  s->disk_total_files = 0;
  s->disk_total_bytes = 0;
  s->disk_matching_files = 0;
  s->disk_matching_bytes = 0;
  s->disk_tagged_files = 0;
  s->disk_tagged_bytes = 0;
  s->disk_total_directories = 0;

  if (s->tree) {
    s->disk_total_directories++;
    RecalcDir(ctx, s->tree, s);
  }
}

/* ************************************************************************* */
/*                           DISPLAY HELPERS                                 */
/* ************************************************************************* */

static void FormatNumber(const ViewContext *ctx, char *buf, size_t size,
                         long long val) {
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
      buf[--j] = ctx->number_seperator;
    }
  }
}

static void FormatShortSize(char *buf, size_t size, long long val) {
  double d = (double)val;
  const char *units[] = {"B", "K", "M", "G", "T", "P"};
  int i = 0;

  /* Handle negative values gracefully (though they shouldn't happen) */
  if (val < 0) {
    snprintf(buf, size, "Err");
    return;
  }

  while (d >= 999.5 &&
         i < 5) { /* threshold slightly < 1000 to avoid "1000K" -> "1.0M" */
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

static void SetColor(ViewContext *ctx) {
  wattrset(ctx->ctx_border_window, COLOR_PAIR(CPAIR_WINDIR));
}

static void DrawBoxFrame(ViewContext *ctx) {
  int y;
  int sep_y = ctx->layout.dir_win_y + ctx->layout.dir_win_height;

  wattron(ctx->ctx_border_window, COLOR_PAIR(CPAIR_BORDERS) | A_ALTCHARSET);

  /* --- Top Border with embedded " FILTER " --- */
  {
    const char *title = " FILTER ";
    int hline_len = R_BORDER - L_BORDER - 1;
    int t_len = strlen(title);
    int left_len = (hline_len - t_len) / 2;
    int right_len = hline_len - t_len - left_len;
    int x = L_BORDER + 1;

    /* Left HLINE */
    mvwhline(ctx->ctx_border_window, Y_TOP, x, ACS_HLINE, left_len);
    x += left_len;

    /* Title */
    wattroff(ctx->ctx_border_window, A_ALTCHARSET);
    wattron(ctx->ctx_border_window, A_BOLD);
    mvwaddstr(ctx->ctx_border_window, Y_TOP, x, title);
    wattroff(ctx->ctx_border_window, A_BOLD);
    wattron(ctx->ctx_border_window, A_ALTCHARSET);
    x += t_len;

    /* Right HLINE */
    mvwhline(ctx->ctx_border_window, Y_TOP, x, ACS_HLINE, right_len);
  }

  mvwaddch(ctx->ctx_border_window, Y_TOP, R_BORDER, ACS_URCORNER);

  /* --- Bottom Border --- */
  mvwaddch(ctx->ctx_border_window, ctx->layout.bottom_border_y, L_BORDER,
           ACS_LLCORNER);
  mvwhline(ctx->ctx_border_window, ctx->layout.bottom_border_y, L_BORDER + 1,
           ACS_HLINE, R_BORDER - L_BORDER - 1);
  mvwaddch(ctx->ctx_border_window, ctx->layout.bottom_border_y, R_BORDER,
           ACS_LRCORNER);

  /* --- Vertical Lines --- */
  for (y = Y_TOP + 1; y < ctx->layout.bottom_border_y; y++) {
    mvwaddch(ctx->ctx_border_window, y, R_BORDER, ACS_VLINE);
    mvwaddch(ctx->ctx_border_window, y, L_BORDER, ACS_VLINE);
  }

  /* --- Junctions --- */
  mvwaddch(ctx->ctx_border_window, Y_TOP, L_BORDER,
           ACS_TTEE); /* Connects to Path bar in main win */

  /* Handle File Window artifact */
  if (ctx->ctx_file_window == ctx->ctx_big_file_window) {
    mvwaddch(ctx->ctx_border_window, sep_y, L_BORDER, ACS_VLINE);
  } else {
    mvwaddch(ctx->ctx_border_window, sep_y, L_BORDER, ACS_RTEE);
  }
  mvwaddch(ctx->ctx_border_window, ctx->layout.bottom_border_y, L_BORDER,
           ACS_BTEE);

  wattroff(ctx->ctx_border_window, A_ALTCHARSET);
  wattrset(ctx->ctx_border_window, A_NORMAL);
  SetColor(ctx);
}

static void DrawSeparator(ViewContext *ctx, int y, const char *title) {
  int text_len = title ? strlen(title) : 0;
  int total_inner_width = R_BORDER - L_BORDER - 1;

  if (y <= 0)
    return;

  wattron(ctx->ctx_border_window, COLOR_PAIR(CPAIR_BORDERS) | A_ALTCHARSET);

  /* Side Junctions */
  mvwaddch(ctx->ctx_border_window, y, L_BORDER, ACS_LTEE);
  mvwaddch(ctx->ctx_border_window, y, R_BORDER, ACS_RTEE);

  if (title && text_len > 0) {
    int pad = 2; /* 1 space each side */

    if (total_inner_width >= text_len + pad) {
      int left_hline_len;
      int title_content_start_x;
      int rem = total_inner_width - (text_len + pad);
      left_hline_len = rem / 2;
      title_content_start_x = L_BORDER + 1 + left_hline_len;

      /* Left Line */
      mvwhline(ctx->ctx_border_window, y, L_BORDER + 1, ACS_HLINE,
               left_hline_len);

      /* Title */
      wattroff(ctx->ctx_border_window, A_ALTCHARSET);
      wattron(ctx->ctx_border_window, A_BOLD);
      mvwaddstr(ctx->ctx_border_window, y, title_content_start_x, " ");
      waddstr(ctx->ctx_border_window, title);
      waddstr(ctx->ctx_border_window, " ");
      wattroff(ctx->ctx_border_window, A_BOLD);
      wattron(ctx->ctx_border_window, A_ALTCHARSET);

      /* Right Line */
      mvwhline(ctx->ctx_border_window, y,
               title_content_start_x + text_len + pad, ACS_HLINE,
               total_inner_width - left_hline_len - text_len - pad);
    } else {
      /* No room for line, just title */
      wattrset(ctx->ctx_border_window, COLOR_PAIR(CPAIR_BORDERS) | A_BOLD);
      mvwaddnstr(ctx->ctx_border_window, y, L_BORDER + 1, title,
                 total_inner_width);
    }
  } else {
    /* Pure line */
    mvwhline(ctx->ctx_border_window, y, L_BORDER + 1, ACS_HLINE,
             total_inner_width);
  }
  wattroff(ctx->ctx_border_window, A_ALTCHARSET);
  wattrset(ctx->ctx_border_window, A_NORMAL);
}

static void PrintStatRow(ViewContext *ctx, int y, const char *label,
                         long long count, long long bytes) {
  char count_buf[32];
  char size_buf[32];

  if (y >= ctx->layout.bottom_border_y)
    return;

  FormatNumber(ctx, count_buf, sizeof(count_buf), count);
  FormatShortSize(size_buf, sizeof(size_buf), bytes);

  SetColor(ctx);
  /* Format: "Tot: 1,831,129 12.5M"
   * Math: 4 (Label) + 1 (Space) + 9 (Count) + 1 (Space) + 6 (Size) = 21 chars.
   * INNER_W is 22. This leaves 1 char padding. Perfect.
   */
  mvwprintw(ctx->ctx_border_window, y, STAT_X + 1, "%-4s %9s %6s", label,
            count_buf, size_buf);
}

static void DrawAttributes(ViewContext *ctx, const char *name,
                           const struct stat *s, const FileEntry *fe) {
  char buf[128];
  char num_buf[32];
  char time_buf[20];
  char temp[256]; /* Increased from 128 to fix truncation warning */

  if (!name || !s)
    return;

  DrawSeparator(ctx, ctx->layout.stats_y_attr_sep, "ATTRIBUTES");

  /* Name */
  CutPathname(buf, (char *)name, INNER_W); /* Keep CutPathname for stats box */

  /* Explicitly clear the line to avoid background artifacts */
  mvwhline(ctx->ctx_border_window, ctx->layout.stats_y_attr_val, STAT_X + 1,
           ' ', INNER_W);

  if (fe) {
#ifdef COLOR_SUPPORT
    int color = GetFileTypeColor(ctx, fe);
    wattron(ctx->ctx_border_window, COLOR_PAIR(color) | A_BOLD);
    mvwprintw(ctx->ctx_border_window, ctx->layout.stats_y_attr_val, STAT_X + 1,
              "%s", buf); /* Print without padding */
    wattroff(ctx->ctx_border_window, COLOR_PAIR(color) | A_BOLD);
#else
    wattron(ctx->ctx_border_window, A_BOLD);
    mvwprintw(ctx->ctx_border_window, ctx->layout.stats_y_attr_val, STAT_X + 1,
              "%s", buf);
    wattroff(ctx->ctx_border_window, A_BOLD);
#endif
  } else {
    wattron(ctx->ctx_border_window, A_BOLD);
    mvwprintw(ctx->ctx_border_window, ctx->layout.stats_y_attr_val, STAT_X + 1,
              "%s", buf); /* Print without padding */
    wattroff(ctx->ctx_border_window, A_BOLD);
  }

  /* Size */
  FormatShortSize(num_buf, sizeof(num_buf), s->st_size);
  snprintf(temp, sizeof(temp), "Size: %s", num_buf);
  mvwprintw(ctx->ctx_border_window, ctx->layout.stats_y_attr_val + 1,
            STAT_X + 1, "%-22s", temp);

  /* Attr */
  GetAttributes(s->st_mode, buf);
  snprintf(temp, sizeof(temp), "Attr: %s", buf);
  mvwprintw(ctx->ctx_border_window, ctx->layout.stats_y_attr_val + 2,
            STAT_X + 1, "%-22s", temp);

  /* Owner */
  {
    const char *owner = GetDisplayPasswdName(s->st_uid);
    const char *group = GetDisplayGroupName(s->st_gid);
    char owner_buf[32];
    char grp_buf[32];
    if (!owner) {
      snprintf(owner_buf, sizeof(owner_buf), "%d", s->st_uid);
      owner = owner_buf;
    }
    if (!group) {
      snprintf(grp_buf, sizeof(grp_buf), "%d", s->st_gid);
      group = grp_buf;
    }

    char full_own[64];
    snprintf(full_own, sizeof(full_own), "%s:%s", owner, group);
    CutName(buf, full_own, INNER_W - 6); /* "Own : " is 6 chars */
    mvwprintw(ctx->ctx_border_window, ctx->layout.stats_y_attr_val + 3,
              STAT_X + 1, "Own : %-16s", buf);
  }

  /* Mod */
  CTime(s->st_mtime, time_buf);
  mvwprintw(ctx->ctx_border_window, ctx->layout.stats_y_attr_val + 4,
            STAT_X + 1, "Mod : %-16s", time_buf);
}

/* ************************************************************************* */
/*                           DISPLAY FUNCTIONS */
/* ************************************************************************* */

void DisplayDiskName(ViewContext *ctx, const Statistic *s) {
  char buf[128];
  char path_buf[PATH_LENGTH + 1];
  int total_volumes = HASH_COUNT(ctx->volumes_head);
  int current_index = 0;

  if (ctx->layout.stats_width == 0)
    return;

  /* Recalculate layout based on current terminal height */
  RecalcLayout(ctx);

  /* 1. Determine Volume Index */
  if (ctx->volumes_head) {
    struct Volume *vol_iter, *tmp;
    int i = 1;
    HASH_ITER(hh, ctx->volumes_head, vol_iter, tmp) {
      if (&vol_iter->vol_stats == s) {
        current_index = i;
        break;
      }
      i++;
    }
  }
  if (current_index == 0 && total_volumes > 0)
    current_index = 1;

  /* 2. Setup Panel */
  SetColor(ctx);
  DrawBoxFrame(ctx); /* Draws Top Border with "FILTER" */

  /* 3. Filter Value */
  CutName(buf, s->file_spec, INNER_W);
  wattron(ctx->ctx_border_window, A_BOLD);
  /* Center filter text using padding format to clear ghosts */
  {
    int pad = (INNER_W - strlen(buf)) / 2;
    mvwprintw(ctx->ctx_border_window, ctx->layout.stats_y_filter_val,
              STAT_X + 1, "%*s%-*s", pad, "", INNER_W - pad, buf);
  }
  wattroff(ctx->ctx_border_window, A_BOLD);

  /* 4. Volume Section */
  snprintf(buf, sizeof(buf), "VOLUME %d/%d", current_index, total_volumes);
  DrawSeparator(ctx, ctx->layout.stats_y_vol_sep, buf);

  /* Path */
  if (ctx->view_mode == ARCHIVE_MODE)
    strncpy(path_buf, s->log_path, PATH_LENGTH);
  else
    strncpy(path_buf, s->path, PATH_LENGTH);
  path_buf[PATH_LENGTH] = '\0';

  CutPathname(buf, path_buf, INNER_W); /* Changed to CutPathname */
  mvwprintw(ctx->ctx_border_window, ctx->layout.stats_y_vol_info, STAT_X + 1,
            "%-*s", INNER_W, buf); /* Pad to clear */

  /* FS */
  char fs_buf[64];
  if (ctx->view_mode == ARCHIVE_MODE)
    snprintf(fs_buf, sizeof(fs_buf), "FS: ARCHIVE");
  else
    snprintf(fs_buf, sizeof(fs_buf), "FS: %s", s->disk_name);
  /* Truncate to fit */
  CutName(buf, fs_buf, INNER_W);
  mvwprintw(ctx->ctx_border_window, ctx->layout.stats_y_vol_info + 1,
            STAT_X + 1, "%-*s", INNER_W, buf);

  /* Free */
  if (ctx->view_mode == ARCHIVE_MODE) {
    snprintf(fs_buf, sizeof(fs_buf), "Free: -");
  } else {
    char size_buf[32];
    FormatShortSize(size_buf, sizeof(size_buf), s->disk_space);
    snprintf(fs_buf, sizeof(fs_buf), "Free: %s", size_buf);
  }
  mvwprintw(ctx->ctx_border_window, ctx->layout.stats_y_vol_info + 2,
            STAT_X + 1, "%-*s", INNER_W, fs_buf);
}

void DisplayAvailBytes(ViewContext *ctx, const Statistic *s) {
  DisplayDiskStatistic(ctx, s);
}

void DisplayFilter(ViewContext *ctx, const Statistic *s) {
  DisplayDiskStatistic(ctx, s);
}

void DisplayDiskStatistic(ViewContext *ctx, const Statistic *s) {
  if (ctx->layout.stats_width == 0)
    return;

  DisplayDiskName(ctx, s);

  DrawSeparator(ctx, ctx->layout.stats_y_vstat_sep, "VOLUME STATS");

  PrintStatRow(ctx, ctx->layout.stats_y_vstat_val, "Tot:", s->disk_total_files,
               s->disk_total_bytes);
  PrintStatRow(ctx, ctx->layout.stats_y_vstat_val + 1,
               "Mat:", s->disk_matching_files, s->disk_matching_bytes);
  PrintStatRow(ctx, ctx->layout.stats_y_vstat_val + 2,
               "Tag:", s->disk_tagged_files, s->disk_tagged_bytes);
}

void DisplayDirStatistic(ViewContext *ctx, const DirEntry *de,
                         const char *title, const Statistic *s) {
  char buf[128];

  if (ctx->layout.stats_width == 0)
    return;

  if (!de)
    return;

  /* Use provided title, or fallback to default logic */
  if (title) {
    DrawSeparator(ctx, ctx->layout.stats_y_dstat_sep, title);
  } else if (de->global_flag) {
    DrawSeparator(ctx, ctx->layout.stats_y_dstat_sep, "SHOW ALL");
  } else {
    if (ctx->view_mode == ARCHIVE_MODE) {
      DrawSeparator(ctx, ctx->layout.stats_y_dstat_sep, "ARCHIVE");
    } else {
      DrawSeparator(ctx, ctx->layout.stats_y_dstat_sep, "CURRENT DIR");
    }
  }

  /* Dir Name */
  CutPathname(buf, de->name, INNER_W); /* Changed to CutPathname */
  wattron(ctx->ctx_border_window, A_BOLD);
  mvwprintw(ctx->ctx_border_window, ctx->layout.stats_y_dstat_val, STAT_X + 1,
            "%-*s", INNER_W, buf); /* Clear ghosting */
  wattroff(ctx->ctx_border_window, A_BOLD);

  if (de->global_flag) {
    /* In Show All mode, display global totals */
    PrintStatRow(ctx, ctx->layout.stats_y_dstat_val + 1,
                 "Tot:", s->disk_total_files, s->disk_total_bytes);
    PrintStatRow(ctx, ctx->layout.stats_y_dstat_val + 2,
                 "Mat:", s->disk_matching_files, s->disk_matching_bytes);
  } else {
    /* In Normal mode, display current directory totals */
    PrintStatRow(ctx, ctx->layout.stats_y_dstat_val + 1,
                 "Tot:", de->total_files, de->total_bytes);
    PrintStatRow(ctx, ctx->layout.stats_y_dstat_val + 2,
                 "Mat:", de->matching_files, de->matching_bytes);
  }

  /* Tag count always shows global disk total in Show All mode, but we use the
   * disk stats directly if global_flag is set. */
  if (de->global_flag) {
    PrintStatRow(ctx, ctx->layout.stats_y_dstat_val + 3,
                 "Tag:", s->disk_tagged_files, s->disk_tagged_bytes);
  } else {
    PrintStatRow(ctx, ctx->layout.stats_y_dstat_val + 3,
                 "Tag:", de->tagged_files, de->tagged_bytes);
  }
}

/*
 * DisplayFileStatistic
 * Shows individual file information in the "CURRENT DIR" statistics area
 * when the user is navigating files (small or big window mode).
 */
void DisplayFileStatistic(ViewContext *ctx, const FileEntry *fe,
                          const Statistic *s) {
  char buf[128];
  char size_buf[32];
  char time_buf[20];

  if (ctx->layout.stats_width == 0)
    return;

  if (!fe)
    return;

  /* Title */
  DrawSeparator(ctx, ctx->layout.stats_y_dstat_sep, "CURRENT FILE");

  /* File Name */
  CutPathname(buf, fe->name, INNER_W);
#ifdef COLOR_SUPPORT
  int color = GetFileTypeColor(ctx, fe);
  wattron(ctx->ctx_border_window, COLOR_PAIR(color) | A_BOLD);
  mvwprintw(ctx->ctx_border_window, ctx->layout.stats_y_dstat_val, STAT_X + 1,
            "%-*s", INNER_W, buf);
  wattroff(ctx->ctx_border_window, COLOR_PAIR(color) | A_BOLD);
#else
  wattron(ctx->ctx_border_window, A_BOLD);
  mvwprintw(ctx->ctx_border_window, ctx->layout.stats_y_dstat_val, STAT_X + 1,
            "%-*s", INNER_W, buf);
  wattroff(ctx->ctx_border_window, A_BOLD);
#endif

  /* Size */
  FormatShortSize(size_buf, sizeof(size_buf), fe->stat_struct.st_size);
  mvwprintw(ctx->ctx_border_window, ctx->layout.stats_y_dstat_val + 1,
            STAT_X + 1, "Size: %-*s", INNER_W - 6, size_buf);

  /* Permissions */
  GetAttributes(fe->stat_struct.st_mode, buf);
  mvwprintw(ctx->ctx_border_window, ctx->layout.stats_y_dstat_val + 2,
            STAT_X + 1, "Perm: %-*s", INNER_W - 6, buf);

  /* Modified Time */
  CTime(fe->stat_struct.st_mtime, time_buf);
  mvwprintw(ctx->ctx_border_window, ctx->layout.stats_y_dstat_val + 3,
            STAT_X + 1, "Mod : %-*s", INNER_W - 6, time_buf);
}

void DisplayFileParameter(ViewContext *ctx, FileEntry *fe) {
  if (ctx->layout.stats_width == 0)
    return;
  if (fe) {
    DrawAttributes(ctx, fe->name, &fe->stat_struct, fe);
  }
}

/* ************************************************************************* */
/*                           COMPATIBILITY WRAPPERS                          */
/* ************************************************************************* */

void DisplayDiskTagged(ViewContext *ctx, const Statistic *s) {
  if (ctx->layout.stats_width == 0)
    return;
  DisplayDiskStatistic(ctx, s);
}

void DisplayDirTagged(ViewContext *ctx, const DirEntry *de,
                      const Statistic *s) {
  if (ctx->layout.stats_width == 0)
    return;
  DisplayDirStatistic(ctx, de, NULL, s);
}

void DisplayDirParameter(ViewContext *ctx, DirEntry *de) {
  if (ctx->layout.stats_width == 0)
    return;
  if (de) {
    DrawAttributes(ctx, de->name, &de->stat_struct, NULL);
  }
}

void DisplayGlobalFileParameter(ViewContext *ctx, FileEntry *fe) {
  if (ctx->layout.stats_width == 0)
    return;
  DisplayFileParameter(ctx, fe);
}
