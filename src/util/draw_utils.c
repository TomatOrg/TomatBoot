#include <Library/DebugLib.h>
#include "draw_utils.h"

void write_at(int x, int y, const CHAR8* fmt, ...) {
    ASSERT_EFI_ERROR(gST->ConOut->SetCursorPosition(gST->ConOut, x, y));
    VA_LIST marker;
    VA_START(marker, fmt);
    DebugVPrint(0, fmt, marker);
    VA_END(marker);
}

void draw_image(int _x, int _y, CHAR8 image[], int width, int height) {
    static CHAR16 output[] = { BLOCKELEMENT_FULL_BLOCK, 0 };
    for(int y = _y; y < _y + height; y++) {
        ASSERT_EFI_ERROR(gST->ConOut->SetCursorPosition(gST->ConOut, _x, y));
        for(int x = _x; x < _x + width; x++) {
            CHAR8 pix = image[(x - _x) + (y - _y) * width];
            ASSERT_EFI_ERROR(gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(pix, EFI_BLACK)));
            ASSERT_EFI_ERROR(gST->ConOut->OutputString(gST->ConOut, output));
            ASSERT_EFI_ERROR(gST->ConOut->OutputString(gST->ConOut, output));
        }
    }

    gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_DARKGRAY, EFI_BLACK));
}
