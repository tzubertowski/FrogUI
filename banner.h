#pragma once
#include <stdint.h>

/* Windows-style wallpaper fit modes. */
enum { BANNER_FIT_FILL, BANNER_FIT_FIT, BANNER_FIT_STRETCH, BANNER_FIT_CENTER, BANNER_FIT_TILE };
void banner_load(const char *path);                 /* stretch (system art) */
void banner_load_fit(const char *path, int mode, uint16_t bg);  /* wallpaper */
void banner_clear(void);
void banner_render(uint16_t *framebuffer);
/* Draw the currently loaded artwork into a contained card without decoding it
 * again. Used by the pastel system-list layouts. */
void banner_draw_card(uint16_t *framebuffer, int x, int y, int w, int h);
int  banner_is_loaded(void);
int  banner_is_animating(void);
void banner_set_anim(int enabled);
void banner_set_dim(int percent);
void banner_fill_region(uint16_t *fb, int x, int y, int w, int h, uint16_t fallback);
