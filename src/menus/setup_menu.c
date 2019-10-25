#include <Uefi.h>
#include <util/draw_utils.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
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

    // draw the initial menu
    draw();

    // for the different options
    UINTN boot_delay = 2;
    UINTN op = NO_OP;

    // for drawing
    UINTN controls_start = 3;
    UINTN control_line_start = 2;
    UINTN selected = 2;
    do {
        UINTN control_line = control_line_start;
        fill_box(2, 1, width - 22, height - 2, EFI_TEXT_ATTR(EFI_BLUE, EFI_LIGHTGRAY));

        /*
         * Boot delay: changes the amount of time it take to boot by default
         * min: 1 // just so you can have time to actually change shit
         * max: 30 (?)
         */
        IF_SELECTED({
            // check last button press
            if(op == OP_INC) {
                boot_delay++;
                if(boot_delay > 30) {
                    boot_delay = 30;
                }
            }else if(op == OP_DEC) {
                boot_delay--;
                if(boot_delay <= 0) {
                    boot_delay = 1;
                }
            }
        });
        write_at(controls_start, control_line++, "Boot Delay: %d", boot_delay);

        /*
         * Default os to load
         */
        IF_SELECTED({
        });
        write_at(controls_start, control_line++, "Default OS: %a", "NULL");

        // reset the op
        op = NO_OP;

        // get key press
        UINTN which = 0;
        EFI_INPUT_KEY key = {};
        ASSERT_EFI_ERROR(gBS->WaitForEvent(1, &gST->ConIn->WaitForKey, &which));
        ASSERT_EFI_ERROR(gST->ConIn->ReadKeyStroke(gST->ConIn, &key));

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
            // TODO: Save
            return MENU_MAIN_MENU;

        // quit, without saving
        }else if(key.ScanCode == SCAN_ESC) {
            return MENU_MAIN_MENU;
        }
    } while(TRUE);

}
