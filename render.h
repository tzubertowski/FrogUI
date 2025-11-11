#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>

// Screen dimensions
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// Colors (RGB565) - MinUI Exact Style
#define COLOR_BG        0x0000  // Black background
#define COLOR_TEXT      0xFFFF  // White text
#define COLOR_SELECT_BG 0xFFFF  // White selection background (pill)
#define COLOR_SELECT_TEXT 0x0000  // Black text when selected
#define COLOR_HEADER    0xCE59  // Light gray for header
#define COLOR_FOLDER    0xFFFF  // White for folders (same as text)

// MinUI Layout Constants
#define HEADER_HEIGHT 30
#define ITEM_HEIGHT 24
#define PADDING 16
#define START_Y 40
#define VISIBLE_ENTRIES 7

// Initialize rendering system
void render_init(uint16_t *framebuffer);

// Clear screen with background color
void render_clear_screen(uint16_t *framebuffer);

// Draw a filled rectangle
void render_fill_rect(uint16_t *framebuffer, int x, int y, int width, int height, uint16_t color);

// Draw a rounded rectangle (pill shape)
void render_rounded_rect(uint16_t *framebuffer, int x, int y, int width, int height, int radius, uint16_t color);

// Draw menu header with title
void render_header(uint16_t *framebuffer, const char *title);

// Draw menu legend at bottom
void render_legend(uint16_t *framebuffer);

// Draw a menu item (file or folder)
void render_menu_item(uint16_t *framebuffer, int index, const char *name, int is_dir, 
                     int is_selected, int scroll_offset);

#endif // RENDER_H