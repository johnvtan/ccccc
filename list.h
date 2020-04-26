#ifndef LIST_H
#define LIST_H

#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

typedef struct  _list_node {
    void *data;
    struct _list_node *next;
} list_node_t;

typedef struct {
    list_node_t *head;
    list_node_t *tail;
    int len;
} list_t;

list_t *list_new(void);
int list_init(list_t *list);
int list_push(list_t *list, void *data);
int list_concat(list_t *l1, list_t *l2);
void *list_peek(list_t *list);
void *list_pop(list_t *list);
void list_free(list_t *list);

// Helper function to iterate over a list.
// Returns True if there's more data in the list, False otherwise.
bool __list_iterate(list_node_t **node, void **data);

#define list_first(list) (list->head->next)

#define list_for_each(list, data_ptr)\
    list_node_t *__node_iter = list_first(list);\
    while (__list_iterate(&__node_iter, (void**)&data_ptr))

#endif
