#ifndef __TOMATBOOT_DEVICE_PATH_UTILS_H__
#define __TOMATBOOT_DEVICE_PATH_UTILS_H__

#include <Uefi.h>
#include <Protocol/SimpleFileSystem.h>

EFI_DEVICE_PATH* LastDevicePathNode(EFI_DEVICE_PATH* Dp);

EFI_DEVICE_PATH* RemoveLastDevicePathNode(EFI_DEVICE_PATH* Dp);

EFI_STATUS EFIAPI OpenFileByDevicePath(EFI_DEVICE_PATH_PROTOCOL** FilePath, EFI_FILE_PROTOCOL** File, UINT64 OpenMode, UINT64 Attributes);

EFI_STATUS GetBootDevicePath(EFI_DEVICE_PATH** BootDrive);

#endif //__TOMATBOOT_DEVICE_PATH_UTILS_H__
