#include <ProcessorBind.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/PrintLib.h>
#include "StringUtils.h"
#include "Except.h"

CHAR16* StrDup(CHAR16* Original) {
    ASSERT(Original != NULL);
    return AllocateCopyPool(StrSize(Original), Original);
}

CHAR8* ConvertToAscii(CHAR16* Original) {
    ASSERT(Original != NULL);
    UINTN CmdlineLen = StrLen(Original) + 1;
    CHAR8* Cmdline = AllocateZeroPool(CmdlineLen);
    if (Cmdline == NULL) {
        return NULL;
    }
    UnicodeStrToAsciiStrS(Original, Cmdline, CmdlineLen);
    return Cmdline;
}

const char* EfiMemoryTypeToString(EFI_MEMORY_TYPE Type) {
    static CHAR8 TempBuffer[256] = {0};
    switch (Type) {
        case EfiReservedMemoryType: return "EfiReservedMemoryType";
        case EfiLoaderCode: return "EfiLoaderCode";
        case EfiLoaderData: return "EfiLoaderData";
        case EfiBootServicesCode: return "EfiBootServicesCode";
        case EfiBootServicesData: return "EfiBootServicesData";
        case EfiRuntimeServicesCode: return "EfiRuntimeServicesCode";
        case EfiRuntimeServicesData: return "EfiRuntimeServicesData";
        case EfiConventionalMemory: return "EfiConventionalMemory";
        case EfiUnusableMemory: return "EfiUnusableMemory";
        case EfiACPIReclaimMemory: return "EfiACPIReclaimMemory";
        case EfiACPIMemoryNVS: return "EfiACPIMemoryNVS";
        case EfiMemoryMappedIO: return "EfiMemoryMappedIO";
        case EfiMemoryMappedIOPortSpace: return "EfiMemoryMappedIOPortSpace (kernel/modules marking)";
        case EfiPalCode: return "EfiPalCode";
        case EfiPersistentMemory: return "EfiPersistentMemory";
        default:
            AsciiSPrint(TempBuffer, ARRAY_SIZE(TempBuffer), "<EFI_MEMORY_TYPE: %d>", Type);
            return TempBuffer;
    }
}
