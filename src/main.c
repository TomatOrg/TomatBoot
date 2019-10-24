#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

EFI_HANDLE gImageHandle;
EFI_SYSTEM_TABLE* gST;
EFI_RUNTIME_SERVICES* gRT;
EFI_BOOT_SERVICES* gBS;

EFI_STATUS EFIAPI EfiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS status = EFI_SUCCESS;

    // setup some stuff
    gImageHandle = ImageHandle;
    gST = SystemTable;
    gRT = gST->RuntimeServices;
    gBS = gST->BootServices;

    // setup all of the libraries
    DebugLibConstructor(ImageHandle, SystemTable);

    SystemTable->ConOut->ClearScreen(SystemTable->ConOut);
    DebugPrint(0, "Hello world!");


    CpuBreakpoint();

    return status;
}
