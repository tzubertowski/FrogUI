#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>
#include <stddef.h>

// Screen dimensions
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// Colors (RGB565) - MinUI Exact Style
#define COLOR_BG        0x0000  // Black background
#define COLOR_TEXT      0xFFFF  // White text
#define COLOR_SELECT_BG 0xFFFF  // White selection background (pill)
#define COLOR_SELECT_TEXT 0x0000  // Black text when selected
#define COLOR_HEADER    0x8410  // Better grey for headers/titles (more like MinUI)
#define COLOR_FOLDER    0xFFFF  // White for folders (same as text)
#define COLOR_LEGEND    0xFFFF  // White text for legends (readable on dark bg)
#define COLOR_LEGEND_BG 0x2104  // Better dark grey background for legend pills
#define COLOR_DISABLED  0x8410  // Better grey for unselectable options

// MinUI Layout Constants
#define HEADER_HEIGHT 30
#define ITEM_HEIGHT 24
#define PADDING 16
#define START_Y 40
#define VISIBLE_ENTRIES 7  // 7 items as requested

// Thumbnail layout - rendered as BACKGROUND on the right side
#define THUMBNAIL_AREA_X 160    // Start thumbnail area 
#define THUMBNAIL_AREA_Y 40     // Start from header
#define THUMBNAIL_MAX_WIDTH 160 // Full width to screen edge (320-160=160) 
#define THUMBNAIL_MAX_HEIGHT 200 // Support up to 200px height as requested

// Text scrolling for filenames
#define MAX_FILENAME_DISPLAY_LEN 20 // Limit filename length to prevent overlap
#define SCROLL_DELAY_FRAMES 60      // Delay before scrolling starts (1 second at 60fps)
#define SCROLL_SPEED_FRAMES 8       // Frames between scroll steps (slower = easier to read)

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

// Thumbnail functions
typedef struct {
    uint16_t *data;
    int width;
    int height;
} Thumbnail;

// Load thumbnail from PNG file
int load_thumbnail(const char *png_path, Thumbnail *thumb);

// Load raw RGB565 file (fallback)
int load_raw_rgb565(const char *path, Thumbnail *thumb);

// Free thumbnail memory
void free_thumbnail(Thumbnail *thumb);

// Draw thumbnail in the thumbnail area
void render_thumbnail(uint16_t *framebuffer, const Thumbnail *thumb);

// Get thumbnail path for a given game file
void get_thumbnail_path(const char *game_path, char *thumb_path, size_t thumb_path_size);

#endif // RENDER_H