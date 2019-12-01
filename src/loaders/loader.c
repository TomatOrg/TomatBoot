#include <Library/DebugLib.h>

#include "linux_loader.h"
#include "tboot_loader.h"

#include "loader.h"

void load_binary(boot_entry_t* entry) {
    switch(entry->protocol) {
        case BOOT_LINUX:
            load_linux_binary(entry);
            break;

        case BOOT_TBOOT:
            load_tboot_binary(entry);
            break;

        default:
            ASSERT(FALSE);
    }
}
