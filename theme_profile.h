#ifndef THEME_PROFILE_H
#define THEME_PROFILE_H

typedef struct {
    int has_ignore_overrides, ignore_overrides;
    int has_quick_resume, quick_resume;
    int has_autosave_autoload, autosave_autoload;
    int has_pillboxes, pillboxes;
    int has_font_size, font_size;
    int has_background_images, background_images;
    int has_game_switcher, game_switcher;
    int has_apps_in_settings, apps_in_settings;
    int has_load_recents, load_recents;
    int has_rounded_screenshots, rounded_screenshots;
    int has_stock_battery, stock_battery;
    int has_battery_color, battery_color;
    int has_settings_layout;
    char settings_layout[16];
    int has_font;
    char font[96];
} ThemeProfile;

/* Load a flat JSON profile from /mnt/sdcard/frogui/style-profiles/<style>.json.
 * Returns 1 for a valid profile, 0 when missing or invalid. */
int theme_profile_load(const char *theme_name, ThemeProfile *profile);

#endif
