#ifndef __LOADERS_ELF_ELFLOADER_H__
#define __LOADERS_ELF_ELFLOADER_H__

#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>

typedef struct _ELF_INFO {
    // will subtract this value from the Virtual address
    // if zero then physical address is used
    UINT64 VirtualOffset;
    EFI_PHYSICAL_ADDRESS PhysicalBase;

    // The entry of the image
    UINTN Entry;

    // section entry info
    void* SectionHeaders;
    UINTN SectionHeadersSize;
    UINTN SectionEntrySize;
    UINTN StringSectionIndex;
} ELF_INFO;

EFI_STATUS LoadElf32(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs, CHAR16* file, ELF_INFO* info);

EFI_STATUS LoadElf64(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs, CHAR16* file, ELF_INFO* info);

#endif //__LOADERS_ELF_ELFLOADER_H__
