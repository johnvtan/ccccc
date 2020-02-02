#include "list.h"
#include <stdio.h>
#include <assert.h>

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
    printf("tests pass\n");
    return 0;
}
