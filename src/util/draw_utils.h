#ifndef TOMATBOOT_UEFI_DRAW_UTILS_H
#define TOMATBOOT_UEFI_DRAW_UTILS_H

#include <Uefi.h>

void write_at(int x, int y, const CHAR8* fmt, ...);
void draw_image(int x, int y, CHAR8 image[], int width, int height);

#endif //TOMATBOOT_UEFI_DRAW_UTILS_H
