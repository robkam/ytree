/***************************************************************************
 *
 * src/ui/dialog.c
 * Centralized Dialog Manager and Window Stack
 *
 ***************************************************************************/

#include "ytree.h"
#include "ytree_dialog.h"

static DialogCtx dialog_stack[MAX_DIALOG_STACK];
static int dialog_count = 0;

void UI_Dialog_Init(void) { dialog_count = 0; }

int UI_Dialog_Push(WINDOW *win, UITier tier) {
  if (dialog_count >= MAX_DIALOG_STACK) {
    return -1; /* Stack full */
  }
  dialog_stack[dialog_count].win = win;
  dialog_stack[dialog_count].tier = tier;
  dialog_count++;
  return 0;
}

int UI_Dialog_Pop(WINDOW *win) {
  if (dialog_count <= 0)
    return -1;

  /* Typically we pop the top, but we allow specifying the win to be safe */
  if (dialog_stack[dialog_count - 1].win == win) {
    dialog_count--;
    return 0;
  }

  /* Find it and remove */
  int i, found = -1;
  for (i = 0; i < dialog_count; i++) {
    if (dialog_stack[i].win == win) {
      found = i;
      break;
    }
  }

  if (found != -1) {
    for (i = found; i < dialog_count - 1; i++) {
      dialog_stack[i] = dialog_stack[i + 1];
    }
    dialog_count--;
    return 0;
  }
  return -1;
}

void UI_Dialog_RefreshAll(void) {
  /* Base layout */
  touchwin(stdscr);
  wnoutrefresh(stdscr);

  /* Active panel windows */
  if (ActivePanel) {
    if (ActivePanel->pan_dir_window) {
      touchwin(ActivePanel->pan_dir_window);
      wnoutrefresh(ActivePanel->pan_dir_window);
    }
    if (ActivePanel->pan_file_window) {
      touchwin(ActivePanel->pan_file_window);
      wnoutrefresh(ActivePanel->pan_file_window);
    }
  }

  /* Preview window if active */
  if (GlobalView && GlobalView->preview_mode &&
      GlobalView->ctx_preview_window) {
    touchwin(GlobalView->ctx_preview_window);
    wnoutrefresh(GlobalView->ctx_preview_window);
  }
  /* Note: Inactive panel usually handled by main display logic, but can be
   * added here if needed */
  RenderInactivePanel(ActivePanel == LeftPanel ? RightPanel : LeftPanel);

  /* Finally, stack */
  int i;
  for (i = 0; i < dialog_count; i++) {
    if (dialog_stack[i].win) {
      touchwin(dialog_stack[i].win);
      wnoutrefresh(dialog_stack[i].win);
    }
  }
}

void UI_Dialog_Close(WINDOW *win) {
  if (!win)
    return;
  UI_Dialog_Pop(win);
  delwin(win);
  /* Flicker free redraw */
  UI_Dialog_RefreshAll();
  doupdate();
}
