#ifndef KRETLIM_UEFI_BOOT_CONFIG_H
#define KRETLIM_UEFI_BOOT_CONFIG_H

#include <stdint.h>
#include <stddef.h>

typedef struct boot_module_entry {
    const wchar_t path[256];
} __attribute__((packed)) boot_module_entry_t;

typedef struct boot_entry {
    size_t size;
    wchar_t name[256];
    wchar_t filename[256];
    wchar_t command_line[256];
    size_t modules_count;
    boot_module_entry_t boot_modules[0];
} __attribute__((packed)) boot_entry_t;

typedef struct boot_config {
    size_t default_option;
    size_t entry_count;
    boot_entry_t entries[0];
} __attribute__((packed)) boot_config_t;


#endif