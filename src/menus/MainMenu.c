#include "Menus.h"

#include <util/DrawUtils.h>

#include <config/BootConfig.h>
#include <config/BootEntries.h>

#include <Uefi.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/GraphicsOutput.h>
#include <Protocol/DevicePathToText.h>
#include <Library/DebugLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>
#include <loaders/Loaders.h>

// 14x13 (28x13)
#define G EFI_GREEN
#define W EFI_LIGHTGRAY
#define R EFI_RED
__attribute__((unused))
static CHAR8 TomatoImage[] = {
        0, 0, G, 0, 0, 0, 0, 0, 0, G, 0, 0, 0, 0,
        0, 0, 0, G, G, 0, 0, G, G, 0, 0, 0, 0, 0,
        G, G, 0, 0, G, G, G, G, G, G, G, G, 0, 0,
        0, 0, G, G, G, G, G, G, G, G, R, R, G, 0,
        0, 0, R, G, G, R, R, R, R, R, G, R, R, 0,
        0, R, G, R, R, R, R, W, W, R, R, R, R, R,
        0, R, R, R, R, R, R, W, W, W, R, R, R, R,
        0, R, R, R, R, R, R, R, W, W, R, R, R, R,
        0, R, R, R, R, R, R, R, R, R, R, R, R, R,
        0, R, R, R, R, R, R, R, R, R, R, R, R, 0,
        0, 0, R, R, R, R, R, R, R, R, R, R, 0, 0,
        0, 0, 0, R, R, R, R, R, R, R, R, 0, 0, 0,
        0, 0, 0, 0, R, R, R, R, R, R, 0, 0, 0, 0,
};
#undef G
#undef W
#undef R

// (14x14) (28x14)
#define G EFI_GREEN
#define W EFI_WHITE
#define r EFI_LIGHTRED
#define R EFI_RED
__attribute__((unused))
static CHAR8 TomatoImage2[] = {
        0, 0, 0, 0, 0, 0, 0, 0, G, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, G, 0, 0, 0, 0, 0,
        0, 0, 0, G, 0, 0, 0, G, 0, 0, G, 0, 0,
        0, 0, 0, 0, G, 0, G, 0, G, G, 0, 0, 0,
        0, 0, 0, 0, r, G, G, G, R, r, 0, 0, 0,
        0, 0, r, r, G, G, R, R, G, r, r, r, 0,
        0, 0, r, G, r, r, r, r, r, W, W, r, 0,
        0, R, r, r, r, r, r, r, r, r, W, r, r,
        0, R, r, r, r, r, r, r, r, r, r, W, r,
        0, R, r, r, r, r, r, r, r, r, r, r, r,
        0, R, R, r, r, r, r, r, r, r, r, r, r,
        0, 0, R, R, r, r, r, r, r, r, r, r, 0,
        0, 0, R, R, R, R, R, r, r, r, r, r, 0,
        0, 0, 0, 0, R, R, R, R, R, R, 0, 0, 0,
};
#undef G
#undef W
#undef R
#undef r

static void draw() {
    ClearScreen(EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));

    WriteAt(0, 1, "TomatBoot v2");
    WriteAt(0, 2, "Copyright (c) 2020 TomatOrg");

    UINTN width = 0;
    UINTN height = 0;
    ASSERT_EFI_ERROR(gST->ConOut->QueryMode(gST->ConOut, gST->ConOut->Mode->Mode, &width, &height));

    // read the config so I can display some stuff from it
    BOOT_CONFIG config;
    LoadBootConfig(&config);

    // get GOP so we can query the resolutions
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;
    ASSERT_EFI_ERROR(gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID**)&gop));
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info = NULL;
    UINTN sizeOfInfo = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
    ASSERT_EFI_ERROR(gop->QueryMode(gop, config.GfxMode, &sizeOfInfo, &info));

    // display some nice info
    EFI_TIME time;
    ASSERT_EFI_ERROR(gRT->GetTime(&time, NULL));
    WriteAt(0, 4, "Current time: %d/%d/%d %d:%d", time.Day, time.Month, time.Year, time.Hour, time.Minute);
    WriteAt(0, 5, "Graphics mode: %dx%d", info->HorizontalResolution, info->VerticalResolution);
    WriteAt(0, 6, "Current OS: %s (%s)", gDefaultEntry->Name, gDefaultEntry->Path);
    WriteAt(0, 7, "UEFI Version: %d.%d", (gST->Hdr.Revision >> 16u) & 0xFFFFu, gST->Hdr.Revision & 0xFFFFu);

    // options for what we can do
    WriteAt(0, 13, "Press B for BOOTMENU");
    WriteAt(0, 14, "Press S for SETUP");
    WriteAt(0, 15, "Press TAB for SHUTDOWN");

    // draw the logo
    DrawImage(30 + ((width - 30) / 2) - 14, 1, TomatoImage2, 13, 14);
}

MENU EnterMainMenu(BOOLEAN first) {
    draw();
    ASSERT_EFI_ERROR(gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_RED, EFI_BLACK)));

    // read the config
    BOOT_CONFIG config;
    LoadBootConfig(&config);

    // create the timer event and counter
    const UINTN TIMER_INTERVAL = 250000 /* 1/40 sec */;
    const UINTN INITIAL_TIMEOUT_COUNTER = ((gBootDelayOverride > 0 ? gBootDelayOverride : config.BootDelay) * 10000000) / TIMER_INTERVAL;
    const UINTN BAR_WIDTH = 80;

    UINTN timeout_counter = INITIAL_TIMEOUT_COUNTER;
    EFI_EVENT events[2] = { gST->ConIn->WaitForKey };
    ASSERT_EFI_ERROR(gBS->CreateEvent(EVT_TIMER, TPL_CALLBACK, NULL, NULL, &events[1]));

    if(first) {
        ASSERT_EFI_ERROR(gBS->SetTimer(events[1], TimerRelative, TIMER_INTERVAL));
    }

    UINTN count = 2;
    do {
        // get key press
        UINTN which = 0;
        EFI_INPUT_KEY key = {};

        ASSERT_EFI_ERROR(gBS->WaitForEvent(count, events, &which));

        // got a keypress
        if(which == 0) {
            // get key
            EFI_STATUS status = gST->ConIn->ReadKeyStroke(gST->ConIn, &key);
            if(status == EFI_NOT_READY) {
                continue;
            }
            ASSERT_EFI_ERROR(status);

            // cancel timer and destroy it
            if(count == 2) {
                ASSERT_EFI_ERROR(gBS->SetTimer(events[1], TimerCancel, 0));
                ASSERT_EFI_ERROR(gBS->CloseEvent(events[1]));
                count = 1;

                // clear the progress bar
                ASSERT_EFI_ERROR(gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK)));
                for (int i = 0; i < BAR_WIDTH; i++) {
                    WriteAt(i, 22, " ");
                }
            }

            // choose the menu or continue
            if(key.UnicodeChar == L'b' || key.UnicodeChar == L'B') {
                return MENU_BOOT_MENU;
            } else if(key.UnicodeChar == L's' || key.UnicodeChar == L'S') {
                return MENU_SETUP;
            } else if(key.ScanCode == CHAR_TAB) {
                return MENU_SHUTDOWN;
            }

            // got timeout
        } else {
            timeout_counter--;
            if(timeout_counter == 0) {
                // set normal text color
                ASSERT_EFI_ERROR(gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK)));

                // close the event
                ASSERT_EFI_ERROR(gBS->CloseEvent(events[1]));

                // call the loader
                LoadKernel(gDefaultEntry);
            } else {
                // set bar color
                ASSERT_EFI_ERROR(gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_BLACK, EFI_LIGHTGRAY)));

                // write new chunk of bar
                int start = ((INITIAL_TIMEOUT_COUNTER - timeout_counter - 1) * BAR_WIDTH) / INITIAL_TIMEOUT_COUNTER;
                int end = ((INITIAL_TIMEOUT_COUNTER - timeout_counter) * BAR_WIDTH) / INITIAL_TIMEOUT_COUNTER;
                for(int i = start; i <= end; i++) {
                    WriteAt(i, 22, " ");
                }

                // restart the timer
                ASSERT_EFI_ERROR(gBS->SetTimer(events[1], TimerRelative, TIMER_INTERVAL));
            }

        }
    } while(TRUE);
}