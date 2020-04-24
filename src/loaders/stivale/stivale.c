#include <Uefi.h>
#include <Guid/Acpi.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

#include <util/Except.h>
#include <config/BootConfig.h>
#include <config/BootEntries.h>
#include <loaders/elf/elf64.h>
#include <loaders/elf/ElfLoader.h>
#include <util/FileUtils.h>

#include "stivale.h"

static UINT32 EfiTypeToStivaleType[] = {
        [EfiReservedMemoryType] = 2,
        [EfiRuntimeServicesCode] = 2,
        [EfiRuntimeServicesData] = 2,
        [EfiMemoryMappedIO] = 2,
        [EfiMemoryMappedIOPortSpace] = 2,
        [EfiPalCode] = 2,
        [EfiUnusableMemory] = 5,
        [EfiACPIReclaimMemory] = 3,
        [EfiLoaderCode] = 1,
        [EfiLoaderData] = 1,
        [EfiBootServicesCode] = 1,
        [EfiBootServicesData] = 1,
        [EfiConventionalMemory] = 1,
        [EfiACPIMemoryNVS] = 4
};

void NORETURN JumpToStivaleKernel(STIVALE_STRUCT* strct, UINT64 Stack, void* KernelEntry);

static EFI_STATUS LoadStivaleHeader(CHAR16* file, STIVALE_HEADER* header, BOOLEAN* HigherHalf) {
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* filesystem = NULL;
    EFI_FILE_PROTOCOL* root = NULL;
    EFI_FILE_PROTOCOL* image = NULL;
    CHAR8* names = NULL;
    CHECK(HigherHalf != NULL);
    *HigherHalf = FALSE;

    // open the executable file
    Print(L"Loading image `%s`\n", file);
    EFI_CHECK(gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID**)&filesystem));
    EFI_CHECK(filesystem->OpenVolume(filesystem, &root));
    EFI_CHECK(root->Open(root, &image, file, EFI_FILE_MODE_READ, 0));

    Elf64_Ehdr ehdr = {0};
    CHECK_AND_RETHROW(FileRead(image, &ehdr, sizeof(ehdr), 0));

    // verify is an elf
    CHECK(IS_ELF(ehdr));

    // verify the elf type
    CHECK(ehdr.e_ident[EI_VERSION] == EV_CURRENT);
    CHECK(ehdr.e_ident[EI_CLASS] == ELFCLASS64);
    CHECK(ehdr.e_ident[EI_DATA] == ELFDATA2LSB);

    // higher half if
    if (ehdr.e_entry > 0xffffffff80000000) {
        *HigherHalf = TRUE;
    }

    // read the string table
    Elf64_Shdr shstr;
    CHECK_AND_RETHROW(FileRead(image, &shstr, sizeof(Elf64_Shdr), ehdr.e_shoff + ehdr.e_shentsize * ehdr.e_shstrndx));
    names = AllocatePool(shstr.sh_size);
    CHECK_AND_RETHROW(FileRead(image, names, shstr.sh_size, shstr.sh_offset));

    // search for the stivale section
    Elf64_Shdr shdr;
    BOOLEAN found = FALSE;
    for (int i = 0; i < ehdr.e_phnum; i++) {
        CHECK_AND_RETHROW(FileRead(image, &shdr, sizeof(Elf64_Shdr), ehdr.e_shoff + ehdr.e_shentsize * i));
        if (AsciiStrCmp(&names[shdr.sh_name], ".stivalehdr") == 0) {
            found = TRUE;
            break;
        }
    }
    CHECK(!found);

    // read it
    CHECK(shdr.sh_size >= sizeof(*header));
    CHECK_AND_RETHROW(FileRead(image, header, sizeof(*header), shdr.sh_offset));

cleanup:
    if (names != NULL) {
        FreePool(names);
    }
    return Status;
}

EFI_STATUS LoadStivaleKernel(BOOT_ENTRY* Entry) {
    EFI_STATUS Status = EFI_SUCCESS;
    STIVALE_HEADER Header = {0};
    ELF_INFO Elf = {0};

    gST->ConOut->ClearScreen(gST->ConOut);
    gST->ConOut->SetCursorPosition(gST->ConOut, 0, 0);

    // load config
    BOOT_CONFIG config;
    LoadBootConfig(&config);

    // set graphics mode right away
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;
    ASSERT_EFI_ERROR(gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID**)&gop));
    ASSERT_EFI_ERROR(gop->SetMode(gop, (UINT32) config.GfxMode));

    // get the header and decide on higher half
    BOOLEAN HigherHalf = FALSE;
    CHECK_AND_RETHROW(LoadStivaleHeader(Entry->Path, &Header, &HigherHalf));
    if (HigherHalf) {
        Elf.VirtualOffset = 0xffffffff80000000;
    }

    // fully-load the kernel
    CHECK_AND_RETHROW(LoadElf64(Entry->Path, &Elf));

    // setup the struct
    STIVALE_STRUCT* Struct = AllocateReservedPool(sizeof(STIVALE_STRUCT));

    // graphics info
    Struct->FramebufferAddr = gop->Mode->FrameBufferBase;
    Struct->FramebufferWidth = gop->Mode->Info->HorizontalResolution;
    Struct->FramebufferHeight = gop->Mode->Info->VerticalResolution;
    Struct->FramebufferPitch = gop->Mode->Info->PixelsPerScanLine * 4;
    Struct->FramebufferBpp = 32;

    // set the acpi table
    void* acpi_table = NULL;
    if (!EFI_ERROR(EfiGetSystemConfigurationTable(&gEfiAcpi20TableGuid, &acpi_table))) {
        Struct->Rsdp = (UINT64)AllocateReservedCopyPool(36, acpi_table);
    } else if (!EFI_ERROR(EfiGetSystemConfigurationTable(&gEfiAcpi10TableGuid, &acpi_table))) {
        Struct->Rsdp = (UINT64)AllocateReservedCopyPool(20, acpi_table);
    } else {
        Print(L"No ACPI table found, RSDP set to NULL\n");
    }

    // TODO: load modules

    // setup the page table correctly
    // first disable write protection so we can modify the table
    IA32_CR0 Cr0 = { .UintN = AsmReadCr0() };
    Cr0.Bits.WP = 0;
    AsmWriteCr0(Cr0.UintN);

    // get the memory map
    UINT64* Pml4 = (UINT64*)AsmReadCr3();

    // take the first pml4 and copy it to
    // the 0xffff800000000000 base
    Pml4[256] = Pml4[0];

    // allocate the 0xffffffff80000000
    Pml4[511] = (UINT64)AllocatePages(1) | 0x3;
    UINT64* Pml3High = &Pml4[511];
    UINT64* Pml3Low = &Pml4[0];
    Pml3High[510] = Pml3Low[0];
    Pml3High[511] = Pml3Low[1];

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // No prints from here
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // setup the mmap information
    UINT8 TmpMemoryMap[1];
    UINTN MemoryMapSize = sizeof(TmpMemoryMap);
    UINTN MapKey = 0;
    UINTN DescriptorSize = 0;
    UINT32 DescriptorVersion = 0;
    CHECK(gBS->GetMemoryMap(&MemoryMapSize, (EFI_MEMORY_DESCRIPTOR *) TmpMemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion) == EFI_BUFFER_TOO_SMALL);

    // allocate space for the efi mmap and take into
    // account that there will be changes
    MemoryMapSize += EFI_PAGE_SIZE;
    EFI_MEMORY_DESCRIPTOR* MemoryMap = AllocatePool(MemoryMapSize);

    // allocate all the space we will need (hopefully)
    STIVALE_MMAP_ENTRY* StartFrom = AllocateReservedPool(MemoryMapSize);

    // call it
    EFI_CHECK(gBS->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion));
    UINTN EntryCount = (MemoryMapSize / DescriptorSize);

    // Exit the memory services
    EFI_CHECK(gBS->ExitBootServices(gImageHandle, MapKey));

    // setup the normal memory map
    STIVALE_MMAP_ENTRY* mmap = (void*)StartFrom;
    Struct->MemoryMapAddr = (UINT64)mmap;
    Struct->MemoryMapEntries = MemoryMapSize / DescriptorSize;
    for (int i = 0; i < EntryCount; i++) {
        STIVALE_MMAP_ENTRY* MmapEntry = &mmap[i];
        EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)((UINTN)MemoryMap + DescriptorSize * i);
        MmapEntry->Type = EfiTypeToStivaleType[desc->Type];
        MmapEntry->Base = desc->PhysicalStart;
        MmapEntry->Length = EFI_PAGES_TO_SIZE(desc->NumberOfPages);
        MmapEntry->Unused = 0;
    }

    // no interrupts
    DisableInterrupts();

    JumpToStivaleKernel(Struct, Header.Stack, (void*)Elf.Entry);

cleanup:
    return Status;
}
