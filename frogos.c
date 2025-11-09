/*
 * FrogOS - MinUI-style File Browser for Multicore
 * A libretro core that provides a file browser interface
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>

#ifdef SF2000
// For SF2000, use the custom dirent implementation
#include "../../dirent.h"
#else
#include <dirent.h>
#endif

#include "libretro.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define MAX_ENTRIES 256
#define MAX_PATH_LEN 512
#define ROMS_PATH "/mnt/sda1/ROMS"
#define BOOT_FILE "/mnt/sda1/frogos_boot.txt"

// MinUI Layout Constants
#define HEADER_HEIGHT 30
#define ITEM_HEIGHT 24
#define PADDING 16
#define START_Y (HEADER_HEIGHT + 6)
#define VISIBLE_ENTRIES 6  // Reduced to 6 to make room for legend at bottom

// Menu state
typedef struct {
    char path[MAX_PATH_LEN];
    char name[256];
    int is_dir;
} MenuEntry;

static MenuEntry entries[MAX_ENTRIES];
static int entry_count = 0;
static int selected_index = 0;
static int scroll_offset = 0;
static char current_path[MAX_PATH_LEN];
static uint16_t *framebuffer = NULL;

// Libretro callbacks
static retro_video_refresh_t video_cb = NULL;
static retro_audio_sample_t audio_cb = NULL;
static retro_audio_sample_batch_t audio_batch_cb = NULL;
static retro_environment_t environ_cb = NULL;
static retro_input_poll_t input_poll_cb = NULL;
static retro_input_state_t input_state_cb = NULL;

// Input state
static int prev_input[16] = {0};
static bool game_queued = false;  // Flag to indicate game is queued, waiting for MENU

// Colors (RGB565) - MinUI Exact Style from screenshot
#define COLOR_BG        0x0000  // Black background
#define COLOR_TEXT      0xFFFF  // White text
#define COLOR_SELECT_BG 0xFFFF  // White selection background (pill)
#define COLOR_SELECT_TEXT 0x0000  // Black text when selected
#define COLOR_HEADER    0xCE59  // Light gray for header
#define COLOR_FOLDER    0xFFFF  // White for folders (same as text)

// Include font bitmap data
#include "font_chillround.h"

// Draw a character using ChillRound 16x16 bitmap font - inlined for speed
static inline void draw_char(int x, int y, char c, uint16_t color) {
    if (c < 32 || c > 127) c = ' ';  // Replace non-printable with space

    const uint8_t *glyph = font_chillround_16x16[c - 32];

    // 16x16 font, 2 bytes per row (16 bits)
    for (int row = 0; row < 16; row++) {
        // Each row is 2 bytes (16 bits)
        uint16_t row_bits = (glyph[row * 2] << 8) | glyph[row * 2 + 1];

        for (int col = 0; col < 16; col++) {
            if (row_bits & (1 << (15 - col))) {  // Check bit from left to right
                int px = x + col;
                int py = y + row;
                if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                    framebuffer[py * SCREEN_WIDTH + px] = color;
                }
            }
        }
    }
}

static void draw_text(int x, int y, const char *text, uint16_t color) {
    int start_x = x;
    while (*text) {
        if (*text == '\n') {
            y += 20;  // Double spacing for 2x font
            x = start_x;
        } else {
            draw_char(x, y, *text, color);
            x += 16;  // 16 pixels per character (8x8 scaled 2x)
        }
        text++;
    }
}

// Inline for speed - called frequently
static inline void draw_filled_rect(int x, int y, int w, int h, uint16_t color) {
    // Clip to screen bounds once
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_WIDTH) w = SCREEN_WIDTH - x;
    if (y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;
    if (w <= 0 || h <= 0) return;

    // Fast fill - direct framebuffer access
    uint16_t *fb = framebuffer + (y * SCREEN_WIDTH + x);
    for (int dy = 0; dy < h; dy++) {
        for (int dx = 0; dx < w; dx++) {
            fb[dx] = color;
        }
        fb += SCREEN_WIDTH;
    }
}

// Draw a rounded rectangle (pill-shaped) - MinUI style
static void draw_rounded_rect(int x, int y, int w, int h, int radius, uint16_t color) {
    // Draw main body (excluding corners)
    draw_filled_rect(x + radius, y, w - 2 * radius, h, color);
    draw_filled_rect(x, y + radius, w, h - 2 * radius, color);

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
                px = x + w - 1 - corner_x;
                py = y + corner_y;
                if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                    framebuffer[py * SCREEN_WIDTH + px] = color;
                }

                // Bottom-left corner
                px = x + corner_x;
                py = y + h - 1 - corner_y;
                if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                    framebuffer[py * SCREEN_WIDTH + px] = color;
                }

                // Bottom-right corner
                px = x + w - 1 - corner_x;
                py = y + h - 1 - corner_y;
                if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
                    framebuffer[py * SCREEN_WIDTH + px] = color;
                }
            }
        }
    }
}

// Fast screen clear using direct memory operations
static inline void clear_screen(uint16_t color) {
    // Use word-sized operations for speed
    uint16_t *fb = framebuffer;
    int count = SCREEN_WIDTH * SCREEN_HEIGHT;

    // Unroll loop for speed
    while (count >= 4) {
        fb[0] = color;
        fb[1] = color;
        fb[2] = color;
        fb[3] = color;
        fb += 4;
        count -= 4;
    }
    while (count--) {
        *fb++ = color;
    }
}

// Get the base name from a path
static const char *get_basename(const char *path) {
    const char *base = strrchr(path, '/');
    return base ? base + 1 : path;
}

// Check if path is a directory - optimized to use d_type first
static inline int is_directory_fast(const char *path, unsigned char d_type) {
    // Use d_type if available (much faster, no stat call needed)
    if (d_type != DT_UNKNOWN) {
        return (d_type == DT_DIR);
    }

    // Fallback to stat only if needed
    struct stat st;
    if (stat(path, &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return 0;
}

// Scan directory and populate entries
static void scan_directory(const char *path) {
    DIR *dir;
    struct dirent *ent;
    int dir_start_index;

    entry_count = 0;
    selected_index = 0;
    scroll_offset = 0;

    // Add parent directory entry if not at root
    if (strcmp(path, ROMS_PATH) != 0) {
        strncpy(entries[entry_count].name, "..", sizeof(entries[entry_count].name) - 1);
        strncpy(entries[entry_count].path, path, sizeof(entries[entry_count].path) - 1);
        entries[entry_count].is_dir = 1;
        entry_count++;
    }

    dir = opendir(path);
    if (!dir) {
        return;
    }

    // Remember where directories start
    dir_start_index = entry_count;

    // Collect all entries in a single pass - optimized
    while ((ent = readdir(dir)) != NULL && entry_count < MAX_ENTRIES) {
        if (ent->d_name[0] == '.') continue;  // Skip hidden files

        char full_path[MAX_PATH_LEN];
        snprintf(full_path, sizeof(full_path), "%s/%s", path, ent->d_name);

        // Fast path: use d_type if available, avoid stat() calls
        int is_dir = is_directory_fast(full_path, ent->d_type);

        // Skip files if in root ROMS directory (only show folders there)
        if (strcmp(path, ROMS_PATH) == 0 && !is_dir) {
            continue;
        }

        // Add directories first, then files
        if (is_dir) {
            // Insert directory in order with other directories
            int insert_pos = dir_start_index;
            while (insert_pos < entry_count && entries[insert_pos].is_dir) {
                insert_pos++;
            }
            // Make room if needed
            if (insert_pos < entry_count) {
                for (int i = entry_count; i > insert_pos; i--) {
                    entries[i] = entries[i - 1];
                }
            }
            strncpy(entries[insert_pos].name, ent->d_name, sizeof(entries[insert_pos].name) - 1);
            strncpy(entries[insert_pos].path, full_path, sizeof(entries[insert_pos].path) - 1);
            entries[insert_pos].is_dir = 1;
            entry_count++;
        } else {
            // Files go at the end
            strncpy(entries[entry_count].name, ent->d_name, sizeof(entries[entry_count].name) - 1);
            strncpy(entries[entry_count].path, full_path, sizeof(entries[entry_count].path) - 1);
            entries[entry_count].is_dir = 0;
            entry_count++;
        }
    }

    closedir(dir);
}

// Render the menu - MinUI exact style
static void render_menu() {
    clear_screen(COLOR_BG);

    // If game is queued, show exit instructions
    if (game_queued) {
        draw_text(30, SCREEN_HEIGHT / 2 - 30, "Game queued!", 0xFFFF);
        draw_text(30, SCREEN_HEIGHT / 2, "Press SEL+START", 0x07E0);
        draw_text(50, SCREEN_HEIGHT / 2 + 20, "then EXIT", 0x07E0);
        return;
    }

    // Use MinUI layout constants
    int visible_entries = VISIBLE_ENTRIES;

    // Draw simple header with path (MinUI style - minimal)
    const char *display_path = current_path;
    if (strcmp(current_path, ROMS_PATH) == 0) {
        display_path = "Systems";  // Simplified root name
    } else {
        // Show just the folder name, not full path
        display_path = get_basename(current_path);
    }
    draw_text(PADDING, 8, display_path, COLOR_HEADER);

    // Draw menu entries with MinUI styling
    for (int i = scroll_offset; i < entry_count && i < scroll_offset + visible_entries; i++) {
        int y = START_Y + ((i - scroll_offset) * ITEM_HEIGHT);

        // Determine colors based on selection
        uint16_t text_color;
        if (i == selected_index) {
            // Draw VERY rounded pill-shaped selection background (MinUI style)
            int sel_x = 12;
            int sel_y = y - 2;
            int sel_w = SCREEN_WIDTH - 24;
            int sel_h = ITEM_HEIGHT - 4;
            int radius = 10;  // Very rounded pill like MinUI (max is sel_h/2 = 10)

            draw_rounded_rect(sel_x, sel_y, sel_w, sel_h, radius, COLOR_SELECT_BG);
            text_color = COLOR_SELECT_TEXT;
        } else {
            text_color = entries[i].is_dir ? COLOR_FOLDER : COLOR_TEXT;
        }

        // Draw entry text (no brackets, MinUI is clean)
        draw_text(PADDING + 4, y + 4, entries[i].name, text_color);
    }

    // Optional: Draw scroll indicator if there are more items
    if (entry_count > visible_entries) {
        int indicator_x = SCREEN_WIDTH - 6;
        int indicator_h = SCREEN_HEIGHT - START_Y - 10;
        int thumb_h = (indicator_h * visible_entries) / entry_count;
        int thumb_y = START_Y + (indicator_h * scroll_offset) / entry_count;

        draw_filled_rect(indicator_x, thumb_y, 2, thumb_h, COLOR_HEADER);
    }

    // Draw legend at bottom right (MinUI style pill)
    int legend_y = SCREEN_HEIGHT - 24;
    const char *legend = " A/B - NAV ";

    // Calculate width (approximate - 16px per char)
    int legend_width = strlen(legend) * 16;

    // Draw legend pill (right-aligned)
    int legend_x = SCREEN_WIDTH - legend_width - 12;
    draw_rounded_rect(legend_x - 4, legend_y - 2, legend_width + 8, 20, 10, COLOR_SELECT_BG);
    draw_text(legend_x, legend_y, legend, COLOR_SELECT_TEXT);
}

// Handle input
static void handle_input() {
    if (!input_poll_cb || !input_state_cb) return;

    input_poll_cb();

    // If game is queued, just show message - user must exit manually
    if (game_queued) {
        // Don't process any input, just wait for user to exit with physical MENU button
        return;
    }

    // Get current input state
    int up = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP);
    int down = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN);
    int a = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
    int b = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);

    // Handle up (on button release)
    if (prev_input[0] && !up) {
        if (selected_index > 0) {
            selected_index--;
            if (selected_index < scroll_offset) {
                scroll_offset = selected_index;
            }
        }
    }

    // Handle down (on button release)
    if (prev_input[1] && !down) {
        if (selected_index < entry_count - 1) {
            selected_index++;
            if (selected_index >= scroll_offset + VISIBLE_ENTRIES) {
                scroll_offset = selected_index - VISIBLE_ENTRIES + 1;
            }
        }
    }

    // Handle A button (select) - on button release
    if (prev_input[2] && !a && entry_count > 0) {
        MenuEntry *entry = &entries[selected_index];

        if (strcmp(entry->name, "..") == 0) {
            // Go to parent directory
            char *last_slash = strrchr(current_path, '/');
            if (last_slash && last_slash != current_path) {
                *last_slash = '\0';
                scan_directory(current_path);
            }
        } else if (entry->is_dir) {
            // Enter directory
            strncpy(current_path, entry->path, sizeof(current_path) - 1);
            scan_directory(current_path);
        } else {
            // File selected - try to launch it
            // Extract core name from parent directory
            const char *core_name = get_basename(current_path);

            // DEBUG: Log selection
            extern void xlog(const char *fmt, ...);
            xlog("[FrogOS] ROM Selection:\n");
            xlog("[FrogOS]   Current path: %s\n", current_path);
            xlog("[FrogOS]   Selected: %s\n", entry->name);
            xlog("[FrogOS]   Full path: %s\n", entry->path);
            xlog("[FrogOS]   Core: %s\n", core_name);

            // Build game path for loader
            char game_path[512];
            const char *filename = strrchr(entry->path, '/');
            if (filename) {
                filename++;  // Skip slash
            } else {
                filename = entry->name;
            }

            // Strip .GBA extension from stub filename to get actual ROM name
            char rom_name[256];
            strncpy(rom_name, filename, sizeof(rom_name) - 1);
            rom_name[sizeof(rom_name) - 1] = '\0';

            char *gba_ext = strstr(rom_name, ".GBA");
            if (!gba_ext) {
                gba_ext = strstr(rom_name, ".gba");
            }
            if (gba_ext) {
                *gba_ext = '\0';  // Remove .GBA extension
            }

            snprintf(game_path, sizeof(game_path), "/mnt/sda1/ROMS/%s;%s", core_name, rom_name);

            xlog("[FrogOS] Loading game: %s\n", game_path);

            // Call custom environment command to queue game
            if (environ_cb(0x10000, game_path)) {
                xlog("[FrogOS] Game queued! Exit to launch: SEL+START -> QUIT\n");
                game_queued = true;
            } else {
                xlog("[FrogOS] ERROR: Failed to queue game\n");
            }
        }
    }

    // Handle B button (back) - on button release
    if (prev_input[3] && !b) {
        if (strcmp(current_path, ROMS_PATH) != 0) {
            char *last_slash = strrchr(current_path, '/');
            if (last_slash && last_slash != current_path) {
                *last_slash = '\0';
                scan_directory(current_path);
            }
        }
    }

    // Store current state for next frame
    prev_input[0] = up;
    prev_input[1] = down;
    prev_input[2] = a;
    prev_input[3] = b;
}

// Libretro API implementation
void retro_init(void) {
    framebuffer = (uint16_t*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint16_t));
    strncpy(current_path, ROMS_PATH, sizeof(current_path) - 1);
    scan_directory(current_path);
}

void retro_deinit(void) {
    if (framebuffer) {
        free(framebuffer);
        framebuffer = NULL;
    }
}

unsigned retro_api_version(void) {
    return RETRO_API_VERSION;
}

void retro_set_controller_port_device(unsigned port, unsigned device) {
    (void)port;
    (void)device;
}

void retro_get_system_info(struct retro_system_info *info) {
    memset(info, 0, sizeof(*info));
    info->library_name     = "FrogOS";
    info->library_version  = "0.1";
    info->need_fullpath    = false;
    info->valid_extensions = "frogos";
}

void retro_get_system_av_info(struct retro_system_av_info *info) {
    info->timing.fps = 60.0;
    info->timing.sample_rate = 44100.0;

    info->geometry.base_width   = SCREEN_WIDTH;
    info->geometry.base_height  = SCREEN_HEIGHT;
    info->geometry.max_width    = SCREEN_WIDTH;
    info->geometry.max_height   = SCREEN_HEIGHT;
    info->geometry.aspect_ratio = (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT;
}

void retro_set_environment(retro_environment_t cb) {
    environ_cb = cb;

    bool no_content = true;
    cb(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_content);

    enum retro_pixel_format fmt = RETRO_PIXEL_FORMAT_RGB565;
    cb(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &fmt);
}

void retro_set_audio_sample(retro_audio_sample_t cb) {
    audio_cb = cb;
}

void retro_set_audio_sample_batch(retro_audio_sample_batch_t cb) {
    audio_batch_cb = cb;
}

void retro_set_input_poll(retro_input_poll_t cb) {
    input_poll_cb = cb;
}

void retro_set_input_state(retro_input_state_t cb) {
    input_state_cb = cb;
}

void retro_set_video_refresh(retro_video_refresh_t cb) {
    video_cb = cb;
}

void retro_reset(void) {
    strncpy(current_path, ROMS_PATH, sizeof(current_path) - 1);
    scan_directory(current_path);
}

void retro_run(void) {
    if (game_queued) {
        // Show message to exit manually
        memset(framebuffer, 0, SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint16_t));

        draw_text(80, 100, "Game queued!", 0xFFFF);
        draw_text(40, 120, "Exit to launch game:", 0xFFFF);
        draw_text(50, 140, "SEL+START -> QUIT", 0xF800); // Red color

        if (video_cb) {
            video_cb(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH * sizeof(uint16_t));
        }
        return;
    }

    handle_input();
    render_menu();

    if (video_cb) {
        video_cb(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH * sizeof(uint16_t));
    }
}

bool retro_load_game(const struct retro_game_info *info) {
    (void)info;
    return true;
}

void retro_unload_game(void) {
}

unsigned retro_get_region(void) {
    return RETRO_REGION_NTSC;
}

bool retro_load_game_special(unsigned type, const struct retro_game_info *info, size_t num) {
    (void)type;
    (void)info;
    (void)num;
    return false;
}

size_t retro_serialize_size(void) {
    return 0;
}

bool retro_serialize(void *data, size_t size) {
    (void)data;
    (void)size;
    return false;
}

bool retro_unserialize(const void *data, size_t size) {
    (void)data;
    (void)size;
    return false;
}

void *retro_get_memory_data(unsigned id) {
    (void)id;
    return NULL;
}

size_t retro_get_memory_size(unsigned id) {
    (void)id;
    return 0;
}

void retro_cheat_reset(void) {
}

void retro_cheat_set(unsigned index, bool enabled, const char *code) {
    (void)index;
    (void)enabled;
    (void)code;
}
