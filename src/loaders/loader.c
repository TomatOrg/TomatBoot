#include <Library/DebugLib.h>

#include "linux_loader.h"
#include "tboot_loader.h"
#include "uefi_loader.h"

#include "loader.h"

void load_linux_binary(boot_entry_t* entry);
void load_tboot_binary(boot_entry_t* entry);
void load_uefi_binary(boot_entry_t* entry);

static void(*loaders[])(boot_entry_t* entry) = {
    [BOOT_LINUX] = load_linux_binary,
    [BOOT_TBOOT] = load_tboot_binary,
    [BOOT_UEFI] = load_uefi_binary,
};

void load_binary(boot_entry_t* entry) {
    ASSERT(ARRAY_SIZE(loaders) > entry->protocol);
    loaders[entry->protocol](entry);
}
