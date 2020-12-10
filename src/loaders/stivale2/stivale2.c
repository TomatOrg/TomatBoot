#include <Uefi.h>

#include <Guid/Acpi.h>

#include <Protocol/MpService.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/LocalApicLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/FileHandleLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>

#include <config/BootEntries.h>
#include <config/BootConfig.h>
#include <loaders/elf/ElfLoader.h>
#include <loaders/elf/elf64.h>
#include <loaders/mb2/gdt.h>
#include <loaders/Loaders.h>
#include <util/FileUtils.h>
#include <util/TimeUtils.h>
#include <util/GfxUtils.h>
#include <util/Except.h>
#include <Library/IoLib.h>

#include "stivale2.h"

static UINT32 EfiTypeToStivaleType[] = {
    [EfiReservedMemoryType] = STIVALE2_RESERVED,
    [EfiLoaderCode] = STIVALE2_BOOTLOADER_RECLAIMABLE,
    [EfiLoaderData] = STIVALE2_BOOTLOADER_RECLAIMABLE,
    [EfiBootServicesCode] = STIVALE2_BOOTLOADER_RECLAIMABLE,
    [EfiBootServicesData] = STIVALE2_BOOTLOADER_RECLAIMABLE,
    [EfiRuntimeServicesCode] = STIVALE2_RESERVED,
    [EfiRuntimeServicesData] = STIVALE2_RESERVED,
    [EfiConventionalMemory] = STIVALE2_USEABLE,
    [EfiUnusableMemory] = STIVALE2_BAD_MEMORY,
    [EfiACPIReclaimMemory] = STIVALE2_ACPI_RECLAIMABLE,
    [EfiACPIMemoryNVS] = STIVALE2_ACPI_NVS,
    [EfiMemoryMappedIO] = STIVALE2_RESERVED,
    [EfiMemoryMappedIOPortSpace] = STIVALE2_RESERVED,
    [EfiPalCode] = STIVALE2_RESERVED,
    [EfiPersistentMemory] = STIVALE2_RESERVED,
};

void NORETURN JumpToStivale2Kernel(STIVALE2_STRUCT* strct, UINT64 Stack, void* KernelEntry, BOOLEAN level5);

static EFI_STATUS LoadStivaleHeader(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FS, CHAR16* file, STIVALE2_HEADER* header, BOOLEAN* HigherHalf) {
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
        if (AsciiStrCmp(&names[shdr.sh_name], ".stivale2hdr") == 0) {
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

/**
 * Smp parameters
 */
extern UINT8 gSmpTrampoline[];
extern IA32_DESCRIPTOR gSmpTplGdt;
extern UINT32 gSmpTplTargetMode;
extern UINT32 gSmpTplPagemap;
extern UINT32 gSmpTplBootedFlag;
extern UINT64 gSmpTplInfoStruct;
extern UINT8 gSmpTrampolineEnd[];

/**
 * The gdt that will be used when booting
 * into the kernel
 */
extern IA32_DESCRIPTOR gGdtPtr;

EFI_STATUS EFIAPI EfiMain(IN EFI_HANDLE ImageHandle, IN EFI_SYSTEM_TABLE *SystemTable);

EFI_STATUS LoadStivale2Kernel(BOOT_ENTRY* Entry) {
    EFI_STATUS Status = EFI_SUCCESS;
    STIVALE2_HEADER Header = {0};
    ELF_INFO Elf = {0};
    BOOLEAN Level5Supported = FALSE;

    // load config
    BOOT_CONFIG config;
    LoadBootConfig(&config);

    // get the header and decide on higher half
    BOOLEAN HigherHalf = FALSE;
    CHECK_AND_RETHROW(LoadStivaleHeader(Entry->Fs, Entry->Path, &Header, &HigherHalf));
    if (HigherHalf) {
        Elf.VirtualOffset = 0xffffffff80000000;
    }

    // kaslr is not supported yet :(
    WARN_ON(Header.Flags & STIVALE2_HEADER_FLAG_KASLR, "KASLR Is not supported yet");

    // check if pml5 is supported
    UINT32 eax, ebx, ecx, edx;
    AsmCpuidEx(0x00000007, 0, &eax, &ebx, &ecx, &edx);
    if (ecx & BIT16) {
        Level5Supported = TRUE;
    }

    // fully-load the kernel
    CHECK_AND_RETHROW(LoadElf64(Entry->Fs, Entry->Path, &Elf));
    if (Header.EntryPoint != 0) {
        Elf.Entry = Header.EntryPoint;
    }

    // flags
    BOOLEAN Pml5Enabled = FALSE;
    STIVALE2_HEADER_TAG_FRAMEBUFFER* FramebufferReq = NULL;
    BOOLEAN RequestedSmp = FALSE;
    BOOLEAN Requestedx2Apic = FALSE;

    // iterate the tags
    STIVALE2_HDR_TAG* Tag = Header.Tags > (void*)0xffffffff80000000 ? Header.Tags - 0xffffffff80000000 : Header.Tags;
    while (Tag) {
        switch (Tag->Identifier) {
            case STIVALE2_HEADER_TAG_PML5_IDENT: {
                TRACE("\t* enabling PML5")
                Pml5Enabled = TRUE;
            } break;

            case STIVALE2_HEADER_TAG_FRAMEBUFFER_IDENT: {
                FramebufferReq = (STIVALE2_HEADER_TAG_FRAMEBUFFER*) Tag;
            } break;

            case STIVALE2_HEADER_TAG_SMP_IDENT: {
                STIVALE2_HEADER_TAG_SMP* Smp = (STIVALE2_HEADER_TAG_SMP*)Tag;
                WARN_ON(TRUE, "SMP boot is not fully supported yet");
                RequestedSmp = TRUE;
                Requestedx2Apic = Smp->Flags & STIVALE2_HEADER_TAG_SMP_FLAG_X2APIC;
            } break;
        }

        Tag = Tag->Next > (void*)0xffffffff80000000 ? Tag->Next - 0xffffffff80000000 : Tag->Next;
    }

    WARN_ON(FramebufferReq == NULL, "Text mode is not supported, forcing graphics mode");

    // choose the gfx mode, either the one in the config or choose the mode closest to
    // the requested mode by the kernel
    INT32 GfxMode = config.GfxMode;
    if (FramebufferReq != NULL && FramebufferReq->FramebufferWidth != 0 && FramebufferReq->FramebufferHeight != 0) {
        GfxMode = GetBestGfxMode(FramebufferReq->FramebufferWidth, FramebufferReq->FramebufferHeight);
    }

    // set graphics mode
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;
    ASSERT_EFI_ERROR(gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID**)&gop));
    ASSERT_EFI_ERROR(gop->SetMode(gop, (UINT32) GfxMode));

    // setup the struct
    STIVALE2_STRUCT* Struct = AllocateZeroPool(sizeof(STIVALE2_STRUCT));
    AsciiStrnCpy(Struct->BootloaderBrand, "TomatBoot-UEFI", sizeof(Struct->BootloaderBrand));
    AsciiStrnCpy(Struct->BootloaderVersion, __GIT_REVISION__, sizeof(Struct->BootloaderVersion));

    // cmdline
    TRACE("Setting cmdline");
    STIVALE2_STRUCT_TAG_CMDLINE* Cmdline = AllocateZeroPool(sizeof(STIVALE2_STRUCT_TAG_CMDLINE));
    Cmdline->Identifier = STIVALE2_STRUCT_TAG_CMDLINE_IDENT;
    Cmdline->Cmdline = AllocateReservedPool(StrLen(Entry->Cmdline) + 1);
    UnicodeStrToAsciiStr(Entry->Cmdline, (CHAR8*)Cmdline->Cmdline);
    Struct->Tags = Cmdline;

    // graphics info
    TRACE("Setting framebuffer info");
    STIVALE2_STRUCT_TAG_FRAMEBUFFER* Framebuffer = AllocateZeroPool(sizeof(STIVALE2_STRUCT_TAG_FRAMEBUFFER));
    Framebuffer->Identifier = STIVALE2_STRUCT_TAG_FRAMEBUFFER_IDENT;
    Framebuffer->FramebufferAddr = gop->Mode->FrameBufferBase;
    Framebuffer->FramebufferWidth = gop->Mode->Info->HorizontalResolution;
    Framebuffer->FramebufferHeight = gop->Mode->Info->VerticalResolution;
    Framebuffer->FramebufferPitch = gop->Mode->Info->PixelsPerScanLine * 4;
    Framebuffer->FramebufferBpp = gop->Mode->Info->PixelFormat;
    Framebuffer->MemoryModel = 1;
    Framebuffer->RedMaskSize = 8;
    Framebuffer->RedMaskShift = 0;
    Framebuffer->GreenMaskSize = 8;
    Framebuffer->GreenMaskShift = 8;
    Framebuffer->BlueMaskSize = 8;
    Framebuffer->BlueMaskShift = 16;
    Cmdline->Next = Framebuffer;

    // set the acpi table
    void** Next = &Framebuffer->Next;
    void* AcpiTable = NULL;
    if (!EFI_ERROR(EfiGetSystemConfigurationTable(&gEfiAcpi20TableGuid, &AcpiTable))) {
        STIVALE2_STRUCT_TAG_RSDP* Rsdp = AllocateZeroPool(sizeof(STIVALE2_STRUCT_TAG_RSDP));
        EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER* ActualRsdp = AcpiTable;
        Rsdp->Identifier = STIVALE2_STRUCT_TAG_RSDP_IDENT;
        Rsdp->Rsdp = AllocateReservedCopyPool(ActualRsdp->Length, ActualRsdp);
        *Next = Rsdp;
        Next = &Rsdp->Next;
    } else if (!EFI_ERROR(EfiGetSystemConfigurationTable(&gEfiAcpi10TableGuid, &AcpiTable))) {
        STIVALE2_STRUCT_TAG_RSDP* Rsdp = AllocateZeroPool(sizeof(STIVALE2_STRUCT_TAG_RSDP));
        Rsdp->Identifier = STIVALE2_STRUCT_TAG_RSDP_IDENT;
        Rsdp->Rsdp = AllocateReservedCopyPool(sizeof(EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER), AcpiTable);
        *Next = Rsdp;
        Next = &Rsdp->Next;
    } else {
        WARN("No ACPI table found");
    }

    TRACE("Setting firmware");
    STIVALE2_STRUCT_TAG_FIRMWARE* Firmware = AllocateZeroPool(sizeof(STIVALE2_STRUCT_TAG_FIRMWARE));
    Firmware->Identifier = STIVALE2_STRUCT_TAG_FIRMWARE_IDENT;
    Firmware->Flags = 0;
    *Next = Firmware;

    TRACE("Setting epoch");
    STIVALE2_STRUCT_TAG_EPOCH* Epoch = AllocateZeroPool(sizeof(STIVALE2_STRUCT_TAG_EPOCH));
    Epoch->Identifier = STIVALE2_STRUCT_TAG_EPOCH_IDENT;
    EFI_TIME Time = { 0 };
    EFI_CHECK(gRT->GetTime(&Time, NULL));
    Epoch->Epoch = GetUnixEpoch(Time.Second, Time.Minute, Time.Hour, Time.Day, Time.Month, Time.Year);
    Firmware->Next = Epoch;
    Next = &Epoch->Next;

    // push the modules
    STIVALE2_STRUCT_TAG_MODULES* Modules = NULL;
    if (!IsListEmpty(&Entry->BootModules)) {
        TRACE("Loading modules");
        UINTN ModulesCount = 0;
        for (LIST_ENTRY* Link = GetFirstNode(&Entry->BootModules); Link != &Entry->BootModules; Link = Link->ForwardLink) {
            ModulesCount++;
        }

        Modules = AllocateZeroPool(sizeof(STIVALE2_STRUCT_TAG_MODULES) + sizeof(STIVALE2_MODULE) * ModulesCount);
        Modules->Identifier = STIVALE2_STRUCT_TAG_MODULES_IDENT;
        Modules->ModuleCount = ModulesCount;
        Epoch->Next = Modules;
        Next = &Modules->Next;

        UINTN Index = 0;
        for (LIST_ENTRY* Link = Entry->BootModules.ForwardLink; Link != &Entry->BootModules; Link = Link->ForwardLink, Index++) {
            BOOT_MODULE* Module = BASE_CR(Link, BOOT_MODULE, Link);
            UINTN Start = 0;
            UINTN Size = 0;
            CHECK_AND_RETHROW(LoadBootModule(Module, &Start, &Size));

            STIVALE2_MODULE* NewModule = &Modules->Modules[Index];
            NewModule->Begin = Start;
            NewModule->End = Start + Size;
            UnicodeStrToAsciiStrS(Module->Tag, NewModule->String, sizeof(NewModule->String));
            TRACE("    Added %s (%s) -> %p - %p", Module->Tag, Module->Path, Start, Start + Size);
        }
    }

    // push smp info
    STIVALE2_STRUCT_TAG_SMP* Smp = NULL;
    EFI_PHYSICAL_ADDRESS SmpTplBase = 0x100000;
    if (RequestedSmp) {
        EFI_MP_SERVICES_PROTOCOL* MpServices = NULL;
        UINTN TotalCpuCount = 0;
        UINTN ValidCpuCount = 0;

        // allocate the smp trampoline address
        EFI_CHECK_LABEL(gBS->AllocatePages(AllocateMaxAddress, EfiBootServicesCode, EFI_SIZE_TO_PAGES(gSmpTrampolineEnd - gSmpTrampoline), &SmpTplBase), invalid_cpu_info);

        // TODO: before version 0x0001000A of the PI specification there was a different protocol
        //       that gave about the same information, we might wanna use it if we ever find such
        //       a machine that has no new MpServices
        EFI_CHECK_LABEL(gBS->LocateProtocol(&gEfiMpServiceProtocolGuid, NULL, (void**)&MpServices), invalid_cpu_info);
        EFI_CHECK_LABEL(MpServices->GetNumberOfProcessors(MpServices, &TotalCpuCount, &ValidCpuCount), invalid_cpu_info);

        // TODO: check if x2apic is supported and adjust Requestedx2Apic accordingly
        Requestedx2Apic = FALSE;

        // enable apic and program it
        Smp = AllocateZeroPool(sizeof(STIVALE2_STRUCT_TAG_SMP) + sizeof(STIVALE2_SMP_INFO) * ValidCpuCount);
        Smp->Identifier = STIVALE2_STRUCT_TAG_SMP_IDENT;
        Smp->CpuCount = 0;
        Smp->Flags = GetApicMode() == Requestedx2Apic ? STIVALE2_HEADER_TAG_SMP_FLAG_X2APIC : 0;

        for (int i = 0; i < TotalCpuCount; i++) {
            EFI_PROCESSOR_INFORMATION Cpu = {0};
            EFI_CHECK_LABEL(MpServices->GetProcessorInfo(MpServices, i, &Cpu), invalid_cpu_info);

            if (!(Cpu.StatusFlag & PROCESSOR_HEALTH_STATUS_BIT)) {
                WARN("Cpu #%d is not healthy, ignoring", i);
            }

            // set the lapic id
            Smp->SmpInfo[Smp->CpuCount].LapicId = Cpu.ProcessorId;

            if (Cpu.StatusFlag & PROCESSOR_AS_BSP_BIT) {
                // this is bsp, set bsp id
                TRACE("Cpu #%d is BSP (lapic = %d)", i, Cpu.ProcessorId);
                Smp->BspLapicId = Cpu.ProcessorId;
            } else {
                // we will start this apic later
                TRACE("Cpu #%d (lapic = %d)", i, Cpu.ProcessorId);
            }

            // next
            Smp->CpuCount++;
        }

        // link it
        *Next = Smp;
        Next = &Smp->Next;

        // we got it, skip the failure
        goto got_cpu_info;

    invalid_cpu_info:
        if (Smp != NULL) {
            FreePool(Smp);
            Smp = NULL;
        }

        // TODO: free SmpTplBase

    got_cpu_info:
        (void)0;
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
    UINT64* Pml3High = AllocatePages(1);
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
    STIVALE2_STRUCT_TAG_MEMMAP* Memmap = AllocateZeroPool(sizeof(STIVALE2_STRUCT_TAG_MEMMAP) + (MemoryMapSize / DescriptorSize) * sizeof(STIVALE2_MMAP_ENTRY));
    Memmap->Identifier = STIVALE2_STRUCT_TAG_MEMMAP_IDENT;
    STIVALE2_MMAP_ENTRY* StartFrom = Memmap->Memmap;
    *Next = Memmap;

    // call it
    EFI_CHECK(gBS->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion));
    UINTN EntryCount = (MemoryMapSize / DescriptorSize);

    // Exit the memory services, we put fences in here to make sure the compiler
    // won't try to mix boot services code and non-bootservices code
    MemoryFence();
    EFI_CHECK(gBS->ExitBootServices(gImageHandle, MapKey));
    MemoryFence();

    // no interrupts
    DisableInterrupts();

    // setup the local apic
    if (Smp != NULL) {
        InitializeLocalApicSoftwareEnable(TRUE);
        ProgramVirtualWireMode();
        if (Requestedx2Apic) {
            SetApicMode(LOCAL_APIC_MODE_X2APIC);
        } else {
            SetApicMode(LOCAL_APIC_MODE_XAPIC);
        }

        // set the global info like gdt descriptor and the pagemap
        gSmpTplGdt = gGdtPtr;
        gSmpTplPagemap = AsmReadCr3();

        // set the optional stuff
        gSmpTplTargetMode = 0;
        if (Pml5Enabled && Level5Supported) {
            gSmpTplTargetMode |= (1 << 1);
        }
        if (Requestedx2Apic) {
            gSmpTplTargetMode |= (1 << 2);
        }

        // allocate space for the trampoline
        MemoryFence();
        CopyMem((void*)SmpTplBase, gSmpTrampoline, gSmpTrampolineEnd - gSmpTrampoline);
        MemoryFence();

        // calculate the physical addresses
        _Atomic(UINT32)* SmpTplBootedFlag = (_Atomic(UINT32)*)((UINT8*)SmpTplBase + ((UINTN)&gSmpTplBootedFlag - (UINTN)gSmpTrampoline));
        UINT64* SmpTplInfoStruct = (UINT64*)((UINT8*)SmpTplBase + ((UINTN)&gSmpTplInfoStruct - (UINTN)gSmpTrampoline));

        // now start all aps
        for (int i = 0; i < Smp->CpuCount; i++) {
            // don't send to bsp lol
            if (Smp->BspLapicId == Smp->SmpInfo[i].LapicId) {
                continue;
            }

            // set the params
            *SmpTplBootedFlag = 0;
            *SmpTplInfoStruct = (UINT64)&Smp->SmpInfo[i];

            // don't run this until the flags are set
            MemoryFence();
            SendInitSipiSipi(Smp->SmpInfo[i].LapicId, SmpTplBase);

            // wait until it is done and we can get to
            // the next one
            while (*SmpTplBootedFlag == 0) {
                CpuPause();
            }
        }
    }

    // initialize all the cores

    // setup the normal memory map
    Memmap->Entries = 0;
    int LastType = -1;
    UINTN LastEnd = 0xFFFFFFFFFFFF;
    for (int i = 0; i < EntryCount; i++) {
        EFI_MEMORY_DESCRIPTOR* Desc = (EFI_MEMORY_DESCRIPTOR*)((UINTN)MemoryMap + DescriptorSize * i);
        EFI_PHYSICAL_ADDRESS PhysicalBase = Desc->PhysicalStart;
        UINTN Length = EFI_PAGES_TO_SIZE(Desc->NumberOfPages);
        EFI_PHYSICAL_ADDRESS PhysicalEnd = Desc->PhysicalStart + Length;

        int Type = STIVALE2_RESERVED;
        if (Desc->Type < EfiMaxMemoryType) {
            Type = EfiTypeToStivaleType[Desc->Type];
        }

        // don't include bootloader in kernel and modules
        if (Type == STIVALE2_KERNEL_AND_MODULES) {
            BOOLEAN IsKernelOrModules = FALSE;
            if (Elf.PhysicalBase == PhysicalBase) {
                IsKernelOrModules = TRUE;
            } else if (Modules != NULL) {
                for (int j = 0; j < Modules->ModuleCount; j++) {
                    if (Modules->Modules[j].Begin == PhysicalBase) {
                        IsKernelOrModules = TRUE;
                    }
                }
            }

            if (!IsKernelOrModules) {
                Type = STIVALE2_BOOTLOADER_RECLAIMABLE;
            }
        }

        // check if we can merge
        if (LastType == Type && LastEnd == Desc->PhysicalStart) {
            StartFrom[-1].Length += Length;
        } else {
            StartFrom->Type = Type;
            StartFrom->Length = Length;
            StartFrom->Base = PhysicalBase;
            StartFrom->Unused = 0;
            LastType = Type;
            StartFrom++;
            Memmap->Entries++;
        }
        LastEnd = PhysicalEnd;
    }

    JumpToStivale2Kernel(Struct, Header.Stack, (void*)Elf.Entry, Pml5Enabled && Level5Supported);

cleanup:
    return Status;
}
