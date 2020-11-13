#include "FileUtils.h"

#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/FileHandleLib.h>

#include <Protocol/LoadedImage.h>

#include "Except.h"

EFI_STATUS FileRead(EFI_FILE_HANDLE Handle, void* Buffer, UINTN Size, UINTN Offset) {
    EFI_STATUS Status = EFI_SUCCESS;
    UINTN ReadSize = Size;

    EFI_CHECK(FileHandleSetPosition(Handle, Offset));
    EFI_CHECK(FileHandleRead(Handle, &ReadSize, Buffer));
    CHECK(ReadSize == Size);

cleanup:
    return Status;
}

EFI_STATUS GetBootFs(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL** BootFs) {
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_LOADED_IMAGE_PROTOCOL* LoadedImage = NULL;
    EFI_DEVICE_PATH_PROTOCOL* BootDevicePath = NULL;
    EFI_HANDLE Handle = NULL;

    CHECK(BootFs != NULL);

    // get the boot image device path
    EFI_CHECK(gBS->HandleProtocol(gImageHandle, &gEfiLoadedImageProtocolGuid, (void**)&LoadedImage));
    EFI_CHECK(gBS->HandleProtocol(LoadedImage->DeviceHandle, &gEfiDevicePathProtocolGuid, (void**)&BootDevicePath));

    // locate the file system
    EFI_CHECK(gBS->LocateDevicePath(&gEfiSimpleFileSystemProtocolGuid, &BootDevicePath, &Handle));
    EFI_CHECK(gBS->OpenProtocol(Handle, &gEfiSimpleFileSystemProtocolGuid, (void**)BootFs, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL));

cleanup:
    return Status;
}
