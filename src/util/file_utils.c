#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Protocol/SimpleFileSystem.h>
#include <Guid/FileInfo.h>
#include "file_utils.h"

void load_file(const CHAR8* path, int memory_type, UINT64* out_base, UINT64* out_len) {
    // convert to unicode so we can open it
    CHAR16* unicode = NULL;
    UINTN len = AsciiStrLen(path) + 1;
    ASSERT_EFI_ERROR(gBS->AllocatePool(EfiBootServicesData, len * 2 + 2, (VOID**)&unicode));
    AsciiStrnToUnicodeStrS(path, len, unicode, len + 1, &len);

    // open the file
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* filesystem = NULL;
    ASSERT_EFI_ERROR(gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID**)&filesystem));

    EFI_FILE_PROTOCOL* root = NULL;
    ASSERT_EFI_ERROR(filesystem->OpenVolume(filesystem, &root));

    EFI_FILE_PROTOCOL* file = NULL;
    ASSERT_EFI_ERROR(root->Open(root, &file, unicode, EFI_FILE_MODE_READ, 0));
    ASSERT_EFI_ERROR(gBS->FreePool(unicode));

    // get the file size
    UINTN file_info_size = SIZE_OF_EFI_FILE_INFO + len * 2 + 2;
    EFI_FILE_INFO* file_info = NULL;
    ASSERT_EFI_ERROR(gBS->AllocatePool(EfiBootServicesData, file_info_size, (void*)&file_info));
    file_info->Size = file_info_size;
    ASSERT_EFI_ERROR(file->GetInfo(file, &gEfiFileInfoGuid, &file_info_size, file_info));
    *out_len = file_info->FileSize;
    ASSERT_EFI_ERROR(gBS->FreePool(file_info));

    // allocate the buffer
    ASSERT_EFI_ERROR(gBS->AllocatePages(AllocateAnyPages, memory_type, EFI_SIZE_TO_PAGES(*out_len), out_base));

    // read it
    ASSERT_EFI_ERROR(file->Read(file, out_len, (void*)*out_base));

    ASSERT_EFI_ERROR(file->Close(file));
    ASSERT_EFI_ERROR(root->Close(root));
}

