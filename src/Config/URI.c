#include <Library/BaseLib.h>
#include <Util/StringUtils.h>
#include <Util/Except.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Util/DevicePathUtils.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/LoadedImage.h>
#include <Library/DevicePathLib.h>
#include <Protocol/BlockIo.h>
#include "URI.h"

static EFI_STATUS ResolveBootResource(CHAR16* Root, CHAR16* Path, EFI_DEVICE_PATH** DevicePath) {
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_DEVICE_PATH* NewPath = NULL;
    EFI_DEVICE_PATH* TempPath = NULL;
    FILEPATH_DEVICE_PATH* FilePath = NULL;
    EFI_HANDLE* Handles = NULL;
    BOOLEAN DeleteDevicePath = FALSE;
    UINTN HandleCount = 0;

    if (*Root == CHAR_NULL) {
        // the partition number is missing, this means we are
        // looking for the root file system
        EFI_CHECK(gBS->HandleProtocol(gImageHandle, &gEfiLoadedImageDevicePathProtocolGuid, (void**)&NewPath));

        // make sure the last node is a filepath
        EFI_DEVICE_PATH* LastNode = LastDevicePathNode(NewPath);
        CHECK(LastNode->Type == MEDIA_DEVICE_PATH && LastNode->SubType == MEDIA_FILEPATH_DP);

        // now remove the last element (Which would be the device path)
        NewPath = RemoveLastDevicePathNode(NewPath);
        DeleteDevicePath = TRUE;
        CHECK_STATUS(NewPath != NULL, EFI_OUT_OF_RESOURCES);

    } else {
        // there is a provided partition number
        UINTN PartNum = StrDecimalToUintn(Root);
        CHECK(PartNum >= 1, "Invalid partition number `%s`", Root);

        // get the boot device path, then iterate the filesystems and check if there is
        // one with the correct partition
        CHECK_AND_RETHROW(GetBootDevicePath(&TempPath));

        // get all the filesystems with this device path
        EFI_CHECK(gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &HandleCount, &Handles));
        for (int i = 0; i < HandleCount; i++) {
            // open it
            EFI_CHECK(gBS->HandleProtocol(Handles[i], &gEfiDevicePathProtocolGuid, (void **)&NewPath));
            EFI_DEVICE_PATH* Last = LastDevicePathNode(NewPath);

            // skip anything which is not on our boot drive
            if (!InsideDevicePath(NewPath, TempPath)) {
                continue;
            }

            if (Last->Type != MEDIA_DEVICE_PATH || Last->SubType != MEDIA_HARDDRIVE_DP) {
                // not a valid hard drive, aka, doesn't have partition
                NewPath = NULL;
                continue;
            }

            // check if correct partition
            HARDDRIVE_DEVICE_PATH* Harddrive = (HARDDRIVE_DEVICE_PATH*)Last;
            if (Harddrive->PartitionNumber != PartNum) {
                NewPath = NULL;
                continue;
            }

            // found it!
            break;
        }
    }

    // check if we found it
    CHECK_STATUS(NewPath != NULL, EFI_NOT_FOUND, "The root for boot resource `%a` could not be found...", Root);

    // allocate the node for the device path
    FilePath = (FILEPATH_DEVICE_PATH*)CreateDeviceNode(MEDIA_DEVICE_PATH, MEDIA_FILEPATH_DP, SIZE_OF_FILEPATH_DEVICE_PATH + 255);
    CHECK_STATUS(FilePath != NULL, EFI_OUT_OF_RESOURCES);

    // copy the pathname
    StrCpyS(FilePath->PathName, 255, Path);
    *DevicePath = AppendDevicePathNode(NewPath, (EFI_DEVICE_PATH_PROTOCOL*)FilePath);
    CHECK_STATUS(*DevicePath != NULL, EFI_OUT_OF_RESOURCES);

cleanup:
    if (NewPath == NULL && DeleteDevicePath) {
        FreePool(NewPath);
    }
    if (TempPath != NULL) {
        FreePool(TempPath);
    }
    if (FilePath != NULL) {
        FreePool(FilePath);
    }
    if (Handles != NULL) {
        FreePool(Handles);
    }
    return Status;
}

static EFI_STATUS ResolveGuidResource(CHAR16* Root, CHAR16* Path, EFI_DEVICE_PATH** DevicePath) {
    EFI_STATUS Status = EFI_SUCCESS;

    CHECK_FAIL_STATUS(EFI_UNSUPPORTED);

cleanup:
    return Status;
}

EFI_STATUS ParseURI(CHAR16* Uri, EFI_DEVICE_PATH** DevicePath) {
    EFI_STATUS Status = EFI_SUCCESS;

    // separate the domain from the uri type
    CHAR16* Root = StrStr(Uri, L":");
    CHECK(Root != NULL, "Missing resource: `%s`", Uri);
    *Root = L'\0';
    Root += 3; // skip ://

    // create the path itself
    CHAR16* Path = StrStr(Root, L"/");
    CHECK(Root != NULL, "Missing path: `%s`", Uri);
    *Path = L'\0';
    Path++; // skip the /

    // convert `/` to `\` for uefi to be happy
    for (CHAR16* C = Path; *C != CHAR_NULL; C++) {
        if (*C == '/') {
            *C = '\\';
        }
    }

    // resolve it correctly
    if (StrCmp(Uri, L"boot") == 0) {
        CHECK_AND_RETHROW(ResolveBootResource(Root, Path, DevicePath));

    } else if (StrCmp(Uri, L"guid") == 0 || StrCmp(Uri, L"uuid") == 0) {
        CHECK_AND_RETHROW(ResolveGuidResource(Root, Path, DevicePath));
    }

cleanup:
    return Status;
}
