
#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/DriverBinding.h>
#include <Util/Except.h>

EFI_STATUS ExtFsSupported(EFI_DRIVER_BINDING_PROTOCOL* This, EFI_HANDLE ControllerHandle, EFI_DEVICE_PATH_PROTOCOL* RemainingDevicePath) {
    EFI_STATUS Status = EFI_SUCCESS;

    CHECK_FAIL_STATUS(EFI_UNSUPPORTED);

cleanup:
    return Status;
}


EFI_STATUS ExtFsStart(EFI_DRIVER_BINDING_PROTOCOL* This, EFI_HANDLE ControllerHandle, EFI_DEVICE_PATH_PROTOCOL* RemainingDevicePath) {
    EFI_STATUS Status = EFI_SUCCESS;

    TRACE("Meow Start");

cleanup:
    return Status;
}

EFI_STATUS ExtFsStop(EFI_DRIVER_BINDING_PROTOCOL* This, EFI_HANDLE ControllerHandle, UINTN NumberOfChildren, EFI_HANDLE* ChildHandleBuffer) {
    EFI_STATUS Status = EFI_SUCCESS;

    TRACE("Meow Stop");

cleanup:
    return Status;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Register everything
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/**
 * The binding protocol
 */
static EFI_DRIVER_BINDING_PROTOCOL gExtFsDriverBinding = {
    .Supported = ExtFsSupported,
    .Start = ExtFsStart,
    .Stop = ExtFsStop,
};

EFI_STATUS LoadExtFs() {
    EFI_STATUS Status = EFI_SUCCESS;

    // setup the handles
    gExtFsDriverBinding.DriverBindingHandle = gImageHandle;
    gExtFsDriverBinding.ImageHandle = gImageHandle;

    // install the binding
    gBS->InstallMultipleProtocolInterfaces(&gExtFsDriverBinding.DriverBindingHandle,
            &gEfiDriverBindingProtocolGuid, &gExtFsDriverBinding,
            NULL, NULL);

cleanup:
    return Status;
}
