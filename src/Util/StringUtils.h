#ifndef __TOMATBOOT_STRING_UTILS_H__
#define __TOMATBOOT_STRING_UTILS_H__

#include <Base.h>
#include <Uefi.h>

CHAR16* StrDup(CHAR16* Original);

CHAR8* ConvertToAscii(CHAR16* Original);

const char* EfiMemoryTypeToString(EFI_MEMORY_TYPE Type);

#endif //__TOMATBOOT_STRING_UTILS_H__
