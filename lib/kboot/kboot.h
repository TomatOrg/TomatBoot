#ifndef KRETLIM_BOOT_H
#define KRETLIM_BOOT_H

#include <stdint.h>

#define KBOOT_MEMORY_TYPE_RESERVED      0
#define KBOOT_MEMORY_TYPE_BAD_MEMORY    1
#define KBOOT_MEMORY_TYPE_ACPI_RECLAIM  2
#define KBOOT_MEMORY_TYPE_USABLE        3
#define KBOOT_MEMORY_TYPE_ACPI_NVS      4

typedef struct kboot_mmap_entry {
    uint32_t type;
    uint64_t addr;
    uint64_t len;
} __attribute__((packed)) kboot_mmap_entry_t;

typedef struct kboot_info {
    struct {
        uint64_t counts;
        kboot_mmap_entry_t* descriptors;
    } __attribute__((packed)) mmap;
    
    struct {
        uint64_t framebuffer_addr;
        uint32_t width;
        uint32_t height;
    } __attribute__((packed)) framebuffer;

    struct {
        uint32_t length;
        char* cmdline;
    } __attribute__((packed)) cmdline;

    uint64_t rsdp;
} __attribute__((packed)) kboot_info_t;

typedef __attribute__((sysv_abi)) void (*kboot_entry_function)(uint32_t magic, kboot_info_t* info);

#endif
