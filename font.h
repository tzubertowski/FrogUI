#ifndef FONT_H
#define FONT_H

#include <stdint.h>

// Initialize font system
void font_init(void);

// Load font from settings (call when font setting changes)
void font_load_from_settings(const char *font_name);

// Draw a single character at position (x, y) with given color
void font_draw_char(uint16_t *framebuffer, int screen_width, int screen_height, 
                   int x, int y, char c, uint16_t color);

// Draw a text string at position (x, y) with given color
void font_draw_text(uint16_t *framebuffer, int screen_width, int screen_height,
                   int x, int y, const char *text, uint16_t color);

// Get font character width/height
#define FONT_CHAR_WIDTH 18
#define FONT_CHAR_HEIGHT 16
#define FONT_CHAR_SPACING 13

#endif // FONT_H