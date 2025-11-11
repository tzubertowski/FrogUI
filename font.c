#include "font.h"
#include "font_gamepocket.h"
#include <string.h>

void font_init(void) {
    // Font initialization if needed
}

// Draw a character using ChillRound 16x16 bitmap font
void font_draw_char(uint16_t *framebuffer, int screen_width, int screen_height, 
                   int x, int y, char c, uint16_t color) {
    if (!framebuffer) return;
    if (c < 32 || c > 127) c = ' ';  // Replace non-printable with space

    const uint8_t *glyph = font_gamepocket_16x16[c - 32];

    // 16x16 font, 2 bytes per row (16 bits)
    for (int row = 0; row < 16; row++) {
        // Each row is 2 bytes (16 bits) - read as big endian
        uint16_t row_bits = (glyph[row * 2] << 8) | glyph[row * 2 + 1];

        for (int col = 0; col < 16; col++) {
            if (row_bits & (1 << (15 - col))) {  // Check bit from left to right
                int px = x + col;
                int py = y + row;
                if (px >= 0 && px < screen_width && py >= 0 && py < screen_height) {
                    framebuffer[py * screen_width + px] = color;
                }
            }
        }
    }
}

// Draw text string with automatic character spacing
void font_draw_text(uint16_t *framebuffer, int screen_width, int screen_height,
                   int x, int y, const char *text, uint16_t color) {
    if (!framebuffer || !text) return;
    
    int start_x = x;
    while (*text) {
        if (*text == '\n') {
            y += 20;  // Line spacing
            x = start_x;
        } else {
            // Convert to uppercase
            char c = *text;
            if (c >= 'a' && c <= 'z') {
                c = c - 'a' + 'A';
            }
            font_draw_char(framebuffer, screen_width, screen_height, x, y, c, color);
            x += FONT_CHAR_SPACING;  // GamePocket character spacing
        }
        text++;
    }
}