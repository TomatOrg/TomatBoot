#ifndef __TOMATBOOT_EXCEPT_H__
#define __TOMATBOOT_EXCEPT_H__

#include <Library/DebugLib.h>
#include "CppMagic.h"

#define TRACE(fmt, ...) DebugPrint(DEBUG_INFO, "[*] " fmt "\n", ## __VA_ARGS__)
#define WARN(fmt, ...) DebugPrint(DEBUG_WARN, "[!] " fmt "\n", ## __VA_ARGS__)
#define ERROR(fmt, ...) DebugPrint(DEBUG_ERROR, "[-] " fmt "\n", ## __VA_ARGS__)

#define CHECK_STATUS_LABEL(expr, status, label, ...) \
    do { \
        if (!(expr)) { \
            Status = status; \
            IF(HAS_ARGS(__VA_ARGS__))(ERROR(__VA_ARGS__);) \
            ERROR("Check failed with status %r at %a (%a:%d)", Status, __FUNCTION__, __FILE__, __LINE__); \
            goto label; \
        } \
    } while(0)

#define CHECK_STATUS(expr, status, ...) CHECK_STATUS_LABEL(expr, status, cleanup, ##__VA_ARGS__)
#define CHECK_LABEL(expr, label, ...)   CHECK_STATUS_LABEL(expr, RETURN_INVALID_PARAMETER, label, ##__VA_ARGS__)
#define CHECK(expr, ...)                CHECK_STATUS_LABEL(expr, RETURN_INVALID_PARAMETER, cleanup, ##__VA_ARGS__)

#define CHECK_FAIL_STATUS_LABEL(status, label, ...) CHECK_STATUS_LABEL(0, status, label, ##__VA_ARGS__)
#define CHECK_FAIL_STATUS(status, ...)              CHECK_STATUS(0, status, ##__VA_ARGS__)
#define CHECK_FAIL_LABEL(label, ...)                CHECK_LABEL(0, label, ##__VA_ARGS__)
#define CHECK_FAIL(...)                             CHECK(0, __VA_ARGS__)

#define CHECK_AND_RETHROW_LABEL(expr, label) \
    do { \
        Status = expr; \
        if (Status != RETURN_SUCCESS) { \
            ERROR("    rethrown at %a (%a:%d)", __FUNCTION__, __FILE__, __LINE__); \
            goto label; \
        } \
    } while(0)

#define CHECK_AND_RETHROW(expr) CHECK_AND_RETHROW_LABEL(expr, cleanup)

#define WARN_ON(expr, fmt, ...) \
    do { \
        if (expr) { \
            WARN(fmt, ##__VA_ARGS__); \
        } \
    } while(0)

#define WARN_ON_ERROR(expr, fmt, ...) WARN_ON((expr) != RETURN_SUCCESS, fmt, ##__VA_ARGS__)

#endif //__TOMATBOOT_EXCEPT_H__
