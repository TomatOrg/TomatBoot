#include <Library/DebugLib.h>
#include <Protocol/GraphicsOutput.h>
#include "gop_utils.h"

static INT32 get_mode_common(INT32 start, INT32 dir) {
    // get GOP so we can query the resolutions
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;
    ASSERT_EFI_ERROR(gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID**)&gop));

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info = NULL;
    UINTN sizeOfInfo = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
    INTN started = start;
    BOOLEAN looped_once = FALSE;
    do {
        // next mode
        start += dir;

        // don't overflow
        if(dir == -1) {
            if(start < 0) {
                start = gop->Mode->MaxMode - 1;
            }
        }else {
            if(start >= gop->Mode->MaxMode) {
                start = 0;
            }
        }

        // handle full loop
        if(started == start) {
            // if we get to this assert it means there is no compatible mode!
            ASSERT(!looped_once);
            looped_once = TRUE;
            break;
        }

        // make sure this is a supported format
        ASSERT_EFI_ERROR(gop->QueryMode(gop, start, &sizeOfInfo, &info));
        if(info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
            break;
        }
    } while(TRUE);

    return start;
}

INT32 get_first_mode() {
    return get_next_mode(-1);
}

INT32 get_next_mode(INT32 current) {
    return get_mode_common(current, 1);
}

INT32 get_prev_mode(INT32 current) {
    return get_mode_common(current, -1);
}
