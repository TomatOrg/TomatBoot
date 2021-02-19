#include <Uefi.h>
#include <Util/Except.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Drivers/ExtFs.h>
#include <Library/MemoryAllocationLib.h>
#include <Protocol/SimpleFileSystem.h>
#include "Config/Config.h"

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// constructors
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EFI_STATUS EFIAPI UefiBootServicesTableLibConstructor(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable);
EFI_STATUS EFIAPI DxeDebugLibConstructor(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable);

static EFI_STATUS CallConstructors(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    EFI_STATUS Status = EFI_SUCCESS;

    CHECK_AND_RETHROW(UefiBootServicesTableLibConstructor(ImageHandle, SystemTable));
    CHECK_AND_RETHROW(DxeDebugLibConstructor(ImageHandle, SystemTable));

cleanup:
    return Status;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Entry point!
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

EFI_HANDLE*
EFIAPI
GetHandleListByProtocol (
        IN CONST EFI_GUID *ProtocolGuid OPTIONAL
)
{
    EFI_HANDLE          *HandleList;
    UINTN               Size;
    EFI_STATUS          Status;

    Size = 0;
    HandleList = NULL;

    //
    // We cannot use LocateHandleBuffer since we need that NULL item on the ends of the list!
    //
    if (ProtocolGuid == NULL) {
        Status = gBS->LocateHandle(AllHandles, NULL, NULL, &Size, HandleList);
        if (Status == EFI_BUFFER_TOO_SMALL) {
            HandleList = AllocateZeroPool(Size + sizeof(EFI_HANDLE));
            if (HandleList == NULL) {
                return (NULL);
            }
            Status = gBS->LocateHandle(AllHandles, NULL, NULL, &Size, HandleList);
            HandleList[Size/sizeof(EFI_HANDLE)] = NULL;
        }
    } else {
        Status = gBS->LocateHandle(ByProtocol, (EFI_GUID*)ProtocolGuid, NULL, &Size, HandleList);
        if (Status == EFI_BUFFER_TOO_SMALL) {
            HandleList = AllocateZeroPool(Size + sizeof(EFI_HANDLE));
            if (HandleList == NULL) {
                return (NULL);
            }
            Status = gBS->LocateHandle(ByProtocol, (EFI_GUID*)ProtocolGuid, NULL, &Size, HandleList);
            HandleList[Size/sizeof(EFI_HANDLE)] = NULL;
        }
    }
    if (EFI_ERROR(Status)) {
        if (HandleList != NULL) {
            FreePool(HandleList);
        }
        return (NULL);
    }
    return (HandleList);
}

static EFI_STATUS ConnectControllers() {
    EFI_STATUS Status = EFI_SUCCESS;
    EFI_HANDLE* ControllerHandleList = NULL;

    // Get all the device paths
    ControllerHandleList = GetHandleListByProtocol(&gEfiDevicePathProtocolGuid);
    CHECK_STATUS(ControllerHandleList != NULL, EFI_OUT_OF_RESOURCES);

    // try to connect for each of them
    for (EFI_HANDLE* HandleWalker = ControllerHandleList; HandleWalker != NULL && *HandleWalker != NULL; HandleWalker++){
        Status = gBS->ConnectController(*HandleWalker, NULL, NULL, FALSE);
        if (Status == EFI_NOT_FOUND) {
            Status = EFI_SUCCESS;
        }
        EFI_CHECK(Status);
    }

cleanup:
    if (ControllerHandleList != NULL) {
        FreePool(ControllerHandleList);
    }
    return Status;
}

EFI_STATUS EfiMain(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    EFI_STATUS Status = EFI_SUCCESS;

    // setup nicely
    CHECK_AND_RETHROW(CallConstructors(ImageHandle, SystemTable));
    EFI_CHECK(SystemTable->ConOut->ClearScreen(SystemTable->ConOut));

    TRACE("Registering drivers and connecting them...");
    CHECK_AND_RETHROW(LoadExtFs());
    CHECK_AND_RETHROW(ConnectControllers());

    EFI_HANDLE* Handles = NULL;
    UINTN HandleCount = 0;
    EFI_CHECK(gBS->LocateHandleBuffer(ByProtocol, &gEfiSimpleFileSystemProtocolGuid, NULL, &HandleCount, &Handles));
    for (int i = 0; i < HandleCount; i++) {
        EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* SimpleFileSystem = NULL;
        EFI_CHECK(gBS->HandleProtocol(Handles[i], &gEfiSimpleFileSystemProtocolGuid, (void**)&SimpleFileSystem));

        EFI_FILE_PROTOCOL* RootFile;
        EFI_CHECK(SimpleFileSystem->OpenVolume(SimpleFileSystem, &RootFile));

        TRACE("Opening hello");

        EFI_FILE_PROTOCOL* Yes;
        RootFile->Open(RootFile, &Yes, L"Hello", 0, 0);

        TRACE("Done");
    }

//    TRACE("Reading the Config...");
//    CHECK_AND_RETHROW(ParseConfig());

cleanup:
    TRACE("Done...");
    while(1);
}
