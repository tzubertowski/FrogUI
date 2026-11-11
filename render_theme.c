#include "render_theme.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SCHEME_LINE_MAX 256
#define COLOUR_BIT(n) (1u << (n))

static RenderTheme current;

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
        "background", "text", "selection", "selection_text", "header",
        "folder", "legend", "legend_background", "disabled"
    };
    for (int i = 0; i < RENDER_COLOUR_COUNT; i++)
        if (strcmp(key, keys[i]) == 0) return i;
    return -1;
}

static void apply_line(const char *section, char *key, char *value) {
    static const char *geometry[] = {
        "header_height", "row_height", "horizontal_padding", "content_top",
        "legend_height", "selected_radius",
        "thumbnail_width", "thumbnail_height"
    };
    static const int minimum[] = { 1, 1, 0, 0, 1, 0, 1, 1 };
    static const int maximum[] = { 80, 100, 64, 120, 80, 32, 512, 512 };
    int *fields[] = {
        &current.header_height, &current.row_height, &current.horizontal_padding,
        &current.content_top, &current.legend_height, &current.selected_radius,
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
            current.colour_mask |= COLOUR_BIT(i);
        }
    }
}

static void parse_scheme(FILE *file) {
    char line[SCHEME_LINE_MAX], section[32] = "";
    size_t total = 0;
    while (fgets(line, sizeof line, file)) {
        total += strlen(line);
        if (total > 4096) return;
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
            continue;
        }
        char *equals = strchr(s, '=');
        if (!equals) continue;
        *equals = '\0';
        apply_line(section, trim(s), trim(equals + 1));
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

void render_theme_load(const char *style_name) {
    memset(&current, 0, sizeof current);
    current.header_height = 30;
    current.row_height = 32;
    current.horizontal_padding = 16;
    current.content_top = 40;
    current.legend_height = 26;
    current.selected_radius = 6;
    current.thumbnail_width = 250;
    current.thumbnail_height = 250;
    if (style_name && strcmp(style_name, "prominimal") == 0)
        current.selected_radius = 5;
    load_file(style_name);
}

const RenderTheme *render_theme_current(void) { return &current; }

uint16_t render_theme_colour(RenderColour colour, uint16_t palette_default) {
    if (colour < 0 || colour >= RENDER_COLOUR_COUNT ||
        !(current.colour_mask & COLOUR_BIT(colour))) return palette_default;
    return current.colours[colour];
}
