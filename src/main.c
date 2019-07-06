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
    config->entry_count = 2;
    config->entries[0].size = sizeof(boot_entry_t);
    memcpy(config->entries[0].name, L"Kretlim", sizeof(L"Kretlim"));
    memcpy(config->entries[0].filename, L"kretlim.elf", sizeof(L"kretlim.elf"));
    memcpy(config->entries[0].command_line, L"", sizeof(L""));

    config->entries[1].size = sizeof(boot_entry_t);
    memcpy(config->entries[1].name, L"Kretlim Debug", sizeof(L"Kretlim Debug"));
    memcpy(config->entries[1].filename, L"kretlim.elf", sizeof(L"kretlim.elf"));
    memcpy(config->entries[1].command_line, L"logger=debug", sizeof(L"logger=debug"));
    return config;
}

/**
 * Most of the implementation is here
 */
EFI_STATUS EFIAPI EfiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
    gST = SystemTable;
    gBS = SystemTable->BootServices;
    gImageHandle = ImageHandle;

    // read the boot config
    boot_config_t* config = get_boot_config();
    boot_entry_t* entry = start_menu(config);

    gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK));
    gST->ConOut->ClearScreen(gST->ConOut);
    printf(L"Booting `%s` (%s)\n\r", entry->name, entry->filename);
    printf(L"Command line: `%s`\n\r", entry->command_line);
    // TODO: Boot modules
    
    while(1);

    return EFI_SUCCESS;
}
