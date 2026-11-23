#include "screenshot.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

static void put_le32(unsigned char *dst, uint32_t value) {
    dst[0] = (unsigned char)value;
    dst[1] = (unsigned char)(value >> 8);
    dst[2] = (unsigned char)(value >> 16);
    dst[3] = (unsigned char)(value >> 24);
}

int screenshot_write_bmp(FILE *file, const uint16_t *pixels,
                         unsigned width, unsigned height, size_t pitch) {
    if (!file || !pixels || !width || !height || width > INT_MAX ||
        height > INT_MAX || pitch < (size_t)width * sizeof(*pixels))
        return -1;

    size_t row_size = ((size_t)width * 3 + 3) & ~(size_t)3;
    if (height > (UINT32_MAX - 54u) / row_size)
        return -1;
    uint32_t image_size = (uint32_t)(row_size * height);
    unsigned char header[54] = {0};
    header[0] = 'B'; header[1] = 'M';
    put_le32(header + 2, 54u + image_size);
    put_le32(header + 10, 54);
    put_le32(header + 14, 40);
    put_le32(header + 18, width);
    put_le32(header + 22, (uint32_t)-(int32_t)height);
    header[26] = 1;
    header[28] = 24;
    put_le32(header + 34, image_size);

    unsigned char *row = calloc(1, row_size);
    if (!row)
        return -1;

    int ok = fwrite(header, 1, sizeof(header), file) == sizeof(header);
    for (unsigned y = 0; ok && y < height; y++) {
        const uint16_t *src = (const uint16_t *)((const unsigned char *)pixels + y * pitch);
        memset(row, 0, row_size);
        for (unsigned x = 0; x < width; x++) {
            uint16_t px = src[x];
            unsigned r = (px >> 11) & 31, g = (px >> 5) & 63, b = px & 31;
            row[x * 3]     = (unsigned char)((b << 3) | (b >> 2));
            row[x * 3 + 1] = (unsigned char)((g << 2) | (g >> 4));
            row[x * 3 + 2] = (unsigned char)((r << 3) | (r >> 2));
        }
        ok = fwrite(row, 1, row_size, file) == row_size;
    }
    free(row);
    return ok && !ferror(file) ? 0 : -1;
}
