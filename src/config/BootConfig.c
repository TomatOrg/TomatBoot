#include "BootConfig.h"

#include <util/Except.h>
#include <util/GfxUtils.h>

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/FileHandleLib.h>
#include <Library/UefiBootServicesTableLib.h>

void LoadBootConfig(BOOT_CONFIG* config) {
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* filesystem = NULL;
    ASSERT_EFI_ERROR(gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID**)&filesystem));

    EFI_FILE_PROTOCOL* root = NULL;
    ASSERT_EFI_ERROR(filesystem->OpenVolume(filesystem, &root));

    EFI_FILE_PROTOCOL* configFile = NULL;
    EFI_STATUS status = root->Open(root, &configFile, L"tomatboot.bin", EFI_FILE_MODE_READ, 0);

    // if not found just set default config
    if(status == EFI_NOT_FOUND) {
        config->BootDelay = 4;
        config->DefaultOS = 0;
        config->GfxMode = GetFirstGfxMode();

        // otherwise see if can read the config
    }else {
        ASSERT_EFI_ERROR(status);

        UINTN size = sizeof(BOOT_CONFIG);
        ASSERT_EFI_ERROR(FileHandleRead(configFile, &size, config));
        ASSERT_EFI_ERROR(FileHandleClose(configFile));
    }

    // verify config and change if needed
    if(config->BootDelay > 30 || config->BootDelay < 1) {
        config->BootDelay = 30;
    }

    // TODO: Verify graphics config?
    // TODO: Verify default os config

    // close everything
    ASSERT_EFI_ERROR(FileHandleClose(root));
}

void SaveBootConfig(BOOT_CONFIG* config) {
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* filesystem = NULL;
    ASSERT_EFI_ERROR(gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID**)&filesystem));

    EFI_FILE_PROTOCOL* root = NULL;
    ASSERT_EFI_ERROR(filesystem->OpenVolume(filesystem, &root));

    EFI_FILE_PROTOCOL* configFile = NULL;
    ASSERT_EFI_ERROR(root->Open(root, &configFile, L"tomatboot.bin", EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0));

    UINTN size = sizeof(*config);
    ASSERT_EFI_ERROR(FileHandleWrite(configFile, &size, config));

    // close everything
    ASSERT_EFI_ERROR(FileHandleFlush(configFile));
    ASSERT_EFI_ERROR(FileHandleClose(configFile));
    ASSERT_EFI_ERROR(FileHandleClose(root));
}
