#ifndef KRETLIM_UEFI_BOOT_MENU_H
#define KRETLIM_UEFI_BOOT_MENU_H

#include "config.h"

extern size_t text_width;
extern size_t text_height;

boot_entry_t* start_menu(boot_config_t* config);

#endif