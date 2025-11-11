#ifndef MENU_H
#define MENU_H

#define MAX_ENTRIES 256
#define MAX_PATH_LEN 512
#define ROMS_PATH "/mnt/sda1/ROMS"

// Menu entry structure
typedef struct {
    char path[MAX_PATH_LEN];
    char name[256];
    int is_dir;
} MenuEntry;

// Menu state structure
typedef struct {
    MenuEntry entries[MAX_ENTRIES];
    int entry_count;
    int selected_index;
    int scroll_offset;
    char current_path[MAX_PATH_LEN];
} MenuState;

// Initialize menu system
void menu_init(MenuState *menu);

// Scan directory and populate entries
void menu_scan_directory(MenuState *menu, const char *path);

// Show recent games list
void menu_show_recent_games(MenuState *menu);

// Handle navigation input
void menu_handle_input(MenuState *menu, int up, int down, int left, int right, int a, int b);

// Render the menu
void menu_render(MenuState *menu, uint16_t *framebuffer);

// Get current selected entry
const MenuEntry* menu_get_selected_entry(MenuState *menu);

// Check if directory is a special path
int menu_is_special_path(const char *path);

#endif // MENU_H