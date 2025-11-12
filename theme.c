#include "theme.h"
#include "settings.h"
#include <string.h>

const Theme themes[] = {
    {
        .name = "MinUI Style",
        .bg = RGB565(0, 0, 0),
        .text = RGB565(255, 255, 255),
        .select_bg = RGB565(255, 255, 255),
        .select_text = RGB565(0, 0, 0),
        .header = RGB565(132, 132, 132),
        .folder = RGB565(255, 255, 255),
        .legend = RGB565(255, 255, 255),
        .legend_bg = RGB565(33, 33, 33),
        .disabled = RGB565(132, 132, 132)
    },
    {
        .name = "Emerald",
        .bg = RGB565(46, 125, 102),
        .text = RGB565(255, 255, 255),
        .select_bg = RGB565(255, 255, 255),
        .select_text = RGB565(46, 125, 102),
        .header = RGB565(76, 155, 132),
        .folder = RGB565(255, 255, 255),
        .legend = RGB565(255, 255, 255),
        .legend_bg = RGB565(36, 105, 82),
        .disabled = RGB565(156, 195, 182)
    },
    {
        .name = "Orange",
        .bg = RGB565(255, 102, 51),
        .text = RGB565(255, 255, 255),
        .select_bg = RGB565(255, 255, 255),
        .select_text = RGB565(255, 102, 51),
        .header = RGB565(255, 132, 81),
        .folder = RGB565(255, 255, 255),
        .legend = RGB565(255, 255, 255),
        .legend_bg = RGB565(225, 72, 21),
        .disabled = RGB565(255, 182, 151)
    },
    {
        .name = "Golden",
        .bg = RGB565(255, 193, 7),
        .text = RGB565(0, 0, 0),
        .select_bg = RGB565(0, 0, 0),
        .select_text = RGB565(255, 193, 7),
        .header = RGB565(255, 213, 47),
        .folder = RGB565(0, 0, 0),
        .legend = RGB565(0, 0, 0),
        .legend_bg = RGB565(225, 163, 0),
        .disabled = RGB565(128, 128, 128)
    },
    {
        .name = "Rose",
        .bg = RGB565(200, 200, 200),
        .text = RGB565(102, 51, 51),
        .select_bg = RGB565(153, 51, 51),
        .select_text = RGB565(255, 255, 255),
        .header = RGB565(153, 51, 51),
        .folder = RGB565(102, 51, 51),
        .legend = RGB565(102, 51, 51),
        .legend_bg = RGB565(170, 170, 170),
        .disabled = RGB565(150, 150, 150)
    },
    {
        .name = "Purple",
        .bg = RGB565(111, 66, 193),
        .text = RGB565(255, 255, 255),
        .select_bg = RGB565(255, 255, 255),
        .select_text = RGB565(111, 66, 193),
        .header = RGB565(141, 96, 223),
        .folder = RGB565(255, 255, 255),
        .legend = RGB565(255, 255, 255),
        .legend_bg = RGB565(81, 36, 163),
        .disabled = RGB565(191, 166, 233)
    },
    {
        .name = "Prosty's Pink",
        .bg = RGB565(255, 192, 203),
        .text = RGB565(255, 255, 255),
        .select_bg = RGB565(255, 255, 255),
        .select_text = RGB565(255, 105, 180),
        .header = RGB565(255, 105, 180),
        .folder = RGB565(255, 255, 255),
        .legend = RGB565(255, 255, 255),
        .legend_bg = RGB565(225, 162, 173),
        .disabled = RGB565(255, 222, 227)
    },
    {
        .name = "Green",
        .bg = RGB565(139, 195, 74),
        .text = RGB565(0, 0, 0),
        .select_bg = RGB565(0, 0, 0),
        .select_text = RGB565(139, 195, 74),
        .header = RGB565(169, 225, 104),
        .folder = RGB565(0, 0, 0),
        .legend = RGB565(0, 0, 0),
        .legend_bg = RGB565(109, 165, 44),
        .disabled = RGB565(100, 100, 100)
    },
    {
        .name = "Red",
        .bg = RGB565(244, 67, 54),
        .text = RGB565(255, 255, 255),
        .select_bg = RGB565(255, 255, 255),
        .select_text = RGB565(244, 67, 54),
        .header = RGB565(255, 97, 84),
        .folder = RGB565(255, 255, 255),
        .legend = RGB565(255, 255, 255),
        .legend_bg = RGB565(214, 37, 24),
        .disabled = RGB565(255, 167, 160)
    },
    {
        .name = "Commodore 64",
        .bg = RGB565(64, 50, 133),
        .text = RGB565(120, 105, 196),
        .select_bg = RGB565(120, 105, 196),
        .select_text = RGB565(64, 50, 133),
        .header = RGB565(120, 105, 196),
        .folder = RGB565(120, 105, 196),
        .legend = RGB565(120, 105, 196),
        .legend_bg = RGB565(40, 30, 90),
        .disabled = RGB565(80, 70, 120)
    },
    {
        .name = "Game Boy",
        .bg = RGB565(155, 188, 15),
        .text = RGB565(15, 56, 15),
        .select_bg = RGB565(15, 56, 15),
        .select_text = RGB565(155, 188, 15),
        .header = RGB565(48, 98, 48),
        .folder = RGB565(15, 56, 15),
        .legend = RGB565(15, 56, 15),
        .legend_bg = RGB565(48, 98, 48),
        .disabled = RGB565(99, 139, 25)
    },
    {
        .name = "NES",
        .bg = RGB565(251, 249, 248),
        .text = RGB565(84, 84, 84),
        .select_bg = RGB565(84, 84, 84),
        .select_text = RGB565(251, 249, 248),
        .header = RGB565(252, 89, 83),
        .folder = RGB565(84, 84, 84),
        .legend = RGB565(84, 84, 84),
        .legend_bg = RGB565(200, 200, 200),
        .disabled = RGB565(150, 150, 150)
    },
    {
        .name = "Amber CRT",
        .bg = RGB565(0, 0, 0),
        .text = RGB565(255, 176, 0),
        .select_bg = RGB565(255, 176, 0),
        .select_text = RGB565(0, 0, 0),
        .header = RGB565(255, 204, 68),
        .folder = RGB565(255, 176, 0),
        .legend = RGB565(255, 176, 0),
        .legend_bg = RGB565(51, 35, 0),
        .disabled = RGB565(128, 88, 0)
    },
    {
        .name = "Green CRT",
        .bg = RGB565(0, 0, 0),
        .text = RGB565(51, 255, 51),
        .select_bg = RGB565(51, 255, 51),
        .select_text = RGB565(0, 0, 0),
        .header = RGB565(102, 255, 102),
        .folder = RGB565(51, 255, 51),
        .legend = RGB565(51, 255, 51),
        .legend_bg = RGB565(0, 51, 0),
        .disabled = RGB565(25, 128, 25)
    },
    {
        .name = "DOS",
        .bg = RGB565(0, 0, 168),
        .text = RGB565(255, 255, 255),
        .select_bg = RGB565(0, 168, 168),
        .select_text = RGB565(0, 0, 0),
        .header = RGB565(255, 255, 85),
        .folder = RGB565(255, 255, 255),
        .legend = RGB565(255, 255, 255),
        .legend_bg = RGB565(0, 0, 85),
        .disabled = RGB565(168, 168, 168)
    },
    {
        .name = "Famicom",
        .bg = RGB565(142, 38, 20),
        .text = RGB565(255, 255, 255),
        .select_bg = RGB565(255, 255, 255),
        .select_text = RGB565(142, 38, 20),
        .header = RGB565(251, 242, 54),
        .folder = RGB565(255, 255, 255),
        .legend = RGB565(255, 255, 255),
        .legend_bg = RGB565(90, 25, 15),
        .disabled = RGB565(200, 150, 140)
    },
    {
        .name = "SNES",
        .bg = RGB565(225, 225, 225),
        .text = RGB565(100, 95, 155),
        .select_bg = RGB565(100, 95, 155),
        .select_text = RGB565(255, 255, 255),
        .header = RGB565(255, 204, 68),
        .folder = RGB565(100, 95, 155),
        .legend = RGB565(100, 95, 155),
        .legend_bg = RGB565(180, 180, 180),
        .disabled = RGB565(150, 150, 150)
    },
    {
        .name = "Matrix",
        .bg = RGB565(0, 0, 0),
        .text = RGB565(0, 255, 65),
        .select_bg = RGB565(0, 255, 65),
        .select_text = RGB565(0, 0, 0),
        .header = RGB565(0, 200, 50),
        .folder = RGB565(0, 255, 65),
        .legend = RGB565(0, 255, 65),
        .legend_bg = RGB565(0, 51, 12),
        .disabled = RGB565(0, 128, 32)
    },
    {
        .name = "Sajnaps Green",
        .bg = RGB565(0, 0, 0),
        .text = RGB565(0, 255, 0),
        .select_bg = RGB565(0, 255, 0),
        .select_text = RGB565(0, 0, 0),
        .header = RGB565(0, 216, 86),
        .folder = RGB565(0, 255, 0),
        .legend = RGB565(0, 255, 0),
        .legend_bg = RGB565(0, 40, 0),
        .disabled = RGB565(0, 120, 0)
    }
};

const int theme_count = sizeof(themes) / sizeof(themes[0]);

static int current_theme_index = 0;
static const Theme* current_theme = &themes[0];

void theme_init(void) {
    current_theme_index = 0;
    current_theme = &themes[0];
}

int theme_load_from_settings(const char* theme_name) {
    if (!theme_name) return 0;
    
    // Find theme by name
    for (int i = 0; i < theme_count; i++) {
        if (strcmp(themes[i].name, theme_name) == 0) {
            theme_apply(i);
            return 1;
        }
    }
    
    // Fallback to default theme if not found
    theme_apply(0);
    return 0;
}

void theme_apply(int theme_index) {
    if (theme_index >= 0 && theme_index < theme_count) {
        current_theme_index = theme_index;
        current_theme = &themes[theme_index];
    }
}

const Theme* theme_get_current(void) {
    return current_theme;
}

int theme_get_current_index(void) {
    return current_theme_index;
}

const char* theme_get_name(int index) {
    if (index >= 0 && index < theme_count) {
        return themes[index].name;
    }
    return "Unknown";
}

// Color accessors
uint16_t theme_bg(void) { return current_theme->bg; }
uint16_t theme_text(void) { return current_theme->text; }
uint16_t theme_select_bg(void) { return current_theme->select_bg; }
uint16_t theme_select_text(void) { return current_theme->select_text; }
uint16_t theme_header(void) { return current_theme->header; }
uint16_t theme_folder(void) { return current_theme->folder; }
uint16_t theme_legend(void) { return current_theme->legend; }
uint16_t theme_legend_bg(void) { return current_theme->legend_bg; }
uint16_t theme_disabled(void) { return current_theme->disabled; }