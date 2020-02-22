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
    void* Buffer = NULL;

    // open the executable file
    EFI_CHECK(gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (void**)&filesystem));
    EFI_CHECK(filesystem->OpenVolume(filesystem, &root));
    EFI_CHECK(root->Open(root, &moduleImage, Module->Path, EFI_FILE_MODE_READ, 0));

    // read it all
    EFI_CHECK(FileHandleGetSize(moduleImage, Size));
    Buffer = AllocatePages(EFI_SIZE_TO_PAGES(*Size));
    CHECK_AND_RETHROW(FileRead(moduleImage, Buffer, *Size, 0));

cleanup:
    if (EFI_ERROR(Status)) {
        if (Buffer != NULL) {
            FreePool(Buffer);
        }
    }
    return Status;
}

EFI_STATUS LoadKernel(BOOT_ENTRY* Entry) {
    EFI_STATUS Status = EFI_SUCCESS;

    switch (Entry->Protocol) {
        case BOOT_MB2:
            CHECK_AND_RETHROW(LoadMB2(Entry));
            break;

        case BOOT_LINUX:
            CHECK_FAIL();
            break;

        case BOOT_TBOOT:
            CHECK_FAIL();
            break;
    }

cleanup:
    return Status;
}
