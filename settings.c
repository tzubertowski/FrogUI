#include "settings.h"
#include "theme.h"
#include "font.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef SF2000
#include "../../debug.h"
#else
// For non-SF2000 builds, use printf as fallback
#define xlog printf
#endif

static SettingsOption settings[MAX_SETTINGS];
static int settings_count = 0;
static int settings_active = 0;
static int settings_selected = 0;
static int settings_scroll_offset = 0;
static int settings_saving = 0;  // Flag to indicate save in progress

// Track current config file being edited
static char current_config_path[512] = "";

// Auto-detect config directory based on platform
static const char* get_config_directory(void) {
	static const char *config_dir = NULL;
	if (!config_dir) {
		// Check if GB300 config directory exists
		if (access("/mnt/sda1/cores/config", 0) == 0) {
			config_dir = "/mnt/sda1/cores/config";
		} else {
			// Fall back to SF2000 structure
			config_dir = "/mnt/sda1/configs";
		}
	}
	return config_dir;
}

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
    if (!name_end) {
        xlog("Settings: Parse fail - no closing ] for name\n");
        return 0;
    }

    int name_len = name_end - name_start;
    if (name_len >= MAX_OPTION_NAME_LEN) {
        xlog("Settings: Parse fail - name too long: %d\n", name_len);
        return 0;
    }

    strncpy(option->name, name_start, name_len);
    option->name[name_len] = '\0';

    // Find current value (between first : and next ])
    const char *current_start = strchr(name_end, ':');
    if (!current_start) {
        xlog("Settings: Parse fail [%s] - no colon after name\n", option->name);
        return 0;
    }
    current_start++; // Skip ':'

    // Skip whitespace and '[' if present
    while (*current_start == ' ' || *current_start == '\t' || *current_start == '[') current_start++;

    const char *current_end = strchr(current_start, ']');
    if (!current_end) {
        xlog("Settings: Parse fail [%s] - no ] after current value\n", option->name);
        return 0;
    }

    // Trim trailing whitespace from current value
    while (current_end > current_start && (*(current_end - 1) == ' ' || *(current_end - 1) == '\t')) {
        current_end--;
    }

    int current_len = current_end - current_start;
    if (current_len >= MAX_OPTION_VALUE_LEN || current_len <= 0) {
        xlog("Settings: Parse fail [%s] - current value len invalid: %d\n", option->name, current_len);
        return 0;
    }

    strncpy(option->current_value, current_start, current_len);
    option->current_value[current_len] = '\0';
    
    // Find possible values (between [ and last ])
    const char *values_start = strchr(current_end, '[');
    if (!values_start) {
        xlog("Settings: Parse fail [%s] - no [ for values list\n", option->name);
        return 0;
    }
    values_start++; // Skip '['

    // Find the LAST ']' on the line (not the first one)
    const char *values_end = strrchr(values_start, ']');
    if (!values_end) {
        xlog("Settings: Parse fail [%s] - no closing ] for values list\n", option->name);
        return 0;
    }

    // Parse pipe-separated values
    option->value_count = 0;
    option->current_index = 0;

    char values_str[16384];  // Increased to 16KB to handle large palette lists
    int values_len = values_end - values_start;
    if (values_len >= sizeof(values_str)) {
        xlog("Settings: Parse fail [%s] - value list too long (%d bytes, max %d)\n",
             option->name, values_len, (int)sizeof(values_str));
        return 0;
    }
    
    strncpy(values_str, values_start, values_len);
    values_str[values_len] = '\0';
    
    char *token = strtok(values_str, "|");
    while (token && option->value_count < MAX_OPTION_VALUES) {
        // Trim leading whitespace
        while (*token == ' ' || *token == '\t') token++;

        // Trim trailing whitespace
        char *end = token + strlen(token) - 1;
        while (end > token && (*end == ' ' || *end == '\t')) {
            *end = '\0';
            end--;
        }

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
    char config_path[512];

    // For testing on dev machine, use sdcard path if it exists
    FILE *test = fopen("/app/sdcard/configs/multicore.opt", "r");
    if (test) {
        fclose(test);
        snprintf(config_path, sizeof(config_path), "/app/sdcard/configs/multicore.opt");
        xlog("Settings: Using dev path: %s\n", config_path);
    } else {
        // Try SF2000 location first: /mnt/sda1/configs/multicore.opt
        snprintf(config_path, sizeof(config_path), "/mnt/sda1/configs/multicore.opt");
        test = fopen(config_path, "r");
        if (test) {
            fclose(test);
            xlog("Settings: Using SF2000 path: %s\n", config_path);
        } else {
            // Fall back to GB300 location: /mnt/sda1/cores/config/multicore.opt
            snprintf(config_path, sizeof(config_path), "/mnt/sda1/cores/config/multicore.opt");
            xlog("Settings: Using GB300 path: %s\n", config_path);
        }
    }

    strncpy(current_config_path, config_path, sizeof(current_config_path) - 1);
    int result = settings_load_file(config_path);
    xlog("Settings: Loaded %d settings from multicore.opt\n", result);
    return result;
}

// Load core-specific settings
int settings_load_core(const char *core_name) {
    char config_path[512];
    char core_name_lower[256];

    xlog("Settings: Loading core settings for: %s\n", core_name);

    // Create lowercase version of core name
    strncpy(core_name_lower, core_name, sizeof(core_name_lower) - 1);
    core_name_lower[sizeof(core_name_lower) - 1] = '\0';
    for (int i = 0; core_name_lower[i]; i++) {
        if (core_name_lower[i] >= 'A' && core_name_lower[i] <= 'Z') {
            core_name_lower[i] = core_name_lower[i] + 32;
        }
    }

    // Try sdcard path first for dev machines - note the subdirectory structure!
    // Try lowercase directory name first (more common)
    snprintf(config_path, sizeof(config_path), "/app/sdcard/configs/%s/%s.opt", core_name_lower, core_name);
    FILE *test = fopen(config_path, "r");
    if (test) {
        fclose(test);
        xlog("Settings: Found config at: %s\n", config_path);
        strncpy(current_config_path, config_path, sizeof(current_config_path) - 1);
        int result = settings_load_file(config_path);
        return result;
    }

    // Try capitalized directory name
    snprintf(config_path, sizeof(config_path), "/app/sdcard/configs/%s/%s.opt", core_name, core_name);
    test = fopen(config_path, "r");
    if (test) {
        fclose(test);
        xlog("Settings: Found config at: %s\n", config_path);
        strncpy(current_config_path, config_path, sizeof(current_config_path) - 1);
        int result = settings_load_file(config_path);
        return result;
    }

    // Try GB300 structure first: /cores/config/{core}.opt
    const char *base_dir = get_config_directory();
    snprintf(config_path, sizeof(config_path), "%s/%s.opt", base_dir, core_name);
    test = fopen(config_path, "r");
    if (test) {
        fclose(test);
        xlog("Settings: Found config at: %s\n", config_path);
        strncpy(current_config_path, config_path, sizeof(current_config_path) - 1);
        int result = settings_load_file(config_path);
        return result;
    }

    // Fall back to SF2000 structure: /configs/{core}/{core}.opt (lowercase dir)
    snprintf(config_path, sizeof(config_path), "%s/%s/%s.opt", base_dir, core_name_lower, core_name);
    test = fopen(config_path, "r");
    if (test) {
        fclose(test);
        xlog("Settings: Found config at: %s\n", config_path);
        strncpy(current_config_path, config_path, sizeof(current_config_path) - 1);
        int result = settings_load_file(config_path);
        return result;
    }

    // Final fallback to SF2000 structure with capitalized dir
    snprintf(config_path, sizeof(config_path), "%s/%s/%s.opt", base_dir, core_name, core_name);
    strncpy(current_config_path, config_path, sizeof(current_config_path) - 1);
    int result = settings_load_file(config_path);
    xlog("Settings: Loaded %d settings from core config (final attempt)\n", result);
    return result;
}

// Common settings loading function
static int settings_load_file(const char *config_path) {
    xlog("Settings: Loading file: %s\n", config_path);

    FILE *fp = fopen(config_path, "rb");  // Binary mode to prevent newline translation
    if (!fp) {
        xlog("Settings: Failed to open file\n");
        return 0;
    }

    // Read entire file into memory to bypass FILE buffering issues
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char *file_contents = (char*)malloc(file_size + 1);
    if (!file_contents) {
        xlog("Settings: Failed to allocate memory for file\n");
        fclose(fp);
        return 0;
    }

    size_t bytes_read = fread(file_contents, 1, file_size, fp);
    file_contents[bytes_read] = '\0';
    fclose(fp);

    char line[16384];
    settings_count = 0;

    // Parse lines from memory
    char *line_start = file_contents;
    char *line_end;

    while (line_start < file_contents + bytes_read && settings_count < MAX_SETTINGS) {
        // Find end of line
        line_end = line_start;
        while (line_end < file_contents + bytes_read && *line_end != '\n' && *line_end != '\r') {
            line_end++;
        }

        // Copy line to buffer
        int line_len = line_end - line_start;
        if (line_len > 0 && line_len < (int)sizeof(line)) {
            memcpy(line, line_start, line_len);
            line[line_len] = '\0';

            // Parse comment lines that define options
            if (parse_option_line(line, &settings[settings_count])) {
                settings_count++;
            } else if (strncmp(line, "### [", 5) == 0) {
                xlog("Settings: Failed to parse line (len=%d): %.80s...\n", line_len, line);
            }
        }

        // Skip past line ending characters (\r, \n, or \r\n)
        while (line_end < file_contents + bytes_read && (*line_end == '\n' || *line_end == '\r')) {
            line_end++;
        }
        line_start = line_end;
    }

    xlog("Settings: First pass parsed %d options\n", settings_count);

    // Second pass: parse actual current values from setting lines
    line_start = file_contents;
    while (line_start < file_contents + bytes_read && settings_count < MAX_SETTINGS) {
        // Find end of line
        line_end = line_start;
        while (line_end < file_contents + bytes_read && *line_end != '\n' && *line_end != '\r') {
            line_end++;
        }

        // Copy line to buffer
        int line_len = line_end - line_start;
        if (line_len > 0 && line_len < (int)sizeof(line)) {
            memcpy(line, line_start, line_len);
            line[line_len] = '\0';

            // Skip comment lines
            if (strncmp(line, "###", 3) != 0) {
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
        }

        // Skip past line ending characters
        while (line_end < file_contents + bytes_read && (*line_end == '\n' || *line_end == '\r')) {
            line_end++;
        }
        line_start = line_end;
    }

    free(file_contents);

    // Apply theme and font changes after loading settings
    apply_theme_from_settings();
    apply_font_from_settings();

    return settings_count;
}

int settings_save(void) {
    settings_saving = 1;  // Set saving flag to prevent premature exit

    // Use the current config path that was set during load
    const char *config_path = current_config_path;
    char temp_path[512];
    snprintf(temp_path, sizeof(temp_path), "%s.tmp", config_path);

    FILE *fp_read = fopen(config_path, "r");
    if (!fp_read) {
        settings_saving = 0;
        return 0;
    }

    // Create temporary file
    FILE *fp_write = fopen(temp_path, "w");
    if (!fp_write) {
        fclose(fp_read);
        settings_saving = 0;
        return 0;
    }

    char line[512];
    int write_error = 0;

    while (fgets(line, sizeof(line), fp_read)) {
        // Check if this is a config line (not comment)
        if (strncmp(line, "###", 3) == 0) {
            // Keep comment lines as-is
            if (fputs(line, fp_write) == EOF) {
                write_error = 1;
                break;
            }
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
                        if (fprintf(fp_write, "%s = \"%s\"\n", option_name,
                                   settings[i].possible_values[settings[i].current_index]) < 0) {
                            write_error = 1;
                            break;
                        }
                        found = 1;
                        break;
                    }
                }

                if (write_error) break;

                if (!found) {
                    // Restore original line
                    *equals = '=';
                    if (fputs(line, fp_write) == EOF) {
                        write_error = 1;
                        break;
                    }
                    // Make sure line has newline
                    if (!strchr(line, '\n')) {
                        if (fputc('\n', fp_write) == EOF) {
                            write_error = 1;
                            break;
                        }
                    }
                }
            } else {
                if (fputs(line, fp_write) == EOF) {
                    write_error = 1;
                    break;
                }
                // Make sure line has newline
                if (!strchr(line, '\n')) {
                    if (fputc('\n', fp_write) == EOF) {
                        write_error = 1;
                        break;
                    }
                }
            }
        }
    }

    fclose(fp_read);

    // Flush to ensure data is written to disk
    if (!write_error) {
        if (fflush(fp_write) != 0) {
            write_error = 1;
        }
    }

    fclose(fp_write);

    // If there was a write error, remove temp file and abort
    if (write_error) {
        remove(temp_path);
        settings_saving = 0;
        return 0;
    }

    // Atomically replace original file with temp file using rename
    // This is atomic on most filesystems and safer than copying
    if (rename(temp_path, config_path) != 0) {
        // If rename fails, fall back to copy method
        FILE *src = fopen(temp_path, "r");
        FILE *dst = fopen(config_path, "w");

        if (src && dst) {
            char buffer[1024];
            size_t bytes;
            int copy_error = 0;
            while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
                if (fwrite(buffer, 1, bytes, dst) != bytes) {
                    copy_error = 1;
                    break;
                }
            }

            if (!copy_error) {
                fflush(dst);
            }

            fclose(src);
            fclose(dst);

            if (!copy_error) {
                remove(temp_path);
            } else {
                settings_saving = 0;
                return 0;
            }
        } else {
            if (src) fclose(src);
            if (dst) fclose(dst);
            settings_saving = 0;
            return 0;
        }
    }

    settings_saving = 0;

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

int settings_handle_input(int up, int down, int left, int right, int a, int b, int y) {
    if (!settings_active) return 0;

    // Don't allow any input while saving is in progress
    if (settings_saving) return 1;

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

    if (y) {
        // Reset to defaults
        if (settings_reset_to_defaults()) {
            // Successfully reset, settings are reloaded automatically
            // Stay in settings menu to show the reset values
        }
        return 1;
    }

    if (a) {
        // Save settings and exit (don't exit until save completes)
        if (settings_save()) {
            settings_active = 0;
        }
        return 1;
    }

    if (b) {
        // Exit and save (B button should save like most settings menus)
        if (settings_save()) {
            settings_active = 0;
        }
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

// Get default configs directory - always use /mnt/sda1/default_configs
static const char* get_default_config_directory(void) {
    return "/mnt/sda1/default_configs";
}

// Reset settings to defaults by copying from default_configs
int settings_reset_to_defaults(void) {
    xlog("Settings: Reset to defaults called\n");

    if (current_config_path[0] == '\0') {
        xlog("Settings: No config path set, aborting reset\n");
        return 0;  // No config file loaded
    }

    settings_saving = 1;  // Set saving flag
    xlog("Settings: Current config path: %s\n", current_config_path);

    // Determine the default config path
    // Defaults are ALWAYS in nested structure: /mnt/sda1/default_configs/{coreName}/{coreName}.opt
    char default_path[512];
    const char *default_base = get_default_config_directory();

    // Extract core name from filename
    const char *filename = strrchr(current_config_path, '/');
    if (filename) {
        filename++;  // Skip '/'
    } else {
        filename = current_config_path;
    }

    // Get core name by removing .opt extension
    char core_name[256];
    strncpy(core_name, filename, sizeof(core_name) - 1);
    core_name[sizeof(core_name) - 1] = '\0';
    char *ext = strstr(core_name, ".opt");
    if (ext) {
        *ext = '\0';
    }

    // Build path: /mnt/sda1/default_configs/{coreName}/{coreName}.opt
    // Exception: multicore.opt stays flat
    if (strcmp(core_name, "multicore") == 0) {
        snprintf(default_path, sizeof(default_path), "%s/multicore.opt", default_base);
    } else {
        snprintf(default_path, sizeof(default_path), "%s/%s/%s.opt", default_base, core_name, core_name);
    }

    xlog("Settings: Looking for default at: %s\n", default_path);

    // Try to open default config file
    FILE *default_file = fopen(default_path, "r");
    if (!default_file) {
        xlog("Settings: Default config not found\n");
        settings_saving = 0;
        return 0;  // Default config doesn't exist
    }

    xlog("Settings: Found default config, overwriting current config...\n");

    // Open current config for writing (this truncates the file)
    FILE *dest_file = fopen(current_config_path, "w");
    if (!dest_file) {
        xlog("Settings: Failed to open config for writing\n");
        fclose(default_file);
        settings_saving = 0;
        return 0;
    }

    // Copy default config directly to current config
    char buffer[1024];
    size_t bytes;
    int copy_error = 0;
    while ((bytes = fread(buffer, 1, sizeof(buffer), default_file)) > 0) {
        if (fwrite(buffer, 1, bytes, dest_file) != bytes) {
            copy_error = 1;
            xlog("Settings: Write error during copy\n");
            break;
        }
    }

    fclose(default_file);

    // Flush to ensure data is written
    if (!copy_error) {
        if (fflush(dest_file) != 0) {
            copy_error = 1;
            xlog("Settings: Flush error\n");
        }
    }

    fclose(dest_file);

    if (copy_error) {
        xlog("Settings: Copy failed\n");
        settings_saving = 0;
        return 0;
    }

    xlog("Settings: Overwrite successful\n");

    settings_saving = 0;

    xlog("Settings: Reset successful, reloading settings\n");

    // Reload settings from the reset file
    settings_load_file(current_config_path);

    // Reset UI state to show the reloaded settings
    settings_selected = 0;
    settings_scroll_offset = 0;

    xlog("Settings: Reset complete, %d settings loaded\n", settings_count);

    return 1;
}

// Check if settings are currently being saved
int settings_is_saving(void) {
    return settings_saving;
}