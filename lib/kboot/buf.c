#include "buf.h"

#include <stdio.h>

void *buf__grow(const void *buf, size_t new_len, size_t elem_size) {
    // TODO: assert(buf_cap(buf) <= (SIZE_MAX - 1)/2);
    size_t new_cap = CLAMP_MIN(2*buf_cap(buf), MAX(new_len, 16));
    // TODO: assert(new_len <= new_cap);
    // TODO: assert(new_cap <= (SIZE_MAX - offsetof(BufHdr, buf))/elem_size);
    size_t new_size = offsetof(bufhdr_t, buf) + new_cap * elem_size;
    bufhdr_t* new_hdr;
    if (buf) {
        new_hdr = realloc(buf__hdr(buf), new_size);
    } else {
        new_hdr = malloc(new_size);
        new_hdr->len = 0;
    }
    new_hdr->cap = new_cap;
    return new_hdr->buf;
}

wchar_t *buf__printf(wchar_t *buf, const wchar_t *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    size_t cap = buf_cap(buf) - buf_len(buf);
    size_t n = 1 + vsnprintf(buf_end(buf), cap, fmt, args);
    va_end(args);
    if (n > cap) {
        buf_fit(buf, n + buf_len(buf));
        va_start(args, fmt);
        size_t new_cap = buf_cap(buf) - buf_len(buf);
        n = 1 + vsnprintf(buf_end(buf), new_cap, fmt, args);
        // TODO: assert(n <= new_cap);
        va_end(args);
    }
    buf__hdr(buf)->len += n - 1;
    return buf;
}