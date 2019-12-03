#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Protocol/GraphicsOutput.h>
#include <debug/exception.h>

#include <config.h>
#include <util/draw_utils.h>
#include <util/gop_utils.h>

#include "setup_menu.h"

static void draw() {
    UINTN width = 0;
    UINTN height = 0;
    ASSERT_EFI_ERROR(gST->ConOut->QueryMode(gST->ConOut, gST->ConOut->Mode->Mode, &width, &height));

    // draw the frame
    clear_screen(EFI_BACKGROUND_BLUE);

    gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLUE));
    write_at(width / 2 - AsciiStrLen("BOOT SETUP") / 2, 0, "BOOT SETUP");

    // draw the controls
    UINTN controls_start = width - 18;
    gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLUE));

    write_at(controls_start, 2, "Press [-] to");
    write_at(controls_start, 3, "decrease value");

    write_at(controls_start, 5, "Press [+] to");
    write_at(controls_start, 6, "increase value");

    write_at(controls_start, 8, "Press ENTER");
    write_at(controls_start, 9, "to save and exit");

    write_at(controls_start, 11, "Press ESC to");
    write_at(controls_start, 12, "quit");
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

menu_t enter_setup_menu() {
    UINTN width = 0;
    UINTN height = 0;
    ASSERT_EFI_ERROR(gST->ConOut->QueryMode(gST->ConOut, gST->ConOut->Mode->Mode, &width, &height));

    // get GOP so we can query the resolutions
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;
    ASSERT_EFI_ERROR(gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID**)&gop));

    // draw the initial menu
    draw();

    // for the different options
    boot_config_t config = {};
    load_boot_config(&config);
    UINTN op = NO_OP;

    // for drawing
    UINTN controls_start = 3;
    UINTN control_line_start = 2;
    UINTN selected = 2;
    do {
        UINTN control_line = control_line_start;
        fill_box(2, 1, width - 22, height - 2, EFI_TEXT_ATTR(EFI_BLUE, EFI_LIGHTGRAY));

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
                config.boot_delay++;
                if(config.boot_delay > 30) {
                    config.boot_delay = 30;
                }
            }else if(op == OP_DEC) {
                config.boot_delay--;
                if(config.boot_delay <= 0) {
                    config.boot_delay = 1;
                }
            }
        });
        write_at(controls_start, control_line++, "Boot Delay: %d", config.boot_delay);

        /*
         * Graphics Mode: changes the resolution and format of the graphics buffer
         */
        IF_SELECTED({
            if(op == OP_INC) {
                config.gfx_mode = get_next_mode(config.gfx_mode);
            }else if(op == OP_DEC) {
                config.gfx_mode = get_prev_mode(config.gfx_mode);
            }
        });
        EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* info = NULL;
        UINTN sizeOfInfo = sizeof(EFI_GRAPHICS_OUTPUT_MODE_INFORMATION);
        ASSERT_EFI_ERROR(gop->QueryMode(gop, config.gfx_mode, &sizeOfInfo, &info));
        write_at(controls_start, control_line++, "Graphics Mode: %dx%d (BGRA8)", info->HorizontalResolution, info->VerticalResolution);

        /*
         * Default os to load
         */
        IF_SELECTED({
            // check last button press
            if(op == OP_INC) {
                config.default_os++;
                if(config.default_os >= boot_entries.count) {
                    config.default_os = 0;
                }
            }else if(op == OP_DEC) {
                config.default_os--;
                if(config.default_os < 0) {
                    config.default_os = (UINT32)boot_entries.count - 1;
                }
            }
        });
        boot_entry_t* entry = &boot_entries.entries[config.default_os];
        write_at(controls_start, control_line++, "Default OS: %a (%a)", entry->name, entry->path);

        #ifdef TBOOT_DEBUG
        IF_SELECTED({
            // check last button press
            if(op == OP_INC) {
                hook_idt();
            }else if(op == OP_DEC) {
                unhook_idt();
            }
        });
        write_at(controls_start, control_line++, "Hook IDT", config.boot_delay);
        #endif


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
            save_boot_config(&config);
            return MENU_MAIN_MENU;

        // quit, without saving
        }else if(key.ScanCode == SCAN_ESC) {
            return MENU_MAIN_MENU;
        }
    } while(TRUE);

}
