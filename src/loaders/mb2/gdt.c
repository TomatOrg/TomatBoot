#include "gdt.h"

#include <Uefi.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

typedef struct _GDT_ENTRY {
    UINT16 Limit15_0;
    UINT16 Base15_0;
    UINT8  Base23_16;
    UINT8  Type;
    UINT8  Limit19_16_and_flags;
    UINT8  Base31_24;
} GDT_ENTRY;

typedef struct _GDT_ENTRIES {
    GDT_ENTRY Null;
    GDT_ENTRY Linear;
    GDT_ENTRY LinearCode;
} GDT_ENTRIES;

GDT_ENTRIES GdtTemplate = {
        //
        // Null
        //
        .Null = {
                0x0,            // limit 15:0
                0x0,            // base 15:0
                0x0,            // base 23:16
                0x0,            // type
                0x0,            // limit 19:16, flags
                0x0,            // base 31:24
        },
        .Linear = {
                0x0FFFF,        // limit 0xFFFFF
                0x0,            // base 0
                0x0,
                0x09A,          // present, ring 0, data, expand-up, writable
                0x0CF,          // page-granular, 32-bit
                0x0,
        },
        .LinearCode = {
                0x0FFFF,        // limit 0xFFFFF
                0x0,            // base 0
                0x0,
                0x092,          // present, ring 0, data, expand-up, writable
                0x0CF,          // page-granular, 32-bit
                0x0,
        }
};

void SetupMB2Gdt() {
    // allocate an aligned gdt
    UINTN gdt = 0;
    ASSERT_EFI_ERROR(gBS->AllocatePool(EfiRuntimeServicesData, sizeof (GdtTemplate) + 8, (void**)&gdt));
    ASSERT(gdt != 0);
    gdt = ALIGN_VALUE(gdt, 8);
    CopyMem((void*)gdt, &GdtTemplate, sizeof(GdtTemplate));

    // setup the descriptors
    IA32_DESCRIPTOR GdtPtr = {
        .Base = (UINTN)gdt,
        .Limit = sizeof(GdtTemplate) - 1
    };

    IA32_DESCRIPTOR IdtPtr = {
        .Base = 0,
        .Limit = 0,
    };

    AsmWriteGdtr(&GdtPtr);
    AsmWriteIdtr(&IdtPtr);
}