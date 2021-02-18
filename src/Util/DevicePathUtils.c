#include <Library/MemoryAllocationLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DevicePathLib.h>
#include "DevicePathUtils.h"

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
