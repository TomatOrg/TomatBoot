#ifndef __LOADERS_STIVALE_STIVALE_H__
#define __LOADERS_STIVALE_STIVALE_H__

#include <Base.h>

#pragma pack(1)

#define STIVALE_MMAP_USABLE_RAM     0

typedef struct {
    UINT64 Stack;
    UINT16 Flags;
    UINT16 FramebufferWidth;
    UINT16 FramebufferHeight;
    UINT16 FramebufferBpp;
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
} STIVALE_STRUCT;

#define E820_TYPE_USABLE            (1)
#define E820_TYPE_RESERVED          (2)
#define E820_TYPE_ACPI_RECLAIM      (3)
#define E820_TYPE_ACPI_NVS          (4)
#define E820_TYPE_BAD_MEMORY        (5)

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
