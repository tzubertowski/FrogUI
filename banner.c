#include "banner.h"
#include "render.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/stat.h>
#include <pthread.h>
#include <unistd.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

static uint16_t *banner_buf = NULL;   /* current (target) banner */
static uint16_t *banner_prev = NULL;  /* outgoing banner during crossfade */
static int banner_loaded = 0;
static char banner_active_key[600] = "";
static int banner_anim = 1;           /* crossfade enabled */
static int banner_dim = 0;            /* 0 = unchanged, 100 = black */
#define BANNER_WORK_ROWS 24            /* keep SD/UI work below one frame */
static unsigned char *banner_pending_img;
static uint16_t *banner_pending_buf;
static int banner_pending_w, banner_pending_h;
static int banner_pending_mode;
static uint16_t banner_pending_bg;
static int banner_pending_light;
static int banner_pending_row;
static char banner_pending_key[600] = "";

#define BANNER_PREFETCH_N 256
typedef struct { char path[512]; int mode; uint16_t bg; int dim; unsigned generation; } BannerPrefetch;
static BannerPrefetch prefetch_queue[BANNER_PREFETCH_N];
static unsigned prefetch_head, prefetch_count, prefetch_generation;
static pthread_mutex_t prefetch_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t prefetch_cond = PTHREAD_COND_INITIALIZER;
static pthread_t prefetch_thread;
static int prefetch_started;
static inline uint16_t rgb_to_565(unsigned char r, unsigned char g, unsigned char b);

static unsigned banner_key_hash(const char *key) {
    unsigned h = 2166136261u;
    for (const unsigned char *p = (const unsigned char *)key; *p; p++) {
        h ^= *p;
        h *= 16777619u;
    }
    return h;
}

static void banner_make_key(const char *path, int mode, uint16_t bg, int dim,
                            int sw, int sh, char *key, size_t size) {
    struct stat st;
    long long mtime = 0, source_size = 0;
    if (stat(path, &st) == 0) {
        mtime = (long long)st.st_mtime;
        source_size = (long long)st.st_size;
    }
    snprintf(key, size, "%s|%d|%u|dim=%d|panel=%dx%d|src=%lld:%lld",
             path, mode, (unsigned)bg, dim, sw, sh, mtime, source_size);
}

static int banner_disk_cache_load(const char *key, uint16_t *dst, size_t bytes) {
    char path[160];
    snprintf(path, sizeof path, "/mnt/sdcard/frogui/.cache/banner-%08x.rgb565",
             banner_key_hash(key));
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    size_t got = fread(dst, 1, bytes, f);
    fclose(f);
    return got == bytes;
}

static void banner_disk_cache_store(const char *key, const uint16_t *src, size_t bytes) {
    char path[160], tmp[176];
    snprintf(path, sizeof path, "/mnt/sdcard/frogui/.cache/banner-%08x.rgb565",
             banner_key_hash(key));
    mkdir("/mnt/sdcard/frogui/.cache", 0755);
    snprintf(tmp, sizeof tmp, "%s.%lu.tmp", path, (unsigned long)pthread_self());
    FILE *f = fopen(tmp, "wb");
    if (!f) return;
    if (fwrite(src, 1, bytes, f) == bytes) {
        fclose(f);
        rename(tmp, path);
    } else {
        fclose(f);
        remove(tmp);
    }
}

static int prefetch_is_current(unsigned generation) {
    int current;
    pthread_mutex_lock(&prefetch_mutex);
    current = generation == prefetch_generation;
    pthread_mutex_unlock(&prefetch_mutex);
    return current;
}

static void banner_prefetch_one(const BannerPrefetch *job) {
    int sw = SCREEN_WIDTH, sh = SCREEN_HEIGHT;
    char key[600], cache_path[160];
    banner_make_key(job->path, job->mode, job->bg, job->dim, sw, sh, key, sizeof key);
    snprintf(cache_path, sizeof cache_path,
             "/mnt/sdcard/frogui/.cache/banner-%08x.rgb565", banner_key_hash(key));
    struct stat st;
    if (stat(cache_path, &st) == 0 && st.st_size == (off_t)sw * sh * 2) return;

    int w, h, ch;
    unsigned char *img = stbi_load(job->path, &w, &h, &ch, 3);
    if (!img || w <= 0 || h <= 0) { if (img) stbi_image_free(img); return; }
    uint16_t *out = malloc((size_t)sw * sh * sizeof(*out));
    if (!out) { stbi_image_free(img); return; }

    int dx0 = 0, dy0 = 0, dw = sw, dh = sh;
    if (job->mode == BANNER_FIT_FILL) {
        if ((long)w * sh > (long)h * sw) { dh = sh; dw = (int)((long)w * sh / h); }
        else                             { dw = sw; dh = (int)((long)h * sw / w); }
        dx0 = (sw - dw) / 2; dy0 = (sh - dh) / 2;
    } else if (job->mode == BANNER_FIT_FIT) {
        if ((long)w * sh > (long)h * sw) { dw = sw; dh = (int)((long)h * sw / w); }
        else                             { dh = sh; dw = (int)((long)w * sh / h); }
        dx0 = (sw - dw) / 2; dy0 = (sh - dh) / 2;
    } else if (job->mode == BANNER_FIT_CENTER) {
        dw = w; dh = h; dx0 = (sw - w) / 2; dy0 = (sh - h) / 2;
    }
    int light = 100 - job->dim;
    for (int y = 0; y < sh; y++) {
        if ((y % BANNER_WORK_ROWS) == 0 && !prefetch_is_current(job->generation)) goto done;
        for (int x = 0; x < sw; x++) {
            int sx, sy;
            if (job->mode == BANNER_FIT_TILE) { sx = x % w; sy = y % h; }
            else if (x < dx0 || x >= dx0 + dw || y < dy0 || y >= dy0 + dh) {
                out[y * sw + x] = job->bg;
                continue;
            } else {
                sx = (int)((long)(x - dx0) * w / dw);
                sy = (int)((long)(y - dy0) * h / dh);
                if (sx < 0) sx = 0;
                if (sx >= w) sx = w - 1;
                if (sy < 0) sy = 0;
                if (sy >= h) sy = h - 1;
            }
            unsigned char *p = img + (sy * w + sx) * 3;
            out[y * sw + x] = rgb_to_565(
                (unsigned char)(p[0] * light / 100),
                (unsigned char)(p[1] * light / 100),
                (unsigned char)(p[2] * light / 100));
        }
        if ((y % BANNER_WORK_ROWS) == BANNER_WORK_ROWS - 1) usleep(5000);
    }
    if (prefetch_is_current(job->generation))
        banner_disk_cache_store(key, out, (size_t)sw * sh * 2);
done:
    free(out);
    stbi_image_free(img);
}

static void *banner_prefetch_worker(void *unused) {
    (void)unused;
    for (;;) {
        BannerPrefetch job;
        pthread_mutex_lock(&prefetch_mutex);
        while (!prefetch_count) pthread_cond_wait(&prefetch_cond, &prefetch_mutex);
        job = prefetch_queue[prefetch_head];
        prefetch_head = (prefetch_head + 1) % BANNER_PREFETCH_N;
        prefetch_count--;
        pthread_mutex_unlock(&prefetch_mutex);
        if (prefetch_is_current(job.generation)) banner_prefetch_one(&job);
    }
    return NULL;
}

void banner_prefetch_clear(void) {
    pthread_mutex_lock(&prefetch_mutex);
    prefetch_generation++;
    prefetch_head = prefetch_count = 0;
    pthread_mutex_unlock(&prefetch_mutex);
}

void banner_prefetch(const char *path, int mode, uint16_t bg) {
    if (!path || !*path) return;
    pthread_mutex_lock(&prefetch_mutex);
    if (!prefetch_started) {
        if (pthread_create(&prefetch_thread, NULL, banner_prefetch_worker, NULL) != 0) {
            pthread_mutex_unlock(&prefetch_mutex);
            return;
        }
        pthread_detach(prefetch_thread);
        prefetch_started = 1;
    }
    for (unsigned i = 0; i < prefetch_count; i++) {
        BannerPrefetch *queued = &prefetch_queue[(prefetch_head + i) % BANNER_PREFETCH_N];
        if (queued->generation == prefetch_generation && queued->mode == mode &&
            queued->bg == bg && queued->dim == banner_dim && strcmp(queued->path, path) == 0) {
            pthread_mutex_unlock(&prefetch_mutex);
            return;
        }
    }
    /* shortcut: queue caps at 256 images; raise it if a theme pack exceeds that. */
    if (prefetch_count < BANNER_PREFETCH_N) {
        unsigned tail = (prefetch_head + prefetch_count) % BANNER_PREFETCH_N;
        snprintf(prefetch_queue[tail].path, sizeof prefetch_queue[tail].path, "%s", path);
        prefetch_queue[tail].mode = mode;
        prefetch_queue[tail].bg = bg;
        prefetch_queue[tail].dim = banner_dim;
        prefetch_queue[tail].generation = prefetch_generation;
        prefetch_count++;
        pthread_cond_signal(&prefetch_cond);
    }
    pthread_mutex_unlock(&prefetch_mutex);
}
#define FADE_FRAMES 20                /* ~330ms @ 60fps — slow, iPhone-like */
static int fade_frame = FADE_FRAMES;  /* 0..FADE_FRAMES; FADE_FRAMES = done */

static inline uint16_t rgb_to_565(unsigned char r, unsigned char g, unsigned char b) {
    return ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
}

/* blend a->b by t (0..256): result = a*(256-t) + b*t */
static inline uint16_t blend565(uint16_t a, uint16_t b, int t) {
    int it = 256 - t;
    int ar = (a >> 11) & 0x1f, ag = (a >> 5) & 0x3f, ab = a & 0x1f;
    int br = (b >> 11) & 0x1f, bg = (b >> 5) & 0x3f, bb = b & 0x1f;
    int r = (ar * it + br * t) >> 8;
    int g = (ag * it + bg * t) >> 8;
    int bl = (ab * it + bb * t) >> 8;
    return (uint16_t)((r << 11) | (g << 5) | bl);
}

static int fade_amount(void) {
    float p = (float)fade_frame / (float)FADE_FRAMES;
    if (p > 1.0f) p = 1.0f;
    float e = p * p * (3.0f - 2.0f * p);
    int t = (int)(e * 256.0f + 0.5f);
    return t > 256 ? 256 : t;
}

void banner_set_anim(int enabled) {
    banner_anim = enabled ? 1 : 0;
    if (!banner_anim) fade_frame = FADE_FRAMES;
}

void banner_set_dim(int percent) {
    if (percent < 0) percent = 0;
    if (percent > 100) percent = 100;
    banner_dim = percent;
}

/* LRU cache of fully-decoded + transformed panel-res backgrounds, keyed by
 * "path|mode". Root scrolling flips between folder backgrounds constantly; without
 * this each step re-decoded a PNG. A cache hit is just a memcpy. */
#define BCACHE_N 6
static struct { char key[600]; uint16_t *buf; int valid; } bcache[BCACHE_N];
static int bcache_next;
static int bcache_find(const char *key) {
    for (int i = 0; i < BCACHE_N; i++)
        if (bcache[i].valid && strcmp(bcache[i].key, key) == 0) return i;
    return -1;
}
static void bcache_store(const char *key, const uint16_t *src, int npix) {
    int i = bcache_next; bcache_next = (bcache_next + 1) % BCACHE_N;
    if (!bcache[i].buf) bcache[i].buf = malloc((size_t)npix * 2);
    if (!bcache[i].buf) { bcache[i].valid = 0; return; }
    memcpy(bcache[i].buf, src, (size_t)npix * 2);
    strncpy(bcache[i].key, key, sizeof bcache[i].key - 1);
    bcache[i].key[sizeof bcache[i].key - 1] = 0;
    bcache[i].valid = 1;
}

static void banner_snapshot_for_fade(int sw, int sh) {
    if (banner_anim && banner_loaded && banner_buf) {
        if (!banner_prev) banner_prev = malloc(sw * sh * sizeof(uint16_t));
        if (banner_prev) {
            int npix = sw * sh;
            /* A fast second input can arrive before the previous crossfade
             * finishes. Snapshot the image actually on screen, not its target,
             * so repeated carousel steps stay continuous instead of flashing. */
            if (fade_frame < FADE_FRAMES) {
                int t = fade_amount();
                for (int i = 0; i < npix; i++)
                    banner_prev[i] = blend565(banner_prev[i], banner_buf[i], t);
            } else {
                memcpy(banner_prev, banner_buf, (size_t)npix * sizeof(uint16_t));
            }
            fade_frame = 0;
        } else {
            fade_frame = FADE_FRAMES;
        }
    } else {
        fade_frame = FADE_FRAMES;
    }
}

static void banner_load_common(const char *path, int mode, uint16_t bg) {
    int sw = SCREEN_WIDTH, sh = SCREEN_HEIGHT, npix = sw * sh;
    char key[600];
    /* Include the source identity and target geometry.  The old cache key only
     * used the pathname, so replacing theme assets or switching between the
     * 854x480 and 640x480 panels kept displaying stale/blank renders. */
    banner_make_key(path, mode, bg, banner_dim, sw, sh, key, sizeof key);

    /* Several systems can fall back to the same main image. Do not start a
     * pointless full-screen fade, or even memcpy, when the resolved art did
     * not actually change. */
    if (banner_loaded && strcmp(key, banner_active_key) == 0)
        return;

    if (banner_pending_img && strcmp(key, banner_pending_key) == 0)
        return;

    if (banner_pending_img) {
        stbi_image_free(banner_pending_img);
        banner_pending_img = NULL;
    }
    free(banner_pending_buf);
    banner_pending_buf = NULL;

    if (!banner_buf) banner_buf = malloc(npix * sizeof(uint16_t));
    if (!banner_buf) { banner_loaded = 0; return; }

    /* Cache hit: no decode, just fade-snapshot + copy the cached result. */
    int ci = bcache_find(key);
    if (ci >= 0) {
        banner_snapshot_for_fade(sw, sh);
        memcpy(banner_buf, bcache[ci].buf, (size_t)npix * 2);
        banner_loaded = 1;
        snprintf(banner_active_key, sizeof banner_active_key, "%s", key);
        return;
    }

    if (banner_disk_cache_load(key, banner_buf, (size_t)npix * 2)) {
        banner_snapshot_for_fade(sw, sh);
        banner_loaded = 1;
        snprintf(banner_active_key, sizeof banner_active_key, "%s", key);
        bcache_store(key, banner_buf, npix);
        return;
    }

    int w, h, ch;
    unsigned char *img = stbi_load(path, &w, &h, &ch, 3);
    if (!img || w <= 0 || h <= 0) {
        if (img) stbi_image_free(img);
        banner_loaded = 0;
        return;
    }

    /* Decode once, then transform a small number of rows per frame. The old
     * implementation did the full-screen rescale here, blocking input while
     * scrolling through systems on the SF3000. */
    banner_pending_img = img;
    if (!banner_pending_buf)
        banner_pending_buf = malloc((size_t)npix * sizeof(uint16_t));
    if (!banner_pending_buf) {
        stbi_image_free(banner_pending_img);
        banner_pending_img = NULL;
        return;
    }
    banner_pending_w = w;
    banner_pending_h = h;
    banner_pending_mode = mode;
    banner_pending_bg = bg;
    banner_pending_light = 100 - banner_dim;
    banner_pending_row = 0;
    snprintf(banner_pending_key, sizeof banner_pending_key, "%s", key);
    fade_frame = FADE_FRAMES;
}

void banner_load(const char *path) {
    /* System artwork comes from mixed-aspect-ratio theme resources. Preserve
     * the composition and crop the excess instead of stretching consoles and
     * artwork into the panel's aspect ratio. */
    banner_load_common(path, BANNER_FIT_FILL, COLOR_BG);
}
void banner_load_fit(const char *path, int mode, uint16_t bg) {
    banner_load_common(path, mode, bg);
}

void banner_clear(void) {
    if (banner_pending_img) {
        stbi_image_free(banner_pending_img);
        banner_pending_img = NULL;
    }
    free(banner_pending_buf);
    banner_pending_buf = NULL;
    banner_loaded = 0;
    banner_active_key[0] = '\0';
    fade_frame = FADE_FRAMES;
}

void banner_render(uint16_t *framebuffer) {
    if (banner_pending_img) {
        int sw = SCREEN_WIDTH, sh = SCREEN_HEIGHT;
        int w = banner_pending_w, h = banner_pending_h;
        int mode = banner_pending_mode;
        int dx0 = 0, dy0 = 0, dw = sw, dh = sh;
        if (mode == BANNER_FIT_FILL) {
            if ((long)w * sh > (long)h * sw) { dh = sh; dw = (int)((long)w * sh / h); }
            else                             { dw = sw; dh = (int)((long)h * sw / w); }
            dx0 = (sw - dw) / 2; dy0 = (sh - dh) / 2;
        } else if (mode == BANNER_FIT_FIT) {
            if ((long)w * sh > (long)h * sw) { dw = sw; dh = (int)((long)h * sw / w); }
            else                             { dh = sh; dw = (int)((long)w * sh / h); }
            dx0 = (sw - dw) / 2; dy0 = (sh - dh) / 2;
        } else if (mode == BANNER_FIT_CENTER) {
            dw = w; dh = h; dx0 = (sw - w) / 2; dy0 = (sh - h) / 2;
        }
        int end = banner_pending_row + BANNER_WORK_ROWS;
        if (end > sh) end = sh;
        for (int y = banner_pending_row; y < end; y++) {
            for (int x = 0; x < sw; x++) {
                int sx, sy;
                if (mode == BANNER_FIT_TILE) {
                    sx = x % w; sy = y % h;
                } else if (x < dx0 || x >= dx0 + dw || y < dy0 || y >= dy0 + dh) {
                    banner_pending_buf[y * sw + x] = banner_pending_bg;
                    continue;
                } else {
                    sx = (int)((long)(x - dx0) * w / dw);
                    sy = (int)((long)(y - dy0) * h / dh);
                    if (sx < 0) sx = 0; if (sx >= w) sx = w - 1;
                    if (sy < 0) sy = 0; if (sy >= h) sy = h - 1;
                }
                unsigned char *p = banner_pending_img + (sy * w + sx) * 3;
                banner_pending_buf[y * sw + x] = rgb_to_565(
                    (unsigned char)(p[0] * banner_pending_light / 100),
                    (unsigned char)(p[1] * banner_pending_light / 100),
                    (unsigned char)(p[2] * banner_pending_light / 100));
            }
        }
        banner_pending_row = end;
        if (banner_pending_row >= sh) {
            stbi_image_free(banner_pending_img);
            banner_pending_img = NULL;
            free(banner_buf);
            banner_buf = banner_pending_buf;
            banner_pending_buf = NULL;
            banner_loaded = 1;
            snprintf(banner_active_key, sizeof banner_active_key, "%s", banner_pending_key);
            bcache_store(banner_active_key, banner_buf, sw * sh);
            banner_disk_cache_store(banner_active_key, banner_buf, (size_t)sw * sh * 2);
        }
    }
    if (!banner_loaded || !banner_buf || !framebuffer) return;
    int n = SCREEN_WIDTH * SCREEN_HEIGHT;

    if (fade_frame >= FADE_FRAMES || !banner_prev) {
        memcpy(framebuffer, banner_buf, n * sizeof(uint16_t));
        return;
    }

    /* Advance one frame, ease-in-out (smoothstep) for a slow, soft iPhone-like
     * crossfade. p: 0..1 progress; e = 3p²−2p³. Only the background animates —
     * the selection/text is drawn separately and updates immediately. */
    fade_frame++;
    int t = fade_amount();

    for (int i = 0; i < n; i++)
        framebuffer[i] = blend565(banner_prev[i], banner_buf[i], t);
}

void banner_draw_card(uint16_t *framebuffer, int x, int y, int w, int h) {
    if (!banner_loaded || !banner_buf || !framebuffer || w <= 2 || h <= 2) return;
    if (x < 0 || y < 0 || x + w > SCREEN_WIDTH || y + h > SCREEN_HEIGHT) return;
    for (int yy = 0; yy < h; yy++) {
        int sy = (int)((long)yy * SCREEN_HEIGHT / h);
        for (int xx = 0; xx < w; xx++) {
            int sx = (int)((long)xx * SCREEN_WIDTH / w);
            framebuffer[(y + yy) * SCREEN_WIDTH + (x + xx)] =
                banner_buf[sy * SCREEN_WIDTH + sx];
        }
    }
    /* Thin rounded-card outline. The fill remains the real artwork; the
     * outline is deliberately subtle so the system name stays primary. */
    uint16_t edge = COLOR_HEADER;
    for (int xx = x + 3; xx < x + w - 3; xx++) {
        framebuffer[y * SCREEN_WIDTH + xx] = edge;
        framebuffer[(y + h - 1) * SCREEN_WIDTH + xx] = edge;
    }
    for (int yy = y + 3; yy < y + h - 3; yy++) {
        framebuffer[yy * SCREEN_WIDTH + x] = edge;
        framebuffer[yy * SCREEN_WIDTH + x + w - 1] = edge;
    }
}

int banner_is_loaded(void) {
    return banner_loaded;
}

/* True while a crossfade is in progress (background still changing each frame). */
int banner_is_animating(void) {
    return banner_pending_img || (banner_anim && banner_prev && fade_frame < FADE_FRAMES);
}

/* Repaint a rect with the current background: the banner image slice when one
 * is loaded, otherwise the given fallback color. Lets overlays (box-art panel)
 * cover list-text overflow without drawing an opaque card. */
void banner_fill_region(uint16_t *fb, int x, int y, int w, int h, uint16_t fallback) {
    if (!fb) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_WIDTH)  w = SCREEN_WIDTH - x;
    if (y + h > SCREEN_HEIGHT) h = SCREEN_HEIGHT - y;
    if (w <= 0 || h <= 0) return;
    if (banner_loaded && banner_buf) {
        for (int yy = 0; yy < h; yy++)
            memcpy(fb + (size_t)(y + yy) * SCREEN_WIDTH + x,
                   banner_buf + (size_t)(y + yy) * SCREEN_WIDTH + x,
                   (size_t)w * sizeof(uint16_t));
    } else {
        for (int yy = 0; yy < h; yy++) {
            uint16_t *row = fb + (size_t)(y + yy) * SCREEN_WIDTH + x;
            for (int xx = 0; xx < w; xx++) row[xx] = fallback;
        }
    }
}
