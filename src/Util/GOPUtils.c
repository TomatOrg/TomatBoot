#include <Protocol/GraphicsOutput.h>
#include <Library/UefiBootServicesTableLib.h>
#include "GOPUtils.h"
#include "Except.h"

EFI_STATUS SelectBestGopMode(UINTN* Width, UINTN* Height, UINTN* Scanline, EFI_PHYSICAL_ADDRESS* Framebuffer) {
    EFI_STATUS Status = EFI_SUCCESS;

    CHECK(Width != NULL);
    CHECK(Height != NULL);
    CHECK(Scanline != NULL);

    // get the protocol
    EFI_GRAPHICS_OUTPUT_PROTOCOL* GOP = NULL;
    EFI_CHECK(gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (void**)&GOP));

    // if null get the largest resolution
    if (*Width == 0) {
        *Width = MAX_UINTN;
    }

    if (*Height == 0) {
        *Height = MAX_UINTN;
    }

    // choose the best mode based on the requirements
    UINT32 GoodOption = GOP->Mode->Mode;
    INT32 BestWidth = 0;
    INT32 BestHeight = 0;
    for (int i = 0; i < GOP->Mode->MaxMode; i++) {
        // make sure this is a supported format
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* Info = NULL;
        UINTN SizeOfInfo = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
        EFI_CHECK(GOP->QueryMode(GOP, i, &SizeOfInfo, &Info));

        // only support this format
        if(Info->PixelFormat != PixelBlueGreenRedReserved8BitPerColor) {
            continue;
        }

        // found exact thing, stop here
        if (Info->VerticalResolution == *Height && Info->HorizontalResolution == *Width) {
            GoodOption = i;
            break;
        }

        if (
            (Info->VerticalResolution > *Height && Info->HorizontalResolution > *Width) ||
            (Info->VerticalResolution < BestHeight && Info->HorizontalResolution < BestWidth)
        ) {
            // this is bigger than what we want or smaller than the best we got so far
            continue;
        }

        BestWidth = Info->HorizontalResolution;
        BestHeight = Info->VerticalResolution;
        GoodOption = i;
    }

    // found a good option, set it and return the information about it
    EFI_CHECK(GOP->SetMode(GOP, GoodOption));
    *Width = GOP->Mode->Info->VerticalResolution;
    *Height = GOP->Mode->Info->HorizontalResolution;
    *Scanline = GOP->Mode->Info->PixelsPerScanLine;
    *Framebuffer = GOP->Mode->FrameBufferBase;

cleanup:
    return Status;
}
