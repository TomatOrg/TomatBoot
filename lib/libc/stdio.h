#ifndef KRETLIM_UEFI_BOOT_STDIO_H
#define KRETLIM_UEFI_BOOT_STDIO_H

#include <stdint.h>
#include <stdarg.h>
#include <stddef.h>

typedef void FILE;

void __attribute__((cdecl)) printf(const wchar_t* const format, ...);
void __attribute__((cdecl)) vprintf(const wchar_t* const format, va_list list);
size_t __attribute__((cdecl)) snprintf(wchar_t* const buffer, size_t bufferSize, const wchar_t* const format, ...);
size_t __attribute__((cdecl)) vsnprintf(wchar_t* const buffer, size_t bufferSize, const wchar_t* const format, va_list list);
size_t __attribute__((cdecl)) fprintf(FILE* const file, const wchar_t* const format, ...);

#endif