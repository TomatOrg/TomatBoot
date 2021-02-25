
#include <Uefi.h>

#include <Library/UefiBootServicesTableLib.h>

#include <Util/DrawUtils.h>
#include <Config/Config.h>
#include <Library/UefiRuntimeLib.h>
#include <Util/Except.h>
#include <Library/BaseLib.h>
#include <Loaders/Loader.h>

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
    WriteAt(0, 14, "Press B for BOOTMENU");
    WriteAt(0, 15, "Press E for EDIT");

    // draw the logo
    DrawImage(30 + ((width - 30) / 2) - 14, 1, TomatoImage, 13, 14);
}

MENU EnterMainMenu(BOOLEAN first) {
    EFI_STATUS Status = EFI_SUCCESS;

    // set params
    CONFIG_ENTRY* DefaultEntry = GetEntryAt(gConfig.DefaultEntry);
    const UINTN TIMER_INTERVAL = 25000 /* 1/40 sec */;
    const UINTN INITIAL_TIMEOUT_COUNTER = (gConfig.Timeout * 1000000) / TIMER_INTERVAL;
    const UINTN BAR_WIDTH = 80;

    // check for timeout
    if (gConfig.Timeout <= 0 && DefaultEntry != NULL) {
        goto boot_default_entry;
    }

    // draw the menu
    Draw();

    // setup the timer
    INTN TimeoutCounter = -1;
    if (first) {
        TimeoutCounter = INITIAL_TIMEOUT_COUNTER;
    }

    do {
        // check if we got a key stroke
        EFI_INPUT_KEY Key = { 0 };
        Status = gST->ConIn->ReadKeyStroke(gST->ConIn, &Key);
        if (Status == EFI_SUCCESS) {
            // we got a keystroke!

            // disable progress bar if enabled
            if (TimeoutCounter >= 0) {
                TimeoutCounter = -1;
                gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
                for (int i = 0; i < BAR_WIDTH; i++) {
                    WriteAt(i, 22, " ");
                }
            }

            // handle the key press
            switch (Key.UnicodeChar) {
                // enter boot menu
                case L'b':
                case L'B': return MENU_BOOT_MENU;

                // enter editor
                case L'e':
                case L'E': return MENU_EDIT;

                // other key, don't care
                default: break;
            }
        }
        CHECK_STATUS(Status == EFI_SUCCESS || Status == EFI_NOT_READY, Status);

        // if we have the timer enabled (aka, the counter is larger than 0) then
        // process the progress bar in here
        if (TimeoutCounter >= 0) {
            // got a timeout on the timer
            TimeoutCounter--;

            if (TimeoutCounter == 0) {
                // timeout expired, boot default entry
                gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
                goto boot_default_entry;
            } else {
                // set bar color
                gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_BLACK, EFI_LIGHTGRAY));

                // write new chunk of bar
                int start = ((INITIAL_TIMEOUT_COUNTER - TimeoutCounter - 1) * BAR_WIDTH) / INITIAL_TIMEOUT_COUNTER;
                int end = ((INITIAL_TIMEOUT_COUNTER - TimeoutCounter) * BAR_WIDTH) / INITIAL_TIMEOUT_COUNTER;
                for(int i = start; i <= end; i++) {
                    WriteAt(i, 22, " ");
                }

                // restart the timer
                EFI_CHECK(gBS->Stall(TIMER_INTERVAL));
            }
        } else {
            // if there is no counter use WaitForEvent instead of polling so
            // we won't burn the cpu lol
            UINTN Index = 0;
            TRACE("Waiting for event...");
            EFI_CHECK(gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &Index));
        }
    } while(TRUE);

boot_default_entry:
    // boot the default entry
    ClearScreen(EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
    TRACE("Booting default entry: %s", DefaultEntry->Name);
    LoadKernel(DefaultEntry);

cleanup:
    ERROR(":(");
    CpuDeadLoop();
    return MENU_MAIN_MENU;
}
