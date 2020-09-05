#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiLib.h>
#include <Library/CpuLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <config/BootEntries.h>
#include <util/Except.h>
#include <config/BootConfig.h>
#include <menus/Menus.h>

// define all constructors
extern EFI_STATUS EFIAPI UefiBootServicesTableLibConstructor(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable);
extern EFI_STATUS EFIAPI DxeDebugLibConstructor(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable);
extern EFI_STATUS EFIAPI UefiRuntimeServicesTableLibConstructor (IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE* SystemTable);

/**
 * The entry of the os
 */
EFI_STATUS EFIAPI EfiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable) {
    EFI_STATUS Status = EFI_SUCCESS;

    // Call constructors
    EFI_CHECK(DxeDebugLibConstructor(ImageHandle, SystemTable));
    EFI_CHECK(UefiBootServicesTableLibConstructor(ImageHandle, SystemTable));
    EFI_CHECK(UefiRuntimeServicesTableLibConstructor(ImageHandle, SystemTable));

    // make sure we got everything nice and dandy
    CHECK(gST != NULL);
    CHECK(gBS != NULL);
    CHECK(gRT != NULL);

    // disable the watchdog timer
    EFI_CHECK(gST->BootServices->SetWatchdogTimer(0, 0, 0, NULL));

    // just a signature that we booted
    EFI_CHECK(gST->ConOut->ClearScreen(gST->ConOut));
    Print(L"Hello World!\n\n\n");

    // Load the boot configs and set the default one
    BOOT_CONFIG config;
    LoadBootConfig(&config);
    CHECK_AND_RETHROW(GetBootEntries(&gBootEntries));
    gDefaultEntry = GetBootEntryAt(config.DefaultOS);

    // we are ready to do shit :yay:
    StartMenus();

cleanup:
    if (EFI_ERROR(Status)) {
        ASSERT_EFI_ERROR(Status);
    }

    while(1) CpuSleep();

    return EFI_SUCCESS;
}
