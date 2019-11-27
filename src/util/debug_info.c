#include <Base.h>
#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>
#include <Library/DebugLib.h>
#include <Library/PrintLib.h>

#include "debug_info.h"

static char buffer[1024] = {0};

static void fprintf(EFI_FILE_PROTOCOL* file, const char* fmt, ...) {
    VA_LIST va;
    VA_START(va, fmt);
    UINTN size = AsciiVSPrint(buffer, sizeof(buffer), fmt, va);
    VA_END(va);

    ASSERT_EFI_ERROR(file->Write(file, &size, buffer));
}

static const char* MEMORY_TYPE_NAMES[] = {
    "EfiReservedMemoryType",
    "EfiLoaderCode",
    "EfiLoaderData",
    "EfiBootServicesCode",
    "EfiBootServicesData",
    "EfiRuntimeServicesCode",
    "EfiRuntimeServicesData",
    "EfiConventionalMemory",
    "EfiUnusableMemory",
    "EfiACPIReclaimMemory",
    "EfiACPIMemoryNVS",
    "EfiMemoryMappedIO",
    "EfiMemoryMappedIOPortSpace",
    "EfiPalCode",
};

void save_debug_info() {
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* filesystem = NULL;
    ASSERT_EFI_ERROR(gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID**)&filesystem));

    EFI_FILE_PROTOCOL* root = NULL;
    ASSERT_EFI_ERROR(filesystem->OpenVolume(filesystem, &root));

    EFI_FILE_PROTOCOL* file = NULL;
    ASSERT_EFI_ERROR(root->Open(root, &file, L"tomato_debug_info.txt", EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0));

    // general information
    fprintf(file, "TomatBoot debug information:\n");
    fprintf(file, "\tUEFI Version: %d.%d\n", (gST->Hdr.Revision >> 16u) & 0xFFFFu, gST->Hdr.Revision & 0xFFFFu);
    fprintf(file, "\tsave_debug_info: %p\n", save_debug_info);
    fprintf(file, "\tSystem Table: %p\n", gST);
    fprintf(file, "\tRuntime Services: %p\n", gRT);
    fprintf(file, "\tBoot Services: %p\n", gBS);

    UINTN mapSize = 0;
    UINTN mapKey = 0;
    UINTN descSize = 0;
    UINT32 descVersion = 0;
    gBS->GetMemoryMap(&mapSize, NULL, NULL, &descSize, NULL);
    mapSize += 64 * descSize; // take into account some changes after exiting boot services
    EFI_MEMORY_DESCRIPTOR* descs = NULL;
    ASSERT_EFI_ERROR(gBS->AllocatePages(AllocateAnyPages, EfiBootServicesData, EFI_SIZE_TO_PAGES(mapSize), (EFI_PHYSICAL_ADDRESS*)&descs));
    ASSERT_EFI_ERROR(gBS->GetMemoryMap(&mapSize, descs, &mapKey, &descSize, &descVersion));

    fprintf(file, "\tMemory map:\n");
    EFI_MEMORY_DESCRIPTOR* desc = descs;
    for(int i = 0; i < mapSize / descSize; i++) {
        const char* type_name;

        if(ARRAY_SIZE(MEMORY_TYPE_NAMES) >= desc->Type) {
            type_name = MEMORY_TYPE_NAMES[desc->Type];
        }else if(0x70000000 <= desc->Type && desc->Type <= 0x7FFFFFFF) {
            type_name = "reserved for OEM";
        }else if(0x80000000 <= desc->Type && desc->Type <= 0xFFFFFFFF) {
            type_name = "reserved for UEFI OS loaders";
        }else {
            type_name = "Invalid";
        }

        fprintf(file, "\t\t%016p-%016p: %a (0x%x)\n", desc->PhysicalStart, desc->PhysicalStart + desc->NumberOfPages * EFI_PAGE_SIZE, type_name, desc->Type);

        // next
        desc = (EFI_MEMORY_DESCRIPTOR*)((UINTN)desc + descSize);
    }

    ASSERT_EFI_ERROR(gBS->FreePages((EFI_PHYSICAL_ADDRESS)descs, EFI_SIZE_TO_PAGES(mapSize)));

    // close everything
    ASSERT_EFI_ERROR(file->Flush(file));
    ASSERT_EFI_ERROR(file->Close(file));
    ASSERT_EFI_ERROR(root->Close(root));
}
