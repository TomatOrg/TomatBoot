#include "map.h"

#define MIN(x, y) ((x) <= (y) ? (x) : (y))
#define MAX(x, y) ((x) >= (y) ? (x) : (y))
#define CLAMP_MAX(x, max) MIN(x, max)
#define CLAMP_MIN(x, min) MAX(x, min)

uint64_t map_get_uint64_from_uint64(map_t *map, uint64_t key) {
    if (map->len == 0) {
        return 0;
    }
    // TODO: assert(IS_POW2(map->cap));
    size_t i = (size_t)hash_uint64(key);
    // TODO: assert(map->len < map->cap);
    for (;;) {
        i &= map->cap - 1;
        if (map->keys[i] == key) {
            return map->vals[i];
        } else if (!map->keys[i]) {
            return 0;
        }
        i++;
    }
    return 0;
}

void map_put_uint64_from_uint64(map_t *map, uint64_t key, uint64_t val);

void map_grow(map_t *map, size_t new_cap) {
    new_cap = CLAMP_MIN(new_cap, 16);
    map_t new_map = {
            .keys = calloc(new_cap, sizeof(uint64_t)),
            .vals = malloc(new_cap * sizeof(uint64_t)),
            .cap = new_cap,
    };
    for (size_t i = 0; i < map->cap; i++) {
        if (map->keys[i]) {
            map_put_uint64_from_uint64(&new_map, map->keys[i], map->vals[i]);
        }
    }
    free((void *)map->keys);
    free(map->vals);
    *map = new_map;
}

void map_put_uint64_from_uint64(map_t *map, uint64_t key, uint64_t val) {
    // TODO: assert(key);

    // if val is NULL will remove the entry
    if (!val) {
        if(map_get_uint64_from_uint64(map, key)) {
            map->len--;
        }
        return;
    }

    if (2*map->len >= map->cap) {
        map_grow(map, 2*map->cap);
    }
    // TODO: assert(2*map->len < map->cap);
    // TODO: assert(IS_POW2(map->cap));
    size_t i = (size_t)hash_uint64(key);
    for (;;) {
        i &= map->cap - 1;
        if (!map->keys[i]) {
            map->len++;
            map->keys[i] = key;
            map->vals[i] = val;
            return;
        } else if (map->keys[i] == key) {
            map->vals[i] = val;
            return;
        }
        i++;
    }
}