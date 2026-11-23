#ifndef RENDER_THEME_H
#define RENDER_THEME_H

#include <stdint.h>

typedef enum {
    RENDER_COLOUR_BACKGROUND,
    RENDER_COLOUR_HEADER_BACKGROUND,
    RENDER_COLOUR_LIST_BACKGROUND,
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

typedef enum {
    RENDER_GRID_BACKGROUND,
    RENDER_GRID_FOCUS_BACKGROUND,
    RENDER_GRID_TEXT,
    RENDER_GRID_FOCUS_TEXT,
    RENDER_GRID_COLOUR_COUNT
} RenderGridColour;

typedef enum {
    RENDER_HOME_DEFAULT,
    RENDER_HOME_SWITCH,
    RENDER_HOME_CATOS,
    RENDER_HOME_VERTICAL
} RenderHomeLayout;

typedef enum {
    RENDER_SCREEN_BROWSER,
    RENDER_SCREEN_HOME,
    RENDER_SCREEN_RECENTS,
    RENDER_SCREEN_APPS,
    RENDER_SCREEN_SETTINGS
} RenderThemeScreen;

/* Baseline-pixel geometry and optional palette overrides for one UI style. */
typedef struct {
    int header_height;
    int row_height;
    int horizontal_padding;
    int content_top;
    int content_width;
    int content_height;
    int content_alignment;
    int legend_height;
    int selected_radius;
    int thumbnail_width;
    int thumbnail_height;
    int font_size;
    int visible_rows;
    int grid_columns;
    int grid_rows;
    int grid_column_gap;
    int grid_row_gap;
    int grid_cell_width;
    int grid_cell_height;
    int grid_cell_radius;
    int grid_image_padding_top;
    int list_padding_left;
    int list_glyph_padding_left;
    int list_padding_top;
    int list_padding_bottom;
    RenderHomeLayout home_layout;
    char style_name[32];
    char scheme_name[32];
    char font_file[96];
    char background_image[96];
    uint16_t colours[RENDER_COLOUR_COUNT];
    uint8_t colour_alpha[RENDER_COLOUR_COUNT];
    unsigned colour_mask;
    uint16_t grid_colours[RENDER_GRID_COLOUR_COUNT];
    unsigned grid_colour_mask;
} RenderTheme;

void render_theme_load(const char *style_name, const char *scheme_name);
void render_theme_set_screen(RenderThemeScreen screen);
const RenderTheme *render_theme_current(void);
int render_theme_font_size(void);
const char *render_theme_font_file(void);
int render_theme_background(char *path, unsigned path_size);
uint16_t render_theme_colour(RenderColour colour, uint16_t palette_default);
uint8_t render_theme_colour_alpha(RenderColour colour);
uint16_t render_theme_grid_colour(RenderGridColour colour, uint16_t default_colour);
int render_theme_asset_path(char *path, unsigned path_size, const char *relative_path);

#endif
