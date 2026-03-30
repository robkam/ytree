/***************************************************************************
 *
 * src/ui/view_preview.c
 * F7 Preview Renderer implementation
 *
 ***************************************************************************/

#include "ytree_fs.h"
#include "ytree_ui.h"
#include <errno.h>
#include <sys/stat.h>
#include <unistd.h>

static char last_preview_archive[PATH_LENGTH] = "";
static char last_preview_internal[PATH_LENGTH] = "";

static int append_bounded(char *dst, size_t dst_size, const char *src) {
  size_t used;
  size_t src_len;

  if (dst_size == 0)
    return -1;

  used = strlen(dst);
  if (used >= dst_size)
    return -1;

  src_len = strlen(src);
  if (src_len >= (dst_size - used)) {
    size_t copy_len = dst_size - used - 1;
    if (copy_len > 0)
      memcpy(dst + used, src, copy_len);
    dst[dst_size - 1] = '\0';
    return -1;
  }

  memcpy(dst + used, src, src_len + 1);
  return 0;
}

void RenderFilePreview(ViewContext *ctx, WINDOW *win, char *filename,
                       long *line_offset_ptr, int col_offset) {
  int fd;
  char buf[1024];
  ssize_t nread;
  BOOL is_hex = FALSE;
  int i;
  int max_y, max_x;
  long line_offset = *line_offset_ptr;

  if (!win || !filename)
    return;

  getmaxyx(win, max_y, max_x);
  werase(win);

  fd = open(filename, O_RDONLY);
  if (fd == -1) {
    mvwprintw(win, 0, 0, "Error opening file:");
    mvwprintw(win, 1, 0, "%.*s", max_x, strerror(errno));
    wnoutrefresh(win);
    return;
  }

  /* Detect Type: Check first 1KB for null bytes */
  nread = read(fd, buf, sizeof(buf));
  if (nread > 0) {
    for (i = 0; i < nread; i++) {
      if (buf[i] == '\0') {
        is_hex = TRUE;
        break;
      }
    }
  }

  /* Reset file position */
  lseek(fd, 0, SEEK_SET);

  if (is_hex) {
    /* Check Bounds for Hex Mode */
    struct stat st;
    if (fstat(fd, &st) != -1) {
      /* If size is 0, line_offset must be 0. If size > 0, calculate pages. */
      long max_offset = (st.st_size > 0) ? (st.st_size / 16) : 0;

      /* If requested offset is beyond EOF (or start of last partial page),
       * clamp it. */
      /* Actually, let's allow scrolling to the very last partial line. */
      if (line_offset > max_offset) {
        line_offset = max_offset;
        *line_offset_ptr = line_offset;
      }
    }

    /* Hex Mode */
    unsigned char hexbuf[16];
    long current_offset = line_offset * 16;
    int row = 0;

    if (lseek(fd, current_offset, SEEK_SET) == -1) {
      mvwprintw(win, 0, 0, "Seek error");
      close(fd);
      wnoutrefresh(win);
      return;
    }

    while (row < max_y) {
      nread = read(fd, hexbuf, 16);
      if (nread <= 0)
        break;

      char hex_part[64] = "";
      char asc_part[17] = "";
      int hex_overflow = 0;

      /* Build Hex and ASCII Parts */
      for (i = 0; i < 16; i++) {
        if (i < nread) {
          char tmp[4];
          if (snprintf(tmp, sizeof(tmp), "%02X ", hexbuf[i]) < 0) {
            hex_overflow = 1;
          }
          if (!hex_overflow &&
              append_bounded(hex_part, sizeof(hex_part), tmp) != 0) {
            hex_overflow = 1;
          }
          if (isprint(hexbuf[i]))
            asc_part[i] = hexbuf[i];
          else
            asc_part[i] = '.';
        } else {
          if (!hex_overflow &&
              append_bounded(hex_part, sizeof(hex_part), "   ") != 0) {
            hex_overflow = 1;
          }
          asc_part[i] = ' ';
        }
      }
      asc_part[16] = '\0';

      /* Format: Offset (8) + 2 spaces + Hex (48) + 1 space + ASCII (16) */
      mvwprintw(win, row, 0, "%08lX  %s %s", current_offset, hex_part,
                asc_part);

      current_offset += 16;
      row++;
    }

  } else {
    /* Text Mode */
    FILE *fp = fdopen(fd, "r");
    if (!fp) {
      close(fd);
      return;
    }

    /* Bounds Check Logic for Text Mode */
    if (line_offset > 0) {
      char skip_buf[4096];
      long skipped = 0;

      /* Fast-forward to validate line_offset */
      while (skipped < line_offset) {
        if (!fgets(skip_buf, sizeof(skip_buf), fp)) {
          /* EOF reached before target offset. 'skipped' is total lines. */
          /* Clamp to valid start position (end - window_height) */
          long new_start = MAXIMUM(0, skipped - max_y + 1);

          /* Update pointers */
          *line_offset_ptr = new_start;
          line_offset = new_start;

          /* Restart file stream for drawing */
          rewind(fp);
          break;
        }

        size_t len = strlen(skip_buf);
        if (len == 0)
          break;

        if (skip_buf[len - 1] == '\n') {
          skipped++;
        } else {
          /* Line too long (partial read), consume rest */
          int c;
          while ((c = fgetc(fp)) != '\n' && c != EOF)
            ;
          skipped++;
        }
      }

      /* If loop completed naturally, file pointer is at correct position. */
      /* If clamped/rewound, loop broke and we are at start, ready to skip to
       * new offset. */
    }

    char line_buf[4096];
    long skipped = 0;

    /* Skip lines (either original or corrected offset) */
    /* Only need to skip if we rewound or started from 0 */
    if (ftell(fp) == 0 && line_offset > 0) {
      while (skipped < line_offset) {
        if (!fgets(line_buf, sizeof(line_buf), fp))
          break;
        size_t len = strlen(line_buf);
        if (len == 0)
          break;
        if (line_buf[len - 1] == '\n') {
          skipped++;
        } else {
          int c;
          while ((c = fgetc(fp)) != '\n' && c != EOF)
            ;
          skipped++;
        }
      }
    }

    /* Render */
    int row = 0;
    while (row < max_y && fgets(line_buf, sizeof(line_buf), fp)) {
      /* Remove newline */
      size_t len = strlen(line_buf);
      if (len > 0 && line_buf[len - 1] == '\n')
        line_buf[len - 1] = '\0';

      /* Handle col_offset */
      char *disp_ptr = line_buf;
      if ((int)strlen(line_buf) > col_offset) {
        disp_ptr += col_offset;
      } else {
        disp_ptr = "";
      }

      wmove(win, row, 0);
      int remaining_width = max_x;

      if (ctx->global_search_term[0] != '\0') {
        char *ptr = disp_ptr;
        char *match;
        size_t search_len = strlen(ctx->global_search_term);

        /* Simple highlighting logic */
        while (remaining_width > 0 && *ptr) {
          match = strcasestr(ptr, ctx->global_search_term);
          if (match) {
            /* Print text before match */
            int pre_len = match - ptr;
            if (pre_len > remaining_width)
              pre_len = remaining_width;

            if (pre_len > 0) {
              waddnstr(win, ptr, pre_len);
              remaining_width -= pre_len;
            }

            if (remaining_width <= 0)
              break;

            /* Print Match */
            int match_len = (int)search_len;
            if (match_len > remaining_width)
              match_len = remaining_width;

#ifdef COLOR_SUPPORT
            if (ctx->color_enabled) {
              wattron(win, COLOR_PAIR(CPAIR_HIGLOBAL));
              waddnstr(win, match, match_len);
              wattroff(win, COLOR_PAIR(CPAIR_HIGLOBAL));
            } else
#endif
            {
              wattron(win, A_REVERSE);
              waddnstr(win, match, match_len);
              wattroff(win, A_REVERSE);
            }

            remaining_width -= match_len;
            ptr = match + match_len;
          } else {
            /* No more matches, print rest */
            waddnstr(win, ptr, remaining_width);
            break;
          }
        }
      } else {
        waddnstr(win, disp_ptr, remaining_width);
      }

      row++;
    }
    fclose(fp); /* Closes fd */
    wnoutrefresh(win);
    return;
  }

  close(fd);
  wnoutrefresh(win);
}

static int PreviewProgressCallback(int status, const char *msg,
                                   void *user_data) {
  ViewContext *ctx = (ViewContext *)user_data;
  (void)msg;

  if (status == ARCHIVE_STATUS_PROGRESS) {
    if (ctx)
      DrawSpinner(ctx);
    if (EscapeKeyPressed()) {
      return ARCHIVE_CB_ABORT;
    }
  }
  return ARCHIVE_CB_CONTINUE;
}

void RenderArchivePreview(ViewContext *ctx, WINDOW *win,
                          const char *archive_path, const char *internal_path,
                          long *line_offset_ptr) {
  char cache_file[] = "/tmp/ytree_preview.cache";

  if (strcmp(archive_path, last_preview_archive) != 0 ||
      strcmp(internal_path, last_preview_internal) != 0) {

    /* New file requested, extract it */
    unlink(cache_file);

    /* Assuming ExtractArchiveNode is available via ytree.h */
    if (ExtractArchiveNode(archive_path, internal_path, cache_file,
                           PreviewProgressCallback, ctx) == 0) {
      /* Update Cache Keys */
      strncpy(last_preview_archive, archive_path, PATH_LENGTH - 1);
      last_preview_archive[PATH_LENGTH - 1] = '\0';
      strncpy(last_preview_internal, internal_path, PATH_LENGTH - 1);
      last_preview_internal[PATH_LENGTH - 1] = '\0';
    } else {
      wclear(win);
      mvwprintw(win, 0, 0, "Preview extraction failed.");
      wnoutrefresh(win);
      /* Invalidate cache */
      last_preview_archive[0] = '\0';
      return;
    }
  }

  RenderFilePreview(ctx, win, cache_file, line_offset_ptr, 0);
}
