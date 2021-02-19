#include <Uefi.h>
#include <Util/Except.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Drivers/ExtFs.h>
#include "Config/Config.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// constructors
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EFI_STATUS EFIAPI UefiBootServicesTableLibConstructor(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable);
EFI_STATUS EFIAPI DxeDebugLibConstructor(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable);

static EFI_STATUS CallConstructors(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    EFI_STATUS Status = EFI_SUCCESS;

    CHECK_AND_RETHROW(UefiBootServicesTableLibConstructor(ImageHandle, SystemTable));
    CHECK_AND_RETHROW(DxeDebugLibConstructor(ImageHandle, SystemTable));

cleanup:
    return Status;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Entry point!
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EFI_STATUS EfiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    EFI_STATUS Status = EFI_SUCCESS;

    // setup nicely
    CHECK_AND_RETHROW(CallConstructors(ImageHandle, SystemTable));
    EFI_CHECK(SystemTable->ConOut->ClearScreen(SystemTable->ConOut));

    TRACE("Registering drivers and connecting them...");
//    CHECK_AND_RETHROW(LoadExtFs());
//    EFI_CHECK(gBS->ConnectController(gImageHandle, NULL, NULL, TRUE));

    TRACE("Reading the Config...");
    CHECK_AND_RETHROW(ParseConfig());

cleanup:
    while(1);
}
