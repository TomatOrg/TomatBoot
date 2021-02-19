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
 * Returns the device path of the boot drive
 *
 * @param BootDrive [OUT] The boot drive
 */
static EFI_STATUS GetBootDevicePath(EFI_DEVICE_PATH** BootDrive) {
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_DEVICE_PATH* BootImage = NULL;

    CHECK(BootDrive != NULL);
    *BootDrive = NULL;

    // get the boot image device path
    EFI_CHECK(gBS->HandleProtocol(gImageHandle, &gEfiLoadedImageDevicePathProtocolGuid, (void**)&BootImage));

    // now remove the last element (Which would be the device path)
    BootImage = RemoveLastDevicePathNode(BootImage);
    CHECK_STATUS(BootImage, EFI_OUT_OF_RESOURCES);

    // set it
    *BootDrive = BootImage;

    // display it for debugging
    CHAR16* Text = ConvertDevicePathToText(BootImage, TRUE, TRUE);
    CHECK_STATUS(Text != NULL, EFI_OUT_OF_RESOURCES);
    TRACE("Boot drive: %s", Text);
    FreePool(Text);

cleanup:
    return Status;
}

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
static EFI_STATUS SearchForConfigUnder(EFI_FILE_PROTOCOL** ConfigFile, EFI_DEVICE_PATH* DevicePath) {
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_DEVICE_PATH* NewPath = NULL;
    FILEPATH_DEVICE_PATH* FilePath = NULL;

    CHECK(ConfigFile != NULL);
    *ConfigFile = NULL;

    // allocate this once
    FilePath = (FILEPATH_DEVICE_PATH*)CreateDeviceNode(MEDIA_DEVICE_PATH, MEDIA_FILEPATH_DP, SIZE_OF_FILEPATH_DEVICE_PATH + 255);
    CHECK_STATUS(FilePath != NULL, EFI_OUT_OF_RESOURCES);

    // try each of them
    for (INTN Index = 0; Index < ARRAY_SIZE(mConfigPaths); Index++) {
        // Create it
        StrCpyS(FilePath->PathName, 255, mConfigPaths[Index]);
        NewPath = AppendDevicePathNode(DevicePath, (EFI_DEVICE_PATH_PROTOCOL*)FilePath);
        CHECK_STATUS(NewPath != NULL, EFI_OUT_OF_RESOURCES);

        // try to open it
        Status = OpenFileByDevicePath(&NewPath, ConfigFile, EFI_FILE_MODE_READ, 0);
        if (Status == EFI_SUCCESS) {
            // found it!
            goto cleanup;
        }
        CHECK_STATUS(Status == EFI_NOT_FOUND, Status);

        // free them
        FreePool(NewPath);
        NewPath = NULL;
    }

    // we failed to find the Config file....
    Status = EFI_NOT_FOUND;

cleanup:
    if (NewPath != NULL) {
        FreePool(NewPath);
    }
    if (FilePath != NULL) {
        FreePool(FilePath);
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

    CHECK(ConfigFile != NULL);
    *ConfigFile = NULL;

    // get the boot device path
    CHECK_AND_RETHROW(GetBootDevicePath(&BootDevicePath));

    // depending on the type we will need to do different searches
    EFI_DEVICE_PATH* LastNode = LastDevicePathNode(BootDevicePath);
    if (LastNode->Type == MEDIA_DEVICE_PATH && LastNode->SubType == MEDIA_HARDDRIVE_DP) {
        // TODO: try all the partitions somehow...
        EFI_CHECK(SearchForConfigUnder(ConfigFile, BootDevicePath));
    } else {
        // this is another form of boot drive, just try the main boot drive
        EFI_CHECK(SearchForConfigUnder(ConfigFile, BootDevicePath));
    }

cleanup:
    if (BootDevicePath != NULL) {
        FreePool(BootDevicePath);
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

                    TRACE("Added `%s`", CurrentEntry->Name);
                    TRACE("    Cmdline: `%a`", CurrentEntry->Cmdline);

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
                LastModule = AllocateZeroPool(sizeof(MODULE));

                // check if compressed
                if (Value[0] == '$') {
                    LastModule->Compressed = TRUE;
                    Value++;
                }

                // parse and insert the URI
                CHECK_AND_RETHROW(ParseURI(Value, &LastModule->Path));
                InsertTailList(&CurrentEntry->Modules, &LastModule->Link);
            });

            CHECK_OPTION("MODULE_STRING", {
                CHECK_STATUS(CurrentEntry->Protocol != PROTOCOL_INVALID, EFI_NOT_FOUND, "Missing protocol");
                CHECK_STATUS(LastModule != NULL, EFI_NOT_FOUND, "MODULE_STRING before a MODULE_PATH");

                LastModule->String = ConvertToAscii(Value);
                CHECK_STATUS(LastModule->String != NULL, EFI_OUT_OF_RESOURCES);
            });
        }

        // finally free the line
    found:
        FreePool(Line);
        Line = NULL;
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
