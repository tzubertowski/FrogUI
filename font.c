#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
#include "font.h"
#include "settings.h"
#include "common/i18n.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static stbtt_fontinfo font_info;
static unsigned char *font_buffer = NULL;
static float font_scale;
static int font_loaded = 0;
/* Optional broad-Unicode fallback.  The normal UI font stays small; this
 * face is loaded only when a string actually contains a glyph the UI font
 * does not provide. */
static stbtt_fontinfo fallback_info;
static unsigned char *fallback_buffer = NULL;
static float fallback_scale;
static int fallback_loaded = 0;
static stbtt_fontinfo latin_info;
static unsigned char *latin_buffer = NULL;
static float latin_scale;
static int latin_loaded = 0;
static int active_font_id = 0;
/* 0 = selected UI font, 1 = broad CJK/Cyrillic fallback, 2 = Latin Extended
 * fallback. A locale always uses one face throughout the UI. */
static int language_force_font_id = 0;
static uint32_t utf8_next(const char **p);
static uint32_t unicode_upper(uint32_t cp) {
    if (cp >= 'a' && cp <= 'z') return cp - 32;
    if (cp >= 0xE0 && cp <= 0xF6) return cp - 0x20;
    if (cp >= 0xF8 && cp <= 0xFE) return cp - 0x20;
    switch (cp) { case 0x0105:return 0x0104; case 0x0107:return 0x0106; case 0x0119:return 0x0118; case 0x0142:return 0x0141; case 0x0144:return 0x0143; case 0x015B:return 0x015A; case 0x017A:return 0x0179; case 0x017C:return 0x017B; default:return cp; }
}

#ifndef UI_SCALE
#define UI_SCALE 100
#endif
#define FONT_SIZE (26.0f * UI_SCALE / 100.0f)   /* NextUI-style larger text */

// Internal function to load a font file
static int load_font_file(const char *font_filename) {
    // Free previous font if loaded
    if (font_buffer) {
        free(font_buffer);
        font_buffer = NULL;
        font_loaded = 0;
    }

    // Build search paths for the font (SF3000 paths first)
    char font_paths[4][256];
    snprintf(font_paths[0], sizeof(font_paths[0]), "/mnt/sdcard/cubegm/fonts/%s", font_filename);
    snprintf(font_paths[1], sizeof(font_paths[1]), "/mnt/sdcard/frogui/fonts/%s", font_filename);
    snprintf(font_paths[2], sizeof(font_paths[2]), "/mnt/sdcard/frogui/fonts/%s", font_filename);
    snprintf(font_paths[3], sizeof(font_paths[3]), "fonts/%s", font_filename);

    FILE *fp = NULL;
    for (int i = 0; i < 4; i++) {
        fp = fopen(font_paths[i], "rb");
        if (fp) break;
    }

    if (!fp) {
        return 0;
    }

    // Get file size
    fseek(fp, 0, SEEK_END);
    long font_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    // Allocate buffer and read font
    font_buffer = (unsigned char*)malloc(font_size);
    if (!font_buffer) {
        fclose(fp);
        return 0;
    }

    fread(font_buffer, 1, font_size, fp);
    fclose(fp);

    // Initialize font
    if (!stbtt_InitFont(&font_info, font_buffer, stbtt_GetFontOffsetForIndex(font_buffer, 0))) {
        free(font_buffer);
        font_buffer = NULL;
        return 0;
    }

    // Calculate scale for desired pixel height
    font_scale = stbtt_ScaleForPixelHeight(&font_info, FONT_SIZE);
    font_loaded = 1;
    return 1;
}

void font_load_file(const char *font_filename) {
    if (!font_filename || !font_filename[0]) return;
    load_font_file(font_filename);
    if (font_loaded)
        font_scale = stbtt_ScaleForPixelHeight(&font_info, FONT_SIZE);
}

void font_load_from_settings(const char *font_name) {
    const char *font_filename = NULL;
    float custom_size = FONT_SIZE;

    // Map font names to font files — always render at FONT_SIZE (scaled by UI_SCALE)
    if (strcmp(font_name, "Monogram") == 0) {
        font_filename = "monogram.ttf";
    } else if (strcmp(font_name, "GamePocket") == 0) {
        font_filename = "GamePocket-Regular-ZeroKern.ttf";
    } else {
        font_filename = "BPreplayBold.otf";   /* NextUI-style default */
    }
    custom_size = FONT_SIZE;  // use compile-time size, not hardcoded per-font px

    load_font_file(font_filename);

    if (font_loaded) {
        font_scale = stbtt_ScaleForPixelHeight(&font_info, custom_size);
    }
}

void font_init(void) {
    // Default to the NextUI-style bold font; fall back if the file is missing.
    if (load_font_file("BPreplayBold.otf"))
        font_scale = stbtt_ScaleForPixelHeight(&font_info, FONT_SIZE);
    else
        font_load_from_settings("GamePocket");
}

static int load_fallback_font(void) {
    const char *paths[] = {
        "/mnt/sdcard/frogui/fonts/TreeFrogUnicode.ttf",
        "/mnt/sdcard/cubegm/fonts/TreeFrogUnicode.ttf",
        "fonts/TreeFrogUnicode.ttf"
    };
    FILE *fp = NULL;
    long size;
    int i;
    if (fallback_loaded) return 1;
    for (i = 0; i < (int)(sizeof(paths) / sizeof(paths[0])); i++) {
        fp = fopen(paths[i], "rb");
        if (fp) break;
    }
    if (!fp) return 0;
    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (size <= 0 || size > 32 * 1024 * 1024) {
        fclose(fp);
        return 0;
    }
    fallback_buffer = (unsigned char *)malloc((size_t)size);
    if (!fallback_buffer || fread(fallback_buffer, 1, (size_t)size, fp) != (size_t)size) {
        free(fallback_buffer);
        fallback_buffer = NULL;
        fclose(fp);
        return 0;
    }
    fclose(fp);
    if (!stbtt_InitFont(&fallback_info, fallback_buffer,
                        stbtt_GetFontOffsetForIndex(fallback_buffer, 0))) {
        free(fallback_buffer);
        fallback_buffer = NULL;
        return 0;
    }
    fallback_scale = stbtt_ScaleForPixelHeight(&fallback_info, FONT_SIZE);
    fallback_loaded = 1;
    return 1;
}

static int load_latin_fallback(void) {
    const char *paths[] = { "/mnt/sdcard/frogui/fonts/TreeFrogLatin.ttf", "/mnt/sdcard/cubegm/fonts/TreeFrogLatin.ttf", "fonts/TreeFrogLatin.ttf" };
    FILE *fp = NULL; long size; int i;
    if (latin_loaded) return 1;
    for (i = 0; i < 3; i++) { fp = fopen(paths[i], "rb"); if (fp) break; }
    if (!fp) return 0;
    fseek(fp, 0, SEEK_END); size = ftell(fp); fseek(fp, 0, SEEK_SET);
    if (size <= 0 || size > 4 * 1024 * 1024) { fclose(fp); return 0; }
    latin_buffer = (unsigned char *)malloc((size_t)size);
    if (!latin_buffer || fread(latin_buffer, 1, (size_t)size, fp) != (size_t)size) { free(latin_buffer); latin_buffer = NULL; fclose(fp); return 0; }
    fclose(fp);
    if (!stbtt_InitFont(&latin_info, latin_buffer, stbtt_GetFontOffsetForIndex(latin_buffer, 0))) { free(latin_buffer); latin_buffer = NULL; return 0; }
    latin_scale = stbtt_ScaleForPixelHeight(&latin_info, FONT_SIZE); latin_loaded = 1; return 1;
}

/* Rasterize glyphs into a static buffer instead of stbtt_GetGlyphBitmap (which
 * mallocs per glyph per frame). On memory-pressured devices those allocs can
 * transiently fail -> draw bails -> glyphs vanish for a frame. Ported from the
 * same fix in picoarch/menu_font.c. No per-frame allocation now. */
#define GLYPH_MAX 128
/* Per-glyph cache: rasterize each character ONCE at the current font scale and
 * reuse it. The old code ran stbtt_MakeGlyphBitmap + GetFontVMetrics for every
 * character every frame, which made scrolling crawl with the larger font. */
struct gcache_ent { uint32_t codepoint; int font_id, valid, w, h, xoff, yoff; unsigned char *bmp; };
static struct gcache_ent gcache[512];
static float gcache_scale = -1.0f;
static int   gcache_baseline = 0;
static void gcache_reset(void) {
    for (int i = 0; i < 512; i++) { free(gcache[i].bmp); gcache[i].bmp = NULL; gcache[i].valid = 0; }
}

void font_draw_char(uint16_t *framebuffer, int screen_width, int screen_height,
                   int x, int y, char c, uint16_t color) {
    if (!font_loaded || !framebuffer) return;

    // Convert to uppercase
    if (c >= 'a' && c <= 'z') {
        c = c - 'a' + 'A';
    }
    unsigned char idx = (unsigned char)c;
    if (idx >= 128) return;

    // Rebuild cache if the font/scale changed
    if (gcache_scale != font_scale) {
        gcache_reset();
        gcache_scale = font_scale;
        int ascent, descent, line_gap;
        stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap);
        gcache_baseline = (int)(ascent * font_scale);
    }

    struct gcache_ent *g = &gcache[idx];
    if (!g->valid) {
        int glyph_index = stbtt_FindGlyphIndex(&font_info, c);
        if (glyph_index == 0) { g->valid = 1; g->w = g->h = 0; }   // no glyph: cache empty
        else {
            int xoff, yoff, x1, y1;
            stbtt_GetGlyphBitmapBox(&font_info, glyph_index, font_scale, font_scale, &xoff, &yoff, &x1, &y1);
            int width = x1 - xoff, height = y1 - yoff;
            if (width <= 0 || height <= 0 || width > GLYPH_MAX || height > GLYPH_MAX) {
                g->valid = 1; g->w = g->h = 0;
            } else {
                g->bmp = (unsigned char*)malloc((size_t)width * height);
                if (!g->bmp) return;   // alloc fail: try again next frame
                stbtt_MakeGlyphBitmap(&font_info, g->bmp, width, height, width, font_scale, font_scale, glyph_index);
                g->w = width; g->h = height; g->xoff = xoff; g->yoff = yoff; g->valid = 1;
            }
        }
    }
    if (g->w <= 0 || g->h <= 0) return;   // space / empty

    int width = g->w, height = g->h, xoff = g->xoff, yoff = g->yoff, baseline = gcache_baseline;
    const unsigned char *bitmap = g->bmp;
    // Draw the glyph
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            unsigned char alpha = bitmap[row * width + col];
            if (alpha > 0) {
                int px = x + xoff + col;
                int py = y + baseline + yoff + row;

                if (px >= 0 && px < screen_width && py >= 0 && py < screen_height) {
                    uint16_t *dst = &framebuffer[py * screen_width + px];
                    if (alpha >= 255) {
                        *dst = color;
                    } else {
                        uint16_t bg = *dst;
                        int fr = (color >> 11) & 0x1F, fg = (color >> 5) & 0x3F, fb = color & 0x1F;
                        int br = (bg >> 11) & 0x1F, bgc = (bg >> 5) & 0x3F, bb = bg & 0x1F;
                        int ia = 255 - alpha;
                        int rr = (fr * alpha + br * ia) / 255;
                        int rg = (fg * alpha + bgc * ia) / 255;
                        int rb = (fb * alpha + bb * ia) / 255;
                        *dst = (uint16_t)((rr << 11) | (rg << 5) | rb);
                    }
                }
            }
        }
    }
}

/* Draw a Unicode codepoint using the primary face when possible and the
 * bundled broad-Unicode fallback otherwise.  ROM names are UTF-8, not a byte
 * stream: keeping decoding here also makes measuring and drawing agree. */
static void font_blend_pixel(uint16_t *dst, uint16_t color, unsigned char alpha) {
    if (alpha == 255) { *dst = color; return; }
    uint16_t bg = *dst;
    int fr = (color >> 11) & 31, fg = (color >> 5) & 63, fb = color & 31;
    int br = (bg >> 11) & 31, bgc = (bg >> 5) & 63, bb = bg & 31, ia = 255 - alpha;
    *dst = (uint16_t)((((fr * alpha + br * ia) / 255) << 11) |
                      (((fg * alpha + bgc * ia) / 255) << 5) |
                      ((fb * alpha + bb * ia) / 255));
}

static void font_draw_codepoint(uint16_t *framebuffer, int screen_width,
                                int screen_height, int x, int y,
                                uint32_t cp, uint16_t color) {
    stbtt_fontinfo *info = &font_info;
    float scale = font_scale;
    int font_id = 0;
    int glyph_index;
    int slot = -1;
    if (cp < 128 && active_font_id == 0) {
        char c = (char)cp;
        if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
        font_draw_char(framebuffer, screen_width, screen_height, x, y, c, color);
        return;
    }
    if (!font_loaded) return;
    glyph_index = stbtt_FindGlyphIndex(info, (int)cp);
    if (active_font_id == 1 && load_fallback_font()) { info = &fallback_info; scale = fallback_scale; font_id = 1; glyph_index = stbtt_FindGlyphIndex(info, (int)cp); }
    else if (active_font_id == 2 && load_latin_fallback()) { info = &latin_info; scale = latin_scale; font_id = 2; glyph_index = stbtt_FindGlyphIndex(info, (int)cp); }
    if (!glyph_index) {
        if (load_fallback_font()) { info = &fallback_info; scale = fallback_scale; font_id = 1; }
        else if (load_latin_fallback()) { info = &latin_info; scale = latin_scale; font_id = 2; }
        else return;
        glyph_index = stbtt_FindGlyphIndex(info, (int)cp);
    }
    if (!glyph_index && font_id == 1 && load_latin_fallback()) { info = &latin_info; scale = latin_scale; font_id = 2; glyph_index = stbtt_FindGlyphIndex(info, (int)cp); }
    if (!glyph_index) return;
    for (int i = 128; i < 512; i++)
        if (gcache[i].valid && gcache[i].font_id == font_id && gcache[i].codepoint == cp) { slot = i; break; }
    if (slot < 0) {
        for (int i = 128; i < 512; i++) if (!gcache[i].valid) { slot = i; break; }
    }
    if (slot < 0) return;
    struct gcache_ent *g = &gcache[slot];
    if (!g->valid) {
        int xoff, yoff, x1, y1;
        stbtt_GetGlyphBitmapBox(info, glyph_index, scale, scale, &xoff, &yoff, &x1, &y1);
        g->codepoint = cp; g->font_id = font_id;
        g->w = x1 - xoff; g->h = y1 - yoff; g->xoff = xoff; g->yoff = yoff;
        if (g->w <= 0 || g->h <= 0 || g->w > GLYPH_MAX || g->h > GLYPH_MAX) {
            g->w = g->h = 0;
        } else {
            g->bmp = (unsigned char *)malloc((size_t)g->w * g->h);
            if (!g->bmp) return;
            stbtt_MakeGlyphBitmap(info, g->bmp, g->w, g->h, g->w, scale, scale, glyph_index);
        }
        g->valid = 1;
    }
    if (g->w > 0 && g->h > 0) {
        int ascent, descent, line_gap;
        int baseline;
        stbtt_GetFontVMetrics(info, &ascent, &descent, &line_gap);
        baseline = (int)(ascent * scale);
        for (int row = 0; row < g->h; row++) for (int col = 0; col < g->w; col++) {
            unsigned char alpha = g->bmp[row * g->w + col];
            int px = x + g->xoff + col, py = y + baseline + g->yoff + row;
            if (alpha && px >= 0 && px < screen_width && py >= 0 && py < screen_height) {
                font_blend_pixel(&framebuffer[py * screen_width + px], color, alpha);
                /* The broad Unicode face is intentionally a little heavier
                 * when it becomes the language-wide UI face, so it carries
                 * the same visual weight as BPreplayBold. */
                if (language_force_font_id && font_id == language_force_font_id &&
                    px + 1 < screen_width)
                    font_blend_pixel(&framebuffer[py * screen_width + px + 1], color, alpha);
            }
        }
    }
}

static int choose_text_font(const char *text) {
    if (language_force_font_id == 1 && load_fallback_font()) return 1;
    if (language_force_font_id == 2 && load_latin_fallback()) return 2;
    int need = 0; const char *p = text;
    while (p && *p) {
        uint32_t cp = unicode_upper(utf8_next(&p));
        if (!stbtt_FindGlyphIndex(&font_info, (int)cp)) need = 1;
    }
    if (!need) return 0;
    if (load_fallback_font()) {
        p = text; int all = 1; while (*p) { uint32_t cp = utf8_next(&p); if (!stbtt_FindGlyphIndex(&fallback_info, (int)cp)) { all = 0; break; } }
        if (all) return 1;
    }
    if (load_latin_fallback()) {
        p = text; int all = 1; while (*p) { uint32_t cp = utf8_next(&p); if (!stbtt_FindGlyphIndex(&latin_info, (int)cp)) { all = 0; break; } }
        if (all) return 2;
    }
    return 0;
}

static int active_language_supported(stbtt_fontinfo *info) {
    char selected_key[32];
    snprintf(selected_key, sizeof(selected_key), "language.%s", i18n_current_language());
    for (int i = 0; i < i18n_value_count(); i++) {
        const char *key = i18n_key_at(i);
        const char *text = i18n_value_at(i);
        /* Other locale names are stored in every pack for the selector but
         * are not rendered in the active UI. Only inspect the selected one. */
        if (key && strncmp(key, "language.", 9) == 0 && strcmp(key, selected_key) != 0)
            continue;
        for (const char *p = text; p && *p; ) {
            uint32_t cp = unicode_upper(utf8_next(&p));
            if (cp >= 128 && !stbtt_FindGlyphIndex(info, (int)cp)) return 0;
        }
    }
    return 1;
}

void font_sync_language_fallback(void) {
    language_force_font_id = 0;
    if (!font_loaded || active_language_supported(&font_info)) return;

    /* Prefer one fallback that covers the whole active pack. WenQuanYi is our
     * broad CJK/Cyrillic face, while DejaVu covers Latin Extended (including
     * Polish), which WenQuanYi deliberately does not ship. */
    if (load_fallback_font() && active_language_supported(&fallback_info))
        language_force_font_id = 1;
    else if (load_latin_fallback() && active_language_supported(&latin_info))
        language_force_font_id = 2;
}

static uint32_t utf8_next(const char **p) {
    const unsigned char *s = (const unsigned char *)*p;
    uint32_t cp;
    if (s[0] < 0x80) { *p += 1; return s[0]; }
    if ((s[0] & 0xe0) == 0xc0 && s[1]) { cp = s[0] & 0x1f; cp = (cp << 6) | (s[1] & 0x3f); *p += 2; return cp; }
    if ((s[0] & 0xf0) == 0xe0 && s[1] && s[2]) { cp = s[0] & 0x0f; cp = (cp << 6) | (s[1] & 0x3f); cp = (cp << 6) | (s[2] & 0x3f); *p += 3; return cp; }
    if ((s[0] & 0xf8) == 0xf0 && s[1] && s[2] && s[3]) { cp = s[0] & 7; cp = (cp << 6) | (s[1] & 0x3f); cp = (cp << 6) | (s[2] & 0x3f); cp = (cp << 6) | (s[3] & 0x3f); *p += 4; return cp; }
    *p += 1;
    return 0xfffd;
}

/* Vertical metrics for centering: baseline = pixels from glyph-cell top down to
 * the baseline; cap_height = pixel height of capital letters (all text is
 * uppercased, so the visible ink is the cap band [baseline-cap_height, baseline]). */
void font_cap_metrics(int *baseline_out, int *cap_height_out) {
    int baseline = 0, cap = 0;
    if (font_loaded) {
        int ascent, descent, line_gap;
        stbtt_GetFontVMetrics(&font_info, &ascent, &descent, &line_gap);
        baseline = (int)(ascent * font_scale);
        int gi = stbtt_FindGlyphIndex(&font_info, 'H');
        int x0, y0, x1, y1;
        if (gi && stbtt_GetGlyphBox(&font_info, gi, &x0, &y0, &x1, &y1))
            cap = (int)((y1 - y0) * font_scale);
        else
            cap = baseline;
    }
    if (baseline_out)   *baseline_out = baseline;
    if (cap_height_out) *cap_height_out = cap;
}

void font_draw_text(uint16_t *framebuffer, int screen_width, int screen_height,
                   int x, int y, const char *text, uint16_t color) {
    if (!font_loaded || !framebuffer || !text) return;

    int start_x = x;
    active_font_id = choose_text_font(text);
    int prev_glyph = 0;

    while (*text) {
        if (*text == '\n') {
            y += FONT_SIZE + 4;  // Line spacing
            x = start_x;
            text++;
            prev_glyph = 0;
            continue;
        }

        uint32_t cp = utf8_next(&text);
        cp = unicode_upper(cp);
        stbtt_fontinfo *info = &font_info;
        float scale = font_scale;
        if (active_font_id == 1 && load_fallback_font()) { info = &fallback_info; scale = fallback_scale; }
        else if (active_font_id == 2 && load_latin_fallback()) { info = &latin_info; scale = latin_scale; }
        int glyph_index = stbtt_FindGlyphIndex(info, (int)cp);
        if (!glyph_index && active_font_id == 0 && cp >= 128) {
            if (load_fallback_font()) { info = &fallback_info; scale = fallback_scale; glyph_index = stbtt_FindGlyphIndex(info, (int)cp); }
            if (!glyph_index && load_latin_fallback()) { info = &latin_info; scale = latin_scale; glyph_index = stbtt_FindGlyphIndex(info, (int)cp); }
        }

        if (glyph_index != 0) {
            // Get advance width and left side bearing
            int advance_width, left_side_bearing;
            stbtt_GetGlyphHMetrics(info, glyph_index, &advance_width, &left_side_bearing);

            // Apply kerning if we have a previous character
            if (prev_glyph != 0) {
                int kern = stbtt_GetGlyphKernAdvance(info, prev_glyph, glyph_index);
                x += (int)(kern * scale);
            }

            // Draw the character
            font_draw_codepoint(framebuffer, screen_width, screen_height, x, y, cp, color);

            // Advance cursor
            x += (int)(advance_width * scale);
            prev_glyph = glyph_index;
        } else {
            // Space or unknown character
            x += FONT_CHAR_SPACING;
            prev_glyph = 0;
        }
    }
    active_font_id = 0;
}

int font_measure_text(const char *text) {
    if (!text || !font_loaded) return 0;

    int width = 0;
    active_font_id = choose_text_font(text);
    int prev_glyph = 0;

    while (*text) {
        // Skip newlines
        if (*text == '\n') {
            text++;
            prev_glyph = 0;
            continue;
        }

        uint32_t cp = utf8_next(&text);
        cp = unicode_upper(cp);
        stbtt_fontinfo *info = &font_info;
        float scale = font_scale;
        if (active_font_id == 1 && load_fallback_font()) { info = &fallback_info; scale = fallback_scale; }
        else if (active_font_id == 2 && load_latin_fallback()) { info = &latin_info; scale = latin_scale; }
        int glyph_index = stbtt_FindGlyphIndex(info, (int)cp);
        if (!glyph_index && active_font_id == 0 && cp >= 128) {
            if (load_fallback_font()) { info = &fallback_info; scale = fallback_scale; glyph_index = stbtt_FindGlyphIndex(info, (int)cp); }
            if (!glyph_index && load_latin_fallback()) { info = &latin_info; scale = latin_scale; glyph_index = stbtt_FindGlyphIndex(info, (int)cp); }
        }

        if (glyph_index != 0) {
            // Get advance width
            int advance_width, left_side_bearing;
            stbtt_GetGlyphHMetrics(info, glyph_index, &advance_width, &left_side_bearing);

            // Apply kerning if we have a previous character
            if (prev_glyph != 0) {
                int kern = stbtt_GetGlyphKernAdvance(info, prev_glyph, glyph_index);
                width += (int)(kern * scale);
            }

            // Add character width
            width += (int)(advance_width * scale);
            prev_glyph = glyph_index;
        } else {
            // Space or unknown character
            width += FONT_CHAR_SPACING;
            prev_glyph = 0;
        }
    }

    active_font_id = 0;
    return width;
}
