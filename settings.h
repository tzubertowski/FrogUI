#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>

#define MAX_SETTINGS 32
#define MAX_OPTION_NAME_LEN 64
#define MAX_OPTION_VALUE_LEN 32
#define MAX_OPTION_VALUES 32

// Settings option structure
typedef struct {
    char name[MAX_OPTION_NAME_LEN];
    char current_value[MAX_OPTION_VALUE_LEN];
    char possible_values[MAX_OPTION_VALUES][MAX_OPTION_VALUE_LEN];
    int value_count;
    int current_index;
} SettingsOption;

// Initialize settings system
void settings_init(void);

// Load settings from multicore.opt
int settings_load(void);

// Load core-specific settings (e.g., Gambatte.opt)
int settings_load_core(const char *core_name);

// Save settings to multicore.opt
int settings_save(void);

// Get settings count
int settings_get_count(void);

// Get settings option by index
const SettingsOption* settings_get_option(int index);

// Set option to next value (cycles through possible values)
void settings_cycle_option(int index);

// Show settings menu
void settings_show_menu(void);

// Handle settings menu input
int settings_handle_input(int up, int down, int left, int right, int a, int b, int y);

// Check if we're in settings mode
int settings_is_active(void);

// Get currently selected option index
int settings_get_selected_index(void);

// Get current scroll offset
int settings_get_scroll_offset(void);

// Get setting value by name (returns NULL if not found)
const char* settings_get_value(const char *setting_name);

// Reset settings to defaults from default_configs directory
int settings_reset_to_defaults(void);

// Check if settings are currently being saved
int settings_is_saving(void);

#endif // SETTINGS_H