#ifndef KRETLIM_UEFI_BOOT_STDDEF_H
#define KRETLIM_UEFI_BOOT_STDDEF_H

#include <stdint.h>

typedef uint64_t size_t;
typedef uint64_t uintptr_t;
typedef int64_t ptrdiff_t;
typedef unsigned short wchar_t; 

#define offsetof(member, type) __builtin_offsetof(member, type)

#endif //KRETLIM_UEFI_BOOT_STDDEF_H
