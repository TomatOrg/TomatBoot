#include "uefi.h"

#include <stdio.h>

#define ABS(x) (x > 0 ? x : x * -1)

EFI_SYSTEM_TABLE* gST;
EFI_BOOT_SERVICES* gBS;
EFI_HANDLE gImageHandle;

UINT32 GetGraphicsMode(EFI_GRAPHICS_OUTPUT_PROTOCOL* CONST gop, UINT32* CONST width, UINT32* CONST height, EFI_GRAPHICS_PIXEL_FORMAT* CONST format) {
	EFI_GRAPHICS_OUTPUT_MODE_INFORMATION* pModeInfo = 0;

	EFI_STATUS status = 0;
	UINTN size = 0;

	for (UINT32 i = 0; i < gop->Mode->MaxMode; i++) {
		gop->QueryMode(gop, i, &size, &pModeInfo);
		if (pModeInfo->HorizontalResolution == *width && pModeInfo->VerticalResolution == *height && pModeInfo->PixelFormat == *format) {
			return i;
		}
	}
	
	UINT32 best = ~0;

	if (*width == 0 && *height == 0) {
		best = gop->Mode->MaxMode-1;

		gop->QueryMode(gop, best, &size, &pModeInfo);

		*width = pModeInfo->HorizontalResolution;
		*height = pModeInfo->VerticalResolution;
		*format = pModeInfo->PixelFormat;
	} else {
		UINT32 tmpWidth = *width;
		UINT32 tmpIndex = 0;

		for (UINT32 i = 0; i < gop->Mode->MaxMode; i++) {
			gop->QueryMode(gop, i, &size, &pModeInfo);

			INT32 w = (INT32)pModeInfo->HorizontalResolution;

			INT32 wDiff = w - (INT32)(*width);

			if (ABS(wDiff) < tmpWidth) {
				tmpWidth = ABS(wDiff);
				tmpIndex = i;
			}
		}

		gop->QueryMode(gop, tmpIndex, &size, &pModeInfo);

		*width = pModeInfo->HorizontalResolution;
		*height = pModeInfo->VerticalResolution;
		*format = pModeInfo->PixelFormat;

		best = tmpIndex;
	}
	
	return best;
}

UINTN GetTextMode(EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL* CONST text, UINTN* CONST columns, UINTN* CONST rows) {
	UINTN c;
	UINTN r;

	if (*columns == 80 && *rows == 25) return 0;

	if (*columns == 80 && *rows == 50) {
		if (text->QueryMode(text, 1, &c, &r) != EFI_UNSUPPORTED) {
			return 1;
		} 

		*rows = 25;

		return 0;
	}

	for (UINTN i = 2; i < text->Mode->MaxMode; i++) {
		text->QueryMode(text, i, &c, &r);

		if (c == *columns && r == *rows) {
			return i;
		}
	}

	UINTN best = text->Mode->MaxMode-1;

	text->QueryMode(text, best, columns, rows);

	return best;
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
