#include <Library/BaseLib.h>
#include <Library/UefiRuntimeLib.h>
#include "Menus.h"
#include "MainMenu.h"

void StartMenus() {
    MENU CurrentMenu = MENU_MAIN_MENU;
    BOOLEAN First = TRUE;

    while(TRUE) {
        // choose the correct menu to display
        switch(CurrentMenu) {
            case MENU_MAIN_MENU:
                CurrentMenu = EnterMainMenu(First);
                break;

            case MENU_BOOT_MENU:
//                current_menu = EnterBootMenu();
                break;
//
            case MENU_SETUP:
//                current_menu = EnterSetupMenu();
                break;

            case MENU_SHUTDOWN:
                EfiResetSystem(EfiResetShutdown, EFI_SUCCESS, sizeof("shutdown"), "shutdown");
                CpuDeadLoop();
                break;
        }

        First = FALSE;
    }

}
