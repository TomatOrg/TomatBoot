#ifndef TOMATBOOT_UEFI_PARSE_CONFIG_H
#define TOMATBOOT_UEFI_PARSE_CONFIG_H

#include <Base.h>

typedef struct boot_entry {
    const CHAR8* name;
    const CHAR8* path;
    const CHAR8* cmd;
} boot_entry_t;

typedef struct boot_entries {
    UINT8 count;
    boot_entry_t entries;
} boot_entries_t;

boot_entries_t* get_boot_entries();
void free_boot_entries(boot_entries_t* config);

typedef struct boot_config {
    INT32 boot_delay;
    INT32 default_os;
    INT32 gfx_mode;
} boot_config_t;

void load_boot_config(boot_config_t* config);
void save_boot_config(boot_config_t* config);

#endif //TOMATBOOT_UEFI_PARSE_CONFIG_H
