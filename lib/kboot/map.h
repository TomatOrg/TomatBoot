#ifndef KRETLIM_UEFI_BOOT_MAP_H
#define KRETLIM_UEFI_BOOT_MAP_H

#include <stddef.h>
#include <stdlib.h>
#include "hash.h"

typedef struct map {
    uint64_t *keys;
    uint64_t *vals;
    size_t len;
    size_t cap;
} map_t;

void map_grow(map_t *map, size_t new_cap);
void map_put_uint64_from_uint64(map_t *map, uint64_t key, uint64_t val);
uint64_t map_get_uint64_from_uint64(map_t *map, uint64_t key);

static inline void *map_get(map_t *map, const void *key) {
    return (void *)(uintptr_t)map_get_uint64_from_uint64(map, (uint64_t)(uintptr_t)key);
}

static inline void map_put(map_t *map, const void *key, void *val) {
    map_put_uint64_from_uint64(map, (uint64_t)(uintptr_t)key, (uint64_t)(uintptr_t)val);
}

static inline void *map_get_from_uint64(map_t *map, uint64_t key) {
    return (void *)(uintptr_t)map_get_uint64_from_uint64(map, key);
}

static inline void map_put_from_uint64(map_t *map, uint64_t key, void *val) {
    map_put_uint64_from_uint64(map, key, (uint64_t)(uintptr_t)val);
}

static inline uint64_t map_get_uint64(map_t *map, void *key) {
    return map_get_uint64_from_uint64(map, (uint64_t)(uintptr_t)key);
}

static inline void map_put_uint64(map_t *map, void *key, uint64_t val) {
    map_put_uint64_from_uint64(map, (uint64_t)(uintptr_t)key, val);
}

#endif //KRETLIM_UEFI_BOOT_MAP_H