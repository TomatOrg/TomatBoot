#include <Uefi.h>
#include <Library/DebugLib.h>

#include <util/draw_utils.h>
#include <loaders/loader.h>
#include <config.h>

#include "boot_menu.h"

static void draw() {
    UINTN width = 0;
    UINTN height = 0;
    ASSERT_EFI_ERROR(gST->ConOut->QueryMode(gST->ConOut, gST->ConOut->Mode->Mode, &width, &height));

    clear_screen(EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));

    fill_box(0, 0, (int) width, 1, EFI_TEXT_ATTR(EFI_BLACK, EFI_LIGHTGRAY));
    write_at((int) (width / 2 - 6), 0, "TomatBoot v2");

    fill_box(0, (int) (height - 2), (int) width, 1, EFI_TEXT_ATTR(EFI_BLACK, EFI_LIGHTGRAY));
}

static const char* loader_names[] = {
    [BOOT_LINUX] = "Linux Boot",
    [BOOT_TBOOT] = "TomatBoot",
    [BOOT_UEFI] = "UEFI Boot",
    [BOOT_MB2] = "MultiBoot2",
};

menu_t enter_boot_menu() {
    UINTN width = 0;
    UINTN height = 0;
    ASSERT_EFI_ERROR(gST->ConOut->QueryMode(gST->ConOut, gST->ConOut->Mode->Mode, &width, &height));

    draw();

    INTN  selected = 0;
    while(TRUE) {

        // draw the entries
        // TODO: Add a way to edit the command line
        for(int i = 0; i < boot_entries.count; i++) {
            boot_entry_t* entry = &boot_entries.entries[i];

            // draw the correct background
            if(i == selected) {
                fill_box(4, 2 + i, (int) width - 8, 1, EFI_TEXT_ATTR(EFI_BLACK, EFI_LIGHTGRAY));
            }else {
                fill_box(4, 2 + i, (int) width - 8, 1, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
            }

            // write the option
            write_at(6, 2 + i, "%a (%a) - %a", entry->name, entry->path, loader_names[entry->protocol]);
        }

        // draw the shutdown option
        if(boot_entries.count == selected) {
            fill_box(4, (int) (2 + boot_entries.count + 1), (int) width - 8, 1, EFI_TEXT_ATTR(EFI_BLACK, EFI_LIGHTGRAY));
        }else {
            fill_box(4, (int) (2 + boot_entries.count + 1), (int) width - 8, 1, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));
        }
        write_at(6, (int) (2 + boot_entries.count + 1), "Shutdown");

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
            if(selected > boot_entries.count + 1) {
                selected = 0;
            }

            // prev potion
        }else if(key.ScanCode == SCAN_UP) {
            selected--;
            if(selected < 0) {
                selected = boot_entries.count;
            }

            // save and exit
        }else if(key.UnicodeChar == CHAR_CARRIAGE_RETURN) {

            // shutdown option
            if(selected >= boot_entries.count) {
                return MENU_SHUTDOWN;

            // choose an os to start
            }else {
                load_binary(&boot_entries.entries[selected]);
            }
        }
    }
}
