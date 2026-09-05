/*
 * input.c - Cubevol shared memory input for SF3000
 * Reads button state from /tmp/joy_key shared memory.
 * Remap table loaded from KEYMAP_FILE; falls back to hardcoded defaults.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/time.h>
#include "input.h"

static volatile uint32_t *cubevol_keys = NULL;
static int shmid = -1;

static uint32_t current_state = 0;
static uint32_t prev_state    = 0;

/* Default raw bit for each logical button (-1 = unmapped). */
static const int default_bits[FROG_BTN_COUNT] = {
    [FROG_BTN_UP]     = 4,
    [FROG_BTN_DOWN]   = 6,
    [FROG_BTN_LEFT]   = 7,
    [FROG_BTN_RIGHT]  = 5,
    [FROG_BTN_A]      = 13,
    [FROG_BTN_B]      = 14,
    [FROG_BTN_X]      = 12,
    [FROG_BTN_Y]      = 15,
    [FROG_BTN_L1]     = 10,
    [FROG_BTN_R1]     = 11,
    [FROG_BTN_L2]     = -1,
    [FROG_BTN_R2]     = -1,
    [FROG_BTN_START]  = 3,
    [FROG_BTN_SELECT] = 0,
    [FROG_BTN_FN]     = 16,
};

static int remap_bits[FROG_BTN_COUNT];
static uint32_t remap_raw_masks[FROG_BTN_COUNT]; /* precomputed: 1<<bit or 0 if unmapped */
static uint32_t remap_logical_bits[FROG_BTN_COUNT]; /* precomputed: 1<<logical */

static void rebuild_masks(void) {
    for (int i = 0; i < FROG_BTN_COUNT; i++) {
        int b = remap_bits[i];
        remap_raw_masks[i]    = (b >= 0 && b <= FROG_RAW_MAX_BIT) ? (1u << b) : 0;
        remap_logical_bits[i] = 1u << i;
    }
}

static const char *btn_names[FROG_BTN_COUNT] = {
    "UP","DOWN","LEFT","RIGHT","A","B","X","Y","L1","R1","L2","R2","START","SELECT","FN"
};

const char *input_btn_name(FrogButton btn) {
    if (btn < 0 || btn >= FROG_BTN_COUNT) return "?";
    return btn_names[btn];
}

/* FN capability — mirrors picoarch's sf3000_fn_capability_init() semantics.
 * The physical FN button is only confirmed on the R36SX family (raw bit 16);
 * other devices (SF3000/SF3500/GB350/SF3100/R36HD-class clones) must not get an
 * FN default mapping nor an FN wizard step, or they would be asked to map a
 * button they do not have and any unrelated bit 16 signal would be read as FN.
 * Detection: TF_DEVICE=R36SX from the boot env file zhijack writes
 * (/tmp/tfdevice.env), falling back to the inherited TF_DEVICE env var.
 * Overrides (both directions, same as picoarch): SF3000_HAS_FN env var and
 * /mnt/sdcard/fn_enable flag file. Raw bit 16 itself stays data-driven: if FN
 * is not mapped, the bit is simply inert. */
bool input_fn_available(void) {
    static int available = -1;
    if (available >= 0) return available != 0;

    int has = 0;
    const char *tfdev = getenv("TF_DEVICE");
    if (tfdev && strcmp(tfdev, "R36SX") == 0) has = 1;
    else {
        FILE *tf = fopen("/tmp/tfdevice.env", "r");
        if (tf) {
            char line[128];
            while (fgets(line, sizeof line, tf)) {
                if (strstr(line, "TF_DEVICE=R36SX")) { has = 1; break; }
            }
            fclose(tf);
        }
    }
    const char *ev = getenv("SF3000_HAS_FN");
    if (ev && (ev[0]=='1' || ev[0]=='y' || ev[0]=='Y')) has = 1;
    else if (ev && (ev[0]=='0' || ev[0]=='n' || ev[0]=='N')) has = 0;
    FILE *f = fopen("/mnt/sdcard/fn_enable", "r");
    if (f) {
        char c;
        if (fread(&c, 1, 1, f) == 1) {
            if (c=='1' || c=='y' || c=='Y') has = 1;
            else if (c=='0' || c=='n' || c=='N') has = 0;
        }
        fclose(f);
    }

    available = has;
    return has != 0;
}

void input_reset_defaults(void) {
    for (int i = 0; i < FROG_BTN_COUNT; i++)
        remap_bits[i] = default_bits[i];
    /* FN defaults to raw bit 16 only on devices that have the button; on
     * everything else it ships unmapped (-1 = inert). A saved FN=<bit> line is
     * likewise ignored at load time on FN-less devices (bit 16 there is an
     * unrelated signal, not FN). */
    if (!input_fn_available())
        remap_bits[FROG_BTN_FN] = -1;
    rebuild_masks();
}

int input_init(void) {
    input_reset_defaults();
    key_t key = ftok("/tmp/joy_key", 'a');
    if (key == (key_t)-1) return -1;
    shmid = shmget(key, 4, 0666);
    if (shmid < 0) return -1;
    cubevol_keys = (volatile uint32_t *)shmat(shmid, NULL, 0);
    if (cubevol_keys == (void *)-1) { cubevol_keys = NULL; return -1; }
    current_state = 0;
    prev_state    = 0;
    return 0;
}

void input_deinit(void) {
    if (cubevol_keys) { shmdt((void *)cubevol_keys); cubevol_keys = NULL; }
    shmid = -1;
}

/* Right-stick directions reach us as the A/B/X/Y bits (cubevol merges the analog
 * stick into the same GPIO matrix bits as the face buttons — no separable signal
 * anywhere). In games that's fine (stick acts like the buttons), but in menus the
 * stick's drift and accidental brushes fire false A/B/X/Y. We can't tell a real
 * press from the stick (same bits), so we debounce: a face bit must stay set for
 * a short, fixed time before it registers. Drift glances and quick
 * brushes (shorter than that) are dropped; a deliberate tap/hold passes with a
 * small latency. Release clears instantly. Dpad and the rest stay instant.
 * This runs only in the FrogUI menu (games read the shm directly), so in-game
 * response is unaffected. */
/* Raw state from a libretro frontend (rkgame/picoarch), set each frame by the core
 * via input_set_ext_raw — this is picoarch's ALREADY-debounced input. */
static uint32_t ext_raw = 0;

#define FACE_BITS    0xF000u
#define FACE_HOLD_MS 65   /* stable duration required for A/B/X/Y */

static long input_now_ms(void) {
    struct timeval t; gettimeofday(&t, NULL);
    return (long)t.tv_sec * 1000 + t.tv_usec / 1000;
}

void input_update(void) {
    /* Combine the raw joy_key shm with ext_raw (picoarch's ALREADY-debounced input
     * via input_state_cb). The shm alone flickers from the rkgame+cubevol two-writer
     * race → ghost menu inputs; ORing+debouncing below removes that. */
    uint32_t raw = (cubevol_keys ? (*cubevol_keys & FROG_RAW_BUTTON_MASK) : 0) | (ext_raw & FROG_RAW_BUTTON_MASK);

    /* Debounce face bits by ELAPSED TIME, not update count. The redraw-on-demand
     * UI polls near 1kHz between presented frames, so the old 4-update debounce
     * shrank from its intended ~65ms to ~4ms. That allowed the right stick's
     * merged X-bit glitches to open Search while a game list was scrolling.
     * Navigation stays immediate; release always clears immediately. */
    static long down_since[FROG_RAW_BIT_COUNT] = {0};
    static uint32_t raw_down = 0;
    static uint32_t committed = 0;
    long now = input_now_ms();
    for (int b = 0; b < FROG_RAW_BIT_COUNT; b++) {
        uint32_t m = 1u << b;
        if (raw & m) {
            if (!(raw_down & m)) {
                raw_down |= m;
                down_since[b] = now;
            }
            if (!(m & FACE_BITS) || now - down_since[b] >= FACE_HOLD_MS)
                committed |= m;
        } else {
            raw_down &= ~m;
            down_since[b] = 0;
            committed &= ~m;
        }
    }
    raw = committed;

    prev_state = current_state;
    uint32_t s = 0;
    for (int i = 0; i < FROG_BTN_COUNT; i++) {
        if (raw & remap_raw_masks[i])
            s |= remap_logical_bits[i];
    }
    current_state = s;
}

bool input_is_pressed(FrogButton btn) {
    return (current_state >> btn) & 1;
}

bool input_was_pressed(FrogButton btn) {
    return ((current_state >> btn) & 1) && !((prev_state >> btn) & 1);
}

/* Auto-repeat for menu navigation: fires on the initial press, then (after a
 * short delay) repeatedly while held. TIME-based (ms), so the repeat rate is the
 * same regardless of frame rate - no more one-tap-per-item scrolling. */
#define REPEAT_DELAY_MS 300   /* hold this long before repeating */
#define REPEAT_RATE_MS  70    /* then one step every this many ms */
bool input_repeat(FrogButton btn) {
    static long hold_start[32], last_rep[32];
    int held = (current_state >> btn) & 1;
    int was  = (prev_state    >> btn) & 1;
    long t = input_now_ms();
    if (held && !was) { hold_start[btn] = t; last_rep[btn] = t; return true; }  /* edge */
    if (held && was &&
        t - hold_start[btn] >= REPEAT_DELAY_MS &&
        t - last_rep[btn]   >= REPEAT_RATE_MS) {
        last_rep[btn] = t;
        return true;
    }
    return false;
}

void input_set_ext_raw(uint32_t raw) { ext_raw = raw & FROG_RAW_BUTTON_MASK; }

uint32_t input_get_raw_state(void) {
    uint32_t shm = cubevol_keys ? (*cubevol_keys & FROG_RAW_BUTTON_MASK) : 0;
    return (shm | ext_raw) & FROG_RAW_BUTTON_MASK;
}

void input_set_raw_bit(FrogButton btn, int raw_bit) {
    if (btn >= 0 && btn < FROG_BTN_COUNT &&
        raw_bit >= -1 && raw_bit <= FROG_RAW_MAX_BIT) {
        remap_bits[btn] = raw_bit;
        rebuild_masks();
    }
}

int input_get_raw_bit(FrogButton btn) {
    if (btn < 0 || btn >= FROG_BTN_COUNT) return -1;
    return remap_bits[btn];
}

int input_load_remap(const char *path) {
    static int loaded = 0;
    if (loaded) return 0;
    FILE *f = fopen(path, "r");
    if (!f) { loaded = 1; return -1; }
    char line[64];
    while (fgets(line, sizeof(line), f)) {
        char *eq = strchr(line, '=');
        if (!eq) continue;
        *eq = '\0';
        int val = atoi(eq + 1);
        for (int i = 0; i < FROG_BTN_COUNT; i++) {
            if (strcmp(line, btn_names[i]) == 0) {
                /* Ignore a saved FN mapping on devices without the button (e.g.
                 * a keymap written on an R36SX SD reused elsewhere) — bit 16
                 * there is an unrelated signal, not FN. */
                if (i == FROG_BTN_FN && !input_fn_available()) break;
                /* Never let a malformed hand-edited keymap create an invalid
                 * shift/mask later in the remap wizard. */
                if (val >= -1 && val <= FROG_RAW_MAX_BIT)
                    remap_bits[i] = val;
                break;
            }
        }
    }
    fclose(f);
    rebuild_masks();
    loaded = 1;
    return 0;
}

int input_save_remap(const char *path) {
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    for (int i = 0; i < FROG_BTN_COUNT; i++)
        fprintf(f, "%s=%d\n", btn_names[i], remap_bits[i]);
    fflush(f);
    fclose(f);
    return 0;
}
