#include "map.h"

#include <stdio.h>

map_t *map_new(void) {
    map_t *ret = malloc(sizeof(map_t)); 
    if (!ret)
        return NULL;
    ret->pairs = list_new();
    if (!ret->pairs)
        return NULL;
    return ret;
}

static pair_t *map_get_pair_from_key(map_t *map, string_t *key) {
    pair_t *pair;
    list_for_each(map->pairs, pair) {
        if (string_eq(key, pair->key) == 0) {
            return pair;
        }
    }
    return NULL;
}

void *map_get(map_t *map, string_t *key) {
    if (!map)
        return NULL;
    pair_t *pair = map_get_pair_from_key(map, key);
    return pair ? pair->value : NULL;
}

int map_set(map_t *map, string_t *key, void *value) {
    if (!map)
        return -1;

    // Overwrite the value if the key already exists in the map
    pair_t *pair = map_get_pair_from_key(map, key);
    if (pair) {
        pair->value = value;
        return 0;
    }
   
    pair = malloc(sizeof(pair_t));
    if (!pair)
        return -1;
    pair->key = key;
    pair->value = value;
    list_push(map->pairs, pair);
    return 0;
}

bool map_contains(map_t *map, string_t *key) {
    return map_get_pair_from_key(map, key) ? true : false;
}
