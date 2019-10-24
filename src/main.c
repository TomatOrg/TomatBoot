#include <Uefi.h>
#include <Library/BaseLib.h>

EFI_HANDLE gImageHandle;
EFI_SYSTEM_TABLE* gST;
EFI_RUNTIME_SERVICES* gRT;
EFI_BOOT_SERVICES* gBS;

EFI_STATUS EFIAPI EfiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS status = EFI_SUCCESS;

    gImageHandle = ImageHandle;
    gST = SystemTable;
    gRT = gST->RuntimeServices;
    gBS = gST->BootServices;

    return status;
}
