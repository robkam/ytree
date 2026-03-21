/***************************************************************************
 *
 * src/ui/animate.c
 * Animation routines for the directory scan phase
 *
 ***************************************************************************/

#include "ytree.h"

#define WARP_STARS 50

typedef struct {
  float x, y, z;
} WarpStar;

static void reset_star(WarpStar *s) {
  /* Randomize -1.0 to 1.0 */
  s->x = ((float)rand() / RAND_MAX * 2.0f) - 1.0f;
  s->y = ((float)rand() / RAND_MAX * 2.0f) - 1.0f;
  /* Start deep in the back */
  s->z = 3.0f + ((float)rand() / RAND_MAX);
}

void InitAnimation(ViewContext *ctx) {
  int i;
  WarpStar *stars;

  if (ctx->anim_is_initialized)
    return;

  if (!ctx->anim_stars) {
    ctx->anim_stars = xmalloc(WARP_STARS * sizeof(WarpStar));
  }
  stars = (WarpStar *)ctx->anim_stars;

  srand(time(NULL));

  for (i = 0; i < WARP_STARS; i++) {
    reset_star(&stars[i]);
    /* Distribute initial Z to fill the volume */
    stars[i].z = ((float)rand() / RAND_MAX) * 3.0f;
  }
  ctx->anim_is_initialized = TRUE;
}

void StopAnimation(ViewContext *ctx) { ctx->anim_is_initialized = FALSE; }

void DrawAnimationStep(ViewContext *ctx, WINDOW *win) {
  int i, scr_w, scr_h;
  WarpStar *stars;

  if (!ctx->anim_is_initialized)
    InitAnimation(ctx);

  stars = (WarpStar *)ctx->anim_stars;

  getmaxyx(win, scr_h, scr_w);
  werase(win);

  for (i = 0; i < WARP_STARS; i++) {
    /* Move star closer */
    stars[i].z -= 0.08f;

    /* Reset if behind camera */
    if (stars[i].z <= 0.0f)
      reset_star(&stars[i]);

    /* 3D Projection */
    int sx = (int)((stars[i].x / stars[i].z) * scr_w + (scr_w / 2));
    int sy = (int)((stars[i].y / stars[i].z) * scr_h + (scr_h / 2));

    /* Draw if inside window */
    if (sx >= 0 && sx < scr_w && sy >= 0 && sy < scr_h) {
      char c = '.';
      if (stars[i].z < 2.0f)
        c = '+';
      if (stars[i].z < 1.0f)
        c = '*';
      if (stars[i].z < 0.5f)
        c = '#';

#ifdef COLOR_SUPPORT
      wattron(win, COLOR_PAIR(CPAIR_DIR) | A_BOLD);
#endif
      mvwaddch(win, sy, sx, c);
#ifdef COLOR_SUPPORT
      wattroff(win, COLOR_PAIR(CPAIR_DIR) | A_BOLD);
#endif
    } else {
      /* Reset if off-screen but close to camera to keep density high */
      if (stars[i].z < 1.5f)
        reset_star(&stars[i]);
    }
  }
  wnoutrefresh(win);
}

/* New Activity Spinner Implementation */
void DrawSpinner(ViewContext *ctx) {
  if (LINES > 0 && COLS > 0) {
    static char spin_chars[] = "|/-\\";
    /* Draw at bottom right (menu line) */
    mvaddch(LINES - 2, COLS - 2, spin_chars[ctx->spin_counter++ % 4]);

    /* Force immediate update to screen */
    doupdate();
  }
}
