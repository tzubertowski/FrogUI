#ifndef RENDER_H
#define RENDER_H

#include <stdint.h>
#include <stddef.h>
#include "theme.h"

// Screen dimensions
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240

// Colors are now provided by the theme system
// Legacy defines for backwards compatibility - will be removed
#define COLOR_BG        theme_bg()
#define COLOR_TEXT      theme_text()
#define COLOR_SELECT_BG theme_select_bg()
#define COLOR_SELECT_TEXT theme_select_text()
#define COLOR_HEADER    theme_header()
#define COLOR_FOLDER    theme_folder()
#define COLOR_LEGEND    theme_legend()
#define COLOR_LEGEND_BG theme_legend_bg()
#define COLOR_DISABLED  theme_disabled()

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
#define MAX_FILENAME_DISPLAY_LEN 20 // Max length for selected item (with scrolling)
#define MAX_UNSELECTED_DISPLAY_LEN 10 // Max length for unselected items (to avoid thumbnail overlap)
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

// Draw a text pillbox with proper padding (unified method)
void render_text_pillbox(uint16_t *framebuffer, int x, int y, const char *text, 
                        uint16_t bg_color, uint16_t text_color, int padding);

// Draw menu header with title
void render_header(uint16_t *framebuffer, const char *title);

// Draw menu legend at bottom
void render_legend(uint16_t *framebuffer);

// Draw a menu item (file or folder)
void render_menu_item(uint16_t *framebuffer, int index, const char *name, int is_dir,
                     int is_selected, int scroll_offset, int is_favorited);

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