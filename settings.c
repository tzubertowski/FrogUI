#include "settings.h"
#include "theme.h"
#include "font.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SettingsOption settings[MAX_SETTINGS];
static int settings_count = 0;
static int settings_active = 0;
static int settings_selected = 0;
static int settings_scroll_offset = 0;

// Track current config file being edited
static char current_config_path[512] = "/mnt/sda1/configs/multicore.opt";

// Forward declarations
static int settings_load_file(const char *config_path);
static void apply_theme_from_settings(void);
static void apply_font_from_settings(void);

void settings_init(void) {
    settings_count = 0;
    settings_active = 0;
    settings_selected = 0;
    settings_scroll_offset = 0;
}

// Parse a line like: ### [option_name] :[current] :[val1|val2|val3]
static int parse_option_line(const char *line, SettingsOption *option) {
    if (strncmp(line, "### [", 5) != 0) return 0;
    
    // Find option name
    const char *name_start = line + 5;
    const char *name_end = strchr(name_start, ']');
    if (!name_end) return 0;
    
    int name_len = name_end - name_start;
    if (name_len >= MAX_OPTION_NAME_LEN) return 0;
    
    strncpy(option->name, name_start, name_len);
    option->name[name_len] = '\0';
    
    // Find current value (between first : and next :)
    const char *current_start = strchr(name_end, ':');
    if (!current_start) return 0;
    current_start++; // Skip ':'
    
    if (*current_start == '[') current_start++; // Skip '[' if present
    
    const char *current_end = strchr(current_start, ']');
    if (!current_end) return 0;
    
    int current_len = current_end - current_start;
    if (current_len >= MAX_OPTION_VALUE_LEN) return 0;
    
    strncpy(option->current_value, current_start, current_len);
    option->current_value[current_len] = '\0';
    
    // Find possible values (between [ and ])
    const char *values_start = strchr(current_end, '[');
    if (!values_start) return 0;
    values_start++; // Skip '['
    
    const char *values_end = strchr(values_start, ']');
    if (!values_end) return 0;
    
    // Parse pipe-separated values
    option->value_count = 0;
    option->current_index = 0;
    
    char values_str[512];
    int values_len = values_end - values_start;
    if (values_len >= sizeof(values_str)) return 0;
    
    strncpy(values_str, values_start, values_len);
    values_str[values_len] = '\0';
    
    char *token = strtok(values_str, "|");
    while (token && option->value_count < MAX_OPTION_VALUES) {
        strncpy(option->possible_values[option->value_count], token, MAX_OPTION_VALUE_LEN - 1);
        option->possible_values[option->value_count][MAX_OPTION_VALUE_LEN - 1] = '\0';
        
        // Check if this is the current value
        if (strcmp(token, option->current_value) == 0) {
            option->current_index = option->value_count;
        }
        
        option->value_count++;
        token = strtok(NULL, "|");
    }
    
    return 1;
}

int settings_load(void) {
    // For testing on dev machine, use sdcard path if it exists  
    const char *config_path = "/mnt/sda1/configs/multicore.opt";
    FILE *test = fopen("/app/sdcard/configs/multicore.opt", "r");
    if (test) {
        fclose(test);
        config_path = "/app/sdcard/configs/multicore.opt";
    }
    
    strncpy(current_config_path, config_path, sizeof(current_config_path) - 1);
    return settings_load_file(config_path);
}

// Load core-specific settings
int settings_load_core(const char *core_name) {
    char config_path[512];
    
    // Try sdcard path first for dev machines - note the subdirectory structure!
    snprintf(config_path, sizeof(config_path), "/app/sdcard/configs/%s/%s.opt", core_name, core_name);
    FILE *test = fopen(config_path, "r");
    if (test) {
        fclose(test);
        strncpy(current_config_path, config_path, sizeof(current_config_path) - 1);
        return settings_load_file(config_path);
    }
    
    // Use standard path with subdirectory structure
    snprintf(config_path, sizeof(config_path), "/mnt/sda1/configs/%s/%s.opt", core_name, core_name);
    strncpy(current_config_path, config_path, sizeof(current_config_path) - 1);
    return settings_load_file(config_path);
}

// Common settings loading function
static int settings_load_file(const char *config_path) {
    
    FILE *fp = fopen(config_path, "r");
    if (!fp) return 0;
    
    char line[512];
    settings_count = 0;
    
    // First pass: parse comment lines to get options
    while (fgets(line, sizeof(line), fp) && settings_count < MAX_SETTINGS) {
        // Remove newline
        line[strcspn(line, "\r\n")] = 0;
        
        // Parse comment lines that define options
        if (parse_option_line(line, &settings[settings_count])) {
            settings_count++;
        }
    }
    
    // Second pass: read actual current values from setting lines
    rewind(fp);
    while (fgets(line, sizeof(line), fp)) {
        // Remove newline
        line[strcspn(line, "\r\n")] = 0;
        
        // Skip comment lines
        if (strncmp(line, "###", 3) == 0) continue;
        
        // Parse setting lines (option_name = "value")
        char *equals = strchr(line, '=');
        if (equals) {
            *equals = '\0';
            char *option_name = line;
            char *value_start = equals + 1;
            
            // Trim whitespace from option name
            while (*option_name == ' ' || *option_name == '\t') option_name++;
            char *end = option_name + strlen(option_name) - 1;
            while (end > option_name && (*end == ' ' || *end == '\t')) end--;
            *(end + 1) = '\0';
            
            // Trim whitespace and quotes from value
            while (*value_start == ' ' || *value_start == '\t' || *value_start == '"') value_start++;
            end = value_start + strlen(value_start) - 1;
            while (end > value_start && (*end == ' ' || *end == '\t' || *end == '"')) end--;
            *(end + 1) = '\0';
            
            // Find matching option and update its current value
            for (int i = 0; i < settings_count; i++) {
                if (strcmp(settings[i].name, option_name) == 0) {
                    strncpy(settings[i].current_value, value_start, MAX_OPTION_VALUE_LEN - 1);
                    settings[i].current_value[MAX_OPTION_VALUE_LEN - 1] = '\0';
                    
                    // Update current_index to match the new value
                    for (int j = 0; j < settings[i].value_count; j++) {
                        if (strcmp(settings[i].possible_values[j], value_start) == 0) {
                            settings[i].current_index = j;
                            break;
                        }
                    }
                    break;
                }
            }
        }
    }
    
    fclose(fp);

    // Apply theme and font changes after loading settings
    apply_theme_from_settings();
    apply_font_from_settings();

    return settings_count;
}

int settings_save(void) {
    // Use the current config path that was set during load
    const char *config_path = current_config_path;
    char temp_path[512];
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", config_path);
    
    FILE *fp_read = fopen(config_path, "r");
    if (!fp_read) {
        return 0;
    }
    
    // Create temporary file
    FILE *fp_write = fopen(temp_path, "w");
    if (!fp_write) {
        fclose(fp_read);
        return 0;
    }
    
    char line[512];
    while (fgets(line, sizeof(line), fp_read)) {
        // Check if this is a config line (not comment)
        if (strncmp(line, "###", 3) == 0) {
            // Keep comment lines as-is
            fputs(line, fp_write);
        } else {
            // Check if this is a setting we need to update
            char *equals = strchr(line, '=');
            if (equals) {
                *equals = '\0';
                char *option_name = line;
                
                // Trim whitespace
                while (*option_name == ' ' || *option_name == '\t') option_name++;
                char *end = option_name + strlen(option_name) - 1;
                while (end > option_name && (*end == ' ' || *end == '\t')) end--;
                *(end + 1) = '\0';
                
                // Find matching setting
                int found = 0;
                for (int i = 0; i < settings_count; i++) {
                    if (strcmp(settings[i].name, option_name) == 0) {
                        fprintf(fp_write, "%s = \"%s\"\n", option_name, 
                               settings[i].possible_values[settings[i].current_index]);
                        found = 1;
                        break;
                    }
                }
                
                if (!found) {
                    // Restore original line
                    *equals = '=';
                    fputs(line, fp_write);
                    // Make sure line has newline
                    if (!strchr(line, '\n')) {
                        fputc('\n', fp_write);
                    }
                }
            } else {
                fputs(line, fp_write);
                // Make sure line has newline
                if (!strchr(line, '\n')) {
                    fputc('\n', fp_write);
                }
            }
        }
    }
    
    fclose(fp_read);
    fclose(fp_write);
    
    // Replace original file using copy approach (more reliable on embedded systems)
    FILE *src = fopen(temp_path, "r");
    FILE *dst = fopen(config_path, "w");
    
    if (src && dst) {
        char buffer[1024];
        size_t bytes;
        while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
            fwrite(buffer, 1, bytes, dst);
        }
        fclose(src);
        fclose(dst);
        remove(temp_path);  // Delete temp file
    } else {
        if (src) fclose(src);
        if (dst) fclose(dst);
        return 0;
    }

    // Apply theme and font changes after saving settings
    apply_theme_from_settings();
    apply_font_from_settings();

    return 1;
}

int settings_get_count(void) {
    return settings_count;
}

const SettingsOption* settings_get_option(int index) {
    if (index < 0 || index >= settings_count) return NULL;
    return &settings[index];
}

void settings_cycle_option(int index) {
    if (index < 0 || index >= settings_count) return;
    
    settings[index].current_index = (settings[index].current_index + 1) % settings[index].value_count;
    strncpy(settings[index].current_value, 
           settings[index].possible_values[settings[index].current_index], 
           MAX_OPTION_VALUE_LEN - 1);
    settings[index].current_value[MAX_OPTION_VALUE_LEN - 1] = '\0';
}

void settings_show_menu(void) {
    settings_active = 1;
    settings_selected = 0;
    settings_scroll_offset = 0;
}

int settings_handle_input(int up, int down, int left, int right, int a, int b, int select) {
    if (!settings_active) return 0;
    
    int max_visible = 3; // Reduced to ensure no overlap with legend
    
    if (up) {
        if (settings_selected > 0) {
            settings_selected--;
        } else {
            settings_selected = settings_count - 1;
        }
        
        // Adjust scroll offset
        if (settings_selected < settings_scroll_offset) {
            settings_scroll_offset = settings_selected;
        } else if (settings_selected >= settings_scroll_offset + max_visible) {
            settings_scroll_offset = settings_selected - max_visible + 1;
        }
        return 1;
    }
    
    if (down) {
        if (settings_selected < settings_count - 1) {
            settings_selected++;
        } else {
            settings_selected = 0;
        }
        
        // Adjust scroll offset
        if (settings_selected < settings_scroll_offset) {
            settings_scroll_offset = settings_selected;
        } else if (settings_selected >= settings_scroll_offset + max_visible) {
            settings_scroll_offset = settings_selected - max_visible + 1;
        }
        return 1;
    }
    
    if (right) {
        // Cycle to next value
        settings_cycle_option(settings_selected);
        return 1;
    }
    
    if (left) {
        // Cycle to previous value
        if (settings_selected >= 0 && settings_selected < settings_count) {
            SettingsOption *option = &settings[settings_selected];
            option->current_index = (option->current_index - 1 + option->value_count) % option->value_count;
            strncpy(option->current_value, option->possible_values[option->current_index], MAX_OPTION_VALUE_LEN - 1);
            option->current_value[MAX_OPTION_VALUE_LEN - 1] = '\0';
        }
        return 1;
    }
    
    if (a) {
        // Save settings and exit
        settings_save();
        settings_active = 0;
        return 1;
    }
    
    if (b) {
        // Exit and save (B button should save like most settings menus)
        settings_save();
        settings_active = 0;
        return 1;
    }
    
    return 1; // Consumed input
}

int settings_is_active(void) {
    return settings_active;
}

int settings_get_selected_index(void) {
    return settings_selected;
}

int settings_get_scroll_offset(void) {
    return settings_scroll_offset;
}

// Apply theme changes from loaded settings
static void apply_theme_from_settings(void) {
    // Look for the frogui_theme setting
    for (int i = 0; i < settings_count; i++) {
        if (strcmp(settings[i].name, "frogui_theme") == 0) {
            theme_load_from_settings(settings[i].current_value);
            break;
        }
    }
}

// Apply font changes from loaded settings
static void apply_font_from_settings(void) {
    // Look for the frogui_font setting
    for (int i = 0; i < settings_count; i++) {
        if (strcmp(settings[i].name, "frogui_font") == 0) {
            // Debug logging
            FILE *log = fopen("/app/log.txt", "a");
            if (log) {
                fprintf(log, "[SETTINGS] Applying font: %s\n", settings[i].current_value);
                fclose(log);
            }
            font_load_from_settings(settings[i].current_value);
            break;
        }
    }
}

// Get setting value by name
const char* settings_get_value(const char *setting_name) {
    for (int i = 0; i < settings_count; i++) {
        if (strcmp(settings[i].name, setting_name) == 0) {
            return settings[i].current_value;
        }
    }
    return NULL;
}