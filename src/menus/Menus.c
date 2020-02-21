#include <Uefi.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include "Menus.h"

MENU EnterMainMenu(BOOLEAN first);
MENU EnterSetupMenu();
MENU EnterBootMenu();

void StartMenus() {
    MENU current_menu = MENU_MAIN_MENU;
    BOOLEAN first = TRUE;

    while(TRUE) {
        // choose the correct menu to display
        switch(current_menu) {
            case MENU_MAIN_MENU:
                current_menu = EnterMainMenu(first);
                break;

            case MENU_BOOT_MENU:
                current_menu = EnterBootMenu();
                break;

            case MENU_SETUP:
                current_menu = EnterSetupMenu();
                break;

            case MENU_SHUTDOWN:
                gRT->ResetSystem(EfiResetShutdown, 0, 0, "shutdown");
                break;
        }

        first = FALSE;
    }

}
