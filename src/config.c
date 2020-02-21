#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>
#include <Library/DebugLib.h>
#include <util/gop_utils.h>
#include <Library/BaseLib.h>

#include "config.h"

#define CHECK_OPTION(x) (AsciiStrnCmp(line,  #x "=", sizeof(#x)) == 0)

boot_entries_t boot_entries;

CHAR8* read_line(EFI_FILE_PROTOCOL* file) {
    CHAR8* path = NULL;
    ASSERT_EFI_ERROR(gBS->AllocatePool(EfiBootServicesData, 255, (VOID**)&path));
    UINTN length = 0;

    // max length of 255 for simplicity
    while(length < 255) {
        // read 1 byte
        UINTN one = 1;
        ASSERT_EFI_ERROR(file->Read(file, &one, &path[length]));
//        path[length] = AsciiCharToUpper(path[length]);

        // got to new line or eof
        if(one == 0 || path[length] == '\n') {
            // if was end of line and nothing was read, just return null to indicate nothing to read
            if(one == 0 && length == 0) {
                ASSERT_EFI_ERROR(gBS->FreePool(path));
                return NULL;
            }

            path[length] = 0;
            break;
        }

        // read next character if not a \r
        if(path[length] != '\r') {
            length++;
        }
    }

    ASSERT(length < 255);

    return path;
}

#define CHECK(err) \
    do { \
        if(EFI_ERROR(err)) { \
            return err; \
        } \
    } while(0);

EFI_STATUS get_boot_entries(boot_entries_t* entries) {
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* filesystem = NULL;
    CHECK(gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID**)&filesystem));

    EFI_FILE_PROTOCOL* root = NULL;
    CHECK(filesystem->OpenVolume(filesystem, &root));

    EFI_FILE_PROTOCOL* file = NULL;
    CHECK(root->Open(root, &file, L"tomatboot.cfg", EFI_FILE_MODE_READ, 0));

    // first count how many entries there are
    CHECK(file->SetPosition(file, 0));
    UINTN entryCount = 0;
    while(TRUE) {
        CHAR8* line = read_line(file);
        if(line == NULL) break;

        if(line[0] == ':') {
            entryCount++;
        }

        CHECK(gBS->FreePool(line));
    }

    // allocate entries
    entries->count = entryCount;
    CHECK(gBS->AllocatePool(EfiBootServicesData, sizeof(boot_entry_t) * entryCount, (VOID**)&entries->entries));

    boot_module_t* last_module = NULL;

    // now do the actual processing of everything
    CHECK(file->SetPosition(file, 0));
    INTN index = -1;
    while(TRUE) {
        CHAR8* line = read_line(file);
        if(line == NULL) break;

        if(line[0] == ':') {
            // got new entry (this is the name)
            index++;
            entries->entries[index].protocol = BOOT_TBOOT;
            entries->entries[index].name = line + 1;
            entries->entries[index].path = NULL;
            entries->entries[index].cmd = NULL;
            entries->entries[index].modules = NULL;
            entries->entries[index].modules_count = 0;
            last_module = NULL;

        }else {
            // this is just a parameter
            CHECK(index != -1);

            // path of the bootable (only one)
            if(CHECK_OPTION(PATH)) {
                entries->entries[index].path = line + sizeof("PATH");

            // command line arguments (only one)
            }else if(CHECK_OPTION(CMDLINE)) {
                entries->entries[index].cmd = line + sizeof("CMDLINE");

            // module path (can have multiple)
            }else if(CHECK_OPTION(MODULE)) {
                line += sizeof("MODULE");

                // get the end of the name
                CHAR8 *path = line;
                while (*path != 0 && *path != ',') {
                    path++;
                }

                // make sure there is even a path
                CHECK(*path != 0);

                // set the , to null termination and skip it
                *path++ = 0;

                // allocate the entry
                if (last_module == NULL) {
                    CHECK(gBS->AllocatePool(EfiBootServicesData, sizeof(boot_module_t), (void *) &last_module));
                    entries->entries[index].modules = last_module;
                } else {
                    CHECK(gBS->AllocatePool(EfiBootServicesData, sizeof(boot_module_t), (void *) &last_module->next));
                    last_module = last_module->next;
                }

                // set the info
                last_module->path = path;
                last_module->tag = line;
                last_module->next = NULL;
                entries->entries[index].modules_count++;

                // if nothing just free the line
            } else if(CHECK_OPTION(PROTOCOL)) {
                char *protocol = line + sizeof("PROTOCOL");

                // check the options
                if (AsciiStrCmp(protocol, "LINUX") == 0) {
                    entries->entries[index].protocol = BOOT_LINUX;
                } else if (AsciiStrCmp(protocol, "TBOOT") == 0) {
                    entries->entries[index].protocol = BOOT_TBOOT;
                } else if (AsciiStrCmp(protocol, "UEFI") == 0) {
                    entries->entries[index].protocol = BOOT_UEFI;
                } else {
                    DebugPrint(0, "Unknown protocol `%a` for option `%a`\n", protocol, entries->entries[index].name);
                    CHECK(FALSE);
                }

                CHECK(gBS->FreePool(line));
            } else if(CHECK_OPTION(DEVICE_PATH)) {
                char* device_path = line + sizeof("DEVICE_PATH");



            }else {
                CHECK(gBS->FreePool(line));
            }
        }
    }

    CHECK(file->Close(file));
    CHECK(root->Close(root));

    return EFI_SUCCESS;
}

void load_boot_config(boot_config_t* config) {
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* filesystem = NULL;
    ASSERT_EFI_ERROR(gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID**)&filesystem));

    EFI_FILE_PROTOCOL* root = NULL;
    ASSERT_EFI_ERROR(filesystem->OpenVolume(filesystem, &root));

    EFI_FILE_PROTOCOL* configFile = NULL;
    EFI_STATUS status = root->Open(root, &configFile, L"tomatboot.bin", EFI_FILE_MODE_READ, 0);

    // if not found just set default config
    if(status == EFI_NOT_FOUND) {
        config->boot_delay = 4;
        config->default_os = 0;
        config->gfx_mode = get_first_mode();

    // otherwise see if can read the config
    }else {
        ASSERT_EFI_ERROR(status);

        UINTN size = sizeof(boot_config_t);
        ASSERT_EFI_ERROR(configFile->Read(configFile, &size, config));
        ASSERT_EFI_ERROR(configFile->Close(configFile));
    }

    // verify config and change if needed
    if(config->boot_delay > 30 || config->boot_delay < 1) {
        config->boot_delay = 30;
    }

    // TODO: Verify graphics config?
    // TODO: Verify default os config

    // close everything
    ASSERT_EFI_ERROR(root->Close(root));
}

void save_boot_config(boot_config_t* config) {
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* filesystem = NULL;
    ASSERT_EFI_ERROR(gBS->LocateProtocol(&gEfiSimpleFileSystemProtocolGuid, NULL, (VOID**)&filesystem));

    EFI_FILE_PROTOCOL* root = NULL;
    ASSERT_EFI_ERROR(filesystem->OpenVolume(filesystem, &root));

    EFI_FILE_PROTOCOL* configFile = NULL;
    ASSERT_EFI_ERROR(root->Open(root, &configFile, L"tomatboot.bin", EFI_FILE_MODE_READ | EFI_FILE_MODE_WRITE | EFI_FILE_MODE_CREATE, 0));

    UINTN size = sizeof(boot_config_t);
    ASSERT_EFI_ERROR(configFile->Write(configFile, &size, config));

    // close everything
    ASSERT_EFI_ERROR(configFile->Flush(configFile));
    ASSERT_EFI_ERROR(configFile->Close(configFile));
    ASSERT_EFI_ERROR(root->Close(root));
}
