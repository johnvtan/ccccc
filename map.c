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

void *map_get(map_t *map, string_t *key) {
    if (!map)
        return NULL;

    pair_t *iter;
    list_for_each(map->pairs, iter) {
        if (string_eq(key, pair->key) == 0) {
            return pair->value;
        }
    }

    return NULL;
}

int map_set(map_t *map, string_t *key, void *value) {
    if (!map)
        return -1;

    // Overwrite the value if the key already exists in the map
    pair_t *iter;
    list_for_each(map->pairs, iter) {
        if (string_eq(key, pair->key) == 0) {
            pair->value = value;
            return 0;
        }
    }
    
    pair_t *new = malloc(sizeof(pair_t));
    if (!new)
        return -1;
    new->key = key;
    new->value = value;
    list_push(map->pairs, new);
    return 0;
}
