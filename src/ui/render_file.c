/***************************************************************************
 *
 * src/ui/render_file.c
 * Rendering logic for the file window (List View)
 *
 ***************************************************************************/

#include "ytnova_appstate_panel.h"
#include "ytnova_cmd.h"
#include "ytnova_fs.h"
#include "ytnova_ui.h"

#include <ctype.h>

#define MAX(a, b) (((a) > (b)) ? (a) : (b))

static char GetTypeOfFile(struct stat fst);
static int GetVisualFileEntryLength(ViewContext *ctx, YtreeNovaPanel *p);
static void BuildFileRowLabel(char *buffer, size_t buffer_size,
                              const YtreeNovaPanel *panel,
                              const FileEntry *fe_ptr, char type_of_file);
static void FormatPanelSize(const ViewContext *ctx, const YtreeNovaPanel *panel,
                            long long value, char *buffer, size_t buffer_size);
static void BuildBasicSummaryDetail(const ViewContext *ctx,
                                    const YtreeNovaPanel *panel,
                                    const FileEntry *fe_ptr,
                                    char type_of_file, char *buffer,
                                    size_t buffer_size);
static void BuildOverlayDetail(const ViewContext *ctx,
                               const YtreeNovaPanel *panel,
                               const FileEntry *fe_ptr, char type_of_file,
                               char *buffer, size_t buffer_size);

static void TrimTrailingSpaces(char *buffer) {
  size_t len;

  if (!buffer)
    return;

  len = strlen(buffer);
  while (len > 0 && isspace((unsigned char)buffer[len - 1])) {
    buffer[len - 1] = '\0';
    len--;
  }
}

static void NormalizeOverlayText(const unsigned char *src, size_t src_len,
                                 char *buffer, size_t buffer_size) {
  size_t in_pos;
  size_t out_pos = 0;
  BOOL last_was_space = TRUE;

  if (!buffer || buffer_size == 0)
    return;

  buffer[0] = '\0';
  if (!src || src_len == 0)
    return;

  for (in_pos = 0; in_pos < src_len && src[in_pos] != '\0'; in_pos++) {
    unsigned char ch = src[in_pos];

    if (isspace(ch)) {
      if (!last_was_space && out_pos + 1 < buffer_size) {
        buffer[out_pos++] = ' ';
        last_was_space = TRUE;
      }
      continue;
    }

    if (!isprint(ch))
      break;

    if (out_pos + 1 >= buffer_size)
      break;

    buffer[out_pos++] = (char)ch;
    last_was_space = FALSE;
  }

  buffer[out_pos] = '\0';
  TrimTrailingSpaces(buffer);
}

static BOOL BufferLooksTextual(const unsigned char *buffer, size_t length) {
  size_t i;
  size_t printable_count = 0;

  if (!buffer)
    return FALSE;
  if (length == 0)
    return TRUE;

  for (i = 0; i < length; i++) {
    unsigned char ch = buffer[i];

    if (ch == '\0')
      return FALSE;
    if (isprint(ch) || isspace(ch)) {
      printable_count++;
      continue;
    }
    if (ch == '\b' || ch == '\f')
      continue;
    return FALSE;
  }

  return printable_count * 5 >= length * 4;
}

static BOOL BuildSnippetDetail(const FileEntry *fe_ptr, char *buffer,
                               size_t buffer_size) {
  char path[PATH_LENGTH + 1];
  unsigned char sample[97];
  char snippet[80];
  FILE *file_fp;
  size_t read_now;
  BOOL truncated = FALSE;

  if (!buffer || buffer_size == 0) {
    return FALSE;
  }
  buffer[0] = '\0';

  if (!fe_ptr || S_ISDIR(fe_ptr->stat_struct.st_mode))
    return FALSE;

  GetFileNamePath((FileEntry *)fe_ptr, path);
  path[PATH_LENGTH] = '\0';

  file_fp = fopen(path, "rb");
  if (!file_fp)
    return FALSE;

  read_now = fread(sample, 1, sizeof(sample) - 1, file_fp);
  truncated = (read_now == sizeof(sample) - 1 && !feof(file_fp)) ? TRUE : FALSE;
  fclose(file_fp);

  if (read_now == 0) {
    snprintf(buffer, buffer_size, " [empty]");
    return TRUE;
  }

  sample[read_now] = '\0';
  if (!BufferLooksTextual(sample, read_now)) {
    snprintf(buffer, buffer_size, " [binary]");
    return TRUE;
  }

  NormalizeOverlayText(sample, read_now, snippet, sizeof(snippet));
  if (snippet[0] == '\0') {
    snprintf(buffer, buffer_size, " [empty]");
    return TRUE;
  }

  snprintf(buffer, buffer_size, " [%s%s]", snippet, truncated ? " ..." : "");
  return TRUE;
}

static void BuildBasicSummaryDetail(const ViewContext *ctx,
                                    const YtreeNovaPanel *panel,
                                    const FileEntry *fe_ptr,
                                    char type_of_file, char *buffer,
                                    size_t buffer_size) {
  const char *type_summary = "file";
  char size_buf[32];

  if (!buffer || buffer_size == 0)
    return;

  buffer[0] = '\0';
  if (!panel || !fe_ptr)
    return;

  if (S_ISDIR(fe_ptr->stat_struct.st_mode))
    type_summary = "dir";
  else if (S_ISLNK(fe_ptr->stat_struct.st_mode))
    type_summary = "link";
  else if (S_ISREG(fe_ptr->stat_struct.st_mode) &&
           (fe_ptr->stat_struct.st_mode & 0111) != 0)
    type_summary = "exec";
  else if (type_of_file == '@')
    type_summary = "link";

  FormatPanelSize(ctx, panel, (long long)fe_ptr->stat_struct.st_size, size_buf,
                  sizeof(size_buf));
  (void)snprintf(buffer, buffer_size, " [%s %s]", type_summary, size_buf);
}

static BOOL BuildFileCommandDetail(const FileEntry *fe_ptr, char *buffer,
                                   size_t buffer_size) {
  char path[PATH_LENGTH + 1];
  char quoted_path[(PATH_LENGTH * 4) + 3];
  char command[(PATH_LENGTH * 4) + 32];
  char output[160];
  char normalized[128];
  FILE *pipe_fp;

  if (!buffer || buffer_size == 0)
    return FALSE;

  buffer[0] = '\0';
  if (!fe_ptr)
    return FALSE;

  GetFileNamePath((FileEntry *)fe_ptr, path);
  path[PATH_LENGTH] = '\0';
  if (!Path_ShellQuote(path, quoted_path, sizeof(quoted_path)))
    return FALSE;
  if (snprintf(command, sizeof(command), "file -b -- %s 2>/dev/null",
               quoted_path) >= (int)sizeof(command))
    return FALSE;

  pipe_fp = popen(command, "r");
  if (!pipe_fp)
    return FALSE;

  output[0] = '\0';
  if (!fgets(output, sizeof(output), pipe_fp)) {
    (void)pclose(pipe_fp);
    return FALSE;
  }
  if (pclose(pipe_fp) != 0)
    return FALSE;

  NormalizeOverlayText((const unsigned char *)output, strlen(output),
                       normalized, sizeof(normalized));
  if (normalized[0] == '\0')
    return FALSE;

  snprintf(buffer, buffer_size, " [%s]", normalized);
  return TRUE;
}

static void FormatBinarySize(const ViewContext *ctx, long long value,
                             char *buffer, size_t buffer_size) {
  char temp[64];
  int len;
  int commas;
  int i;
  int j;
  char separator;

  if (!buffer || buffer_size == 0)
    return;

  snprintf(temp, sizeof(temp), "%lld", value);
  len = (int)strlen(temp);
  commas = (len - 1) / 3;
  if ((size_t)(len + commas + 1) > buffer_size) {
    snprintf(buffer, buffer_size, "%lld", value);
    return;
  }

  separator = (ctx && ctx->number_seperator) ? ctx->number_seperator : ',';
  j = len + commas;
  buffer[j] = '\0';
  for (i = len - 1; i >= 0; i--) {
    buffer[--j] = temp[i];
    if (i > 0 && (len - i) % 3 == 0)
      buffer[--j] = separator;
  }
}

static void FormatPanelSize(const ViewContext *ctx, const YtreeNovaPanel *panel,
                            long long value, char *buffer, size_t buffer_size) {
  double scaled = (double)value;
  int unit_index = 0;
  static const char *units[] = {"B", "K", "M", "G", "T", "P"};

  if (!buffer || buffer_size == 0)
    return;

  if (!panel || !panel->human_size_units) {
    FormatBinarySize(ctx, value, buffer, buffer_size);
    return;
  }

  if (value < 0) {
    snprintf(buffer, buffer_size, "Err");
    return;
  }

  while (scaled >= 999.5 && unit_index < 5) {
    scaled /= 1024.0;
    unit_index++;
  }

  if (unit_index == 0) {
    snprintf(buffer, buffer_size, "%lld%s", value, units[unit_index]);
  } else {
    snprintf(buffer, buffer_size, "%.1f%s", scaled, units[unit_index]);
  }
}

static void BuildOverlayDetail(const ViewContext *ctx,
                               const YtreeNovaPanel *panel,
                               const FileEntry *fe_ptr, char type_of_file,
                               char *buffer, size_t buffer_size) {
  char attr_buf[16];
  char size_buf[32];
  char time_buf[20];

  if (!buffer || buffer_size == 0) {
    return;
  }
  buffer[0] = '\0';
  if (!panel || !fe_ptr)
    return;

  if (panel->fileinfo_overlay_mode == FILEINFO_OVERLAY_NONE)
    return;

  if (panel->fileinfo_overlay_mode == FILEINFO_OVERLAY_SUMMARY) {
    if (BuildFileCommandDetail(fe_ptr, buffer, buffer_size))
      return;
    BuildBasicSummaryDetail(ctx, panel, fe_ptr, type_of_file, buffer,
                            buffer_size);
    return;
  }

  if (panel->fileinfo_overlay_mode == FILEINFO_OVERLAY_GIT) {
    FileInfoGitDescribe(panel, fe_ptr, buffer, buffer_size);
    return;
  }

  if (panel->fileinfo_overlay_mode == FILEINFO_OVERLAY_RICH &&
      BuildSnippetDetail(fe_ptr, buffer, buffer_size))
    return;

  GetAttributes(fe_ptr->stat_struct.st_mode, attr_buf);
  FormatPanelSize(ctx, panel, (long long)fe_ptr->stat_struct.st_size, size_buf,
                  sizeof(size_buf));
  CTime(fe_ptr->stat_struct.st_mtime, time_buf);
  (void)snprintf(buffer, buffer_size, " %10s %11s %16s", attr_buf, size_buf,
                 time_buf);
}

static void AddClippedAtCursor(WINDOW *win, const char *text, int width) {
  int y, x, remaining;

  if (!win || !text || width <= 0)
    return;

  getyx(win, y, x);
  (void)y;
  remaining = width - x;
  if (remaining <= 0)
    return;

  waddnstr(win, text, remaining);
}

void SetFileRenderingMetrics(YtreeNovaPanel *p, unsigned max_filename,
                             unsigned max_linkname, unsigned max_userview) {
  if (!p)
    return;
  (void)AppStateCommitPanelFileRenderingMetrics(
      p, max_filename, max_linkname, max_userview, max_userview > 0);
}

void SetRenderSortOrder(YtreeNovaPanel *p, BOOL reverse) {
  if (!p)
    return;
  (void)AppStateCommitPanelFileSortOrder(p, reverse);
}

int GetPanelFileMode(const YtreeNovaPanel *p) {
  if (!p)
    return MODE_1;
  return p->file_mode;
}

int GetPanelMaxColumn(const YtreeNovaPanel *p) {
  if (!p)
    return 1;
  return p->max_column;
}

void SetPanelFileMode(ViewContext *ctx, YtreeNovaPanel *p, int new_file_mode) {
  int width;
  unsigned max_column;

  if (!p)
    return;

  if (!AppStateCommitPanelFileDisplayMode(p, new_file_mode))
    return;

  /* Use the existing window if available, otherwise calculate from layout or
   * defer */
  if (p->pan_file_window) {
    width = getmaxx(p->pan_file_window);
  } else {
    /* Fallback if window not created yet (e.g. init), use layout hint or
     * default */
    if (p == ctx->left)
      width = ctx->layout.dir_win_width; /* approximation */
    else
      width = COLS - ctx->layout.dir_win_width;
    if (width < 10)
      width = 80; /* Safe default */
  }

  if (p->fileinfo_overlay_mode != FILEINFO_OVERLAY_NONE &&
      p->fixed_col_width == 0) {
    max_column = 1;
  } else {
    max_column = (unsigned)(width / (GetVisualFileEntryLength(ctx, p) + 1));

    if (max_column == 0)
      max_column = 1;
  }

  (void)AppStateCommitPanelFileMaxColumn(p, max_column);
}

void SelectPanelFileMode(ViewContext *ctx, YtreeNovaPanel *p, int selection) {
  int new_mode = MODE_3;
  int current_mode;

  if (!p)
    return;

  current_mode = p->file_mode;

  switch (selection) {
  case 1:
    new_mode = MODE_3;
    break;
  case 2:
    new_mode = (current_mode == MODE_1) ? MODE_3 : MODE_1;
    break;
  case 3:
    new_mode = (current_mode == MODE_2) ? MODE_3 : MODE_2;
    break;
  case 4:
    new_mode = (current_mode == MODE_4) ? MODE_3 : MODE_4;
    break;
  default:
    return;
  }

  if ((ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) &&
      new_mode == MODE_4)
    return;
  SetPanelFileMode(ctx, p, new_mode);
}

void RotatePanelFileMode(ViewContext *ctx, YtreeNovaPanel *p) {
  if (!p)
    return;

  switch (p->file_mode) {
  case MODE_1:
    SetPanelFileMode(ctx, p, MODE_3);
    break;
  case MODE_2:
    SetPanelFileMode(ctx, p, MODE_5);
    break;
  case MODE_3:
    SetPanelFileMode(ctx, p, MODE_4);
    break;
  case MODE_4:
    SetPanelFileMode(ctx, p, MODE_2);
    break;
  case MODE_5:
    SetPanelFileMode(ctx, p, MODE_1);
    break;
  }
  if ((ctx->view_mode != DISK_MODE && ctx->view_mode != USER_MODE) &&
      p->file_mode == MODE_4) {
    RotatePanelFileMode(ctx, p);
  } else if (p->file_mode == MODE_5 &&
             !strcmp((GetProfileValue)(ctx, "USERVIEW"), "")) {
    RotatePanelFileMode(ctx, p);
  }
}

static int OverlayDetailBudget(const YtreeNovaPanel *panel) {
  if (!panel)
    return 0;

  switch (panel->fileinfo_overlay_mode) {
  case FILEINFO_OVERLAY_RICH:
  case FILEINFO_OVERLAY_SUMMARY:
    return 56;
  case FILEINFO_OVERLAY_GIT:
    return 14;
  default:
    return 0;
  }
}

static int OverlayNameColumnWidth(const YtreeNovaPanel *panel, int window_width,
                                  int filename_width) {
  int max_name_width;

  if (!panel || panel->fileinfo_overlay_mode == FILEINFO_OVERLAY_NONE ||
      window_width <= 0)
    return filename_width;

  max_name_width = window_width - OverlayDetailBudget(panel) - 4;
  if (max_name_width < 12)
    max_name_width = 12;
  if (max_name_width > filename_width)
    return filename_width;
  return max_name_width;
}

static int GetVisualFileEntryLength(ViewContext *ctx, YtreeNovaPanel *p) {
  int filename_len = p->max_visual_filename_len;
  int overlay_extra = 0;
  int render_mode = p->file_mode;
  if (filename_len == 0 &&
      !strcmp((GetProfileValue)(ctx, "USERVIEW"), ""))
    filename_len = 14; /* Sensible default for small windows */

  if (p->fixed_col_width > 0)
    return p->fixed_col_width;

  if (p->fileinfo_overlay_mode != FILEINFO_OVERLAY_NONE) {
    render_mode = MODE_3;
    overlay_extra = OverlayDetailBudget(p);
  }

  int len = 0;

  switch (render_mode) {
  case MODE_1:
    len = (p->max_visual_linkname_len) ? p->max_visual_linkname_len + 4
                                       : 0; /* linkname + " -> " */
    len +=
        filename_len + 46; /* filename + format (increased for 11-digit size) */
    break;

  case MODE_2:
    len = filename_len + 44; /* filename + format (11-digit size) */
    break;

  case MODE_3:
    len = filename_len + 2; /* filename + format */
    break;

  case MODE_4:
    len = (p->max_visual_linkname_len) ? p->max_visual_linkname_len + 4
                                       : 0; /* linkname + " -> " */
    len += filename_len +
           47; /* filename + format (increased by 8 for two 16-char dates) */
    break;

  case MODE_5:
    len = GetVisualUserFileEntryLength(filename_len, p->max_visual_linkname_len,
                                       (GetProfileValue)(ctx, "USERVIEW"));
    (void)AppStateCommitPanelFileRenderingMetrics(
        p, p->max_visual_filename_len, p->max_visual_linkname_len,
        (unsigned)len, TRUE);
    break;
  }

  len += overlay_extra;
  return len;
}

int ResolveCompactFileWidth(const ViewContext *ctx,
                            const YtreeNovaPanel *panel) {
  int width;
  int current_columns;
  int target_columns;
  int compact_width;

  if (!ctx || !panel)
    return 24;

  if (panel->pan_file_window)
    width = getmaxx(panel->pan_file_window);
  else if (ctx->ctx_file_window)
    width = getmaxx(ctx->ctx_file_window);
  else if (panel == ctx->left)
    width = ctx->layout.dir_win_width;
  else
    width = COLS - ctx->layout.dir_win_width;

  if (width < 24)
    width = 24;

  current_columns = (int)panel->max_column;
  if (current_columns < 1)
    current_columns = 1;
  target_columns = current_columns + 1;

  compact_width = (width / target_columns) - 1;
  if (compact_width < 12)
    compact_width = 12;
  if (compact_width >= width)
    compact_width = width - 1;
  if (compact_width < 1)
    compact_width = 1;

  return compact_width;
}

static char GetTypeOfFile(struct stat fst) {
  if (S_ISLNK(fst.st_mode))
    return '@';
  else if (S_ISSOCK(fst.st_mode))
    return '=';
  else if (S_ISCHR(fst.st_mode))
    return '-';
  else if (S_ISBLK(fst.st_mode))
    return '+';
  else if (S_ISFIFO(fst.st_mode))
    return '|';
  else if (S_ISREG(fst.st_mode))
    return ' ';
  else
    return '?';
}

static void BuildFileRowLabel(char *buffer, size_t buffer_size,
                              const YtreeNovaPanel *panel,
                              const FileEntry *fe_ptr, char type_of_file) {
  const char *name_text;
  const char *link_target = NULL;
  int written = 0;

  if (!buffer || buffer_size == 0)
    return;

  if (!fe_ptr) {
    buffer[0] = '\0';
    return;
  }

  name_text = fe_ptr->name;
  if (panel && panel->show_symlink_targets &&
      S_ISLNK(fe_ptr->stat_struct.st_mode)) {
    size_t name_len = strlen(fe_ptr->name);
    link_target = &fe_ptr->name[name_len + 1];
  }

  if (type_of_file == ' ') {
    if (link_target && *link_target) {
      written = snprintf(buffer, buffer_size, "%s -> %s", name_text,
                         link_target);
    } else {
      written = snprintf(buffer, buffer_size, "%s", name_text);
    }
  } else {
    if (link_target && *link_target) {
      written = snprintf(buffer, buffer_size, "%c%s -> %s", type_of_file,
                         name_text, link_target);
    } else {
      written = snprintf(buffer, buffer_size, "%c%s", type_of_file, name_text);
    }
  }

  if (written < 0) {
    buffer[0] = '\0';
  } else if ((size_t)written >= buffer_size) {
    buffer[buffer_size - 1] = '\0';
  }
}

void PrintFileEntry(ViewContext *ctx, YtreeNovaPanel *panel, int entry_no, int y,
                    int x, unsigned char hilight, int start_x, WINDOW *win) {
  char attributes[11];

  if (!ctx || !panel || !panel->vol || !win)
    return;
  char modify_time[20];
  char change_time[20];
  char access_time[20];
  char size_text[32];
  char format[60];
  char justify;
  char *line_ptr;
  int n, pos_x = 0;
  FileEntry *fe_ptr;
  static char *line_buffer = NULL;
  static int old_cols = -1;
  static size_t line_buffer_size = 0;
  char owner[OWNER_NAME_MAX + 1];
  char group[GROUP_NAME_MAX + 1];
  char *owner_name_ptr;
  char *group_name_ptr;
  int ef_window_width;
  char *sym_link_name = NULL;
  char type_of_file;
  int filename_width = 0;
  int linkname_width = 0;
  int base_color_pair;
  int width;
  int render_mode;
  BOOL is_tagged;
  BOOL is_active_panel;
  BOOL align_name_col;
  BOOL uses_overlay_detail;
  int inactive_highlight_attr;
  int highlight_color_pair;
  char overlay_detail[PATH_LENGTH + 128];
  char row_label[PATH_LENGTH * 2 + 8];
  char plain_name[PATH_LENGTH * 2 + 8];
  const char *primary_name;

  if (!panel->file_entry_list)
    return;

  width = getmaxx(win);

  fe_ptr = panel->file_entry_list[entry_no].file;
  if (fe_ptr == NULL)
    return;
  render_mode = panel->file_mode;
  if (panel->fileinfo_overlay_mode != FILEINFO_OVERLAY_NONE) {
    render_mode = MODE_3;
  }
  uses_overlay_detail =
      (panel->fileinfo_overlay_mode != FILEINFO_OVERLAY_NONE &&
       render_mode == MODE_3);
  BuildOverlayDetail(ctx, panel, fe_ptr, GetTypeOfFile(fe_ptr->stat_struct),
                     overlay_detail, sizeof(overlay_detail));
  type_of_file = GetTypeOfFile(fe_ptr->stat_struct);
  align_name_col =
      (panel->pan_small_file_window && win == panel->pan_small_file_window);
  BuildFileRowLabel(plain_name, sizeof(plain_name), panel, fe_ptr, ' ');
  primary_name = plain_name;
  if (align_name_col || render_mode == MODE_3) {
    BuildFileRowLabel(row_label, sizeof(row_label), panel, fe_ptr,
                      type_of_file);
    primary_name = row_label;
  }
  is_tagged = PanelTags_FileIsTagged(panel, fe_ptr);
  is_active_panel = !(ctx->is_split_screen && panel != ctx->active);
  inactive_highlight_attr = A_BOLD | A_UNDERLINE;
  highlight_color_pair = UI_ROLE_SELECTION;

  if (panel->fixed_col_width > 0) {
    pos_x = x * (panel->fixed_col_width + 1);
    wmove(win, y, pos_x);

    /* Prepare Display Name */
    char display_name[PATH_LENGTH * 2 + 8];
    char compact_label[PATH_LENGTH * 2 + 8];
    /* Reserve 2 chars for Tag and spacer. */
    BuildFileRowLabel(compact_label, sizeof(compact_label), panel, fe_ptr,
                      type_of_file);
    CutFilename(display_name, compact_label, panel->fixed_col_width - 2);

    /* Set Attributes */
    int color = (hilight && is_active_panel) ? highlight_color_pair
                                             : GetFileTypeColor(ctx, fe_ptr);
    wattron(win, COLOR_PAIR(color));
    if (is_tagged)
      wattron(win, A_BOLD);
    if (hilight && !is_active_panel)
      wattron(win, inactive_highlight_attr);

    /* Draw */
    wprintw(win, "%c %s", (is_tagged) ? TAGGED_SYMBOL : ' ', display_name);

    /* Pad remaining width */
    int printed_len = 2 + StrVisualLength(display_name);
    int k;
    for (k = printed_len; k < panel->fixed_col_width; k++) {
      waddch(win, ' ');
    }

    /* Cleanup */
    if (hilight && !is_active_panel)
      wattroff(win, inactive_highlight_attr);
    if (is_tagged)
      wattroff(win, A_BOLD);
    wattroff(win, COLOR_PAIR(color));
    return;
  }

  (panel->reverse_sort) ? (justify = '+') : (justify = '-');

  if (old_cols != COLS) {
    old_cols = COLS;
    if (line_buffer)
      free(line_buffer);

    line_buffer_size = COLS + PATH_LENGTH;
    line_buffer = (char *)xmalloc(line_buffer_size);
  }
  if (line_buffer == NULL || line_buffer_size == 0)
    return;

  if (S_ISLNK(fe_ptr->stat_struct.st_mode))
    sym_link_name = &fe_ptr->name[strlen(fe_ptr->name) + 1];
  else
    sym_link_name = "";

#ifdef WITH_UTF8
#if defined(__GNUC__) && __GNUC__ >= 7
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overread"
#endif
  filename_width = panel->max_visual_filename_len +
                   (strlen(fe_ptr->name) - StrVisualLength(fe_ptr->name));
#if defined(__GNUC__) && __GNUC__ >= 7
#pragma GCC diagnostic pop
#endif
  if (S_ISLNK(fe_ptr->stat_struct.st_mode))
    linkname_width = panel->max_visual_linkname_len +
                     (strlen(sym_link_name) - StrVisualLength(sym_link_name));
#else
  filename_width = panel->max_visual_filename_len;
  linkname_width = panel->max_visual_linkname_len;
#endif
  if (uses_overlay_detail)
    filename_width = OverlayNameColumnWidth(panel, width, filename_width);

  /* Calculate starting column position (pos_x) based on column index `x` */
  switch (render_mode) {
  case MODE_1:
    if (panel->max_visual_linkname_len)
      pos_x = x * (panel->max_visual_filename_len +
                   panel->max_visual_linkname_len + 51); /* +47 + 4 = 51 */
    else
      pos_x = x * (panel->max_visual_filename_len + 47); /* +43 + 4 = 47 */
    break;
  case MODE_2:
    if (panel->max_visual_linkname_len)
      pos_x = x * (panel->max_visual_filename_len +
                   panel->max_visual_linkname_len + 43);
    else
      pos_x = x * (panel->max_visual_filename_len + 39);
    break;
  case MODE_3:
    pos_x = x * (panel->max_visual_filename_len + 3);
    break;
  case MODE_4:
    if (panel->max_visual_linkname_len)
      pos_x = x * (panel->max_visual_filename_len +
                   panel->max_visual_linkname_len + 52); /* +44 + 8 = 52 */
    else
      pos_x = x * (panel->max_visual_filename_len + 48); /* +40 + 8 = 48 */
    break;
  case MODE_5:
    pos_x = x * (panel->max_visual_userview_len + 1);
    break;
  default:
    pos_x = x;
    break;
  }

  ef_window_width = width - pos_x - 1; /* Effective width for this column */
  if (ef_window_width < 0)
    ef_window_width = 0;

  wmove(win, y, pos_x);
  base_color_pair = (hilight && ctx->highlight_full_line && is_active_panel)
                        ? highlight_color_pair
                        : GetFileTypeColor(ctx, fe_ptr);

  if (ctx->highlight_full_line) {
    /* --- RENDER METHOD 1: FULL LINE HIGHLIGHT --- */
    wattron(win, COLOR_PAIR(base_color_pair));
    if (is_tagged)
      wattron(win, A_BOLD);
    if (hilight && !is_active_panel)
      wattron(win, inactive_highlight_attr);

    /* Build the full line string */
    switch (render_mode) {
    case MODE_1:
      (void)GetAttributes(fe_ptr->stat_struct.st_mode, attributes);
      (void)CTime(fe_ptr->stat_struct.st_mtime, modify_time);
      FormatPanelSize(ctx, panel, (long long)fe_ptr->stat_struct.st_size,
                      size_text, sizeof(size_text));
      if (align_name_col) {
        (void)snprintf(format, sizeof(format),
                       "%%c %%-%ds %%10s %%3d %%11s %%16s", filename_width);
        (void)snprintf(line_buffer, line_buffer_size, format,
                       (is_tagged) ? TAGGED_SYMBOL : ' ', primary_name,
                       attributes, fe_ptr->stat_struct.st_nlink, size_text,
                       modify_time);
      } else {
        (void)snprintf(format, sizeof(format),
                       "%%c%%c%%%c%ds %%10s %%3d %%11s %%16s", justify,
                       filename_width);
        (void)snprintf(line_buffer, line_buffer_size, format,
                       (is_tagged) ? TAGGED_SYMBOL : ' ', type_of_file,
                       primary_name, attributes, fe_ptr->stat_struct.st_nlink,
                       size_text, modify_time);
      }
      break;
    case MODE_2:
      owner_name_ptr = GetDisplayPasswdName(fe_ptr->stat_struct.st_uid);
      group_name_ptr = GetDisplayGroupName(fe_ptr->stat_struct.st_gid);
      if (!owner_name_ptr) {
        snprintf(owner, sizeof(owner), "%d", (int)fe_ptr->stat_struct.st_uid);
        owner_name_ptr = owner;
      }
      if (!group_name_ptr) {
        snprintf(group, sizeof(group), "%d", (int)fe_ptr->stat_struct.st_gid);
        group_name_ptr = group;
      }
      if (align_name_col) {
        (void)snprintf(format, sizeof(format),
                       "%%c %%%c%ds %%10lld %%-12s %%-12s", justify,
                       filename_width);
        (void)snprintf(line_buffer, line_buffer_size, format,
                       (is_tagged) ? TAGGED_SYMBOL : ' ', primary_name,
                       (long long)fe_ptr->stat_struct.st_ino, owner_name_ptr,
                       group_name_ptr);
      } else {
        (void)snprintf(format, sizeof(format),
                       "%%c%%c%%%c%ds %%10lld %%-12s %%-12s", justify,
                       filename_width);
        (void)snprintf(line_buffer, line_buffer_size, format,
                       (is_tagged) ? TAGGED_SYMBOL : ' ', type_of_file,
                       primary_name, (long long)fe_ptr->stat_struct.st_ino,
                       owner_name_ptr, group_name_ptr);
      }
      break;
    case MODE_3:
      (void)snprintf(format, sizeof(format), "%%c %%%c%ds", justify,
                     filename_width);
      (void)snprintf(line_buffer, line_buffer_size, format,
                     (is_tagged) ? TAGGED_SYMBOL : ' ', primary_name);
      if (uses_overlay_detail && overlay_detail[0] != '\0') {
        (void)snprintf(line_buffer + strlen(line_buffer),
                       line_buffer_size - strlen(line_buffer), "%s",
                       overlay_detail);
      }
      break;
    case MODE_4:
      (void)CTime(fe_ptr->stat_struct.st_ctime, change_time);
      (void)CTime(fe_ptr->stat_struct.st_atime, access_time);
      if (align_name_col) {
        (void)snprintf(format, sizeof(format),
                       "%%c %%%c%ds Chg: %%16s  Acc: %%16s", justify,
                       filename_width);
        (void)snprintf(line_buffer, line_buffer_size, format,
                       (is_tagged) ? TAGGED_SYMBOL : ' ', primary_name,
                       change_time, access_time);
      } else {
        (void)snprintf(format, sizeof(format),
                       "%%c%%c%%%c%ds Chg: %%16s  Acc: %%16s", justify,
                       filename_width);
        (void)snprintf(line_buffer, line_buffer_size, format,
                       (is_tagged) ? TAGGED_SYMBOL : ' ', type_of_file,
                       primary_name, change_time, access_time);
      }
      break;
    case MODE_5:
      BuildUserFileEntry(fe_ptr, filename_width, linkname_width, is_tagged,
                         (GetProfileValue)(ctx, "USERVIEW"), 200, line_buffer);
      break;
    }

    /* Horizontal scrolling logic */
    n = StrVisualLength(line_buffer);
    if (n <= ef_window_width) {
      line_ptr = line_buffer;
    } else {
      int line_end_pos;
      if (n > (start_x + ef_window_width))
        line_ptr =
            &line_buffer[VisualPositionToBytePosition(line_buffer, start_x)];
      else
        line_ptr = &line_buffer[VisualPositionToBytePosition(
            line_buffer, n - ef_window_width)];

      if (line_ptr == NULL)
        return;
      line_end_pos = VisualPositionToBytePosition(line_ptr, ef_window_width);
      if (line_end_pos < 0)
        return;
      line_ptr[line_end_pos] = '\0';
    }
    waddstr(win, line_ptr);

    if (hilight && !is_active_panel)
      wattroff(win, inactive_highlight_attr);
    if (is_tagged)
      wattroff(win, A_BOLD);

  } else {
    /* --- RENDER METHOD 2: NAME-ONLY HIGHLIGHT --- */

    wattron(win, COLOR_PAIR(base_color_pair));
    if (is_tagged)
      wattron(win, A_BOLD);

    /* Print tag and type/spacer */
    {
      char prefix[3];
      prefix[0] = (is_tagged) ? TAGGED_SYMBOL : ' ';
      prefix[1] = (align_name_col || render_mode == MODE_3) ? ' '
                                                                   : type_of_file;
      prefix[2] = '\0';
      AddClippedAtCursor(win, prefix, width);
    }

    /* Calculate available width for name and truncate if necessary */
    int overhead = 0;
    switch (render_mode) {
    case MODE_1:
      overhead = 44;
      break;
    case MODE_2:
      overhead = 40;
      break;
    case MODE_4:
      overhead = 48;
      break;
    default:
      overhead = 0;
      break;
    }
    int max_w = width - pos_x - 3 - overhead;
    /* Prioritize filename: never truncate below 16 chars unless window is tiny
     */
    if (max_w < 16)
      max_w = 16;
    if (max_w > width - pos_x - 3)
      max_w = width - pos_x - 3;

    char display_name[PATH_LENGTH * 2 + 8];
    const char *name_text = primary_name;
    char mode3_name[PATH_LENGTH * 2 + 8];
    if (align_name_col || render_mode == MODE_3) {
      BuildFileRowLabel(mode3_name, sizeof(mode3_name), panel, fe_ptr,
                        type_of_file);
      name_text = mode3_name;
    }
    if ((int)strlen(name_text) > max_w) {
      CutFilename(display_name, name_text, max_w);
    } else {
      int copied_len =
          snprintf(display_name, sizeof(display_name), "%s", name_text);
      if (copied_len < 0) {
        display_name[0] = '\0';
      } else if ((size_t)copied_len >= sizeof(display_name)) {
        display_name[sizeof(display_name) - 1] = '\0';
      }
    }

    /* Highlight only the name */
    if (hilight) {
      if (is_active_panel)
        wattrset(win, COLOR_PAIR(highlight_color_pair));
      else
        wattron(win, inactive_highlight_attr);
    }
    AddClippedAtCursor(win, display_name, width);
    if (uses_overlay_detail && overlay_detail[0] != '\0') {
      int current_x;
      int dummy_y;
      int target_x;

      getyx(win, dummy_y, current_x);
      (void)dummy_y;
      target_x = MINIMUM(pos_x + 2 + filename_width, width - 1);
      for (int i = current_x; i < target_x; i++)
        waddch(win, ' ');
      AddClippedAtCursor(win, overlay_detail, width);
    }
    if (hilight) {
      if (is_active_panel)
        wattrset(win, COLOR_PAIR(base_color_pair));
      else
        wattroff(win, inactive_highlight_attr);
    }

    /* Print attributes for modes other than MODE_3 */
    if (render_mode != MODE_3) {
      int current_x;
      int dummy_y;
      getyx(win, dummy_y, current_x);
      (void)dummy_y;

      /* Adjusted target_x calculation to stay within bounds */
      int target_x = MINIMUM(pos_x + 2 + filename_width, width - overhead);

      /* Fill space between name and attributes */
      for (int i = current_x; i < target_x; i++)
        waddch(win, ' ');

      switch (render_mode) {
      case MODE_1:
        (void)GetAttributes(fe_ptr->stat_struct.st_mode, attributes);
        (void)CTime(fe_ptr->stat_struct.st_mtime, modify_time);
        FormatPanelSize(ctx, panel, (long long)fe_ptr->stat_struct.st_size,
                        size_text, sizeof(size_text));
        {
          char detail[PATH_LENGTH + 128];
          (void)snprintf(detail, sizeof(detail), " %10s %3d %11s %16s",
                         attributes, (int)fe_ptr->stat_struct.st_nlink,
                         size_text, modify_time);
          AddClippedAtCursor(win, detail, width);
        }
        break;
      case MODE_2:
        owner_name_ptr = GetDisplayPasswdName(fe_ptr->stat_struct.st_uid);
        group_name_ptr = GetDisplayGroupName(fe_ptr->stat_struct.st_gid);
        if (!owner_name_ptr) {
          snprintf(owner, sizeof(owner), "%d", (int)fe_ptr->stat_struct.st_uid);
          owner_name_ptr = owner;
        }
        if (!group_name_ptr) {
          snprintf(group, sizeof(group), "%d", (int)fe_ptr->stat_struct.st_gid);
          group_name_ptr = group;
        }
        {
          char detail[PATH_LENGTH + 128];
          (void)snprintf(detail, sizeof(detail), " %10lld %-12s %-12s",
                         (long long)fe_ptr->stat_struct.st_ino, owner_name_ptr,
                         group_name_ptr);
          AddClippedAtCursor(win, detail, width);
        }
        break;
      case MODE_4:
        (void)CTime(fe_ptr->stat_struct.st_ctime, change_time);
        (void)CTime(fe_ptr->stat_struct.st_atime, access_time);
        {
          char detail[PATH_LENGTH + 128];
          (void)snprintf(detail, sizeof(detail), " Chg: %16s  Acc: %16s",
                         change_time, access_time);
          AddClippedAtCursor(win, detail, width);
        }
        break;
      case MODE_5:
        break;
      }
    }

    if (is_tagged)
      wattroff(win, A_BOLD);
  }
  wattroff(win, COLOR_PAIR(base_color_pair));
}

void DisplayFiles(ViewContext *ctx, YtreeNovaPanel *panel, const DirEntry *de_ptr,
                  int start_file_no, int hilight_no, int start_x, WINDOW *win) {
  int x, y, p_x, p_y, j;
  BOOL show_empty_label;

  if (!ctx || !panel || !panel->vol || !win)
    return;
  int height;

  height = getmaxy(win);

#ifdef COLOR_SUPPORT
  WbkgdSet(ctx, win, COLOR_PAIR(UI_ROLE_DYNAMIC_TEXT));
#endif
  werase(win);

  show_empty_label = (panel->file_entry_list == NULL || panel->file_count == 0 ||
                      start_file_no >= (int)panel->file_count);
  if (show_empty_label) {
    const int first_filename_col = 2;
    const char *empty_label = "No files";
    if (de_ptr && de_ptr->access_denied) {
      empty_label = "Permission Denied!";
    } else if (de_ptr && (de_ptr->unlogged_flag || de_ptr->not_scanned)) {
      empty_label = "Unlogged";
    }
    mvwaddstr(win, 0, first_filename_col, empty_label);
  }

  if (!panel->file_entry_list || panel->file_count == 0) {
    wnoutrefresh(win);
    return;
  }

  j = start_file_no;
  p_x = -1;
  p_y = 0;
  for (x = 0; x < panel->max_column; x++) {
    for (y = 0; y < height; y++) {
      if ((unsigned)j < panel->file_count) {
        if (j == hilight_no) {
          p_x = x;
          p_y = y;
        } else {
          PrintFileEntry(ctx, panel, j, y, x, FALSE, start_x, win);
        }
      }
      j++;
    }
  }

  if (p_x >= 0)
    PrintFileEntry(ctx, panel, hilight_no, p_y, p_x, TRUE, start_x, win);

  wnoutrefresh(win);
}
