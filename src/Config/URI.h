#ifndef __TOMATBOOT_URI_H__
#define __TOMATBOOT_URI_H__

#include <Uefi.h>

/**
 * Parse a URI into a device path
 *
 * @param Uri           [IN]    The uri
 * @param DevicePath    [OUT]   The device path
 */
EFI_STATUS ParseURI(CHAR16* Uri, EFI_DEVICE_PATH** DevicePath);

#endif //__TOMATBOOT_URI_H__
