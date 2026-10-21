#define _POSIX_C_SOURCE 200809L
#include "../screenshot.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {
    char directory[] = "/tmp/frogui-screenshot-test-XXXXXX";
    assert(mkdtemp(directory));

    const uint16_t pixels[] = {0xf800, 0x07e0, 0x001f, 0xffff};
    char first[512], second[512];
    assert(screenshot_save_rgb565(directory, pixels, 2, 2, first, sizeof(first)) == 0);
    assert(screenshot_save_rgb565(directory, pixels, 2, 2, second, sizeof(second)) == 0);
    assert(strcmp(first, second) != 0);

    struct stat st;
    assert(stat(first, &st) == 0 && st.st_size == 70);
    FILE *file = fopen(first, "rb");
    assert(file);
    unsigned char bmp[70];
    assert(fread(bmp, 1, sizeof(bmp), file) == sizeof(bmp));
    assert(fclose(file) == 0);
    assert(bmp[0] == 'B' && bmp[1] == 'M');
    assert(bmp[18] == 2 && bmp[22] == 2 && bmp[28] == 24);
    /* BMP rows are bottom-up and pixels are BGR, with two bytes of row padding. */
    const unsigned char expected[] = {
        255, 0, 0, 255, 255, 255, 0, 0,
        0, 0, 255, 0, 255, 0, 0, 0
    };
    assert(memcmp(bmp + 54, expected, sizeof(expected)) == 0);

    assert(screenshot_save_rgb565(directory, pixels, 0, 2, second, sizeof(second)) != 0);
    unlink(first);
    unlink(second);
    assert(rmdir(directory) == 0);
    return 0;
}
