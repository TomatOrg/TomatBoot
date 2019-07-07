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
    boot_config_t* config = malloc(sizeof(boot_config_t));

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
    ReadFile(file, &to_read, config);

    // allocate enough space for everything and read it
    printf(L"Found config file (%d entries)\n\r", config->entry_count);
    size_t total_size = sizeof(boot_config_t) + sizeof(boot_entry_t) * (config->entry_count);
    config = realloc(config, total_size + (sizeof(boot_entry_t) * 2));
    SetPosition(file, 0);
    ReadFile(file, &total_size, config);

    boot_entry_t* entry = config->entries;
    for(int i = 0; i < config->entry_count; i++) {
        printf(L"%a -> %a\n\r", entry->name, entry->filename);
        entry = (boot_entry_t*)(((size_t)entry) + entry->size);
    }

    // add reset and shutdown options
    // TODO: Will need to do proper iteration
    printf(L"Adding shutdown/reset options\n\r");
    config->entries[config->entry_count].size = sizeof(boot_entry_t);
    memcpy(config->entries[config->entry_count].name, "Reset", sizeof("Reset"));
    memcpy(config->entries[config->entry_count].filename, "shutdown.elf", sizeof("shutdown.elf"));
    memcpy(config->entries[config->entry_count].command_line, "reset", sizeof("reset"));
    config->entry_count++;

    config->entries[config->entry_count].size = sizeof(boot_entry_t);
    memcpy(config->entries[config->entry_count].name, "Shutdown", sizeof("Shutdown"));
    memcpy(config->entries[config->entry_count].filename, "shutdown.elf", sizeof("shutdown.elf"));
    memcpy(config->entries[config->entry_count].command_line, "DOESN'T WORK", sizeof("DOESN'T WORK"));
    config->entry_count++;

cleanup:
    if(file) CloseFile(file);
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
    if(config == NULL) {
        printf(L"Error getting the config\n\r");
        while(1);
    }

    printf(L"Starting menu\n\r");
    boot_entry_t* entry = start_menu(config);
    if(entry == NULL) {
        printf(L"Error getting the entry\n\r");
        while(1);
    }

    load_kernel(entry);
    while(1);

    return EFI_SUCCESS;
}
