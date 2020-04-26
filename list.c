#include "list.h"
#include <stdio.h>

list_t *list_new(void) {
    list_t *list = malloc(sizeof(list_t));
    if (list_init(list) < 0)
        return NULL;
    return list;
}

int list_init(list_t *list) {
    if (!list)
        return -1;
    list->len = 0;
    list->head = list->tail = malloc(sizeof(list_node_t));
    list->head->next = NULL;
    return 0;
}

int list_push(list_t *list, void *data) {
    if (!data)
        return -1;

    list_node_t *new_node = malloc(sizeof(list_node_t));
    if (!new_node)
        return -1;

    new_node->data = data;
    new_node->next = NULL;
    list->tail->next = new_node;
    list->tail = list->tail->next;
    list->len++;
    return 0;
}

// appends l2 to the end of l1
// TODO better test this code too
int list_concat(list_t *l1, list_t *l2) {
    if (!l1 || !l2)
        return -1;

    // I have a dummy head, I think
    l1->tail->next = l2->head->next;
    if (l2->len)
        l1->tail = l2->tail;
    l1->len += l2->len;
    free(l2->head);
    free(l2);
    return 0;
}

// pops from front of list
void *list_pop(list_t *list) {
    if (!list)
        return NULL;
    if (list->len == 0)
        return NULL;
    assert(list->head->next);
    list_node_t *removed = list->head->next;
    void *ret_data = removed->data;
    list->head->next = removed->next;
    list->len--;
    if (list->len == 0)
        list->tail = NULL;

    free(removed);
    return ret_data;
}

void *list_peek(list_t *list) {
    if (!list)
        return NULL;
    if (!list->len)
        return NULL;
    assert(list->head->next);
    return list->head->next->data;
}

// free entire list, including the data pointers that are contained in it
void list_free(list_t *list) {
    if (!list)
        return;
    void *data;
    while ((data = list_pop(list))) {
        // assume it's heap allocated
        free(data);
    }
    assert(list->head->next == list->tail);
    free(list->head);
    free(list);
}

inline bool __list_iterate(list_node_t **node, void **data) {
    if (*node == NULL)
        return false;

    *data = (*node)->data;
    *node = (*node)->next;
    return true;
}
