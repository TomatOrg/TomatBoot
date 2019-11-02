#ifndef TOMATBOOT_UEFI_TBOOT_H
#define TOMATBOOT_UEFI_TBOOT_H

#include <uefi/Include/X64/ProcessorBind.h>

#define TBOOT_MAGIC                     0xCAFEBABE

#define TBOOT_MEMORY_TYPE_RESERVED      0
#define TBOOT_MEMORY_TYPE_BAD_MEMORY    1
#define TBOOT_MEMORY_TYPE_ACPI_RECLAIM  2
#define TBOOT_MEMORY_TYPE_USABLE        3
#define TBOOT_MEMORY_TYPE_ACPI_NVS      4

typedef struct tboot_mmap_entry {
    UINT32 type;
    UINT64 addr;
    UINT64 len;
} __attribute__((packed)) tboot_mmap_entry_t;

typedef struct tboot_info {
    struct {
        UINT64 count;
        tboot_mmap_entry_t* entries;
    } __attribute__((packed)) mmap;

    struct {
        UINT64 addr;
        UINT32 width;
        UINT32 height;
    } __attribute__((packed)) framebuffer;

    struct {
        UINT32 length;
        char* cmdline;
    } __attribute__((packed)) cmdline;

    UINT64 tsc_freq;
    UINT64 rsdp;
} __attribute__((packed)) tboot_info_t;

typedef __attribute__((sysv_abi)) void (*tboot_entry_function)(UINT32 magic, tboot_info_t* info);

#endif //TOMATBOOT_UEFI_TBOOT_H
