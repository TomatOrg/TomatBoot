#ifndef __CONFIG_CONFIG_H__
#define __CONFIG_CONFIG_H__

#include <Uefi.h>

typedef enum _BOOT_PROTOCOL {
    BOOT_LINUX,
    BOOT_MB2,
    BOOT_STIVALE,
} BOOT_PROTOCOL;

typedef struct _BOOT_MODULE {
    LIST_ENTRY Link;
    CHAR16* Path;
    CHAR16* Tag;
} BOOT_MODULE;

typedef struct _BOOT_ENTRY {
    BOOT_PROTOCOL Protocol;
    CHAR16* Name;
    CHAR16* Path;
    CHAR16* Cmdline;
    LIST_ENTRY BootModules;
    LIST_ENTRY Link;
} BOOT_ENTRY;

extern BOOT_ENTRY* gDefaultEntry;
extern LIST_ENTRY gBootEntries;

/**
 * Get the entry in a certain index
 */
BOOT_ENTRY* GetBootEntryAt(int i);

/**
 * Will append to the list all the boot entries found in the configuration
 * file
 */
EFI_STATUS GetBootEntries(LIST_ENTRY* Head);

#endif //__CONFIG_CONFIG_H__
