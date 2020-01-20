#include "map.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>

void test_map_set_get_mult(void) {
    printf("test map set mult...");
    map_t *map = map_new();
    string_t *key1 = string_new();
    string_append(key1, "key1", 4);
    int val1 = 124535123;
    assert(map_set(map, key1, &val1) == 0);

    string_t *key2 = string_new();
    string_append(key2, "key2", 4);
    int val2 = 3341523;
    assert(map_set(map, key2, &val2) == 0);

    assert(*(int*)map_get(map, key1) == val1);
    assert(*(int*)map_get(map, key2) == val2);

    int val2_2 = 12314141;
    assert(map_set(map, key2, &val2_2) == 0);
    assert(*(int*)map_get(map, key2) != val2);
    assert(*(int*)map_get(map, key2) == val2_2);

    printf("OK\n");
}

void test_map_set_get(void) {
    printf("test map set...");
    map_t *map = map_new();
    string_t *key = string_new();
    string_append(key, "key", 3);
    int val = 124535123;
    assert(map_set(map, key, &val) == 0);
    assert(*(int*)map_get(map, key) == val);
    printf("OK\n");
}

void test_map_init(void) {
    printf("test map init...");
    map_t *map = map_new();
    assert(map->pairs != NULL);
    printf("OK\n");
}

int main(void) {
    test_map_init();
    test_map_set_get();
    test_map_set_get_mult();
    return 0;
}
