#include "Menus.h"

#include <config/BootEntries.h>
#include <config/BootConfig.h>
#include <util/DrawUtils.h>
#include <util/GfxUtils.h>

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/GraphicsOutput.h>

static void draw() {
    UINTN width = 0;
    UINTN height = 0;
    ASSERT_EFI_ERROR(gST->ConOut->QueryMode(gST->ConOut, gST->ConOut->Mode->Mode, &width, &height));

    // draw the frame
    ClearScreen(EFI_BACKGROUND_BLUE);

    gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLUE));
    WriteAt(width / 2 - AsciiStrLen("BOOT SETUP") / 2, 0, "BOOT SETUP");

    // draw the controls
    UINTN controls_start = width - 18;
    gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLUE));

    WriteAt(controls_start, 2, "Press [-] to");
    WriteAt(controls_start, 3, "decrease value");

    WriteAt(controls_start, 5, "Press [+] to");
    WriteAt(controls_start, 6, "increase value");

    WriteAt(controls_start, 8, "Press ENTER");
    WriteAt(controls_start, 9, "to save and exit");

    WriteAt(controls_start, 11, "Press ESC to");
    WriteAt(controls_start, 12, "quit");
}

#define IF_SELECTED(...) \
    do { \
        if(selected == control_line) { \
            gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_LIGHTGRAY)); \
            __VA_ARGS__; \
        }else { \
            gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_BLUE, EFI_LIGHTGRAY)); \
        } \
    } while(0)

// different operations which could be selected
#define NO_OP (0)
#define OP_INC (1)
#define OP_DEC (2)

MENU EnterSetupMenu() {
    UINTN width = 0;
    UINTN height = 0;
    ASSERT_EFI_ERROR(gST->ConOut->QueryMode(gST->ConOut, gST->ConOut->Mode->Mode, &width, &height));

    // get GOP so we can query the resolutions
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;
    ASSERT_EFI_ERROR(gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID**)&gop));

    // draw the initial menu
    draw();

    // for the different options
    BOOT_CONFIG config = {};
    LoadBootConfig(&config);
    UINTN op = NO_OP;

    // for drawing
    UINTN controls_start = 3;
    UINTN control_line_start = 2;
    UINTN selected = 2;
    do {
        UINTN control_line = control_line_start;
        FillBox(2, 1, width - 22, height - 2, EFI_TEXT_ATTR(EFI_BLUE, EFI_LIGHTGRAY));

        ////////////////////////////////////////////////////////////////////////
        // Setup entries
        ////////////////////////////////////////////////////////////////////////

        /*
         * Boot delay: changes the amount of time it take to boot by default
         * min: 1 // just so you can have time to actually change shit
         * max: 30 (?)
         */
        IF_SELECTED({
            // check last button press
            if(op == OP_INC) {
                config.BootDelay++;
                if(config.BootDelay > 30) {
                    config.BootDelay = 30;
                }
            }else if(op == OP_DEC) {
                config.BootDelay--;
                if(config.BootDelay <= 0) {
                    config.BootDelay = 1;
                }
            }
        });
        WriteAt(controls_start, control_line++, "Boot Delay: %d", config.BootDelay);


        /*
         * Graphics Mode: changes the resolution and format of the graphics buffer
         */
        IF_SELECTED({
            if(op == OP_INC) {
                config.GfxMode = GetNextGfxMode(config.GfxMode);
            }else if(op == OP_DEC) {
                config.GfxMode = GetPrevGfxMode(config.GfxMode);
            }
        });
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info = NULL;
        UINTN sizeOfInfo = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
        ASSERT_EFI_ERROR(gop->QueryMode(gop, config.GfxMode, &sizeOfInfo, &info));
        WriteAt(controls_start, control_line++, "Graphics Mode: %dx%d (BGRA8)", info->HorizontalResolution, info->VerticalResolution);

        /*
         * Default os to load
         */
        IF_SELECTED({
            // check last button press
            if(op == OP_INC) {
                config.DefaultOS++;
            }else if(op == OP_DEC) {
                config.DefaultOS--;
                if (config.DefaultOS < 0) {
                    config.DefaultOS = 0;
                }
            }
        });
        BOOT_ENTRY* entry = GetBootEntryAt(config.DefaultOS);
        if (entry == NULL) {
            config.DefaultOS--;
            entry = GetBootEntryAt(config.DefaultOS);
            ASSERT(entry != NULL);
        }
        WriteAt(controls_start, control_line++, "Default OS: %s (%s)", entry->Name, entry->Path);

        ////////////////////////////////////////////////////////////////////////
        // Input handling
        ////////////////////////////////////////////////////////////////////////

        // reset the op
        op = NO_OP;

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
        if(key.UnicodeChar == L'-') {
            op = OP_DEC;

            // increase value
        }else if(key.UnicodeChar == L'+') {
            op = OP_INC;

            // next option
        }else if(key.ScanCode == SCAN_DOWN) {
            selected++;
            if(selected >= control_line) {
                selected = control_line_start;
            }

            // prev potion
        }else if(key.ScanCode == SCAN_UP) {
            selected--;
            if(selected < control_line_start) {
                selected = control_line - 1;
            }

            // save and exit
        }else if(key.UnicodeChar == CHAR_CARRIAGE_RETURN) {
            SaveBootConfig(&config);
            return MENU_MAIN_MENU;

            // quit, without saving
        }else if(key.ScanCode == SCAN_ESC) {
            return MENU_MAIN_MENU;
        }
    } while(TRUE);

}