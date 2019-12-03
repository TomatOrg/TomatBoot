#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Protocol/Cpu.h>

#include <menus/menus.h>
#include <elf64/elf64.h>
#include "config.h"
#include <debug/exception.h>

EFI_HANDLE gImageHandle;
EFI_SYSTEM_TABLE* gST;
EFI_RUNTIME_SERVICES* gRT;
EFI_BOOT_SERVICES* gBS;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The entry
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EFI_STATUS EFIAPI EfiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
    // setup some stuff
    gImageHandle = ImageHandle;
    gST = SystemTable;
    gRT = gST->RuntimeServices;
    gBS = gST->BootServices;

    ASSERT_EFI_ERROR(gBS->SetWatchdogTimer(0, 0, 0, NULL));

    // setup all of the libraries
    DebugLibConstructor(ImageHandle, SystemTable);

    // load the boot config
    get_boot_entries(&boot_entries);

    // start the menu thingy
    start_menus();

    // failed to load the kernel
    return EFI_LOAD_ERROR;
}
