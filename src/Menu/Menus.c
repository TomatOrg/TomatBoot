#include <Library/BaseLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Util/Except.h>
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
            case MENU_EDIT:
//                current_menu = EnterSetupMenu();
                TRACE("Main menu");
                break;
        }

        First = FALSE;
    }

}
