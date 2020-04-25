#ifndef __CONFIG_BOOT_CONFIG_H__
#define __CONFIG_BOOT_CONFIG_H__

#include <Uefi.h>

typedef struct _BOOT_CONFIG {
    INT32 BootDelay;
    INT32 DefaultOS;
    UINT32 GfxMode;
} BOOT_CONFIG;

void LoadBootConfig(BOOT_CONFIG* config);
void SaveBootConfig(BOOT_CONFIG* config);

#endif //__CONFIG_BOOT_CONFIG_H__
