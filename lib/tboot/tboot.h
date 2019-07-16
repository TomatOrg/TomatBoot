#ifndef KRETLIM_BOOT_H
#define KRETLIM_BOOT_H

#include <stdint.h>

#define TBOOT_MAGIC                     0xCAFEBABE

#define TBOOT_MEMORY_TYPE_RESERVED      0
#define TBOOT_MEMORY_TYPE_BAD_MEMORY    1
#define TBOOT_MEMORY_TYPE_ACPI_RECLAIM  2
#define TBOOT_MEMORY_TYPE_USABLE        3
#define TBOOT_MEMORY_TYPE_ACPI_NVS      4

typedef struct tboot_mmap_entry {
    uint32_t type;
    uint64_t addr;
    uint64_t len;
} __attribute__((packed)) tboot_mmap_entry_t;

typedef struct kboot_info {
    struct {
        uint64_t count;
        tboot_mmap_entry_t* entries;
    } __attribute__((packed)) mmap;
    
    struct {
        uint64_t addr;
        uint32_t width;
        uint32_t height;
    } __attribute__((packed)) framebuffer;

    struct {
        uint32_t length;
        char* cmdline;
    } __attribute__((packed)) cmdline;

    uint64_t rsdp;
} __attribute__((packed)) tboot_info_t;

typedef __attribute__((sysv_abi)) void (*tboot_entry_function)(uint32_t magic, tboot_info_t* info);

#endif
