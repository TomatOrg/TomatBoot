#ifndef __TOMATBOOT_ELFLOADER_H__
#define __TOMATBOOT_ELFLOADER_H__

#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>

typedef struct _ELF_INFO {
    // the entry point of the elf
    UINT64 EntryPoint;

    // the top of the loaded file
    EFI_PHYSICAL_ADDRESS Top;
} ELF_INFO;

/**
 * Load an elf binary
 *
 * @param File          [IN] The elf file
 * @param ElfInfo       [IN] The elf information
 * @param Slide         [IN] The kaslr slide
 * @param MemoryType    [IN] The memory type to allocate the sections as
 */
EFI_STATUS Elf64Load(EFI_FILE_PROTOCOL* File, ELF_INFO* ElfInfo, UINT64 Slide, EFI_MEMORY_TYPE MemoryType);

/**
 * Read a section from the elf file
 *
 * @param File          [IN]    The elf file handle
 * @param Buffer        [OUT]   The output buffer, free with FreePool
 * @param BufferSize    [OUT]   The buffer size
 * @param Name          [IN]    The name of the section
 * @param Slide         [IN]    The kaslr slide
 */
EFI_STATUS Elf64LoadSection(EFI_FILE_PROTOCOL* File, void** Buffer, UINTN* BufferSize, const char* Name, UINT64 Slide);


#endif //__TOMATBOOT_ELFLOADER_H__
