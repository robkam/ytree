/***************************************************************************
 *
 * src/ui/stats.c
 * Statistics Module - Modernized Boxed Layout
 * Refactored to share attribute display logic between files and directories.
 * Responsive layout update.
 *
 ***************************************************************************/

#include "ytnova_appstate_volume.h"
#include "ytnova_cmd.h"
#include "ytnova_ui.h"

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
static void SetStatsBaseColor(ViewContext *ctx);
static void SetStatsStaticColor(ViewContext *ctx);
static void SetStatsDynamicColor(ViewContext *ctx);
static void SetStatsBorderColor(ViewContext *ctx);
static void DrawBoxFrame(ViewContext *ctx);
static void DrawSeparator(ViewContext *ctx, int y, const char *title);
static void PrintStatRow(ViewContext *ctx, int y, const char *label,
                         long long count, long long bytes);
static void PrintStatsDynamicLine(ViewContext *ctx, int y, const char *value);
static void PrintStatsLabelValue(ViewContext *ctx, int y, const char *label,
                                 const char *value);
static void DrawAttributes(ViewContext *ctx, const char *name,
                           const struct stat *s, const FileEntry *fe);
static void RecalcDir(BOOL hide_dot_files, DirEntry *d, Statistic *s);

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

static void RecalcDir(BOOL hide_dot_files, DirEntry *d, Statistic *s) {
  FileEntry *f;
  DirEntry *sub;
  unsigned int total_files;
  unsigned int tagged_files;
  long long total_bytes;
  long long tagged_bytes;

  /* Apply current filter to this directory */
  ApplyFilter(d, s);

  total_files = 0;
  total_bytes = 0;
  tagged_files = d->tagged_files;
  tagged_bytes = d->tagged_bytes;
  /* matching_files/bytes already updated by ApplyFilter, but we sum them
   * globally below */

  for (f = d->file; f; f = f->next) {
    if (hide_dot_files && f->name[0] == '.')
      continue;

    total_files++;
    total_bytes += f->stat_struct.st_size;

    if (f->tagged) {
      tagged_files++;
      tagged_bytes += f->stat_struct.st_size;
    }
  }
  if (!AppStateCommitDirEntryTotalPayload(d, total_files, total_bytes))
    return;
  if (!AppStateCommitDirEntryTaggedPayload(d, tagged_files, tagged_bytes))
    return;

  sub = d->sub_tree;
  while (sub) {
    RecalcDir(hide_dot_files, sub, s);
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
  BOOL hide_dot_files = (ctx && ctx->active && ctx->active->hide_dot_files);

  s->disk_total_files = 0;
  s->disk_total_bytes = 0;
  s->disk_matching_files = 0;
  s->disk_matching_bytes = 0;
  s->disk_tagged_files = 0;
  s->disk_tagged_bytes = 0;
  s->disk_total_directories = 0;

  if (s->tree) {
    s->disk_total_directories++;
    RecalcDir(hide_dot_files, s->tree, s);
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

static void SetStatsBaseColor(ViewContext *ctx) {
#ifdef COLOR_SUPPORT
  wattrset(ctx->ctx_border_window, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));
#else
  wattrset(ctx->ctx_border_window, A_NORMAL);
#endif
}

static void SetStatsStaticColor(ViewContext *ctx) {
#ifdef COLOR_SUPPORT
  wattrset(ctx->ctx_border_window, COLOR_PAIR(UI_ROLE_STATIC_TEXT));
#else
  wattrset(ctx->ctx_border_window, A_BOLD);
#endif
}

static void SetStatsDynamicColor(ViewContext *ctx) {
#ifdef COLOR_SUPPORT
  wattrset(ctx->ctx_border_window, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));
#else
  wattrset(ctx->ctx_border_window, A_NORMAL);
#endif
}

static void SetStatsBorderColor(ViewContext *ctx) {
#ifdef COLOR_SUPPORT
  wattrset(ctx->ctx_border_window, COLOR_PAIR(UI_ROLE_BOX_LINES));
#else
  wattrset(ctx->ctx_border_window, A_NORMAL);
#endif
}

static void DrawBoxFrame(ViewContext *ctx) {
  int y;
  int sep_y = ctx->layout.dir_win_y + ctx->layout.dir_win_height;

  SetStatsBorderColor(ctx);
  wattron(ctx->ctx_border_window, A_ALTCHARSET);

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

    wattroff(ctx->ctx_border_window, A_ALTCHARSET);
    SetStatsStaticColor(ctx);
    mvwaddstr(ctx->ctx_border_window, Y_TOP, x, title);
    SetStatsBorderColor(ctx);
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
  SetStatsBaseColor(ctx);
}

static void DrawSeparator(ViewContext *ctx, int y, const char *title) {
  int text_len = title ? strlen(title) : 0;
  int total_inner_width = R_BORDER - L_BORDER - 1;

  if (y <= 0)
    return;

  SetStatsBorderColor(ctx);
  wattron(ctx->ctx_border_window, A_ALTCHARSET);

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

      wattroff(ctx->ctx_border_window, A_ALTCHARSET);
      SetStatsStaticColor(ctx);
      mvwaddstr(ctx->ctx_border_window, y, title_content_start_x, " ");
      waddstr(ctx->ctx_border_window, title);
      waddstr(ctx->ctx_border_window, " ");
      SetStatsBorderColor(ctx);
      wattron(ctx->ctx_border_window, A_ALTCHARSET);

      /* Right Line */
      mvwhline(ctx->ctx_border_window, y,
               title_content_start_x + text_len + pad, ACS_HLINE,
               total_inner_width - left_hline_len - text_len - pad);
    } else {
      SetStatsStaticColor(ctx);
      mvwaddnstr(ctx->ctx_border_window, y, L_BORDER + 1, title,
                 total_inner_width);
      SetStatsBorderColor(ctx);
    }
  } else {
    /* Pure line */
    mvwhline(ctx->ctx_border_window, y, L_BORDER + 1, ACS_HLINE,
             total_inner_width);
  }
  wattroff(ctx->ctx_border_window, A_ALTCHARSET);
  SetStatsBaseColor(ctx);
}

static void PrintStatRow(ViewContext *ctx, int y, const char *label,
                         long long count, long long bytes) {
  char count_buf[32];
  char size_buf[32];

  if (y >= ctx->layout.bottom_border_y)
    return;

  FormatNumber(ctx, count_buf, sizeof(count_buf), count);
  FormatShortSize(size_buf, sizeof(size_buf), bytes);

  SetStatsBaseColor(ctx);
  mvwhline(ctx->ctx_border_window, y, STAT_X + 1, ' ', INNER_W);
  SetStatsStaticColor(ctx);
  mvwprintw(ctx->ctx_border_window, y, STAT_X + 1, "%-4s ", label);
  SetStatsDynamicColor(ctx);
  mvwprintw(ctx->ctx_border_window, y, STAT_X + 6, "%9s %6s", count_buf,
            size_buf);
  SetStatsBaseColor(ctx);
}

static void PrintStatsDynamicLine(ViewContext *ctx, int y, const char *value) {
  char clipped[256];

  if (y >= ctx->layout.bottom_border_y)
    return;

  CutPathname(clipped, (char *)value, INNER_W);
  SetStatsBaseColor(ctx);
  mvwhline(ctx->ctx_border_window, y, STAT_X + 1, ' ', INNER_W);
  SetStatsDynamicColor(ctx);
  mvwprintw(ctx->ctx_border_window, y, STAT_X + 1, "%-*s", INNER_W, clipped);
  SetStatsBaseColor(ctx);
}

static void PrintStatsLabelValue(ViewContext *ctx, int y, const char *label,
                                 const char *value) {
  int label_len;
  int value_width;

  if (y >= ctx->layout.bottom_border_y)
    return;

  label_len = (int)strlen(label);
  value_width = INNER_W - label_len;
  if (value_width < 0)
    value_width = 0;

  SetStatsBaseColor(ctx);
  mvwhline(ctx->ctx_border_window, y, STAT_X + 1, ' ', INNER_W);
  SetStatsStaticColor(ctx);
  mvwprintw(ctx->ctx_border_window, y, STAT_X + 1, "%s", label);
  SetStatsDynamicColor(ctx);
  if (value_width > 0)
    mvwprintw(ctx->ctx_border_window, y, STAT_X + 1 + label_len, "%-*.*s",
              value_width, value_width, value);
  SetStatsBaseColor(ctx);
}

static void DrawAttributes(ViewContext *ctx, const char *name,
                           const struct stat *s, const FileEntry *fe) {
  char buf[128];
  char num_buf[32];
  char time_buf[20];

  if (!name || !s)
    return;

  DrawSeparator(ctx, ctx->layout.stats_y_attr_sep, "ATTRIBUTES");

  (void)fe;
  PrintStatsDynamicLine(ctx, ctx->layout.stats_y_attr_val, name);

  FormatShortSize(num_buf, sizeof(num_buf), s->st_size);
  PrintStatsLabelValue(ctx, ctx->layout.stats_y_attr_val + 1, "Size: ",
                       num_buf);

  GetAttributes(s->st_mode, buf);
  PrintStatsLabelValue(ctx, ctx->layout.stats_y_attr_val + 2, "Attr: ", buf);

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
    PrintStatsLabelValue(ctx, ctx->layout.stats_y_attr_val + 3, "Own : ", buf);
  }

  CTime(s->st_mtime, time_buf);
  PrintStatsLabelValue(ctx, ctx->layout.stats_y_attr_val + 4, "Mod : ",
                       time_buf);
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

  SetStatsBaseColor(ctx);
  DrawBoxFrame(ctx);

  CutName(buf, s->file_spec, INNER_W);
  SetStatsBaseColor(ctx);
  mvwhline(ctx->ctx_border_window, ctx->layout.stats_y_filter_val, STAT_X + 1,
           ' ', INNER_W);
  SetStatsDynamicColor(ctx);
  {
    int pad = (INNER_W - strlen(buf)) / 2;
    mvwprintw(ctx->ctx_border_window, ctx->layout.stats_y_filter_val,
              STAT_X + 1, "%*s%-*s", pad, "", INNER_W - pad, buf);
  }
  SetStatsBaseColor(ctx);

  snprintf(buf, sizeof(buf), "VOLUME %d/%d", current_index, total_volumes);
  DrawSeparator(ctx, ctx->layout.stats_y_vol_sep, buf);

  if (ctx->view_mode == ARCHIVE_MODE)
    strncpy(path_buf, s->log_path, PATH_LENGTH);
  else
    strncpy(path_buf, s->path, PATH_LENGTH);
  path_buf[PATH_LENGTH] = '\0';

  PrintStatsDynamicLine(ctx, ctx->layout.stats_y_vol_info, path_buf);

  char fs_buf[64];
  if (ctx->view_mode == ARCHIVE_MODE)
    snprintf(fs_buf, sizeof(fs_buf), "ARCHIVE");
  else
    snprintf(fs_buf, sizeof(fs_buf), "%s", s->disk_name);
  CutName(buf, fs_buf, INNER_W - 4);
  PrintStatsLabelValue(ctx, ctx->layout.stats_y_vol_info + 1, "FS: ", buf);

  if (ctx->view_mode == ARCHIVE_MODE) {
    snprintf(fs_buf, sizeof(fs_buf), "-");
  } else {
    char size_buf[32];
    int free_percent = -1;
    FormatShortSize(size_buf, sizeof(size_buf), s->disk_space);
    if (s->disk_capacity > 0) {
      double percent = ((double)s->disk_space * 100.0) / (double)s->disk_capacity;
      if (percent < 0.0)
        percent = 0.0;
      if (percent > 100.0)
        percent = 100.0;
      free_percent = (int)(percent + 0.5);
    }
    if (free_percent >= 0)
      snprintf(fs_buf, sizeof(fs_buf), "%s (%d%%)", size_buf, free_percent);
    else
      snprintf(fs_buf, sizeof(fs_buf), "%s", size_buf);
  }
  PrintStatsLabelValue(ctx, ctx->layout.stats_y_vol_info + 2, "Free: ",
                       fs_buf);
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

  PrintStatsDynamicLine(ctx, ctx->layout.stats_y_dstat_val, de->name);

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
  char size_buf[32];
  char time_buf[20];

  if (ctx->layout.stats_width == 0)
    return;

  if (!fe)
    return;

  DrawSeparator(ctx, ctx->layout.stats_y_dstat_sep, "CURRENT FILE");

  PrintStatsDynamicLine(ctx, ctx->layout.stats_y_dstat_val, fe->name);

  FormatShortSize(size_buf, sizeof(size_buf), fe->stat_struct.st_size);
  PrintStatsLabelValue(ctx, ctx->layout.stats_y_dstat_val + 1, "Size: ",
                       size_buf);

  {
    char attr_buf[16];
    GetAttributes(fe->stat_struct.st_mode, attr_buf);
    PrintStatsLabelValue(ctx, ctx->layout.stats_y_dstat_val + 2, "Perm: ",
                         attr_buf);
  }

  CTime(fe->stat_struct.st_mtime, time_buf);
  PrintStatsLabelValue(ctx, ctx->layout.stats_y_dstat_val + 3, "Mod : ",
                       time_buf);
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
