#include "render.h"
#include "font.h"
#include <string.h>

void render_init(uint16_t *framebuffer) {
    if (framebuffer) {
        render_clear_screen(framebuffer);
    }
}

void render_clear_screen(uint16_t *framebuffer) {
    if (!framebuffer) return;
    
    // Fill with background color
    for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
        framebuffer[i] = COLOR_BG;
    }
}

void render_fill_rect(uint16_t *framebuffer, int x, int y, int width, int height, uint16_t color) {
    if (!framebuffer) return;
    
    for (int py = y; py < y + height && py < SCREEN_HEIGHT; py++) {
        for (int px = x; px < x + width && px < SCREEN_WIDTH; px++) {
            if (px >= 0 && py >= 0) {
                framebuffer[py * SCREEN_WIDTH + px] = color;
            }
        }
    }
}

void render_header(uint16_t *framebuffer, const char *title) {
    if (!framebuffer || !title) return;
    
    // Draw header background
    render_fill_rect(framebuffer, 0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_BG);
    
    // Draw header text
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, PADDING, 8, title, COLOR_HEADER);
}

void render_legend(uint16_t *framebuffer) {
    if (!framebuffer) return;
    
    // No legend - clean interface
}

void render_menu_item(uint16_t *framebuffer, int index, const char *name, int is_dir, 
                     int is_selected, int scroll_offset) {
    if (!framebuffer || !name) return;
    
    int visible_index = index - scroll_offset;
    if (visible_index < 0 || visible_index >= VISIBLE_ENTRIES) return;
    
    int y = START_Y + (visible_index * ITEM_HEIGHT);
    
    if (is_selected) {
        // Calculate text width for dynamic pillbox
        int text_width = strlen(name) * FONT_CHAR_SPACING;
        
        // Draw selection background (pill shape) sized to text
        render_fill_rect(framebuffer, PADDING - 4, y - 2, 
                        text_width + 8, ITEM_HEIGHT - 4, COLOR_SELECT_BG);
        
        // Draw text in selection color
        font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, PADDING, y, name, COLOR_SELECT_TEXT);
    } else {
        // Draw normal text
        uint16_t text_color = is_dir ? COLOR_FOLDER : COLOR_TEXT;
        font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, PADDING, y, name, text_color);
    }
}