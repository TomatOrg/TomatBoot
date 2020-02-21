#ifndef __MENUS_MENUS_H__
#define __MENUS_MENUS_H__

#include <ProcessorBind.h>

typedef enum _MENU {
    MENU_MAIN_MENU,
    MENU_BOOT_MENU,
    MENU_SETUP,
    MENU_SHUTDOWN,
} MENU;

void StartMenus();

#endif //__MENUS_MENUS_H__
