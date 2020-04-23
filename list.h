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
int list_init(list_t *list);
int list_push(list_t *list, void *data);
int list_concat(list_t *l1, list_t *l2);
void *list_peek(list_t *list);
void *list_pop(list_t *list);
void list_free(list_t *list);


#define list_first(list) (list->head->next)

// Gets the next node from the current pointer to data.
// Relies on the fact that data is the first element in the list_node_t struct,
// and will need to change if that ever changes.
#define next_node_from_data(data) (((list_node_t*)data)->next)

#define list_for_each(list, iterator)\
    for (iterator = list_first(list)->data;\
         iterator != NULL;\
         iterator = next_node_from_data(data) ? next_node_from_data(data)->data : NULL)

#define list_for_each_until(list, iterator, condition)\
    for (iterator = list_first(list)->data;\
         !(condition);\
         iterator = next_node_from_data(data) ? next_node_from_data(data)->data : NULL)

#endif
