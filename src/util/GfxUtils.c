#include "GfxUtils.h"
#include "Except.h"

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Protocol/GraphicsOutput.h>
#include <Library/UefiBootServicesTableLib.h>

static INT32 GetModeCommon(INT32 start, INT32 dir) {
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

INT32 GetFirstGfxMode() {
    return GetNextGfxMode(-1);
}

INT32 GetNextGfxMode(INT32 Current) {
    return GetModeCommon(Current, 1);
}

INT32 GetPrevGfxMode(INT32 Current) {
    return GetModeCommon(Current, -1);
}

INT32 GetBestGfxMode(INT32 Width, INT32 Height) {
    // get GOP so we can query the resolutions
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;
    ASSERT_EFI_ERROR(gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID**)&gop));

    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info = NULL;
    UINTN sizeOfInfo = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);

    INTN i;
    INT32 goodOption;
    INT32 BestWidth = 0;
    INT32 BestHeight = 0;
    for (i = 0; i < gop->Mode->MaxMode; i++) {
        // make sure this is a supported format
        ASSERT_EFI_ERROR(gop->QueryMode(gop, i, &sizeOfInfo, &info));
        if(info->PixelFormat != PixelBlueGreenRedReserved8BitPerColor) {
            continue;
        }

        // found exact thing, stop here
        if (info->VerticalResolution == Height && info->HorizontalResolution == Width) {
            return i;
        }

        if (
            (info->VerticalResolution > Height && info->HorizontalResolution > Width) ||
            (info->VerticalResolution < BestHeight && info->HorizontalResolution < BestWidth)
        ) {
            // this is bigger than what we want or smaller than the best we got so far
            continue;
        }

        BestWidth = info->HorizontalResolution;
        BestHeight = info->VerticalResolution;
        goodOption = i;
    }

    return goodOption;
}
