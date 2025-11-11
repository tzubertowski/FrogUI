#include "render.h"
#include "font.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <dirent.h>

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

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
    
    // Draw folder/section name in header area
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, PADDING, 10, title, COLOR_HEADER);
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
    render_rounded_rect(framebuffer, legend_x - 4, legend_y - 2, legend_width + 8, 20, 10, COLOR_LEGEND_BG);
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, legend_x, legend_y, legend, COLOR_LEGEND);
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

// Thumbnail implementation

void get_thumbnail_path(const char *game_path, char *thumb_path, size_t thumb_path_size) {
    if (!game_path || !thumb_path || game_path[0] == '\0') {
        thumb_path[0] = '\0';
        return;
    }
    
    // Find the last slash to get directory
    const char *last_slash = strrchr(game_path, '/');
    if (!last_slash) {
        thumb_path[0] = '\0';
        return;
    }
    
    // Copy directory path
    size_t dir_len = last_slash - game_path;
    if (dir_len + 1 >= thumb_path_size) {
        thumb_path[0] = '\0';
        return;
    }
    
    strncpy(thumb_path, game_path, dir_len);
    thumb_path[dir_len] = '\0';
    
    // Add /.res/ subdirectory
    strncat(thumb_path, "/.res/", thumb_path_size - strlen(thumb_path) - 1);
    
    // Get filename without extension
    const char *filename = last_slash + 1;
    const char *last_dot = strrchr(filename, '.');
    
    if (last_dot) {
        size_t name_len = last_dot - filename;
        strncat(thumb_path, filename, min(name_len, thumb_path_size - strlen(thumb_path) - 1));
    } else {
        strncat(thumb_path, filename, thumb_path_size - strlen(thumb_path) - 1);
    }
    
    // Use raw RGB565 format - no parsing, fixed size, minimal memory
    strncat(thumb_path, ".rgb565", thumb_path_size - strlen(thumb_path) - 1);
}

static uint16_t rgb24_to_rgb565(uint8_t r, uint8_t g, uint8_t b) {
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

int load_thumbnail(const char *rgb565_path, Thumbnail *thumb) {
    if (!rgb565_path || !thumb) return 0;
    
    // Initialize thumbnail
    thumb->data = NULL;
    thumb->width = 0;
    thumb->height = 0;
    
    extern void xlog(const char *fmt, ...);
    
    // Use the path as-is for real device, only modify for dev environment
    const char *path_to_use = rgb565_path;
    char dev_path[512];
    
    // Only use dev path mapping in development environment (/app exists)
    if (access("/app", F_OK) == 0 && strstr(rgb565_path, "/mnt/sda1/ROMS/")) {
        snprintf(dev_path, sizeof(dev_path), "/app/sdcard/ROMS/%s", 
                 rgb565_path + strlen("/mnt/sda1/ROMS/"));
        path_to_use = dev_path;
        xlog("[FrogOS Thumbnail] DEV MODE: Using dev path: %s\n", dev_path);
    } else {
        xlog("[FrogOS Thumbnail] DEVICE MODE: Using original path: %s\n", rgb565_path);
    }
    
    // Just use the raw RGB565 loader - no parsing, no dynamic allocation
    return load_raw_rgb565(path_to_use, thumb);
}

// Static buffer for thumbnail - no malloc/free hell
static uint16_t thumbnail_buffer[250 * 200]; // Max size: 250x200

int load_raw_rgb565(const char *path, Thumbnail *thumb) {
    extern void xlog(const char *fmt, ...);
    
    xlog("[FrogOS Thumbnail] === DEBUG: Attempting to load RGB565 ===\n");
    xlog("[FrogOS Thumbnail] Target path: %s\n", path);
    
    // Check if file exists
    if (access(path, F_OK) != 0) {
        xlog("[FrogOS Thumbnail] File does not exist: %s\n", path);
        
        // List directory contents for debugging
        char *dir_path = strdup(path);
        char *last_slash = strrchr(dir_path, '/');
        if (last_slash) {
            *last_slash = '\0';
            xlog("[FrogOS Thumbnail] Checking directory: %s\n", dir_path);
            
            DIR *dir = opendir(dir_path);
            if (dir) {
                struct dirent *entry;
                xlog("[FrogOS Thumbnail] Directory contents:\n");
                while ((entry = readdir(dir)) != NULL) {
                    xlog("[FrogOS Thumbnail]   - %s\n", entry->d_name);
                }
                closedir(dir);
            } else {
                xlog("[FrogOS Thumbnail] Cannot open directory: %s\n", dir_path);
            }
        }
        free(dir_path);
        return 0;
    }
    
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        xlog("[FrogOS Thumbnail] Failed to open RGB565 (file exists but can't open): %s\n", path);
        return 0;
    }
    
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    xlog("[FrogOS Thumbnail] RGB565 file size: %ld bytes\n", file_size);
    
    // Try common dimensions - including 160x160 for the resized images
    int dimensions[][2] = {{64,64}, {128,128}, {160,160}, {200,200}, {250,200}, {200,250}};
    int num_dims = sizeof(dimensions) / sizeof(dimensions[0]);
    
    for (int i = 0; i < num_dims; i++) {
        int w = dimensions[i][0];
        int h = dimensions[i][1];
        if (w * h * 2 == file_size) {
            xlog("[FrogOS Thumbnail] Detected dimensions: %dx%d\n", w, h);
            
            // Check if it fits in our static buffer
            if (w * h > sizeof(thumbnail_buffer) / 2) {
                xlog("[FrogOS Thumbnail] Image too large for static buffer\n");
                fclose(fp);
                return 0;
            }
            
            thumb->width = w;
            thumb->height = h;
            thumb->data = thumbnail_buffer; // Use static buffer
            
            size_t read_bytes = fread(thumb->data, 1, file_size, fp);
            fclose(fp);
            
            if (read_bytes == file_size) {
                xlog("[FrogOS Thumbnail] Successfully loaded %dx%d RGB565\n", w, h);
                return 1;
            } else {
                xlog("[FrogOS Thumbnail] Read error: got %zu bytes, expected %ld\n", read_bytes, file_size);
                return 0;
            }
        }
    }
    
    xlog("[FrogOS Thumbnail] No matching dimensions found for file size %ld\n", file_size);
    fclose(fp);
    return 0;
}

void free_thumbnail(Thumbnail *thumb) {
    if (thumb) {
        // No need to free static buffer, just reset pointer
        thumb->data = NULL;
        thumb->width = 0;
        thumb->height = 0;
    }
}

void render_thumbnail(uint16_t *framebuffer, const Thumbnail *thumb) {
    if (!framebuffer || !thumb || !thumb->data) {
        extern void xlog(const char *fmt, ...);
        xlog("[FrogOS Thumbnail] render_thumbnail called but no data: fb=%p, thumb=%p, data=%p\n", 
             framebuffer, thumb, thumb ? thumb->data : NULL);
        return;
    }
    
    extern void xlog(const char *fmt, ...);
    xlog("[FrogOS Thumbnail] Rendering %dx%d thumbnail\n", thumb->width, thumb->height);
    
    // Calculate scaled dimensions to fit in thumbnail area
    int display_width = thumb->width;
    int display_height = thumb->height;
    
    // Scale down if too large
    if (display_width > THUMBNAIL_MAX_WIDTH) {
        display_height = (display_height * THUMBNAIL_MAX_WIDTH) / display_width;
        display_width = THUMBNAIL_MAX_WIDTH;
    }
    
    if (display_height > THUMBNAIL_MAX_HEIGHT) {
        display_width = (display_width * THUMBNAIL_MAX_HEIGHT) / display_height;
        display_height = THUMBNAIL_MAX_HEIGHT;
    }
    
    // Center in thumbnail area (vertically) and align to right edge
    int start_x = SCREEN_WIDTH - display_width;  // Align to right edge of screen
    
    // Center thumbnail vertically on screen
    int start_y = (SCREEN_HEIGHT - display_height) / 2;
    
    // Draw background frame with dark gray border and light gray fill
    #define FRAME_COLOR 0x39E7      // Dark gray border (RGB565: 7,15,7)
    #define BG_COLOR    0x2104      // Very dark gray background (RGB565: 4,8,4)
    
    int frame_x = start_x - 2;
    int frame_y = start_y - 2; 
    int frame_w = display_width + 4;
    int frame_h = display_height + 4;
    
    // Draw border frame
    render_fill_rect(framebuffer, frame_x, frame_y, frame_w, frame_h, FRAME_COLOR);
    // Draw inner background
    render_fill_rect(framebuffer, start_x, start_y, display_width, display_height, BG_COLOR);
    
    // Draw scaled thumbnail (simple nearest neighbor for now)
    for (int y = 0; y < display_height; y++) {
        for (int x = 0; x < display_width; x++) {
            int screen_x = start_x + x;
            int screen_y = start_y + y;
            
            if (screen_x >= 0 && screen_x < SCREEN_WIDTH && 
                screen_y >= 0 && screen_y < SCREEN_HEIGHT) {
                
                // Simple scaling - map display coords to source coords
                int src_x = (x * thumb->width) / display_width;
                int src_y = (y * thumb->height) / display_height;
                
                if (src_x < thumb->width && src_y < thumb->height) {
                    uint16_t pixel = thumb->data[src_y * thumb->width + src_x];
                    // Only draw non-black pixels, let dark gray background show through
                    if (pixel != 0x0000) {  
                        framebuffer[screen_y * SCREEN_WIDTH + screen_x] = pixel;
                    }
                }
            }
        }
    }
}