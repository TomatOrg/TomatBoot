#include <Library/DebugLib.h>

#include "loader.h"

void load_linux_binary(boot_entry_t* entry);
void load_tboot_binary(boot_entry_t* entry);
void load_uefi_binary(boot_entry_t* entry);
void load_mb2_binary(boot_entry_t* entry);

static void(*loaders[])(boot_entry_t* entry) = {
    [BOOT_LINUX] = load_linux_binary,
    [BOOT_TBOOT] = load_tboot_binary,
    [BOOT_UEFI] = load_uefi_binary,
    [BOOT_MB2] = load_mb2_binary,
};

void load_binary(boot_entry_t* entry) {
    // clear everything, this is going to be a simple log of how we loaded the stuff
    ASSERT_EFI_ERROR(gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK)));
    ASSERT_EFI_ERROR(gST->ConOut->SetCursorPosition(gST->ConOut, 0, 0));
    ASSERT_EFI_ERROR(gST->ConOut->ClearScreen(gST->ConOut));

    ASSERT(ARRAY_SIZE(loaders) > entry->protocol);
    loaders[entry->protocol](entry);
}
