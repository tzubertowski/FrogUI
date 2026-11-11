#ifndef RENDER_THEME_H
#define RENDER_THEME_H

#include <stdint.h>

typedef enum {
    RENDER_COLOUR_BACKGROUND,
    RENDER_COLOUR_TEXT,
    RENDER_COLOUR_SELECTION,
    RENDER_COLOUR_SELECTION_TEXT,
    RENDER_COLOUR_HEADER,
    RENDER_COLOUR_FOLDER,
    RENDER_COLOUR_LEGEND,
    RENDER_COLOUR_LEGEND_BACKGROUND,
    RENDER_COLOUR_DISABLED,
    RENDER_COLOUR_COUNT
} RenderColour;

/* Baseline-pixel geometry and optional palette overrides for one UI style. */
typedef struct {
    int header_height;
    int row_height;
    int horizontal_padding;
    int content_top;
    int legend_height;
    int selected_radius;
    int thumbnail_width;
    int thumbnail_height;
    uint16_t colours[RENDER_COLOUR_COUNT];
    unsigned colour_mask;
} RenderTheme;

void render_theme_load(const char *style_name);
const RenderTheme *render_theme_current(void);
uint16_t render_theme_colour(RenderColour colour, uint16_t palette_default);

#endif
