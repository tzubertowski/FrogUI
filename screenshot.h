#ifndef FROGUI_SCREENSHOT_H
#define FROGUI_SCREENSHOT_H

#include <stddef.h>
#include <stdint.h>

int screenshot_save_rgb565(const char *directory, const uint16_t *pixels,
                           int width, int height, char *path, size_t path_size);

#endif
