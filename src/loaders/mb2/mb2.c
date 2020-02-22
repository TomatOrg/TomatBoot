#include "multiboot2.h"
#include "gdt.h"

#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>
#include <Library/DebugLib.h>
#include <Library/FileHandleLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <config/BootEntries.h>
#include <util/FileUtils.h>
#include <util/Except.h>
#include <config/BootConfig.h>
#include <loaders/Loaders.h>
#include <Guid/Acpi.h>
#include <loaders/elf/ElfLoader.h>
#include <Library/UefiRuntimeLib.h>
#include <Library/CpuLib.h>
#include <util/DrawUtils.h>

static UINT8* mBootParamsBuffer = NULL;
static UINTN mBootParamsSize = 0;

/**
 * Will jump to the mb2 kernel
 */
extern void JumpToMB2Kernel(void* KernelStart, void* KernelParams);

// TODO: need to make this force allocations below 4GB
static void* PushBootParams(void* data, UINTN size) {
    // align to 8 bytes
    UINTN AllocationSize = ALIGN_VALUE(size, MULTIBOOT_TAG_ALIGN);

    // allocate the buffer
    if (mBootParamsBuffer == NULL) {
        mBootParamsBuffer = AllocatePool(AllocationSize);
    } else {
        UINT8* old = mBootParamsBuffer;
        mBootParamsBuffer = AllocateCopyPool(mBootParamsSize + AllocationSize, mBootParamsBuffer);
        FreePool(old);
    }

    // copy to the buffer
    if (data != NULL) {
        CopyMem(mBootParamsBuffer + mBootParamsSize, data, size);
    }

    // save the base we allocated from
    UINT8* base = mBootParamsBuffer + mBootParamsSize;

    // add to the size
    mBootParamsSize += AllocationSize;

    // return the base of the new tag
    return base;
}

static multiboot_uint32_t EfiTypeToMB2Type[] = {
    [EfiReservedMemoryType] = MULTIBOOT_MEMORY_RESERVED,
    [EfiRuntimeServicesCode] = MULTIBOOT_MEMORY_RESERVED,
    [EfiRuntimeServicesData] = MULTIBOOT_MEMORY_RESERVED,
    [EfiMemoryMappedIO] = MULTIBOOT_MEMORY_RESERVED,
    [EfiMemoryMappedIOPortSpace] = MULTIBOOT_MEMORY_RESERVED,
    [EfiPalCode] = MULTIBOOT_MEMORY_RESERVED,
    [EfiUnusableMemory] = MULTIBOOT_MEMORY_BADRAM,
    [EfiACPIReclaimMemory] = MULTIBOOT_MEMORY_ACPI_RECLAIMABLE,
    [EfiLoaderCode] = MULTIBOOT_MEMORY_AVAILABLE,
    [EfiLoaderData] = MULTIBOOT_MEMORY_AVAILABLE,
    [EfiBootServicesCode] = MULTIBOOT_MEMORY_AVAILABLE,
    [EfiBootServicesData] = MULTIBOOT_MEMORY_AVAILABLE,
    [EfiConventionalMemory] = MULTIBOOT_MEMORY_AVAILABLE,
    [EfiACPIMemoryNVS] = MULTIBOOT_MEMORY_NVS
};

static struct multiboot_header* LoadMB2Header(CHAR16* file, UINTN* headerOff) {
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* filesystem = NULL;
    EFI_FILE_PROTOCOL* root = NULL;
    EFI_FILE_PROTOCOL* mb2image = NULL;
    struct multiboot_header* ptr = NULL;

    // open the executable file
    Print(L"Loading image `%s`\n", file);
    EFI_CHECK(gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID**)&filesystem));
    EFI_CHECK(filesystem->OpenVolume(filesystem, &root));
    EFI_CHECK(root->Open(root, &mb2image, file, EFI_FILE_MODE_READ, 0));

    Print(L"Searching for mb2 header\n");
    struct multiboot_header header;
    for (int i = 0; i < MULTIBOOT_SEARCH; i += MULTIBOOT_HEADER_ALIGN) {
        CHECK_AND_RETHROW(FileRead(mb2image, &header, sizeof(header), i));

        // check if this is a valid header
        if (header.magic == MULTIBOOT2_HEADER_MAGIC &&
            header.architecture == MULTIBOOT_ARCHITECTURE_I386 &&
            (header.checksum + header.magic + header.architecture + header.header_length) == 0) {

            // set the offset
            *headerOff = i;

            // found it, allocate something big enough for the whole header and return it
            ptr = AllocatePool(header.header_length);
            CHECK_AND_RETHROW(FileRead(mb2image, ptr, header.header_length, i));
            goto cleanup;
        }
    }

cleanup:
    if (root != NULL) {
        FileHandleClose(root);
    }

    if (mb2image != NULL) {
        FileHandleClose(mb2image);
    }

    return ptr;
}

EFI_STATUS LoadMB2(BOOT_ENTRY* Entry) {
    EFI_STATUS Status = EFI_SUCCESS;
    UINTN HeaderOffset = 0;

    gST->ConOut->ClearScreen(gST->ConOut);
    gST->ConOut->SetCursorPosition(gST->ConOut, 0, 0);

    // load config
    BOOT_CONFIG config;
    LoadBootConfig(&config);

    // set graphics mode right away
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop = NULL;
    ASSERT_EFI_ERROR(gBS->LocateProtocol(&gEfiGraphicsOutputProtocolGuid, NULL, (VOID**)&gop));
    ASSERT_EFI_ERROR(gop->SetMode(gop, (UINT32) config.GfxMode));

    // get the header
    struct multiboot_header* header = LoadMB2Header(Entry->Path, &HeaderOffset);
    CHECK_ERROR_TRACE(header != NULL, EFI_NOT_FOUND, "Could not find a valid multiboot2 header!");
    Print(L"Found header at offset %d\n", HeaderOffset);

    // some info we extract
    UINTN EntryAddressOverride = 0;
    BOOLEAN MustHaveOldAcpi = FALSE;
    BOOLEAN MustHaveNewAcpi = FALSE;
    BOOLEAN NotElf = FALSE;

    // push the size and something else
    mBootParamsSize = 8;
    mBootParamsBuffer = AllocatePool(8);

    // iterate the entries
    for (struct multiboot_header_tag* tag = (struct multiboot_header_tag*)(header + 1);
         tag < (struct multiboot_header_tag*)((UINTN)header + header->header_length) && tag->type != MULTIBOOT_HEADER_TAG_END;
         tag = (struct multiboot_header_tag*)((UINTN)tag + ALIGN_VALUE(tag->size, MULTIBOOT_TAG_ALIGN))) {

        switch (tag->type) {
            case MULTIBOOT_HEADER_TAG_INFORMATION_REQUEST:
                // iterate the requests and check if they are supported or not
                struct multiboot_header_tag_information_request* request = (void*)tag;
                for (int i = 0;
                    i < (request->size - OFFSET_OF(struct multiboot_header_tag_information_request, requests)) / sizeof(multiboot_uint32_t);
                    i++) {
                    multiboot_uint32_t r = request->requests[i];
                    switch(r) {
                        // stuff we support, so no need to check
                        case MULTIBOOT_TAG_TYPE_CMDLINE:
                        case MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME:
                        case MULTIBOOT_TAG_TYPE_MODULE:
                        case MULTIBOOT_TAG_TYPE_MMAP:
                        case MULTIBOOT_TAG_TYPE_EFI_MMAP:
                        case MULTIBOOT_TAG_TYPE_FRAMEBUFFER:
                        case MULTIBOOT_TAG_TYPE_ELF_SECTIONS:
                            break;

                        // stuff that we might not have so fail if doesn't have
                        case MULTIBOOT_TAG_TYPE_ACPI_OLD: MustHaveOldAcpi = !(tag->flags & MULTIBOOT_HEADER_TAG_OPTIONAL); break;
                        case MULTIBOOT_TAG_TYPE_ACPI_NEW: MustHaveNewAcpi = !(tag->flags & MULTIBOOT_HEADER_TAG_OPTIONAL); break;

                        // if got an unknown type then make sure it is optional
                        default:
                            CHECK_ERROR_TRACE(tag->flags & MULTIBOOT_HEADER_TAG_OPTIONAL, EFI_UNSUPPORTED, "Requested tag %d which is unsupported!", r);
                    }
                }
                break;

            case MULTIBOOT_HEADER_TAG_ADDRESS: {
                NotElf = TRUE;
            } break;

            case MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS: {
                struct multiboot_header_tag_entry_address* entry_address = (void*)tag;
                EntryAddressOverride = entry_address->entry_addr;
            } break;

            case MULTIBOOT_HEADER_TAG_CONSOLE_FLAGS: {
                struct multiboot_header_tag_console_flags* console_flags = (void*)tag;
                if (console_flags->console_flags & MULTIBOOT_CONSOLE_FLAGS_CONSOLE_REQUIRED) {
                    CHECK_FAIL_TRACE("We do not support text mode");
                }
            } break;

            case MULTIBOOT_HEADER_TAG_FRAMEBUFFER: {
                /*
                 * we are gonna ignore the framebuffer configurations and use our own
                 * configurations from the boot menu
                 */
            } break;

            case MULTIBOOT_HEADER_TAG_MODULE_ALIGN: {
                /*
                 * We always align modules
                 */
            } break;

            case MULTIBOOT_HEADER_TAG_EFI_BS: {
                if (!(tag->flags & MULTIBOOT_HEADER_TAG_OPTIONAL)) {
                    CHECK_FAIL_TRACE("We do not support passing boot services");
                }
            } break;

            case MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS_EFI32:  {
                if (!(tag->flags & MULTIBOOT_HEADER_TAG_OPTIONAL)) {
                    CHECK_FAIL_TRACE("We do not support EFI boot for i386");
                }
            } break;

            case MULTIBOOT_HEADER_TAG_ENTRY_ADDRESS_EFI64: {
                if (!(tag->flags & MULTIBOOT_HEADER_TAG_OPTIONAL)) {
                    CHECK_FAIL_TRACE("We do not support EFI boot for x86_64");
                }
            } break;

            case MULTIBOOT_HEADER_TAG_RELOCATABLE: {
                if (!(tag->flags & MULTIBOOT_HEADER_TAG_OPTIONAL)) {
                    // TODO: we probably wanna support this
                    CHECK_FAIL_TRACE("We do not support ELF relocations");
                }
            } break;

            default:
                CHECK_FAIL_TRACE("Invalid tag type %d", tag->type);
        }
    }

    // push the command line
    {
        Print(L"Pushing cmdline\n");
        UINTN size = StrLen(Entry->Cmdline) + 1 + OFFSET_OF(struct multiboot_tag_string, string);
        struct multiboot_tag_string* string = PushBootParams(NULL, size);
        string->type = MULTIBOOT_TAG_TYPE_CMDLINE;
        string->size = size;
        UnicodeStrToAsciiStr(Entry->Cmdline, string->string);
    }

    // push the bootloader name
    {
        Print(L"Pushing bootloader name\n");
        UINTN size = sizeof("TomatBoot v2 UEFI") + OFFSET_OF(struct multiboot_tag_string, string);
        struct multiboot_tag_string* string = PushBootParams(NULL, size);
        string->type = MULTIBOOT_TAG_TYPE_BOOT_LOADER_NAME;
        string->size = size;
        AsciiStrCpy(string->string, "TomatBoot v2 UEFI");
    }

    // push the modules
    Print(L"Pushing modules\n");
    for (LIST_ENTRY* Link = Entry->BootModules.ForwardLink; Link != &Entry->BootModules; Link = Link->ForwardLink) {
        BOOT_MODULE* Module = BASE_CR(Link, BOOT_MODULE, Link);
        UINTN Start = 0;
        UINTN Size = 0;
        CHECK_AND_RETHROW(LoadBootModule(Module, &Start, &Size));

        UINTN TotalTagSize = OFFSET_OF(struct multiboot_tag_module, cmdline) + StrLen(Module->Tag) + 1;
        struct multiboot_tag_module* mod = PushBootParams(NULL, TotalTagSize);
        mod->size = TotalTagSize;
        mod->type = MULTIBOOT_TAG_TYPE_MODULE;
        mod->mod_start = Start;
        mod->mod_end = Start + Size;
        UnicodeStrToAsciiStr(Module->Tag, mod->cmdline);

        Print(L"    Added %s (%s)\n", Module->Tag, Module->Path);
    }

    // push framebuffer info
    Print(L"Pushing framebuffer info\n");
    struct multiboot_tag_framebuffer framebuffer = {
        .common = {
            .type = MULTIBOOT_TAG_TYPE_FRAMEBUFFER,
            .size = sizeof(struct multiboot_tag_framebuffer),
            .framebuffer_addr = gop->Mode->FrameBufferBase,
            .framebuffer_pitch = gop->Mode->Info->PixelsPerScanLine * 4,
            .framebuffer_width = gop->Mode->Info->HorizontalResolution,
            .framebuffer_height = gop->Mode->Info->VerticalResolution,
            .framebuffer_bpp = 32,
            .framebuffer_type = MULTIBOOT_FRAMEBUFFER_TYPE_RGB
        },
        .framebuffer_red_field_position = 16,
        .framebuffer_red_mask_size = 8,
        .framebuffer_green_field_position = 8,
        .framebuffer_green_mask_size = 8,
        .framebuffer_blue_field_position = 0,
        .framebuffer_blue_mask_size = 8
    };
    PushBootParams(&framebuffer, sizeof(framebuffer));

    // push the old acpi table if has it
    void* acpi10table;
    if (!EFI_ERROR(EfiGetSystemConfigurationTable(&gEfiAcpi10TableGuid, &acpi10table))) {
        // RSDP is 20 bytes long
        Print(L"Pushing old ACPI info\n");
        struct multiboot_tag_old_acpi* old_acpi = PushBootParams(NULL, 20 + OFFSET_OF(struct multiboot_tag_old_acpi, rsdp));
        old_acpi->size = 20 + OFFSET_OF(struct multiboot_tag_old_acpi, rsdp);
        old_acpi->type = MULTIBOOT_TAG_TYPE_ACPI_OLD;
        CopyMem(old_acpi->rsdp, acpi10table, 20);
    } else if (MustHaveOldAcpi) {
        CHECK_FAIL_TRACE("Old ACPI Table is not present");
    }

    // push the new acpi table if has it
    void* acpi20table;
    if (!EFI_ERROR(EfiGetSystemConfigurationTable(&gEfiAcpi20TableGuid, &acpi20table))) {
        // XSDP is 36 bytes long
        Print(L"Pushing new ACPI info\n");
        struct multiboot_tag_new_acpi* new_acpi = PushBootParams(NULL, 36 + OFFSET_OF(struct multiboot_tag_new_acpi, rsdp));
        new_acpi->size = 36 + OFFSET_OF(struct multiboot_tag_new_acpi, rsdp);
        new_acpi->type = MULTIBOOT_TAG_TYPE_ACPI_NEW;
        CopyMem(new_acpi->rsdp, acpi20table, 20);
    } else if (MustHaveNewAcpi) {
        CHECK_FAIL_TRACE("New ACPI Table is not present");
    }

    // load the elf
    if (NotElf) {
        // TODO: Load raw image
        CHECK_FAIL_TRACE("Raw image is not supported yet");
    } else {
        ELF_INFO elf_info;

        // try with 32bit elf
        Print(L"Trying to load ELF32\n");
        if (EFI_ERROR(LoadElf32(Entry->Path, &elf_info))) {
            // try with elf64
            Print(L"Not ELF32, trying ELF64\n");
            CHECK_AND_RETHROW(LoadElf64(Entry->Path, &elf_info));
        }

        // push elf info
        Print(L"Pushing ELF info\n");
        UINTN Size = OFFSET_OF(struct multiboot_tag_elf_sections, sections) + elf_info.SectionHeadersSize;
        struct multiboot_tag_elf_sections* sections = PushBootParams(NULL, Size);
        sections->size = Size;
        sections->type = MULTIBOOT_TAG_TYPE_ELF_SECTIONS;
        sections->entsize = elf_info.SectionEntrySize;
        sections->num = elf_info.SectionHeadersSize / elf_info.SectionEntrySize;
        sections->shndx = elf_info.StringSectionIndex;
        CopyMem(sections->sections, elf_info.SectionHeaders, elf_info.SectionHeadersSize);

        if (EntryAddressOverride == 0) {
            EntryAddressOverride = elf_info.Entry;
        }
    }

    // allocate the needed space for gdt
    Print(L"Allocating area for GDT\n");
    InitLinuxDescriptorTables();

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    // No prints from here
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // setup the mmap information
    UINT8 TmpMemoryMap[1];
    UINTN MemoryMapSize = sizeof(TmpMemoryMap);
    UINTN MapKey;
    UINTN DescriptorSize;
    UINT32 DescriptorVersion;
    CHECK(gBS->GetMemoryMap(&MemoryMapSize, (EFI_MEMORY_DESCRIPTOR *) TmpMemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion
    ) == EFI_BUFFER_TOO_SMALL);

    // allocate space for the efi mmap and take into
    // account that there will be changes
    MemoryMapSize += EFI_PAGE_SIZE;
    EFI_MEMORY_DESCRIPTOR* MemoryMap = AllocatePool(MemoryMapSize);

    // allocate all the space we will need (hopefully)
    UINT8* start_from = PushBootParams(NULL, OFFSET_OF(struct multiboot_tag_efi_mmap, efi_mmap) + MemoryMapSize * OFFSET_OF(struct multiboot_tag_mmap, entries) + MemoryMapSize);

    // call it
    EFI_CHECK(gBS->GetMemoryMap(&MemoryMapSize, MemoryMap, &MapKey, &DescriptorSize, &DescriptorVersion));
    UINTN EntryCount = (MemoryMapSize / DescriptorSize);

    // Exit the memory services
    EFI_CHECK(gBS->ExitBootServices(gImageHandle, MapKey));

    // setup the normal memory map
    struct multiboot_tag_mmap* mmap = (void*)start_from;
    mmap->type = MULTIBOOT_TAG_TYPE_MMAP;
    mmap->entry_size = sizeof(struct multiboot_mmap_entry);
    mmap->entry_version = 0;
    mmap->size = OFFSET_OF(struct multiboot_tag_mmap, entries) + EntryCount * sizeof(struct multiboot_mmap_entry);
    for (int i = 0; i < EntryCount; i++) {
        struct multiboot_mmap_entry* entry = &mmap->entries[i];
        EFI_MEMORY_DESCRIPTOR* desc = &MemoryMap[i];
        entry->type = EfiTypeToMB2Type[desc->Type];
        entry->addr = desc->PhysicalStart;
        entry->len = EFI_PAGES_TO_SIZE(desc->NumberOfPages);
        entry->zero = 0;
    }

    // setup the efi memory type
    struct multiboot_tag_efi_mmap* efi_mmap = (struct multiboot_tag_efi_mmap*)ALIGN_VALUE((UINTN)mmap + mmap->size, MULTIBOOT_TAG_ALIGN);
    efi_mmap->size = MemoryMapSize + OFFSET_OF(struct multiboot_tag_efi_mmap, efi_mmap);
    efi_mmap->type = MULTIBOOT_TAG_TYPE_EFI_MMAP;
    efi_mmap->descr_size = DescriptorSize;
    efi_mmap->descr_vers = DescriptorVersion;
    CopyMem(efi_mmap->efi_mmap, MemoryMap, MemoryMapSize);

    // append the end tag now
    struct multiboot_tag* end_tag = (struct multiboot_tag*)ALIGN_VALUE((UINTN)efi_mmap + efi_mmap->size, MULTIBOOT_TAG_ALIGN);
    end_tag->type = MULTIBOOT_TAG_TYPE_END;
    end_tag->size = sizeof(struct multiboot_tag);

    // no interrupts
    DisableInterrupts();

    // setup GDT and IDT
    SetLinuxDescriptorTables();

    // jump to the kernel
    JumpToMB2Kernel((void*)EntryAddressOverride, mBootParamsBuffer);

    // if we ever return sleep
    while(1) CpuSleep();

cleanup:
    if (header != NULL) {
        FreePool(header);
    }

    return Status;
}
