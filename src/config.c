#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>
#include <Library/DebugLib.h>

#include "config.h"

Config* ParseConfig() {
    Config* config = NULL;
    ASSERT_EFI_ERROR(gBS->AllocatePool(EfiBootServicesData, sizeof(*config), (VOID**)&config));

    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* filesystem = NULL;
    ASSERT_EFI_ERROR(gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID**)&filesystem));

    EFI_FILE_PROTOCOL* root = NULL;
    ASSERT_EFI_ERROR(filesystem->OpenVolume(filesystem, &root));

    EFI_FILE_PROTOCOL* configFile = NULL;
    ASSERT_EFI_ERROR(root->Open(root, &configFile, L"tomatboot.cfg", EFI_FILE_MODE_READ, 0));

    return config;
}

void FreeConfig(Config* config) {
    ASSERT_EFI_ERROR(gBS->FreePool(config));
}
