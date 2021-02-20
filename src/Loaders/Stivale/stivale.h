#ifndef __STIVALE__STIVALE_H__
#define __STIVALE__STIVALE_H__

#include <Base.h>
typedef UINT64 uint64_t;
typedef UINT32 uint32_t;
typedef UINT16 uint16_t;
typedef UINT8 uint8_t;

/* --- Header --------------------------------------------------------------- */
/*  Information passed from the kernel to the bootloader                      */

struct stivale_header {
    uint64_t stack;
    uint16_t flags;
    uint16_t framebuffer_width;
    uint16_t framebuffer_height;
    uint16_t framebuffer_bpp;
    uint64_t entry_point;
} __attribute__((__packed__));

/* --- Struct --------------------------------------------------------------- */
/*  Information passed from the bootloader to the kernel                      */

struct stivale_module {
    uint64_t begin;
    uint64_t end;
    char string[128];
    uint64_t next;
} __attribute__((__packed__));

#define STIVALE_MMAP_USABLE                 1
#define STIVALE_MMAP_RESERVED               2
#define STIVALE_MMAP_ACPI_RECLAIMABLE       3
#define STIVALE_MMAP_ACPI_NVS               4
#define STIVALE_MMAP_BAD_MEMORY             5
#define STIVALE_MMAP_KERNEL_AND_MODULES     10
#define STIVALE_MMAP_BOOTLOADER_RECLAIMABLE 0x1000

struct stivale_mmap_entry {
    uint64_t base;
    uint64_t length;
    uint32_t type;
    uint32_t unused;
} __attribute__((__packed__));

#define STIVALE_FBUF_MMODEL_RGB 1

struct stivale_struct {
    uint64_t cmdline;
    uint64_t memory_map_addr;
    uint64_t memory_map_entries;
    uint64_t framebuffer_addr;
    uint16_t framebuffer_pitch;
    uint16_t framebuffer_width;
    uint16_t framebuffer_height;
    uint16_t framebuffer_bpp;
    uint64_t rsdp;
    uint64_t module_count;
    uint64_t modules;
    uint64_t epoch;
    uint64_t flags; // bit 0: 1 if booted with BIOS, 0 if booted with UEFI
                    // bit 1: 1 if extended colour information passed, 0 if not
    uint8_t  fb_memory_model;
    uint8_t  fb_red_mask_size;
    uint8_t  fb_red_mask_shift;
    uint8_t  fb_green_mask_size;
    uint8_t  fb_green_mask_shift;
    uint8_t  fb_blue_mask_size;
    uint8_t  fb_blue_mask_shift;
} __attribute__((__packed__));

#endif