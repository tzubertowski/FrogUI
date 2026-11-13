#include "render.h"
#include "frogui_settings.h"
#include "common/i18n.h"
#include "font.h"
#include "stb_image.h" /* decls only; impl lives in banner.c */
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

/* Runtime panel geometry (defaults = SF3000 854x480; picoarch overrides via
 * env). */
int g_screen_w = 854;
int g_screen_h = 480;
int g_ui_scale = 150;

void render_set_geometry(int w, int h, int ui_scale) {
  if (w > 0)
    g_screen_w = w;
  if (h > 0)
    g_screen_h = h;
  if (ui_scale > 0)
    g_ui_scale = ui_scale;
}

void render_init(uint16_t *framebuffer) {
  if (framebuffer) {
    render_clear_screen(framebuffer);
  }
}

void render_clear_screen(uint16_t *framebuffer) {
  if (!framebuffer)
    return;

  // Fill with background color
  for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
    framebuffer[i] = COLOR_BG;
  }
}

void render_fill_rect(uint16_t *framebuffer, int x, int y, int width,
                      int height, uint16_t color) {
  if (!framebuffer)
    return;

  for (int py = y; py < y + height && py < SCREEN_HEIGHT; py++) {
    for (int px = x; px < x + width && px < SCREEN_WIDTH; px++) {
      if (px >= 0 && py >= 0) {
        framebuffer[py * SCREEN_WIDTH + px] = color;
      }
    }
  }
}

static uint16_t blend_pixel(uint16_t foreground, uint16_t background, unsigned alpha) {
  unsigned r = (((foreground >> 11) & 31) * alpha + ((background >> 11) & 31) * (255 - alpha)) / 255;
  unsigned g = (((foreground >> 5) & 63) * alpha + ((background >> 5) & 63) * (255 - alpha)) / 255;
  unsigned b = ((foreground & 31) * alpha + (background & 31) * (255 - alpha)) / 255;
  return (uint16_t)((r << 11) | (g << 5) | b);
}

void render_rounded_rect_alpha(uint16_t *framebuffer, int x, int y, int width,
                               int height, int radius, uint16_t color,
                               uint8_t alpha) {
  if (!framebuffer)
    return;
  if (!alpha) return;
  if (alpha == 255) {
    render_fill_rect(framebuffer, x + radius, y, width - 2 * radius, height, color);
    render_fill_rect(framebuffer, x, y + radius, width, height - 2 * radius, color);
  } else {
    for (int py = y; py < y + height; py++)
      for (int px = x; px < x + width; px++) {
        if (px < 0 || px >= SCREEN_WIDTH || py < 0 || py >= SCREEN_HEIGHT) continue;
        int cx = px < x + radius ? x + radius : px >= x + width - radius ? x + width - radius - 1 : px;
        int cy = py < y + radius ? y + radius : py >= y + height - radius ? y + height - radius - 1 : py;
        int dx = px - cx, dy = py - cy;
        if ((px >= x + radius && px < x + width - radius) ||
            (py >= y + radius && py < y + height - radius) ||
            dx * dx + dy * dy <= radius * radius)
          framebuffer[py * SCREEN_WIDTH + px] = blend_pixel(color, framebuffer[py * SCREEN_WIDTH + px], alpha);
      }
    return;
  }

  // Draw main body (excluding corners)
  // Draw rounded corners using circle approximation
  for (int corner_y = 0; corner_y < radius; corner_y++) {
    for (int corner_x = 0; corner_x < radius; corner_x++) {
      int dx = radius - corner_x;
      int dy = radius - corner_y;
      int dist_sq = dx * dx + dy * dy;
      int radius_sq = radius * radius;

      if (dist_sq <= radius_sq) {
        // Top-left corner
        int px = x + corner_x;
        int py = y + corner_y;
        if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
          framebuffer[py * SCREEN_WIDTH + px] = color;
        }

        // Top-right corner
        px = x + width - 1 - corner_x;
        py = y + corner_y;
        if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
          framebuffer[py * SCREEN_WIDTH + px] = color;
        }

        // Bottom-left corner
        px = x + corner_x;
        py = y + height - 1 - corner_y;
        if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
          framebuffer[py * SCREEN_WIDTH + px] = color;
        }

        // Bottom-right corner
        px = x + width - 1 - corner_x;
        py = y + height - 1 - corner_y;
        if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
          framebuffer[py * SCREEN_WIDTH + px] = color;
        }
      }
    }
  }
}

void render_rounded_rect(uint16_t *framebuffer, int x, int y, int width,
                         int height, int radius, uint16_t color) {
  render_rounded_rect_alpha(framebuffer, x, y, width, height, radius, color, 255);
}

void render_text_pillbox(uint16_t *framebuffer, int x, int y, const char *text,
                         uint16_t bg_color, uint16_t text_color, int padding,
                         int is_bold) {
  if (!framebuffer || !text)
    return;

  int baseline, cap_h;
  font_cap_metrics(&baseline, &cap_h);
  int text_width = font_measure_text(text, is_bold);

  int pad_x = padding;
  int pad_y = padding;

  // Total box height based on actual cap height
  int pillbox_width = text_width + (pad_x * 2);
  int pillbox_height = cap_h + (pad_y * 2);

  int pillbox_x = x - pad_x;
  // Offset pillbox background to bound text vertically
  int pillbox_y = y + (baseline - cap_h) - pad_y;

  render_rounded_rect(framebuffer, pillbox_x, pillbox_y, pillbox_width,
                      pillbox_height, 6, bg_color);
  font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, x, y, text,
                 text_color, is_bold);
}

void render_battery_colors(uint16_t *framebuffer, int pct, uint16_t bg_color,
                           uint16_t accent_color) {
  if (!framebuffer || pct < 0)
    return;
  if (pct > 100)
    pct = 100;
  extern int frogui_battery_charging(void);
  extern int frogui_battery_color_mode(void);
  int charging = frogui_battery_charging();

  /* Same battery glyph in both modes. "Battery Colour Mode" just changes the
   * fill colour to a level band: green 70-100, blue 30-70, red 0-30 (charging
   * = green). Normal mode: accent fill, red under 15%. */
  int color_mode = frogui_battery_color_mode();

  /* NextUI-style: rounded body + a little terminal nub, fill proportional. */
  int compact = frogui_prominimal_style_active();
  int bw = UI_S(compact ? 18 : 26), bh = UI_S(compact ? 10 : 13); /* body */
  int nub_w = UI_S(compact ? 2 : 3), nub_h = bh / 2;
  int pad = UI_S(2);
  int x = SCREEN_WIDTH - PADDING - bw - nub_w;
  int y = 10; /* header baseline area */

  uint16_t outline = accent_color;
  uint16_t green = 0x2FE6; /* charging fill */
  uint16_t fillcol;
  if (compact)
    fillcol = outline;
  else if (color_mode)
    fillcol = charging      ? green
              : (pct >= 70) ? green
              : (pct >= 30) ? 0x041F
                            : 0xF800;
  else
    fillcol = charging ? green : ((pct <= 15) ? 0xF800 /*red*/ : accent_color);

  /* body outline (rounded), hollow */
  int r = UI_S(3);
  render_rounded_rect(framebuffer, x, y, bw, bh, r, outline);
  render_rounded_rect(framebuffer, x + 1, y + 1, bw - 2, bh - 2, r - 1,
                      bg_color);
  /* terminal nub */
  render_fill_rect(framebuffer, x + bw, y + (bh - nub_h) / 2, nub_w, nub_h,
                   outline);
  /* fill proportional to pct */
  int inner_w = bw - 2 * pad;
  int fw = inner_w * pct / 100;
  if (fw < 0)
    fw = 0;
  if (fw > inner_w)
    fw = inner_w;
  if (fw > 0)
    render_fill_rect(framebuffer, x + pad, y + pad, fw, bh - 2 * pad, fillcol);

  /* Charging bolt: a small lightning glyph centered on the body. */
  if (charging && !compact) {
    int cx = x + bw / 2, cy = y + bh / 2;
    uint16_t bolt = compact ? outline : 0xFFFF;
    int s = UI_S(1);
    if (s < 1)
      s = 1;
    /* two offset triangles forming a zig-zag bolt (cheap raster) */
    for (int i = -bh / 2 + pad; i < bh / 2 - pad; i++) {
      int w = (i < 0) ? (bh / 2 + i) : (bh / 2 - i); /* taper */
      int off = (i < 0) ? -s : s;
      for (int j = -s; j <= s; j++)
        render_fill_rect(framebuffer, cx + off + j, cy + i, 1, 1, bolt);
      (void)w;
    }
  }
}

void render_battery(uint16_t *framebuffer, int pct) {
  render_battery_colors(framebuffer, pct, COLOR_BG, COLOR_HEADER);
}

void render_header(uint16_t *framebuffer, const char *title) {
  if (!framebuffer || !title)
    return;

  render_fill_rect(framebuffer, 0, 0, SCREEN_WIDTH, HEADER_HEIGHT,
                   render_theme_colour(RENDER_COLOUR_HEADER_BACKGROUND, COLOR_BG));
  // Draw folder/section name in header area
  font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, CONTENT_X, 10, title,
                 COLOR_HEADER, 1);

  // Battery indicator, top-right (custom icon; fb1 masking is disabled for
  // test).
  {
    extern int frogui_battery_pct(void);
    render_battery(framebuffer, frogui_battery_pct());
  }
}

void render_tabs(uint16_t *framebuffer, int active, uint16_t header_bg) {
  static const char *keys[] = {"tab.recents", "tab.games", "tab.apps",
                               "tab.settings"};
  extern int frogui_battery_pct(void);
  extern int frogui_apps_in_settings_get(void);
  if (!framebuffer)
    return;

  int x = CONTENT_X;
  int y = UI_S(5);
  int h = HEADER_HEIGHT - UI_S(5);
  int gap = UI_S(8);
  int compact = frogui_prominimal_style_active();
  header_bg = render_theme_colour(RENDER_COLOUR_HEADER_BACKGROUND, header_bg);
  render_fill_rect(framebuffer, 0, 0, SCREEN_WIDTH, HEADER_HEIGHT, header_bg);
  if (frogui_apps_in_settings_get() && active == 2)
    active = 3;
  for (int i = 0; i < 4; i++) {
    if (i == 2 && frogui_apps_in_settings_get())
      continue;
    int pad = UI_S(compact ? 5 : 7);
    const char *label = tr(keys[i]);
    int tw = font_measure_text(label, 1);
    int w = tw + pad * 2;
    int baseline, cap_h;
    font_cap_metrics(&baseline, &cap_h);
    int ty = y + h / 2 - (baseline - cap_h / 2);
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, x + pad, ty, label,
                   i == active ? COLOR_TEXT : COLOR_DISABLED, 1);
    x += w + gap;
  }
  render_battery_colors(framebuffer, frogui_battery_pct(), header_bg,
                        COLOR_HEADER);
}

void render_legend(uint16_t *framebuffer, int x_button_mode, int show_select,
                   int show_search) {
  if (!framebuffer)
    return;

  font_set_render_scale(85);
  int pill_h = UI_S(render_theme_current()->legend_height);
  int legend_y = SCREEN_HEIGHT - pill_h - UI_S(6);
  int baseline, cap_h;
  font_cap_metrics(&baseline, &cap_h);
  int text_y = legend_y + pill_h / 2 - (baseline - cap_h / 2);
  int spacing = UI_S(6);
  int anchor = SCREEN_WIDTH - PADDING;

  {
    const char *nav = tr("legend.enter_back");
    int w = font_measure_text(nav, 1);
    int x = anchor - w;
    if (frogui_pillboxes_get())
      render_rounded_rect(framebuffer, x - UI_S(3), legend_y, w + UI_S(6), pill_h, UI_S(6),
                          COLOR_LEGEND_BG);
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, x, text_y, nav,
                   COLOR_LEGEND, 1);
    anchor = x - spacing;
  }

  if (x_button_mode != LEGEND_X_NONE) {
    const char *xl = tr(x_button_mode == LEGEND_X_REMOVE ? "legend.unfavourite"
                                                         : "legend.favourite");
    int w = font_measure_text(xl, 1);
    int x = anchor - w;
    if (frogui_pillboxes_get())
      render_rounded_rect(framebuffer, x - UI_S(3), legend_y, w + UI_S(6), pill_h, UI_S(6),
                          COLOR_LEGEND_BG);
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, x, text_y, xl,
                   COLOR_LEGEND, 1);
    anchor = x - spacing;
  }

  if (show_select) {
    const char *sl = tr("legend.options");
    int w = font_measure_text(sl, 1);
    int x = anchor - w;
    if (frogui_pillboxes_get())
      render_rounded_rect(framebuffer, x - UI_S(3), legend_y, w + UI_S(6), pill_h, UI_S(6),
                          COLOR_LEGEND_BG);
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, x, text_y, sl,
                   COLOR_LEGEND, 1);
    anchor = x - spacing;
  }

  if (show_search) {
    const char *xs = tr("legend.search");
    int w = font_measure_text(xs, 1);
    int x = anchor - w;
    if (frogui_pillboxes_get())
      render_rounded_rect(framebuffer, x - UI_S(3), legend_y, w + UI_S(6), pill_h, UI_S(6),
                          COLOR_LEGEND_BG);
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, x, text_y, xs,
                   COLOR_LEGEND, 1);
  }

  font_set_render_scale(100);
}

void render_scroll_indicator(uint16_t *framebuffer, int total, int selected,
                             int visible) {
  if (!framebuffer || total <= visible || visible <= 0)
    return;
  int top = START_Y;
  int bottom = render_theme_current()->content_height > 0
             ? START_Y + UI_S(render_theme_current()->content_height)
             : SCREEN_HEIGHT - ITEM_HEIGHT - UI_S(10);
  if (bottom > SCREEN_HEIGHT - ITEM_HEIGHT - UI_S(10))
    bottom = SCREEN_HEIGHT - ITEM_HEIGHT - UI_S(10);
  int h = bottom - top;
  if (h < UI_S(20))
    return;
  int w = UI_S(3);
  if (w < 2)
    w = 2;
  int x = CONTENT_X + CONTENT_WIDTH - UI_S(7) - w;
  render_rounded_rect(framebuffer, x, top, w, h, w / 2, COLOR_LEGEND_BG);
  int thumb_h = h * visible / total;
  if (thumb_h < UI_S(12))
    thumb_h = UI_S(12);
  if (thumb_h > h)
    thumb_h = h;
  int range = h - thumb_h;
  int y = top + (total > 1 ? range * selected / (total - 1) : 0);
  render_rounded_rect(framebuffer, x, y, w, thumb_h, w / 2, COLOR_SELECT_BG);
}

void render_toast(uint16_t *framebuffer, const char *text) {
  if (!framebuffer || !text || !*text)
    return;
  int pad = UI_S(10);
  int w = font_measure_text(text, 1) + pad * 2;
  int max_w = SCREEN_WIDTH - PADDING * 2;
  if (w > max_w)
    w = max_w;
  int h = ITEM_HEIGHT - UI_S(3);
  int x = (SCREEN_WIDTH - w) / 2;
  int y = SCREEN_HEIGHT - ITEM_HEIGHT * 2 - UI_S(8);
  int pillboxes = frogui_pillboxes_get();
  if (pillboxes)
    render_rounded_rect(framebuffer, x, y, w, h, UI_S(8), COLOR_SELECT_BG);
  int baseline, cap_h;
  font_cap_metrics(&baseline, &cap_h);
  int ty = y + h / 2 - (baseline - cap_h / 2);
  int tx = (SCREEN_WIDTH - font_measure_text(text, 1)) / 2;
  if (tx < x + UI_S(4))
    tx = x + UI_S(4);
  font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, tx, ty, text,
                 pillboxes ? COLOR_SELECT_TEXT : COLOR_HEADER, 1);
}

static void render_text_pillbox_moving(uint16_t *framebuffer, int x, int text_y,
                                       int pill_y, const char *text) {
  int baseline, cap_h;
  font_cap_metrics(&baseline, &cap_h);
  int pad = frogui_prominimal_style_active() ? UI_S(5) : 7;
  int width = font_measure_text(text, 0) + 2 * pad;
  int height = cap_h + 2 * pad;
  render_rounded_rect(framebuffer, x - pad,
                      pill_y + (baseline - cap_h) - pad,
                      width, height,
                      frogui_prominimal_style_active()
                          ? UI_S(render_theme_current()->selected_radius)
                          : render_theme_current()->selected_radius,
                      COLOR_SELECT_BG);
  font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, x, text_y, text,
                 COLOR_SELECT_TEXT, 0);
}

/* Draw one row at an explicit pixel y (used by the animated list). */
void render_menu_row(uint16_t *framebuffer, const char *name, int is_dir,
                     int is_selected, int is_favorited, int y,
                     int selected_pill_y) {
  if (!framebuffer || !name)
    return;

  int text_x = render_theme_current()->scheme_name[0]
             ? CONTENT_X + UI_S(render_theme_current()->list_padding_left)
             : PADDING;
  if (render_theme_current()->scheme_name[0]) {
    int baseline = 0, cap_h = 0;
    font_cap_metrics(&baseline, &cap_h);
    int text_y = y + ITEM_HEIGHT / 2 - baseline + cap_h / 2;
    if (is_selected)
      render_rounded_rect_alpha(framebuffer, CONTENT_X, selected_pill_y, CONTENT_WIDTH,
                                ITEM_HEIGHT, render_theme_current()->selected_radius,
                                COLOR_SELECT_BG,
                                render_theme_colour_alpha(RENDER_COLOUR_SELECTION));
    if (is_favorited) {
      font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, CONTENT_X, text_y, "*",
                     COLOR_HEADER, 0);
      text_x += UI_S(16);
    }
    uint16_t icon = is_selected ? COLOR_SELECT_TEXT : is_dir ? COLOR_FOLDER : COLOR_TEXT;
    int icon_x = CONTENT_X + UI_S(render_theme_current()->list_glyph_padding_left);
    int icon_size = UI_S(25);
    int icon_y = y + (ITEM_HEIGHT - icon_size) / 2;
    if (is_dir) {
      render_fill_rect(framebuffer, icon_x, icon_y + UI_S(7), UI_S(24), UI_S(17), icon);
      render_fill_rect(framebuffer, icon_x + UI_S(2), icon_y + UI_S(1), UI_S(11), UI_S(7), icon);
    } else {
      render_rounded_rect(framebuffer, icon_x + UI_S(2), icon_y + UI_S(2),
                          UI_S(21), UI_S(21), UI_S(3), icon);
      render_fill_rect(framebuffer, icon_x + UI_S(8), icon_y + UI_S(8),
                       UI_S(9), UI_S(2), COLOR_BG);
    }
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, text_x, text_y, name,
                   is_selected ? COLOR_SELECT_TEXT : COLOR_TEXT, 0);
    return;
  }
  if (is_favorited) {
    const char *star = "*";
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, PADDING, y, star,
                   COLOR_HEADER, 0);
    text_x = PADDING + 15;
  }

  if (is_selected) {
    render_text_pillbox_moving(framebuffer, text_x, y, selected_pill_y, name);
  } else {
    uint16_t text_color = is_dir ? COLOR_FOLDER : COLOR_DISABLED;
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, text_x, y, name,
                   text_color, 0);
  }
}

void render_menu_item(uint16_t *framebuffer, int index, const char *name,
                      int is_dir, int is_selected, int scroll_offset,
                      int is_favorited, int selected_pill_y) {
  if (!framebuffer || !name)
    return;

  int visible_index = index - scroll_offset;
  if (visible_index < 0 || visible_index >= VISIBLE_ENTRIES)
    return;

  render_menu_row(framebuffer, name, is_dir, is_selected, is_favorited,
                  START_Y + UI_S(render_theme_current()->list_padding_top) +
                  visible_index * ITEM_HEIGHT, selected_pill_y);
}

void render_menu_item_centered(uint16_t *framebuffer, int index,
                               const char *name, int is_dir, int is_selected,
                               int scroll_offset, int selected_pill_y) {
  if (!framebuffer || !name)
    return;

  int visible_index = index - scroll_offset;
  if (visible_index < 0 || visible_index >= VISIBLE_ENTRIES)
    return;

  int y = START_Y + visible_index * ITEM_HEIGHT;
  int text_x = (SCREEN_WIDTH - font_measure_text(name, 0)) / 2;
  if (text_x < PADDING)
    text_x = PADDING;
  if (is_selected) {
    render_text_pillbox_moving(framebuffer, text_x, y, selected_pill_y, name);
  } else {
    font_draw_text(framebuffer, SCREEN_WIDTH, SCREEN_HEIGHT, text_x, y, name,
                   is_dir ? COLOR_FOLDER : COLOR_DISABLED, 0);
  }
}

// Thumbnail implementation

void get_thumbnail_path(const char *game_path, char *thumb_path,
                        size_t thumb_path_size) {
  if (!game_path || !thumb_path || game_path[0] == '\0') {
    thumb_path[0] = '\0';
    return;
  }

  // Find the last slash to get directory
  const char *last_slash = strrchr(game_path, '/');
  if (!last_slash) {
    thumb_path[0] = '\0';
    return;
  }

  // Copy directory path
  size_t dir_len = last_slash - game_path;
  if (dir_len + 1 >= thumb_path_size) {
    thumb_path[0] = '\0';
    return;
  }

  strncpy(thumb_path, game_path, dir_len);
  thumb_path[dir_len] = '\0';

  // Add /.res/ subdirectory
  strncat(thumb_path, "/.res/", thumb_path_size - strlen(thumb_path) - 1);

  // Get filename without extension
  const char *filename = last_slash + 1;
  const char *last_dot = strrchr(filename, '.');

  if (last_dot) {
    size_t name_len = last_dot - filename;
    strncat(thumb_path, filename,
            min(name_len, thumb_path_size - strlen(thumb_path) - 1));
  } else {
    strncat(thumb_path, filename, thumb_path_size - strlen(thumb_path) - 1);
  }

  // No extension: load_thumbnail() probes .png/.jpg/.jpeg/.bmp then .rgb565.
}

static int artwork_base_path(const char *game_path, const char *folder,
                             const char *suffix, char *out, size_t out_size) {
  const char *slash, *filename, *dot;
  size_t dir_len, stem_len;
  if (!game_path || !folder || !suffix || !out || out_size == 0)
    return 0;
  slash = strrchr(game_path, '/');
  if (!slash)
    return 0;
  dir_len = (size_t)(slash - game_path);
  filename = slash + 1;
  dot = strrchr(filename, '.');
  stem_len = dot ? (size_t)(dot - filename) : strlen(filename);
  if (stem_len == 0 ||
      snprintf(out, out_size, "%.*s/%s/%.*s%s", (int)dir_len, game_path, folder,
               (int)stem_len, filename, suffix) >= (int)out_size)
    return 0;
  return 1;
}

/* Persistent negative cache for missing artwork. Folder mtimes invalidate
 * entries when a scraper adds or replaces images. */
typedef struct {
  uint64_t game_hash;
  uint64_t folder_sig;
  uint8_t kind;
  uint8_t reserved[7];
} ArtworkMiss;
#define ARTWORK_MISS_FILE "/mnt/sdcard/frogui/.cache/artwork-misses.bin"
#define ARTWORK_MISS_MAX 4096
static ArtworkMiss artwork_misses[ARTWORK_MISS_MAX];
static size_t artwork_miss_count;
static int artwork_miss_loaded;

static uint64_t artwork_hash(const char *s) {
  uint64_t h = UINT64_C(1469598103934665603);
  if (!s)
    return h;
  for (const unsigned char *p = (const unsigned char *)s; *p; p++)
    h = (h ^ *p) * UINT64_C(1099511628211);
  return h;
}

static uint64_t artwork_folder_signature(const char *game_path) {
  static const char *folders[] = {".res", "Imgs", "images", "Images", NULL};
  const char *slash = game_path ? strrchr(game_path, '/') : NULL;
  if (!slash)
    return 0;
  size_t dir_len = (size_t)(slash - game_path);
  uint64_t sig = UINT64_C(1469598103934665603);
  char path[1024];
  for (int i = 0; folders[i]; i++) {
    struct stat st;
    int n = snprintf(path, sizeof path, "%.*s/%s", (int)dir_len, game_path,
                     folders[i]);
    if (n > 0 && (size_t)n < sizeof path && stat(path, &st) == 0) {
      sig ^= (uint64_t)st.st_mtime +
             UINT64_C(0x9e3779b97f4a7c15) * (uint64_t)(i + 1);
      sig = (sig << 7) | (sig >> 57);
      sig ^= (uint64_t)st.st_size;
    } else {
      sig ^= UINT64_C(0x517cc1b727220a95) + (uint64_t)i;
      sig = (sig << 7) | (sig >> 57);
    }
  }
  return sig;
}

static void artwork_miss_load(void) {
  if (artwork_miss_loaded)
    return;
  artwork_miss_loaded = 1;
  FILE *f = fopen(ARTWORK_MISS_FILE, "rb");
  if (!f)
    return;
  artwork_miss_count =
      fread(artwork_misses, sizeof(ArtworkMiss), ARTWORK_MISS_MAX, f);
  fclose(f);
}

static int artwork_miss_contains(uint64_t game_hash, uint64_t folder_sig,
                                 ArtworkKind kind) {
  artwork_miss_load();
  for (size_t i = 0; i < artwork_miss_count; i++)
    if (artwork_misses[i].game_hash == game_hash &&
        artwork_misses[i].folder_sig == folder_sig &&
        artwork_misses[i].kind == (uint8_t)kind)
      return 1;
  return 0;
}

static void artwork_miss_add(uint64_t game_hash, uint64_t folder_sig,
                             ArtworkKind kind) {
  artwork_miss_load();
  if (artwork_miss_contains(game_hash, folder_sig, kind))
    return;
  ArtworkMiss miss = {game_hash, folder_sig, (uint8_t)kind, {0}};
  if (artwork_miss_count < ARTWORK_MISS_MAX)
    artwork_misses[artwork_miss_count++] = miss;
  else {
    memmove(artwork_misses, artwork_misses + 1,
            (ARTWORK_MISS_MAX - 1) * sizeof(ArtworkMiss));
    artwork_misses[ARTWORK_MISS_MAX - 1] = miss;
  }
  mkdir("/mnt/sdcard/frogui", 0755);
  mkdir("/mnt/sdcard/frogui/.cache", 0755);
  FILE *f = fopen(ARTWORK_MISS_FILE, "ab");
  if (f) {
    fwrite(&miss, sizeof miss, 1, f);
    fclose(f);
  }
}

int load_game_artwork(const char *game_path, ArtworkKind kind,
                      Thumbnail *thumb) {
  static const char *folders[] = {".res", "Imgs", "images", "Images", NULL};
  static const char *box_suffixes[] = {"", NULL};
  static const char *title_suffixes[] = {"-title",       "_title",
                                         ".title",       "-titlescreen",
                                         "_titlescreen", ".titlescreen",
                                         "-screenshot",  "_screenshot",
                                         ".screenshot",  "-screen",
                                         "_screen",      "-preview",
                                         "_preview",     NULL};
  const char **suffixes =
      kind == ARTWORK_TITLE_SCREEN ? title_suffixes : box_suffixes;
  char base[1024];
  if (!game_path || !thumb)
    return 0;
  /* Port launchers live at ports/ root; data is in a hidden sibling folder. */
  const char *slash = strrchr(game_path, '/');
  const char *dot = strrchr(game_path, '.');
  if (kind == ARTWORK_BOXART && slash && dot && strcasecmp(dot, ".sh") == 0) {
    int n = snprintf(base, sizeof base, "%.*s.png", (int)(dot - game_path),
                     game_path);
    if (n > 0 && (size_t)n < sizeof base && load_thumbnail(base, thumb))
      return 1;
  }
  uint64_t game_hash = artwork_hash(game_path);
  uint64_t folder_sig = artwork_folder_signature(game_path);
  if (artwork_miss_contains(game_hash, folder_sig, kind)) {
    free_thumbnail(thumb);
    return 0;
  }
  for (int f = 0; folders[f]; f++) {
    for (int s = 0; suffixes[s]; s++) {
      if (!artwork_base_path(game_path, folders[f], suffixes[s], base,
                             sizeof base))
        continue;
      if (load_thumbnail(base, thumb))
        return 1;
    }
  }
  artwork_miss_add(game_hash, folder_sig, kind);
  return 0;
}

static uint16_t rgb24_to_rgb565(uint8_t r, uint8_t g, uint8_t b) {
  return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

// Static buffers for thumbnail - no malloc/free hell
static uint16_t thumbnail_buffer[250 * 250]; // Max size: 250x250
static uint8_t
    thumbnail_alpha[250 * 250]; // per-pixel alpha, composited at render
#define THUMB_BUF_W 250
#define THUMB_BUF_H 250

// Decode a JPG/PNG/BMP via stb_image and nearest-neighbor downscale into the
// static RGB565 buffer (aspect-preserving, capped at 250x250). Alpha is kept
// in a side buffer and composited per-pixel at render time, so transparent
// boxart shows whatever is actually behind it (theme/background image).
static int load_image_thumbnail(const char *path, Thumbnail *thumb) {
  int w, h, ch;
  unsigned char *img = stbi_load(path, &w, &h, &ch, 4); // force RGBA
  if (!img)
    return 0;
  if (w <= 0 || h <= 0) {
    stbi_image_free(img);
    return 0;
  }

  int dw = w, dh = h;
  if (dw > THUMB_BUF_W) {
    dh = dh * THUMB_BUF_W / dw;
    dw = THUMB_BUF_W;
  }
  if (dh > THUMB_BUF_H) {
    dw = dw * THUMB_BUF_H / dh;
    dh = THUMB_BUF_H;
  }
  if (dw < 1)
    dw = 1;
  if (dh < 1)
    dh = 1;

  for (int y = 0; y < dh; y++) {
    int sy = y * h / dh;
    for (int x = 0; x < dw; x++) {
      int sx = x * w / dw;
      const unsigned char *p = img + (sy * w + sx) * 4;
      thumbnail_buffer[y * dw + x] = rgb24_to_rgb565(p[0], p[1], p[2]);
      thumbnail_alpha[y * dw + x] = p[3];
    }
  }
  stbi_image_free(img);

  thumb->data = thumbnail_buffer;
  thumb->alpha = thumbnail_alpha;
  thumb->width = dw;
  thumb->height = dh;
  return 1;
}

int load_thumbnail(const char *base_path, Thumbnail *thumb) {
  if (!base_path || !thumb)
    return 0;
  thumb->data = NULL;
  thumb->alpha = NULL;
  thumb->width = 0;
  thumb->height = 0;

  char p[1024];
  // Decodable image formats first (easy to drop in), then raw fallback.
  static const char *exts[] = {".png", ".jpg", ".jpeg", ".bmp", NULL};
  for (int i = 0; exts[i]; i++) {
    snprintf(p, sizeof p, "%s%s", base_path, exts[i]);
    if (access(p, F_OK) == 0 && load_image_thumbnail(p, thumb))
      return 1;
  }
  snprintf(p, sizeof p, "%s.rgb565", base_path);
  return load_raw_rgb565(p, thumb);
}

int load_raw_rgb565(const char *path, Thumbnail *thumb) {
  // Check if file exists
  if (access(path, F_OK) != 0) {
    return 0;
  }

  FILE *fp = fopen(path, "rb");
  if (!fp) {
    return 0;
  }

  fseek(fp, 0, SEEK_END);
  long file_size = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  // Try common dimensions - including 160x160 for the resized images
  int dimensions[][2] = {{64, 64},   {128, 128}, {160, 160},
                         {200, 200}, {250, 200}, {200, 250}};
  int num_dims = sizeof(dimensions) / sizeof(dimensions[0]);

  for (int i = 0; i < num_dims; i++) {
    int w = dimensions[i][0];
    int h = dimensions[i][1];
    if (w * h * 2 == file_size) {

      // Check if it fits in our static buffer
      if (w * h > sizeof(thumbnail_buffer) / 2) {
        fclose(fp);
        return 0;
      }

      thumb->width = w;
      thumb->height = h;
      thumb->data = thumbnail_buffer; // Use static buffer
      thumb->alpha = NULL;            // raw rgb565 has no alpha: opaque

      size_t read_bytes = fread(thumb->data, 1, file_size, fp);
      fclose(fp);

      if (read_bytes == file_size) {
        return 1;
      } else {
        return 0;
      }
    }
  }

  fclose(fp);
  return 0;
}

void free_thumbnail(Thumbnail *thumb) {
  if (thumb) {
    // No need to free static buffer, just reset pointer
    thumb->data = NULL;
    thumb->alpha = NULL;
    thumb->width = 0;
    thumb->height = 0;
  }
}

void render_thumbnail(uint16_t *framebuffer, const Thumbnail *thumb) {
  if (!framebuffer || !thumb || !thumb->data) {
    return;
  }

  // Calculate scaled dimensions to fit in thumbnail area
  int display_width = thumb->width;
  int display_height = thumb->height;

  // Scale down if too large
  if (display_width > THUMBNAIL_MAX_WIDTH) {
    display_height = (display_height * THUMBNAIL_MAX_WIDTH) / display_width;
    display_width = THUMBNAIL_MAX_WIDTH;
  }

  if (display_height > THUMBNAIL_MAX_HEIGHT) {
    display_width = (display_width * THUMBNAIL_MAX_HEIGHT) / display_height;
    display_height = THUMBNAIL_MAX_HEIGHT;
  }

  // Center in thumbnail area (vertically) and align to right edge
  int start_x = SCREEN_WIDTH - display_width; // Align to right edge of screen

  // Center thumbnail vertically on screen
  int start_y = (SCREEN_HEIGHT - display_height) / 2;

  // No backing card/frame: the image composites straight onto whatever is
  // behind it (theme fill or background image) using its own alpha.
  for (int y = 0; y < display_height; y++) {
    for (int x = 0; x < display_width; x++) {
      int screen_x = start_x + x;
      int screen_y = start_y + y;

      if (screen_x >= 0 && screen_x < SCREEN_WIDTH && screen_y >= 0 &&
          screen_y < SCREEN_HEIGHT) {

        // Simple scaling - map display coords to source coords
        int src_x = (x * thumb->width) / display_width;
        int src_y = (y * thumb->height) / display_height;

        if (src_x < thumb->width && src_y < thumb->height) {
          int si = src_y * thumb->width + src_x;
          unsigned a = thumb->alpha ? thumb->alpha[si] : 255;
          if (a == 0)
            continue;
          uint16_t pixel = thumb->data[si];
          uint16_t *dst = &framebuffer[screen_y * SCREEN_WIDTH + screen_x];
          if (a == 255) {
            *dst = pixel;
          } else {
            // blend RGB565 src over dst by alpha
            unsigned sr = (pixel >> 11) & 0x1F, sg = (pixel >> 5) & 0x3F,
                     sb = pixel & 0x1F;
            unsigned dr = (*dst >> 11) & 0x1F, dg = (*dst >> 5) & 0x3F,
                     db = *dst & 0x1F;
            unsigned r = (sr * a + dr * (255 - a)) / 255;
            unsigned g = (sg * a + dg * (255 - a)) / 255;
            unsigned b = (sb * a + db * (255 - a)) / 255;
            *dst = (uint16_t)((r << 11) | (g << 5) | b);
          }
        }
      }
    }
  }
}

void render_thumbnail_contained(uint16_t *framebuffer, const Thumbnail *thumb,
                                int x, int y, int width, int height) {
  if (!framebuffer || !thumb || !thumb->data || thumb->width <= 0 ||
      thumb->height <= 0 || width <= 0 || height <= 0) return;
  int dw = width, dh = height;
  if ((int64_t)width * thumb->height > (int64_t)height * thumb->width) {
    dw = height * thumb->width / thumb->height;
  } else {
    dh = width * thumb->height / thumb->width;
  }
  if (dw < 1) dw = 1;
  if (dh < 1) dh = 1;
  int left = x + (width - dw) / 2;
  int top = y + (height - dh) / 2;
  for (int py = 0; py < dh; py++) {
    int sy = py * thumb->height / dh;
    for (int px = 0; px < dw; px++) {
      int dx = left + px, dy = top + py;
      if (dx < 0 || dx >= SCREEN_WIDTH || dy < 0 || dy >= SCREEN_HEIGHT) continue;
      int sx = px * thumb->width / dw;
      int si = sy * thumb->width + sx;
      unsigned a = thumb->alpha ? thumb->alpha[si] : 255;
      if (!a) continue;
      uint16_t *dst = &framebuffer[dy * SCREEN_WIDTH + dx];
      if (a == 255) {
        *dst = thumb->data[si];
      } else {
        uint16_t src = thumb->data[si], bg = *dst;
        unsigned r = (((src >> 11) & 31u) * a + ((bg >> 11) & 31u) * (255 - a)) / 255;
        unsigned g = (((src >> 5) & 63u) * a + ((bg >> 5) & 63u) * (255 - a)) / 255;
        unsigned b = ((src & 31u) * a + (bg & 31u) * (255 - a)) / 255;
        *dst = (uint16_t)((r << 11) | (g << 5) | b);
      }
    }
  }
}

typedef struct {
  char path[512];
  int screen_w, screen_h, width, height, left, top;
  uint16_t *pixels;
  uint8_t *alpha;
} ThemeCanvasCache;

static ThemeCanvasCache theme_canvas_cache[6];
static unsigned theme_canvas_next;

static void theme_canvas_blit(uint16_t *framebuffer, const ThemeCanvasCache *cache) {
  for (int y = 0; y < cache->height; y++) {
    uint16_t *dst = framebuffer + (cache->top + y) * SCREEN_WIDTH + cache->left;
    const uint16_t *src = cache->pixels + y * cache->width;
    if (!cache->alpha) {
      memcpy(dst, src, (size_t)cache->width * sizeof(*src));
      continue;
    }
    const uint8_t *alpha = cache->alpha + y * cache->width;
    for (int x = 0; x < cache->width; x++) {
      unsigned a = alpha[x];
      if (a == 255) dst[x] = src[x];
      else if (a) {
        uint16_t bg = dst[x], fg = src[x];
        unsigned r = (((fg >> 11) & 31u) * a + ((bg >> 11) & 31u) * (255 - a)) / 255;
        unsigned g = (((fg >> 5) & 63u) * a + ((bg >> 5) & 63u) * (255 - a)) / 255;
        unsigned b = ((fg & 31u) * a + (bg & 31u) * (255 - a)) / 255;
        dst[x] = (uint16_t)((r << 11) | (g << 5) | b);
      }
    }
  }
}

int render_theme_canvas(uint16_t *framebuffer, const char *path) {
  if (!framebuffer || !path) return 0;
  for (size_t i = 0; i < sizeof theme_canvas_cache / sizeof theme_canvas_cache[0]; i++) {
    ThemeCanvasCache *cached = &theme_canvas_cache[i];
    if (cached->pixels && cached->screen_w == SCREEN_WIDTH &&
        cached->screen_h == SCREEN_HEIGHT && strcmp(cached->path, path) == 0) {
      theme_canvas_blit(framebuffer, cached);
      return 1;
    }
  }

  /* shortcut: accepts MuOS 640x480 canvases; add per-resolution lookup for other package sizes. */
  int sw = 0, sh = 0, channels = 0;
  if (!stbi_info(path, &sw, &sh, &channels) || sw != 640 || sh != 480) return 0;
  unsigned char *rgba = stbi_load(path, &sw, &sh, &channels, 4);
  if (!rgba) return 0;
  int dw = SCREEN_WIDTH, dh = dw * sh / sw;
  if (dh > SCREEN_HEIGHT) { dh = SCREEN_HEIGHT; dw = dh * sw / sh; }
  int left = (SCREEN_WIDTH - dw) / 2, top = (SCREEN_HEIGHT - dh) / 2;
  size_t count = (size_t)dw * dh;
  ThemeCanvasCache *cached = &theme_canvas_cache[theme_canvas_next++ %
                                      (sizeof theme_canvas_cache / sizeof theme_canvas_cache[0])];
  free(cached->pixels);
  free(cached->alpha);
  memset(cached, 0, sizeof(*cached));
  cached->pixels = malloc(count * sizeof(*cached->pixels));
  cached->alpha = malloc(count);
  if (!cached->pixels || !cached->alpha) {
    free(cached->pixels); free(cached->alpha); memset(cached, 0, sizeof(*cached));
    stbi_image_free(rgba);
    return 0;
  }
  int opaque = 1;
  for (int y = 0; y < dh; y++) {
    int sy = y * sh / dh;
    for (int x = 0; x < dw; x++) {
      const unsigned char *src = rgba + ((size_t)sy * sw + x * sw / dw) * 4;
      size_t p = (size_t)y * dw + x;
      cached->pixels[p] = rgb24_to_rgb565(src[0], src[1], src[2]);
      cached->alpha[p] = src[3];
      if (src[3] != 255) opaque = 0;
    }
  }
  stbi_image_free(rgba);
  if (opaque) { free(cached->alpha); cached->alpha = NULL; }
  snprintf(cached->path, sizeof cached->path, "%s", path);
  cached->screen_w = SCREEN_WIDTH; cached->screen_h = SCREEN_HEIGHT;
  cached->width = dw; cached->height = dh; cached->left = left; cached->top = top;
  theme_canvas_blit(framebuffer, cached);
  return 1;
}
