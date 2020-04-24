#include <util/FileUtils.h>
#include <Library/FileHandleLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include "Loaders.h"

EFI_STATUS LoadBootModule(BOOT_MODULE* Module, UINTN* Base, UINTN* Size) {
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* filesystem = NULL;
    EFI_FILE_PROTOCOL* root = NULL;
    EFI_FILE_PROTOCOL* moduleImage = NULL;

    // open the executable file
    EFI_CHECK(gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (void**)&filesystem));
    EFI_CHECK(filesystem->OpenVolume(filesystem, &root));
    EFI_CHECK(root->Open(root, &moduleImage, Module->Path, EFI_FILE_MODE_READ, 0));

    // read it all
    *Base = BASE_4GB;
    EFI_CHECK(FileHandleGetSize(moduleImage, Size));
    EFI_CHECK(gBS->AllocatePages(AllocateMaxAddress, EfiRuntimeServicesData, EFI_SIZE_TO_PAGES(*Size), Base));
    CHECK_AND_RETHROW(FileRead(moduleImage, (void*)*Base, *Size, 0));

cleanup:
    // TODO: Free if failed on reading
    return Status;
}

EFI_STATUS LoadKernel(BOOT_ENTRY* Entry) {
    EFI_STATUS Status = EFI_SUCCESS;

    switch (Entry->Protocol) {
        case BOOT_MB2:
            CHECK_AND_RETHROW(LoadMB2Kernel(Entry));
            break;

        case BOOT_LINUX:
            CHECK_AND_RETHROW(LoadLinuxKernel(Entry));
            break;

        case BOOT_STIVALE:
            CHECK_AND_RETHROW(LoadStivaleKernel(Entry));
            break;
    }

cleanup:
    return Status;
}
