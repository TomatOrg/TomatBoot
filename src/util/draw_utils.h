#ifndef TOMATBOOT_UEFI_DRAW_UTILS_H
#define TOMATBOOT_UEFI_DRAW_UTILS_H

#include <Uefi.h>

void write_at(int x, int y, const CHAR8* fmt, ...);
void draw_image(int x, int y, CHAR8 image[], int width, int height);
void clear_screen(CHAR8 color);
void fill_box(int _x, int _y, int width, int height, CHAR8 color);

#endif //TOMATBOOT_UEFI_DRAW_UTILS_H
