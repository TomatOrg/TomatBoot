#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#include "menus.h"
#include "main_menu.h"

void NORETURN start_menus() {
    menu_t current_menu = MENU_MAIN_MENU;

    while(TRUE) {
        // clea it
        gST->ConOut->ClearScreen(gST->ConOut);

        // choose the correct menu to display
        switch(current_menu) {
            case MENU_MAIN_MENU:
                current_menu = enter_main_menu();
                break;

            case MENU_BOOT_MENU:
                ASSERT(FALSE);
                break;

            case MENU_SETUP:
                ASSERT(FALSE);
                break;

            case MENU_SHUTDOWN:
                gRT->ResetSystem(EfiResetShutdown, 0, 0, "shutdown");
                break;
        }

    }
}
