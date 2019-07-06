#ifndef KRETLIM_UEFI_BOOT_LIMITS_H
#define KRETLIM_UEFI_BOOT_LIMITS_H

#define CHAR_BIT            8

#define SCHAR_MIN           0x80
#define SHRT_MIN            0x8000
#define INT_MIN             0x80000000
#define LONG_MIN            0x8000000000000000l
#define LLONG_MIN           0x8000000000000000ll

#define SCHAR_MAX           0x7F
#define SHRT_MAX            0x7FFF
#define INT_MAX             0x7FFFFFFF
#define LONG_MAX            0x7FFFFFFFFFFFFFFFl
#define LLONG_MAX           0x7FFFFFFFFFFFFFFFll

#define UCHAR_MAX           0xFFu
#define USHRT_MAX           0xFFFFu
#define UINT_MAX            0xFFFFFFFFu
#define ULONG_MAX           0xFFFFFFFFFFFFFFFFul
#define ULLONG_MAX          0xFFFFFFFFFFFFFFFFull

#endif //KRETLIM_UEFI_BOOT_LIMITS_H
