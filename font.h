#ifndef FONT_H
#define FONT_H

#include <stdint.h>

// Initialize font system
void font_init(void);

// Draw a single character at position (x, y) with given color
void font_draw_char(uint16_t *framebuffer, int screen_width, int screen_height, 
                   int x, int y, char c, uint16_t color);

// Draw a text string at position (x, y) with given color
void font_draw_text(uint16_t *framebuffer, int screen_width, int screen_height,
                   int x, int y, const char *text, uint16_t color);

// Get font character width/height
#define FONT_CHAR_WIDTH 16
#define FONT_CHAR_HEIGHT 16
#define FONT_CHAR_SPACING 10

#endif // FONT_H