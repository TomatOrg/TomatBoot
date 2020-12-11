#ifndef __UTIL_EXCEPT_H__
#define __UTIL_EXCEPT_H__

#include <Uefi.h>
#include <Library/UefiLib.h>

#ifndef __FILENAME__
    #define __FILENAME__ __FILE__
#endif

#define TRACE(fmt, ...) Print(L"[*] " fmt "\n", ## __VA_ARGS__);
#define WARN(fmt, ...) Print(L"[!] " fmt "\n", ## __VA_ARGS__);
#define ERROR(fmt, ...) Print(L"[-] " fmt "\n", ## __VA_ARGS__);

#define CHECK_ERROR_LABEL_TRACE(expr, error, label, fmt, ...) \
    do { \
        if (!(expr)) { \
            Status = error; \
            ERROR("%r at %a (%a:%d)", Status, __func__, __FILENAME__, __LINE__); \
            if (fmt[0] != '\0') { \
                ERROR(fmt, ## __VA_ARGS__); \
            } \
            goto label; \
        } \
    } while(0)

#define CHECK_ERROR_LABEL(expr, error, label) CHECK_ERROR_LABEL_TRACE(expr, error, label, "")
#define CHECK_ERROR_TRACE(expr, error, fmt, ...) CHECK_ERROR_LABEL_TRACE(expr, error, cleanup, fmt, ## __VA_ARGS__)
#define CHECK_ERROR(expr, error) CHECK_ERROR_LABEL_TRACE(expr, error, cleanup, "")
#define CHECK_TRACE(expr, fmt, ...) CHECK_ERROR_TRACE(expr, EFI_INVALID_PARAMETER, fmt, ## __VA_ARGS__)
#define CHECK(expr) CHECK_ERROR(expr, EFI_INVALID_PARAMETER)

#define CHECK_FAIL_ERROR(err) CHECK_ERROR(0, err)
#define CHECK_FAIL() CHECK(0)
#define CHECK_FAIL_TRACE(fmt, ...) CHECK_ERROR_TRACE(0, EFI_INVALID_PARAMETER, fmt, ## __VA_ARGS__)

#define EFI_CHECK_LABEL(status, label) \
    do { \
        Status = status; \
        if (EFI_ERROR(Status)) { \
            ERROR("%r at %a (%a:%d)", Status, __func__, __FILENAME__, __LINE__); \
            goto label; \
        } \
    } while(0)

#define EFI_CHECK(status) EFI_CHECK_LABEL(status, cleanup)

#define CHECK_AND_RETHROW_LABEL(error, label) \
    do { \
        Status = error; \
        if (EFI_ERROR(Status)) { \
            ERROR("  rethrown at %a (%a:%d)", __func__, __FILENAME__, __LINE__); \
            goto label; \
        } \
    } while(0)

#define CHECK_AND_RETHROW(error) CHECK_AND_RETHROW_LABEL(error, cleanup)

#define WARN_ON(expr, fmt, ...) \
    do { \
        if (expr) { \
            WARN("Warning! " fmt " at (%a:%d)", ## __VA_ARGS__ , __func__, __FILENAME__, __LINE__); \
        } \
    } while(0)

#endif //__UTIL_EXCEPT_H__
