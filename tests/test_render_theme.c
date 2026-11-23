#include "../render_theme.h"

#include <assert.h>

int main(void) {
    const RenderTheme *theme;
    render_theme_load("vertical", "ArtBookNext");
    theme = render_theme_current();
    assert(theme->home_layout == RENDER_HOME_SWITCH);
    assert(theme->colours[RENDER_COLOUR_BACKGROUND] == 0x2104);
    assert(theme->colours[RENDER_COLOUR_TEXT] == 0xd69a);
    assert(theme->colours[RENDER_COLOUR_SELECTION] == 0xffff);
    assert(render_theme_colour_alpha(RENDER_COLOUR_SELECTION) == 25);
    assert(theme->colours[RENDER_COLOUR_SELECTION_TEXT] == 0xffff);
    assert(theme->selected_radius == 0);
    assert(theme->content_alignment == 1 && theme->list_padding_left == 35);
    assert(theme->list_glyph_padding_left == 5);

    render_theme_load("vertical", "CatOS");
    theme = render_theme_current();
    assert(theme->home_layout == RENDER_HOME_CATOS);
    assert(theme->grid_columns == 4 && theme->grid_rows == 2);
    assert(theme->colours[RENDER_COLOUR_BACKGROUND] != 0);
    assert(theme->grid_colour_mask != 0);
    {
        char path[256];
        assert(render_theme_asset_path(path, sizeof path,
            "640x480/image/wall/muxlaunch.png"));
        assert(render_theme_asset_path(path, sizeof path,
            "640x480/image/grid/muxlaunch/explore_focused.png"));
        assert(render_theme_asset_path(path, sizeof path,
            "640x480/image/grid/muxlaunch/shutdown_focused.png"));
    }
    render_theme_load("vertical", "Analogue");
    theme = render_theme_current();
    assert(theme->colour_mask & (1u << RENDER_COLOUR_SELECTION));
    assert(theme->colours[RENDER_COLOUR_SELECTION] == 0x0000);
    assert(render_theme_colour_alpha(RENDER_COLOUR_SELECTION) == 0);
    render_theme_load("vertical", "GbOS");
    theme = render_theme_current();
    assert(theme->colour_mask & (1u << RENDER_COLOUR_BACKGROUND));
    assert(theme->colours[RENDER_COLOUR_BACKGROUND] == 0x2104);
    assert(theme->colours[RENDER_COLOUR_SELECTION] == 0x8d61);
    assert(render_theme_colour_alpha(RENDER_COLOUR_SELECTION) == 25);
    {
        char path[256];
        assert(render_theme_asset_path(path, sizeof path,
            "640x480/image/wall/default.png"));
    }
    {
        static const char *themes[] = {
            "Analogue", "Def-not-SNES", "dsOS", "GbOS", "MinUIfied",
            "PipBoy", "PipBoyFrameless", "PipBoyTube"
        };
        char path[256];
        for (unsigned i = 0; i < sizeof themes / sizeof themes[0]; i++) {
            render_theme_load("vertical", themes[i]);
            assert(render_theme_current()->home_layout == RENDER_HOME_SWITCH);
            if (i == 4) {
                assert(render_theme_current()->list_padding_left == 35);
                assert(render_theme_current()->list_glyph_padding_left == 5);
            }
            render_theme_set_screen(RENDER_SCREEN_HOME);
            assert(render_theme_asset_path(path, sizeof path, i == 4
                ? "640x480/image/static/muxlaunch/explore.png"
                : "640x480/image/wall/muxlaunch/explore.png"));
            if (i == 4)
                assert(render_theme_asset_path(path, sizeof path,
                    "640x480/image/static/muxlaunch/collection.png"));
        }
    }
    render_theme_load("vertical", "MinUIfiedVertical");
    theme = render_theme_current();
    assert(theme->home_layout == RENDER_HOME_SWITCH);
    assert(theme->grid_columns == 0 && theme->grid_rows == 0);
    render_theme_load("vertical", "MinUIfied");
    theme = render_theme_current();
    assert(theme->home_layout == RENDER_HOME_SWITCH);
    assert(theme->grid_columns == 4 && theme->grid_rows == 2);
    {
        char path[256];
        assert(render_theme_asset_path(path, sizeof path,
            "640x480/image/static/muxlaunch/explore.png"));
    }
    return 0;
}
