#include "DPUtils.h"
#include "Except.h"

#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>

BOOLEAN InsideDevicePath(EFI_DEVICE_PATH* All, EFI_DEVICE_PATH* One) {
    // iterate this one
    EFI_DEVICE_PATH* Path;
    for (
        Path = One;
        DevicePathNodeLength(Path) == DevicePathNodeLength(All) &&
        !IsDevicePathEndType(Path) &&
        CompareMem(Path, All, DevicePathNodeLength(All)) == 0;
        Path = NextDevicePathNode(Path), All = NextDevicePathNode(All)
    ) {
        TRACE("MATCH");
    }

    // return true if we reached the end of the one device path
    // that we were looking for
    return IsDevicePathEndType(Path);
}

EFI_DEVICE_PATH* LastDevicePathNode(EFI_DEVICE_PATH* Dp) {
    if (Dp == NULL) {
        return NULL;
    }

    EFI_DEVICE_PATH* LastDp = NULL;
    for ( ; !IsDevicePathEndType(Dp); Dp = NextDevicePathNode(Dp)) {
        LastDp = Dp;
    }

    return LastDp;
}

EFI_DEVICE_PATH* RemoveLastDevicePathNode(EFI_DEVICE_PATH* Dp) {
    if (Dp == NULL) {
        return NULL;
    }

    // get the last node and calculate the new node size
    EFI_DEVICE_PATH* LastNode = LastDevicePathNode(Dp);
    UINTN Len = (UINTN)LastNode - (UINTN)Dp;

    // allocate the new node with another one for the path end
    EFI_DEVICE_PATH* NewNode = AllocatePool(Len + sizeof(EFI_DEVICE_PATH));

    // copy it
    CopyMem(NewNode, Dp, Len);

    // set the last one to be empty
    LastNode = (EFI_DEVICE_PATH*)((UINTN)NewNode + Len);
    SetDevicePathEndNode(LastNode);

    return NewNode;
}
