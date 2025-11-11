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

void (*load_and_run_core)(const char*, int*) = (void (*)(const char*, int*))0x800016d0;
#include "../../stockfw.h"

// For SF2000, use the custom dirent implementation
#include "../../dirent.h"
#else
#include <dirent.h>
#endif

#include "libretro.h"
#include "font.h"
#include "render.h"
#include "recent_games.h"
#include "settings.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define MAX_ENTRIES 256
#define MAX_PATH_LEN 512
#define ROMS_PATH "/mnt/sda1/ROMS"
#define HISTORY_FILE "/mnt/sda1/game_history.txt"
#define MAX_RECENT_GAMES 10

// Layout constants are now in render.h

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
static bool game_queued = false;  // Flag to indicate game is queued

// Colors (RGB565) - MinUI Exact Style from screenshot
#define COLOR_BG        0x0000  // Black background
#define COLOR_TEXT      0xFFFF  // White text
#define COLOR_SELECT_BG 0xFFFF  // White selection background (pill)
#define COLOR_SELECT_TEXT 0x0000  // Black text when selected
#define COLOR_HEADER    0xCE59  // Light gray for header
#define COLOR_FOLDER    0xFFFF  // White for folders (same as text)

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

// Comparison function to sort entries alphabetically by name
int compare_entries(const void *a, const void *b) {
    const MenuEntry *entry_a = (const MenuEntry *)a;
    const MenuEntry *entry_b = (const MenuEntry *)b;
    return strcmp(entry_a->name, entry_b->name);  // Compare by name
}

// Show recent games list
static void show_recent_games(void) {
    entry_count = 0;
    selected_index = 0;
    scroll_offset = 0;

    const RecentGame* recent_list = recent_games_get_list();
    int recent_count = recent_games_get_count();

    if (recent_count == 0) {
        // Only show back entry if no recent games
        strncpy(entries[entry_count].name, "..", sizeof(entries[entry_count].name) - 1);
        strncpy(entries[entry_count].path, ROMS_PATH, sizeof(entries[entry_count].path) - 1);
        entries[entry_count].is_dir = 1;
        entry_count++;
    } else {
        // Add recent games first
        for (int i = 0; i < recent_count && entry_count < MAX_ENTRIES; i++) {
            strncpy(entries[entry_count].name, recent_list[i].display_name, sizeof(entries[entry_count].name) - 1);
            snprintf(entries[entry_count].path, sizeof(entries[entry_count].path), 
                    "%s;%s", recent_list[i].core_name, recent_list[i].game_name);
            entries[entry_count].is_dir = 0;
            entry_count++;
        }
        
        // Add back entry after recent games
        strncpy(entries[entry_count].name, "..", sizeof(entries[entry_count].name) - 1);
        strncpy(entries[entry_count].path, ROMS_PATH, sizeof(entries[entry_count].path) - 1);
        entries[entry_count].is_dir = 1;
        entry_count++;
    }
}

// Scan directory and populate entries
static void scan_directory(const char *path) {
    DIR *dir;
    struct dirent *ent;

    entry_count = 0;
    selected_index = 0;
    scroll_offset = 0;

    // Store whether we're at root for recent games insertion later
    int is_root = (strcmp(path, ROMS_PATH) == 0);

    // Add parent directory entry if not at root
    if (!is_root) {
        strncpy(entries[entry_count].name, "..", sizeof(entries[entry_count].name) - 1);
        strncpy(entries[entry_count].path, path, sizeof(entries[entry_count].path) - 1);
        entries[entry_count].is_dir = 1;
        entry_count++;
    }

    dir = opendir(path);
    if (!dir) {
        return;
    }

    // Collect all entries in a single pass - optimized
    while ((ent = readdir(dir)) != NULL && entry_count < MAX_ENTRIES) {
        if (ent->d_name[0] == '.') continue;  // Skip hidden files

        // Skip frogos and saves folders
        if (strcasecmp(ent->d_name, "frogos") == 0 || strcasecmp(ent->d_name, "saves") == 0) {
            continue;
        }

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
            // Add directory entry
            strncpy(entries[entry_count].name, ent->d_name, sizeof(entries[entry_count].name) - 1);
            strncpy(entries[entry_count].path, full_path, sizeof(entries[entry_count].path) - 1);
            entries[entry_count].is_dir = 1;
            entry_count++;
        } else {
            // Add file entry
            strncpy(entries[entry_count].name, ent->d_name, sizeof(entries[entry_count].name) - 1);
            strncpy(entries[entry_count].path, full_path, sizeof(entries[entry_count].path) - 1);
            entries[entry_count].is_dir = 0;
            entry_count++;
        }
    }

    // Close the directory after reading
    closedir(dir);

    // Sort all entries alphabetically by name
    qsort(entries, entry_count, sizeof(MenuEntry), compare_entries);
    
    // Add Recent games at the very top if in root directory
    if (is_root) {
        // Shift all entries down by 1 to make room for Recent games at index 0
        for (int i = entry_count; i > 0; i--) {
            entries[i] = entries[i - 1];
        }
        
        // Insert Recent games at the top
        strncpy(entries[0].name, "Recent games", sizeof(entries[0].name) - 1);
        strncpy(entries[0].path, "RECENT_GAMES", sizeof(entries[0].path) - 1);
        entries[0].is_dir = 1;
        entry_count++;
    }
}

// Render settings menu
static void render_settings_menu() {
    // Draw title
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, PADDING, 10, "SETTINGS", COLOR_HEADER);
    
    int settings_count = settings_get_count();
    int start_y = 40;
    int selected_index = settings_get_selected_index();
    int scroll_offset = settings_get_scroll_offset();
    
    // Show settings options (two lines per option)
    // Reserve space for legend at bottom - ensure no overlap
    int max_visible = 3; // Reduced from 4 to ensure no overlap with legend
    for (int i = 0; i < max_visible && (scroll_offset + i) < settings_count; i++) {
        int option_index = scroll_offset + i;
        const SettingsOption *option = settings_get_option(option_index);
        if (!option) continue;
        
        int y_name = start_y + (i * ITEM_HEIGHT * 2);
        int y_value = y_name + ITEM_HEIGHT;
        
        // Check if this option is selected
        int is_selected = (option_index == selected_index);
        
        // Draw setting name (always white)
        font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, PADDING, y_name, option->name, COLOR_TEXT);
        
        // Draw setting value with selection background and arrows
        if (is_selected) {
            // Format value with arrows: "< current_value >"
            char value_text[256];
            snprintf(value_text, sizeof(value_text), "< %s >", option->current_value);
            
            int text_width = strlen(value_text) * FONT_CHAR_SPACING;
            render_rounded_rect(framebuffer, PADDING - 4, y_value - 2, 
                            text_width + 12, ITEM_HEIGHT - 4, 8, COLOR_SELECT_BG);
            
            font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, PADDING, y_value, value_text, COLOR_SELECT_TEXT);
        } else {
            font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, PADDING, y_value, option->current_value, COLOR_TEXT);
        }
    }
    
    // Draw legend with pillbox highlighting
    const char *legend = " A - SAVE   B - EXIT ";
    int legend_y = SCREEN_HEIGHT - 24;
    
    // Calculate width and position (right-aligned)
    int legend_width = strlen(legend) * FONT_CHAR_SPACING;
    int legend_x = SCREEN_WIDTH - legend_width - 12;
    
    // Draw legend pill with rounded corners
    render_rounded_rect(framebuffer, legend_x - 4, legend_y - 2, legend_width + 8, 20, 10, COLOR_SELECT_BG);
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, legend_x, legend_y, legend, COLOR_SELECT_TEXT);
}

// Render the menu using modular render system
static void render_menu() {
    render_clear_screen(framebuffer);

    // If game is queued, just show loading screen
    if (game_queued) {
        font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, 30, SCREEN_HEIGHT / 2, "LOADING...", 0xFFFF);
        return;
    }

    // If settings are active, render settings menu
    if (settings_is_active()) {
        render_settings_menu();
        return;
    }

    // Draw header with current folder name
    const char *display_path = current_path;
    if (strcmp(current_path, ROMS_PATH) == 0) {
        display_path = "SYSTEMS";  // Simplified root name
    } else {
        // Show just the folder name, not full path
        display_path = get_basename(current_path);
    }
    render_header(framebuffer, display_path);

    // Adjust the scroll_offset if necessary to keep the selected item visible
    if (selected_index < scroll_offset) {
        scroll_offset = selected_index;  // Scroll up to make the item visible
    } else if (selected_index >= scroll_offset + VISIBLE_ENTRIES) {
        scroll_offset = selected_index - VISIBLE_ENTRIES + 1;  // Scroll down to make the item visible
    }

    // Draw menu entries
    for (int i = scroll_offset; i < entry_count && i < scroll_offset + VISIBLE_ENTRIES; i++) {
        render_menu_item(framebuffer, i, entries[i].name, entries[i].is_dir, 
                        (i == selected_index), scroll_offset);
    }

    // Draw legend
    render_legend(framebuffer);
}

// Handle input
static void handle_input() {
    if (!input_poll_cb || !input_state_cb) return;

    input_poll_cb();

    // If game is queued, just show loading screen
    if (game_queued) {
        // Don't process any input
        return;
    }

    // Get current input state
    int up = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP);
    int down = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN);
    int a = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A);
    int b = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B);
    int l = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_L);
    int r = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_R);
    int select = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_SELECT);

    int left = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT);
    int right = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT);

    // Check if settings menu should handle input
    if (settings_handle_input(prev_input[0] && !up, prev_input[1] && !down, 
                            prev_input[7] && !left, prev_input[8] && !right,
                            prev_input[2] && !a, prev_input[3] && !b, prev_input[6] && !select)) {
        // Settings consumed the input, update prev_input and return
        prev_input[0] = up;
        prev_input[1] = down;
        prev_input[2] = a;
        prev_input[3] = b;
        prev_input[4] = l;
        prev_input[5] = r;
        prev_input[6] = select;
        prev_input[7] = left;
        prev_input[8] = right;
        return;
    }

    // Handle SELECT button to open settings (on button release)
    if (prev_input[6] && !select && strcmp(current_path, ROMS_PATH) == 0) {
        settings_show_menu();
        prev_input[6] = select;
        return;
    }

    // Handle up (on button release)
    if (prev_input[0] && !up) {
        if (selected_index > 0) {
            selected_index--;
        } else {
            // Loop to the last entry when at the top
            selected_index = entry_count - 1;
        }
        // Adjust scroll_offset if necessary
        if (selected_index < scroll_offset) {
            scroll_offset = selected_index;
        }
    }

    // Handle down (on button release)
    if (prev_input[1] && !down) {
        if (selected_index < entry_count - 1) {
            selected_index++;
        } else {
            // Loop to the first entry when at the bottom
            selected_index = 0;
        }
        // Adjust scroll_offset if necessary
        if (selected_index >= scroll_offset + VISIBLE_ENTRIES) {
            scroll_offset = selected_index - VISIBLE_ENTRIES + 1;
        }
    }

    // Handle L button (move up by 10 entries)
    if (prev_input[4] && !l) {
        if (selected_index >= 10) {
            selected_index -= 10;
        } else {
            // Loop to the bottom when reaching the top
            selected_index = entry_count - (10 - selected_index);
        }
        // Adjust scroll_offset if necessary
        if (selected_index < scroll_offset) {
            scroll_offset = selected_index;
        }
    }

    // Handle R button (move down by 10 entries)
    if (prev_input[5] && !r) {
        if (selected_index < entry_count - 10) {
            selected_index += 10;
        } else {
            // Loop to the top when reaching the bottom
            selected_index = (selected_index + 10) % entry_count;  // Wrap around to the top
        }
        // Adjust scroll_offset if necessary
        if (selected_index >= scroll_offset + VISIBLE_ENTRIES) {
            scroll_offset = selected_index - VISIBLE_ENTRIES + 1;
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
            if (strcmp(entry->path, "RECENT_GAMES") == 0) {
                // Show recent games list
                show_recent_games();
                strncpy(current_path, "RECENT_GAMES", sizeof(current_path) - 1);
            } else {
                strncpy(current_path, entry->path, sizeof(current_path) - 1);
                scan_directory(current_path);
            }
        } else {
            // File selected - try to launch it
            const char *core_name;
            const char *filename;
            
            // Check if we're in Recent games
            if (strcmp(current_path, "RECENT_GAMES") == 0) {
                // Parse core_name;game_name from entry->path
                char *separator = strchr(entry->path, ';');
                if (separator) {
                    *separator = '\0';
                    core_name = entry->path;
                    filename = separator + 1;
                    
                    // Add to recent history (moves to top)
                    recent_games_add(core_name, filename);
                } else {
                    return; // Invalid format
                }
            } else {
                // Extract core name from parent directory
                core_name = get_basename(current_path);
                const char *filename_path = strrchr(entry->path, '/');
                filename = filename_path ? filename_path + 1 : entry->name;
                
                // Add to recent history
                recent_games_add(core_name, filename);
            }

            // DEBUG: Log selection
            extern void xlog(const char *fmt, ...);
            xlog("[FrogOS] ROM Selection:\n");
            xlog("[FrogOS]   Current path: %s\n", current_path);
            xlog("[FrogOS]   Selected: %s\n", entry->name);
            xlog("[FrogOS]   Full path: %s\n", entry->path);
            xlog("[FrogOS]   Core: %s\n", core_name);
            xlog("[FrogOS]   Filename: %s\n", filename);

            sprintf((char *)ptr_gs_run_game_file, "/mnt/sda1/ROMS/%s;%s.gba", core_name, filename); // Workaround for loading a core from within a core, loader corrects
            sprintf((char *)ptr_gs_run_folder, "/mnt/sda1/ROMS"); // Expects "/mnt/sda1/ROMS" format
            sprintf((char *)ptr_gs_run_game_name, "%s;%s", core_name, filename); // Expects the filename without any extension 

            // Remove extension from ptr_gs_run_game_name
            char *dot_position = strrchr(ptr_gs_run_game_name, '.');
            if (dot_position != NULL) {
                *dot_position = '\0'; 
            }

            game_queued = true; // Pass to retro_run, can only load the core from there
        }
    }

    // Handle B button (back) - on button release
    if (prev_input[3] && !b) {
        if (strcmp(current_path, "RECENT_GAMES") == 0) {
            // Go back from Recent games to main ROMS directory
            strncpy(current_path, ROMS_PATH, sizeof(current_path) - 1);
            scan_directory(current_path);
        } else if (strcmp(current_path, ROMS_PATH) != 0) {
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
    prev_input[4] = l;
    prev_input[5] = r;
    prev_input[6] = select;
    prev_input[7] = left;
    prev_input[8] = right;
}

// Libretro API implementation
void retro_init(void) {
    framebuffer = (uint16_t*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint16_t));
    
    // Initialize modular systems
    render_init(framebuffer);
    font_init();
    recent_games_init();
    settings_init();
    
    recent_games_load();
    settings_load();
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
    handle_input();
    render_menu();
    if (video_cb) {
        video_cb(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH * sizeof(uint16_t));
    }
    if (game_queued) { // Can only load the game from here without crashing
        retro_deinit();
        load_and_run_core(ptr_gs_run_game_file, 0);
        return;
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
