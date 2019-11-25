#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Protocol/LoadedImage.h>
#include <Protocol/DevicePathToText.h>

#include <util/draw_utils.h>
#include <Protocol/GraphicsOutput.h>
#include <config.h>
#include <loaders/tboot_loader.h>

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

static const char* UEFI_VERSIONS[] = {

};

static void draw() {
    clear_screen(EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK));

    write_at(0, 1, "TomatBoot v2");
    write_at(0, 2, "Copyright (c) 2019 TomatOrg");

    UINTN width = 0;
    UINTN height = 0;
    ASSERT_EFI_ERROR(gST->ConOut->QueryMode(gST->ConOut, gST->ConOut->Mode->Mode, &width, &height));

    // read the config so I can display some stuff from it
    boot_config_t config;
    load_boot_config(&config);

    // get GOP so we can query the resolutions
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;
    ASSERT_EFI_ERROR(gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID**)&gop));
    EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info = NULL;
    UINTN sizeOfInfo = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
    ASSERT_EFI_ERROR(gop->QueryMode(gop, config.gfx_mode, &sizeOfInfo, &info));

    // get the boot entry
    boot_entry_t* entry = &boot_entries.entries[config.default_os];

    // display some nice info
    EFI_TIME time;
    ASSERT_EFI_ERROR(gRT->GetTime(&time, NULL));
    write_at(0, 4, "Current time: %d/%d/%d %d:%d", time.Day, time.Month, time.Year, time.Hour, time.Minute);
    write_at(0, 5, "Graphics mode: %dx%d", info->HorizontalResolution, info->VerticalResolution);
    write_at(0, 6, "Current OS: %a (%a)", entry->name, entry->path);
    write_at(0, 7, "UEFI Version: %d.%d", (gST->Hdr.Revision >> 16u) & 0xFFFFu, gST->Hdr.Revision & 0xFFFFu);

    // options for what we can do
    // TODO: Change colors for the button
    write_at(0, 9, "Press B for BOOTMENU");
    write_at(0, 10, "Press S for SETUP");
    write_at(0, 11, "Press ESC for SHUTDOWN");

    /**
     * display the boot device path
     *
     * it seems that some UEFI implementations (Thinkpad x220) do not
     * support this protocol, so we will not assert on it but just
     * display a warning instead :shrug:
     */
    EFI_STATUS status;
    EFI_DEVICE_PATH_PROTOCOL* device_path = NULL;
    if(!EFI_ERROR(status = gBS->OpenProtocol(
            gImageHandle,
            &gEfiLoadedImageDevicePathProtocolGuid,
            (VOID**)&device_path,
            gImageHandle,
            NULL,
            EFI_OPEN_PROTOCOL_GET_PROTOCOL))) {

        EFI_DEVICE_PATH_TO_TEXT_PROTOCOL* devpath_to_text = NULL;
        if(!EFI_ERROR(status = gBS->LocateProtocol(&gEfiDevicePathToTextProtocolGuid, NULL, (VOID**)&devpath_to_text))) {
            CHAR16* devpath = devpath_to_text->ConvertDevicePathToText(device_path, TRUE, TRUE);
            write_at(0, 17, "%s", devpath);
            // TODO: How do I free the devpath?
        }else {
            write_at(0, 17, "Could not get EFI_DEVICE_PATH_TO_TEXT_PROTOCOL (Status=%r)", status);
        }
    }else {
        write_at(0, 17, "Could not get EFI_LOADED_IMAGE_DEVICE_PATH_PROTOCOL (Status=%r)", status);
    }


    // draw the logo
    draw_image(30 + ((width - 30) / 2) - 14, 1, tomato_image, 14, 13);
}

// dummy
VOID TimerHandler(IN EFI_EVENT Event, IN VOID* Context) {}

menu_t enter_main_menu(BOOLEAN first) {
    draw();
    ASSERT_EFI_ERROR(gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_RED, EFI_BLACK)));

    // read the config
    boot_config_t config;
    load_boot_config(&config);

    // create the timer event
    EFI_EVENT events[2] = { gST->ConIn->WaitForKey };
    ASSERT_EFI_ERROR(gBS->CreateEvent(EVT_TIMER | EVT_NOTIFY_WAIT, TPL_CALLBACK, TimerHandler, TimerHandler, &events[1]));

    if(first) {
        ASSERT_EFI_ERROR(gBS->SetTimer(events[1], TimerRelative, config.boot_delay * 10000000));
    }

    UINTN count = 2;
    do {
        // get key press
        UINTN which = 0;
        EFI_INPUT_KEY key = {};

        ASSERT_EFI_ERROR(gBS->WaitForEvent(count, events, &which));

        // got a keypress
        if(which == 0) {
            // cancel timer and destroy it
            if(count == 2) {
                ASSERT_EFI_ERROR(gBS->SetTimer(events[1], TimerCancel, 0));
                ASSERT_EFI_ERROR(gBS->CloseEvent(events[1]));
                count = 1;
            }

            // get key
            ASSERT_EFI_ERROR(gST->ConIn->ReadKeyStroke(gST->ConIn, &key));

            // choose the menu or continue
            if(key.UnicodeChar == L'b' || key.UnicodeChar == L'B') {
                return MENU_BOOT_MENU;
            } else if(key.UnicodeChar == L's' || key.UnicodeChar == L'S') {
                return MENU_SETUP;
            } else if(key.ScanCode == SCAN_ESC) {
                return MENU_SHUTDOWN;
            }

        // got timeout
        }else {
            // close the event
            ASSERT_EFI_ERROR(gBS->CloseEvent(events[1]));

            // call the loader
            load_tboot_binary(&boot_entries.entries[config.default_os]);
        }
    } while(TRUE);
}
