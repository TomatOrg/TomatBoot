#include <Library/UefiBootServicesTableLib.h>
#include "DrawUtils.h"
#include "Except.h"

void WriteAt(int x, int y, const CHAR8* fmt, ...) {
    gST->ConOut->SetCursorPosition(gST->ConOut, x, y);
    VA_LIST marker;
    VA_START(marker, fmt);
    DebugVPrint(DEBUG_INFO, fmt, marker);
    VA_END(marker);
}

void DrawImage(int _x, int _y, CHAR8 image[], int width, int height) {
    CHAR16 str[] = { BLOCKELEMENT_FULL_BLOCK, BLOCKELEMENT_FULL_BLOCK, BLOCKELEMENT_FULL_BLOCK, 0 };
    for(int y = _y; y < _y + height; y++) {
        gST->ConOut->SetCursorPosition(gST->ConOut, _x, y);
        for(int x = _x; x < _x + width; x++) {
            CHAR8 pix = image[(x - _x) + (y - _y) * width];
            gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(pix, EFI_BLACK));
            gST->ConOut->OutputString(gST->ConOut, str);
        }
    }

    gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_DARKGRAY, EFI_BLACK));
}

void ClearScreen(CHAR8 color) {
    UINTN width = 0;
    UINTN height = 0;
    gST->ConOut->QueryMode(gST->ConOut, gST->ConOut->Mode->Mode, &width, &height);

    FillBox(0, 0, width, height, color);
}

void FillBox(int _x, int _y, int width, int height, CHAR8 color) {
    gST->ConOut->SetAttribute(gST->ConOut, color);
    for(int y = _y; y < _y + height; y++) {
        gST->ConOut->SetCursorPosition(gST->ConOut, _x, y);
        for(int x = _x; x < _x + width; x++) {
            gST->ConOut->OutputString(gST->ConOut, L" ");
        }
    }
}