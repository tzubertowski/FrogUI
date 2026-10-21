#define _POSIX_C_SOURCE 200809L
#include "screenshot.h"

#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

static void put_u16le(unsigned char *p, unsigned value) {
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
}

static void put_u32le(unsigned char *p, unsigned value) {
    p[0] = (unsigned char)value;
    p[1] = (unsigned char)(value >> 8);
    p[2] = (unsigned char)(value >> 16);
    p[3] = (unsigned char)(value >> 24);
}

static int write_all(int fd, const void *data, size_t size) {
    const unsigned char *p = data;
    while (size) {
        ssize_t n = write(fd, p, size);
        if (n < 0 && errno == EINTR) continue;
        if (n <= 0) return -1;
        p += n;
        size -= (size_t)n;
    }
    return 0;
}

int screenshot_save_rgb565(const char *directory, const uint16_t *pixels,
                           int width, int height, char *path, size_t path_size) {
    if (!directory || !pixels || width <= 0 || height <= 0 || !path || !path_size)
        return -1;

    if ((size_t)width > (SIZE_MAX - 3) / 3) return -1;
    size_t row_bytes = ((size_t)width * 3 + 3) & ~(size_t)3;
    if (row_bytes > (SIZE_MAX - 54) / (size_t)height ||
        54 + row_bytes * (size_t)height > UINT_MAX)
        return -1;
    unsigned file_size = (unsigned)(54 + row_bytes * (size_t)height);

    if (mkdir(directory, 0755) != 0 && errno != EEXIST) return -1;
    time_t now = time(NULL);
    struct tm tm_now;
    if (now == (time_t)-1 || !localtime_r(&now, &tm_now)) return -1;

    int fd = -1;
    for (unsigned sequence = 0; sequence < 10000; sequence++) {
        int n = snprintf(path, path_size,
                         "%s/frogui_%04d%02d%02d_%02d%02d%02d_%04u.bmp",
                         directory, tm_now.tm_year + 1900, tm_now.tm_mon + 1,
                         tm_now.tm_mday, tm_now.tm_hour, tm_now.tm_min,
                         tm_now.tm_sec, sequence);
        if (n < 0 || (size_t)n >= path_size) return -1;
        fd = open(path, O_WRONLY | O_CREAT | O_EXCL, 0644);
        if (fd >= 0) break;
        if (errno != EEXIST) return -1;
    }
    if (fd < 0) return -1;

    unsigned char header[54] = {0};
    header[0] = 'B'; header[1] = 'M';
    put_u32le(header + 2, file_size);
    put_u32le(header + 10, 54);
    put_u32le(header + 14, 40);
    put_u32le(header + 18, (unsigned)width);
    put_u32le(header + 22, (unsigned)height);
    put_u16le(header + 26, 1);
    put_u16le(header + 28, 24);
    put_u32le(header + 34, file_size - 54);

    unsigned char *row = calloc(1, row_bytes);
    int result = row ? write_all(fd, header, sizeof(header)) : -1;
    for (int y = height - 1; result == 0 && y >= 0; y--) {
        for (int x = 0; x < width; x++) {
            uint16_t pixel = pixels[(size_t)y * (size_t)width + (size_t)x];
            unsigned char *bgr = row + (size_t)x * 3;
            unsigned r = (pixel >> 11) & 31;
            unsigned g = (pixel >> 5) & 63;
            unsigned b = pixel & 31;
            bgr[0] = (unsigned char)((b << 3) | (b >> 2));
            bgr[1] = (unsigned char)((g << 2) | (g >> 4));
            bgr[2] = (unsigned char)((r << 3) | (r >> 2));
        }
        result = write_all(fd, row, row_bytes);
    }
    free(row);
    if (close(fd) != 0) result = -1;
    if (result != 0) {
        unlink(path);
        path[0] = '\0';
        return -1;
    }
    return 0;
}
