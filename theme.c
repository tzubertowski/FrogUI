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
    },
    {
        .name = "Q_ta's Light Wii",
        .bg = RGB565(192, 192, 192),
        .text = RGB565(0, 176, 204),
        .select_bg = RGB565(0, 176, 204),
        .select_text = RGB565(255, 255, 255),
        .header = RGB565(0, 176, 204),
        .folder = RGB565(0, 176, 204),
        .legend = RGB565(0, 176, 204),
        .legend_bg = RGB565(160, 160, 160),
        .disabled = RGB565(128, 160, 168)
    },
    {
        .name = "Q_ta's Dark Wii",
        .bg = RGB565(16, 24, 24),
        .text = RGB565(0, 176, 204),
        .select_bg = RGB565(0, 96, 112),
        .select_text = RGB565(255, 255, 255),
        .header = RGB565(0, 176, 204),
        .folder = RGB565(0, 176, 204),
        .legend = RGB565(0, 176, 204),
        .legend_bg = RGB565(8, 12, 12),
        .disabled = RGB565(0, 88, 102)
    },
    {
        .name = "Desoxyn's Purple",
        .bg = RGB565(0, 0, 0),
        .text = RGB565(191, 64, 191),
        .select_bg = RGB565(191, 64, 191),
        .select_text = RGB565(0, 0, 0),
        .header = RGB565(244, 116, 242),
        .folder = RGB565(191, 64, 191),
        .legend = RGB565(191, 64, 191),
        .legend_bg = RGB565(55, 0, 59),
        .disabled = RGB565(244, 116, 242)
    },
    {
        .name = "Ocean",
        .bg = RGB565(10, 25, 47),
        .text = RGB565(100, 210, 255),
        .select_bg = RGB565(100, 210, 255),
        .select_text = RGB565(10, 25, 47),
        .header = RGB565(255, 255, 255),
        .folder = RGB565(100, 210, 255),
        .legend = RGB565(100, 210, 255),
        .legend_bg = RGB565(5, 15, 30),
        .disabled = RGB565(50, 100, 150)
    },
    {
        .name = "Sunset",
        .bg = RGB565(30, 15, 50),
        .text = RGB565(255, 140, 50),
        .select_bg = RGB565(255, 140, 50),
        .select_text = RGB565(30, 15, 50),
        .header = RGB565(255, 100, 150),
        .folder = RGB565(255, 140, 50),
        .legend = RGB565(255, 140, 50),
        .legend_bg = RGB565(20, 10, 35),
        .disabled = RGB565(150, 80, 100)
    },
    {
        .name = "Mono Dark",
        .bg = RGB565(0, 0, 0),
        .text = RGB565(255, 255, 255),
        .select_bg = RGB565(255, 255, 255),
        .select_text = RGB565(0, 0, 0),
        .header = RGB565(180, 180, 180),
        .folder = RGB565(255, 255, 255),
        .legend = RGB565(255, 255, 255),
        .legend_bg = RGB565(40, 40, 40),
        .disabled = RGB565(100, 100, 100)
    },
    {
        .name = "Nord",
        .bg = RGB565(46, 52, 64),
        .text = RGB565(136, 192, 208),
        .select_bg = RGB565(136, 192, 208),
        .select_text = RGB565(46, 52, 64),
        .header = RGB565(236, 239, 244),
        .folder = RGB565(136, 192, 208),
        .legend = RGB565(136, 192, 208),
        .legend_bg = RGB565(30, 35, 45),
        .disabled = RGB565(76, 86, 106)
    },
    {
        .name = "Dracula",
        .bg = RGB565(40, 42, 54),
        .text = RGB565(189, 147, 249),
        .select_bg = RGB565(189, 147, 249),
        .select_text = RGB565(40, 42, 54),
        .header = RGB565(255, 121, 198),
        .folder = RGB565(189, 147, 249),
        .legend = RGB565(189, 147, 249),
        .legend_bg = RGB565(25, 27, 38),
        .disabled = RGB565(98, 114, 164)
    },
    {
        .name = "Gruvbox",
        .bg = RGB565(40, 40, 40),
        .text = RGB565(250, 189, 47),
        .select_bg = RGB565(250, 189, 47),
        .select_text = RGB565(40, 40, 40),
        .header = RGB565(235, 219, 178),
        .folder = RGB565(250, 189, 47),
        .legend = RGB565(250, 189, 47),
        .legend_bg = RGB565(60, 56, 54),
        .disabled = RGB565(146, 131, 116)
    },
    {
        .name = "Tokyo Night",
        .bg = RGB565(26, 27, 38),
        .text = RGB565(115, 128, 243),
        .select_bg = RGB565(115, 128, 243),
        .select_text = RGB565(26, 27, 38),
        .header = RGB565(122, 226, 230),
        .folder = RGB565(115, 128, 243),
        .legend = RGB565(115, 128, 243),
        .legend_bg = RGB565(15, 15, 25),
        .disabled = RGB565(86, 95, 137)
    },
    {
        .name = "Solarized Dark",
        .bg = RGB565(0, 43, 54),
        .text = RGB565(131, 148, 150),
        .select_bg = RGB565(131, 148, 150),
        .select_text = RGB565(0, 43, 54),
        .header = RGB565(181, 137, 0),
        .folder = RGB565(131, 148, 150),
        .legend = RGB565(131, 148, 150),
        .legend_bg = RGB565(7, 54, 66),
        .disabled = RGB565(88, 110, 117)
    },
    {
        .name = "Catppuccin Mocha",
        .bg = RGB565(30, 30, 46),
        .text = RGB565(205, 214, 244),
        .select_bg = RGB565(203, 166, 247),
        .select_text = RGB565(30, 30, 46),
        .header = RGB565(137, 180, 250),
        .folder = RGB565(166, 227, 161),
        .legend = RGB565(186, 194, 222),
        .legend_bg = RGB565(49, 50, 68),
        .disabled = RGB565(127, 132, 156)
    },
    {
        .name = "Aura",
        .bg = RGB565(24, 23, 38),
        .text = RGB565(242, 233, 225),
        .select_bg = RGB565(235, 188, 186),
        .select_text = RGB565(45, 35, 52),
        .header = RGB565(196, 167, 231),
        .folder = RGB565(245, 194, 231),
        .legend = RGB565(221, 193, 255),
        .legend_bg = RGB565(53, 43, 70),
        .disabled = RGB565(150, 137, 171)
    },
    /* Allium-Themes palettes; provenance and mapping: fonts/alium_README.md. */
    {
        .name = "alium_default",
        .bg = RGB565(0, 0, 0),
        .text = RGB565(255, 255, 255),
        .select_bg = RGB565(114, 135, 253),
        .select_text = RGB565(255, 255, 255),
        .header = RGB565(255, 255, 255),
        .folder = RGB565(255, 255, 255),
        .legend = RGB565(255, 255, 255),
        .legend_bg = RGB565(0, 0, 0),
        .disabled = RGB565(88, 91, 112)
    },
    {
        .name = "alium_alliumboy",
        .bg = RGB565(8, 24, 32),
        .text = RGB565(224, 248, 208),
        .select_bg = RGB565(52, 104, 86),
        .select_text = RGB565(224, 248, 208),
        .header = RGB565(224, 248, 208),
        .folder = RGB565(224, 248, 208),
        .legend = RGB565(224, 248, 208),
        .legend_bg = RGB565(8, 24, 32),
        .disabled = RGB565(52, 104, 86)
    },
    {
        .name = "alium_blue",
        .bg = RGB565(27, 38, 59),
        .text = RGB565(233, 236, 239),
        .select_bg = RGB565(58, 134, 255),
        .select_text = RGB565(255, 255, 255),
        .header = RGB565(58, 134, 255),
        .folder = RGB565(233, 236, 239),
        .legend = RGB565(255, 255, 255),
        .legend_bg = RGB565(27, 38, 59),
        .disabled = RGB565(108, 117, 125)
    },
    {
        .name = "alium_boxart",
        .bg = RGB565(0, 0, 0),
        .text = RGB565(255, 255, 255),
        .select_bg = RGB565(255, 255, 255),
        .select_text = RGB565(0, 0, 0),
        .header = RGB565(255, 255, 255),
        .folder = RGB565(255, 255, 255),
        .legend = RGB565(255, 255, 255),
        .legend_bg = RGB565(0, 0, 0),
        .disabled = RGB565(151, 151, 151)
    },
    {
        .name = "alium_brown",
        .bg = RGB565(58, 24, 0),
        .text = RGB565(204, 124, 75),
        .select_bg = RGB565(221, 221, 221),
        .select_text = RGB565(204, 124, 75),
        .header = RGB565(238, 159, 2),
        .folder = RGB565(204, 124, 75),
        .legend = RGB565(204, 124, 75),
        .legend_bg = RGB565(58, 24, 0),
        .disabled = RGB565(71, 74, 109)
    },
    {
        .name = "alium_bubblegum",
        .bg = RGB565(191, 211, 186),
        .text = RGB565(131, 147, 144),
        .select_bg = RGB565(252, 215, 221),
        .select_text = RGB565(254, 254, 254),
        .header = RGB565(254, 254, 254),
        .folder = RGB565(131, 147, 144),
        .legend = RGB565(254, 254, 254),
        .legend_bg = RGB565(191, 211, 186),
        .disabled = RGB565(194, 163, 165)
    },
    {
        .name = "alium_eva_01",
        .bg = RGB565(95, 42, 98),
        .text = RGB565(255, 255, 255),
        .select_bg = RGB565(160, 222, 89),
        .select_text = RGB565(255, 255, 255),
        .header = RGB565(160, 222, 89),
        .folder = RGB565(255, 255, 255),
        .legend = RGB565(255, 255, 255),
        .legend_bg = RGB565(95, 42, 98),
        .disabled = RGB565(88, 91, 112)
    },
    {
        .name = "alium_min",
        .bg = RGB565(0, 0, 0),
        .text = RGB565(255, 255, 255),
        .select_bg = RGB565(255, 255, 255),
        .select_text = RGB565(0, 0, 0),
        .header = RGB565(255, 255, 255),
        .folder = RGB565(255, 255, 255),
        .legend = RGB565(255, 255, 255),
        .legend_bg = RGB565(0, 0, 0),
        .disabled = RGB565(151, 151, 151)
    },
    {
        .name = "alium_snes",
        .bg = RGB565(84, 88, 90),
        .text = RGB565(255, 255, 255),
        .select_bg = RGB565(81, 70, 137),
        .select_text = RGB565(255, 255, 255),
        .header = RGB565(167, 164, 224),
        .folder = RGB565(255, 255, 255),
        .legend = RGB565(255, 255, 255),
        .legend_bg = RGB565(84, 88, 90),
        .disabled = RGB565(84, 88, 90)
    },
    {
        .name = "alium_spruceos",
        .bg = RGB565(0, 0, 0),
        .text = RGB565(235, 219, 178),
        .select_bg = RGB565(251, 241, 199),
        .select_text = RGB565(40, 40, 40),
        .header = RGB565(104, 157, 106),
        .folder = RGB565(235, 219, 178),
        .legend = RGB565(235, 219, 178),
        .legend_bg = RGB565(0, 0, 0),
        .disabled = RGB565(124, 111, 100)
    },
    {
        .name = "alium_synthwave",
        .bg = RGB565(0, 0, 0),
        .text = RGB565(255, 255, 255),
        .select_bg = RGB565(118, 23, 153),
        .select_text = RGB565(255, 255, 255),
        .header = RGB565(81, 97, 157),
        .folder = RGB565(255, 255, 255),
        .legend = RGB565(255, 255, 255),
        .legend_bg = RGB565(0, 0, 0),
        .disabled = RGB565(88, 91, 112)
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
