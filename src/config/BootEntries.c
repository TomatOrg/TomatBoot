#include "BootEntries.h"
#include "BootConfig.h"

#include <util/FileUtils.h>
#include <util/Except.h>

#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DevicePathLib.h>
#include <Library/FileHandleLib.h>
#include <Library/DebugLib.h>
#include <Protocol/LoadedImage.h>
#include <util/DPUtils.h>
#include <Library/BaseMemoryLib.h>

#define CHECK_OPTION(x) (StrnCmp(Line, x L"=", ARRAY_SIZE(x)) == 0)

BOOT_ENTRY* gDefaultEntry = NULL;
LIST_ENTRY gBootEntries = INITIALIZE_LIST_HEAD_VARIABLE(gBootEntries);

static CHAR16* ConfigPaths[] = {
    L"boot\\tomatboot.cfg",
    L"tomatboot.cfg",

    // fallback on limine configuration
    // file because they are compatible
    L"boot\\limine.cfg",
    L"limine.cfg"
};

BOOT_ENTRY* GetBootEntryAt(int index) {
    int i = 0;
    for (LIST_ENTRY* Link = gBootEntries.ForwardLink; Link != &gBootEntries; Link = Link->ForwardLink, i++) {
        if (index == i) {
            return BASE_CR(Link, BOOT_ENTRY, Link);
        }
    }
    return NULL;
}

static CHAR16* CopyString(CHAR16* String) {
    return AllocateCopyPool((1 + StrLen(String)) * sizeof(CHAR16), String);
}

static EFI_STATUS ParseUri(CHAR16* Uri, EFI_SIMPLE_FILE_SYSTEM_PROTOCOL** OutFs, CHAR16** OutPath) {
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_LOADED_IMAGE_PROTOCOL* LoadedImage = NULL;
    EFI_DEVICE_PATH* BootDevicePath = NULL;
    EFI_HANDLE* Handles = NULL;
    UINTN HandleCount = 0;

    CHECK(Uri != NULL);
    CHECK(OutFs != NULL);
    CHECK(OutPath != NULL);

    // separate the domain from the uri type
    CHAR16* Root = StrStr(Uri, L":");
    *Root = L'\0';
    Root += 3; // skip ://

    // create the path itself
    CHAR16* Path = StrStr(Root, L"/");
    *Path = L'\0';
    Path++; // skip the /
    *OutPath = CopyString(Path);

    // convert `/` to `\` for uefi
    for (CHAR16* C = *OutPath; *C != CHAR_NULL; C++) {
        if (*C == '/') {
            *C = '\\';
        }
    }

    // check the uri
    if (StrCmp(Uri, L"boot") == 0) {
        // boot://[<part num>]/

        if (*Root == L'\0') {
            // the part num is missing, use
            // the boot fs so there is nothing
            // to do here really
        } else {
            // this has a part num, get it
            UINTN PartNum = StrDecimalToUintn(Root);

            // we need the boot drive device path so we can check the filesystems
            // are on the same drive
            EFI_CHECK(gBS->HandleProtocol(gImageHandle, &gEfiLoadedImageProtocolGuid, (void**)&LoadedImage));
            EFI_CHECK(gBS->HandleProtocol(LoadedImage->DeviceHandle, &gEfiDevicePathProtocolGuid, (void**)&BootDevicePath));

            // remove the last part which should be the partition itself

            BootDevicePath = RemoveLastDevicePathNode(BootDevicePath);
            CHECK(BootDevicePath != NULL);

            CHAR16* Text = ConvertDevicePathToText(BootDevicePath, TRUE, TRUE);
            TRACE("Boot device path: %s", Text);
            FreePool(Text);

            // iterate the protocols
            EFI_CHECK(gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &HandleCount, &Handles));
            int index = -1;
            for (int i = 0; i < HandleCount; i++) {
                EFI_DEVICE_PATH* DevicePath = NULL;
                EFI_CHECK(gBS->HandleProtocol(Handles[i], &gEfiDevicePathProtocolGuid, (void**)&DevicePath));

                Text = ConvertDevicePathToText(DevicePath, TRUE, TRUE);
                TRACE("Testing against filesystem at %s", Text);
                FreePool(Text);

                // check this is part of the same drive
                if (!InsideDevicePath(DevicePath, BootDevicePath)) {
                    // skip this entry
                    WARN("This is not on the same drive, next");
                    continue;
                }

                // get the last one, and make sure it is of
                // a harddrive part, since we are looking for
                // partitions
                DevicePath = LastDevicePathNode(DevicePath);
                if (DevicePathType(DevicePath) != MEDIA_DEVICE_PATH || DevicePathSubType(DevicePath) != MEDIA_HARDDRIVE_DP) {
                    // skip it
                    WARN("The last node is not a harddrive :(");
                    continue;
                }

                // we found one which might be valid! check it
                HARDDRIVE_DEVICE_PATH* Hd = (HARDDRIVE_DEVICE_PATH*)DevicePath;
                if (Hd->PartitionNumber == PartNum) {
                    // we freaking found it!
                    index = i;
                    break;
                }
            }

            // make sure we found it
            CHECK_TRACE(index != -1, "Could not find partition number `%d`", PartNum);

            // get the fs and set it
            EFI_CHECK(gBS->HandleProtocol(Handles[index], &gEfiSimpleFileSystemProtocolGuid, (void**)OutFs));
        }
    } else if (StrCmp(Uri, L"guid") == 0 || StrCmp(Uri, L"uuid") == 0) {
        // guid://<guid>/

        // parse the guid
        EFI_GUID Guid;
        EFI_CHECK(StrToGuid(Root, &Guid));

        // iterate the protocols
        EFI_CHECK(gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &HandleCount, &Handles));
        int index = -1;
        for (int i = 0; i < HandleCount; i++) {
            EFI_DEVICE_PATH* DevicePath = NULL;
            EFI_CHECK(gBS->HandleProtocol(Handles[i], &gEfiDevicePathProtocolGuid, (void**)&DevicePath));

            // get the last one, and make sure it is of
            // a harddrive part, since we are looking for
            // partitions
            DevicePath = LastDevicePathNode(DevicePath);
            if (DevicePathType(DevicePath) != MEDIA_DEVICE_PATH || DevicePathSubType(DevicePath) != MEDIA_HARDDRIVE_DP) {
                // skip it
                continue;
            }

            // we found one which might be valid! check that it is a gpt partition (important)
            // and that it matches the signature
            HARDDRIVE_DEVICE_PATH* Hd = (HARDDRIVE_DEVICE_PATH*)DevicePath;
            if (Hd->SignatureType == SIGNATURE_TYPE_GUID && CompareGuid(&Guid, (EFI_GUID*)Hd->Signature)) {
                // we freaking found it!
                index = i;
                break;
            }
        }

        // make sure we found it
        CHECK_TRACE(index != -1, "Could not find partition or fs with guid of `%s`", Root);

        // get the fs from the guid
        EFI_CHECK(gBS->HandleProtocol(Handles[index], &gEfiSimpleFileSystemProtocolGuid, (void**)OutFs));
    } else {
        CHECK_FAIL_TRACE("Unsupported resource type `%s`", Uri);
    }

cleanup:
    if (Handles != NULL) {
        FreePool(Handles);
    }

    if (BootDevicePath != NULL) {
        FreePool(BootDevicePath);
    }

    return Status;
}

static EFI_STATUS LoadBootEntries(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FS, LIST_ENTRY* Head) {
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_FILE_PROTOCOL* root = NULL;
    EFI_FILE_PROTOCOL* file = NULL;

    // open the configurations
    CHECK(FS != NULL);
    EFI_CHECK(FS->OpenVolume(FS, &root));

    for (int i = 0; i < ARRAY_SIZE(ConfigPaths); i++) {
        if (!EFI_ERROR(root->Open(root, &file, ConfigPaths[i], EFI_FILE_MODE_READ, 0))) {
            break;
        }

        // just in case
        file = NULL;
    }

    // if no config found just ignore and continue to next fs
    if (file == NULL) {
        goto cleanup;
    }

    // Start from a clean state
    *Head = (LIST_ENTRY)INITIALIZE_LIST_HEAD_VARIABLE(*Head);
    BOOT_ENTRY* CurrentEntry = NULL;
    BOOT_MODULE* CurrentModuleString = NULL;

    // now do the actual processing of everything
    while(TRUE) {
        CHAR16 Line[255] = {0};
        UINTN LineSize = sizeof(Line);
        BOOLEAN Ascii = TRUE;

        if (FileHandleEof(file)) {
            break;
        }
        EFI_CHECK(FileHandleReadLine(file, Line, &LineSize, FALSE, &Ascii));

        //------------------------------------------
        // New entry
        //------------------------------------------
        if (Line[0] == L':') {
            // got new entry (this is the name)
            CurrentEntry = AllocateZeroPool(sizeof(BOOT_ENTRY));
            CurrentEntry->Protocol = BOOT_STIVALE;
            CurrentEntry->Fs = FS;
            CurrentEntry->Name = CopyString(Line + 1);
            CurrentEntry->Protocol = BOOT_INVALID;
            CurrentEntry->Cmdline = L"";
            CurrentEntry->BootModules = (LIST_ENTRY) INITIALIZE_LIST_HEAD_VARIABLE(CurrentEntry->BootModules);
            InsertTailList(Head, &CurrentEntry->Link);
            CurrentModuleString = NULL;

            TRACE("Adding %s", CurrentEntry->Name);

        //------------------------------------------
        // Global keys
        //------------------------------------------
        } else if (CurrentEntry == NULL) {
            if (CHECK_OPTION(L"TIMEOUT")) {
                gBootConfigOverride.BootDelay = (INT32)StrDecimalToUintn(StrStr(Line, L"=") + 1);
            } else if (CHECK_OPTION(L"DEFAULT_ENTRY")) {
                gBootConfigOverride.DefaultOS = (INT32)StrDecimalToUintn(StrStr(Line, L"=") + 1);
            } else {
                WARN("Invalid line `%s`, ignoring", Line);
            }

        //------------------------------------------
        // Local keys
        //------------------------------------------
        } else {

            //------------------------------------------
            // path of the kernel
            //------------------------------------------
            if(CHECK_OPTION(L"PATH") || CHECK_OPTION(L"KERNEL_PATH")) {
                CHAR16* Path = StrStr(Line, L"=") + 1;
                CHECK_AND_RETHROW(ParseUri(Path, &CurrentEntry->Fs, &CurrentEntry->Path));

            //------------------------------------------
            // command line arguments
            //------------------------------------------
            }else if(CHECK_OPTION(L"CMDLINE") || CHECK_OPTION(L"KERNEL_CMDLINE")) {
                CurrentEntry->Cmdline = CopyString(StrStr(Line, L"=") + 1);

                // the boot protocol to use (onyl one)
            } else if(CHECK_OPTION(L"PROTOCOL") || CHECK_OPTION("KERNEL_PROTO") || CHECK_OPTION("KERNEL_PROTOCOL")) {
                CHAR16* Protocol = StrStr(Line, L"=") + 1;

                // check the options
                if (StrCmp(Protocol, L"linux") == 0) {
                    CurrentEntry->Protocol = BOOT_LINUX;
                } else if (StrCmp(Protocol, L"mb2") == 0) {
                    CurrentEntry->Protocol = BOOT_MB2;
                } else if (StrCmp(Protocol, L"stivale") == 0) {
                    CurrentEntry->Protocol = BOOT_STIVALE;
                } else if (StrCmp(Protocol, L"stivale2") == 0) {
                    CurrentEntry->Protocol = BOOT_STIVALE2;
                } else {
                    CHECK_FAIL_TRACE("Unknown protocol `%s` for option `%s`", Protocol, CurrentEntry->Name);
                }

            //------------------------------------------
            // module
            //------------------------------------------
            } else if (CHECK_OPTION(L"INITRD_PATH")) {
                CHECK_TRACE(CurrentEntry->Protocol == BOOT_LINUX, "`INITRD_PATH` is only available for linux");
                CHAR16* Path = StrStr(Line, L"=") + 1;

                // create the module entry
                BOOT_MODULE* Module = AllocateZeroPool(sizeof(BOOT_MODULE));
                Module->Fs = FS;
                Module->Tag = L"INITRD";
                CHECK_AND_RETHROW(ParseUri(Path, &Module->Fs, &Module->Path));
                InsertTailList(&CurrentEntry->BootModules, &Module->Link);

            } else if (CHECK_OPTION(L"MODULE_PATH")) {
                CHECK_TRACE(
                        CurrentEntry->Protocol == BOOT_MB2 ||
                        CurrentEntry->Protocol == BOOT_STIVALE ||
                        CurrentEntry->Protocol == BOOT_STIVALE2,
                        "`MODULE_PATH` is only available for mb2 and stivale{,2} (%d)", CurrentEntry->Protocol);
                CHAR16* Path = StrStr(Line, L"=") + 1;

                BOOT_MODULE* Module = AllocateZeroPool(sizeof(BOOT_MODULE));
                Module->Fs = FS;
                Module->Tag = L"";
                CHECK_AND_RETHROW(ParseUri(Path, &Module->Fs, &Module->Path));
                InsertTailList(&CurrentEntry->BootModules, &Module->Link);

                // this is the next one which will need a string
                if (CurrentModuleString == NULL) {
                    CurrentModuleString = Module;
                }

            } else if (CHECK_OPTION(L"MODULE_STRING")) {
                CHECK_TRACE(
                        CurrentEntry->Protocol == BOOT_MB2 ||
                        CurrentEntry->Protocol == BOOT_STIVALE ||
                        CurrentEntry->Protocol == BOOT_STIVALE2,
                        "`MODULE_STRING` is only available for mb2 and stivale{,2} (%d)", CurrentEntry->Protocol);
                CHECK_TRACE(CurrentModuleString != NULL, "MODULE_STRING must only appear after a MODULE_PATH");

                // set the tag
                CurrentModuleString->Tag = CopyString(StrStr(Line, L"=") + 1);

                // next
                if (IsNodeAtEnd(&CurrentEntry->BootModules, &CurrentModuleString->Link)) {
                    CurrentModuleString = NULL;
                } else {
                    CurrentModuleString = BASE_CR(GetNextNode(&CurrentEntry->BootModules, &CurrentModuleString->Link), BOOT_MODULE, Link);
                }
            }
        }
    }

cleanup:
    if (file != NULL) {
        FileHandleClose(file);
    }

    if (root != NULL) {
        FileHandleClose(root);
    }

    return Status;
}

EFI_STATUS GetBootEntries(LIST_ENTRY* Head) {
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_LOADED_IMAGE_PROTOCOL* LoadedImage = NULL;
    EFI_DEVICE_PATH_PROTOCOL* BootDevicePath = NULL;
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* BootFs = NULL;
    EFI_HANDLE FsHandle = NULL;

    // get the boot image device path
    EFI_CHECK(gBS->HandleProtocol(gImageHandle, &gEfiLoadedImageProtocolGuid, (void**)&LoadedImage));
    EFI_CHECK(gBS->HandleProtocol(LoadedImage->DeviceHandle, &gEfiDevicePathProtocolGuid, (void**)&BootDevicePath));

    // locate the file system
    EFI_CHECK(gBS->LocateDevicePath(&gEfiSimpleFileSystemProtocolGuid, &BootDevicePath, &FsHandle));
    EFI_CHECK(gBS->HandleProtocol(FsHandle, &gEfiSimpleFileSystemProtocolGuid, (void**)&BootFs));

    // try to load a config from it
    CHECK_AND_RETHROW(LoadBootEntries(BootFs, Head));

cleanup:
    return Status;
}