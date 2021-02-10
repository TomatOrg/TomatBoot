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
#include <loaders/Loaders.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FileHandleLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <loaders/mb2/gdt.h>
#include <Library/BaseLib.h>
#include <util/TimeUtils.h>
#include <util/GfxUtils.h>

#include "stivale.h"

static UINT32 EfiTypeToStivaleType[] = {
    [EfiReservedMemoryType] = STIVALE_RESERVED,
    [EfiRuntimeServicesCode] = STIVALE_RESERVED,
    [EfiRuntimeServicesData] = STIVALE_RESERVED,
    [EfiMemoryMappedIO] = STIVALE_RESERVED,
    [EfiMemoryMappedIOPortSpace] = STIVALE_RESERVED,
    [EfiPalCode] = STIVALE_RESERVED,
    [EfiUnusableMemory] = STIVALE_BAD_MEMORY,
    [EfiACPIReclaimMemory] = STIVALE_ACPI_RECLAIM,
    [EfiLoaderCode] = STIVALE_BOOTLOADER_RECLAIM,
    [EfiLoaderData] = STIVALE_BOOTLOADER_RECLAIM,
    [EfiBootServicesCode] = STIVALE_BOOTLOADER_RECLAIM,
    [EfiBootServicesData] = STIVALE_BOOTLOADER_RECLAIM,
    [EfiConventionalMemory] = STIVALE_USABLE,
    [EfiACPIMemoryNVS] = STIVALE_ACPI_NVS
};

void NORETURN JumpToStivaleKernel(STIVALE_STRUCT* strct, UINT64 Stack, void* KernelEntry, BOOLEAN level5);



static EFI_STATUS LoadStivaleHeader(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FS, CHAR16* file, STIVALE_HEADER* header, BOOLEAN* HigherHalf) {
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_FILE_PROTOCOL* root = NULL;
    EFI_FILE_PROTOCOL* image = NULL;
    CHAR8* names = NULL;
    CHECK(HigherHalf != NULL);
    *HigherHalf = FALSE;

    // open the executable file
    Print(L"Loading image `%s`\n", file);
    EFI_CHECK(FS->OpenVolume(FS, &root));
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
    for (int i = 0; i < ehdr.e_shnum; i++) {
        CHECK_AND_RETHROW(FileRead(image, &shdr, sizeof(Elf64_Shdr), ehdr.e_shoff + ehdr.e_shentsize * i));
        if (AsciiStrCmp(&names[shdr.sh_name], ".stivalehdr") == 0) {
            found = TRUE;
            break;
        }
    }
    CHECK(found);

    // zero and read it
    CHECK(sizeof(*header) == shdr.sh_size);
    CHECK_AND_RETHROW(FileRead(image, header, sizeof(*header), shdr.sh_offset));

    // change the higher half spec if we have a
    // different entry point
    if (header->EntryPoint != 0) {
        *HigherHalf = header->EntryPoint > 0xffffffff80000000;
    }

cleanup:
    if (names != NULL) {
        FreePool(names);
    }

    if (root != NULL) {
        FileHandleClose(root);
    }

    if (image != NULL) {
        FileHandleClose(image);
    }

    return Status;
}

EFI_STATUS LoadStivaleKernel(BOOT_ENTRY* Entry) {
    EFI_STATUS Status = EFI_SUCCESS;
    STIVALE_HEADER Header = {0};
    ELF_INFO Elf = {0};
    BOOLEAN level5Supported = FALSE;

    // load config
    BOOT_CONFIG config;
    LoadBootConfig(&config);

    // get the header and decide on higher half
    BOOLEAN HigherHalf = FALSE;
    CHECK_AND_RETHROW(LoadStivaleHeader(Entry->Fs, Entry->Path, &Header, &HigherHalf));
    if (HigherHalf) {
        Elf.VirtualOffset = 0xffffffff80000000;
    }

    // we don't support text mode!
    CHECK_TRACE(Header.GraphicsFramebuffer, "Text mode is not supported in UEFI!");

    // override the gfx info
    INT32 GfxMode = config.GfxMode;
    if (!config.OverrideGfx && Header.FramebufferHeight != 0 && Header.FramebufferWidth != 0) {
        GfxMode = GetBestGfxMode(Header.FramebufferWidth, Header.FramebufferHeight);
    }

    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;
    ASSERT_EFI_ERROR(gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID**)&gop));
    ASSERT_EFI_ERROR(gop->SetMode(gop, (UINT32) GfxMode));

    UINT32 eax, ebx, ecx, edx;
    AsmCpuidEx(0x00000007, 0, &eax, &ebx, &ecx, &edx);
    if (ecx & BIT16) {
        level5Supported = TRUE;
    }

    WARN_ON(!Header.EnableKASLR, "KASLR Is not supported yet! ignoring.");

    // fully-load the kernel
    CHECK_AND_RETHROW(LoadElf64(Entry->Fs, Entry->Path, &Elf));
    if (Header.EntryPoint != 0) {
        Elf.Entry = Header.EntryPoint;
    }

    // setup the struct
    STIVALE_STRUCT* Struct = AllocateReservedPool(sizeof(STIVALE_STRUCT));
    SetMem(Struct, sizeof(STIVALE_STRUCT), 0);

    // cmdline
    TRACE("Setting cmdline");
    Struct->Cmdline = (UINT64)AllocateReservedPool(StrLen(Entry->Cmdline) + 1);
    UnicodeStrToAsciiStr(Entry->Cmdline, (CHAR8*)Struct->Cmdline);

    // graphics info
    TRACE("Setting framebuffer info");
    Struct->FramebufferAddr = gop->Mode->FrameBufferBase;
    Struct->FramebufferWidth = gop->Mode->Info->HorizontalResolution;
    Struct->FramebufferHeight = gop->Mode->Info->VerticalResolution;
    Struct->FramebufferPitch = gop->Mode->Info->PixelsPerScanLine * 4;
    Struct->FramebufferBpp = 32;
    Struct->FbMemoryModel = 1;
    Struct->FbRedMaskSize = 8;
    Struct->FbRedMaskShift = 0;
    Struct->FbGreenMaskSize = 8;
    Struct->FbGreenMaskShift = 8;
    Struct->FbBlueMaskSize = 8;
    Struct->FbBlueMaskShift = 16;
    Struct->Flags |= STIVALE_STRUCT_EXT_FB_INFO;

    // set the acpi table
    void* acpi_table = NULL;
    if (!EFI_ERROR(EfiGetSystemConfigurationTable(&gEfiAcpi20TableGuid, &acpi_table))) {
        Struct->Rsdp = (UINT64)AllocateReservedCopyPool(36, acpi_table);
    } else if (!EFI_ERROR(EfiGetSystemConfigurationTable(&gEfiAcpi10TableGuid, &acpi_table))) {
        Struct->Rsdp = (UINT64)AllocateReservedCopyPool(20, acpi_table);
    } else {
        WARN("No ACPI table found, RSDP set to NULL");
    }

    TRACE("Setting epoch");
    EFI_TIME Time = { 0 };
    EFI_CHECK(gRT->GetTime(&Time, NULL));
    Struct->Epoch = GetUnixEpoch(Time.Second, Time.Minute, Time.Hour, Time.Day, Time.Month, Time.Year);

    // push the modules
    TRACE("Loading modules");
    STIVALE_MODULE* LastModule = NULL;
    for (LIST_ENTRY* Link = Entry->BootModules.ForwardLink; Link != &Entry->BootModules; Link = Link->ForwardLink) {
        BOOT_MODULE* Module = BASE_CR(Link, BOOT_MODULE, Link);
        UINTN Start = 0;
        UINTN Size = 0;
        CHECK_AND_RETHROW(LoadBootModule(Module, &Start, &Size));

        STIVALE_MODULE* NewModule = AllocateReservedZeroPool(sizeof(STIVALE_MODULE));
        NewModule->Begin = Start;
        NewModule->End = Start + Size;
        UnicodeStrToAsciiStrS(Module->Tag, NewModule->String, sizeof(NewModule->String));

        if (LastModule != NULL) {
            LastModule->Next = (UINT64)NewModule;
        } else {
            Struct->Modules = (UINT64)NewModule;
        }

        Struct->ModuleCount++;
        LastModule = NewModule;

        TRACE("    Added %s (%s) -> %p - %p", Module->Tag, Module->Path, Start, Start + Size);
    }

    // setup the page table correctly
    // first disable write protection so we can modify the table
    TRACE("Preparing higher half");
    IA32_CR0 Cr0 = { .UintN = AsmReadCr0() };
    Cr0.Bits.WP = 0;
    AsmWriteCr0(Cr0.UintN);

    // get the memory map
    UINT64* Pml4 = (UINT64*)AsmReadCr3();

    // take the first pml4 and copy it to 0xffff800000000000
    Pml4[256] = Pml4[0];

    // allocate pml3 for 0xffffffff80000000
    UINT64* Pml3High = AllocateReservedPages(1);
    SetMem(Pml3High, EFI_PAGE_SIZE, 0);
    TRACE("Allocated page %p", Pml3High);
    Pml4[511] = ((UINT64)Pml3High) | 0x3u;

    // map first 2 pages to 0xffffffff80000000
    UINT64* Pml3Low = (UINT64*)(Pml4[0] & 0x7ffffffffffff000u);
    Pml3High[510] = Pml3Low[0];
    Pml3High[511] = Pml3Low[1];

    TRACE("Getting memory map");
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
    STIVALE_MMAP_ENTRY* StartFrom = AllocateReservedPool((MemoryMapSize / DescriptorSize) * sizeof(STIVALE_MMAP_ENTRY));

    // call it
    EFI_CHECK(gBS->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion));
    UINTN EntryCount = (MemoryMapSize / DescriptorSize);

    // Exit the memory services
    EFI_CHECK(gBS->ExitBootServices(gImageHandle, MapKey));

    // setup the normal memory map
    Struct->MemoryMapAddr = (UINT64)StartFrom;
    Struct->MemoryMapEntries = 0;
    int LastType = -1;
    UINTN LastEnd = 0xFFFFFFFFFFFF;
    for (int i = 0; i < EntryCount; i++) {
        EFI_MEMORY_DESCRIPTOR* Desc = (EFI_MEMORY_DESCRIPTOR*)((UINTN)MemoryMap + DescriptorSize * i);
        EFI_PHYSICAL_ADDRESS PhysicalBase = Desc->PhysicalStart;
        UINTN Length = EFI_PAGES_TO_SIZE(Desc->NumberOfPages);
        EFI_PHYSICAL_ADDRESS PhysicalEnd = Desc->PhysicalStart + Length;

        int Type = STIVALE_RESERVED;
        if (Desc->Type == gKernelAndModulesMemoryType) {
            Type = STIVALE_KERNEL_MODULES;
        } else if (Desc->Type < EfiMaxMemoryType) {
            Type = EfiTypeToStivaleType[Desc->Type];
        }

        if (LastType == Type && LastEnd == Desc->PhysicalStart) {
            StartFrom[-1].Length += Length;
        } else {
            StartFrom->Type = Type;
            StartFrom->Length = Length;
            StartFrom->Base = PhysicalBase;
            StartFrom->Unused = 0;
            LastType = Type;
            StartFrom++;
            Struct->MemoryMapEntries++;
        }
        LastEnd = PhysicalEnd;
    }

    // no interrupts
    DisableInterrupts();

    JumpToStivaleKernel(Struct, Header.Stack, (void*)Elf.Entry, Header.Pml5Enable && level5Supported);

cleanup:
    return Status;
}
