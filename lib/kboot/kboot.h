#ifndef KRETLIM_BOOT_H
#define KRETLIM_BOOT_H

#include <stdint.h>

#define KBOOT_MAPPING_IDENTITY 0
#define KBOOT_MAPPING_VIRTUAL 1

typedef struct kboot_header {
  struct {
      uint8_t type;
      uint64_t direct_mapping_base;
  } mapping;
  
  struct {
    uint8_t bpp;
    uint32_t width;
    uint32_t height;
  } framebuffer;
} kboot_header_t;

typedef struct kboot_info {
    struct {
        uint64_t counts;
        uint64_t descriptor_size;
        EFI_MEMORY_DESCRIPTOR* descriptors;
    } mmap;
    
    struct {
        uint64_t framebuffer_addr;
        uint32_t width;
        uint32_t height;
        uint32_t bpp;
    } framebuffer;

    struct {
        uint32_t length;
        char* cmdline;
    } cmdline;

    uint64_t rsdp;
} kboot_info_t;

typedef void (*kboot_entry_function)(uint32_t magic, kboot_info_t* info);

#endif
