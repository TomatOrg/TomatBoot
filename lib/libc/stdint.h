#ifndef KRETLIM_UEFI_BOOT_STDINT_H
#define KRETLIM_UEFI_BOOT_STDINT_H

#include "limits.h"

typedef unsigned char       uint8_t;
typedef unsigned short      uint16_t;
typedef unsigned int        uint32_t;
typedef unsigned long long  uint64_t;

typedef signed char         int8_t;
typedef signed short        int16_t;
typedef signed int          int32_t;
typedef signed long long    int64_t;

#define INT8_MIN            ((int8_t)0x80)
#define INT16_MIN           ((int16_t)0x8000)
#define INT32_MIN           ((int32_t)0x80000000)
#define INT64_MIN           ((int64_t)0x8000000000000000l)

#define INT8_MAX            ((int8_t)0x7F)
#define INT16_MAX           ((int16_t)0x7FFF)
#define INT32_MAX           ((int32_t)0x7FFFFFFF)
#define INT64_MAX           ((int64_t)0x7FFFFFFFFFFFFFFFl)

#define UINT8_MAX           ((uint8_t)0xFFu)
#define UINT16_MAX          ((uint16_t)0xFFFFu)
#define UINT32_MAX          ((uint32_t)0xFFFFFFFFu)
#define UINT64_MAX          ((uint64_t)0xFFFFFFFFFFFFFFFFul)

// TODO: Static asserts on sizes

#endif //KRETLIM_UEFI_BOOT_STDINT_H
