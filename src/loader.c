#include "loader.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <uefi/uefi.h>
#include <kboot/kboot.h>
#include <elf64/elf64.h>

// this is a shellcode we will load in order to switch the address space
__attribute__((flatten))
static void virtual_load_stub(uint64_t cr3) {
    
}
__attribute__((flatten))
static void virtual_load_stub_end() {}

#define ASSERT(cond, fmt, ...) \
    do { \
        if(!(cond)) { \
            printf(L"Assertion failed: "fmt"\n\r", ## __VA_ARGS__); \
            goto failed; \
        } \
    } while(0)

void load_kernel(boot_entry_t* entry) {
    gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_WHITE, EFI_BLACK));
    gST->ConOut->ClearScreen(gST->ConOut);
    printf(L"Booting `%s` (%s)\n\r", entry->name, entry->filename);
    printf(L"Command line: `%s`\n\r", entry->command_line);
    char name_buffer[256];
 
    // get the root of the file system
    EFI_GUID sfpGuid = EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID;
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* sfp = 0;
    gBS->LocateProtocol(&sfpGuid, 0, (VOID**)&sfp);
    EFI_FILE_PROTOCOL* rootDir = 0;
	sfp->OpenVolume(sfp, &rootDir);

    // load the kernel
    EFI_FILE_PROTOCOL* file = OpenFile(rootDir, entry->filename, EFI_FILE_MODE_READ, 0);
    if(file == NULL) goto failed;

    Elf64_Ehdr header;
    size_t read_size = sizeof(Elf64_Ehdr);
    ReadFile(file, &read_size, &header);
    ASSERT(read_size == sizeof(Elf64_Ehdr), L"File too small");

    // verify elf
    ASSERT(header.e_ident[EI_MAG0] == ELFMAG0, L"Elf magic incorrect");
    ASSERT(header.e_ident[EI_MAG1] == ELFMAG1, L"Elf magic incorrect");
    ASSERT(header.e_ident[EI_MAG2] == ELFMAG2, L"Elf magic incorrect");
    ASSERT(header.e_ident[EI_MAG3] == ELFMAG3, L"Elf magic incorrect");
    ASSERT(header.e_ident[EI_CLASS] == ELFCLASS64, L"Elf class incorrect (only elf64 is supported)");
    ASSERT(header.e_ident[EI_DATA] == ELFDATA2LSB, L"Elf data incorrect (only little endian is supported)");
    ASSERT(header.e_type == ET_EXEC, L"Elf type incorrect (only executable is supported)");
    ASSERT(header.e_machine == EM_X86_64, L"Elf machine incorrect (only x86_64 is supported)");

    // make sure we have the sections we need
    ASSERT(header.e_shoff, "Elf missing section table");
    ASSERT(header.e_shstrndx, "Elf missing the str table index");
    
    // get the string table
    Elf64_Shdr strtab;
    read_size = sizeof(Elf64_Shdr);
    SetPosition(file, header.e_shoff + header.e_shentsize * header.e_shstrndx);   
    ReadFile(file, &read_size, &strtab);
    ASSERT(read_size == sizeof(Elf64_Ehdr), L"File too small");

    // find the .kboot.header section
    Elf64_Shdr table;
    bool found = false;
    for(int i = 0; i < header.e_shnum; i++) {
        read_size = sizeof(Elf64_Shdr);
        SetPosition(file, header.e_shoff + header.e_shentsize * i);   
        ReadFile(file, &read_size, &table);
        
        read_size = 256;
        SetPosition(file, strtab.sh_offset + table.sh_name);
        ReadFile(file, &read_size, &name_buffer);
        
        if(strcmp(name_buffer, ".kboot.header") == 0) {
            found = true;
            break;
        }
    }
    ASSERT(found, L"Could not find the `.kboot.header` section");

    // read the header
    kboot_header_t kheader = {0};
    read_size = sizeof(kboot_header_t);
    SetPosition(file, table.sh_offset);
    ReadFile(file, &read_size, &kheader);
    ASSERT(read_size == sizeof(kboot_header_t), L"File too small");

    // print the info for debugging
    printf(L"kboot header:\n\r");
    printf(L"  mapping: \n\r");
    printf(L"    tpye: %s\n\r", kheader.mapping.type == KBOOT_MAPPING_VIRTUAL ? L"virtual" : L"identity");
    if(kheader.mapping.type == KBOOT_MAPPING_VIRTUAL) {
       printf(L"    direct mapping base: 0x%016llp\n\r", kheader.mapping.direct_mapping_base);
    }
    printf(L"  framebuffer: \n\r");
    printf(L"    width: %d\n\r", kheader.framebuffer.width);
    printf(L"    height: %d\n\r", kheader.framebuffer.height);
    printf(L"    bpp: %d\n\r", kheader.framebuffer.bpp);

    CloseFile(file);

    while(1);

failed:
    // TODO: press any key to shutdown
    while(1);
}
