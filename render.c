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

void render_rounded_rect(uint16_t *framebuffer, int x, int y, int width, int height, int radius, uint16_t color) {
    if (!framebuffer) return;
    
    // Draw main body (excluding corners)
    render_fill_rect(framebuffer, x + radius, y, width - 2 * radius, height, color);
    render_fill_rect(framebuffer, x, y + radius, width, height - 2 * radius, color);
    
    // Draw rounded corners using circle approximation
    for (int corner_y = 0; corner_y < radius; corner_y++) {
        for (int corner_x = 0; corner_x < radius; corner_x++) {
            int dx = radius - corner_x;
            int dy = radius - corner_y;
            int dist_sq = dx * dx + dy * dy;
            int radius_sq = radius * radius;
            
            if (dist_sq <= radius_sq) {
                // Top-left corner
                int px = x + corner_x;
                int py = y + corner_y;
                if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                    framebuffer[py * SCREEN_WIDTH + px] = color;
                }
                
                // Top-right corner
                px = x + width - 1 - corner_x;
                py = y + corner_y;
                if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                    framebuffer[py * SCREEN_WIDTH + px] = color;
                }
                
                // Bottom-left corner
                px = x + corner_x;
                py = y + height - 1 - corner_y;
                if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                    framebuffer[py * SCREEN_WIDTH + px] = color;
                }
                
                // Bottom-right corner
                px = x + width - 1 - corner_x;
                py = y + height - 1 - corner_y;
                if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                    framebuffer[py * SCREEN_WIDTH + px] = color;
                }
            }
        }
    }
}

void render_header(uint16_t *framebuffer, const char *title) {
    if (!framebuffer || !title) return;
    
    // Draw header background only - no text
    render_fill_rect(framebuffer, 0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_BG);
}

void render_legend(uint16_t *framebuffer) {
    if (!framebuffer) return;
    
    // Draw "SEL - SETTINGS" legend in bottom right with highlight
    const char *legend = " SEL - SETTINGS ";
    int legend_y = SCREEN_HEIGHT - 24;
    
    // Calculate width (approximate)
    int legend_width = strlen(legend) * FONT_CHAR_SPACING;
    
    // Draw legend pill (right-aligned) with rounded corners
    int legend_x = SCREEN_WIDTH - legend_width - 12;
    render_rounded_rect(framebuffer, legend_x - 4, legend_y - 2, legend_width + 8, 20, 10, COLOR_SELECT_BG);
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, legend_x, legend_y, legend, COLOR_SELECT_TEXT);
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
        
        // Draw selection background (rounded pill shape) sized to text
        render_rounded_rect(framebuffer, PADDING - 4, y - 2, 
                        text_width + 12, ITEM_HEIGHT - 4, 8, COLOR_SELECT_BG);
        
        // Draw text in selection color
        font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, PADDING, y, name, COLOR_SELECT_TEXT);
    } else {
        // Draw normal text
        uint16_t text_color = is_dir ? COLOR_FOLDER : COLOR_TEXT;
        font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, PADDING, y, name, text_color);
    }
}