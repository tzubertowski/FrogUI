/*
 * FrogOS - MinUI-style File Browser for Multicore
 * A libretro core that provides a file browser interface
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <time.h>

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
#include "theme.h"
#include "recent_games.h"
#include "favorites.h"
#include "settings.h"

// Console to core name mapping (from buildcoresworking.sh)
typedef struct {
    const char *console_name;
    const char *core_name;
} ConsoleMapping;

static const ConsoleMapping console_mappings[] = {
    {"gb", "Gambatte"},
    {"gbb", "TGBDual"}, 
    {"gbgb", "Gearboy"},
    {"dblcherrygb", "DoubleCherry-GB"},
    {"gba", "gpSP"},
    {"gbaf", "gpSP"}, 
    {"gbaff", "gpSP"},
    {"gbav", "VBA-Next"},
    {"mgba", "mGBA"},
    {"nes", "FCEUmm"},
    {"nesq", "QuickNES"},
    {"nest", "Nestopia"},
    {"snes", "Snes9x2005"},
    {"snes02", "Snes9x2002"},
    {"sega", "PicoDrive"},
    {"gg", "Gearsystem"},
    {"gpgx", "Genesis-Plus-GX"},
    {"pce", "Beetle-PCE-Fast"},
    {"pcesgx", "Beetle-SuperGrafx"},
    {"pcfx", "Beetle-PCFX"},
    {"ngpc", "RACE"},
    {"lnx", "Handy"},
    {"lnxb", "Beetle-Lynx"},
    {"wswan", "Beetle-WonderSwan"},
    {"wsv", "Potator"},
    {"pokem", "PokeMini"},
    {"vb", "Beetle-VB"},
    {"a26", "Stella2014"},
    {"a5200", "Atari5200"},
    {"a78", "ProSystem"},
    {"a800", "Atari800"},
    {"int", "FreeIntv"},
    {"col", "Gearcoleco"},
    {"msx", "BlueMSX"},
    {"spec", "Fuse"},
    {"zx81", "EightyOne"},
    {"thom", "Theodore"},
    {"vec", "VecX"},
    {"c64", "VICE-x64"},
    {"c64sc", "VICE-x64sc"},
    {"c64f", "Frodo"},
    {"c64fc", "Frodo"},
    {"vic20", "VICE-xvic"},
    {"amstradb", "CAP32"},
    {"amstrad", "CrocoDS"},
    {"bk", "BK-Emulator"},
    {"pc8800", "QUASI88"},
    {"xmil", "X-Millennium"},
    {"m2k", "MAME2000"},
    {"chip8", "JAXE"},
    {"fcf", "FreeChaF"},
    {"retro8", "Retro8"},
    {"vapor", "VaporSpec"},
    {"gong", "Gong"},
    {"outrun", "Cannonball"},
    {"wolf3d", "ECWolf"},
    {"prboom", "PrBoom"},
    {"flashback", "REminiscence"},
    {"xrick", "XRick"},
    {"gw", "Game-and-Watch"},
    {"cdg", "PocketCDG"},
    {"gme", "Game-Music-Emu"},
    {"fake08", "FAKE-08"},
    {"lowres-nx", "LowRes-NX"},
    {"jnb", "Jump-n-Bump"},
    {"cavestory", "NXEngine"},
    {"o2em", "O2EM"},
    {"quake", "TyrQuake"},
    {"arduboy", "Arduous"},
    {"js2000", "js2000"}
};

// Get core name for a console folder
static const char* get_core_name_for_console(const char* console_name) {
    int mapping_count = sizeof(console_mappings) / sizeof(console_mappings[0]);
    for (int i = 0; i < mapping_count; i++) {
        if (strcmp(console_mappings[i].console_name, console_name) == 0) {
            return console_mappings[i].core_name;
        }
    }
    return NULL; // Unknown console
}

// Show core-specific settings menu
static void show_core_settings(const char* core_name) {
    if (settings_load_core(core_name)) {
        settings_show_menu();
    } else {
        // No settings file found for this core - could show a message or create default
        // For now, just ignore if no settings file exists
    }
}

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define MAX_ENTRIES 256
#define MAX_PATH_LEN 512
#define ROMS_PATH "/mnt/sda1/ROMS"
#define HISTORY_FILE "/mnt/sda1/game_history.txt"
#define MAX_RECENT_GAMES 10

// Layout constants are now in render.h

// Thumbnail cache
static Thumbnail current_thumbnail;
static char cached_thumbnail_path[MAX_PATH_LEN];
static int thumbnail_cache_valid = 0;
static int last_selected_index = -1;

// Text scrolling state
static int text_scroll_frame_counter = 0;
static int text_scroll_offset = 0;
static int text_scroll_direction = 1;

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

// Boundary scroll delay (frames to wait before wrapping)
#define BOUNDARY_DELAY_FRAMES 30
static int boundary_delay_timer = 0;
static int at_boundary = 0; // 1 = at top, 2 = at bottom

// A-Z picker state
static int az_picker_active = 0;
static int az_selected_index = 0; // 0-25 for A-Z, 26 for 0-9, 27 for #

// Reset navigation state when entering new folder
static void reset_navigation_state(void) {
    selected_index = 0;
    scroll_offset = 0;
    boundary_delay_timer = 0;
    at_boundary = 0;
}

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


// Get the base name from a path
static const char *get_basename(const char *path) {
    const char *base = strrchr(path, '/');
    return base ? base + 1 : path;
}

// Auto-launch most recent game if resume on boot is enabled
static void auto_launch_recent_game(void) {
    // Check if resume on boot is enabled
    const char *resume_setting = settings_get_value("frogui_resume_on_boot");
    if (!resume_setting || strcmp(resume_setting, "true") != 0) {
        return; // Feature is disabled
    }

    // Get the most recent game
    const RecentGame *recent_list = recent_games_get_list();
    int recent_count = recent_games_get_count();

    if (recent_count == 0) {
        return; // No recent games to launch
    }

    // Get the first (most recent) game
    const RecentGame *game = &recent_list[0];
    const char *core_name = game->core_name;
    const char *filename = game->game_name;

    // Queue the game for launch
    sprintf((char *)ptr_gs_run_game_file, "%s;%s;%s.gba", core_name, core_name, filename); // TODO: Replace second core_name with full directory (besides /mnt/sda1) and seperate core_name from directory
    sprintf((char *)ptr_gs_run_folder, "/mnt/sda1/ROMS");
    sprintf((char *)ptr_gs_run_game_name, "%s", filename);

    // Remove extension from ptr_gs_run_game_name
    char *dot_position = strrchr(ptr_gs_run_game_name, '.');
    if (dot_position != NULL) {
        *dot_position = '\0';
    }

    game_queued = true;
}

// Get scrolling display text for selected item
static void get_scrolling_text(const char *full_name, int is_selected, char *display_name, size_t display_size) {
    if (!full_name || !display_name) return;

    int name_len = strlen(full_name);

    // Check if we're in main menu or special views (no thumbnails)
    int in_main_menu = (strcmp(current_path, ROMS_PATH) == 0 ||
                        strcmp(current_path, "RECENT_GAMES") == 0 ||
                        strcmp(current_path, "FAVORITES") == 0 ||
                        strcmp(current_path, "TOOLS") == 0 ||
                        strcmp(current_path, "UTILS") == 0 ||
                        strcmp(current_path, "HOTKEYS") == 0 ||
                        strcmp(current_path, "CREDITS") == 0);

    // Use different max lengths: shorter for unselected items only in ROM lists (with thumbnails)
    int max_len = is_selected ? MAX_FILENAME_DISPLAY_LEN :
                  (in_main_menu ? MAX_FILENAME_DISPLAY_LEN : MAX_UNSELECTED_DISPLAY_LEN);

    // If short enough or not selected, just copy/truncate normally
    if (name_len <= max_len || !is_selected) {
        if (name_len <= max_len) {
            strcpy(display_name, full_name);
        } else {
            strncpy(display_name, full_name, max_len);
            display_name[max_len] = '\0';
            strcat(display_name, "...");
        }
        return;
    }
    
    // Handle scrolling for selected long names
    text_scroll_frame_counter++;
    
    // Wait before starting scroll
    if (text_scroll_frame_counter < SCROLL_DELAY_FRAMES) {
        strncpy(display_name, full_name, MAX_FILENAME_DISPLAY_LEN);
        display_name[MAX_FILENAME_DISPLAY_LEN] = '\0';
        return;
    }
    
    // Update scroll position
    if (text_scroll_frame_counter % SCROLL_SPEED_FRAMES == 0) {
        text_scroll_offset += text_scroll_direction;
        
        // Reverse direction at ends
        int max_scroll = name_len - MAX_FILENAME_DISPLAY_LEN;
        if (text_scroll_offset >= max_scroll) {
            text_scroll_direction = -1;
            text_scroll_offset = max_scroll;
        } else if (text_scroll_offset <= 0) {
            text_scroll_direction = 1;
            text_scroll_offset = 0;
        }
    }
    
    // Extract scrolled portion
    int copy_len = min(MAX_FILENAME_DISPLAY_LEN, name_len - text_scroll_offset);
    strncpy(display_name, full_name + text_scroll_offset, copy_len);
    display_name[copy_len] = '\0';
}

// Load thumbnail for currently selected item
static void load_current_thumbnail() {
    if (selected_index < 0 || selected_index >= entry_count || entry_count == 0) {
        thumbnail_cache_valid = 0;
        return;
    }
    
    // Only load thumbnails for files, not directories
    if (entries[selected_index].is_dir) {
        thumbnail_cache_valid = 0;
        return;
    }
    
    char thumb_path[MAX_PATH_LEN];
    
    // Check if we're in Recent games mode
    if (strcmp(current_path, "RECENT_GAMES") == 0) {
        // For recent games, we need to use the full_path from the RecentGame structure
        const RecentGame* recent_list = recent_games_get_list();
        int recent_count = recent_games_get_count();

        if (selected_index < recent_count) {
            const RecentGame *recent_game = &recent_list[selected_index];

            if (recent_game->full_path[0] != '\0') {
                get_thumbnail_path(recent_game->full_path, thumb_path, sizeof(thumb_path));
            } else {
                // No full path available, skip thumbnail
                thumbnail_cache_valid = 0;
                return;
            }
        } else {
            // This is the ".." entry, no thumbnail
            thumbnail_cache_valid = 0;
            return;
        }
    } else if (strcmp(current_path, "FAVORITES") == 0) {
        // For favorites, we need to use the full_path from the FavoriteGame structure
        const FavoriteGame* favorites_list = favorites_get_list();
        int favorites_count = favorites_get_count();

        if (selected_index < favorites_count) {
            const FavoriteGame *favorite_game = &favorites_list[selected_index];

            if (favorite_game->full_path[0] != '\0') {
                get_thumbnail_path(favorite_game->full_path, thumb_path, sizeof(thumb_path));
            } else {
                // No full path available, skip thumbnail
                thumbnail_cache_valid = 0;
                return;
            }
        } else {
            // This is the ".." entry, no thumbnail
            thumbnail_cache_valid = 0;
            return;
        }
    } else {
        // Regular file browser mode
        get_thumbnail_path(entries[selected_index].path, thumb_path, sizeof(thumb_path));
    }
    
    // Check if we already have this thumbnail cached
    if (thumbnail_cache_valid && strcmp(cached_thumbnail_path, thumb_path) == 0) {
        return; // Already cached
    }
    
    // Free previous thumbnail
    if (thumbnail_cache_valid) {
        free_thumbnail(&current_thumbnail);
        thumbnail_cache_valid = 0;
    }
    
    // Try to load new thumbnail
    if (load_thumbnail(thumb_path, &current_thumbnail)) {
        strncpy(cached_thumbnail_path, thumb_path, sizeof(cached_thumbnail_path) - 1);
        cached_thumbnail_path[sizeof(cached_thumbnail_path) - 1] = '\0';
        thumbnail_cache_valid = 1;
        
    } else {
    }
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
    reset_navigation_state();
    
    // Set current_path so thumbnail loading knows we're in recent games mode
    strncpy(current_path, "RECENT_GAMES", sizeof(current_path) - 1);
    current_path[sizeof(current_path) - 1] = '\0';
    
    // Clear thumbnail cache when switching to recent games mode
    thumbnail_cache_valid = 0;

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
    
    // Load thumbnail for initially selected item AND reset last_selected_index to prevent duplicate loading
    load_current_thumbnail();
    last_selected_index = selected_index;  // Prevent render loop from detecting this as a "change"
}

// Show favorites
static void show_favorites(void) {
    entry_count = 0;
    reset_navigation_state();

    // Set current_path so thumbnail loading knows we're in favorites mode
    strncpy(current_path, "FAVORITES", sizeof(current_path) - 1);
    current_path[sizeof(current_path) - 1] = '\0';

    // Clear thumbnail cache when switching to favorites mode
    thumbnail_cache_valid = 0;

    const FavoriteGame* favorites_list = favorites_get_list();
    int favorites_count = favorites_get_count();

    if (favorites_count == 0) {
        // Only show back entry if no favorites
        strncpy(entries[entry_count].name, "..", sizeof(entries[entry_count].name) - 1);
        strncpy(entries[entry_count].path, ROMS_PATH, sizeof(entries[entry_count].path) - 1);
        entries[entry_count].is_dir = 1;
        entry_count++;
    } else {
        // Add favorites first
        for (int i = 0; i < favorites_count && entry_count < MAX_ENTRIES; i++) {
            strncpy(entries[entry_count].name, favorites_list[i].display_name, sizeof(entries[entry_count].name) - 1);
            snprintf(entries[entry_count].path, sizeof(entries[entry_count].path),
                    "%s;%s", favorites_list[i].core_name, favorites_list[i].game_name);
            entries[entry_count].is_dir = 0;
            entry_count++;
        }

        // Add back entry after favorites
        strncpy(entries[entry_count].name, "..", sizeof(entries[entry_count].name) - 1);
        strncpy(entries[entry_count].path, ROMS_PATH, sizeof(entries[entry_count].path) - 1);
        entries[entry_count].is_dir = 1;
        entry_count++;
    }

    // Load thumbnail for initially selected item AND reset last_selected_index to prevent duplicate loading
    load_current_thumbnail();
    last_selected_index = selected_index;  // Prevent render loop from detecting this as a "change"
}

// Show tools menu
static void show_tools_menu(void) {
    entry_count = 0;
    reset_navigation_state();
    
    // Set current_path for tools mode
    strncpy(current_path, "TOOLS", sizeof(current_path) - 1);
    current_path[sizeof(current_path) - 1] = '\0';
    
    // Clear thumbnail cache when switching to tools mode
    thumbnail_cache_valid = 0;
    
    // Add Hotkeys entry
    strncpy(entries[entry_count].name, "Hotkeys", sizeof(entries[entry_count].name) - 1);
    strncpy(entries[entry_count].path, "HOTKEYS", sizeof(entries[entry_count].path) - 1);
    entries[entry_count].is_dir = 1;
    entry_count++;
    
    // Add Credits entry
    strncpy(entries[entry_count].name, "Credits", sizeof(entries[entry_count].name) - 1);
    strncpy(entries[entry_count].path, "CREDITS", sizeof(entries[entry_count].path) - 1);
    entries[entry_count].is_dir = 1;
    entry_count++;
    
    // Add Utils entry
    strncpy(entries[entry_count].name, "Utils", sizeof(entries[entry_count].name) - 1);
    strncpy(entries[entry_count].path, "UTILS", sizeof(entries[entry_count].path) - 1);
    entries[entry_count].is_dir = 1;
    entry_count++;
    
    // Add back entry
    strncpy(entries[entry_count].name, "..", sizeof(entries[entry_count].name) - 1);
    strncpy(entries[entry_count].path, ROMS_PATH, sizeof(entries[entry_count].path) - 1);
    entries[entry_count].is_dir = 1;
    entry_count++;
    
    // Load thumbnail for initially selected item AND reset last_selected_index to prevent duplicate loading
    load_current_thumbnail();
    last_selected_index = selected_index;  // Prevent render loop from detecting this as a "change"
}

// Show utils menu with js2000 files
static void show_utils_menu(void) {
    entry_count = 0;
    reset_navigation_state();
    
    // Set current_path for utils mode
    strncpy(current_path, "UTILS", sizeof(current_path) - 1);
    current_path[sizeof(current_path) - 1] = '\0';
    
    // Clear thumbnail cache when switching to utils mode
    thumbnail_cache_valid = 0;
    
    // Scan js2000 directory for files
    char js2000_path[MAX_PATH_LEN];
    snprintf(js2000_path, sizeof(js2000_path), "%s/js2000", ROMS_PATH);
    
    DIR *dir = opendir(js2000_path);
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL && entry_count < MAX_ENTRIES) {
            if (ent->d_name[0] == '.') continue;  // Skip hidden files
            
            char full_path[MAX_PATH_LEN];
            snprintf(full_path, sizeof(full_path), "%s/%s", js2000_path, ent->d_name);
            
            struct stat st;
            if (stat(full_path, &st) == 0) {
                strncpy(entries[entry_count].name, ent->d_name, sizeof(entries[entry_count].name) - 1);
                strncpy(entries[entry_count].path, full_path, sizeof(entries[entry_count].path) - 1);
                entries[entry_count].is_dir = S_ISDIR(st.st_mode);
                entry_count++;
            }
        }
        closedir(dir);
    }
    
    // Add back entry
    strncpy(entries[entry_count].name, "..", sizeof(entries[entry_count].name) - 1);
    strncpy(entries[entry_count].path, "TOOLS", sizeof(entries[entry_count].path) - 1);
    entries[entry_count].is_dir = 1;
    entry_count++;
    
    // Load thumbnail for initially selected item
    load_current_thumbnail();
    last_selected_index = selected_index;
}

// Show hotkeys screen
static void show_hotkeys_screen(void) {
    // Set current_path for hotkeys mode
    strncpy(current_path, "HOTKEYS", sizeof(current_path) - 1);
    current_path[sizeof(current_path) - 1] = '\0';

    // Clear thumbnail cache and entries for hotkeys mode
    thumbnail_cache_valid = 0;
    entry_count = 0;
    reset_navigation_state();
}

// Show credits screen
static void show_credits_screen(void) {
    // Set current_path for credits mode
    strncpy(current_path, "CREDITS", sizeof(current_path) - 1);
    current_path[sizeof(current_path) - 1] = '\0';
    
    // Clear thumbnail cache and entries for credits mode
    thumbnail_cache_valid = 0;
    entry_count = 0;
    reset_navigation_state();
}

// Scan directory and populate entries
static void scan_directory(const char *path) {
    DIR *dir;
    struct dirent *ent;

    entry_count = 0;
    reset_navigation_state();

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

        // Skip frogui, and saves folders
        if (strcasecmp(ent->d_name, "frogui") == 0 || strcasecmp(ent->d_name, "saves") == 0 || strcasecmp(ent->d_name, "save") == 0) {
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

        // Shift entries down by 1 more to make room for Favorites
        for (int i = entry_count; i > 1; i--) {
            entries[i] = entries[i - 1];
        }

        // Insert Favorites at position 1 (right after Recent games)
        strncpy(entries[1].name, "Favorites", sizeof(entries[1].name) - 1);
        strncpy(entries[1].path, "FAVORITES", sizeof(entries[1].path) - 1);
        entries[1].is_dir = 1;
        entry_count++;

        // Shift entries down by 1 more to make room for Random Game
        for (int i = entry_count; i > 2; i--) {
            entries[i] = entries[i - 1];
        }

        // Insert Random Game at position 2 (right after Favorites)
        strncpy(entries[2].name, "Random game", sizeof(entries[2].name) - 1);
        strncpy(entries[2].path, "RANDOM_GAME", sizeof(entries[2].path) - 1);
        entries[2].is_dir = 1;
        entry_count++;

        // Add Tools at the bottom
        strncpy(entries[entry_count].name, "Tools", sizeof(entries[entry_count].name) - 1);
        strncpy(entries[entry_count].path, "TOOLS", sizeof(entries[entry_count].path) - 1);
        entries[entry_count].is_dir = 1;
        entry_count++;
    }

    // Defer thumbnail loading to first render for faster boot
    // The render loop will handle loading thumbnails on the first frame
    thumbnail_cache_valid = 0;
    last_selected_index = -1;  // Force load on first render
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
            
            // Use unified pillbox rendering
            render_text_pillbox(framebuffer, PADDING, y_value, value_text, COLOR_SELECT_BG, COLOR_SELECT_TEXT, 6);
        } else {
            font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, PADDING, y_value, option->current_value, COLOR_TEXT);
        }
    }
    
    // Draw legend with pillbox highlighting
    const char *legend = " A - SAVE   B - EXIT ";
    int legend_y = SCREEN_HEIGHT - 24;
    
    // Calculate width and position (right-aligned)
    int legend_width = font_measure_text(legend);
    int legend_x = SCREEN_WIDTH - legend_width - 12;
    
    // Draw legend pill with rounded corners
    render_rounded_rect(framebuffer, legend_x - 4, legend_y - 2, legend_width + 8, 20, 10, COLOR_LEGEND_BG);
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, legend_x, legend_y, legend, COLOR_LEGEND);
}

// Render hotkeys screen
static void render_hotkeys_screen() {
    // Draw title
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, PADDING, 10, "HOTKEYS", COLOR_HEADER);

    // Draw hotkey information
    int start_y = 50;
    int line_height = 24;

    // Hotkeys text
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, PADDING, start_y, "SAVE STATE: L + R + X", COLOR_TEXT);
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, PADDING, start_y + line_height, "LOAD STATE: L + R + Y", COLOR_TEXT);
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, PADDING, start_y + line_height * 2, "NEXT SLOT: L + R + >", COLOR_TEXT);
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, PADDING, start_y + line_height * 3, "PREV SLOT: L + R + <", COLOR_TEXT);
    
    // Draw legend
    const char *legend = " B - BACK ";
    int legend_y = SCREEN_HEIGHT - 24;
    int legend_width = font_measure_text(legend);
    int legend_x = SCREEN_WIDTH - legend_width - 12;
    
    render_rounded_rect(framebuffer, legend_x - 4, legend_y - 2, legend_width + 8, 20, 10, COLOR_LEGEND_BG);
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, legend_x, legend_y, legend, COLOR_LEGEND);
}

// Render credits screen
static void render_credits_screen() {
    // Draw title
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, PADDING, 10, "CREDITS", COLOR_HEADER);
    
    // Draw credits information
    int start_y = 50;
    int line_height = 24;
    
    // Credits text with pillboxes for sections
    // FrogUI Dev & Idea section
    const char *section1 = " FrogUI Dev & Idea ";
    int section1_width = font_measure_text(section1);
    render_rounded_rect(framebuffer, PADDING - 4, start_y - 2, section1_width + 8, 20, 10, COLOR_HEADER);
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, PADDING, start_y, section1, COLOR_BG);
    
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, PADDING, start_y + line_height, "Prosty & Desoxyn", COLOR_TEXT);
    
    // Design section
    const char *section2 = " Design ";
    int section2_width = font_measure_text(section2);
    render_rounded_rect(framebuffer, PADDING - 4, start_y + line_height * 2 - 2, section2_width + 8, 20, 10, COLOR_HEADER);
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, PADDING, start_y + line_height * 2, section2, COLOR_BG);
    
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, PADDING, start_y + line_height * 3, "Q_ta", COLOR_TEXT);
    
    // Draw legend
    const char *legend = " B - BACK ";
    int legend_y = SCREEN_HEIGHT - 24;
    int legend_width = font_measure_text(legend);
    int legend_x = SCREEN_WIDTH - legend_width - 12;
    
    render_rounded_rect(framebuffer, legend_x - 4, legend_y - 2, legend_width + 8, 20, 10, COLOR_LEGEND_BG);
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, legend_x, legend_y, legend, COLOR_LEGEND);
}

// Render the menu using modular render system
static void render_menu() {
    render_clear_screen(framebuffer);

    // If game is queued, just show loading screen
    if (game_queued) {
        // Show centered loading pillbox
        const char* loading_text = "LOADING...";
        int text_width = font_measure_text(loading_text);
        int x = (SCREEN_WIDTH - text_width) / 2;
        int y = (SCREEN_HEIGHT - FONT_CHAR_HEIGHT) / 2;
        
        // Use unified pillbox rendering
        render_text_pillbox(framebuffer, x, y, loading_text, theme_header(), theme_bg(), 6);
        return;
    }

    // If settings are active, render settings menu
    if (settings_is_active()) {
        render_settings_menu();
        return;
    }
    
    // If in hotkeys mode, render hotkeys screen
    if (strcmp(current_path, "HOTKEYS") == 0) {
        render_hotkeys_screen();
        return;
    }
    
    // If in credits mode, render credits screen
    if (strcmp(current_path, "CREDITS") == 0) {
        render_credits_screen();
        return;
    }

    // Draw header with current folder name
    const char *display_path = current_path;
    if (strcmp(current_path, ROMS_PATH) == 0) {
        display_path = "FROGUI: SYSTEMS";  // Marketing branding
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

    // Load and display thumbnail for selected item FIRST (background layer)
    // Only reload if selection changed
    if (last_selected_index != selected_index) {
        load_current_thumbnail();
        last_selected_index = selected_index;
        // Reset scrolling state for new selection
        text_scroll_frame_counter = 0;
        text_scroll_offset = 0;
        text_scroll_direction = 1;
    }
    
    if (thumbnail_cache_valid) {
        render_thumbnail(framebuffer, &current_thumbnail);
    }

    // Draw menu entries ON TOP of thumbnail
    for (int i = scroll_offset; i < entry_count && i < scroll_offset + VISIBLE_ENTRIES; i++) {
        // Get display name (with scrolling for selected item)
        char display_name[MAX_FILENAME_DISPLAY_LEN + 4];
        get_scrolling_text(entries[i].name, (i == selected_index), display_name, sizeof(display_name));

        // Check if this item is favorited
        int is_favorited = 0;
        if (!entries[i].is_dir &&
            strcmp(current_path, ROMS_PATH) != 0 &&
            strcmp(current_path, "RECENT_GAMES") != 0 &&
            strcmp(current_path, "FAVORITES") != 0 &&
            strcmp(current_path, "TOOLS") != 0 &&
            strcmp(current_path, "UTILS") != 0 &&
            strcmp(current_path, "HOTKEYS") != 0 &&
            strcmp(current_path, "CREDITS") != 0) {
            const char *core_name = get_basename(current_path);
            const char *filename_path = strrchr(entries[i].path, '/');
            const char *filename = filename_path ? filename_path + 1 : entries[i].name;
            is_favorited = favorites_is_favorited(core_name, filename);
        }

        render_menu_item(framebuffer, i, display_name, entries[i].is_dir,
                        (i == selected_index), scroll_offset, is_favorited);
    }

    // Draw legend - show favorite button only in ROM directories
    int in_rom_dir = (strcmp(current_path, ROMS_PATH) != 0 &&
                      strcmp(current_path, "RECENT_GAMES") != 0 &&
                      strcmp(current_path, "FAVORITES") != 0 &&
                      strcmp(current_path, "TOOLS") != 0 &&
                      strcmp(current_path, "UTILS") != 0 &&
                      strcmp(current_path, "HOTKEYS") != 0 &&
                      strcmp(current_path, "CREDITS") != 0);
    render_legend(framebuffer, in_rom_dir);

    // Draw the "current entry/total entries" label in top-right, above the legend
    char entry_label[20];
    snprintf(entry_label, sizeof(entry_label), "%d/%d", selected_index + 1, entry_count); // 1-based indexing for display
    int label_width = font_measure_text(entry_label);
    int label_x = SCREEN_WIDTH - label_width - 12;  // Right-aligned, just above the legend
    int label_y = 8;  // Position it slightly below the top edge
    render_text_pillbox(framebuffer, label_x, label_y, entry_label, COLOR_LEGEND_BG, COLOR_LEGEND, 6);

    // Draw A-Z picker overlay if active
    if (az_picker_active) {
        // Draw centered background box using theme background color
        int box_width = 280;
        int box_height = 180;
        int box_x = (SCREEN_WIDTH - box_width) / 2;
        int box_y = (SCREEN_HEIGHT - box_height) / 2;
        render_fill_rect(framebuffer, box_x, box_y, box_width, box_height, COLOR_BG);

        // Draw title using theme colors
        const char *title = "QUICK JUMP";
        int title_width = font_measure_text(title);
        int title_x = (SCREEN_WIDTH - title_width) / 2;
        render_text_pillbox(framebuffer, title_x, 30, title, COLOR_SELECT_BG, COLOR_SELECT_TEXT, 6);

        // Draw A-Z grid (7 columns x 4 rows = 28 slots)
        const char *labels[] = {
            "A", "B", "C", "D", "E", "F", "G",
            "H", "I", "J", "K", "L", "M", "N",
            "O", "P", "Q", "R", "S", "T", "U",
            "V", "W", "X", "Y", "Z", "0-9", "#"
        };

        int grid_start_x = 40;
        int grid_start_y = 70;
        int col_width = 38;
        int row_height = 30;

        for (int i = 0; i < 28; i++) {
            int col = i % 7;
            int row = i / 7;
            int x = grid_start_x + col * col_width;
            int y = grid_start_y + row * row_height;

            if (i == az_selected_index) {
                render_text_pillbox(framebuffer, x, y, labels[i], COLOR_SELECT_BG, COLOR_SELECT_TEXT, 6);
            } else {
                font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, x, y, labels[i], COLOR_TEXT);
            }
        }
    }
}

// Pick and launch a random game by randomly navigating the menu
static void pick_random_game(void) {
    printf("Random game: Starting selection...\n");

    int max_attempts = 100; // Prevent infinite loop
    int attempts = 0;

    // Keep trying random selections until we find a file
    while (attempts < max_attempts) {
        attempts++;
        printf("Random game: Attempt %d/%d\n", attempts, max_attempts);

        // First, pick a random console directory from root
        strncpy(current_path, ROMS_PATH, sizeof(current_path) - 1);
        scan_directory(current_path);
        printf("Random game: Scanned root, found %d entries\n", entry_count);

        // Filter out non-console entries (Recent games, Favorites, Random game, Tools)
        int valid_console_count = 0;
        for (int i = 0; i < entry_count; i++) {
            if (entries[i].is_dir &&
                strcmp(entries[i].path, "RECENT_GAMES") != 0 &&
                strcmp(entries[i].path, "FAVORITES") != 0 &&
                strcmp(entries[i].path, "RANDOM_GAME") != 0 &&
                strcmp(entries[i].path, "TOOLS") != 0) {
                valid_console_count++;
            }
        }

        printf("Random game: Found %d valid console directories\n", valid_console_count);
        if (valid_console_count == 0) {
            printf("Random game: No console directories found!\n");
            strncpy(current_path, ROMS_PATH, sizeof(current_path) - 1);
            scan_directory(current_path);
            return;
        }

        // Pick a random console directory
        int random_console = rand() % valid_console_count;
        printf("Random game: Selecting console index %d\n", random_console);

        int console_idx = 0;
        for (int i = 0; i < entry_count; i++) {
            if (entries[i].is_dir &&
                strcmp(entries[i].path, "RECENT_GAMES") != 0 &&
                strcmp(entries[i].path, "FAVORITES") != 0 &&
                strcmp(entries[i].path, "RANDOM_GAME") != 0 &&
                strcmp(entries[i].path, "TOOLS") != 0) {
                if (console_idx == random_console) {
                    strncpy(current_path, entries[i].path, sizeof(current_path) - 1);
                    printf("Random game: Selected console directory: %s\n", entries[i].name);
                    break;
                }
                console_idx++;
            }
        }

        // Scan the console directory
        scan_directory(current_path);
        printf("Random game: Scanned console dir, found %d entries\n", entry_count);

        // Count files (not directories, not ..)
        int file_count = 0;
        for (int i = 0; i < entry_count; i++) {
            if (!entries[i].is_dir && strcmp(entries[i].name, "..") != 0) {
                file_count++;
            }
        }

        printf("Random game: Found %d game files in this directory\n", file_count);
        if (file_count == 0) {
            printf("Random game: No files found, trying another console...\n");
            continue; // No files in this directory, try again
        }

        // Pick a random file
        int random_file = rand() % file_count;
        printf("Random game: Selecting file index %d\n", random_file);

        int file_idx = 0;
        for (int i = 0; i < entry_count; i++) {
            if (!entries[i].is_dir && strcmp(entries[i].name, "..") != 0) {
                if (file_idx == random_file) {
                    // Found our random game! Launch it
                    const char *core_name = get_basename(current_path);
                    const char *filename_path = strrchr(entries[i].path, '/');
                    const char *filename = filename_path ? filename_path + 1 : entries[i].name;

                    printf("Random game: Selected game: %s (core: %s)\n", filename, core_name);
                    printf("Random game: Full path: %s\n", entries[i].path);

                    // Launch the game - match format used by normal game selection
                    sprintf((char *)ptr_gs_run_game_file, "%s;%s;%s.gba", core_name, core_name, filename); // TODO: Replace second core_name with full directory (besides /mnt/sda1) and seperate core_name from directory
                    sprintf((char *)ptr_gs_run_folder, "/mnt/sda1/ROMS");
                    sprintf((char *)ptr_gs_run_game_name, "%s", filename);

                    // Remove extension
                    char *dot_position = strrchr(ptr_gs_run_game_name, '.');
                    if (dot_position != NULL) {
                        *dot_position = '\0';
                    }

                    printf("Random game: Launching with game_file=%s\n", ptr_gs_run_game_file);
                    printf("Random game: folder=%s, game_name=%s\n", ptr_gs_run_folder, ptr_gs_run_game_name);

                    // Add to recent history
                    recent_games_add(core_name, filename, entries[i].path);

                    game_queued = true;
                    printf("Random game: Game queued for launch!\n");
                    return;
                }
                file_idx++;
            }
        }
    }

    // If we get here, we couldn't find a game after max_attempts
    printf("Random game: Failed to find a game after %d attempts\n", max_attempts);
    strncpy(current_path, ROMS_PATH, sizeof(current_path) - 1);
    scan_directory(current_path);
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
    int x = input_state_cb(0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_X);
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

    // Handle A-Z picker input
    if (az_picker_active) {
        // Navigate the A-Z grid
        if (prev_input[0] && !up) { // UP
            if (az_selected_index >= 7) az_selected_index -= 7;
        }
        if (prev_input[1] && !down) { // DOWN
            if (az_selected_index < 21) az_selected_index += 7;
        }
        if (prev_input[7] && !left) { // LEFT
            if (az_selected_index > 0) az_selected_index--;
        }
        if (prev_input[8] && !right) { // RIGHT
            if (az_selected_index < 27) az_selected_index++;
        }

        // A button - select letter and jump
        if (prev_input[2] && !a) {
            const char *search_chars[] = {
                "A", "B", "C", "D", "E", "F", "G",
                "H", "I", "J", "K", "L", "M", "N",
                "O", "P", "Q", "R", "S", "T", "U",
                "V", "W", "X", "Y", "Z", "0", ""
            };

            char first_char = search_chars[az_selected_index][0];

            // Find first entry starting with this letter (case insensitive)
            for (int i = 0; i < entry_count; i++) {
                char entry_first = entries[i].name[0];
                if (entry_first >= 'a' && entry_first <= 'z') {
                    entry_first = entry_first - 'a' + 'A'; // Convert to uppercase
                }

                // Handle 0-9 category
                if (az_selected_index == 26 && entry_first >= '0' && entry_first <= '9') {
                    selected_index = i;
                    break;
                }
                // Handle # category (special characters)
                else if (az_selected_index == 27 &&
                         !((entry_first >= 'A' && entry_first <= 'Z') ||
                           (entry_first >= '0' && entry_first <= '9'))) {
                    selected_index = i;
                    break;
                }
                // Handle A-Z
                else if (az_selected_index < 26 && entry_first == first_char) {
                    selected_index = i;
                    break;
                }
            }

            az_picker_active = 0;
        }

        // B button - cancel
        if (prev_input[3] && !b) {
            az_picker_active = 0;
        }

        // Update prev_input and return (picker consumed input)
        prev_input[0] = up;
        prev_input[1] = down;
        prev_input[2] = a;
        prev_input[3] = b;
        prev_input[7] = left;
        prev_input[8] = right;
        return;
    }

    // Handle RIGHT button to open A-Z picker (on button release)
    if (prev_input[8] && !right) {
        // Don't activate in special menus
        if (strcmp(current_path, "RECENT_GAMES") != 0 &&
            strcmp(current_path, "FAVORITES") != 0 &&
            strcmp(current_path, "TOOLS") != 0 &&
            strcmp(current_path, "UTILS") != 0 &&
            strcmp(current_path, "HOTKEYS") != 0 &&
            strcmp(current_path, "CREDITS") != 0 &&
            entry_count > 0) {
            az_picker_active = 1;
            az_selected_index = 0;
        }
    }

    // Handle SELECT button to open settings (on button release)
    if (prev_input[6] && !select) {
        if (strcmp(current_path, ROMS_PATH) == 0) {
            // Main menu settings - reload and show multicore.opt
            settings_load();
            settings_show_menu();
        } else {
            // Check if we're in a console folder that has core-specific settings
            char console_folder[256];
            const char *slash = strrchr(current_path, '/');
            if (slash && slash != current_path) {
                // Extract folder name from path like "/mnt/sda1/ROMS/gb"
                strcpy(console_folder, slash + 1);
                const char *core_name = get_core_name_for_console(console_folder);
                if (core_name) {
                    // Show core-specific settings
                    show_core_settings(core_name);
                }
            }
        }
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

    // Handle L button (move up by 7 entries)
    if (prev_input[4] && !l) {
        if (selected_index >= 7) {
            selected_index -= 7;
        } else {
            // Loop to the bottom when reaching the top
            selected_index = entry_count - (7 - selected_index);
        }
        // Adjust scroll_offset if necessary
        if (selected_index < scroll_offset) {
            scroll_offset = selected_index;
        }
    }

    // Handle R button (move down by 7 entries)
    if (prev_input[5] && !r) {
        if (selected_index < entry_count - 7) {
            selected_index += 7;
        } else {
            // Loop to the top when reaching the bottom
            selected_index = (selected_index + 7) % entry_count;  // Wrap around to the top
        }
        // Adjust scroll_offset if necessary
        if (selected_index >= scroll_offset + VISIBLE_ENTRIES) {
            scroll_offset = selected_index - VISIBLE_ENTRIES + 1;
        }
    }

    // Handle X button (toggle favorite) - on button release
    if (prev_input[9] && !x && entry_count > 0) {
        MenuEntry *entry = &entries[selected_index];

        // Only allow favoriting in ROM directories (not in special menus)
        if (!entry->is_dir &&
            strcmp(current_path, "RECENT_GAMES") != 0 &&
            strcmp(current_path, "FAVORITES") != 0 &&
            strcmp(current_path, "TOOLS") != 0 &&
            strcmp(current_path, "UTILS") != 0 &&
            strcmp(current_path, "HOTKEYS") != 0 &&
            strcmp(current_path, "CREDITS") != 0 &&
            strcmp(current_path, ROMS_PATH) != 0) {

            // Get core name and filename
            const char *core_name = get_basename(current_path);
            const char *filename_path = strrchr(entry->path, '/');
            const char *filename = filename_path ? filename_path + 1 : entry->name;

            // Toggle favorite
            favorites_toggle(core_name, filename, entry->path);
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
            } else if (strcmp(entry->path, "FAVORITES") == 0) {
                // Show favorites list
                show_favorites();
                strncpy(current_path, "FAVORITES", sizeof(current_path) - 1);
            } else if (strcmp(entry->path, "RANDOM_GAME") == 0) {
                // Pick and launch a random game
                pick_random_game();
                return;
            } else if (strcmp(entry->path, "TOOLS") == 0) {
                // Show tools menu
                show_tools_menu();
                strncpy(current_path, "TOOLS", sizeof(current_path) - 1);
            } else if (strcmp(entry->path, "HOTKEYS") == 0) {
                // Show hotkeys screen
                show_hotkeys_screen();
                strncpy(current_path, "HOTKEYS", sizeof(current_path) - 1);
            } else if (strcmp(entry->path, "CREDITS") == 0) {
                // Show credits screen
                show_credits_screen();
                strncpy(current_path, "CREDITS", sizeof(current_path) - 1);
            } else if (strcmp(entry->path, "UTILS") == 0) {
                // Show utils menu
                show_utils_menu();
                strncpy(current_path, "UTILS", sizeof(current_path) - 1);
            } else {
                strncpy(current_path, entry->path, sizeof(current_path) - 1);
                scan_directory(current_path);
            }
        } else {
            // File selected - try to launch it
            const char *core_name;
            const char *filename;
            
            // Check if we're in Utils - launch js2000 core
            if (strcmp(current_path, "UTILS") == 0) {
                // Launch selected file with js2000 core using format: corename;full_path
                sprintf((char *)ptr_gs_run_game_file, "js2000;js2000;%s.gba", entry->name);
                sprintf((char *)ptr_gs_run_folder, "/mnt/sda1/ROMS");
                sprintf((char *)ptr_gs_run_game_name, "%s", entry->name);

                // Remove extension from game name
                char *dot_position = strrchr(ptr_gs_run_game_name, '.');
                if (dot_position != NULL) {
                    *dot_position = '\0';
                }

                game_queued = true; // Pass to retro_run, can only load the core from there
                return;
            }
            
            // Check if we're in Recent games
            if (strcmp(current_path, "RECENT_GAMES") == 0) {
                // Parse core_name;game_name from entry->path
                char *separator = strchr(entry->path, ';');
                if (separator) {
                    *separator = '\0';
                    core_name = entry->path;
                    filename = separator + 1;

                    // For recent games, get the full_path from the RecentGame structure
                    const RecentGame* recent_list = recent_games_get_list();
                    int recent_count = recent_games_get_count();
                    const char* full_path = "";

                    for (int i = 0; i < recent_count; i++) {
                        if (strcmp(recent_list[i].core_name, core_name) == 0 &&
                            strcmp(recent_list[i].game_name, filename) == 0) {
                            full_path = recent_list[i].full_path;
                            break;
                        }
                    }

                    // Add to recent history (moves to top) - use actual full path
                    recent_games_add(core_name, filename, full_path);
                } else {
                    return; // Invalid format
                }
            } else if (strcmp(current_path, "FAVORITES") == 0) {
                // Parse core_name;game_name from entry->path
                char *separator = strchr(entry->path, ';');
                if (separator) {
                    *separator = '\0';
                    core_name = entry->path;
                    filename = separator + 1;

                    // For favorites, get the full_path from the FavoriteGame structure
                    const FavoriteGame* favorites_list = favorites_get_list();
                    int favorites_count = favorites_get_count();
                    const char* full_path = "";

                    for (int i = 0; i < favorites_count; i++) {
                        if (strcmp(favorites_list[i].core_name, core_name) == 0 &&
                            strcmp(favorites_list[i].game_name, filename) == 0) {
                            full_path = favorites_list[i].full_path;
                            break;
                        }
                    }

                    // Add to recent history when launching from favorites
                    recent_games_add(core_name, filename, full_path);
                } else {
                    return; // Invalid format
                }
            } else {
                // Extract core name from parent directory
                core_name = get_basename(current_path);
                const char *filename_path = strrchr(entry->path, '/');
                filename = filename_path ? filename_path + 1 : entry->name;
                
                // Add to recent history - use full entry path
                recent_games_add(core_name, filename, entry->path);
            }

            sprintf((char *)ptr_gs_run_game_file, "%s;%s;%s.gba", core_name, core_name, filename); // TODO: Replace second core_name with full directory (besides /mnt/sda1) and seperate core_name from directory
            sprintf((char *)ptr_gs_run_folder, "/mnt/sda1/ROMS"); // Expects "/mnt/sda1/ROMS" format
            sprintf((char *)ptr_gs_run_game_name, "%s", filename); // Expects the filename without any extension 

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
        } else if (strcmp(current_path, "FAVORITES") == 0) {
            // Go back from Favorites to main ROMS directory
            strncpy(current_path, ROMS_PATH, sizeof(current_path) - 1);
            scan_directory(current_path);
        } else if (strcmp(current_path, "TOOLS") == 0) {
            // Go back from Tools to main ROMS directory
            strncpy(current_path, ROMS_PATH, sizeof(current_path) - 1);
            scan_directory(current_path);
        } else if (strcmp(current_path, "HOTKEYS") == 0) {
            // Go back from Hotkeys to Tools
            show_tools_menu();
            strncpy(current_path, "TOOLS", sizeof(current_path) - 1);
        } else if (strcmp(current_path, "CREDITS") == 0) {
            // Go back from Credits to Tools
            show_tools_menu();
            strncpy(current_path, "TOOLS", sizeof(current_path) - 1);
        } else if (strcmp(current_path, "UTILS") == 0) {
            // Go back from Utils to Tools
            show_tools_menu();
            strncpy(current_path, "TOOLS", sizeof(current_path) - 1);
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
    prev_input[9] = x;
}

// Libretro API implementation
void retro_init(void) {
    framebuffer = (uint16_t*)malloc(SCREEN_WIDTH * SCREEN_HEIGHT * sizeof(uint16_t));

    // Seed random number generator for random game picker
    srand(time(NULL));

    // Initialize modular systems
    render_init(framebuffer);
    font_init();
    theme_init();
    recent_games_init();
    favorites_init();
    settings_init();

    recent_games_load();
    favorites_load();
    settings_load();

    // Auto-launch most recent game if resume on boot is enabled
    auto_launch_recent_game();

    // Skip directory scan if we're auto-launching a game (faster boot)
    if (!game_queued) {
        strncpy(current_path, ROMS_PATH, sizeof(current_path) - 1);
        scan_directory(current_path);
    }
}

void retro_deinit(void) {
    // Free thumbnail cache
    if (thumbnail_cache_valid) {
        free_thumbnail(&current_thumbnail);
        thumbnail_cache_valid = 0;
    }
    
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
    info->library_name     = "FrogUI";
    info->library_version  = "0.1";
    info->need_fullpath    = false;
    info->valid_extensions = "frogui";
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
