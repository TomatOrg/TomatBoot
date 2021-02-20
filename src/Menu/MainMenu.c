
#include <Uefi.h>

#include <Library/UefiBootServicesTableLib.h>

#include <Util/DrawUtils.h>
#include <Config/Config.h>
#include <Library/UefiRuntimeLib.h>
#include <Util/Except.h>
#include <Library/BaseLib.h>

#include "MainMenu.h"
#include "Menus.h"

// (14x14) (28x14)
#define G EFI_GREEN
#define W EFI_WHITE
#define r EFI_LIGHTRED
#define R EFI_RED
static CHAR8 TomatoImage[] = {
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

static void Draw() {
    ClearScreen(EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));

    WriteAt(0, 1, "TomatBoot");
    WriteAt(0, 2, "Copyright (c) 2021 TomatOrg");

    UINTN width = 0;
    UINTN height = 0;
    gST->ConOut->QueryMode(gST->ConOut, gST->ConOut->Mode->Mode, &width, &height);

    // Display the time
    EFI_TIME Time = { 0 };
    EfiGetTime(&Time, NULL);
    WriteAt(0, 4, "Current time: %t", &Time);

    // TODO: get the current resolution
    WriteAt(0, 5, "Graphics mode: ...");

    // Print the default entry
    CONFIG_ENTRY* Entry = GetEntryAt(gConfig.DefaultEntry);
    if (Entry != NULL) {
        WriteAt(0, 6, "Current OS: %s", Entry->Name);
    } else {
        WriteAt(0, 6, "No default boot entry!");
    }

    // print the firmware info
    WriteAt(0, 7, "Firmware: %s (%08x)", gST->FirmwareVendor, gST->FirmwareRevision);
    WriteAt(0, 8, "UEFI Version: %d.%d", (gST->Hdr.Revision >> 16u) & 0xFFFFu, gST->Hdr.Revision & 0xFFFFu);

    // options
    WriteAt(0, 13, "Press B for BOOTMENU");
    WriteAt(0, 14, "Press S for SETUP");
    WriteAt(0, 15, "Press TAB for SHUTDOWN");

    // draw the logo
    DrawImage(30 + ((width - 30) / 2) - 14, 1, TomatoImage, 13, 14);
}

MENU EnterMainMenu(BOOLEAN first) {
    // set params
    CONFIG_ENTRY* DefaultEntry = GetEntryAt(gConfig.DefaultEntry);
    const UINTN TIMER_INTERVAL = 250000 /* 1/40 sec */;
    const UINTN INITIAL_TIMEOUT_COUNTER = (gConfig.Timeout * 10000000) / TIMER_INTERVAL;
//    const UINTN BAR_WIDTH = 80;

    // check for timeout
    if (gConfig.Timeout <= 0 && DefaultEntry != NULL) {
        goto boot_default_entry;
    }

    // draw the menu
    Draw();

    // setup the timer if needed
    UINTN TimeoutCounter = INITIAL_TIMEOUT_COUNTER;
    TRACE("%d", TimeoutCounter);
    ASSERT_EFI_ERROR(gBS->Stall(100000000));

//    do {
//        // TODO: check key stroke
//
//        // got a timeout on the timer
//        TimeoutCounter--;
//
//        if (TimeoutCounter == 0) {
//            ASSERT_EFI_ERROR(gBS->Stall(1000000));
//
//            // timeout expired, boot default entry
//            gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
//            goto boot_default_entry;
//        } else {
//            // set bar color
//            gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_BLACK, EFI_LIGHTGRAY));
//
//            // write new chunk of bar
//            int start = ((INITIAL_TIMEOUT_COUNTER - TimeoutCounter - 1) * BAR_WIDTH) / INITIAL_TIMEOUT_COUNTER;
//            int end = ((INITIAL_TIMEOUT_COUNTER - TimeoutCounter) * BAR_WIDTH) / INITIAL_TIMEOUT_COUNTER;
//            for(int i = start; i <= end; i++) {
//                WriteAt(i, 22, " ");
//            }
//
//            // restart the timer
//            ASSERT_EFI_ERROR(gBS->Stall(TIMER_INTERVAL));
//        }
//
//    } while(TRUE);

boot_default_entry:
    // boot the default entry
    ClearScreen(EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));

    TRACE("Booting default entry: %s", DefaultEntry->Name);
    while(1);
}
