#include <uefi/uefi.h>
#include <kboot/kboot.h>

#include <stddef.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include <kboot/map.h>
#include <kboot/buf.h>

#include "menu.h"
#include "loader.h"

static boot_config_t* get_boot_config() {
    // TODO load from the file

    boot_config_t* config = calloc(sizeof(boot_config_t) + sizeof(boot_entry_t) * 2, 1);
    config->default_option = 0;
    config->entry_count = 1;
    config->entries[0].size = sizeof(boot_entry_t);
    memcpy(config->entries[0].name, L"Reset", sizeof(L"Reset"));
    memcpy(config->entries[0].filename, L"shutdown.elf", sizeof(L"shutdown.elf"));
    memcpy(config->entries[0].command_line, L"reset", sizeof(L"reset"));

    return config;
}

/**
 * Most of the implementation is here
 */
EFI_STATUS EFIAPI EfiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
    gST = SystemTable;
    gBS = SystemTable->BootServices;
    gImageHandle = ImageHandle;
    
    boot_config_t* config = get_boot_config();
    boot_entry_t* entry = start_menu(config);
    load_kernel(entry);

    return EFI_SUCCESS;
}
