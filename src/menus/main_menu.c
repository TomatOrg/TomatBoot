#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePathToText.h>

#include <util/draw_utils.h>

#include "main_menu.h"

// green
#define G EFI_GREEN
// gray
#define W EFI_LIGHTGRAY
// red
#define R EFI_RED

// 14x13 (28x13)
static CHAR8 tomato_image[] = {
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

static void draw() {
    write_at(0, 1, "TomatBoot v2");
    write_at(0, 2, "Copyright (c) 2019 TomatOrg");

    UINTN width = 0;
    UINTN height = 0;
    ASSERT_EFI_ERROR(gST->ConOut->QueryMode(gST->ConOut, gST->ConOut->Mode->Mode, &width, &height));

    EFI_TIME time;
    ASSERT_EFI_ERROR(gRT->GetTime(&time, NULL));
    write_at(0, 4, "Boot time: %d/%d/%d %d:%d", time.Day, time.Month, time.Year, time.Hour, time.Minute);

    // TODO: Change colors for the button
    write_at(0, 6, "Press B for BOOTMENY");
    write_at(0, 7, "Press S for SETUP");
    write_at(0, 8, "Press ESC for SHUTDOWN");

    EFI_DEVICE_PATH_PROTOCOL* device_path = NULL;
    ASSERT_EFI_ERROR(gBS->OpenProtocol(
            gImageHandle,
            &gEfiLoadedImageDevicePathProtocolGuid,
            (VOID**)&device_path,
            gImageHandle,
            NULL,
            EFI_OPEN_PROTOCOL_GET_PROTOCOL));

    EFI_DEVICE_PATH_TO_TEXT_PROTOCOL* devpath_to_text = NULL;
    ASSERT_EFI_ERROR(gBS->LocateProtocol(&gEfiDevicePathToTextProtocolGuid, NULL, (VOID**)&devpath_to_text));
    CHAR16* devpath = devpath_to_text->ConvertDevicePathToText(device_path, TRUE, TRUE);
    write_at(0, 17, "%s", devpath);
    // TODO: How do I free the devpath?

    draw_image(30 + ((width - 30) / 2) - 14, 1, tomato_image, 14, 13);
}

menu_t enter_main_menu() {
    draw();

    do {
        // get key press
        UINTN which = 0;
        EFI_INPUT_KEY key = {};
        ASSERT_EFI_ERROR(gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &which));
        ASSERT_EFI_ERROR(gST->ConIn->ReadKeyStroke(gST->ConIn, &key));

        // choose the menu or continue
        if(key.UnicodeChar == L'B') {
            return MENU_BOOT_MENU;
        } else if(key.UnicodeChar == L'S') {
            return MENU_SETUP;
        } else if(key.ScanCode == SCAN_ESC) {
            return MENU_SHUTDOWN;
        }
    } while(TRUE);
}
