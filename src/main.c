#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

#include <menus/menus.h>
#include <elf64/elf64.h>
#include "config.h"
#include "debug.h"

EFI_HANDLE gImageHandle;
EFI_SYSTEM_TABLE* gST;
EFI_RUNTIME_SERVICES* gRT;
EFI_BOOT_SERVICES* gBS;

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Exception handlers
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// The entry
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EFI_STATUS EFIAPI EfiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
    // setup some stuff
    gImageHandle = ImageHandle;
    gST = SystemTable;
    gRT = gST->RuntimeServices;
    gBS = gST->BootServices;

    // disable the watchdog timer
    ASSERT_EFI_ERROR(gBS->SetWatchdogTimer(0, 0, 0, NULL));

    // setup all of the libraries
    DebugLibConstructor(ImageHandle, SystemTable);
    setup_exception_handlers();

    // load the boot config
    if(EFI_ERROR(get_boot_entries(&boot_entries))) {
        // if we get an error open an editor to edit the boot entries

        gRT->ResetSystem(EfiResetWarm, EFI_SUCCESS, 0, NULL);
    }else {
        // start the menu thingy
        start_menus();
    }

    // failed to load the kernel
    return EFI_LOAD_ERROR;
}
