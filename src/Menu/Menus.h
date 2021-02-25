#ifndef __TOMATBOOT_MENUS_H__
#define __TOMATBOOT_MENUS_H__

#include <Uefi.h>

typedef enum _MENU {
    MENU_MAIN_MENU,
    MENU_BOOT_MENU,
    MENU_EDIT,
} MENU;

void StartMenus();

#endif //__TOMATBOOT_MENUS_H__
