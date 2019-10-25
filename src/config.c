#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>
#include <Library/DebugLib.h>
#include <util/gop_utils.h>

#include "config.h"

boot_entries_t* get_boot_entries() {
    ASSERT(FALSE);
    return NULL;
}

void free_boot_entries(boot_entries_t* config) {

}

void load_boot_config(boot_config_t* config) {
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* filesystem = NULL;
    ASSERT_EFI_ERROR(gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID**)&filesystem));

    EFI_FILE_PROTOCOL* root = NULL;
    ASSERT_EFI_ERROR(filesystem->OpenVolume(filesystem, &root));

    EFI_FILE_PROTOCOL* configFile = NULL;
    EFI_STATUS status = root->Open(root, &configFile, L"tomatboot.config.bin", EFI_FILE_MODE_READ, 0);

    // if not found just set default config
    if(status == EFI_NOT_FOUND) {
        config->boot_delay = 4;
        config->default_os = 0;
        config->gfx_mode = get_prev_mode(0); // get the highest resolution possible

    // otherwise see if can read the config
    }else {
        ASSERT_EFI_ERROR(status);

        UINTN size = sizeof(boot_config_t);
        ASSERT_EFI_ERROR(configFile->Read(configFile, &size, config));
        ASSERT_EFI_ERROR(configFile->Close(configFile));

        DebugPrint(0, "%d", config->gfx_mode);
    }

    // verify config and change if needed
    if(config->boot_delay > 30 || config->boot_delay < 1) {
        config->boot_delay = 30;
    }

    // TODO: Verify graphics config?
    // TODO: Verify default os config

    // close everything
    ASSERT_EFI_ERROR(root->Close(root));
}

void save_boot_config(boot_config_t* config) {
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* filesystem = NULL;
    ASSERT_EFI_ERROR(gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID**)&filesystem));

    EFI_FILE_PROTOCOL* root = NULL;
    ASSERT_EFI_ERROR(filesystem->OpenVolume(filesystem, &root));

    EFI_FILE_PROTOCOL* configFile = NULL;
    ASSERT_EFI_ERROR(root->Open(root, &configFile, L"tomatboot.config.bin", EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0));

    UINTN size = sizeof(boot_config_t);
    ASSERT_EFI_ERROR(configFile->Write(configFile, &size, config));

    // close everything
    ASSERT_EFI_ERROR(configFile->Flush(configFile));
    ASSERT_EFI_ERROR(configFile->Close(configFile));
    ASSERT_EFI_ERROR(root->Close(root));
}
