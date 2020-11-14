#include "FileUtils.h"

#include <Library/UefiBootServicesTableLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/FileHandleLib.h>
#include <Library/DevicePathLib.h>

#include <Protocol/LoadedImage.h>
#include <Protocol/BlockIo.h>

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
