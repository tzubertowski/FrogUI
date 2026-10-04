#include "font.h"
#include "common/i18n.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include <hb-ft.h>
#include <hb.h>

#include <SheenBidi/SheenBidi.h>

#ifndef UI_SCALE
#define UI_SCALE 100
#endif
#define DEFAULT_FONT_SIZE 23
static float ui_font_size = DEFAULT_FONT_SIZE * UI_SCALE / 100.0f;

#define GLYPH_CACHE_SIZE 4096
#define MAX_STACK_CODEPOINTS 512
#define GLYPH_PIXEL_POOL_SIZE (2 * 1024 * 1024)

typedef struct {
  uint32_t glyph_index;
  int font_id;
  int is_bold;
  int width;
  int rows;
  int left;
  int top;
  uint8_t *bitmap;
} CachedGlyph;

typedef struct {
  FT_Face ft_face;
  hb_font_t *hb_font;
  void *mmap_ptr;
  size_t mmap_size;
  int loaded;
} FontFace;

static uint8_t glyph_bitmap_arena[GLYPH_PIXEL_POOL_SIZE];
static size_t arena_offset = 0;

static FT_Library ft_library = NULL;
static FontFace primary_font = {0};
static FontFace fallback_font = {0};
static FontFace latin_font = {0};

static hb_buffer_t *hb_buf = NULL;
static int language_force_font_id = 0;

static CachedGlyph glyph_cache[GLYPH_CACHE_SIZE];

static void clear_glyph_cache(void) {
  memset(glyph_cache, 0, sizeof(glyph_cache));
  for (int i = 0; i < GLYPH_CACHE_SIZE; i++) {
    glyph_cache[i].font_id = -1;
  }
  arena_offset = 0;
}

static inline uint16_t blend_channel(uint32_t fg, uint32_t bg,
                                     unsigned char alpha) {
  uint32_t res = (fg * alpha + bg * (255 - alpha) + 128) >> 8;
  return (uint16_t)res;
}

static inline void font_blend_pixel(uint16_t *dst, uint16_t color,
                                    unsigned char alpha) {
  if (alpha == 255) {
    *dst = color;
    return;
  }
  if (alpha == 0)
    return;

  uint16_t bg = *dst;
  uint32_t fr = (color >> 11) & 0x1F;
  uint32_t fg = (color >> 5) & 0x3F;
  uint32_t fb = color & 0x1F;

  uint32_t br = (bg >> 11) & 0x1F;
  uint32_t bgc = (bg >> 5) & 0x3F;
  uint32_t bb = bg & 0x1F;

  uint32_t rr = blend_channel(fr, br, alpha);
  uint32_t rg = blend_channel(fg, bgc, alpha);
  uint32_t rb = blend_channel(fb, bb, alpha);

  *dst = (uint16_t)((rr << 11) | (rg << 5) | rb);
}

static uint32_t utf8_next(const char **p) {
  const unsigned char *s = (const unsigned char *)*p;
  uint32_t cp;
  if (s[0] < 0x80) {
    *p += 1;
    return s[0];
  }
  if ((s[0] & 0xe0) == 0xc0 && s[1]) {
    cp = s[0] & 0x1f;
    cp = (cp << 6) | (s[1] & 0x3f);
    *p += 2;
    return cp;
  }
  if ((s[0] & 0xf0) == 0xe0 && s[1] && s[2]) {
    cp = s[0] & 0x0f;
    cp = (cp << 6) | (s[1] & 0x3f);
    cp = (cp << 6) | (s[2] & 0x3f);
    *p += 3;
    return cp;
  }
  if ((s[0] & 0xf8) == 0xf0 && s[1] && s[2] && s[3]) {
    cp = s[0] & 7;
    cp = (cp << 6) | (s[1] & 0x3f);
    cp = (cp << 6) | (s[2] & 0x3f);
    cp = (cp << 6) | (s[3] & 0x3f);
    *p += 4;
    return cp;
  }
  *p += 1;
  return 0xfffd;
}

static uint8_t *arena_alloc(size_t size) {
  if (arena_offset + size > GLYPH_PIXEL_POOL_SIZE) {
    clear_glyph_cache();
  }

  uint8_t *ptr = &glyph_bitmap_arena[arena_offset];
  arena_offset += size;
  return ptr;
}

static CachedGlyph *get_cached_glyph(FontFace *face, uint32_t glyph_index,
                                     int font_id, int is_bold) {
  int is_native_bold = (face->ft_face->style_flags & FT_STYLE_FLAG_BOLD) != 0;
  int effective_bold = is_bold && !is_native_bold;

  uint32_t hash = (glyph_index ^ ((uint32_t)font_id * 0x9e3779b9) ^
                   ((uint32_t)effective_bold * 0x85ebca6b)) %
                  GLYPH_CACHE_SIZE;

  if (glyph_cache[hash].glyph_index == glyph_index &&
      glyph_cache[hash].font_id == font_id &&
      glyph_cache[hash].is_bold == effective_bold && glyph_cache[hash].bitmap) {
    return &glyph_cache[hash];
  }

  FT_Int32 load_flags = FT_LOAD_TARGET_NORMAL | FT_LOAD_DEFAULT;
  if (FT_Load_Glyph(face->ft_face, glyph_index, load_flags)) {
    return NULL;
  }

  if (effective_bold &&
      face->ft_face->glyph->format == FT_GLYPH_FORMAT_OUTLINE) {
    FT_Pos strength = (1 << 6);
    FT_Outline_Embolden(&face->ft_face->glyph->outline, strength);
  }

  if (FT_Render_Glyph(face->ft_face->glyph, FT_RENDER_MODE_NORMAL)) {
    return NULL;
  }

  FT_Bitmap *bitmap = &face->ft_face->glyph->bitmap;

  glyph_cache[hash].glyph_index = glyph_index;
  glyph_cache[hash].font_id = font_id;
  glyph_cache[hash].is_bold = effective_bold;
  glyph_cache[hash].width = bitmap->width;
  glyph_cache[hash].rows = bitmap->rows;
  glyph_cache[hash].left = face->ft_face->glyph->bitmap_left;
  glyph_cache[hash].top = face->ft_face->glyph->bitmap_top;

  size_t size = (size_t)bitmap->width * bitmap->rows;
  if (size > 0) {
    glyph_cache[hash].bitmap = arena_alloc(size);
    if (glyph_cache[hash].bitmap) {
      for (unsigned int r = 0; r < bitmap->rows; r++) {
        memcpy(glyph_cache[hash].bitmap + (r * bitmap->width),
               bitmap->buffer + (r * bitmap->pitch), bitmap->width);
      }
    }
  } else {
    glyph_cache[hash].bitmap = NULL;
  }

  return &glyph_cache[hash];
}

static void update_face_pixel_sizes(FontFace *face) {
  if (face && face->loaded && face->ft_face) {
    FT_Set_Pixel_Sizes(face->ft_face, 0, (FT_UInt)ui_font_size);
    hb_ft_font_changed(face->hb_font);
  }
}

void font_set_size(int pixels) {
  if (pixels < 18)
    pixels = 18;
  if (pixels > DEFAULT_FONT_SIZE)
    pixels = DEFAULT_FONT_SIZE;
  ui_font_size = pixels * UI_SCALE / 100.0f;

  update_face_pixel_sizes(&primary_font);
  update_face_pixel_sizes(&fallback_font);
  update_face_pixel_sizes(&latin_font);

  clear_glyph_cache();
}

int font_get_size(void) {
  return (int)(ui_font_size * 100.0f / UI_SCALE + 0.5f);
}

static void unload_font_face(FontFace *face) {
  if (face->hb_font) {
    hb_font_destroy(face->hb_font);
    face->hb_font = NULL;
  }
  if (face->ft_face) {
    FT_Done_Face(face->ft_face);
    face->ft_face = NULL;
  }
  if (face->mmap_ptr && face->mmap_ptr != MAP_FAILED) {
    munmap(face->mmap_ptr, face->mmap_size);
    face->mmap_ptr = NULL;
    face->mmap_size = 0;
  }
  face->loaded = 0;
  clear_glyph_cache();
}

static int load_font_face(FontFace *face, const char *paths[], int path_count) {
  int fd = -1;
  struct stat st;

  for (int i = 0; i < path_count; i++) {
    if (!paths[i])
      continue;
    fd = open(paths[i], O_RDONLY);
    if (fd >= 0)
      break;
  }

  if (fd < 0)
    return 0;

  if (fstat(fd, &st) < 0 || st.st_size <= 0) {
    close(fd);
    return 0;
  }

  unload_font_face(face);

  void *mapped = mmap(NULL, st.st_size, PROT_READ, MAP_SHARED, fd, 0);
  close(fd);

  if (mapped == MAP_FAILED) {
    return 0;
  }

  face->mmap_ptr = mapped;
  face->mmap_size = (size_t)st.st_size;

  if (FT_New_Memory_Face(ft_library, (const FT_Byte *)face->mmap_ptr,
                         (FT_Long)face->mmap_size, 0, &face->ft_face)) {
    unload_font_face(face);
    return 0;
  }

  FT_Set_Pixel_Sizes(face->ft_face, 0, (FT_UInt)ui_font_size);
  face->hb_font = hb_ft_font_create_referenced(face->ft_face);
  if (!face->hb_font) {
    unload_font_face(face);
    return 0;
  }

  face->loaded = 1;
  return 1;
}

static int load_font_file(const char *font_filename) {
  char font_paths[3][256];
  snprintf(font_paths[0], sizeof(font_paths[0]), "/mnt/sdcard/cubegm/fonts/%s",
           font_filename);
  snprintf(font_paths[1], sizeof(font_paths[1]), "/mnt/sdcard/frogui/fonts/%s",
           font_filename);
  snprintf(font_paths[2], sizeof(font_paths[2]), "fonts/%s", font_filename);

  const char *paths[3] = {font_paths[0], font_paths[1], font_paths[2]};
  return load_font_face(&primary_font, paths, 3);
}

static int load_fallback_font(void) {
  if (fallback_font.loaded)
    return 1;

  const char *font_name = get_default_language_font_name("TreeFrogUnicode.ttf");

  char path0[256], path1[256], path2[256];
  snprintf(path0, sizeof(path0), "/mnt/sdcard/frogui/fonts/%s", font_name);
  snprintf(path1, sizeof(path1), "/mnt/sdcard/cubegm/fonts/%s", font_name);
  snprintf(path2, sizeof(path2), "fonts/%s", font_name);

  const char *paths[] = {path0, path1, path2};
  return load_font_face(&fallback_font, paths, 3);
}

static int load_latin_fallback(void) {
  if (latin_font.loaded)
    return 1;
  const char *paths[] = {"/mnt/sdcard/frogui/fonts/TreeFrogLatin.ttf",
                         "/mnt/sdcard/cubegm/fonts/TreeFrogLatin.ttf",
                         "fonts/TreeFrogLatin.ttf"};
  return load_font_face(&latin_font, paths, 3);
}

void font_load_file(const char *font_filename) {
  if (!font_filename || !font_filename[0])
    return;
  if (load_font_file(font_filename)) {
    font_sync_language_fallback();
  }
}

void font_load_from_settings(const char *font_name) {
  const char *font_filename = NULL;
  if (strcmp(font_name, "Monogram") == 0) {
    font_filename = "monogram.ttf";
  } else if (strcmp(font_name, "GamePocket") == 0) {
    font_filename = "GamePocket-Regular-ZeroKern.ttf";
  } else {
    font_filename = "BPreplayBold.otf";
  }
  if (load_font_file(font_filename)) {
    font_sync_language_fallback();
  }
}

void font_init(void) {
  if (FT_Init_FreeType(&ft_library))
    return;
  hb_buf = hb_buffer_create();

  if (!load_font_file("BPreplayBold.otf")) {
    font_load_from_settings("GamePocket");
  }

  load_fallback_font();
  load_latin_fallback();
  font_sync_language_fallback();
}

static inline FontFace *get_face_for_codepoint(uint32_t cp, int *out_font_id) {
  if (language_force_font_id == 0 && primary_font.loaded) {
    if (FT_Get_Char_Index(primary_font.ft_face, cp) != 0) {
      if (out_font_id)
        *out_font_id = 0;
      return &primary_font;
    }
  }

  if ((language_force_font_id == 1 || language_force_font_id == 0) &&
      load_fallback_font()) {
    if (FT_Get_Char_Index(fallback_font.ft_face, cp) != 0) {
      if (out_font_id)
        *out_font_id = 1;
      return &fallback_font;
    }
  }

  if ((language_force_font_id == 2 || language_force_font_id == 0) &&
      load_latin_fallback()) {
    if (FT_Get_Char_Index(latin_font.ft_face, cp) != 0) {
      if (out_font_id)
        *out_font_id = 2;
      return &latin_font;
    }
  }

  if (primary_font.loaded) {
    if (out_font_id)
      *out_font_id = 0;
    return &primary_font;
  }

  if (out_font_id)
    *out_font_id = 1;
  return fallback_font.loaded ? &fallback_font : NULL;
}

static void shape_and_render_text(uint16_t *framebuffer, int screen_width,
                                  int screen_height, int x, int y,
                                  const char *text, uint16_t color, int is_bold,
                                  int measure_only, int *out_width) {
  if (!text || !*text) {
    if (out_width)
      *out_width = 0;
    return;
  }

  int start_x = x;
  int current_y = y;
  int max_width = 0;
  const char *line_start = text;

  while (*line_start) {
    SBUInt32 utf32_buf[MAX_STACK_CODEPOINTS];
    SBUInt32 codepoint_count = 0;
    const char *p = line_start;

    while (*p && *p != '\n' && codepoint_count < MAX_STACK_CODEPOINTS) {
      utf32_buf[codepoint_count++] = utf8_next(&p);
    }

    if (codepoint_count > 0) {
      SBCodepointSequence sequence = {.stringEncoding = SBStringEncodingUTF32,
                                      .stringBuffer = utf32_buf,
                                      .stringLength = codepoint_count};

      SBAlgorithmRef algorithm = SBAlgorithmCreate(&sequence);
      if (algorithm) {
        SBParagraphRef paragraph = SBAlgorithmCreateParagraph(
            algorithm, 0, codepoint_count, SBLevelDefaultLTR);
        if (paragraph) {
          SBUInt32 paragraph_length = SBParagraphGetLength(paragraph);
          SBLineRef line =
              SBParagraphCreateLine(paragraph, 0, paragraph_length);

          if (line) {
            SBUInt32 run_count = SBLineGetRunCount(line);
            const SBRun *runs = SBLineGetRunsPtr(line);

            int line_width_fractional = 0;
            int cursor_x_fractional = start_x << 6;

            for (SBUInt32 r = 0; r < run_count; r++) {
              SBUInt32 run_offset = runs[r].offset;
              SBUInt32 run_len = runs[r].length;
              SBLevel run_level = runs[r].level;

              SBUInt32 sub_idx = 0;
              while (sub_idx < run_len) {
                int active_font_id = 0;
                FontFace *face = get_face_for_codepoint(
                    utf32_buf[run_offset + sub_idx], &active_font_id);

                if (!face || !face->loaded) {
                  sub_idx++;
                  continue;
                }

                SBUInt32 chunk_len = 0;
                while ((sub_idx + chunk_len) < run_len) {
                  int next_font_id = 0;
                  get_face_for_codepoint(
                      utf32_buf[run_offset + sub_idx + chunk_len],
                      &next_font_id);
                  if (next_font_id != active_font_id)
                    break;
                  chunk_len++;
                }

                hb_buffer_clear_contents(hb_buf);
                hb_buffer_add_utf32(
                    hb_buf, (const uint32_t *)&utf32_buf[run_offset + sub_idx],
                    chunk_len, 0, chunk_len);
                hb_buffer_set_direction(hb_buf, (run_level & 1)
                                                    ? HB_DIRECTION_RTL
                                                    : HB_DIRECTION_LTR);
                hb_buffer_guess_segment_properties(hb_buf);

                hb_shape(face->hb_font, hb_buf, NULL, 0);

                unsigned int glyph_count;
                hb_glyph_info_t *glyph_info =
                    hb_buffer_get_glyph_infos(hb_buf, &glyph_count);
                hb_glyph_position_t *glyph_pos =
                    hb_buffer_get_glyph_positions(hb_buf, &glyph_count);

                int baseline =
                    (face->ft_face->size->metrics.ascender + 32) >> 6;
                int is_native_bold =
                    (face->ft_face->style_flags & FT_STYLE_FLAG_BOLD) != 0;

                for (unsigned int i = 0; i < glyph_count; i++) {
                  uint32_t glyph_index = glyph_info[i].codepoint;

                  int x_offset = glyph_pos[i].x_offset;
                  int y_offset = glyph_pos[i].y_offset >> 6;
                  int x_advance = glyph_pos[i].x_advance;

                  if (is_bold && !is_native_bold) {
                    x_advance += (1 << 6);
                  }

                  if (!measure_only && framebuffer) {
                    CachedGlyph *cg = get_cached_glyph(face, glyph_index,
                                                       active_font_id, is_bold);
                    if (cg && cg->bitmap) {
                      int pen_x = (cursor_x_fractional + 32) >>
                                  6; // Round to nearest pixel
                      int draw_x = pen_x + (x_offset >> 6) + cg->left;
                      int draw_y = current_y + baseline - cg->top - y_offset;

                      for (int row = 0; row < cg->rows; row++) {
                        int py = draw_y + row;
                        if (py < 0 || py >= screen_height)
                          continue;

                        uint16_t *line_dst = &framebuffer[py * screen_width];
                        uint8_t *src_ptr = &cg->bitmap[row * cg->width];

                        for (int col = 0; col < cg->width; col++) {
                          uint8_t alpha = src_ptr[col];
                          if (alpha > 0) {
                            int px = draw_x + col;
                            if (px >= 0 && px < screen_width) {
                              font_blend_pixel(&line_dst[px], color, alpha);
                            }
                          }
                        }
                      }
                    }
                  }

                  cursor_x_fractional += x_advance;
                  line_width_fractional += x_advance;
                }

                sub_idx += chunk_len;
              }
            }

            int line_width_px = (line_width_fractional + 32) >> 6;
            if (line_width_px > max_width) {
              max_width = line_width_px;
            }

            SBLineRelease(line);
          }
          SBParagraphRelease(paragraph);
        }
        SBAlgorithmRelease(algorithm);
      }
    }

    line_start = p;
    if (*line_start == '\n') {
      line_start++;
      current_y += (int)ui_font_size + 4;
    }
  }

  if (out_width)
    *out_width = max_width;
}

void font_draw_text(uint16_t *framebuffer, int screen_width, int screen_height,
                    int x, int y, const char *text, uint16_t color, ...) {
  int is_bold = 0;
  va_list args;
  va_start(args, color);
  /* Extract optional is_bold parameter if passed */
  is_bold = va_arg(args, int);
  va_end(args);

  shape_and_render_text(framebuffer, screen_width, screen_height, x, y, text,
                        color, is_bold, 0, NULL);
}

int font_measure_text(const char *text, ...) {
  int is_bold = 0;
  va_list args;
  va_start(args, text);
  /* Extract optional is_bold parameter if passed */
  is_bold = va_arg(args, int);
  va_end(args);

  int width = 0;
  shape_and_render_text(NULL, 0, 0, 0, 0, text, 0, is_bold, 1, &width);
  return width;
}

void font_cap_metrics(int *baseline_out, int *cap_height_out) {
  int baseline = 0, cap = 0;

  FontFace *active_face = &primary_font;
  if (language_force_font_id == 1 && fallback_font.loaded) {
    active_face = &fallback_font;
  } else if (language_force_font_id == 2 && latin_font.loaded) {
    active_face = &latin_font;
  }

  if (active_face->loaded && active_face->ft_face) {
    FT_Size_Metrics *metrics = &active_face->ft_face->size->metrics;
    baseline = metrics->ascender >> 6;

    FT_UInt gi = FT_Get_Char_Index(active_face->ft_face, 'H');
    if (gi && !FT_Load_Glyph(active_face->ft_face, gi, FT_LOAD_DEFAULT)) {
      cap = (int)(active_face->ft_face->glyph->metrics.height >> 6);
    } else {
      cap = baseline;
    }
  }

  if (baseline_out)
    *baseline_out = baseline;
  if (cap_height_out)
    *cap_height_out = cap;
}

static int active_language_supported(FontFace *face) {
  if (!face || !face->loaded)
    return 0;

  char selected_key[32];
  snprintf(selected_key, sizeof(selected_key), "language.%s",
           i18n_current_language());

  int total_checks = 0;
  int supported_glyphs = 0;

  for (int i = 0; i < i18n_value_count(); i++) {
    const char *key = i18n_key_at(i);
    const char *text = i18n_value_at(i);

    if (key && strncmp(key, "language.", 9) == 0 &&
        strcmp(key, selected_key) != 0) {
      continue;
    }

    for (const char *p = text; p && *p;) {
      uint32_t cp = utf8_next(&p);

      if (cp <= 0x007F)
        continue;

      total_checks++;
      if (FT_Get_Char_Index(face->ft_face, cp) != 0) {
        supported_glyphs++;
      } else {
        return 0;
      }
    }
  }

  if (total_checks == 0)
    return 1;
  return (supported_glyphs == total_checks);
}

void font_sync_language_fallback(void) {
  int old_font_id = language_force_font_id;
  language_force_font_id = 0;

  if (fallback_font.loaded) {
    unload_font_face(&fallback_font);
  }

  if (!primary_font.loaded || active_language_supported(&primary_font)) {
    if (old_font_id != language_force_font_id) {
      clear_glyph_cache();
    }
    return;
  }

  if (load_fallback_font() && active_language_supported(&fallback_font)) {
    language_force_font_id = 1;
  } else if (load_latin_fallback() && active_language_supported(&latin_font)) {
    language_force_font_id = 2;
  }

  clear_glyph_cache();
}
