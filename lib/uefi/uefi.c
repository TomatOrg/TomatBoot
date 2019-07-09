#include "uefi.h"

#include <stdio.h>

#define ABS(x) (x > 0 ? x : x * -1)

EFI_SYSTEM_TABLE* gST;
EFI_BOOT_SERVICES* gBS;
EFI_HANDLE gImageHandle;

static const unsigned short* gop_mode_names[] = {
	L"RGB (8x4)",
	L"BGR (8x4)",
	L"Bitmask",
	L"BLT Only",
};

INT32 GetGraphicsMode(EFI_GRAPHICS_OUTPUT_PROTOCOL* CONST gop, UINT32* CONST width, UINT32* CONST height) {
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* pModeInfo = 0;
	EFI_STATUS status = 0;
	UINTN size = 0;	
	INT32 best = gop->Mode->MaxMode;
	INTN found = 0; 
	while(best > 0) {
		best--;

		gop->QueryMode(gop, best, &size, &pModeInfo);
		printf(L"\t%dx%d (%s)\n\r", pModeInfo->HorizontalResolution, pModeInfo->VerticalResolution, gop_mode_names[pModeInfo->PixelFormat]);
		
		// only this format
		if(!found && pModeInfo->PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
			// max resolution
			if(*width != 0 && *width < pModeInfo->HorizontalResolution) continue;
			if(*height != 0 && *height < pModeInfo->VerticalResolution) continue;

			// found a good mode
			*width = pModeInfo->HorizontalResolution;
			*height = pModeInfo->VerticalResolution;
			
			found = 1;
		}
	}	
	return found ? best : -1;
}

EFI_FILE_PROTOCOL* OpenFile(EFI_FILE_PROTOCOL* CONST root, CONST CHAR16* CONST filename, UINT64 openMode, UINT64 attributes) {
	EFI_FILE_PROTOCOL* newFile = 0;

	EFI_STATUS status = root->Open((EFI_FILE_PROTOCOL*)root, &newFile, (CHAR16*)filename, openMode, attributes);

	if (status == EFI_SUCCESS) return newFile;
	
	switch (status) {
		case EFI_NOT_FOUND:
			printf(L"File not found: ");
			break;
		case EFI_NO_MEDIA:
			printf(L"No media: ");
			break;
		case EFI_MEDIA_CHANGED:
			printf(L"Media changed: ");
			break;
		case EFI_DEVICE_ERROR:
			printf(L"Device error: ");
			break;
		case EFI_VOLUME_CORRUPTED:
			printf(L"Volume corrupted: ");
			break;
		case EFI_WRITE_PROTECTED:
			printf(L"Write protected: ");
			break;
		case EFI_ACCESS_DENIED:
			printf(L"Access denied: ");
			break;
		case EFI_OUT_OF_RESOURCES:
			printf(L"Out of resources: ");
			break;
		case EFI_VOLUME_FULL:
			printf(L"Volume full: ");
			break;
		default:
			printf(L"Unknown error code %H (file handle %H root handle %H): ", status, newFile, root);
	}

	printf(L"\"%s\"\n", filename, status);

	return 0;
}

VOID FlushFile(EFI_FILE_PROTOCOL* CONST file) {
	file->Flush(file);
}

VOID CloseFile(EFI_FILE_PROTOCOL* CONST file) {
	file->Close(file);
}

BOOLEAN DeleteFile(EFI_FILE_PROTOCOL* CONST file) {
	return file->Delete(file) == EFI_SUCCESS ? 1 : 0;
}

EFI_STATUS ReadFile(EFI_FILE_PROTOCOL* CONST file, UINTN* CONST size, VOID* CONST buffer) {
	return file->Read(file, size, buffer);
}

EFI_STATUS WriteFile(EFI_FILE_PROTOCOL* CONST file, UINTN* CONST size, CONST VOID* CONST buffer) {
	return file->Write(file, size, (VOID*)buffer);
}

EFI_STATUS SetPosition(EFI_FILE_PROTOCOL* CONST file, UINT64 position) {
	return file->SetPosition(file, position);
}

UINT64 GetPosition(EFI_FILE_PROTOCOL* CONST file) {
	UINT64 pos;

	EFI_STATUS status = file->GetPosition(file, &pos);

	if (status == EFI_UNSUPPORTED) {
		pos = ~0;
	}

	return pos;
}
