/***************************************************************************
 *
 * src/ui/nav_utils.c
 * Generic list navigation logic
 *
 ***************************************************************************/

#include "ytree_ui.h"

/*
 * Nav_MoveDown
 * Moves the cursor down by 'step' positions.
 * - If possible, moves the cursor within the current page.
 * - If the cursor reaches the bottom of the page, increments the offset
 * (scrolls).
 * - Stops at the last item.
 */
void Nav_MoveDown(int *cursor, int *offset, int total_items, int page_height,
                  int step) {
  int i;

  if (total_items <= 0)
    return;

  for (i = 0; i < step; i++) {
    /* Check if we are already at the last item */
    if ((*offset + *cursor + 1) >= total_items) {
      break;
    }

    /* If cursor is not at the bottom of the visible page, move cursor down */
    if (*cursor < (page_height - 1)) {
      (*cursor)++;
    }
    /* Cursor is at bottom, scroll offset down */
    else {
      (*offset)++;
    }
  }
}

/*
 * Nav_MoveUp
 * Moves the cursor up by 1 position.
 * - If possible, moves the cursor within the current page.
 * - If the cursor is at the top of the page, decrements the offset (scrolls).
 * - Stops at the first item (0).
 */
void Nav_MoveUp(int *cursor, int *offset) {
  if (*cursor > 0) {
    (*cursor)--;
  } else if (*offset > 0) {
    (*offset)--;
  }
}

/*
 * Nav_PageDown
 * Jumps down by 'page_height'.
 * - Scrolls the view.
 * - Adjusts cursor/offset to ensure they remain valid.
 */
void Nav_PageDown(int *cursor, int *offset, int total_items, int page_height) {
  int new_offset;
  int max_offset;
  int step;

  if (total_items <= 0 || page_height <= 0)
    return;

  step = page_height - 1;
  if (step < 1)
    step = 1;

  /* Keep row/line stable and only advance page offset. */
  new_offset = *offset + step;

  /* Calculate the maximum valid offset */
  if (total_items > page_height) {
    max_offset = total_items - page_height;
  } else {
    max_offset = 0;
  }

  /* Apply new offset, clamped to max */
  if (new_offset > max_offset)
    new_offset = max_offset;
  if (new_offset < 0)
    new_offset = 0;
  *offset = new_offset;

  /* Ensure cursor is within page bounds (sanity check) */
  if (*cursor >= page_height)
    *cursor = page_height - 1;
  if (*cursor < 0)
    *cursor = 0;

  /* Keep selection valid for short lists. */
  if ((*offset + *cursor) >= total_items) {
    *cursor = total_items - 1 - *offset;
    if (*cursor < 0)
      *cursor = 0;
  }
}

/*
 * Nav_PageUp
 * Jumps up by 'page_height'.
 * - Scrolls the view.
 */
void Nav_PageUp(int *cursor, int *offset, int page_height) {
  int new_offset;
  int step;

  if (page_height <= 0)
    return;

  step = page_height - 1;
  if (step < 1)
    step = 1;

  /* Keep row/line stable and only rewind page offset. */
  new_offset = *offset - step;

  if (new_offset < 0) {
    *offset = 0;
  } else {
    *offset = new_offset;
  }

  /* Ensure cursor is within visible page bounds. */
  if (*cursor >= page_height)
    *cursor = page_height - 1;
  if (*cursor < 0)
    *cursor = 0;
}

/*
 * Nav_Home
 * Jumps to the first item.
 */
void Nav_Home(int *cursor, int *offset) {
  *cursor = 0;
  *offset = 0;
}

/*
 * Nav_End
 * Jumps to the last item.
 * - Adjusts offset such that the last item is at the bottom of the page
 *   (or top if total items < page height).
 */
void Nav_End(int *cursor, int *offset, int total_items, int page_height) {
  if (total_items <= 0) {
    *cursor = 0;
    *offset = 0;
    return;
  }

  if (total_items <= page_height) {
    /* All items fit on screen */
    *offset = 0;
    *cursor = total_items - 1;
  } else {
    /* Scroll so last item is at bottom */
    *offset = total_items - page_height;
    *cursor = page_height - 1;
  }
}
