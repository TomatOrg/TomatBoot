#include <Protocol/LoadedImage.h>

#include <Library/DevicePathLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Util/Except.h>
#include <Util/DevicePathUtils.h>
#include <Library/MemoryAllocationLib.h>

#include "Config.h"

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

static EFI_STATUS SearchForConfig(EFI_DEVICE_PATH** ConfigFile) {
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_DEVICE_PATH* BootDevicePath = NULL;

    CHECK(ConfigFile != NULL);
    *ConfigFile = NULL;

    // get the boot device path
    CHECK_AND_RETHROW(GetBootDevicePath(&BootDevicePath));



cleanup:
    if (BootDevicePath != NULL) {
        FreePool(BootDevicePath);
    }
    return Status;
}

EFI_STATUS ParseConfig() {
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_DEVICE_PATH* ConfigFilePath = NULL;

    CHECK_AND_RETHROW(SearchForConfig(&ConfigFilePath));

cleanup:
    return Status;
}
