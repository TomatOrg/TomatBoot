#include <Uefi.h>

#include <Protocol/LoadedImage.h>

#include <Library/BaseLib.h>
#include <Library/FileHandleLib.h>
#include <Library/DevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>

#include <Util/Except.h>
#include <Util/DevicePathUtils.h>
#include <Util/StringUtils.h>

#include "Config.h"
#include "URI.h"

/**
 * The global configuration object
 */
CONFIG gConfig = {
    .Timeout = -1,
    .DefaultEntry = 1,
    .Entries = INITIALIZE_LIST_HEAD_VARIABLE(gConfig.Entries)
};


/**
 * The possible paths for the Config file
 */
static CHAR16* mConfigPaths[] = {
    L"boot\\tomatboot.cfg",
    L"tomatboot.cfg",

    // fallback on limine configuration
    // file because they are compatible
    L"boot\\limine.cfg",
    L"limine.cfg"
};

/**
 * Searches for the Config files under the given device path
 *
 * @param ConfigFile    [OUT]   The handle to the Config file
 * @param DevicePath    [IN]    The device path to search under
 */
static EFI_STATUS SearchForConfigUnder(EFI_FILE_PROTOCOL** ConfigFile, EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FS) {
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_FILE_PROTOCOL* RootFile = NULL;

    CHECK(FS != NULL);
    CHECK(ConfigFile != NULL);
    *ConfigFile = NULL;

    // open the root
    EFI_CHECK(FS->OpenVolume(FS, &RootFile));

    // try each of them
    for (INTN Index = 0; Index < ARRAY_SIZE(mConfigPaths); Index++) {
        // try to open it
        Status = RootFile->Open(RootFile, ConfigFile, mConfigPaths[Index], EFI_FILE_MODE_READ, 0);
        if (Status == EFI_SUCCESS) {
            // found it!
            goto cleanup;
        }
        CHECK_STATUS(Status == EFI_NOT_FOUND, Status);
    }

    // we failed to find the Config file....
    Status = EFI_NOT_FOUND;

cleanup:
    if (RootFile != NULL) {
        WARN_ON_ERROR(FileHandleClose(RootFile), "Failed to close root file");
    }
    return Status;
}

/**
 * Will search for the Config on the boot drive
 *
 * @param ConfigFile    [IN] Search for the Config on the boot drive
 */
static EFI_STATUS SearchForConfig(EFI_FILE_PROTOCOL** ConfigFile) {
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_DEVICE_PATH* BootDevicePath = NULL;
    EFI_HANDLE* Handles = NULL;
    UINTN HandleCount = 0;

    CHECK(ConfigFile != NULL);
    *ConfigFile = NULL;

    // get the boot device path
    CHECK_AND_RETHROW(GetBootDevicePath(&BootDevicePath));

    // get all the filesystems with this device path
    EFI_CHECK(gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &HandleCount, &Handles));
    for (int i = 0; i < HandleCount; i++) {
        // open it
        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FS = NULL;
        EFI_CHECK(gBS->HandleProtocol(Handles[i], &gEfiSimpleFileSystemProtocolGuid, (void**)&FS));

        // search for config
        Status = SearchForConfigUnder(ConfigFile, FS);
        if (Status == EFI_SUCCESS) {
            goto cleanup;
        }
        CHECK_STATUS(Status == EFI_NOT_FOUND, Status);
    }

    CHECK_FAIL_STATUS(EFI_NOT_FOUND, "Could not find the config file");

cleanup:
    if (BootDevicePath != NULL) {
        FreePool(BootDevicePath);
    }
    if (Handles != NULL) {
        FreePool(Handles);
    }
    return Status;
}

#define CHECK_OPTION(key, ...) \
    do { \
        if (StrnCmp(Line, L##key L"=", ARRAY_SIZE(L##key L"=") - 1) == 0) { \
            CHAR16* Value = Line + ARRAY_SIZE(L##key L"=") - 1; \
            __VA_ARGS__ \
            goto found; \
        } \
    } while(0)

/**
 * This will attempt to search for the Config file and then properly parse the configuration
 * file into the global configuration object
 */
EFI_STATUS ParseConfig() {
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_FILE_PROTOCOL* ConfigFile = NULL;
    CHAR16* Line = NULL;

    // find the Config file
    CHECK_AND_RETHROW(SearchForConfig(&ConfigFile));

    // the current entry
    CONFIG_ENTRY* CurrentEntry = NULL;
    MODULE* LastModule = NULL;
    BOOLEAN Ascii = FALSE;

    // now parse it nicely
    while (!FileHandleEof(ConfigFile)) {
        // read the line
        Line = FileHandleReturnLine(ConfigFile, &Ascii);
        CHECK_STATUS(Line != NULL, EFI_OUT_OF_RESOURCES);

        // TODO: support for directories

        if (Line[0] == ':') {
            //----------------------------------------------------------------------------------------------------------
            // New entry
            //----------------------------------------------------------------------------------------------------------

            if (CurrentEntry != NULL) {
                if (CurrentEntry->Protocol == PROTOCOL_INVALID) {
                    // empty entry, ignore
                    FreePool(CurrentEntry->Name);
                } else {
                    // insert to the list
                    CHECK_STATUS(CurrentEntry->KernelPath != NULL, EFI_NOT_FOUND, "Missing kernel path!");
                    InsertTailList(&gConfig.Entries, &CurrentEntry->Link);
                    PrintConfigEntry(CurrentEntry);
                    CurrentEntry = NULL;
                }
            }

            // check if need to allocate a new one
            if (CurrentEntry == NULL) {
                CurrentEntry = AllocateZeroPool(sizeof(CONFIG_ENTRY));
            }

            // set the name
            CurrentEntry->Name = StrDup(&Line[1]);
            CHECK_STATUS(CurrentEntry->Name != NULL, EFI_OUT_OF_RESOURCES);
            InitializeListHead(&CurrentEntry->Modules);

            // no last module this time
            LastModule = NULL;

        } else if (CurrentEntry == NULL) {

            //----------------------------------------------------------------------------------------------------------
            // global keys
            //----------------------------------------------------------------------------------------------------------

            CHECK_OPTION("TIMEOUT", {
                if (StrCmp(Value, L"no") == 0) {
                    gConfig.Timeout = -1;
                } else {
                    gConfig.Timeout = StrDecimalToUintn(Value);
                }
            });

            CHECK_OPTION("DEFAULT_ENTRY", {
                gConfig.DefaultEntry = StrDecimalToUintn(Value);
                if (gConfig.DefaultEntry <= 0) {
                    gConfig.DefaultEntry = 1;
                }
            });
        } else {

            //----------------------------------------------------------------------------------------------------------
            // private keys
            //----------------------------------------------------------------------------------------------------------

            CHECK_OPTION("PROTOCOL", {
                if (StrCmp(Value, L"linux") == 0) {
                    CurrentEntry->Protocol = PROTOCOL_LINUX;
                } else if (StrCmp(Value, L"stivale") == 0) {
                    CurrentEntry->Protocol = PROTOCOL_STIVALE;
                } else if (StrCmp(Value, L"stivale2") == 0) {
                    CurrentEntry->Protocol = PROTOCOL_STIVALE2;
                } else if (StrCmp(Value, L"multiboot2") == 0) {
                    CurrentEntry->Protocol = PROTOCOL_MULTIBOOT2;
                } else {
                    CHECK_FAIL_STATUS(EFI_NOT_FOUND, "Unknown boot protocol `%s`", Value);
                }
            });

            CHECK_OPTION("KERNEL_PATH", {
                CHECK_STATUS(CurrentEntry->Protocol != PROTOCOL_INVALID, EFI_NOT_FOUND, "Missing protocol");
                CHECK_AND_RETHROW(ParseURI(Value, &CurrentEntry->KernelPath));
            });

            CHECK_OPTION("KERNEL_CMDLINE", {
                CHECK_STATUS(CurrentEntry->Protocol != PROTOCOL_INVALID, EFI_NOT_FOUND, "Missing protocol");

                // convert to ascii and set
                UINTN CmdlineLen = StrLen(Value) + 1;
                CHAR8* Cmdline = AllocateZeroPool(CmdlineLen);
                EFI_CHECK(UnicodeStrToAsciiStrS(Value, Cmdline, CmdlineLen));
                CurrentEntry->Cmdline = Cmdline;
            });

            CHECK_OPTION("MODULE_PATH", {
                CHECK_STATUS(CurrentEntry->Protocol != PROTOCOL_INVALID, EFI_NOT_FOUND, "Missing protocol");

                // allocate it
                MODULE* NewModule = AllocateZeroPool(sizeof(MODULE));

                // check if compressed
                if (Value[0] == '$') {
                    NewModule->Compressed = TRUE;
                    Value++;
                }

                // parse and insert the URI
                CHECK_AND_RETHROW(ParseURI(Value, &NewModule->Path));
                InsertTailList(&CurrentEntry->Modules, &NewModule->Link);

                // there is no last module, set this as the last one
                if (LastModule == NULL) {
                    LastModule = NewModule;
                }
            });

            CHECK_OPTION("MODULE_STRING", {
                CHECK_STATUS(CurrentEntry->Protocol != PROTOCOL_INVALID, EFI_NOT_FOUND, "Missing protocol");
                CHECK_STATUS(LastModule != NULL, EFI_NOT_FOUND, "MODULE_STRING before a MODULE_PATH");

                LastModule->String = ConvertToAscii(Value);
                CHECK_STATUS(LastModule->String != NULL, EFI_OUT_OF_RESOURCES);

                // increment for the next module string
                if (IsNodeAtEnd(&CurrentEntry->Modules, &LastModule->Link)) {
                    LastModule = NULL;
                } else {
                    LastModule = BASE_CR(GetNextNode(&CurrentEntry->Modules, &LastModule->Link), MODULE, Link);
                }
            });
        }

        // finally free the line
    found:
        FreePool(Line);
        Line = NULL;
    }

    // add the last entry
    if (CurrentEntry != NULL) {
        if (CurrentEntry->Protocol == PROTOCOL_INVALID) {
            // empty entry, ignore
            FreePool(CurrentEntry->Name);
        } else {
            // insert to the list
            CHECK_STATUS(CurrentEntry->KernelPath != NULL, EFI_NOT_FOUND, "Missing kernel path!");
            InsertTailList(&gConfig.Entries, &CurrentEntry->Link);
            PrintConfigEntry(CurrentEntry);
        }
    }

cleanup:
    if (ConfigFile != NULL) {
        WARN_ON_ERROR(FileHandleClose(ConfigFile), "Failed to close Config file");
    }
    if (Line != NULL) {
        FreePool(Line);
    }
    return Status;
}

void PrintConfigEntry(CONFIG_ENTRY* Entry) {
    // print it nicely because why not
    TRACE("Added `%s`", Entry->Name);
    TRACE("  Protocol: `%d`", Entry->Protocol);
    TRACE("  Cmdline: `%a`", Entry->Cmdline);
    TRACE("  Modules:");
    LIST_ENTRY* List = &Entry->Modules;
    for (LIST_ENTRY* Iter = GetFirstNode(List); Iter != &Entry->Modules; Iter = GetNextNode(List, Iter)) {
        MODULE* Module = BASE_CR(Iter, MODULE, Link);
        CHAR16* Text = ConvertDevicePathToText(Module->Path, TRUE, TRUE);
        TRACE("    %a - %s%a", Module->String, Text, Module->Compressed ? " (Compressed)" : "");
        FreePool(Text);
    }
}

CONFIG_ENTRY* GetEntryAt(int index) {
    LIST_ENTRY* List = &gConfig.Entries;
    for (LIST_ENTRY* Iter = GetFirstNode(List); Iter != &gConfig.Entries; Iter = GetNextNode(List, Iter)) {
        index -= 1;
        if (index == 0) {
            return BASE_CR(Iter, CONFIG_ENTRY, Link);
        }
    }
    return NULL;
}
