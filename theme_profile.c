#include "theme_profile.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PROFILE_PATH_MAX 256
#define PROFILE_FILE_MAX 2048

typedef struct { const char *p; } Json;
typedef enum { J_STRING, J_BOOL, J_NUMBER, J_NULL } JsonType;
typedef struct {
    JsonType type;
    char string[128];
    long number;
    int boolean;
} JsonValue;

static void ws(Json *j) {
    while (isspace((unsigned char)*j->p)) j->p++;
}

static int string(Json *j, char *out, size_t cap) {
    size_t n = 0;
    if (*j->p++ != '"') return 0;
    while (*j->p && *j->p != '"') {
        unsigned char c = (unsigned char)*j->p++;
        if (c == '\\') {
            c = (unsigned char)*j->p++;
            if (!c) return 0;
            if (c == '"' || c == '\\' || c == '/') {
                /* JSON quoting escapes. */
            } else if (c == 'n') c = '\n';
            else if (c == 'r') c = '\r';
            else if (c == 't') c = '\t';
            else if (c == 'b') c = '\b';
            else if (c == 'f') c = '\f';
            else if (c == 'u') {
                int value = 0;
                for (int i = 0; i < 4; i++) {
                    unsigned char h = (unsigned char)*j->p++;
                    if (!isxdigit(h)) return 0;
                    value = value * 16 + (isdigit(h) ? h - '0' :
                            (tolower(h) - 'a' + 10));
                }
                c = value < 0x80 ? (unsigned char)value : '?';
            } else return 0;
        }
        if (c < 0x20) return 0;
        if (out) {
            if (n + 1 >= cap) return 0;
            out[n] = (char)c;
        }
        n++;
    }
    if (*j->p++ != '"') return 0;
    if (out) out[n] = '\0';
    return 1;
}

static int value(Json *j, JsonValue *v) {
    ws(j);
    memset(v, 0, sizeof(*v));
    if (*j->p == '"') {
        v->type = J_STRING;
        return string(j, v->string, sizeof(v->string));
    }
    if (strncmp(j->p, "true", 4) == 0) {
        v->type = J_BOOL; v->boolean = 1; j->p += 4; return 1;
    }
    if (strncmp(j->p, "false", 5) == 0) {
        v->type = J_BOOL; j->p += 5; return 1;
    }
    if (strncmp(j->p, "null", 4) == 0) {
        v->type = J_NULL; j->p += 4; return 1;
    }
    if (*j->p != '-' && !isdigit((unsigned char)*j->p)) return 0;
    char *end;
    v->number = strtol(j->p, &end, 10);
    if (end == j->p) return 0;
    v->type = J_NUMBER; j->p = end;
    return 1;
}

static int bool_field(JsonValue *v, int *has, int *dst) {
    if (v->type != J_BOOL) return 0;
    *has = 1; *dst = v->boolean; return 1;
}

static int apply_field(ThemeProfile *p, const char *key, JsonValue *v) {
    if (strcmp(key, "ignoreOverrides") == 0)
        return bool_field(v, &p->has_ignore_overrides, &p->ignore_overrides);
    if (strcmp(key, "quick_resume") == 0)
        return bool_field(v, &p->has_quick_resume, &p->quick_resume);
    if (strcmp(key, "autosave_autoload") == 0)
        return bool_field(v, &p->has_autosave_autoload, &p->autosave_autoload);
    if (strcmp(key, "pillboxes") == 0)
        return bool_field(v, &p->has_pillboxes, &p->pillboxes);
    if (strcmp(key, "background_images") == 0)
        return bool_field(v, &p->has_background_images, &p->background_images);
    if (strcmp(key, "game_switcher") == 0)
        return bool_field(v, &p->has_game_switcher, &p->game_switcher);
    if (strcmp(key, "apps_in_settings") == 0)
        return bool_field(v, &p->has_apps_in_settings, &p->apps_in_settings);
    if (strcmp(key, "rounded_screenshots") == 0)
        return bool_field(v, &p->has_rounded_screenshots, &p->rounded_screenshots);
    if (strcmp(key, "stock_battery") == 0)
        return bool_field(v, &p->has_stock_battery, &p->stock_battery);
    if (strcmp(key, "battery_color") == 0)
        return bool_field(v, &p->has_battery_color, &p->battery_color);
    if (strcmp(key, "font_size") == 0) {
        if (v->type != J_NUMBER || v->number < 18 || v->number > 26) return 0;
        p->has_font_size = 1; p->font_size = (int)v->number; return 1;
    }
    if (strcmp(key, "font") == 0) {
        if (v->type != J_STRING || strlen(v->string) >= sizeof(p->font)) return 0;
        p->has_font = 1; strcpy(p->font, v->string); return 1;
    }
    if (strcmp(key, "settings_layout") == 0) {
        if (v->type != J_STRING ||
            (strcmp(v->string, "folder") != 0 && strcmp(v->string, "sections") != 0)) return 0;
        p->has_settings_layout = 1;
        strcpy(p->settings_layout, v->string);
    }
    return 1; /* Unknown scalar keys are forward-compatible. */
}

static int parse(const char *src, ThemeProfile *p) {
    Json j = { src };
    ws(&j);
    if (*j.p++ != '{') return 0;
    ws(&j);
    if (*j.p == '}') j.p++;
    else for (;;) {
        char key[128]; JsonValue v;
        if (!string(&j, key, sizeof(key))) return 0;
        ws(&j);
        if (*j.p++ != ':') return 0;
        if (!value(&j, &v) || !apply_field(p, key, &v)) return 0;
        ws(&j);
        if (*j.p == '}') { j.p++; break; }
        if (*j.p++ != ',') return 0;
        ws(&j);
    }
    ws(&j);
    return *j.p == '\0';
}

int theme_profile_load(const char *theme_name, ThemeProfile *profile) {
    char path[PROFILE_PATH_MAX], data[PROFILE_FILE_MAX + 1];
    FILE *f;
    size_t n;
    if (!theme_name || !*theme_name || !profile) return 0;
    for (const unsigned char *c = (const unsigned char *)theme_name; *c; c++)
        if (!(isalnum(*c) || *c == '_' || *c == '-' || *c == ' ' || *c == '\'')) return 0;
    if (snprintf(path, sizeof(path), "/mnt/sdcard/frogui/style-profiles/%s.json", theme_name)
        >= (int)sizeof(path)) return 0;
    f = fopen(path, "r");
    if (!f) return 0;
    n = fread(data, 1, sizeof(data), f);
    if (ferror(f) || n > PROFILE_FILE_MAX || (n == PROFILE_FILE_MAX && !feof(f))) {
        fclose(f); return 0;
    }
    fclose(f);
    data[n] = '\0';
    memset(profile, 0, sizeof(*profile));
    if (!parse(data, profile)) {
        memset(profile, 0, sizeof(*profile));
        return 0;
    }
    return 1;
}
