#ifndef TOMATOS_DPUTILS_H
#define TOMATOS_DPUTILS_H

#include <Uefi.h>
#include <Protocol/DevicePath.h>
#include <Library/DevicePathLib.h>

BOOLEAN InsideDevicePath(EFI_DEVICE_PATH* All, EFI_DEVICE_PATH* One);

EFI_DEVICE_PATH* LastDevicePathNode(EFI_DEVICE_PATH* Dp);

EFI_DEVICE_PATH* RemoveLastDevicePathNode(EFI_DEVICE_PATH* Dp);

#endif //TOMATOS_DPUTILS_H
