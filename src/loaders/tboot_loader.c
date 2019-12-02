#include <Uefi.h>
#include <Guid/Acpi.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Protocol/SimpleFileSystem.h>

#include <elf64/elf64.h>
#include <tboot/tboot.h>
#include <uefi/Include/Protocol/GraphicsOutput.h>
#include <uefi/Include/Guid/FileInfo.h>
#include <util/file_utils.h>
#include "tboot_loader.h"

#define CUSTOM_TYPE_BOOT_INFO   (0x80000000 + 0)
#define CUSTOM_TYPE_KERNEL      (0x80000000 + 1)
#define CUSTOM_TYPE_MODULE_N(n) (0x80000000 + 2 + (n))

typedef struct acpi_table_entry {
    EFI_GUID* guid;
    UINTN size;
} acpi_table_entry_t;

// just in case I will set them all to that size
static acpi_table_entry_t acpi_table_guids[] = {
    { &gEfiAcpi20TableGuid, sizeof(EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER) },
    { &gEfiAcpi10TableGuid, sizeof(EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER) },
    { &gEfiAcpiTableGuid, sizeof(EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER) },
};

static void read_bytes(EFI_FILE_PROTOCOL* file, UINTN size, void* res) {
    ASSERT_EFI_ERROR(file->Read(file, &size, res));
}

static tboot_entry_function load_elf_file(const CHAR8* path) {
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

    // finally load the elf itself
    Elf64_Ehdr ehdr;
    read_bytes(file, sizeof(Elf64_Ehdr), &ehdr);

    // verify the header
    ASSERT(ehdr.e_ident[EI_MAG0] == ELFMAG0);
    ASSERT(ehdr.e_ident[EI_MAG1] == ELFMAG1);
    ASSERT(ehdr.e_ident[EI_MAG2] == ELFMAG2);
    ASSERT(ehdr.e_ident[EI_MAG3] == ELFMAG3);
    // TODO: Check from current architecture
    ASSERT(ehdr.e_ident[EI_CLASS] == ELFCLASS64);
    ASSERT(ehdr.e_ident[EI_DATA] == ELFDATA2LSB);
    ASSERT(ehdr.e_type == ET_EXEC);
    ASSERT(ehdr.e_machine == EM_X86_64);

    // load the program sections
    ASSERT(ehdr.e_phnum != 0);
    Elf64_Phdr phdr;
    for(int i = 0; i < ehdr.e_phoff; i++) {
        // read it
        ASSERT_EFI_ERROR(file->SetPosition(file, ehdr.e_phoff + ehdr.e_phentsize * i));
        read_bytes(file, sizeof(Elf64_Phdr), &phdr);

        switch(phdr.p_type) {

            // a section which is needed to be loaded derp
            case PT_LOAD: {
                // allocate the pages
                EFI_PHYSICAL_ADDRESS addr = phdr.p_paddr;
                ASSERT_EFI_ERROR(gBS->AllocatePages(AllocateAddress, CUSTOM_TYPE_KERNEL, EFI_SIZE_TO_PAGES(phdr.p_memsz), &addr));
                ZeroMem((void*)addr, phdr.p_memsz);

                // read the data
                ASSERT_EFI_ERROR(file->SetPosition(file, phdr.p_offset));
                read_bytes(file, phdr.p_filesz, (VOID*)addr);
            } break;

            default:
                break;
        }
    }

    // free and close everything
    ASSERT_EFI_ERROR(file->Close(file));
    ASSERT_EFI_ERROR(root->Close(root));
    ASSERT_EFI_ERROR(gBS->FreePool(unicode));

    return (tboot_entry_function)ehdr.e_entry;
}

void load_tboot_binary(boot_entry_t* entry) {
    // clear everything, this is going to be a simple log of how we loaded the stuff
    ASSERT_EFI_ERROR(gST->ConOut->SetAttribute(gST->ConOut, EFI_TEXT_ATTR(EFI_LIGHTGRAY, EFI_BLACK)));
    ASSERT_EFI_ERROR(gST->ConOut->SetCursorPosition(gST->ConOut, 0, 0));
    ASSERT_EFI_ERROR(gST->ConOut->ClearScreen(gST->ConOut));

    // get the config once more
    boot_config_t config;
    load_boot_config(&config);

    // set graphics mode right away
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;
    ASSERT_EFI_ERROR(gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID**)&gop));
    ASSERT_EFI_ERROR(gop->SetMode(gop, (UINT32) config.gfx_mode));

    // read the elf file
    DebugPrint(0, "Loading: %a - %a\n", entry->name, entry->path);
    tboot_entry_function kmain = load_elf_file(entry->path);

    // prepare the boot info
    tboot_info_t* info = NULL;
    ASSERT_EFI_ERROR(gBS->AllocatePool(CUSTOM_TYPE_BOOT_INFO, sizeof(tboot_info_t), (void*)&info));
    SetMem(info, sizeof(tboot_info_t), 0);

    // set the cmd
    info->flags.cmdline = 1;
    info->cmdline.length = AsciiStrLen(entry->cmd);
    ASSERT_EFI_ERROR(gBS->AllocatePool(CUSTOM_TYPE_BOOT_INFO, info->cmdline.length, (void*)&info->cmdline.cmdline));
    AsciiStrCpy(info->cmdline.cmdline, entry->cmd);
    DebugPrint(0, "Command line: %a\n", info->cmdline.cmdline);

    // load the boot modules
    info->flags.modules = 1;
    info->modules.count = entry->modules_count;
    if(info->modules.count != 0) {
        ASSERT_EFI_ERROR(gBS->AllocatePool(CUSTOM_TYPE_BOOT_INFO, sizeof(tboot_module_t) * info->modules.count, (void*)&info->modules.entries));

        int index = 0;
        boot_module_t* module = entry->modules;
        while(module != NULL) {

            // load the file
            load_file(module->path, CUSTOM_TYPE_MODULE_N(index), &info->modules.entries[index].base, &info->modules.entries[index].len);

            // prepare the name
            ASSERT_EFI_ERROR(gBS->AllocatePool(CUSTOM_TYPE_BOOT_INFO, AsciiStrLen(module->tag), (void*)&info->modules.entries[index].name));
            AsciiStrCpy(info->modules.entries[index].name, module->tag);

            tboot_module_t* mod = &info->modules.entries[index];
            DebugPrint(0, "Module `%a`: %p, %d bytes\n", mod->name, mod->base, mod->len);

            // next entry
            index++;
            module = module->next;
        }
    }else {
        info->modules.entries = NULL;
    }

    // calculate the tsc freq
    info->flags.tsc_freq = 1; // TODO: check the cpu supports the tsc first
    uint64_t tsc_freq = AsmReadTsc();
    gBS->Stall(1000);
    info->tsc_freq = (AsmReadTsc() - tsc_freq) * 1000;
    DebugPrint(0, "TSC Freq: %lld Hz\n", info->tsc_freq);

    // search for the ACPI table, prefer newer guids than older ones
    int index = 0;
    void* table = NULL;
    for(int i = 0; i < gST->NumberOfTableEntries; i++) {
        EFI_CONFIGURATION_TABLE config_table = gST->ConfigurationTable[i];

        // check if this is a acpi table
        for(int j = 0; j < ARRAY_SIZE(acpi_table_guids); j++) {
            acpi_table_entry_t* e = &acpi_table_guids[j];
            if(CompareGuid(&config_table.VendorGuid, e->guid)) {
                if(table == NULL || index > j) {
                    // free if already found a good entry
                    if(table != NULL) {
                        ASSERT_EFI_ERROR(gBS->FreePages((EFI_PHYSICAL_ADDRESS)table, EFI_SIZE_TO_PAGES(acpi_table_guids[j].size)));
                    }

                    // allocate and copy it
                    ASSERT_EFI_ERROR(gBS->AllocatePages(AllocateAnyPages, CUSTOM_TYPE_BOOT_INFO, EFI_SIZE_TO_PAGES(e->size), (EFI_PHYSICAL_ADDRESS*)&table));
                    CopyMem(table, config_table.VendorTable, e->size);
                    index = j;

                    // we have a table
                    info->flags.rsdp = 1;
                }
            }
        }
    }
    DebugPrint(0, "ACPI table: %p\n", table);
    info->rsdp = (UINT64)table;

    // set the graphics mode
    info->flags.framebuffer = 1;
    info->framebuffer.width = gop->Mode->Info->HorizontalResolution;
    info->framebuffer.height = gop->Mode->Info->VerticalResolution;
    info->framebuffer.pitch = gop->Mode->Info->PixelsPerScanLine;
    info->framebuffer.addr = gop->Mode->FrameBufferBase;
    DebugPrint(0, "Framebuffer addr: %p\n", info->framebuffer.addr);
    DebugPrint(0, "Framebuffer width: %d\n", info->framebuffer.width);
    DebugPrint(0, "Framebuffer height: %d\n", info->framebuffer.height);
    DebugPrint(0, "Framebuffer pitch: %d\n", info->framebuffer.pitch);


    // This is taken from:
    // https://github.com/tianocore/edk2/blob/master/OvmfPkg/Library/LoadLinuxLib/Linux.c#L243
    EFI_STATUS status;
    UINTN mapKey = 0;
    UINTN mapSize = 0;
    UINTN descSize = 0;
    UINT32 descVersion = 0;
    EFI_MEMORY_DESCRIPTOR* descs = NULL;
    if(EFI_ERROR(status = gBS->GetMemoryMap(&mapSize, NULL, &mapKey, &descSize, &descVersion)) && status != EFI_BUFFER_TOO_SMALL) ASSERT_EFI_ERROR(status);
    ASSERT_EFI_ERROR(gBS->AllocatePool(CUSTOM_TYPE_BOOT_INFO, mapSize, (void**)&descs));
    mapSize += EFI_PAGE_SIZE;
    ASSERT_EFI_ERROR(gBS->GetMemoryMap(&mapSize, descs, &mapKey, &descSize, &descVersion));
    info->mmap.entries = (tboot_mmap_entry_t*)descs;

    // after this we exit the boot services
    DebugPrint(0, "Bai Bai\n");

    // destroy all the libs we use and exit boot services
    ASSERT_EFI_ERROR(DebugLibDestructor(gImageHandle, gST));

    // exit boot services
    gBS->ExitBootServices(gImageHandle, mapKey);

    // make sure interrupts are disabled
    DisableInterrupts();

    // transform the memory map to be in the correct format
    info->flags.mmap = 1;
    index = 0;
    EFI_MEMORY_DESCRIPTOR* desc = descs;
    for(int i = 0; i < mapSize / descSize; i++) {
        UINTN addr = desc->PhysicalStart;
        UINTN len = desc->NumberOfPages * 4096;
        uint8_t type = 0;
        switch(desc->Type) {
            case EfiUnusableMemory:
                type = TBOOT_MEMORY_TYPE_BAD_MEMORY;
                break;

            case EfiACPIReclaimMemory:
                type = TBOOT_MEMORY_TYPE_ACPI_RECLAIM;
                break;

            case EfiLoaderCode:
            case EfiLoaderData:
            case EfiBootServicesCode:
            case EfiBootServicesData:
            case EfiConventionalMemory:
                type = TBOOT_MEMORY_TYPE_USABLE;
                break;

            case EfiACPIMemoryNVS:
                type = TBOOT_MEMORY_TYPE_ACPI_NVS;
                break;

            case CUSTOM_TYPE_KERNEL:
                type = TBOOT_MEMORY_TYPE_KERNEL;
                break;

            case CUSTOM_TYPE_BOOT_INFO:
                type = TBOOT_MEMORY_TYPE_BOOT_INFO;
                break;

            case EfiReservedMemoryType:
            case EfiMemoryMappedIO:
            case EfiMemoryMappedIOPortSpace:
            case EfiRuntimeServicesCode:
            case EfiRuntimeServicesData:
            case EfiPalCode:
            default:
                // special case, the boot modules
                if(desc->Type >= CUSTOM_TYPE_MODULE_N(0)) {
                    type = TBOOT_MEMORY_TYPE_MODULE_N(desc->Type - CUSTOM_TYPE_MODULE_N(0));

                // otherwise consider reserved
                }else {
                    type = TBOOT_MEMORY_TYPE_RESERVED;
                }

                break;
        }

        if(     index > 0
                &&  info->mmap.entries[index - 1].addr + info->mmap.entries[index - 1].len == addr
                &&  info->mmap.entries[index - 1].type == type) {
            // we can merge these
            info->mmap.entries[index - 1].len += len;
        }else {
            // set a new type
            info->mmap.entries[index].addr = addr;
            info->mmap.entries[index].len = len;
            info->mmap.entries[index].type = type;
            index++;
            info->mmap.count++;
        }

        // next
        desc = (EFI_MEMORY_DESCRIPTOR*)((UINTN)desc + descSize);
    }

    // set a nicer environment

    // clear WP (can cause GPD when trying to
    // update the page table)
    IA32_CR0 cr0 = { .UintN = AsmReadCr0() };
    cr0.Bits.WP = 0;
    AsmWriteCr0(cr0.UintN);

    // Disable interrupts, just in case
    DisableInterrupts();

    // and call the kernel
    DebugPrint(0, "calling kernel\n");
    kmain(TBOOT_MAGIC, info);
}
