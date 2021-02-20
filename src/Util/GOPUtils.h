#ifndef __TOMATBOOT_GOPUTILS_H__
#define __TOMATBOOT_GOPUTILS_H__

#include <Uefi.h>

EFI_STATUS SelectBestGopMode(UINTN* Width, UINTN* Height, UINTN* Scanline, EFI_PHYSICAL_ADDRESS* Framebuffer);

#endif //__TOMATBOOT_GOPUTILS_H__
