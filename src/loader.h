#ifndef KRETLIM_UEFI_BOOT_LOADER_H
#define KRETLIM_UEFI_BOOT_LOADER_H

#include "config.h"

void load_kernel(boot_config_t* config, boot_entry_t* entry);

#endif