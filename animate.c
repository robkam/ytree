/***************************************************************************
 *
 * animate.c
 * Animation routines for the directory scan phase
 *
 ***************************************************************************/

#include "ytree.h"

#define WARP_STARS 100

typedef struct {
    float x, y, z;
} WarpStar;

static WarpStar stars[WARP_STARS];
static BOOL is_initialized = FALSE;

static void reset_star(WarpStar *s) {
    /* Randomize -1.0 to 1.0 */
    s->x = ((float)rand() / RAND_MAX * 2.0f) - 1.0f;
    s->y = ((float)rand() / RAND_MAX * 2.0f) - 1.0f;
    /* Start deep in the back */
    s->z = 3.0f + ((float)rand() / RAND_MAX);
}

void InitAnimation(void) {
    int i;

    if (is_initialized) return;

    srand(time(NULL));

    for(i = 0; i < WARP_STARS; i++) {
        reset_star(&stars[i]);
        /* Distribute initial Z to fill the volume */
        stars[i].z = ((float)rand() / RAND_MAX) * 3.0f;
    }
    is_initialized = TRUE;
}

void StopAnimation(void) {
    is_initialized = FALSE;
}

void DrawAnimationStep(WINDOW *win) {
    int i, scr_w, scr_h;

    if (!is_initialized) InitAnimation();

    getmaxyx(win, scr_h, scr_w);
    werase(win);

    /* Optional: Draw status text */
    mvwaddstr(win, scr_h - 2, 2, "Scanning directory structure...");
    mvwaddstr(win, scr_h - 1, 2, "Please wait...");

    for(i = 0; i < WARP_STARS; i++) {
        /* Move star closer */
        stars[i].z -= 0.08f;

        /* Reset if behind camera */
        if(stars[i].z <= 0.0f) reset_star(&stars[i]);

        /* 3D Projection */
        int sx = (int)((stars[i].x / stars[i].z) * scr_w + (scr_w / 2));
        int sy = (int)((stars[i].y / stars[i].z) * scr_h + (scr_h / 2));

        /* Draw if inside window */
        if(sx >= 0 && sx < scr_w && sy >= 0 && sy < scr_h) {
            char c = '.';
            if(stars[i].z < 2.0f) c = '+';
            if(stars[i].z < 1.0f) c = '*';
            if(stars[i].z < 0.5f) c = '#';

#ifdef COLOR_SUPPORT
            wattron(win, COLOR_PAIR(CPAIR_DIR) | A_BOLD);
#endif
            mvwaddch(win, sy, sx, c);
#ifdef COLOR_SUPPORT
            wattroff(win, COLOR_PAIR(CPAIR_DIR) | A_BOLD);
#endif
        } else {
            /* Reset if off-screen but close to camera to keep density high */
            if(stars[i].z < 1.5f) reset_star(&stars[i]);
        }
    }
    wnoutrefresh(win);
}