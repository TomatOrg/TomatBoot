#ifndef __CONFIG_BOOT_CONFIG_H__
#define __CONFIG_BOOT_CONFIG_H__

#include <Uefi.h>

typedef struct _BOOT_CONFIG {
    INT32 BootDelay;
    INT32 DefaultOS;
    UINT32 GfxMode;
    BOOLEAN OverrideGfx;
} BOOT_CONFIG;

/**
 * This allows to set overrides on the tomatboot config
 * from the configuration file.
 */
extern BOOT_CONFIG gBootConfigOverride;

/**
 * Loaded the config file into the given struct,
 * if not found will create new configurations
 */
void LoadBootConfig(BOOT_CONFIG* config);

/**
 * Save the boot configurations to the disk
 */
void SaveBootConfig(BOOT_CONFIG* config);

#endif //__CONFIG_BOOT_CONFIG_H__
