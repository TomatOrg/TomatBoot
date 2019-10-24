#ifndef TOMATBOOT_UEFI_PARSE_CONFIG_H
#define TOMATBOOT_UEFI_PARSE_CONFIG_H

#include <Base.h>

typedef struct _ConfigEntry {
    const char* name;
    const char* cmd;
} ConfigEntry;

typedef struct _Config {
    UINTN EntryCount;
    ConfigEntry entries;
} Config;

Config* ParseConfig();

void FreeConfig(Config* config);

#endif //TOMATBOOT_UEFI_PARSE_CONFIG_H
