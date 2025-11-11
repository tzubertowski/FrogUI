#include "settings.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static SettingsOption settings[MAX_SETTINGS];
static int settings_count = 0;
static int settings_active = 0;
static int settings_selected = 0;
static int settings_scroll_offset = 0;

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
    FILE *fp = fopen("/mnt/sda1/configs/multicore.opt", "r");
    if (!fp) return 0;
    
    char line[512];
    settings_count = 0;
    
    while (fgets(line, sizeof(line), fp) && settings_count < MAX_SETTINGS) {
        // Remove newline
        line[strcspn(line, "\r\n")] = 0;
        
        // Parse comment lines that define options
        if (parse_option_line(line, &settings[settings_count])) {
            settings_count++;
        }
    }
    
    fclose(fp);
    return settings_count;
}

int settings_save(void) {
    // Read current file to preserve structure
    FILE *fp_read = fopen("/mnt/sda1/configs/multicore.opt", "r");
    if (!fp_read) return 0;
    
    // Create temporary file
    FILE *fp_write = fopen("/mnt/sda1/configs/multicore.opt.tmp", "w");
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
                }
            } else {
                fputs(line, fp_write);
            }
        }
    }
    
    fclose(fp_read);
    fclose(fp_write);
    
    // Replace original file
    rename("/mnt/sda1/configs/multicore.opt.tmp", "/mnt/sda1/configs/multicore.opt");
    
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

int settings_handle_input(int up, int down, int left, int right, int b, int select) {
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
    
    if (b || select) {
        // Save settings and exit
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