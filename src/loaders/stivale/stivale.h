#ifndef __LOADERS_STIVALE_STIVALE_H__
#define __LOADERS_STIVALE_STIVALE_H__

#include <Base.h>

#pragma pack(1)

typedef struct {
    UINT64 Stack;
    union {
        struct {
            UINT16 GraphicsFramebuffer : 1;
            UINT16 Pml5Enable : 1;
            UINT16 EnableKASLR : 1;
        };
        UINT16 Flags;
    };
    UINT16 FramebufferWidth;
    UINT16 FramebufferHeight;
    UINT16 FramebufferBpp;
    UINT64 EntryPoint;
} STIVALE_HEADER;

typedef struct {
    UINT64 Cmdline;
    UINT64 MemoryMapAddr;
    UINT64 MemoryMapEntries;
    UINT64 FramebufferAddr;
    UINT16 FramebufferPitch;
    UINT16 FramebufferWidth;
    UINT16 FramebufferHeight;
    UINT16 FramebufferBpp;
    UINT64 Rsdp;
    UINT64 ModuleCount;
    UINT64 Modules;
    UINT64 Epoch;
    UINT64 Flags;
#define STIVALE_STRUCT_BIOS         BIT0
#define STIVALE_STRUCT_EXT_FB_INFO  BIT1
    UINT8 FbMemoryModel;
    UINT8 FbRedMaskSize;
    UINT8 FbRedMaskShift;
    UINT8 FbGreenMaskSize;
    UINT8 FbGreenMaskShift;
    UINT8 FbBlueMaskSize;
    UINT8 FbBlueMaskShift;
} STIVALE_STRUCT;

#define STIVALE_USABLE              1
#define STIVALE_RESERVED            2
#define STIVALE_ACPI_RECLAIM        3
#define STIVALE_ACPI_NVS            4
#define STIVALE_BAD_MEMORY          5
#define STIVALE_KERNEL_MODULES      10
#define STIVALE_BOOTLOADER_RECLAIM  0x1000

typedef struct {
    UINT64 Base;
    UINT64 Length;
    UINT32 Type;
    UINT32 Unused;
} STIVALE_MMAP_ENTRY;

typedef struct {
    UINT64 Begin;
    UINT64 End;
    CHAR8 String[128];
    UINT64 Next;
} STIVALE_MODULE;

#pragma pack()

#endif //__LOADERS_STIVALE_STIVALE_H__
