#include "list.h"
#include <stdio.h>

list_t *new_list(void) {
    list_t *list = malloc(sizeof(list_t));
    if (!list)
        return NULL;

    list->len = 0;
    // dummy head
    list->head = list->tail = malloc(sizeof(list_node_t));
    list->head->next = NULL;
    return list;
}

int list_push(list_t *list, void *data) {
    if (!data)
        return -1;

    list_node_t *new_node = malloc(sizeof(list_node_t));
    if (!new_node)
        return -1;

    new_node->data = data;
    list->tail->next = new_node;
    list->tail = list->tail->next;
    list->len++;
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

