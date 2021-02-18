#ifndef __TOMATBOOT_DEVICE_PATH_UTILS_H__
#define __TOMATBOOT_DEVICE_PATH_UTILS_H__

#include <Uefi.h>

EFI_DEVICE_PATH* LastDevicePathNode(EFI_DEVICE_PATH* Dp);

EFI_DEVICE_PATH* RemoveLastDevicePathNode(EFI_DEVICE_PATH* Dp);

#endif //__TOMATBOOT_DEVICE_PATH_UTILS_H__
