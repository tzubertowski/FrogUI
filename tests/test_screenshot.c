#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "../screenshot.h"

int main(void) {
    const uint16_t pixels[] = {0xf800, 0x07e0}; /* red, green */
    unsigned char bmp[62];
    FILE *f = tmpfile();
    assert(f);
    assert(screenshot_write_bmp(f, pixels, 2, 1, sizeof(pixels)) == 0);
    rewind(f);
    assert(fread(bmp, 1, sizeof(bmp), f) == sizeof(bmp));
    fclose(f);

    assert(memcmp(bmp, "BM", 2) == 0);
    assert(bmp[2] == sizeof(bmp) && bmp[10] == 54 && bmp[28] == 24);
    assert(bmp[18] == 2 && bmp[22] == 0xff); /* 2x1, top-down */
    assert(bmp[54] == 0 && bmp[55] == 0 && bmp[56] == 255); /* red BGR */
    assert(bmp[57] == 0 && bmp[58] == 255 && bmp[59] == 0); /* green BGR */
    assert(bmp[60] == 0 && bmp[61] == 0); /* row padding */
    return 0;
}
