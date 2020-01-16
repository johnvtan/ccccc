#ifndef MAP_H
#define MAP_H

#include "list.h"
#include "string.h"

/*
 * Maps are wrappers around the list type. Each node contains a pair, which maps a key to a value.
 */
typedef struct {
    string_t *key;
    void *value;
} pair_t;

typedef struct {
    list_t *pairs;
} map_t;

map_t *map_new(void);
void *map_get(map_t *map, string_t *key);
int map_set(map_t *map, string_t *key, void *value);

#endif
