#include "ElfLoader.h"

#include <util/Except.h>
#include <util/FileUtils.h>

#include <Uefi.h>
#include <Library/FileHandleLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "elf64.h"

EFI_STATUS LoadElf64(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs, CHAR16* file, ELF_INFO* info) {
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_FILE_PROTOCOL* root = NULL;
    EFI_FILE_PROTOCOL* elfFile = NULL;

    // open the executable file
    EFI_CHECK(fs->OpenVolume(fs, &root));
    EFI_CHECK(root->Open(root, &elfFile, file, EFI_FILE_MODE_READ, 0));

    // read the header
    Elf64_Ehdr ehdr;
    CHECK_AND_RETHROW(FileRead(elfFile, &ehdr, sizeof(Elf64_Ehdr), 0));

    // verify is an elf
    CHECK(IS_ELF(ehdr));

    // verify the elf type
    CHECK(ehdr.e_ident[EI_VERSION] == EV_CURRENT);
    CHECK(ehdr.e_ident[EI_CLASS] == ELFCLASS64);
    CHECK(ehdr.e_ident[EI_DATA] == ELFDATA2LSB);

    // Load from section headers
    Elf64_Phdr phdr;
    for (int i = 0; i < ehdr.e_phnum; i++) {
        CHECK_AND_RETHROW(FileRead(elfFile, &phdr, sizeof(Elf64_Phdr), ehdr.e_phoff + ehdr.e_phentsize * i));

        switch (phdr.p_type) {
            // normal section
            case PT_LOAD:
                // ignore empty sections
                if (phdr.p_memsz == 0) continue;

                // get the type and pages to allocate
                EFI_MEMORY_TYPE MemType = (phdr.p_flags & PF_X) ? EfiRuntimeServicesCode : EfiRuntimeServicesData;
                UINTN nPages = EFI_SIZE_TO_PAGES(ALIGN_VALUE(phdr.p_memsz, EFI_PAGE_SIZE));

                // allocate the address
                EFI_PHYSICAL_ADDRESS base = info->VirtualOffset ? phdr.p_vaddr - info->VirtualOffset : phdr.p_paddr;
                Print(L"    BASE = %p, PAGES = %d\n", base, nPages);
                EFI_CHECK(gBS->AllocatePages(AllocateAddress, MemType, nPages, &base));
                CHECK_AND_RETHROW(FileRead(elfFile, (void*)base, phdr.p_filesz, phdr.p_offset));
                ZeroMem((void*)(base + phdr.p_filesz), phdr.p_memsz - phdr.p_filesz);

            // ignore entry
            default:
                break;
        }
    }

    // copy the section headers
    info->SectionHeadersSize = ehdr.e_shnum * ehdr.e_shentsize;
    info->SectionHeaders = AllocatePool(info->SectionHeadersSize); // TODO: Delete if error
    info->SectionEntrySize = ehdr.e_shentsize;
    info->StringSectionIndex = ehdr.e_shstrndx;
    CHECK_AND_RETHROW(FileRead(elfFile, info->SectionHeaders, info->SectionHeadersSize, ehdr.e_shoff));

    // copy the entry
    info->Entry = ehdr.e_entry;

cleanup:
    if (root != NULL) {
        FileHandleClose(root);
    }

    if (elfFile != NULL) {
        FileHandleClose(elfFile);
    }

    return Status;
}
