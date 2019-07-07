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
    boot_config_t* config = NULL;
    boot_config_t header;

    // get the root of the file system
    EFI_GUID sfpGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* sfp = 0;
    gBS->LocateProtocol(&sfpGuid, 0, (VOID**)&sfp);
    EFI_FILE_PROTOCOL* rootDir = 0;
	sfp->OpenVolume(sfp, &rootDir);

    // open the config file
    EFI_FILE_PROTOCOL* file = OpenFile(rootDir, L"kbootcfg.bin", EFI_FILE_MODE_READ, 0);
    if(file == NULL) goto cleanup;

    // read the header (so we know how much more need)
    size_t to_read = sizeof(boot_config_t);
    ReadFile(file, &to_read, &header);

    // allocate enough space for everything and read it
    size_t total_size = sizeof(boot_config_t) + sizeof(boot_entry_t) * (header.entry_count + 2);
    config = malloc(total_size);
    ReadFile(file, &total_size, config);

    // add reset and shutdown options
    config->entries[config->entry_count].size = sizeof(boot_entry_t);
    memcpy(config->entries[config->entry_count].name, L"Reset", sizeof(L"Reset"));
    memcpy(config->entries[config->entry_count].filename, L"shutdown.elf", sizeof(L"shutdown.elf"));
    memcpy(config->entries[config->entry_count].command_line, L"reset", sizeof(L"reset"));
    config->entry_count++;

    config->entries[config->entry_count].size = sizeof(boot_entry_t);
    memcpy(config->entries[config->entry_count].name, L"Shutdown", sizeof(L"Shutdown"));
    memcpy(config->entries[config->entry_count].filename, L"shutdown.elf", sizeof(L"shutdown.elf"));
    memcpy(config->entries[config->entry_count].command_line, L"DOESN'T WORK", sizeof(L"DOESN'T WORK"));
    config->entry_count++;

cleanup:
    if(file) CloseFile(file);
    if(rootDir) CloseFile(rootDir);
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
    if(config == NULL) while(1);
    boot_entry_t* entry = start_menu(config);
    if(entry == NULL) while(1);
    load_kernel(entry);
    while(1);

    return EFI_SUCCESS;
}
