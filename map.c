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

    for (list_node_t *curr = map->pairs->head->next; curr; curr = curr->next) {
        pair_t *pair = (pair_t*)curr->data;
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
    for (list_node_t *curr = map->pairs->head->next; curr; curr = curr->next) {
        pair_t *pair = (pair_t*)curr->data;
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
