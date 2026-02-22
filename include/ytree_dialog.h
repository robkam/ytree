/***************************************************************************
 * include/ytree_dialog.h
 * UI Dialog Management Header
 *
 * Contains declarations for the UI dialog stack and core API.
 ***************************************************************************/

#ifndef YTREE_DIALOG_H
#define YTREE_DIALOG_H

#include "ytree_defs.h"
#include <ncurses.h>

/* Dialog stack limits */
#define MAX_DIALOG_STACK 16

/* UI Tiers as per architecture spec */
typedef enum {
  UI_TIER_FOOTER = 1,     /* Tier 1: Prompts at bottom (AS:, TO:, etc) */
  UI_TIER_POPOVER = 2,    /* Tier 2: Left-aligned boxes (History) */
  UI_TIER_MODAL = 3,      /* Tier 3: Centered modales (Vol Menu, Alerts) */
  UI_TIER_INTERACTIVE = 4 /* Tier 4: Interactive widgets (F2 Selector) */
} UITier;

/* Dialog context */
typedef struct {
  WINDOW *win;
  UITier tier;
} DialogCtx;

/* Core API */
void UI_Dialog_Init(void);
int UI_Dialog_Push(WINDOW *win, UITier tier);
int UI_Dialog_Pop(WINDOW *win);
void UI_Dialog_RefreshAll(void);

/* Helpers to handle flicker free update */
void UI_Dialog_Close(WINDOW *win);

#endif /* YTREE_DIALOG_H */
