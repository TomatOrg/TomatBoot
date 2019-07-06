#ifndef KRETLIM_UEFI_BOOT_HASH_H
#define KRETLIM_UEFI_BOOT_HASH_H

#include <stdint.h>
#include <stddef.h>

/**
 * Will hash a 64bit integer
 */
static inline uint64_t hash_uint64(uint64_t x) {
    x *= 0xff51afd7ed558ccd;
    x ^= x >> 32;
    return x;
}

/**
 * Will hash a 64bit pointer (same as hashing a 64bit integer)
 */
static inline uint64_t hash_ptr(const void *ptr) {
    return hash_uint64((uintptr_t)ptr);
}

/**
 * Hash two 64bit integers
 */
static inline uint64_t hash_mix(uint64_t x, uint64_t y) {
    x ^= y;
    x *= 0xff51afd7ed558ccd;
    x ^= x >> 32;
    return x;
}

/**
 * Hash the given buffer byte by byte
 *
 * @param ptr   [IN] Pointer to the buffer to hash
 * @param len   [IN] The amount of bytes to hash
 */
static inline uint64_t hash_bytes(const void *ptr, size_t len) {
    uint64_t x = 0xcbf29ce484222325;
    const char *buf = (const char *)ptr;
    for (size_t i = 0; i < len; i++) {
        x ^= buf[i];
        x *= 0x100000001b3;
        x ^= x >> 32;
    }
    return x;
}

#endif //KRETLIM_UEFI_BOOT_HASH_H