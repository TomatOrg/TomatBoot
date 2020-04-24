#include "Menus.h"

#include <util/DrawUtils.h>
#include <config/BootEntries.h>

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <loaders/Loaders.h>
#include <Library/CpuLib.h>

static void draw() {
    UINTN width = 0;
    UINTN height = 0;
    ASSERT_EFI_ERROR(gST->ConOut->QueryMode(gST->ConOut, gST->ConOut->Mode->Mode, &width, &height));

    ClearScreen(EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));

    FillBox(0, 0, (int) width, 1, EFI_TEXT_ATTR(EFI_BLACK, EFI_LIGHTGRAY));
    WriteAt((int) (width / 2 - 6), 0, "TomatBoot v2");

    FillBox(0, (int) (height - 2), (int) width, 1, EFI_TEXT_ATTR(EFI_BLACK, EFI_LIGHTGRAY));
}

static const char* loader_names[] = {
        [BOOT_LINUX] = "Linux Boot",
        [BOOT_MB2] = "MultiBoot2",
        [BOOT_STIVALE] = "Stivale",
};

MENU EnterBootMenu() {
    UINTN width = 0;
    UINTN height = 0;
    ASSERT_EFI_ERROR(gST->ConOut->QueryMode(gST->ConOut, gST->ConOut->Mode->Mode, &width, &height));

    draw();

    INTN  selected = 0;
    BOOT_ENTRY* selectedEntry = NULL;
    while(TRUE) {

        // draw the entries
        // TODO: Add a way to edit the command line
        INTN i = 0;
        for(LIST_ENTRY* link = gBootEntries.ForwardLink; link != &gBootEntries; link = link->ForwardLink, i++) {
            BOOT_ENTRY* entry = BASE_CR(link, BOOT_ENTRY, Link);

            // draw the correct background
            if (i == selected) {
                selectedEntry = entry;
                FillBox(4, 2 + i, (int) width - 8, 1, EFI_TEXT_ATTR(EFI_BLACK, EFI_LIGHTGRAY));
            } else {
                FillBox(4, 2 + i, (int) width - 8, 1, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
            }

            // write the option
            WriteAt(6, 2 + i, "%s (%s) - %a", entry->Name, entry->Path, loader_names[entry->Protocol]);
        }

        // draw the shutdown option
        if(i == selected) {
            FillBox(4, (int) (2 + i + 1), (int) width - 8, 1, EFI_TEXT_ATTR(EFI_BLACK, EFI_LIGHTGRAY));
        }else {
            FillBox(4, (int) (2 + i + 1), (int) width - 8, 1, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
        }
        WriteAt(6, (int) (2 + i + 1), "Shutdown");

        // get key press
        UINTN which = 0;
        EFI_INPUT_KEY key = {};
        ASSERT_EFI_ERROR(gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &which));
        EFI_STATUS status = gST->ConIn->ReadKeyStroke(gST->ConIn, &key);
        if(status == EFI_NOT_READY) {
            continue;
        }
        ASSERT_EFI_ERROR(status);

        // decrease value
        if(key.ScanCode == SCAN_DOWN) {
            selected++;
            if(selected > i + 1) {
                selected = 0;
            }

            // prev potion
        }else if(key.ScanCode == SCAN_UP) {
            selected--;
            if(selected < 0) {
                selected = i;
            }

            // save and exit
        }else if(key.UnicodeChar == CHAR_CARRIAGE_RETURN) {

            // shutdown option
            if(selected >= i) {
                return MENU_SHUTDOWN;

                // choose an os to start
            }else {
                LoadKernel(selectedEntry);
                while(1) CpuSleep();
            }
        }
    }
}