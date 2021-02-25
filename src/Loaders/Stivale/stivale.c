
#include <Uefi.h>

#include <Library/RngLib.h>
#include <Library/BaseLib.h>
#include <Library/TimeBaseLib.h>
#include <Library/FileHandleLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <Protocol/GraphicsOutput.h>

#include <Util/Except.h>
#include <Loaders/Cpu.h>
#include <Util/GOPUtils.h>
#include <Util/DevicePathUtils.h>

#include <Config/Config.h>

#include <Loaders/Elf/ElfLoader.h>
#include <Loaders/VMM.h>
#include <Util/StringUtils.h>

#include "stivale.h"

/**
 * This is the slide of the KASLR, it is basically the possible offset
 * for the kernel in the last 2gb of memory
 */
#define KASLR_SLIDE_BITMASK 0x000FFF000u

/**
 * This map converts between uefi memory types to stivale memory types.
 *
 * We mark the loader, boot services and runtime code/data as bootloader reclaim since we don't have
 * EFI handover/runtime services passing.
 *
 * We also mark the memory mapped io port space as Kernel/Modules as a hack to get around the fact that on
 * older machines the custom memory types cause the machine to crash when getting the memory map :(
 */
static UINT32 EfiTypeToStivaleType[] = {
    [EfiReservedMemoryType] = STIVALE_MMAP_RESERVED,
    [EfiRuntimeServicesCode] = STIVALE_MMAP_BOOTLOADER_RECLAIMABLE,
    [EfiRuntimeServicesData] = STIVALE_MMAP_BOOTLOADER_RECLAIMABLE,
    [EfiMemoryMappedIO] = STIVALE_MMAP_RESERVED,
    [EfiMemoryMappedIOPortSpace] = STIVALE_MMAP_KERNEL_AND_MODULES,
    [EfiPalCode] = STIVALE_MMAP_RESERVED,
    [EfiUnusableMemory] = STIVALE_MMAP_BAD_MEMORY,
    [EfiACPIReclaimMemory] = STIVALE_MMAP_ACPI_RECLAIMABLE,
    [EfiLoaderCode] = STIVALE_MMAP_BOOTLOADER_RECLAIMABLE,
    [EfiLoaderData] = STIVALE_MMAP_BOOTLOADER_RECLAIMABLE,
    [EfiBootServicesCode] = STIVALE_MMAP_BOOTLOADER_RECLAIMABLE,
    [EfiBootServicesData] = STIVALE_MMAP_BOOTLOADER_RECLAIMABLE,
    [EfiConventionalMemory] = STIVALE_MMAP_USABLE,
    [EfiACPIMemoryNVS] = STIVALE_MMAP_ACPI_NVS
};

/**
 * This will properly replace the CR3 and setup pml5 if needed, then it will reset the registers
 * and simply jump to the kernel nicely.
 *
 * @param Struct        [IN] The stivale struct
 * @param Stack         [IN] The stack of the new kernel
 * @param KernelEntry   [IN] The new kernel entry
 * @param Pml5          [IN] Should pml5 be enabled
 *
 * Additionally the new cr3 is passed in cr2 (freaking msabi with their only 4 registers for parameters)
 */
void StivaleSpinup(struct stivale_struct* Struct, UINT64 Stack, void* KernelEntry, BOOLEAN Pml5);

EFI_STATUS LoadStivaleKernel(CONFIG_ENTRY* Config) {
    EFI_STATUS Status = EFI_SUCCESS;

    // open the file
    EFI_FILE_PROTOCOL* File = NULL;
    EFI_CHECK(OpenFileByDevicePath(&Config->KernelPath, &File, EFI_FILE_MODE_READ, 0));

    //------------------------------------------------------------------------------------------------------------------
    // KASLR
    //------------------------------------------------------------------------------------------------------------------
    UINT64 Slide = 0;
    if (Config->KASLR) {
        CHECK_STATUS(GetRandomNumber64(&Slide), EFI_UNSUPPORTED, "Failed to get random number for KASLR");
        Slide &= KASLR_SLIDE_BITMASK;
        TRACE("  KASLR Support enabled");
    }

    //------------------------------------------------------------------------------------------------------------------
    // Stivale header & kernel loading
    //------------------------------------------------------------------------------------------------------------------

    // get the stivale info
    TRACE("  Loading stivale header");
    struct stivale_header* StivaleHeader = NULL;
    UINTN HeaderSize = 0;
    CHECK_AND_RETHROW(Elf64LoadSection(File, (void**)&StivaleHeader, &HeaderSize, ".stivalehdr", Slide));
    CHECK(HeaderSize == sizeof(struct stivale_header),
            "`.stivalehdr` has invalid size, expected %d, got %d", sizeof(struct stivale_header), HeaderSize);

    // we don't support text mode (for now)
    WARN_ON(!(StivaleHeader->flags & BIT0), "text mode is not supported under uefi, forcing framebuffer");

    // now load the kernel itself
    TRACE("  Loading kernel...");
    ELF_INFO Info = {0};
    CHECK_AND_RETHROW(Elf64Load(File, &Info, Slide, EfiMemoryMappedIOPortSpace));

    // set the entry
    if (StivaleHeader->entry_point != 0) {
        Info.EntryPoint = StivaleHeader->entry_point;
    }

    //------------------------------------------------------------------------------------------------------------------
    // 5 Level paging
    //------------------------------------------------------------------------------------------------------------------

    BOOLEAN Pml5 = FALSE;
    UINT64 DirectMapBase = 0xffff800000000000;
    if (StivaleHeader->flags & BIT1) {
        // requested pml5, check support
        Pml5 = Check5LevelPagingSupport();
        if (Pml5) {
            TRACE("  PML5 support enabled");
            DirectMapBase = 0xff00000000000000;
        }
    }

    //------------------------------------------------------------------------------------------------------------------
    // Setup the structure
    //------------------------------------------------------------------------------------------------------------------

    // allocate the stivale struct
    struct stivale_struct* StivaleStruct = AllocateZeroPool(sizeof(struct stivale_struct));
    CHECK_STATUS(StivaleStruct != NULL, EFI_OUT_OF_RESOURCES);
    StivaleStruct->cmdline = (uint64_t)Config->Cmdline;
    StivaleStruct->flags = BIT1; // UEFI, Extended color information

    //------------------------------------------------------------------------------------------------------------------
    // Set epoch time
    //------------------------------------------------------------------------------------------------------------------

    EFI_TIME CurrentTime = { 0 };
    if (!EFI_ERROR(EfiGetTime(&CurrentTime, NULL))) {
        StivaleStruct->epoch = EfiTimeToEpoch(&CurrentTime);
        TRACE("  Got current epoch: %d", StivaleStruct->epoch);
    } else {
        StivaleStruct->epoch = 0;
        WARN("  Failed to get time for epoch...");
    }

    //------------------------------------------------------------------------------------------------------------------
    // Set boot modules
    //------------------------------------------------------------------------------------------------------------------

    LIST_ENTRY* Modules = &Config->Modules;
    struct stivale_module* last_module = NULL;
    for (LIST_ENTRY* Iter = GetFirstNode(Modules); Iter != Modules; Iter = GetNextNode(Modules, Iter)) {
        // get the module and open the file
        MODULE* Module = BASE_CR(Iter, MODULE, Link);
        EFI_FILE_PROTOCOL* ModuleFile = NULL;
        EFI_CHECK(OpenFileByDevicePath(&Module->Path, &ModuleFile, EFI_FILE_MODE_READ, 0));

        // fully read it
        // TODO: if compressed need to do something about it
        CHECK_STATUS(!Module->Compressed, EFI_UNSUPPORTED, "TODO: implement compressed modules lol");

        // read the module fully into memory
        EFI_PHYSICAL_ADDRESS FileBuffer = 0;
        UINTN FileSize = 0;
        EFI_CHECK(FileHandleGetSize(ModuleFile, &FileSize));
        EFI_CHECK(gBS->AllocatePages(AllocateAnyPages, EfiMemoryMappedIOPortSpace, EFI_SIZE_TO_PAGES(FileSize), &FileBuffer));
        EFI_CHECK(FileHandleRead(ModuleFile, &FileSize, (void*)FileBuffer));

        // allocate the structure
        struct stivale_module* current = AllocateZeroPool(sizeof(struct stivale_module));
        CHECK_STATUS(current != NULL, EFI_OUT_OF_RESOURCES);
        if (Module->String != NULL) {
            AsciiStrCpyS(current->string, sizeof(current->string), Module->String);
        }
        current->begin = FileBuffer;
        current->end = FileBuffer + FileSize;

        // link it and continue
        if (last_module != NULL) {
            last_module->next = (uint64_t)current;
        } else {
            // no last module yet, meaning we are first
            StivaleStruct->modules = (uint64_t)current;
        }
        StivaleStruct->module_count++;
        last_module = current;
    }

    //------------------------------------------------------------------------------------------------------------------
    // Set the acpi table
    //------------------------------------------------------------------------------------------------------------------

    //------------------------------------------------------------------------------------------------------------------
    // Prepare the higher half
    //------------------------------------------------------------------------------------------------------------------

    TRACE("  Creating page tables");
    PAGE_MAP PageMap = {0};
    CHECK_AND_RETHROW(NewPagemap(&PageMap, Pml5 ? 5 : 4));

    TRACE("    Map 0 to 2GiB at 0xffffffff80000000");
    for (UINTN Index = 0; Index < SIZE_2GB; Index += PAGE_SIZE) {
        CHECK_AND_RETHROW(MapPage(&PageMap, Index + 0xffffffff80000000ull, Index));
    }

    TRACE("    Map 0 to 4GiB at higher half base and 0");
    for (UINTN Index = 0; Index < SIZE_4GB; Index += PAGE_SIZE) {
        CHECK_AND_RETHROW(MapPage(&PageMap, Index, Index));
        CHECK_AND_RETHROW(MapPage(&PageMap, Index + DirectMapBase, Index));
    }

    TRACE("    Map everything above 4gb according to the efi memory map");

    UINTN DescriptorSize = 0;
    UINT32 DescriptorVersion = 0;
    UINTN MemoryMapSize = 0;
    UINTN MapKey = 0;
    void* Descriptors = NULL;

    // get the initial info
    Status = gBS->GetMemoryMap(&MemoryMapSize, Descriptors, &MapKey, &DescriptorSize, &DescriptorVersion);
    CHECK_STATUS(Status == EFI_BUFFER_TOO_SMALL, Status);
    CHECK(DescriptorVersion == EFI_MEMORY_DESCRIPTOR_VERSION, "Invalid UEFI descriptor version");
    Status = EFI_SUCCESS;
    MemoryMapSize += DescriptorSize * 10;
    Descriptors = AllocatePool(MemoryMapSize);
    CHECK_STATUS(Descriptors != NULL, EFI_OUT_OF_RESOURCES);
    EFI_CHECK(gBS->GetMemoryMap(&MemoryMapSize, Descriptors, &MapKey, &DescriptorSize, &DescriptorVersion));

    // anything which we did not map already map now
    for (UINTN Index = 0; Index < MemoryMapSize / DescriptorSize; Index++) {
        EFI_MEMORY_DESCRIPTOR* Descriptor = Descriptors + DescriptorSize * Index;
        UINTN Base = ALIGN_VALUE(Descriptor->PhysicalStart, PAGE_SIZE);
        UINTN End = ALIGN_VALUE(Descriptor->PhysicalStart + EFI_PAGES_TO_SIZE(Descriptor->NumberOfPages), PAGE_SIZE);
        TRACE("      %p-%p: %a", Base, End, EfiMemoryTypeToString(Descriptor->Type));

        // already mapped the lower 4gb
        if (End < BASE_4GB) {
            continue;
        }

        // map it all
        for (UINTN Yindex = Base; Yindex < End; Yindex += PAGE_SIZE) {
            CHECK_AND_RETHROW(MapPage(&PageMap, Yindex, Yindex));
            CHECK_AND_RETHROW(MapPage(&PageMap, Yindex + DirectMapBase, Yindex));
        }
    }

    // free it for now
    FreePool(Descriptors);

    //------------------------------------------------------------------------------------------------------------------
    // Set the Graphics Mode
    //------------------------------------------------------------------------------------------------------------------

    // get the best mode depending on the initial config
    UINTN Width = StivaleHeader->framebuffer_width;
    UINTN Height = StivaleHeader->framebuffer_height;
    UINTN Scanline = Width;
    EFI_PHYSICAL_ADDRESS Framebuffer = 0;
    CHECK_AND_RETHROW(SelectBestGopMode(&Width, &Height, &Scanline, &Framebuffer));
    TRACE("\tSet framebuffer: %dx%d - %p", Width, Height, Framebuffer);

    // set the extended info
    StivaleStruct->framebuffer_bpp = 32;
    StivaleStruct->framebuffer_width = Width;
    StivaleStruct->framebuffer_height = Height;
    StivaleStruct->framebuffer_pitch = Scanline;
    StivaleStruct->framebuffer_addr = Framebuffer;
    StivaleStruct->fb_memory_model = STIVALE_FBUF_MMODEL_RGB;
    StivaleStruct->fb_red_mask_size = 8;
    StivaleStruct->fb_red_mask_shift = 0;
    StivaleStruct->fb_green_mask_size = 8;
    StivaleStruct->fb_green_mask_shift = 8;
    StivaleStruct->fb_blue_mask_size = 8;
    StivaleStruct->fb_blue_mask_shift = 16;

    //------------------------------------------------------------------------------------------------------------------
    // Set the Memory map
    //------------------------------------------------------------------------------------------------------------------

    // get the memory map details again
    DescriptorSize = 0;
    DescriptorVersion = 0;
    MemoryMapSize = 0;
    MapKey = 0;
    Descriptors = NULL;
    Status = gBS->GetMemoryMap(&MemoryMapSize, Descriptors, &MapKey, &DescriptorSize, &DescriptorVersion);
    CHECK_STATUS(Status == EFI_BUFFER_TOO_SMALL, Status);
    CHECK(DescriptorVersion == EFI_MEMORY_DESCRIPTOR_VERSION, "Invalid UEFI descriptor version");
    Status = EFI_SUCCESS;
    MemoryMapSize += DescriptorSize * 10;
    Descriptors = AllocatePool(MemoryMapSize);
    CHECK_STATUS(Descriptors != NULL, EFI_OUT_OF_RESOURCES);
    EFI_CHECK(gBS->GetMemoryMap(&MemoryMapSize, Descriptors, &MapKey, &DescriptorSize, &DescriptorVersion));

    // we done bois
    EFI_CHECK(gBS->ExitBootServices(gImageHandle, MapKey));

    // set in the struct
    StivaleStruct->memory_map_addr = (uint64_t)Descriptors;

    // now transform it
    struct stivale_mmap_entry* LastEntry = Descriptors;
    for (UINTN Index = 0; Index < MemoryMapSize / DescriptorSize; Index++) {
        EFI_MEMORY_DESCRIPTOR* Descriptor = Descriptors + DescriptorSize * Index;
        UINT32 Type = EfiTypeToStivaleType[Descriptor->Type];

        if (
            Index != 0 &&
            LastEntry->type == Type &&
            LastEntry->base + LastEntry->length == Descriptor->PhysicalStart
        ) {
            // this is just after the last entry, merge it
            LastEntry->length += EFI_PAGES_TO_SIZE(Descriptor->NumberOfPages);
        } else {
            // get next one
            if (Index != 0) {
                LastEntry++;
            }

            // setup the new entry
            LastEntry->base = Descriptor->PhysicalStart;
            LastEntry->length = EFI_PAGES_TO_SIZE(Descriptor->NumberOfPages);
            LastEntry->type = Type;
            LastEntry->unused = 0;

            // increment for each created entry
            StivaleStruct->memory_map_entries++;
        }
    }

    // prepare paging on final time
    if (Pml5) {
        // set aside the top level for the spinup function
        AsmWriteCr2((UINTN)PageMap.TopLevel);
    } else {
        // switch for the new page tables right now
        AsmWriteCr3((UINTN)PageMap.TopLevel);
    }

    // stivale spinup
    StivaleSpinup(StivaleStruct, StivaleHeader->stack, (void*)Info.EntryPoint, Pml5);

cleanup:
    ERROR("Should not have arrived here...");
    CpuDeadLoop();
    return Status;
}
