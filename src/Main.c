#include <Uefi.h>
#include <Util/Except.h>

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

EFI_STATUS EFIAPI DxeDebugLibDestructor(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable);

static void CallDestructor(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    WARN_ON_ERROR(DxeDebugLibDestructor(ImageHandle, SystemTable), "DebugLib destructor failed!");
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Entry point!
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EFI_STATUS EfiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    EFI_STATUS Status = EFI_SUCCESS;

    TRACE("Hello World!");

    CHECK_AND_RETHROW(CallConstructors(ImageHandle, SystemTable));

cleanup:
    CallDestructor(ImageHandle, SystemTable);
    return Status;
}
