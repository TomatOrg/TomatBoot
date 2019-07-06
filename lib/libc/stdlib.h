#ifndef KRETLIM_UEFI_BOOT_STDLIB_H
#define KRETLIM_UEFI_BOOT_STDLIB_H

#include <stddef.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

void* malloc(size_t size);
void* calloc(size_t size, size_t elemSize);
void* realloc(void* ptr, size_t new_size);
void free(void* ptr);

#endif