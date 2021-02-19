#include <Library/BaseLib.h>
#include <Util/StringUtils.h>
#include <Util/Except.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Util/DevicePathUtils.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/LoadedImage.h>
#include <Library/DevicePathLib.h>
#include "URI.h"

static EFI_STATUS ResolveBootResource(CHAR16* Root, CHAR16* Path, EFI_DEVICE_PATH** DevicePath) {
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_DEVICE_PATH* NewPath = NULL;
    FILEPATH_DEVICE_PATH* FilePath = NULL;

    if (*Root == CHAR_NULL) {
        // the partition number is missing, this means we are
        // looking for the root file system
        EFI_CHECK(gBS->HandleProtocol(gImageHandle, &gEfiLoadedImageDevicePathProtocolGuid, (void**)&NewPath));

        // now remove the last element (Which would be the device path)
        NewPath = RemoveLastDevicePathNode(NewPath);
        CHECK_STATUS(NewPath != NULL, EFI_OUT_OF_RESOURCES);
    } else {
        // there is a provided partition number
        UINTN PartNum = StrDecimalToUintn(Root);
        CHECK(PartNum >= 1, "Invalid partition number `%s`", Root);

        // TODO: get the device path of the hard drive and iterate all its child filesystems to
        //       find the correct one
        CHECK_FAIL_STATUS(EFI_UNSUPPORTED);
    }

    // allocate the node for the device path
    FilePath = (FILEPATH_DEVICE_PATH*)CreateDeviceNode(MEDIA_DEVICE_PATH, MEDIA_FILEPATH_DP, SIZE_OF_FILEPATH_DEVICE_PATH + 255);
    CHECK_STATUS(FilePath != NULL, EFI_OUT_OF_RESOURCES);

    // copy the pathname
    StrCpyS(FilePath->PathName, 255, Path);
    *DevicePath = AppendDevicePathNode(NewPath, (EFI_DEVICE_PATH_PROTOCOL*)FilePath);
    CHECK_STATUS(*DevicePath != NULL, EFI_OUT_OF_RESOURCES);

cleanup:
    if (NewPath == NULL) {
        FreePool(NewPath);
    }
    if (FilePath != NULL) {
        FreePool(FilePath);
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
