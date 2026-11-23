#ifndef FROGUI_SCREENSHOT_H
#define FROGUI_SCREENSHOT_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

int screenshot_write_bmp(FILE *file, const uint16_t *pixels,
                         unsigned width, unsigned height, size_t pitch);

#endif
