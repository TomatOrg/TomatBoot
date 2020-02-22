#include "FileUtils.h"

#include <Library/FileHandleLib.h>

#include "Except.h"

static EFI_STATUS FileRead(EFI_FILE_HANDLE Handle, void* Buffer, UINTN Size, UINTN Offset) {
    EFI_STATUS Status = EFI_SUCCESS;
    UINTN ReadSize = Size;

    EFI_CHECK(FileHandleSetPosition(Handle, Offset));
    EFI_CHECK(FileHandleRead(Handle, &ReadSize, Buffer));
    CHECK(ReadSize == Size);

cleanup:
    return Status;
}
