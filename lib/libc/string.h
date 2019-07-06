#ifndef KRETLIM_UEFI_BOOT_STRING_H
#define KRETLIM_UEFI_BOOT_STRING_H

#include <stddef.h>

#define NULL 0

/**
 *
 *
 * @param destination
 * @param source
 * @param num
 * @return
 */
void* memcpy(void* destination, const void* source, size_t num);

/**
 *
 * @param dest
 * @param src
 * @param num
 * @return
 */
// void* memmove(void* dest, const void* src, size_t num);

/**
 *
 * @param ptr1
 * @param ptr2
 * @param num
 * @return
 */
int memcmp(const void* ptr1, const void* ptr2, size_t num);

/**
 *
 * @param ptr
 * @param value
 * @param num
 * @return
 */
void* memchr(const void * ptr, int value, size_t num);

/**
 *
 * @param ptr
 * @param value
 * @param num
 * @return
 */
void* memset(void* ptr, int value, size_t num);

size_t wcslen(const wchar_t* str);
size_t wcscmp(const wchar_t* str1, const wchar_t* str2);

#endif //KRETLIM_UEFI_BOOT_STRING_H
