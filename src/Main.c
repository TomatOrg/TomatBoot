#include <Uefi.h>
#include <Util/Except.h>
#include "Config.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// constructors
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EFI_STATUS EFIAPI DxeDebugLibConstructor(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable);

static EFI_STATUS CallConstructors(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    EFI_STATUS Status = EFI_SUCCESS;

    CHECK_AND_RETHROW(DxeDebugLibConstructor(ImageHandle, SystemTable));

cleanup:
    return Status;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Entry point!
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EFI_STATUS EfiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    EFI_STATUS Status = EFI_SUCCESS;

    TRACE("Hello World!");

    TRACE("Calling edk2 constructors...");
    CHECK_AND_RETHROW(CallConstructors(ImageHandle, SystemTable));

    TRACE("Reading the config...");
    CHECK_AND_RETHROW(ParseConfig());

cleanup:
    while(1);
}
