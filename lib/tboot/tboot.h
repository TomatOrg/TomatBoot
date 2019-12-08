#ifndef TOMATBOOT_UEFI_TBOOT_H
#define TOMATBOOT_UEFI_TBOOT_H

#include <uefi/Include/X64/ProcessorBind.h>

/*
 * The magic passed as a parameter to the kernel
 */
#define TBOOT_MAGIC                     0xCAFEBABE

/*
 * Usually contains uefi runtime services code or
 */
#define TBOOT_MEMORY_TYPE_RESERVED      0

/*
 * Memory that contains errors and is not to be used
 */
#define TBOOT_MEMORY_TYPE_BAD_MEMORY    1

/*
 * This memory is to be preserved by the UEFI OS loader and OS until
 * ACPI is enabled. Once ACPI is enabled, the memory in this range is
 * available for general use.
 */
#define TBOOT_MEMORY_TYPE_ACPI_RECLAIM  2

/*
 * Memory available for general use
 */
#define TBOOT_MEMORY_TYPE_USABLE        3

/*
 * This memory is to be preserved by the UEFI OS loader and OS in the
 * working and ACPI S1–S3 states.
 */
#define TBOOT_MEMORY_TYPE_ACPI_NVS      4

/*
 * Entry in the memory map, the type is one of
 * the TBOOT_MEMORY_TYPE_*
 */
typedef struct tboot_mmap_entry {
    UINT32 type;
    UINT64 addr;
    UINT64 len;
} __attribute__((packed)) tboot_mmap_entry_t;

/**
 * Entry in the modules list, the base is where it was loaded
 * and the len is the length in memory. the name is taken from
 * the configuration file.
 */
typedef struct tboot_module {
    UINT64 base;
    UINT64 len;
    CHAR8* name;
} __attribute__((packed)) tboot_module_t;

/**
 * The main boot structure, contains all the info you would just need
 */
typedef struct tboot_info {

    /**
     * The features that the loader supports, if the bit is
     * off then the data for that entry should be ignored,
     * otherwise the data is valid
     */
    union {
        struct {
            UINT64 mmap : 1;
            UINT64 framebuffer : 1;
            UINT64 cmdline : 1;
            UINT64 modules : 1;
            UINT64 tsc_freq : 1;
            UINT64 rsdp : 1;
        };
        UINT64 raw;
    } flags;

    /**
     * The memory map, contains information about the memory
     */
    struct {
        UINT64 count;
        tboot_mmap_entry_t* entries;
    } __attribute__((packed)) mmap;

    /**
     * The framebuffer, it is in BGRA format
     */
    struct {
        // the physical address
        UINT64 addr;
        // the width in pixels
        UINT32 width;
        // the height in pixels
        UINT32 height;
        // pixels per scanline
        UINT32 pitch;
    } __attribute__((packed)) framebuffer;

    /**
     * The command line arguments, passed from the
     * configuration file of the loaded entry
     */
    struct {
        UINT32 length;
        char* cmdline;
    } __attribute__((packed)) cmdline;

    /**
     * The boot modules, loaded according to the config file
     */
    struct {
        UINT64 count;
        tboot_module_t* entries;
    } __attribute__((packed)) modules;

    /**
     * The frequency (ticks per second) of the TSC
     *
     * @remark
     * Note that the TSC freq may not be constant, so don't
     * rely on it too much after the initial boot stage
     * or on other cores.
     */
    UINT64 tsc_freq;

    /**
     * Pointer to the RSDP table, may be 0 if the
     * table was not found
     */
    UINT64 rsdp;
} __attribute__((packed)) tboot_info_t;

/**
 * This is the signature of the kernel entry
 */
typedef __attribute__((sysv_abi)) void (*tboot_entry_function)(UINT32 magic, tboot_info_t* info);

#endif //TOMATBOOT_UEFI_TBOOT_H
