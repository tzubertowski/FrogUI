#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "font.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static stbtt_fontinfo font_info;
static unsigned char *font_buffer = NULL;
static float font_scale;
static int font_loaded = 0;

#define FONT_SIZE 16.0f

void font_init(void) {
    // Try to load GamePocket font (smallest and cleanest)
    const char *font_paths[] = {
        "/mnt/sda1/frogui/fonts/GamePocket-Regular-ZeroKern.ttf",
        "/app/sdcard/frogui/fonts/GamePocket-Regular-ZeroKern.ttf",
        "fonts/GamePocket-Regular-ZeroKern.ttf",
        NULL
    };

    FILE *fp = NULL;
    for (int i = 0; font_paths[i] != NULL; i++) {
        fp = fopen(font_paths[i], "rb");
        if (fp) break;
    }

    if (!fp) {
        font_loaded = 0;
        return;
    }

    // Get file size
    fseek(fp, 0, SEEK_END);
    long font_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Allocate buffer and read font
    font_buffer = (unsigned char*)malloc(font_size);
    if (!font_buffer) {
        fclose(fp);
        font_loaded = 0;
        return;
    }

    fread(font_buffer, 1, font_size, fp);
    fclose(fp);

    // Initialize font
    if (!stbtt_InitFont(&font_info, font_buffer, stbtt_GetFontOffsetForIndex(font_buffer, 0))) {
        free(font_buffer);
        font_buffer = NULL;
        font_loaded = 0;
        return;
    }

    // Calculate scale for desired pixel height
    font_scale = stbtt_ScaleForPixelHeight(&font_info, FONT_SIZE);
    font_loaded = 1;
}

void font_draw_char(uint16_t *framebuffer, int screen_width, int screen_height,
                   int x, int y, char c, uint16_t color) {
    if (!font_loaded || !framebuffer) return;

    // Convert to uppercase
    if (c >= 'a' && c <= 'z') {
        c = c - 'a' + 'A';
    }

    // Get glyph index
    int glyph_index = stbtt_FindGlyphIndex(&font_info, c);
    if (glyph_index == 0) return; // Glyph not found

    // Get glyph bitmap
    int width, height, xoff, yoff;
    unsigned char *bitmap = stbtt_GetGlyphBitmap(&font_info, 0, font_scale,
                                                  glyph_index, &width, &height, &xoff, &yoff);

    if (!bitmap) return;

    // Get vertical metrics for proper baseline alignment
    int ascent, descent, line_gap;
    stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap);
    int baseline = (int)(ascent * font_scale);

    // Draw the glyph
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            unsigned char alpha = bitmap[row * width + col];
            if (alpha > 0) {
                int px = x + xoff + col;
                int py = y + baseline + yoff + row;

                if (px >= 0 && px < screen_width && py >= 0 && py < screen_height) {
                    // Simple alpha blending
                    if (alpha > 127) {
                        framebuffer[py * screen_width + px] = color;
                    }
                }
            }
        }
    }

    stbtt_FreeBitmap(bitmap, NULL);
}

void font_draw_text(uint16_t *framebuffer, int screen_width, int screen_height,
                   int x, int y, const char *text, uint16_t color) {
    if (!font_loaded || !framebuffer || !text) return;

    int start_x = x;
    int prev_codepoint = 0;

    while (*text) {
        if (*text == '\n') {
            y += FONT_SIZE + 4;  // Line spacing
            x = start_x;
            text++;
            prev_codepoint = 0;
            continue;
        }

        char c = *text;

        // Convert to uppercase
        if (c >= 'a' && c <= 'z') {
            c = c - 'a' + 'A';
        }

        // Get glyph index
        int glyph_index = stbtt_FindGlyphIndex(&font_info, c);

        if (glyph_index != 0) {
            // Get advance width and left side bearing
            int advance_width, left_side_bearing;
            stbtt_GetGlyphHMetrics(&font_info, glyph_index, &advance_width, &left_side_bearing);

            // Apply kerning if we have a previous character
            if (prev_codepoint != 0) {
                int kern = stbtt_GetGlyphKernAdvance(&font_info, prev_codepoint, glyph_index);
                x += (int)(kern * font_scale);
            }

            // Draw the character
            font_draw_char(framebuffer, screen_width, screen_height, x, y, c, color);

            // Advance cursor
            x += (int)(advance_width * font_scale);
            prev_codepoint = glyph_index;
        } else {
            // Space or unknown character
            x += FONT_CHAR_SPACING;
            prev_codepoint = 0;
        }

        text++;
    }
}
