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

#define CHECK_OPTION(x) (StrnCmp(Line, x L"=", ARRAY_SIZE(x)) == 0)

BOOT_ENTRY* gDefaultEntry = NULL;
LIST_ENTRY gBootEntries = INITIALIZE_LIST_HEAD_VARIABLE(gBootEntries);

static CHAR16* ConfigPaths[] = {
        L"boot/tomatboot.cfg",
        L"tomatboot.cfg"
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

static EFI_STATUS ParseUriIntoBootEntry(CHAR16* Uri, BOOT_ENTRY* BootEntry) {
    EFI_STATUS Status = EFI_SUCCESS;

    if (StrStr(Uri, L":") != NULL) {
        CHECK_FAIL_TRACE("TODO: support uris", Uri);

    } else {
        // this is a normal path, just copy the path
        // and the fs is already set
        BootEntry->Path = CopyString(Uri);
    }

cleanup:
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
                CHECK_AND_RETHROW(ParseUriIntoBootEntry(Path, CurrentEntry));

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

                // create the module entry
                BOOT_MODULE* Module = AllocateZeroPool(sizeof(BOOT_MODULE));
                Module->Tag = L"INITRD";
                Module->Path = CopyString(StrStr(Line, L"=") + 1);
                Module->Fs = FS;
                InsertTailList(&CurrentEntry->BootModules, &Module->Link);

            } else if (CHECK_OPTION(L"MODULE_PATH")) {
                CHECK_TRACE(
                        CurrentEntry->Protocol == BOOT_MB2 ||
                        CurrentEntry->Protocol == BOOT_STIVALE ||
                        CurrentEntry->Protocol == BOOT_STIVALE2,
                        "`MODULE_PATH` is only available for mb2 and stivale{,2} (%d)", CurrentEntry->Protocol);

                BOOT_MODULE* Module = AllocateZeroPool(sizeof(BOOT_MODULE));
                Module->Path = CopyString(StrStr(Line, L"=") + 1);
                CurrentModuleString->Tag = L"";
                Module->Fs = FS;
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
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* FS = NULL;

    CHECK_AND_RETHROW(GetBootFs(&FS));
    CHECK_AND_RETHROW(LoadBootEntries(FS, Head));

cleanup:
    return Status;
}