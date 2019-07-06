#ifndef KRETLIM_UEFI_BOOT_BUF_H
#define KRETLIM_UEFI_BOOT_BUF_H

#include <stddef.h>
#include <stdarg.h>
#include <stdlib.h>

typedef struct bufhdr {
    size_t len;
    size_t cap;
    char buf[];
} bufhdr_t;

#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#define MAX(x, y) ((x) >= (y) ? (x) : (y))
#define CLAMP_MAX(x, max) MIN(x, max)
#define CLAMP_MIN(x, min) MAX(x, min)

#define buf__hdr(b) ((bufhdr_t *)((char *)(b) - offsetof(bufhdr_t, buf)))

#define buf_len(b) ((b) ? buf__hdr(b)->len : 0)
#define buf_cap(b) ((b) ? buf__hdr(b)->cap : 0)
#define buf_end(b) ((b) + buf_len(b))
#define buf_sizeof(b) ((b) ? buf_len(b)*sizeof(*b) : 0)

#define buf_free(b) ((b) ? (free(buf__hdr(b)), (b) = NULL) : 0)
#define buf_fit(b, n) ((n) <= buf_cap(b) ? 0 : ((b) = buf__grow((b), (n), sizeof(*(b)))))
#define buf_push(b, ...) (buf_fit((b), 1 + buf_len(b)), (b)[buf__hdr(b)->len++] = (__VA_ARGS__))
#define buf_printf(b, ...) ((b) = buf__printf((b), __VA_ARGS__))
#define buf_clear(b) ((b) ? buf__hdr(b)->len = 0 : 0)

void *buf__grow(const void *buf, size_t new_len, size_t elem_size);
wchar_t *buf__printf(wchar_t *buf, const wchar_t *fmt, ...);

#endif //TOMATKERNEL_BUF_H