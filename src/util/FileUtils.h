#ifndef __UTIL_FILEUTILS_H__
#define __UTIL_FILEUTILS_H__

#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>

EFI_STATUS FileRead(EFI_FILE_HANDLE Handle, void* Buffer, UINTN Size, UINTN Offset);

EFI_STATUS GetBootFs(EFI_SIMPLE_FILE_SYSTEM_PROTOCOL** BootFs);

#endif //__UTIL_FILEUTILS_H__
