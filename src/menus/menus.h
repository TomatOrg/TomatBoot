#ifndef TOMATBOOT_UEFI_MENUS_H
#define TOMATBOOT_UEFI_MENUS_H

#include <Uefi/UefiBaseType.h>

typedef enum menu {
    MENU_MAIN_MENU,
    MENU_BOOT_MENU,
    MENU_SETUP,
    MENU_SHUTDOWN,
} menu_t;

void start_menus();

#endif //TOMATBOOT_UEFI_MENUS_H
