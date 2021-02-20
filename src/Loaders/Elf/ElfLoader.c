#include <Library/FileHandleLib.h>
#include <Util/Except.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseMemoryLib.h>
#include "ElfLoader.h"

#include "elf64.h"

static EFI_STATUS FileRead(EFI_FILE_PROTOCOL* File, UINTN Offset, void* Buffer, UINTN Size) {
    EFI_STATUS Status = EFI_SUCCESS;

    EFI_CHECK(FileHandleSetPosition(File, Offset));

    UINTN GotSize = Size;
    EFI_CHECK(FileHandleRead(File, Buffer, &GotSize));
    CHECK_STATUS(GotSize == Size, EFI_END_OF_FILE);

cleanup:
    return Status;
}

static EFI_STATUS Elf64ApplyRelocations(EFI_FILE_PROTOCOL* File, Elf64_Ehdr* hdr, void* Buffer, EFI_VIRTUAL_ADDRESS Vaddr, UINTN Size, UINT64 Slide) {
    EFI_STATUS Status = EFI_SUCCESS;

    // go over all the rela sections
    for (int i = 0; i < hdr->e_shnum; i++) {
        Elf64_Shdr Section = { 0 };
        CHECK_AND_RETHROW(FileRead(File, hdr->e_shoff + i * hdr->e_shentsize, &Section, sizeof(Section)));

        if (Section.sh_type != SHT_RELA) {
            continue;
        }

        // check the entsize is a good one
        CHECK_STATUS(Section.sh_entsize != sizeof(Elf64_Rela), EFI_UNSUPPORTED, "Invalid rela entry size");

        // iterate and apply all of the relocations
        for (UINT64 Offset = 0; Offset < Section.sh_size; Offset += Section.sh_entsize) {
            Elf64_Rela Reloc = { 0 };
            CHECK_AND_RETHROW(FileRead(File, Section.sh_offset + Offset, &Reloc, sizeof(Reloc)));

            switch (ELF64_R_TYPE(Reloc.r_info)) {
                case R_X86_64_RELATIVE: {
                    // relocation is not in the buffer
                    if (Reloc.r_offset < Vaddr || Vaddr + Size < Reloc.r_offset + 8) {
                        continue;
                    }

                    // apply the relocation
                    UINT64* Ptr = (UINT64*)((UINT8*)Buffer - Vaddr + Reloc.r_offset);
                    *Ptr = Slide + Reloc.r_addend;
                } break;

                default: {
                    CHECK_FAIL_STATUS(EFI_UNSUPPORTED, "Unknown rela type %x", Reloc.r_info);
                } break;
            }
        }
    }

cleanup:
    return Status;
}

EFI_STATUS Elf64LoadSection(EFI_FILE_PROTOCOL* File, void** Buffer, UINTN* BufferSize, const char* Name, UINT64 Slide) {
    EFI_STATUS Status = EFI_SUCCESS;
    CHAR8* Names = NULL;
    void* NewBuffer = NULL;

    CHECK(File != NULL);

    CHECK(Buffer != NULL);
    *Buffer = NULL;

    CHECK(BufferSize != NULL);
    *BufferSize = 0;

    // read the header
    Elf64_Ehdr hdr = { 0 };
    CHECK_AND_RETHROW(FileRead(File, 0, &hdr, sizeof(Elf64_Ehdr)));

    // verify the elf file
    CHECK_STATUS(IS_ELF(hdr), EFI_UNSUPPORTED, "Invalid elf magic!");
    CHECK_STATUS(hdr.e_ident[EI_CLASS] == ELFCLASS64, EFI_UNSUPPORTED, "Not 64bit elf");
    CHECK_STATUS(hdr.e_ident[EI_DATA] == ELFDATA2LSB, EFI_UNSUPPORTED, "Not 2s complement little endian elf");
    CHECK_STATUS(hdr.e_ident[EI_VERSION] == EV_CURRENT, EFI_UNSUPPORTED, "Not the current elf version");
    CHECK_STATUS(hdr.e_machine == EM_X86_64, EFI_UNSUPPORTED, "Not x86_64 elf");

    // read the name section
    Elf64_Shdr StrTabSection = { 0 };
    CHECK_AND_RETHROW(FileRead(File, hdr.e_shoff + hdr.e_shstrndx * hdr.e_shentsize, &StrTabSection, sizeof(StrTabSection)));
    Names = AllocatePool(StrTabSection.sh_size);
    CHECK_STATUS(Names != NULL, EFI_OUT_OF_RESOURCES);
    CHECK_AND_RETHROW(FileRead(File, StrTabSection.sh_offset, &Names, StrTabSection.sh_size));

    // iterate the section and find the correct one
    Elf64_Shdr Section = { 0 };
    for (int i = 0; i < hdr.e_shnum; i++) {
        CHECK_AND_RETHROW(FileRead(File, hdr.e_shoff + i * hdr.e_shentsize, &Section, sizeof(Section)));

        if (AsciiStrStr(&Names[Section.sh_name], Name) == 0) {
            // read the section
            NewBuffer = AllocatePool(Section.sh_size);
            CHECK_STATUS(NewBuffer != NULL, EFI_OUT_OF_RESOURCES);
            CHECK_AND_RETHROW(FileRead(File, Section.sh_offset, NewBuffer, Section.sh_size));

            // apply relocs
            CHECK_AND_RETHROW(Elf64ApplyRelocations(File, &hdr, NewBuffer, Section.sh_addr, Section.sh_size, Slide));

            break;
        }
    }

    // make sure we got it
    CHECK_STATUS(NewBuffer != NULL, EFI_NOT_FOUND, "Could not find section `%a`", Name);

    // set the outputs
    *Buffer = NewBuffer;
    *BufferSize = Section.sh_size;

cleanup:
    if (Status != EFI_SUCCESS) {
        if (NewBuffer != NULL) {
            FreePool(NewBuffer);
        }
    }
    if (Names != NULL) {
        FreePool(Names);
    }
    return Status;
}

EFI_STATUS Elf64Load(EFI_FILE_PROTOCOL* File, ELF_INFO* ElfInfo, UINT64 Slide, EFI_MEMORY_TYPE MemoryType) {
    EFI_STATUS Status = EFI_SUCCESS;

    CHECK(File != NULL);
    CHECK(ElfInfo != NULL);

    TRACE("Loading elf file");

    // read the header
    Elf64_Ehdr hdr = { 0 };
    CHECK_AND_RETHROW(FileRead(File, 0, &hdr, sizeof(Elf64_Ehdr)));

    // verify the elf file
    CHECK_STATUS(IS_ELF(hdr), EFI_UNSUPPORTED, "Invalid elf magic!");
    CHECK_STATUS(hdr.e_ident[EI_CLASS] == ELFCLASS64, EFI_UNSUPPORTED, "Not 64bit elf");
    CHECK_STATUS(hdr.e_ident[EI_DATA] == ELFDATA2LSB, EFI_UNSUPPORTED, "Not 2s complement little endian elf");
    CHECK_STATUS(hdr.e_ident[EI_VERSION] == EV_CURRENT, EFI_UNSUPPORTED, "Not the current elf version");
    CHECK_STATUS(hdr.e_machine == EM_X86_64, EFI_UNSUPPORTED, "Not x86_64 elf");

    ElfInfo->Top = 0;

    for (int i = 0; i < hdr.e_phnum; i++) {
        Elf64_Phdr phdr = { 0 };
        CHECK_AND_RETHROW(FileRead(File, hdr.e_phoff + hdr.e_phentsize * i, &phdr, sizeof(phdr)));

        // don't care for non-load sections
        if (phdr.p_type != PT_LOAD) {
            continue;
        }

        EFI_PHYSICAL_ADDRESS LoadAddr = phdr.p_vaddr;

        // check if this is a higher half kernel, if so remove
        // the higher half away so we can know the physical addresses
        if (LoadAddr & BIT63) {
            LoadAddr -= 0xffffffff80000000;
        }

        // add the kaslr slide
        LoadAddr += Slide;

        // the new top of memory
        UINT64 ThisTop = LoadAddr + phdr.p_memsz;
        if (ThisTop > ElfInfo->Top) {
            ElfInfo->Top = ThisTop;
        }

        // Allocate specific address
        TRACE("  physical %p, %d pages", LoadAddr, EFI_SIZE_TO_PAGES(phdr.p_memsz));
        EFI_CHECK(gBS->AllocatePages(AllocateAddress, MemoryType, EFI_SIZE_TO_PAGES(phdr.p_memsz), &LoadAddr),
                  "Failed to allocate pages for kernel, this could mean the requested physical address is already in use");

        // now read it from the file
        CHECK_AND_RETHROW(FileRead(File, phdr.p_offset, (void*)LoadAddr, phdr.p_filesz));

        // zero out the rest
        UINTN ToZero = phdr.p_memsz - phdr.p_filesz;
        if (ToZero != 0) {
            ZeroMem((void*)(LoadAddr + phdr.p_filesz), ToZero);
        }

        // apply the relocations
        CHECK_AND_RETHROW(Elf64ApplyRelocations(File, &hdr, (void*)LoadAddr, phdr.p_vaddr, phdr.p_memsz, Slide));
    }

    ElfInfo->EntryPoint = hdr.e_entry + Slide;

cleanup:
    return Status;
}
