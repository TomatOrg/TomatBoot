#ifndef __LOADERS_ELF_ELFLOADER_H__
#define __LOADERS_ELF_ELFLOADER_H__

#include <Uefi.h>

typedef struct _ELF_INFO {
    // The base to laod the image at
    UINTN LoadBase;

    // The entry of the image
    UINTN Entry;

    // section entry info
    void* SectionHeaders;
    UINTN SectionHeadersSize;
    UINTN SectionEntrySize;
    UINTN StringSectionIndex;
} ELF_INFO;

EFI_STATUS LoadElf32(CHAR16* file, ELF_INFO* info);

EFI_STATUS LoadElf64(CHAR16* file, ELF_INFO* info);

#endif //__LOADERS_ELF_ELFLOADER_H__
