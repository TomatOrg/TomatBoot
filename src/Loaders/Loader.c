#include <Util/Except.h>
#include <Library/UefiLib.h>
#include "Loader.h"

EFI_STATUS LoadStivaleKernel(CONFIG_ENTRY* Config);

void LoadKernel(CONFIG_ENTRY* Config) {
    switch (Config->Protocol) {
        case PROTOCOL_STIVALE: {
            WARN_ON_ERROR(LoadStivaleKernel(Config), "Failed to load stivale kernel...");
        } break;

        case PROTOCOL_STIVALE2: {
            ERROR("TODO: not implemented yet");
        } break;

        case PROTOCOL_LINUX: {
            ERROR("TODO: not implemented yet");
        } break;

        case PROTOCOL_MULTIBOOT2: {
            ERROR("TODO: not implemented yet");
        } break;

        default:
            ERROR("Invalid boot entry");
    }

    CpuDeadLoop();
}
