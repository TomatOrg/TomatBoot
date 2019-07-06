#ifndef KRETLIM_BOOT_H
#define KRETLIM_BOOT_H

#include <stdint.h>

typedef struct kboot_info {
    struct {
        uint64_t counts;
        uint64_t descriptor_size;
        EFI_MEMORY_DESCRIPTOR* descriptors;
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
