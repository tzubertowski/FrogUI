#include "render_theme.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SCHEME_LINE_MAX 256
#define COLOUR_BIT(n) (1u << (n))

static RenderTheme current;
static RenderTheme base_theme;
static int active_screen = -1;
static RenderTheme parsing_base;
static int parse_int(const char *s, int min, int max, int *out);

static RenderColour alpha_backdrop(RenderColour colour) {
    switch (colour) {
    case RENDER_COLOUR_HEADER_BACKGROUND:
    case RENDER_COLOUR_LIST_BACKGROUND:
    case RENDER_COLOUR_LEGEND_BACKGROUND:
    case RENDER_COLOUR_DISABLED:
        return RENDER_COLOUR_BACKGROUND;
    case RENDER_COLOUR_HEADER:
        return RENDER_COLOUR_HEADER_BACKGROUND;
    case RENDER_COLOUR_TEXT:
        return RENDER_COLOUR_LIST_BACKGROUND;
    case RENDER_COLOUR_SELECTION:
        return RENDER_COLOUR_LIST_BACKGROUND;
    case RENDER_COLOUR_SELECTION_TEXT:
        return RENDER_COLOUR_SELECTION;
    case RENDER_COLOUR_LEGEND:
        return RENDER_COLOUR_LEGEND_BACKGROUND;
    default:
        return RENDER_COLOUR_BACKGROUND;
    }
}

static int apply_muos_alpha(const char *section, const char *key, const char *value) {
    static const struct { const char *section, *key; RenderColour colour; } alphas[] = {
        {"background", "background_alpha", RENDER_COLOUR_BACKGROUND},
        {"header", "header_background_alpha", RENDER_COLOUR_HEADER_BACKGROUND},
        {"header", "header_text_alpha", RENDER_COLOUR_HEADER},
        {"footer", "footer_background_alpha", RENDER_COLOUR_LEGEND_BACKGROUND},
        {"footer", "footer_text_alpha", RENDER_COLOUR_LEGEND},
        {"list", "list_default_text_alpha", RENDER_COLOUR_TEXT},
        {"list", "list_default_background_alpha", RENDER_COLOUR_LIST_BACKGROUND},
        {"list", "list_focus_background_alpha", RENDER_COLOUR_SELECTION},
        {"list", "list_focus_text_alpha", RENDER_COLOUR_SELECTION_TEXT},
        {"list", "list_disabled_text_alpha", RENDER_COLOUR_DISABLED},
    };
    for (size_t i = 0; i < sizeof alphas / sizeof alphas[0]; i++) {
        int alpha;
        if (strcmp(section, alphas[i].section) != 0 || strcmp(key, alphas[i].key) != 0)
            continue;
        if (!parse_int(value, 0, 255, &alpha)) return 1;
        current.colour_alpha[alphas[i].colour] = (uint8_t)alpha;
        if (alpha == 0) {
            RenderColour colour = alphas[i].colour;
            if (colour == RENDER_COLOUR_BACKGROUND) return 1;
            if (colour == RENDER_COLOUR_HEADER_BACKGROUND ||
                colour == RENDER_COLOUR_LIST_BACKGROUND ||
                colour == RENDER_COLOUR_SELECTION ||
                colour == RENDER_COLOUR_LEGEND_BACKGROUND) {
                RenderColour backdrop = alpha_backdrop(colour);
                if (current.colour_mask & COLOUR_BIT(backdrop))
                    current.colours[colour] = current.colours[backdrop];
                else if (parsing_base.colour_mask & COLOUR_BIT(backdrop))
                    current.colours[colour] = parsing_base.colours[backdrop];
                current.colour_mask |= COLOUR_BIT(colour);
            } else if (parsing_base.colour_mask & COLOUR_BIT(colour)) {
                current.colours[colour] = parsing_base.colours[colour];
                current.colour_mask |= COLOUR_BIT(colour);
            }
        }
        return 1;
    }
    return 0;
}

static char *trim(char *s) {
    while (isspace((unsigned char)*s)) s++;
    size_t n = strlen(s);
    while (n && isspace((unsigned char)s[n - 1])) s[--n] = '\0';
    return s;
}

static int parse_int(const char *s, int min, int max, int *out) {
    char *end;
    long value = strtol(s, &end, 10);
    if (end == s || *trim(end) || value < min || value > max) return 0;
    *out = (int)value;
    return 1;
}

static int parse_colour(const char *s, uint16_t *out) {
    if (*s == '#') s++;
    else if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    if (strlen(s) != 6) return 0;
    unsigned value = 0;
    for (int i = 0; i < 6; i++) {
        unsigned char c = (unsigned char)s[i];
        if (!isxdigit(c)) return 0;
        value = value * 16 + (isdigit(c) ? c - '0' : tolower(c) - 'a' + 10);
    }
    unsigned r = (value >> 16) & 255, g = (value >> 8) & 255, b = value & 255;
    *out = (uint16_t)(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
    return 1;
}

static int colour_key(const char *key) {
    static const char *keys[RENDER_COLOUR_COUNT] = {
        "background", "header_background", "list_background", "text", "selection", "selection_text", "header",
        "folder", "legend", "legend_background", "disabled"
    };
    for (int i = 0; i < RENDER_COLOUR_COUNT; i++)
        if (strcmp(key, keys[i]) == 0) return i;
    return -1;
}

static void apply_muos_line(const char *section, const char *key, const char *value) {
    if (apply_muos_alpha(section, key, value)) return;
    static const struct { const char *section, *key; RenderColour colour; } colours[] = {
        {"background", "background", RENDER_COLOUR_BACKGROUND},
        {"header", "header_background", RENDER_COLOUR_HEADER_BACKGROUND},
        {"header", "header_text", RENDER_COLOUR_HEADER},
        {"footer", "footer_background", RENDER_COLOUR_LEGEND_BACKGROUND},
        {"footer", "footer_text", RENDER_COLOUR_LEGEND},
        {"list", "list_default_text", RENDER_COLOUR_TEXT},
        {"list", "list_default_background", RENDER_COLOUR_LIST_BACKGROUND},
        {"list", "list_default_glyph_recolour", RENDER_COLOUR_FOLDER},
        {"list", "list_focus_background", RENDER_COLOUR_SELECTION},
        {"list", "list_focus_text", RENDER_COLOUR_SELECTION_TEXT},
        {"list", "list_focus_glyph_recolour", RENDER_COLOUR_FOLDER},
        {"list", "list_disabled_text", RENDER_COLOUR_DISABLED},
    };
    for (size_t i = 0; i < sizeof colours / sizeof colours[0]; i++) {
        uint16_t colour;
        if (strcmp(section, colours[i].section) == 0 && strcmp(key, colours[i].key) == 0 &&
            parse_colour(value, &colour)) {
            current.colours[colours[i].colour] = colour;
            current.colour_alpha[colours[i].colour] = 255;
            current.colour_mask |= COLOUR_BIT(colours[i].colour);
            return;
        }
    }

    static const struct { const char *key; RenderGridColour colour; } grid_colours[] = {
        {"cell_default_background", RENDER_GRID_BACKGROUND},
        {"cell_focus_background", RENDER_GRID_FOCUS_BACKGROUND},
        {"cell_default_text", RENDER_GRID_TEXT},
        {"cell_focus_text", RENDER_GRID_FOCUS_TEXT},
    };
    if (strcmp(section, "grid") == 0) {
        for (size_t i = 0; i < sizeof grid_colours / sizeof grid_colours[0]; i++) {
            uint16_t colour;
            if (strcmp(key, grid_colours[i].key) == 0 && parse_colour(value, &colour)) {
                current.grid_colours[grid_colours[i].colour] = colour;
                current.grid_colour_mask |= 1u << grid_colours[i].colour;
                return;
            }
        }
    }
    if (strcmp(section, "list") == 0 && strcmp(key, "list_default_glyph_pad_left") == 0) {
        (void)parse_int(value, 0, 64, &current.list_glyph_padding_left);
        return;
    }

    if (strcmp(section, "frogui") == 0 &&
        (strcmp(key, "font_file") == 0 || strcmp(key, "background_image") == 0)) {
        char *dst = strcmp(key, "font_file") == 0 ? current.font_file : current.background_image;
        size_t cap = strcmp(key, "font_file") == 0 ? sizeof current.font_file : sizeof current.background_image;
        size_t n = strlen(value);
        if (n == 0 || n >= cap) return;
        for (size_t i = 0; i < n; i++)
            if (!(isalnum((unsigned char)value[i]) || value[i] == '_' || value[i] == '-' ||
                  value[i] == '.' || value[i] == ' ')) return;
        if (strstr(value, "..")) return;
        memcpy(dst, value, n + 1);
        return;
    }

    if (strcmp(section, "frogui") == 0 && strcmp(key, "home_layout") == 0) {
        current.home_layout = strcmp(value, "switch") == 0 ? RENDER_HOME_SWITCH :
                              strcmp(value, "catos") == 0 ? RENDER_HOME_CATOS :
                              strcmp(value, "vertical") == 0 ? RENDER_HOME_VERTICAL :
                              RENDER_HOME_DEFAULT;
        return;
    }

    int *field = NULL, min = 1, max = 100;
    if (strcmp(section, "header") == 0 && strcmp(key, "header_height") == 0) {
        field = &current.header_height; max = 80;
    } else if (strcmp(section, "footer") == 0 && strcmp(key, "footer_height") == 0) {
        field = &current.legend_height; max = 80;
    } else if (strcmp(section, "font") == 0 && strcmp(key, "font_list_size") == 0) {
        field = &current.font_size; min = 18; max = 26;
    } else if (strcmp(section, "misc") == 0 && strcmp(key, "content_item_count") == 0) {
        field = &current.visible_rows; max = 20;
    } else if (strcmp(section, "misc") == 0 && strcmp(key, "content_item_height") == 0) {
        field = &current.row_height; max = 100;
    } else if (strcmp(section, "misc") == 0 && strcmp(key, "content_padding_left") == 0) {
        field = &current.horizontal_padding; min = 0; max = 64;
    } else if (strcmp(section, "misc") == 0 && strcmp(key, "content_padding_top") == 0) {
        field = &current.content_top; min = 0; max = 120;
    } else if (strcmp(section, "misc") == 0 && strcmp(key, "content_width") == 0) {
        field = &current.content_width; min = 0; max = 640;
    } else if (strcmp(section, "misc") == 0 && strcmp(key, "content_height") == 0) {
        field = &current.content_height; min = 0; max = 480;
    } else if (strcmp(section, "misc") == 0 && strcmp(key, "content_alignment") == 0) {
        field = &current.content_alignment; min = 0; max = 2;
    } else if (strcmp(section, "font") == 0 && strcmp(key, "font_list_pad_left") == 0) {
        field = &current.list_padding_left; min = 0; max = 64;
    } else if (strcmp(section, "font") == 0 && strcmp(key, "font_list_pad_top") == 0) {
        field = &current.list_padding_top; min = 0; max = 64;
    } else if (strcmp(section, "font") == 0 && strcmp(key, "font_list_pad_bottom") == 0) {
        field = &current.list_padding_bottom; min = 0; max = 64;
    } else if (strcmp(section, "list") == 0 && strcmp(key, "list_focus_radius") == 0) {
        field = &current.selected_radius; min = 0; max = 32;
    } else if (strcmp(section, "grid") == 0 && strcmp(key, "column_count") == 0) {
        field = &current.grid_columns; max = 8;
    } else if (strcmp(section, "grid") == 0 && strcmp(key, "row_count") == 0) {
        field = &current.grid_rows; max = 8;
    } else if (strcmp(section, "grid") == 0 && strcmp(key, "column_padding") == 0) {
        field = &current.grid_column_gap; min = 0; max = 64;
    } else if (strcmp(section, "grid") == 0 && strcmp(key, "row_padding") == 0) {
        field = &current.grid_row_gap; min = 0; max = 64;
    } else if (strcmp(section, "grid") == 0 && strcmp(key, "cell_width") == 0) {
        field = &current.grid_cell_width; max = 480;
    } else if (strcmp(section, "grid") == 0 && strcmp(key, "cell_height") == 0) {
        field = &current.grid_cell_height; max = 320;
    } else if (strcmp(section, "grid") == 0 && strcmp(key, "cell_radius") == 0) {
        field = &current.grid_cell_radius; min = 0; max = 32;
    } else if (strcmp(section, "grid") == 0 && strcmp(key, "cell_image_padding_top") == 0) {
        field = &current.grid_image_padding_top; min = 0; max = 64;
    }
    if (field) (void)parse_int(value, min, max, field);
}

static void apply_line(const char *section, char *key, char *value) {
    static const char *geometry[] = {
        "header_height", "row_height", "horizontal_padding", "content_top",
        "content_width", "content_height", "content_alignment",
        "list_padding_left", "list_glyph_padding_left", "list_padding_top", "list_padding_bottom",
        "legend_height", "selected_radius",
        "thumbnail_width", "thumbnail_height"
    };
    static const int minimum[] = { 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1 };
    static const int maximum[] = { 80, 100, 64, 120, 640, 480, 2, 64, 64, 64, 64, 80, 32, 512, 512 };
    int *fields[] = {
        &current.header_height, &current.row_height, &current.horizontal_padding,
        &current.content_top, &current.content_width, &current.content_height,
        &current.content_alignment, &current.list_padding_left, &current.list_glyph_padding_left,
        &current.list_padding_top,
        &current.list_padding_bottom, &current.legend_height, &current.selected_radius,
        &current.thumbnail_width, &current.thumbnail_height
    };
    if (strcmp(section, "renderer") == 0) {
        for (int i = 0; i < (int)(sizeof geometry / sizeof geometry[0]); i++)
            if (strcmp(key, geometry[i]) == 0) {
                (void)parse_int(value, minimum[i], maximum[i], fields[i]);
                return;
            }
    } else if (strcmp(section, "colours") == 0) {
        int i = colour_key(key);
        uint16_t colour;
        if (i >= 0 && parse_colour(value, &colour)) {
            current.colours[i] = colour;
            current.colour_alpha[i] = 255;
            current.colour_mask |= COLOUR_BIT(i);
        }
    }
    apply_muos_line(section, key, value);
}

static void lowercase(char *s) {
    for (; *s; s++) *s = (char)tolower((unsigned char)*s);
}

static void parse_scheme(FILE *file) {
    char line[SCHEME_LINE_MAX], section[32] = "";
    size_t total = 0;
    parsing_base = current;
    while (fgets(line, sizeof line, file)) {
        total += strlen(line);
        if (total > 8192) return;
        if (!strchr(line, '\n') && !feof(file)) {
            int c;
            while ((c = fgetc(file)) != '\n' && c != EOF) {}
            continue;
        }
        char *s = trim(line);
        if (!*s || *s == '#' || *s == ';') continue;
        size_t len = strlen(s);
        if (*s == '[' && len > 2 && s[len - 1] == ']') {
            s[len - 1] = '\0';
            snprintf(section, sizeof section, "%s", trim(s + 1));
            lowercase(section);
            continue;
        }
        char *equals = strchr(s, '=');
        if (!equals) continue;
        *equals = '\0';
        char *key = trim(s), *value = trim(equals + 1);
        lowercase(key);
        apply_line(section, key, value);
    }
}

static int valid_style_name(const char *name) {
    if (!name || !*name) return 0;
    for (const unsigned char *p = (const unsigned char *)name; *p; p++)
        if (!(isalnum(*p) || *p == '_' || *p == '-')) return 0;
    return 1;
}

static void load_file(const char *name) {
    char path[192];
    if (!valid_style_name(name) ||
        snprintf(path, sizeof path, "/mnt/sdcard/frogui/schemes/%s.ini", name) >= (int)sizeof path)
        return;
    FILE *file = fopen(path, "r");
    if (!file && snprintf(path, sizeof path, "schemes/%s.ini", name) < (int)sizeof path)
        file = fopen(path, "r");
    if (!file) return;
    parse_scheme(file);
    fclose(file);
}

static void load_screen_file(const char *name) {
    char path[256];
    const char *theme = current.scheme_name[0] ? current.scheme_name : current.style_name;
    if (!valid_style_name(theme) || !valid_style_name(name)) return;
    int n = snprintf(path, sizeof path,
                     "/mnt/sdcard/frogui/schemes/%s/640x480/scheme/%s.ini", theme, name);
    FILE *file = n >= 0 && (size_t)n < sizeof path ? fopen(path, "r") : NULL;
    if (!file) {
        n = snprintf(path, sizeof path, "schemes/%s/640x480/scheme/%s.ini", theme, name);
        if (n >= 0 && (size_t)n < sizeof path) file = fopen(path, "r");
    }
    if (!file) return;
    parse_scheme(file);
    fclose(file);
}

void render_theme_load(const char *style_name, const char *scheme_name) {
    memset(&current, 0, sizeof current);
    current.header_height = 30;
    current.row_height = 32;
    current.horizontal_padding = 16;
    current.content_top = 40;
    current.content_width = 0;
    current.content_height = 0;
    current.content_alignment = 0;
    current.legend_height = 26;
    current.selected_radius = 6;
    current.thumbnail_width = 250;
    current.thumbnail_height = 250;
    current.font_size = 0;
    current.visible_rows = 0;
    current.grid_column_gap = 8;
    current.grid_row_gap = 8;
    current.grid_cell_radius = 10;
    current.grid_image_padding_top = 5;
    current.list_padding_left = 35;
    current.list_glyph_padding_left = 5;
    if (style_name) snprintf(current.style_name, sizeof current.style_name, "%s", style_name);
    if (style_name && strcmp(style_name, "prominimal") == 0)
        current.selected_radius = 5;
    load_file(style_name);
    if (scheme_name && valid_style_name(scheme_name) && strcmp(scheme_name, style_name) != 0) {
        snprintf(current.scheme_name, sizeof current.scheme_name, "%s", scheme_name);
        load_file(scheme_name);
    }
    base_theme = current;
    active_screen = -1;
    render_theme_set_screen(RENDER_SCREEN_BROWSER);
}

void render_theme_set_screen(RenderThemeScreen screen) {
    static const char *const screen_files[] = {
        "muxplore", "muxlaunch", "muxplore", "default", "default"
    };
    if (screen < RENDER_SCREEN_BROWSER || screen > RENDER_SCREEN_SETTINGS)
        screen = RENDER_SCREEN_BROWSER;
    if ((int)screen == active_screen) return;
    active_screen = screen;
    current = base_theme;
    load_screen_file(screen_files[screen]);
}

const RenderTheme *render_theme_current(void) { return &current; }

int render_theme_font_size(void) { return current.font_size; }
const char *render_theme_font_file(void) { return current.font_file; }

int render_theme_background(char *path, unsigned path_size) {
    if (!path || !path_size || !current.background_image[0]) return 0;
    int n = snprintf(path, path_size, "/mnt/sdcard/frogui/schemes/%s/%s",
                     current.scheme_name[0] ? current.scheme_name : current.style_name,
                     current.background_image);
    return n >= 0 && (unsigned)n < path_size;
}

uint16_t render_theme_colour(RenderColour colour, uint16_t palette_default) {
    if (colour < 0 || colour >= RENDER_COLOUR_COUNT ||
        !(current.colour_mask & COLOUR_BIT(colour))) return palette_default;
    return current.colours[colour];
}

uint8_t render_theme_colour_alpha(RenderColour colour) {
    return colour < 0 || colour >= RENDER_COLOUR_COUNT ||
           !(current.colour_mask & COLOUR_BIT(colour))
         ? 255 : current.colour_alpha[colour];
}

uint16_t render_theme_grid_colour(RenderGridColour colour, uint16_t default_colour) {
    if (colour < 0 || colour >= RENDER_GRID_COLOUR_COUNT ||
        !(current.grid_colour_mask & (1u << colour))) return default_colour;
    return current.grid_colours[colour];
}

int render_theme_asset_path(char *path, unsigned path_size, const char *relative_path) {
    if (!path || !path_size || !relative_path || !*relative_path) return 0;
    for (const unsigned char *p = (const unsigned char *)relative_path; *p; p++)
        if (!(isalnum(*p) || *p == '_' || *p == '-' || *p == '.' || *p == '/')) return 0;
    if (strstr(relative_path, "..")) return 0;
    const char *name = current.scheme_name[0] ? current.scheme_name : current.style_name;
    int n = snprintf(path, path_size, "/mnt/sdcard/frogui/schemes/%s/%s", name, relative_path);
    if (n >= 0 && (unsigned)n < path_size && access(path, R_OK) == 0) return 1;
    n = snprintf(path, path_size, "schemes/%s/%s", name, relative_path);
    return n >= 0 && (unsigned)n < path_size && access(path, R_OK) == 0;
}
