
#include <Uefi.h>
#include <Config/Config.h>
#include <Library/MemoryAllocationLib.h>
#include <Util/Except.h>
#include <Library/BaseLib.h>
#include <Loaders/Elf/ElfLoader.h>
#include <Util/DevicePathUtils.h>
#include <Register/Intel/Cpuid.h>
#include <Library/TimeBaseLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/FileHandleLib.h>
#include <Protocol/GraphicsOutput.h>
#include <Util/GOPUtils.h>
#include "stivale.h"

//static BOOLEAN Check5LevelPagingSupport() {
//    CPUID_VIR_PHY_ADDRESS_SIZE_EAX VirPhyAddressSize = {0};
//    CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_ECX ExtFeatureEcx = {0};
//    UINT32 MaxExtendedFunctionId = 0;
//
//    // query the virtual address size
//    AsmCpuid (CPUID_EXTENDED_FUNCTION, &MaxExtendedFunctionId, NULL, NULL, NULL);
//    if (MaxExtendedFunctionId >= CPUID_VIR_PHY_ADDRESS_SIZE) {
//        AsmCpuid (CPUID_VIR_PHY_ADDRESS_SIZE, &VirPhyAddressSize.Uint32, NULL, NULL, NULL);
//    } else {
//        VirPhyAddressSize.Bits.PhysicalAddressBits = 36;
//    }
//
//    // query the five level paging support
//    AsmCpuidEx (
//        CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS,
//        CPUID_STRUCTURED_EXTENDED_FEATURE_FLAGS_SUB_LEAF_INFO,
//        NULL, NULL, &ExtFeatureEcx.Uint32, NULL
//    );
//
//    // check we have five level paging and a big enough virtual address space
//    if (ExtFeatureEcx.Bits.FiveLevelPage == 1 && VirPhyAddressSize.Bits.PhysicalAddressBits > 4 * 9 + 12) {
//        return TRUE;
//    } else {
//        return FALSE;
//    }
//}

EFI_STATUS LoadStivaleKernel(CONFIG_ENTRY* Config) {
    EFI_STATUS Status = EFI_SUCCESS;

    // open the file
    EFI_FILE_PROTOCOL* File = NULL;
    EFI_CHECK(OpenFileByDevicePath(&Config->KernelPath, &File, EFI_FILE_MODE_READ, 0));

    //------------------------------------------------------------------------------------------------------------------
    // KASLR
    //------------------------------------------------------------------------------------------------------------------
    UINTN Slide = 0;
    // TODO: kaslr, needs bootloader support

    //------------------------------------------------------------------------------------------------------------------
    // Stivale header
    //------------------------------------------------------------------------------------------------------------------

    // get the stivale info
    struct stivale_header* StivaleHeader = NULL;
    UINTN HeaderSize = 0;
    CHECK_AND_RETHROW(Elf64LoadSection(File, (void**)&StivaleHeader, &HeaderSize, ".stivalehdr", Slide));
    CHECK(HeaderSize != sizeof(struct stivale_header),
            "`.stivalehdr` has invalid size, expected %d, got %d", sizeof(struct stivale_header), HeaderSize);

    // we don't support text mode (for now)
    WARN_ON(!(StivaleHeader->flags & BIT0), "text mode is not supported under uefi, forcing framebuffer");

    //------------------------------------------------------------------------------------------------------------------
    // 5 Level paging
    //------------------------------------------------------------------------------------------------------------------

//    BOOLEAN Pml5 = FALSE;
//    if (StivaleHeader->flags & BIT1) {
//        // requested pml5, check support
//        Pml5 = Check5LevelPagingSupport();
//    }

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
    } else {
        StivaleStruct->epoch = 0;
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
        CHECK(Module->Compressed, "TODO: implement compressed modules lol");
        EFI_PHYSICAL_ADDRESS FileBuffer = 0;
        UINTN FileSize = 0;
        EFI_CHECK(FileHandleGetSize(ModuleFile, &FileSize));
        EFI_CHECK(gBS->AllocatePages(AllocateAnyPages, EfiMemoryMappedIOPortSpace, EFI_SIZE_TO_PAGES(FileSize), &FileBuffer));

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
    // Lastly set the Graphics Mode
    //------------------------------------------------------------------------------------------------------------------

    UINTN Width = StivaleHeader->framebuffer_width, Height = StivaleHeader->framebuffer_height, Scanline;
    EFI_PHYSICAL_ADDRESS Framebuffer;
    CHECK_AND_RETHROW(SelectBestGopMode(&Width, &Height, &Scanline, &Framebuffer));

    // set the info
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

cleanup:
    ERROR("Should not have arrived here...");
    CpuDeadLoop();
    return Status;
}
