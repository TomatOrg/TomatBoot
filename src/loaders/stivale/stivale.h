#ifndef __LOADERS_STIVALE_STIVALE_H__
#define __LOADERS_STIVALE_STIVALE_H__

#include <Base.h>

#pragma pack(1)

#define STIVALE_MMAP_USABLE_RAM     0

typedef struct {
    UINT64 Stack;
    UINT64 Flags;
    UINT64 FramebufferWidth;
    UINT64 FramebufferHeight;
    UINT64 FramebufferBpp;
} STIVALE_HEADER;

typedef struct {
    UINT64 Cmdline;
    UINT64 MemoryMapAddr;
    UINT64 MemoryMapEntries;
    UINT64 FramebufferAddr;
    UINT64 FramebufferPitch;
    UINT64 FramebufferWidth;
    UINT64 FramebufferHeight;
    UINT64 FramebufferBpp;
    UINT64 Rsdp;
    UINT64 ModuleCount;
    UINT64 Modules;
} STIVALE_STRUCT;

typedef struct {
    UINT64 Base;
    UINT64 Length;
    UINT64 Type;
    UINT64 Unused;
} STIVALE_MMAP_ENTRY;

typedef struct {
    UINT64 Begin;
    UINT64 End;
    CHAR8 String[128];
    UINT64 Next;
} STIVALE_MODULE;

#pragma pack()

#endif //__LOADERS_STIVALE_STIVALE_H__
