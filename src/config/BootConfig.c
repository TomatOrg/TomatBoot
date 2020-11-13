#include "BootConfig.h"

#include <util/Except.h>
#include <util/GfxUtils.h>

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/FileHandleLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/LoadedImage.h>
#include <Library/UefiRuntimeServicesTableLib.h>

static EFI_GUID gTomatBootConfigGuid = { 0x2714d689, 0x1da6, 0x49d3,
                                  { 0x9b, 0x82, 0xa9, 0xdf, 0x7a, 0xe1, 0xb8, 0x25 } };

static CHAR16* gTomatBootConfigName = L"TomatBoot";

BOOT_CONFIG gBootConfigOverride = {
    .BootDelay = -1,
    .DefaultOS = -1,
    .GfxMode = -1
};

void LoadBootConfig(BOOT_CONFIG* config) {
    UINT32 Attributes = 0;
    UINTN Size = sizeof(BOOT_CONFIG);

    EFI_STATUS Status = gRT->GetVariable(gTomatBootConfigName, &gTomatBootConfigGuid, &Attributes, &Size, config);
    if(Status == EFI_NOT_FOUND) {
        config->BootDelay = 4;
        config->DefaultOS = 0;
        config->GfxMode = GetFirstGfxMode();

        SaveBootConfig(config);
    } else {
        ASSERT_EFI_ERROR(Status);

        // TODO: verify the boot config and if is invalid just reset it
    }
}

void SaveBootConfig(BOOT_CONFIG* config) {
    ASSERT_EFI_ERROR(gRT->SetVariable(gTomatBootConfigName, &gTomatBootConfigGuid,
            EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS, sizeof(BOOT_CONFIG), config));
}
