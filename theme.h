#ifndef THEME_H
#define THEME_H

#include <stdint.h>

// Theme structure
typedef struct {
    const char* name;
    uint16_t bg;
    uint16_t text;
    uint16_t select_bg;
    uint16_t select_text;
    uint16_t header;
    uint16_t folder;
    uint16_t legend;
    uint16_t legend_bg;
    uint16_t disabled;
} Theme;

// RGB565 color format
#define RGB565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))

extern const Theme themes[];
extern const int theme_count;

void theme_init(void);
int theme_load_from_settings(const char* theme_name);
void theme_apply(int theme_index);
const Theme* theme_get_current(void);
int theme_get_current_index(void);
const char* theme_get_name(int index);

uint16_t theme_bg(void);
void theme_set_bg_override(int enabled, uint16_t color);
uint16_t theme_text(void);
uint16_t theme_select_bg(void);
uint16_t theme_select_text(void);
uint16_t theme_header(void);
uint16_t theme_folder(void);
uint16_t theme_legend(void);
uint16_t theme_legend_bg(void);
uint16_t theme_disabled(void);

#endif // THEME_H
