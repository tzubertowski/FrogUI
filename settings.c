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
    // For testing on dev machine, use sdcard path if it exists  
    const char *config_path = "/mnt/sda1/configs/multicore.opt";
    FILE *test = fopen("/app/sdcard/configs/multicore.opt", "r");
    if (test) {
        fclose(test);
        config_path = "/app/sdcard/configs/multicore.opt";
    }
    
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
    return settings_count;
}

int settings_save(void) {
    extern void xlog(const char *fmt, ...);
    xlog("[FrogOS Settings] === settings_save() started ===\n");
    
    // Read current file to preserve structure
    // For testing on dev machine, use sdcard path if it exists
    const char *config_path = "/mnt/sda1/configs/multicore.opt";
    const char *temp_path = "/mnt/sda1/configs/multicore.opt.tmp";
    FILE *test = fopen("/app/sdcard/configs/multicore.opt", "r");
    if (test) {
        fclose(test);
        config_path = "/app/sdcard/configs/multicore.opt";
        temp_path = "/app/sdcard/configs/multicore.opt.tmp";
        xlog("[FrogOS Settings] Using dev path: %s\n", config_path);
    } else {
        xlog("[FrogOS Settings] Using device path: %s\n", config_path);
    }
    
    FILE *fp_read = fopen(config_path, "r");
    if (!fp_read) {
        // Try without configs subdirectory for testing
        config_path = "/mnt/sda1/multicore.opt";
        temp_path = "/mnt/sda1/multicore.opt.tmp";
        fp_read = fopen(config_path, "r");
        if (!fp_read) {
            xlog("[FrogOS Settings] ERROR: Could not open config file\n");
            return 0;
        }
        xlog("[FrogOS Settings] Using fallback path: %s\n", config_path);
    }
    
    // Create temporary file
    FILE *fp_write = fopen(temp_path, "w");
    if (!fp_write) {
        xlog("[FrogOS Settings] ERROR: Could not create temp file: %s\n", temp_path);
        fclose(fp_read);
        return 0;
    }
    
    xlog("[FrogOS Settings] Opened files successfully, processing...\n");
    
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
                        xlog("[FrogOS Settings] Updating %s from '%s' to '%s'\n", 
                             option_name, settings[i].current_value,
                             settings[i].possible_values[settings[i].current_index]);
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
        xlog("[FrogOS Settings] File save successful\n");
    } else {
        if (src) fclose(src);
        if (dst) fclose(dst);
        xlog("[FrogOS Settings] ERROR: Could not copy temp file to config\n");
        return 0;
    }
    
    xlog("[FrogOS Settings] === settings_save() completed ===\n");
    
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
    
    extern void xlog(const char *fmt, ...);
    xlog("[FrogOS Settings] Cycling option %d (%s) from index %d to ", 
         index, settings[index].name, settings[index].current_index);
    
    settings[index].current_index = (settings[index].current_index + 1) % settings[index].value_count;
    strncpy(settings[index].current_value, 
           settings[index].possible_values[settings[index].current_index], 
           MAX_OPTION_VALUE_LEN - 1);
    settings[index].current_value[MAX_OPTION_VALUE_LEN - 1] = '\0';
    
    xlog("%d (%s)\n", settings[index].current_index, settings[index].current_value);
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
        extern void xlog(const char *fmt, ...);
        xlog("[FrogOS Settings] A button pressed, calling settings_save()\n");
        int save_result = settings_save();
        xlog("[FrogOS Settings] settings_save() returned: %d\n", save_result);
        settings_active = 0;
        return 1;
    }
    
    if (b) {
        // Exit without saving
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