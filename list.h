#ifndef LIST_H
#define LIST_H

#include <stdlib.h>
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
int list_push(list_t *list, void *data);
void *list_peek(list_t *list);
void *list_pop(list_t *list);
void list_free(list_t *list);

#endif
