#ifndef __TOMATBOOT_CONFIG_H__
#define __TOMATBOOT_CONFIG_H__

#include <Uefi.h>

/**
 * The configuration
 */
typedef struct _CONFIG {
    /**
     * Timeout
     */
    INTN Timeout;

    /**
     * The default entry
     */
    INTN DefaultEntry;

    /**
     * The boot entries
     */
    LIST_ENTRY Entries;
} CONFIG;

/**
 * The boot protocols supported by tomatboot
 */
typedef enum _PROTOCOL {
    PROTOCOL_INVALID,
    PROTOCOL_STIVALE,
    PROTOCOL_STIVALE2,
    PROTOCOL_LINUX,
    PROTOCOL_MULTIBOOT2,
} PROTOCOL;

typedef struct _CONFIG_ENTRY {
    LIST_ENTRY Link;

    /**
     * Protocol for this entry
     */
    PROTOCOL Protocol;

    /**
     * The name
     */
    CHAR16* Name;

    /**
     * Cmdline to pass to the kernel
     */
    CHAR8* Cmdline;

    /**
     * The path of the kernel
     */
    EFI_DEVICE_PATH* KernelPath;

    /**
     * The modules of the boot entry
     */
    LIST_ENTRY Modules;

    /**
     * Should kaslr be enabled, only relevant for stivale & stivale 2
     */
    BOOLEAN KASLR;
} CONFIG_ENTRY;

typedef struct _MODULE {
    LIST_ENTRY Link;

    /**
     * String to identify this module
     */
    CHAR8* String;

    /**
     * The path of the module to load
     */
    EFI_DEVICE_PATH* Path;

    /**
     * The file is compressed and needs to be decompressed before jumping
     * to the kernel
     */
    BOOLEAN Compressed;
} MODULE;

/**
 * The boot Config
 */
extern CONFIG gConfig;

/**
 * Parse the configurations of the boot loader
 */
EFI_STATUS ParseConfig();

/**
 * Print nicely the config entry
 */
void PrintConfigEntry(CONFIG_ENTRY* Entry);

CONFIG_ENTRY* GetEntryAt(int index);

#endif //__TOMATBOOT_CONFIG_H__
