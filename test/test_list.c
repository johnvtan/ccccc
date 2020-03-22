#include "list.h"
#include <stdio.h>
#include <assert.h>

void test_list_concat(void) {
    printf("testing empty concat\n");
    list_t *list1 = list_new();
    list_t *list2 = list_new();
    int x = 3;
    list_push(list1, &x);
    assert(list1->len == 1);
    assert(list2->len == 0);

    list_concat(list1, list2);
    assert(list1->len == 1);
    assert(*(int*)list1->head->next->data == x);
}

void test_list_push(void) {
    printf("testing push\n");
    list_t list;
    list_init(&list);
    assert(list.len == 0);
    int x = 3;
    list_push(&list, &x);
    assert(list.len == 1);
    assert(*(int*)list.head->next->data == x);
}

void test_list_pop(void) {
    printf("testing pop\n");
    list_t list;
    list_init(&list);
    int x = 3;
    list_push(&list, &x);
    assert(&x == list_pop(&list)); 
    assert(list.len == 0);
}

int main(void) {
    test_list_push();
    test_list_pop();
    test_list_concat();
    printf("tests pass\n");
    return 0;
}
