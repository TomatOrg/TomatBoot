#ifndef __UTIL_EXCEPT_H__
#define __UTIL_EXCEPT_H__

#include <Uefi.h>
#include <Library/UefiLib.h>

#define CHECK_ERROR_LABEL_TRACE(expr, error, label, fmt, ...) \
    do { \
        if (!(expr)) { \
            Status = error; \
            AsciiPrint("%r at %a (%a:%d)\n", Status, __func__, __FILE__, __LINE__); \
            if (fmt[0] != '\0') { \
                AsciiPrint(fmt "\n", ## __VA_ARGS__); \
            } \
            goto label; \
        } \
    } while(0)

#define CHECK_ERROR_TRACE(expr, error, fmt, ...) CHECK_ERROR_LABEL_TRACE(expr, error, cleanup, fmt, ## __VA_ARGS__)
#define CHECK_ERROR(expr, error) CHECK_ERROR_LABEL_TRACE(expr, error, cleanup, "")
#define CHECK(expr) CHECK_ERROR(expr, EFI_INVALID_PARAMETER)
#define CHECK_FAIL_ERROR(err) CHECK_ERROR(0, err)
#define CHECK_FAIL() CHECK(0)
#define CHECK_FAIL_TRACE(fmt, ...) CHECK_ERROR_TRACE(0, EFI_INVALID_PARAMETER, fmt, ## __VA_ARGS__)

#define EFI_CHECK(status) \
    do { \
        Status = status; \
        if (EFI_ERROR(Status)) { \
            AsciiPrint("%r at %a (%a:%d)\n", Status, __func__, __FILE__, __LINE__); \
            goto cleanup; \
        } \
    } while(0)

#define CHECK_AND_RETHROW_LABEL(error, label) \
    do { \
        Status = error; \
        if (EFI_ERROR(Status)) { \
            AsciiPrint("  rethrown at %a (%a:%d)\n", __func__, __FILE__, __LINE__); \
            goto label; \
        } \
    } while(0)

#define CHECK_AND_RETHROW(error) CHECK_AND_RETHROW_LABEL(error, cleanup)

#define WARN(expr, fmt, ...) \
    do { \
        if (!(expr)) { \
            AsciiPrint("Warning! " fmt " at (%a:%d) \n", ## __VA_ARGS__ , __func__, __FILE__, __LINE__); \
        } \
    } while(0)

#endif //__UTIL_EXCEPT_H__
